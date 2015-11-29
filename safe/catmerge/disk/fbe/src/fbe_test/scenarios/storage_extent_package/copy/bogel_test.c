
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * bogel_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains tests the copy validation job.  I.E. It creates
 *  conditions that should result in the copy request failing during validation.
 *  In other words the negative copy tests.
 *
 * HISTORY
 *  09/22/2014  Ron Proulx  - Created from diabolicaldiscdemon_test.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "sep_tests.h"
#include "sep_test_io.h"
#include "neit_utils.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "sep_event.h"
#include "sep_hook.h"
#include "sep_utils.h"
#include "fbe_database.h"
#include "fbe_spare.h"
#include "fbe_test_configurations.h"
#include "fbe_job_service.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"
#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "sep_test_background_ops.h"
#include "fbe/fbe_random.h"

char * bogel_short_desc = "This scenario will test copy validation (copy request should fail).";
char * bogel_long_desc = 
    "\n"
    "\n"
    "This test verifies all (3) flavors of Copy Operations: Proactive Copy, User Copy and User Copy To.\n"  
    "There are (2) basic types of Copy Operations: User Initiated and System Initiated\n\n"
    "\t 1) User Initiated Copy Operations:-\n"
    "\t\t - These scenarios will initiate a User Copy operation for specific drive using fbe_cli command (PS).\n"
    "\t\t - It will send the control packet to Config Service to start the Proactive Copy operation.\n"
    "\t\t - It will verify that Drive Sparing Service receives Proactive Spare Swap-In request using fbe_cli command.\n"
    "\t\t - It will verify that Drive Sparing Service has found appropriate Proactive Spare for the swap-in.\n"
    "\t\t - It will verify that VD object starts actual Proactive copy operation.\n"
    "\t\t - It will verify that it updates checkpoint regularly on Proactive Spare drive for every chunk it copies.\n"
    "\t\t - It will also verify that Proactive Copy operation is performed in LBA order (Decision might change in future).\n"
    "\t\t - It will verify that at the end of Proactive Copy completion VD object sends notification to Drive Swa service.\n" 
    "\t\t - It will verify that at the end of proactive copy Drive Swap service will transition to Hot Sparing by removing\n"
    "\t\t   edge between VD object and PVD object for Proactive Source drive.\n\n"
    "\t 2) System Initiated Copy Operations:-\n"
    "\t\t - This scenario will insert the specific I/O errors during write to specific drives using NEIT tool.\n"
    "\t\t - After insertion of errors it will verify that EOL attribute is propogated till RAID object.\n"
    "\t\t - It will verify that RAID object has requested for Proactive Sparing operation to Drive Sparing Service.\n"
    "\t\t - This scenario will do the all other verification as user driven Proactive Copy operation does.\n"
    "\n"
    "Dependencies:\n\n"
    "\t - Drive Sparing service must provide the policy to select the Proactive Spare.\n"
    "\t - Proactive Copy operation must be serialized across the SPs\n."
    "\t - VD object gets Private Data Library interface to update the metadata (checkpoint) information.\n"
    "\t - Drive Spare service must provide the handling to update the required configuration database based on requested operation.\n"
    "\n"
    "Starting Config:\n"
    "\n"
    "\t [PP] armada board\n"
    "\t [PP] SAS PMC port\n"
    "\t [PP] viper enclosure\n"
    "\t [PP] 5 SAS drives\n"
    "\t [PP] 5 logical drives\n"
    "\t [SEP] 5 provisioned drives\n"
    "\t [SEP] 5 virtual drives\n"
    "\t [SEP] raid group (raid 0), (raid 1), (raid 5), (raid 6), (raid 1/0) (Note: Different set of drives will be used for different RAID group)\n"
    "\t [SEP] 3 luns\n"
    "\n"
    "Test Sequence:\n\n"
    "1) User driven Proactive Copy operation(fbe_cli):-\n"
    "\n"
    "\tSTEP 1: Bring up the initial topology.\n"
    "\t\t - Create and verify the initial physical package config.\n"
    "\t\t - Create provisioned drives and attach edges to logical drives.\n"
    "\t\t - Create virtual drives and attach edges to provisioned drives.\n"
    "\t\t - Create different RAID group with different RAID type on different set of drives and attach edge with virtual drive (RAID 0, RAID 5 etc).\n"
    "\t\t - RAID group attributes:  width: 5 drives  capacity: 0x6000 blocks.\n"
    "\t\t - Create and attach 3 LUNs to the raid 5 object.\n"
    "\t\t - Each LUN will have capacity of 0x2000 blocks and element size of 128.\n"
    "\n"
    "\tSTEP 2: Start the Proactive Copy operation using fbe api or fbe_cli.\n"
    "\t\t - Issue the command through fbe api/fbe_cli to start the Proactive Copy operation.\n"
    "\t\t - Verify that Drive Spare Service has received request for the Proactive Spare swap-in.\n"
    "\n"
    "\tSTEP 3: Make sure that configured Proactive Spare is available for swap-in.\n"
    "\t\t - Verify that Drive Swap Service selects appropriate Proactive Spare and create edge between VD object and Proactive Spare PVD object.\n"
    "\t\t - Verifies that newly created edge with Proactive Spare drive has PS (Proactive Spare) flag set.\n"
    "\n"
    "\tSTEP 4: Verify the progress of the actual Proactive Copy operation.\n"
    "\t\t - Verify that VD object starts actual Proactive Copy based on LBA order.\n"
    "\t\t - Verifies that VD object updates checkpoint metadata using PDL (Private Data Library) regularly on Proactive Spare drive.\n"
    "\t\t - Verifies that VD object sends notification to Drive Swap Service when Proactive Copy operation completes.\n"
    "\n"
    "\tSTEP 5: Verify the transition to Hot Sparing gets done at the end of Proactive Copy operation.\n"
    "\t\t - Verifies that Drive Swap Service transition to Hot Sparing by removing edge with Proactive Source drive.\n"
    "\t\t - Verifies that Drive Swap Service updates required configuration database.\n"
    "\n"
    "2) Specific I/O error triggers Proactive Copy operation:-\n"
    "\n"
    "\tSTEP 1: Bring up the initial topology.\n"
    "\t\t - Create and verify the initial physical package config.\n"
    "\t\t - Create provisioned drives and attach edges to logical drives.\n"
    "\t\t - Create virtual drives and attach edges to provisioned drives.\n"
    "\t\t - Create different RAID group with different RAID type on different set of drives and attach edge with virtual drive (RAID 0, RAID 5 etc).\n"
    "\t\t - RAID group attributes:  width: 5 drives  capacity: 0x6000 blocks.\n"
    "\t\t - Create and attach 3 LUNs to the raid 5 object.\n"
    "\t\t - Each LUN will have capacity of 0x2000 blocks and element size of 128.\n"
    "\n"
    "\tSTEP 2: Start the Proactive Copy operation by inserting errors using NEIT tool.\n"
    "\t\t - Issue the NEIT commands to insert the errors which triggers EOL for the specificf drive.\n"
    "\t\t - Verify that EOL attribute is propogated till RAID object.\n"
    "\t\t - Verify that RAID object in response sends request to Drive Spare Service to start Proactive Spare swap-in operation.\n"
    "\n"
    "\tSTEP 3: Make sure that configured Proactive Spare is available for swap-in.\n"
    "\t\t - Verify that Drive Swap Service selects appropriate Proactive Spare and create edge between VD object and Proactive Spare PVD object.\n"
    "\t\t - Verifies that newly created edge with Proactive Spare drive has PS (Proactive Spare) flag set.\n"
    "\n"
    "\tSTEP 4: Verify the progress of the actual Proactive Copy operation.\n"
    "\t\t - Verify that VD object starts actual Proactive Copy based on LBA order.\n"
    "\t\t - Verifies that VD object updates checkpoint metadata using PDL (Private Data Library) regularly on Proactive Spare drive.\n"
    "\t\t - Verifies that VD object sends notification to Drive Swap Service when Proactive Copy operation completes.\n"
    "\n"
    "\tSTEP 5: Verify the transition to Hot Sparing gets done at the end of Proactive Copy operation.\n"
    "\t\t - Verifies that Drive Swap Service transition to Hot Sparing by removing edge with Proactive Source drive.\n"
    "\t\t - Verifies that Drive Swap Service updates required configuration database.\n"
    "\n"
    "Description last updated: 04/11/2013.\n"
    "\n"
    "\n";


/* The number of LUNs in our raid group.
 */
#define BOGEL_TEST_LUNS_PER_RAID_GROUP                  3 

/* Element size of our LUNs.
 */
#define BOGEL_TEST_ELEMENT_SIZE                       128

/* Maximum number of hot spares. */
#define BOGEL_TEST_HS_COUNT                            12

/* Maximum number of block we will test with.
 */
#define BOGEL_MAX_IO_SIZE_BLOCKS                        1024

/* Max number of raid groups we will test with.
 */
#define BOGEL_TEST_RAID_GROUP_COUNT                      1
#define BOGEL_TEST_MAX_RETRIES                         300  // retry count - number of times to loop
#define BOGEL_TEST_RETRY_TIME                         1000  // wait time in ms, in between retries 
#define BOGEL_TEST_EVENT_LOG_WAIT_TIME                 100
#define BOGEL_TEST_SWAP_WAIT_TIME_MS                 60000  // Swap-In and Swap-Out wait time is (60) seconds    
#define BOGEL_TEST_ZERO_DISK_TIME_MS                120000  // Wait for disk zeroing to complete  

/*********************************************************************
 * @def     BOGEL_TEST_CHUNKS_PER_LUN
 * 
 * @note    This needs to be fairly big so that the copy operations runs 
 *          for some time. 
 *********************************************************************/
#define BOGEL_TEST_CHUNKS_PER_LUN                       10

/*********************************************************************
 * @def     BOGEL_RAID_GROUP_DEFAULT_DEBUG_FLAGS
 * 
 * @note    Default raid group debug flags (if enabled) for virtual 
 *          drive object specified. 
 *********************************************************************/
#define BOGEL_RAID_GROUP_DEFAULT_DEBUG_FLAGS            (FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR)

/*********************************************************************
 * @def     BOGEL_RAID_LIBRARY_DEFAULT_DEBUG_FLAGS
 * 
 * @note    Default raid group library flags (if enabled) for virtual 
 *          drive object specified. 
 *********************************************************************/
#define BOGEL_RAID_LIBRARY_DEFAULT_DEBUG_FLAGS          (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING             | \
                                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING        | \
                                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_PAGED_MD_WRITE_DATA     )

/*!*******************************************************************
 * @var     bogel_raid_group_debug_flags
 *********************************************************************
 * @brief   These are the debugs flags to add if enabled.
 *
 *********************************************************************/          /*! @note Re-enable flags if needed. */
fbe_raid_group_debug_flags_t    bogel_raid_group_debug_flags  = 0; /* BOGEL_RAID_GROUP_DEFAULT_DEBUG_FLAGS;*/
                                                                        
/*!*******************************************************************
 * @var     bogel_raid_library_debug_flags
 *********************************************************************
 * @brief   These are the debugs flags to add if enabled.
 *
 *********************************************************************/           /*! @note Re-enable flags if needed. */
fbe_raid_library_debug_flags_t  bogel_raid_library_debug_flags = 0; /* BOGEL_RAID_LIBRARY_DEFAULT_DEBUG_FLAGS; */

/* RDGEN test context. */
static fbe_api_rdgen_context_t  bogel_test_context[BOGEL_TEST_LUNS_PER_RAID_GROUP * BOGEL_TEST_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @var bogel_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_lba_t bogel_user_space_lba_to_inject_to          =   0x0ULL; /* lba 0 on first LUN. Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var bogel_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_block_count_t bogel_user_space_blocks_to_inject  = 0x7C0ULL; /* Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var bogel_default_offset
 *********************************************************************
 * @brief This is the default user space offset.
 *
 *********************************************************************/
static fbe_lba_t bogel_default_offset                     = 0x10000ULL; /* Offset used maybe different */

/*!***************************************************************************
 * @var bogel_b_is_DE542_fixed
 *****************************************************************************
 * @brief DE542 - Due to the fact that protocol error are injected on the
 *                way to the disk, the data is never sent to or read from
 *                the disk.  Therefore until this defect is fixed b_start_io
 *                MUST be True.
 *
 * @todo  Fix defect DE542.
 *
 *****************************************************************************/
static fbe_bool_t bogel_b_is_DE542_fixed                  = FBE_FALSE;

/*!***************************************************************************
 * @var     bogel_discovered_drive_locations
 *****************************************************************************
 * @brief   For each test iteration (only (1) raid group is tested per 
 *          iteration) we discover all the disk information and save it here.
 *          There is no need to rediscover the the disk for each test.
 *
 * @todo  Fix defect DE542.
 *
 *****************************************************************************/
static fbe_test_discovered_drive_locations_t bogel_discovered_drive_locations = { 0 };

/*!*******************************************************************
 * @var bogel_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t bogel_raid_group_config[BOGEL_TEST_RAID_GROUP_COUNT + 1] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG,         0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT,  FBE_TEST_RG_CONFIG_RANDOM_TAG,    520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*! @note PLEASE ONLY PUT NEGATIVE (I.E. COPY SHOULD FAIL) COPY TEST HERE*/
typedef enum bogel_test_case_e {    
    BOGEL_USER_COPY_WITH_FAILURE_FIRST = 0,
    BOGEL_USER_COPY_ON_DEGRADED_RAID_GROUP,
    BOGEL_USER_COPY_ON_BROKEN_RAID_GROUP,
    BOGEL_COPY_ALREADY_IN_PROGRESS,
    BOGEL_SIMPLE_PROACTIVE_COPY_SECOND, 
    BOGEL_NUMBER_OF_ENABLED_TEST_CASES,

    BOGEL_FIRST_DISABLED_TEST_CASE = BOGEL_NUMBER_OF_ENABLED_TEST_CASES,   

    BOGEL_NUMBER_OF_TEST_CASES,
} bogel_test_case_t;

typedef struct bogel_test_information_s
{
    fbe_bool_t          is_user_initiated_copy;
    fbe_u32_t           drive_pos;
    bogel_test_case_t   test_case;
    fbe_u32_t           number_of_spare;
} bogel_test_information_t;


bogel_test_information_t bogel_test_info[BOGEL_NUMBER_OF_TEST_CASES] =
{/* user initiated copy?    drive position  test case                                                   Number of spares required*/
    {FBE_TRUE,               1,             BOGEL_USER_COPY_WITH_FAILURE_FIRST,                 1}, 
    {FBE_FALSE,              0,             BOGEL_USER_COPY_ON_DEGRADED_RAID_GROUP,             0}, 
    {FBE_FALSE,              1,             BOGEL_USER_COPY_ON_BROKEN_RAID_GROUP,               0}, 
    {FBE_TRUE,               1,             BOGEL_COPY_ALREADY_IN_PROGRESS,                     1},
    {FBE_TRUE,               0,             BOGEL_SIMPLE_PROACTIVE_COPY_SECOND,                 1},
};

/*****************************
 *   LOCAL FUNCTION PROTOTYPES
 *****************************/
static void bogel_io_error_trigger_proactive_sparing(fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_u32_t        drive_pos,
                                                          fbe_edge_index_t source_edge_index);

static void bogel_io_error_fault_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t        drive_pos,
                                                   fbe_edge_index_t source_edge_index);

/* Wait for the proactive spare to swap-in. */
void bogel_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                        fbe_edge_index_t spare_edge_index,
                                                        fbe_test_raid_group_disk_set_t * spare_location_p);

/* It waits for the proactive copy to complete. */
void bogel_wait_for_proactive_copy_completion(fbe_object_id_t vd_object_id);

/* Wait for the copy job to complete. */
static fbe_status_t bogel_wait_for_copy_job_to_complete(fbe_object_id_t vd_object_id);

/* wait for the source edge to swap-out. */
void bogel_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
                                                                    fbe_edge_index_t source_edge_index);

/* wait for the destination drive to swap-out. */
static void bogel_wait_for_destination_drive_swap_out(fbe_object_id_t vd_object_id,
                                                                fbe_edge_index_t dest_edge_index);

void bogel_load_physical_config(fbe_test_rg_configuration_t *rg_config_p,
                                 fbe_u32_t num_raid_groups);
void bogel_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);

/* Wait for the previous copy test to complete */
static fbe_status_t bogel_wait_for_in_progress_request_to_complete(fbe_object_id_t vd_object_id,
                                                                                 fbe_u32_t timeout_ms);

static fbe_status_t bogel_test_remove_hotspares_from_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                               fbe_u32_t num_of_spares_required);

static fbe_status_t bogel_test_add_hotspares_to_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                          fbe_u32_t num_of_spares_required);

/* test user copy with expected failure senario */
static void bogel_test_user_copy_with_expected_failure(fbe_test_rg_configuration_t * rg_config_p, 
                                                                     fbe_u32_t  drive_pos);

/* test attempts to start a user copy on a broken raid group */
static fbe_status_t bogel_test_user_copy_on_broken_raid_group(fbe_test_rg_configuration_t *rg_config_p);

/* test attempt to start user copy on degraded raid group.*/
static fbe_status_t bogel_test_user_copy_on_degraded_raid_group(fbe_test_rg_configuration_t *rg_config_p);

/* test attempt to start a second copy on a raid group with a copy in progress */
static void bogel_test_second_copy_on_same_raid_group(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_bool_t b_user_copy);

/* test attempt to start a second copy on a virtual drive with a copy in progress */
static void bogel_test_second_copy_on_same_virtual_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                                       fbe_u32_t  drive_pos,
                                                                       fbe_bool_t b_user_copy);

static void bogel_test_debug_fail_destination_during_swap(fbe_test_rg_configuration_t *rg_config_p,
                                                                        fbe_u32_t  drive_pos);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void bogel_set_trace_level(fbe_trace_type_t trace_type, fbe_u32_t id, fbe_trace_level_t level)
{
    fbe_api_trace_level_control_t level_control;
    fbe_status_t status;

    level_control.trace_type = trace_type;
    level_control.fbe_id = id;
    level_control.trace_level = level;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}

void bogel_cleanup(void)
{
    fbe_test_common_cleanup_notifications(FBE_FALSE /* This is a single SP test*/);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 *          bogel_get_vd_and_rg_object_ids()
 ******************************************************************************
 *
 * @brief   Handles RAID-10 raid groups.  Get the rg_object_id associated with
 *          mirror raid group for the requested raid group position.
 *
 * @param   original_rg_object_id - Top level raid group object id
 * @param   position - Position in top level raid group desired
 * @param   vd_object_id_p - Pointer to virtual drive object id to update
 * @param   rg_object_id_p - Pointer to raid group object id to update
 *
 * @return  status 
 *
 * @author
 *  06/13/2014  Ron Proulx  - Created
 ******************************************************************************/
static fbe_status_t bogel_get_vd_and_rg_object_ids(fbe_object_id_t original_rg_object_id,
                                                                 fbe_u32_t position,
                                                                 fbe_object_id_t *vd_object_id_p,
                                                                 fbe_object_id_t *rg_object_id_p)
{
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_raid_group_get_info_t get_info;
    fbe_u32_t  current_time = 0;

    MUT_ASSERT_TRUE(position < FBE_MAX_DOWNSTREAM_OBJECTS);

    while (current_time < BOGEL_TEST_SWAP_WAIT_TIME_MS)
    {
        status = fbe_api_raid_group_get_info(original_rg_object_id, &get_info, FBE_PACKET_FLAG_EXTERNAL);

        if (status == FBE_STATUS_OK)
        {
            break;
        }
        current_time = current_time + BOGEL_TEST_EVENT_LOG_WAIT_TIME;
        fbe_api_sleep(BOGEL_TEST_EVENT_LOG_WAIT_TIME);
    } 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* initialize the vd object id as invalid. */
    *vd_object_id_p = FBE_OBJECT_ID_INVALID;

    if (get_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_u32_t mirror_index;
        fbe_u32_t position_in_mirror;

        fbe_test_sep_util_get_ds_object_list(original_rg_object_id, &downstream_object_list);
        /* For a raid 10 the downstream object list has the IDs of the mirrors. 
         * We need to map the position to specific mirror (mirror index) and a position in the mirror (position_in_mirror).
         */
        mirror_index = position / 2;
        position_in_mirror = position % 2;
        *rg_object_id_p = downstream_object_list.downstream_object_list[mirror_index];
        fbe_test_sep_util_get_ds_object_list(downstream_object_list.downstream_object_list[mirror_index], 
                                             &downstream_object_list);
        *vd_object_id_p = downstream_object_list.downstream_object_list[position_in_mirror];
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *vd_object_id_p);
    }
    else
    {
        /* Get the virtual drive object-id for the specified position. */
        fbe_test_sep_util_get_ds_object_list(original_rg_object_id, &downstream_object_list);
        *rg_object_id_p = original_rg_object_id;
        *vd_object_id_p = downstream_object_list.downstream_object_list[position];
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, *vd_object_id_p);
    }
    return FBE_STATUS_OK;
}
/***************************************************** 
 * end bogel_get_vd_and_rg_object_ids()
 *****************************************************/

/*!****************************************************************************
 *          bogel_remove_drive()
 ******************************************************************************
 *
 * @brief   It is used to remove the specific drive in raid group.
 *
 * @param   rg_config_p - Pointer to raid group
 * @param   position - Position to remove
 * @param   b_wait_for_pvd_fail - Wait for the provision drive to goto the 
 *              Failed state before returning.
 * @param   b_wait_for_vd_fail - Wait for the virtual drive to goto the 
 *              Failed state before returning.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void bogel_remove_drive(fbe_test_rg_configuration_t *rg_config_p,
                                             fbe_u32_t position,
                                             fbe_bool_t b_wait_for_pvd_fail,
                                             fbe_bool_t b_wait_for_vd_fail)
{
    fbe_status_t                        status;
    fbe_object_id_t                     pvd_object_id;
    fbe_object_id_t                     vd_object_id;
    fbe_object_id_t                     rg_object_id;

    
    /* get the vd object id. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    /* get the pvd object id before we remove the drive. */
    shaggy_test_get_pvd_object_id_by_position(rg_config_p, position, &pvd_object_id);
  
    mut_printf(MUT_LOG_TEST_STATUS, "%s:remove the drive port: %d encl: %d slot: %d",
                __FUNCTION__,
               rg_config_p->rg_disk_set[position].bus,
               rg_config_p->rg_disk_set[position].enclosure,
               rg_config_p->rg_disk_set[position].slot);

    /* Remove the drive
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_add_removed_position(rg_config_p, position); 
        /* remove the drive. */
        status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position].bus,
                                             rg_config_p->rg_disk_set[position].enclosure,
                                             rg_config_p->rg_disk_set[position].slot,
                                             &rg_config_p->drive_handle[position]); 
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_sim_transport_connection_target_t   local_sp;
            fbe_sim_transport_connection_target_t   peer_sp;

            /*  Get the ID of the local SP. */
            local_sp = fbe_api_sim_transport_get_target_server();

            /*  Get the ID of the peer SP. */
            if (FBE_SIM_SP_A == local_sp)
            {
                peer_sp = FBE_SIM_SP_B;
            }
            else
            {
                peer_sp = FBE_SIM_SP_A;
            }

            /*  Set the target server to the peer and reinsert the drive there. */
            fbe_api_sim_transport_set_target_server(peer_sp);

            /* remove the drive. */
            status = fbe_test_pp_util_pull_drive(rg_config_p->rg_disk_set[position].bus,
                                             rg_config_p->rg_disk_set[position].enclosure,
                                             rg_config_p->rg_disk_set[position].slot,
                                             &rg_config_p->peer_drive_handle[position]);

            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        /* remove the drive on actual hardware. */
        status = fbe_test_pp_util_pull_drive_hw(rg_config_p->rg_disk_set[position].bus,
                                   rg_config_p->rg_disk_set[position].enclosure,
                                   rg_config_p->rg_disk_set[position].slot);

     }
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If requested, verify that the pvd and vd objects are in the FAIL state. 
     */
    if (b_wait_for_pvd_fail == FBE_TRUE)
    {
        shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    }
    if (b_wait_for_vd_fail == FBE_TRUE)
    {
        shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);
    }

    /* If simulation and dualsp wait on passive
     */
    if (fbe_test_util_is_simulation()            &&
        fbe_test_sep_util_get_dualsp_test_mode()    )
    {
        fbe_sim_transport_connection_target_t   local_sp;
        fbe_sim_transport_connection_target_t   peer_sp;

        /*  Get the ID of the local SP. */  
        local_sp = fbe_api_sim_transport_get_target_server();

        /*  Get the ID of the peer SP. */
        if (FBE_SIM_SP_A == local_sp)
        {
            peer_sp = FBE_SIM_SP_B;
        }
        else
        {
            peer_sp = FBE_SIM_SP_A;
        }

        /*  Set the target server to the peer and reinsert the drive there. */
        fbe_api_sim_transport_set_target_server(peer_sp);

        /* If requested, verify that the pvd and vd objects are in the FAIL state. 
         */
        if (b_wait_for_pvd_fail == FBE_TRUE)
        {
            shaggy_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);
        }
        if (b_wait_for_vd_fail == FBE_TRUE)
        {
            shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);
        }

        /*  Set the target server back to the local SP. */
        fbe_api_sim_transport_set_target_server(local_sp);
    }

    return;
}
/******************************************************************************
 * end bogel_remove_drive()
 ******************************************************************************/

/*!****************************************************************************
 * bogel_remove_small_drive()
 ******************************************************************************
 * @brief
 *  Remove the drive that is purposefully too small.
 *
 * @param   rg_config_p - Pointer to raid group to remove small drive.
 *
 * @return status
 *
 * @author
 *  07/10/2012  Ron Proulx  - Created
 ******************************************************************************/
static fbe_status_t bogel_remove_small_drive(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       retry_count = 0;
    fbe_u32_t                       max_retries = BOGEL_TEST_MAX_RETRIES;
    fbe_object_id_t                 pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    
    /*! @note Need to remove `small' drive so that it doesn't get used as part
     *        of the raid group.
     */
    /* Need to wait until we can find the physical drive object.
     */
    while ((pdo_object_id == FBE_OBJECT_ID_INVALID) &&
           (retry_count   <  max_retries)              )

    {
        /* Get the pdo object (might need to wait for it)
         */
        status = fbe_api_get_physical_drive_object_id_by_location(1, 0, 7, &pdo_object_id);

        /* It may take some time for the drive to be detectable.
         */
        if ((status == FBE_STATUS_OK)                &&
            (pdo_object_id != FBE_OBJECT_ID_INVALID)    )
        {
            break;
        }

        /* Sleep a short time.
         */
        fbe_api_sleep(BOGEL_TEST_RETRY_TIME);
        retry_count++;
    }
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pdo_object_id);

    /* Need to wait until we can find the provision drive
     */
    while ((pvd_object_id == FBE_OBJECT_ID_INVALID) &&
           (retry_count   <  max_retries)              )

    {
        /* Get the pdo object (might need to wait for it)
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(1, 0, 7, &pvd_object_id);

        /* It may take some time for the drive to be detectable.
         */
        if ((status == FBE_STATUS_OK)                &&
            (pvd_object_id != FBE_OBJECT_ID_INVALID)    )
        {
            break;
        }

        /* Sleep a short time.
         */
        fbe_api_sleep(BOGEL_TEST_RETRY_TIME);
        retry_count++;
    }
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);

    /* Wait until the pvd is ready before we pull the pdo.
     */
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 30000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now pull the drive 
     */
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pdo_object_id);
    status = fbe_test_pp_util_pull_drive(1, 0, 7,
                                         &rg_config_p->drive_handle[0]);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now validate that the pdo is destroyed.
     */
    status = fbe_api_wait_till_object_is_destroyed(pdo_object_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

    return status; 
}
/**********************************************
 * end bogel_remove_small_drive()
 **********************************************/

/*!****************************************************************************
 *          bogel_set_unused_drive_info()
 ******************************************************************************
 *
 * @brief   This function is used to set the unused drive info in the raid group
 *          structure.
 *
 * @param   rg_config_p - Pointer to raid group test configuration to populate
 * @param   unused_drive_count - Array of unused drive counters
 * @param   drive_locations_p - Pointer to unused drive locations
 * @param   disk_to_skip - Pointer to disk that shouldn't be added to the unused
 *              array.
 * @param   num_of_spares_required - How many spares must be discovered
 *
 * @return  None
 *
 * @author
 *  10/19/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void bogel_set_unused_drive_info(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_u32_t unused_drive_count[][FBE_DRIVE_TYPE_LAST],
                                                      fbe_test_discovered_drive_locations_t *drive_locations_p,
                                                      fbe_test_raid_group_disk_set_t *disk_to_skip_p,
                                                      fbe_u32_t num_of_spares_required)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       no_of_entries = FBE_RAID_MAX_DISK_ARRAY_WIDTH;
    fbe_test_block_size_t           block_size;
    fbe_test_raid_group_disk_set_t *disk_set_p = NULL;
    fbe_drive_type_t                drive_type = rg_config_p->drive_type;
    fbe_u32_t                       unused_drive_cnt = 0;
    fbe_u32_t                       num_drives_to_check = 0;
    fbe_u32_t                       unused_drive_index = 0;
    fbe_u32_t                       unused_disk_set_index = 0;
    fbe_bool_t                      b_is_healthy_spare = FBE_FALSE;
    
    /*  We find the unused, `healthy' drives to use as spares.
     */  
    if (fbe_test_rg_config_is_enabled(rg_config_p))
    {
        /* Get the block size to select from.
         */
        status = fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Calculate index of unused drive.
         */
        unused_drive_cnt = unused_drive_count[block_size][drive_type];
        MUT_ASSERT_TRUE(unused_drive_cnt >= num_of_spares_required);
        num_drives_to_check = (disk_to_skip_p != NULL) ? (unused_drive_cnt + 1) : unused_drive_cnt;

        /*Initialise the array with zero values
         */
        fbe_zero_memory(rg_config_p->unused_disk_set,
                        no_of_entries * sizeof(fbe_test_raid_group_disk_set_t));

        /* Get the pointer of the first available drive
         */
        disk_set_p = &drive_locations_p->disk_set[block_size][drive_type][0];

        /* For all available drives.
         */
        for (unused_drive_index = 0; unused_drive_index < num_drives_to_check; unused_drive_index++)
        {
            /* Validate that the drive is a healthy spare.
             */
            b_is_healthy_spare = fbe_test_sep_is_spare_drive_healthy(disk_set_p);
            MUT_ASSERT_TRUE(b_is_healthy_spare == FBE_TRUE);

            /* If this is not the `skip' drive, add it to the unused array.
             */
            if ((disk_to_skip_p == NULL)                                   ||
                ((disk_set_p->bus != disk_to_skip_p->bus)             ||
                 (disk_set_p->enclosure != disk_to_skip_p->enclosure) ||
                 (disk_set_p->slot != disk_to_skip_p->slot)              )     )
            {
                if (unused_disk_set_index < no_of_entries)
                {
                    rg_config_p->unused_disk_set[unused_disk_set_index] = *disk_set_p;
                    unused_disk_set_index++;
                }
            }

            /* Goto next drive
             */
            disk_set_p++;
        }
    }

    /* Validate that we found sufficient spare drives.
     */
    MUT_ASSERT_TRUE(unused_disk_set_index >= num_of_spares_required);
    return;
}
/************************************************** 
 * end bogel_set_unused_drive_info()
 **************************************************/

/*!***************************************************************************
 *          bogel_enable_debug_flags_for_virtual_drive_object()
 *****************************************************************************
 *
 * @brief   Enables pre-defined raid group and/or raid library debug flags
 *          for the virtual drive object specified.
 *
 * @param   vd_object_id - Virtual drive object id to enable debug flags for
 * @param   b_enable_raid_group_flags - FBE_TRUE - Enable the raid group debug
 *              flags for this raid group.
 * @param   b_enable_raid_library_flags - FBE_TRUE - Enable raid library debug
 *              flags for this   
 *
 * @return  None
 *
 * @author
 *  03/29/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void bogel_enable_debug_flags_for_virtual_drive_object(fbe_object_id_t vd_object_id,
                                                                            fbe_bool_t b_enable_raid_group_flags,
                                                                            fbe_bool_t b_enable_raid_library_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_debug_flags_t            raid_group_debug_flags = 0;
    fbe_raid_library_debug_flags_t          raid_library_debug_flags = 0;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;

    /* Determine active SP for copy.
     */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Determine the active SP
     */
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the existing debug flags
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If the raid group flags for the associated virtual drive are to be
     * enabled, enable then.
     */
    if (b_enable_raid_group_flags)
    {
        fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", (fbe_u32_t *)&raid_group_debug_flags);
        status = fbe_api_raid_group_set_group_debug_flags(vd_object_id, 
                                                          (raid_group_debug_flags | bogel_raid_group_debug_flags));
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If the raid group flags for the associated virtual drive are to be
     * enabled, enable then.
     */
    if (b_enable_raid_library_flags)
    {
        fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", (fbe_u32_t *)&raid_library_debug_flags);
        status = fbe_api_raid_group_set_library_debug_flags(vd_object_id, 
                                                            (raid_library_debug_flags | bogel_raid_library_debug_flags));

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    
   /* If either value is set trace the flags
    */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s vd obj: 0x%x set raid group flags: 0x%x library flags: 0x%x == ", 
               __FUNCTION__, vd_object_id,
               (b_enable_raid_group_flags) ? (raid_group_debug_flags | bogel_raid_group_debug_flags) : raid_group_debug_flags,
               (b_enable_raid_library_flags) ? (raid_library_debug_flags | bogel_raid_library_debug_flags) : b_enable_raid_library_flags);
   
    return;
}
/***********************************************************************
 * end bogel_enable_debug_flags_for_virtual_drive_object()
 ***********************************************************************/

/*!***************************************************************************
 *          bogel_disable_debug_flags_for_virtual_drive_object()
 *****************************************************************************
 *
 * @brief   Disables pre-defined raid group and/or raid library debug flags
 *          for the virtual drive object specified.
 *
 * @param   vd_object_id - Virtual drive object id to enable debug flags for
 * @param   b_disable_raid_group_flags - FBE_TRUE - Disable the raid group debug
 *              flags for this raid group.
 * @param   b_disable_raid_library_flags - FBE_TRUE - Disable raid library debug
 *              flags for this 
 * @param   b_clear_flags - FBE_TRUE - Set flags to 0 (not the command line value)  
 *
 * @return  None
 *
 * @author
 *  03/29/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void bogel_disable_debug_flags_for_virtual_drive_object(fbe_object_id_t vd_object_id,
                                                                             fbe_bool_t b_disable_raid_group_flags,
                                                                             fbe_bool_t b_disable_raid_library_flags,
                                                                             fbe_bool_t b_clear_flags)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_debug_flags_t            raid_group_debug_flags = 0;
    fbe_raid_library_debug_flags_t          raid_library_debug_flags = 0;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;

    /* Determine active SP for copy.
     */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Determine the active SP
     */
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the existing debug flags
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If the raid group flags for the associated virtual drive are to be
     * disabled, disable then.
     */
    if (b_disable_raid_group_flags)
    {
        fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", (fbe_u32_t *)&raid_group_debug_flags);
        if (b_clear_flags)
        {
            status = fbe_api_raid_group_set_group_debug_flags(vd_object_id, 0);
        }
        else
        {
            status = fbe_api_raid_group_set_group_debug_flags(vd_object_id, raid_group_debug_flags);
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If the raid group flags for the associated virtual drive are to be
     * disabled, disable then.
     */
    if (b_disable_raid_library_flags)
    {

        fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", (fbe_u32_t *)&raid_library_debug_flags);
        if (b_clear_flags)
        {
            status = fbe_api_raid_group_set_library_debug_flags(vd_object_id, 0);
        }
        else
        {
            status = fbe_api_raid_group_set_library_debug_flags(vd_object_id, raid_library_debug_flags);
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    
   /* If either value is set trace the flags
    */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return;
}
/************************************************************************
 * end bogel_disable_debug_flags_for_virtual_drive_object()
 ************************************************************************/

/*!****************************************************************************
 * bogel_test_config_hotspares()
 ******************************************************************************
 * @brief
 *  This test is used to init the hs config
 *  .
 * @param   current_rg_config_p - Pointer to raid group test configuration for this 
 *              iteration.
 * @param   num_of_spares_required - How many spares must be discovered
 *
 * @return FBE_STATUS.
 *
 * @author
 *  10/19/2012  Ron Proulx  - Updated.
 ******************************************************************************/
static fbe_status_t bogel_test_config_hotspares(fbe_test_rg_configuration_t *current_rg_config_p,  
                                                              fbe_u32_t num_of_spares_required)                
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_rg_configuration_t    *rg_config_p = &bogel_raid_group_config[0];
    fbe_test_hs_configuration_t     hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t                       unused_drive_index;
    fbe_u32_t                       hs_index = 0;
    fbe_test_discovered_drive_locations_t unused_pvd_locations;
    fbe_test_raid_group_disk_set_t  disk_to_skip = {1, 0, 7};
    fbe_u32_t                       local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
  
    /*! @note Each test now `cleans up' (clears EOL, re-inserts removed drives 
     *        etc) but the size of the `unused disks' array is still
     *        FBE_RAID_MAX_DISK_ARRAY_WIDTH.  Thus the `num_of_spares_required'
     *        cannot exceed that.  If more tests are required the method for
     *        locating the spare drive to configure as FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE
     *        will need to be modified.
     */
    MUT_ASSERT_TRUE(num_of_spares_required <= FBE_RAID_MAX_DISK_ARRAY_WIDTH);
     
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, BOGEL_TEST_RAID_GROUP_COUNT);

    /* Discover all the `unused drives'
     */
    fbe_test_sep_util_discover_all_drive_information(&bogel_discovered_drive_locations);
    fbe_test_sep_util_get_all_unused_pvd_location(&bogel_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    bogel_set_unused_drive_info(current_rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
                                              &disk_to_skip,
                                              num_of_spares_required);

    /* Now configure the available drives as `hot-spare' to prevent automatic
     * sparing and to select a specific drive.
     */
    for (unused_drive_index = 0; unused_drive_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; unused_drive_index++)
    {
        /* 0_0_0 Is a special drive location that designates the end of the
         * drive list.
         */
        if ((current_rg_config_p->unused_disk_set[unused_drive_index].bus == 0)       &&
            (current_rg_config_p->unused_disk_set[unused_drive_index].enclosure == 0) &&
            (current_rg_config_p->unused_disk_set[unused_drive_index].slot == 0)         )
        {
            /* End of list reached*/
            break;
        }

        /* Else populate the `hot-spare' configuration information.
         */
        hs_config[hs_index].block_size = 520;
        hs_config[hs_index].disk_set = current_rg_config_p->unused_disk_set[hs_index];
        hs_config[hs_index].hs_pvd_object_id = FBE_OBJECT_ID_INVALID;
        hs_index++;
    }
    if (hs_index < num_of_spares_required)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "*** %s rg type: %d required no spares: %d spares located: %d***", 
                   __FUNCTION__, current_rg_config_p->raid_type,
                   num_of_spares_required, hs_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = fbe_test_sep_util_configure_hot_spares(hs_config, hs_index);
    }

    return status;
}
/************************************************* 
 * end bogel_test_config_hotspares()
 *************************************************/

/*!***************************************************************************
 * bogel_clear_eol()
 *****************************************************************************
 * @brief
 *  Clear EOL on an drive that is alive.
 * 
 * @param source_disk_p - Pointer to source disk
 *
 * @return  none
 *
 * @author
 *  06/12/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void bogel_clear_eol(fbe_test_raid_group_disk_set_t *source_disk_p)
{
    fbe_status_t                    status;
    fbe_object_id_t                 pvd_object_id;
    fbe_api_provision_drive_info_t  pvd_info;
    fbe_u32_t                       retry_count = 0;

    /* Cleanup EOL on source drive.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(source_disk_p->bus,
                                                            source_disk_p->enclosure,
                                                            source_disk_p->slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_clear_disk_eol(source_disk_p->bus,       
                                              source_disk_p->enclosure, 
                                              source_disk_p->slot);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
    retry_count = 0;
    while ((pvd_info.end_of_life_state == FBE_TRUE) &&
           (retry_count < 200)                                     )
    {
        /* Wait a short time.
         */
        fbe_api_sleep(100);
        status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        retry_count++;
    }
    MUT_ASSERT_TRUE(pvd_info.end_of_life_state == FBE_FALSE);   
    return;
}
/************************************** 
 * end bogel_clear_eol()
 **************************************/

/*!***************************************************************************
 *          bogel_zero_swapped_out_drive()
 *****************************************************************************
 *
 * @brief   Write the swapped out drive with zeros so that if it swaps in it
 *          will have 0s instead of the valid data (rdgen pattern).
 *
 * @param   pvd_object_id - The pvd object id to be zeroed
 *
 * @return  status
 *
 * @author
 *  06/12/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t bogel_zero_swapped_out_drive(fbe_object_id_t pvd_object_id)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       retry_count = 0;

    /* Wait for the drive to be ready.
     */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          pvd_object_id,
                                                                          FBE_LIFECYCLE_STATE_READY, 
                                                                          BOGEL_TEST_SWAP_WAIT_TIME_MS, 
                                                                          FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for source drive to be zeroed.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Start zeroing on pvd obj: 0x%x", pvd_object_id);
    status = fbe_api_provision_drive_initiate_disk_zeroing(pvd_object_id);
    while ((status != FBE_STATUS_OK) &&
           (retry_count < 100)          )
    {
        /* Wait a short time.
         */
        fbe_api_sleep(100);
        status = fbe_api_provision_drive_initiate_disk_zeroing(pvd_object_id);
        retry_count++;
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, BOGEL_TEST_ZERO_DISK_TIME_MS);
    mut_printf(MUT_LOG_TEST_STATUS, "== Zeroing of pvd obj: 0x%x complete==",  pvd_object_id);

    return status;
}
/**************************************************
 * end bogel_zero_swapped_out_drive()
 **************************************************/

/*!***************************************************************************
 *          bogel_insert_hs_drive()
 ***************************************************************************** 
 * 
 * @brief   Re-insert a previously removed drive.
 *
 * @param   drive_to_insert_p - Pointer to drive location for drive to re-insert.
 * @param   drive_handle_to_insert - Drive handle for drive to re-insert.
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 *****************************************************************************/
static void bogel_insert_hs_drive(fbe_test_raid_group_disk_set_t *drive_to_insert_p,
                                              fbe_api_terminator_device_handle_t drive_handle_to_insert)
                            
{
    fbe_status_t    status;
    fbe_object_id_t pvd_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: re-insert and zero drive: %d_%d_%d",
                __FUNCTION__, drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);

    /* inserting the same drive back */
    status = fbe_test_sep_drive_reinsert_drive(drive_to_insert_p, drive_handle_to_insert);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Zero the data on the drive so that if it swapped in it doesn't have 
     * the rdgen pattern.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus,
                                                            drive_to_insert_p->enclosure,
                                                            drive_to_insert_p->slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = bogel_zero_swapped_out_drive(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/*****************************************
 * end bogel_insert_hs_drive()
 *****************************************/

/*!***************************************************************************
 * bogel_cleanup_reinsert_source_drive()
 *****************************************************************************
 * @brief
 *  insert drives, one per raid group.
 * 
 * @param rg_config_p - raid group config array.
 * @param source_disk_p - Pointer to source disk
 * @param position_to_copy - The position being copied
 *
 * @return None.
 *
 * @author
 *  04/17/2013   Ron Proulx - Updated.
 *
 *****************************************************************************/
static fbe_status_t bogel_cleanup_reinsert_source_drive(fbe_test_rg_configuration_t *rg_config_p, 
                                                                      fbe_test_raid_group_disk_set_t *source_disk_p, 
                                                                      fbe_u32_t position_to_copy)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   removed_index;
    fbe_object_id_t             pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                   position_to_insert;
    fbe_bool_t                  b_object_removed = FBE_FALSE;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_u32_t                   wait_attempts = 0;
    fbe_api_provision_drive_info_t  pvd_info;
    fbe_api_base_config_downstream_object_list_t downstream_object_list; 

    mut_printf(MUT_LOG_TEST_STATUS, "%s: re-insert, cleanup and zero drive: %d_%d_%d",
               __FUNCTION__,
               source_disk_p->bus, source_disk_p->enclosure, source_disk_p->slot);

    /* First checked if the drive is removed or not.
     */
    for (removed_index = 0; removed_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; removed_index++)
    {
        if (rg_config_p->drives_removed_array[removed_index] == position_to_copy)
        {
            b_object_removed = FBE_TRUE;
            position_to_insert = position_to_copy;
            break;
        }
    }

    /* If the source position was removed, re-insert it.
     */
    if (b_object_removed == FBE_TRUE)
    {
        fbe_test_sep_util_removed_position_inserted(rg_config_p, position_to_insert);
        drive_to_insert_p = source_disk_p;

        /* inserting the same drive back */
        if(fbe_test_util_is_simulation())
        {
            status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                 drive_to_insert_p->enclosure, 
                                                 drive_to_insert_p->slot,
                                                 rg_config_p->drive_handle[position_to_insert]);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                         drive_to_insert_p->enclosure, 
                                                         drive_to_insert_p->slot,
                                                         rg_config_p->peer_drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }
        else
        {
            status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                        drive_to_insert_p->enclosure, 
                                                        drive_to_insert_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                /* Set the target server to SPB and insert the drive there. */
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                            drive_to_insert_p->enclosure, 
                                                            drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* Set the target server back to SPA. */
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }

        /* Wait for object to be valid.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for rg %d position %d (%d_%d_%d) to become ready ==", 
                   __FUNCTION__, rg_config_p->raid_group_id, position_to_insert, 
                   drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);
        while (((status != FBE_STATUS_OK)                ||
                (pvd_object_id == FBE_OBJECT_ID_INVALID)    ) &&
               (wait_attempts < 20)                              )
        {
            /* Wait until we can get pvd object id.
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus,
                                                                    drive_to_insert_p->enclosure,
                                                                    drive_to_insert_p->slot,
                                                                    &pvd_object_id);
            if ((status != FBE_STATUS_OK)                ||
                (pvd_object_id == FBE_OBJECT_ID_INVALID)    )
            {
                fbe_api_sleep(500);
            }
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);

        /* Wait for the pdo to become ready
         */
        downstream_object_list.number_of_downstream_objects = 0;
        while ((downstream_object_list.number_of_downstream_objects != 1) &&
               (wait_attempts < 20)                                          )
        {
            /* Wait until we detect the downstream pdo
             */
            status = fbe_api_base_config_get_downstream_object_list(pvd_object_id, &downstream_object_list);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            if (downstream_object_list.number_of_downstream_objects != 1)
            {
                fbe_api_sleep(500);
            }
        }
        MUT_ASSERT_INT_EQUAL(1, downstream_object_list.number_of_downstream_objects);
        pdo_object_id = downstream_object_list.downstream_object_list[0];
        status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If dualsp wait for pdo Ready on peer.
         */
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /* Set the target server to SPB and insert the drive there. */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

            downstream_object_list.number_of_downstream_objects = 0;
            while ((downstream_object_list.number_of_downstream_objects != 1) &&
                   (wait_attempts < 20)                                          )
            {
                /* Wait until we detect the downstream pdo
                 */
                status = fbe_api_base_config_get_downstream_object_list(pvd_object_id, &downstream_object_list);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if (downstream_object_list.number_of_downstream_objects != 1)
                {
                    fbe_api_sleep(500);
                }
            }
            MUT_ASSERT_INT_EQUAL(1, downstream_object_list.number_of_downstream_objects);
            pdo_object_id = downstream_object_list.downstream_object_list[0];
            status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Set the target server back to SPA. */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        } /* end if dualsp need to wait for pdo Ready on peer*/

        /* If needed clear EOL
         */
        status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (pvd_info.end_of_life_state == FBE_TRUE)
        {
            bogel_clear_eol(drive_to_insert_p);
        }

        status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for %d_%d_%d to become ready - complete ==", 
                   __FUNCTION__,
                   drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);
    }
    else
    {
        /* Else we need to clear EOL.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(source_disk_p->bus,
                                                                source_disk_p->enclosure,
                                                                source_disk_p->slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_info(pvd_object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (pvd_info.end_of_life_state == FBE_TRUE)
        {
            status = fbe_test_sep_util_clear_disk_eol(source_disk_p->bus,
                                                      source_disk_p->enclosure,
                                                      source_disk_p->slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }

    /* Now wait for swapped out drive to be zeroed.
     */
    status = bogel_zero_swapped_out_drive(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return FBE_STATUS_OK;
}
/*****************************************************************************
 * end bogel_cleanup_reinsert_source_drive()
 *****************************************************************************/

/*!****************************************************************************
 * bogel_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *     8/26/2009:  Created. Dhaval Patel.
 ******************************************************************************/
void bogel_test(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &bogel_raid_group_config[0];

    /*! @note Need to remove `small' drive so that it doesn't get used as part
     *        of the raid group.
     */
    status = bogel_remove_small_drive(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,bogel_run_tests,
                                   BOGEL_TEST_LUNS_PER_RAID_GROUP,
                                   BOGEL_TEST_CHUNKS_PER_LUN);
    return;    
}
/********************************
 * end bogel_test()
 ********************************/

/*!****************************************************************************
 * bogel_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   8/26/2009:  Created. Dhaval Patel.
 ******************************************************************************/
void bogel_dualsp_test(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &bogel_raid_group_config[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /*! @note Need to remove `small' drive so that it doesn't get used as part
     *        of the raid group.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = bogel_remove_small_drive(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = bogel_remove_small_drive(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_run_test_on_rg_config(rg_config_p,"Dummy",bogel_run_tests,
                                   BOGEL_TEST_LUNS_PER_RAID_GROUP,
                                   BOGEL_TEST_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end bogel_dualsp_test()
 ******************************************************************************/

/*!****************************************************************************
 * bogel_setup()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *     8/26/2009:  Created. Dhaval Patel.
 ******************************************************************************/

void bogel_setup(void)
{
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        /* Randomize which raid type to use.
         */
        rg_config_p = &bogel_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        bogel_load_physical_config(rg_config_p,num_raid_groups);  
        elmo_load_sep_and_neit();

        /* After sep is loaded setup notifications.
         */
        fbe_test_common_setup_notifications(FBE_FALSE /* This is a single SP test*/);

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }
    
    return;
}
/********************************* 
 * end bogel_setup()
 *********************************/

/*!**************************************************************
 * bogel_create_physical_config_for_disk_counts()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param num_520_enclosures - ouput number of 520 enclosures
 * @param num_4160_enclosures - output number of 512 enclosures
 * @param num_520_drives - output number of 520 drives
 * @param num_4160_drives - output number of 512 drives
 *
 * @return None.
 *
 * @author
 *  8/16/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void bogel_create_physical_config_for_disk_counts(fbe_u32_t num_520_drives,
                                                                       fbe_u32_t num_4160_drives,
                                                                       fbe_block_count_t drive_capacity)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t                           slot;
    fbe_u32_t                           num_objects = 1; /* start with board */
    fbe_u32_t                           encl_number = 0;
    fbe_u32_t                           encl_index;
    fbe_u32_t                           num_520_enclosures;
    fbe_u32_t                           num_4160_enclosures;
    fbe_u32_t                           num_first_enclosure_drives;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;


    /*  We need to add extra enclosure for Hot Sparing and 1_0_7 */
    num_520_drives += (FBE_TEST_DRIVES_PER_ENCL + 10);

    num_520_enclosures = (num_520_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_520_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);
    num_4160_enclosures = (num_4160_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_4160_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);

    /* Before loading the physical package we initialize the terminator.
     */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Set the terminator debug flags (if enabled).
     */
    fbe_test_sep_util_set_terminator_drive_debug_flags(0);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    num_first_enclosure_drives = fbe_private_space_layout_get_number_of_system_drives();

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port_handle);

    /* We start off with:
     *  1 board
     *  1 port
     *  1 enclosure
     *  plus one pdo for each of the first 10 drives.
     */
    num_objects = 3 + ( num_first_enclosure_drives); 

    /* Next, add on all the enclosures and drives we will add.
     */
    num_objects += num_520_enclosures + ( num_520_drives);
    num_objects += num_4160_enclosures + ( num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "== Creating %d db drives and %d 520 drives and %d 512 drives ==", 
               num_first_enclosure_drives, (num_520_drives - num_first_enclosure_drives), num_4160_drives);

    /* First enclosure has 4 drives for system rgs.
     */
    fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
    for (slot = 0; slot < num_first_enclosure_drives; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + drive_capacity, &drive_handle);
    }

    /* We inserted one enclosure above, so we are starting with encl #1.
     */
    encl_number = 1;

    /* Insert enclosures and drives for 520.  Start at encl number one.
     */
    for ( encl_index = 0; encl_index < num_520_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);

        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_520_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 520, drive_capacity, &drive_handle);
            num_520_drives--;
            slot++;
        }
        encl_number++;
    }
    /* Insert enclosures and drives for 4160.
     * We pick up the enclosure number from the prior loop. 
     */
    for ( encl_index = 0; encl_index < num_4160_enclosures; encl_index++)
    {
        fbe_test_pp_util_insert_viper_enclosure(0, encl_number, port_handle, &encl_handle);
        slot = 0;
        while ((slot < FBE_TEST_DRIVES_PER_ENCL) && num_4160_drives)
        {
            fbe_test_pp_util_insert_sas_drive(0, encl_number, slot, 4160, drive_capacity, &drive_handle);
            num_4160_drives--;
            slot++;
        }
        encl_number++;
    }

    /* We inserted a drive with 1_0_7 location separately with a smaller 
     * capacity as a destination spare drive in order to make sure the spare 
     * validation is always triggered while doing a spare.  This drive will 
     * be used for user copy with expected failure test case. 
     */
    fbe_test_pp_util_insert_sas_pmc_port(2,
                                         2,
                                         1,
                                         &port_handle);
    fbe_test_pp_util_insert_viper_enclosure(1, 0, port_handle, &encl_handle);
    fbe_test_pp_util_insert_sas_drive(1, 0, 7, 520, TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY / 2, &drive_handle); 

    /* We add:
     *  1 port
     *  1 enclosure
     *  plus one pdo for drvie 1_0_7
     */
    num_objects += 2 +  1;

    mut_printf(MUT_LOG_LOW, "=== starting physical package===");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    mut_printf(MUT_LOG_LOW, "=== wait for all objects ready ===");
    status = fbe_api_wait_for_number_of_objects(num_objects, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    
    /* Wait for all objects ready
     */
    for (object_id = 0 ; object_id < num_objects; object_id++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object ready failed");
    }

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == num_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");
    return;
}
/******************************************************************
 * end bogel_create_physical_config_for_disk_counts()
 ******************************************************************/

/*!**************************************************************
 * bogel_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param rg_config_p - Current config.
 *
 * @return None.
 *
 * @author
 *     8/26/2009:  Created. Dhaval Patel.
 *
 ****************************************************************/
void bogel_load_physical_config(fbe_test_rg_configuration_t *rg_config_p,
                                              fbe_u32_t num_raid_groups)
{
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;

          
     /* First get the count of drives.
     */
    fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, num_raid_groups,
                                                       &num_520_drives,
                                                       &num_4160_drives);
      
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d raid group(s) with %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_raid_groups, num_520_drives, num_4160_drives);

    /* Add a configuration to each raid group config.
     */
    fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, num_raid_groups, 
                                        num_520_drives, num_4160_drives);

    bogel_create_physical_config_for_disk_counts(num_520_drives,
                                                               num_4160_drives,
                                                               TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

    return;
}
/******************************************************************
 * end bogel_load_physical_config()
 ******************************************************************/

static void bogel_write_background_pattern(fbe_api_rdgen_context_t *  in_test_context_p)
{
    fbe_status_t    status;

    /*  Set up the context to do a write (fill) operation. */
    fbe_test_sep_io_setup_fill_rdgen_test_context(in_test_context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN, 
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID,
                                            BOGEL_TEST_ELEMENT_SIZE);
                                            
    /*  Write a background pattern to all of the LUNs in the entire configuration. */
    status = fbe_api_rdgen_test_context_run_all_tests(in_test_context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);
    return;
}


static void bogel_verify_background_pattern(fbe_api_rdgen_context_t *  in_test_context_p,
                                            fbe_u32_t in_element_size)
{
    fbe_status_t    status;

    /* perform read check operation to verify the written background pattern. */
    fbe_test_sep_io_setup_fill_rdgen_test_context(in_test_context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            in_element_size);                                            
    status = fbe_api_rdgen_test_context_run_all_tests(in_test_context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);
    return;
}

static void bogel_get_source_destination_edge_index(fbe_object_id_t     vd_object_id,
                                                    fbe_edge_index_t *  source_edge_index_p,
                                                    fbe_edge_index_t *  dest_edge_index_p)
{
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    *source_edge_index_p = FBE_EDGE_INDEX_INVALID;
    *dest_edge_index_p = FBE_EDGE_INDEX_INVALID;

    /* verify that Proactive spare gets swapped-in and configuration mode gets changed to mirror. */
    fbe_test_sep_util_get_virtual_drive_configuration_mode(vd_object_id, &configuration_mode);
    if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
       (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE))
    {
        *source_edge_index_p = 0;
        *dest_edge_index_p = 1;
    }
    else if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) ||
            (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
    { 
        *source_edge_index_p = 1;
        *dest_edge_index_p = 0;
    }
}

/*!**************************************************************
 * bogel_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a Proactive Sparing  test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *     8/26/2009:  Created. Dhaval Patel.
 *
 ****************************************************************/
void bogel_dualsp_setup(void)
{
    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;

        fbe_test_rg_configuration_t *rg_config_p = NULL;
        
        /* Randomize which raid type to use.
         */
        rg_config_p = &bogel_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        bogel_load_physical_config(rg_config_p, num_raid_groups);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        bogel_load_physical_config(rg_config_p, num_raid_groups);
        
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

        /* After sep is load setup notifications */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dualsp test */);

    }
    return;
}
/******************************************
 * end bogel_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * bogel_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the bogel test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *     8/26/2009:  Created. Dhaval Patel.
 *
 ****************************************************************/
void bogel_dualsp_cleanup(void)
{
    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test*/);

    fbe_test_sep_util_print_dps_statistics();

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
 * end bogel_dualsp_cleanup()
 ******************************************/

/*!**************************************************************
 *          bogel_run_tests()
 ****************************************************************
 * @brief
 *  Run the basic copy test again all configured raid groups.
 *
 * @param   rg_config_p - Pointer to array of raid groups under
 *              test. 
 *
 * @return None.
 *
 * @author
 *  08/09/2009  Dahval Patel    - Created
 *
 ****************************************************************/
void bogel_run_tests(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   test_index;
    fbe_u32_t                   rg_index;
    fbe_u32_t                   hs_index = 0;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   num_spares_required = 0;
    fbe_bool_t                  b_is_dualsp = FBE_FALSE;
    fbe_u32_t                   current_test_case;
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Diabolicdiscdemon test started!!.\n\n", __FUNCTION__);

    b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    /* Set the trace level as information. */
    bogel_set_trace_level(FBE_TRACE_TYPE_DEFAULT, FBE_OBJECT_ID_INVALID, FBE_TRACE_LEVEL_INFO);

    /*! @note Now re-insert the drive that was removed so that it didn't get
     *        used for the raid group.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_pp_util_reinsert_drive(1, 0, 7,
                                             current_rg_config_p->drive_handle[0]);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (b_is_dualsp == FBE_TRUE)
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        status = fbe_test_pp_util_reinsert_drive(1, 0, 7,
                                                 current_rg_config_p->drive_handle[0]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Since these test do not generate corrupt crc etc, we don't expect
     * ANY errors.  So stop the system if any sector errors occur.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);

    /* Speed up VD hot spare */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Determine the */
    /* write background pattern to all the luns before we start copy operation. */
    bogel_write_background_pattern(&bogel_test_context[0]);

    /* For all raid groups under test.
     */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        num_spares_required = 0;
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* Determine the number of spares required.
             */
            current_test_case = 1;
            for (test_index = 0; test_index < BOGEL_NUMBER_OF_ENABLED_TEST_CASES; test_index++, current_test_case <<= 1) 
            {
                if ((current_rg_config_p->test_case_bitmask == 0 ) ||
                    (current_rg_config_p->test_case_bitmask & current_test_case))
                {
                    num_spares_required += bogel_test_info[test_index].number_of_spare;
                }
            }

            /* Always configure the hot-spares.*/
            status =  bogel_test_config_hotspares(current_rg_config_p, num_spares_required);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            hs_index = 0;

            current_test_case = 1;

            /* Run all the tests. */
            for (test_index = BOGEL_USER_COPY_WITH_FAILURE_FIRST; test_index < BOGEL_NUMBER_OF_ENABLED_TEST_CASES; test_index++, current_test_case <<= 1) 
            {

                if ((current_rg_config_p->test_case_bitmask != 0 ) &&
                     !(current_rg_config_p->test_case_bitmask & current_test_case))
                {
                    continue;
                }

                /* If the test position is greater than the number pf positions
                 * skip the test.
                 */
                if (bogel_test_info[test_index].drive_pos >= current_rg_config_p->width)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                    mut_printf(MUT_LOG_TEST_STATUS, "Copy test case (%d) rg type: %d drive_pos: %d not valid for width: %d skip.", 
                               test_index, current_rg_config_p->raid_type, bogel_test_info[test_index].drive_pos, current_rg_config_p->width);
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                }
                else
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                    mut_printf(MUT_LOG_TEST_STATUS, "Copy test case (%d) started on rg type: %d!!!", 
                               test_index, current_rg_config_p->raid_type);
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
    
                    switch(bogel_test_info[test_index].test_case)
                    {
                        case BOGEL_USER_COPY_ON_BROKEN_RAID_GROUP:
                            status = bogel_test_user_copy_on_broken_raid_group(current_rg_config_p);
                            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                            break;
                        case BOGEL_USER_COPY_ON_DEGRADED_RAID_GROUP:
                            status = bogel_test_user_copy_on_degraded_raid_group(current_rg_config_p);
                            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                            break;
                        case BOGEL_COPY_ALREADY_IN_PROGRESS:
                            /* We will randomize which type of copy we use each time.
                             */
                            bogel_test_second_copy_on_same_raid_group(current_rg_config_p,
                                                                      bogel_test_info[test_index].is_user_initiated_copy);
                            break;
                        case BOGEL_USER_COPY_WITH_FAILURE_FIRST:
                            /* it tests user coy with expected failure status. */
                            bogel_test_user_copy_with_expected_failure(current_rg_config_p, 
                                                                       bogel_test_info[test_index].drive_pos);
                            break; 
                        case BOGEL_SIMPLE_PROACTIVE_COPY_SECOND:
                            /* Attempt a copy on a virtual drive with copy in progress.*/
                            bogel_test_second_copy_on_same_virtual_drive(current_rg_config_p,
                                                                         bogel_test_info[test_index].drive_pos,
                                                                         bogel_test_info[test_index].is_user_initiated_copy);
                            break;

                    } /* end switch test case index */

                } /* end else drive position for test is valid */

                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                mut_printf(MUT_LOG_TEST_STATUS, "Copy test case (%d) completed successfully!!!", test_index);
                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");

                /* increment the hot spare index with number of spares for the particular test. */
                hs_index += bogel_test_info[test_index].number_of_spare;
                MUT_ASSERT_TRUE(hs_index <= num_spares_required);

            } /* end for all test cases */

        } /* end if this raid group is enabled for test*/

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups under test*/

    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    /* Set the sector trace stop on error off.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    fbe_test_sep_util_unconfigure_all_spares_in_the_system();

    /* Set the trigger spare timer to the default value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:bogel test completed successfully!!.\n", __FUNCTION__);
    return;
}
/****************************************** 
 * end bogel_run_tests()
 ******************************************/

/*!***************************************************************************
 *          bogel_check_event_log_msg()
 ***************************************************************************** 
 * 
 * @brief   Look for the event log code and validate (1) of (3) options:
 *              1. Both the spare drive and original drive location is valid:
 *                  "Message spare location:b_e_s original:b_e_s"
 *              2. Only the original drive is valid:
 *                  "Failed to find spare for disk:b_e_s"
 *              3. Only the spare drive is valid.
 *                  "Spare drive failed:b_e_s"
 *
 * @param   message_code - The expected event code
 * @param   b_is_spare_drive_valid - The spare drive information is valid
 * @param   spare_drive_location - Pointer to spare drive location
 * @param   b_is_original_drive_valid - The original drive location is valid
 * @param   original_drive_location - Pointer to original drive location
 *
 * @return  none
 *
 * @author
 *  07/31/2012  Ron Proulx  - Created.
 *****************************************************************************/
void bogel_check_event_log_msg(fbe_u32_t message_code,
                                           fbe_bool_t b_is_spare_drive_valid,
                                           fbe_test_raid_group_disk_set_t *spare_drive_location,
                                           fbe_bool_t b_is_original_drive_valid,
                                           fbe_test_raid_group_disk_set_t *original_drive_location)
{
    fbe_status_t    status;
    fbe_bool_t      b_is_msg_present = FBE_FALSE;
    fbe_u32_t       retry_count;
    fbe_u32_t       max_retries;         


    /* set the max retries in a local variable. */
    max_retries = BOGEL_TEST_MAX_RETRIES;

    /*  loop until the number of chunks marked NR is 0 for the drive that we are rebuilding. */
    for (retry_count = 0; retry_count < max_retries; retry_count++)
    {
        /* check that given event message is logged correctly
         */
        if ((b_is_spare_drive_valid == FBE_TRUE)    &&
            (b_is_original_drive_valid == FBE_TRUE)    )
        {
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                         &b_is_msg_present, 
                         message_code,
                         spare_drive_location->bus, spare_drive_location->enclosure, spare_drive_location->slot,
                         original_drive_location->bus, original_drive_location->enclosure, original_drive_location->slot);
        }
        else if ((b_is_spare_drive_valid == FBE_FALSE)   &&
                 (b_is_original_drive_valid == FBE_TRUE)    )
        {
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                         &b_is_msg_present, 
                         message_code,
                         original_drive_location->bus, original_drive_location->enclosure, original_drive_location->slot);
        }
        else if ((b_is_spare_drive_valid == FBE_TRUE)     &&
                 (b_is_original_drive_valid == FBE_FALSE)    )
        {
            status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                         &b_is_msg_present, 
                         message_code,
                         spare_drive_location->bus, spare_drive_location->enclosure, spare_drive_location->slot);
        }
        else
        {
                MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, FBE_FALSE, "Invalid parameters");
        }

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (b_is_msg_present == FBE_TRUE)
        {
            return;
        }

        /*  wait before trying again. */
        fbe_api_sleep(BOGEL_TEST_EVENT_LOG_WAIT_TIME);
    }
    
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, b_is_msg_present, "Event log msg is not present!");
 
    return;
}
/*********************************************
 * end bogel_check_event_log_msg()
 **********************************************/

/*!***************************************************************************
 *          bogel_set_rebuild_hook()
 *****************************************************************************
 * @brief
 *  This function determines the lba for the rebuild hook based
 *  on the user space lba passed.
 *
 * @param   vd_object_id - object id of vd associated with hook
 * @param   percent_copied_before_pause - Percentage copied before we
 *
 * @return  lba - lba of hook
 *
 * @author
 *  09/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_lba_t bogel_set_rebuild_hook(fbe_object_id_t vd_object_id, fbe_u32_t percent_copied_before_pause)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_lba_t                       checkpoint_to_pause_at = FBE_LBA_INVALID;

    /* If we want to stop right away just set the return value to 0.
     */
    if (percent_copied_before_pause == 0)
    {
        checkpoint_to_pause_at = 0;
        return checkpoint_to_pause_at;
    }

    /* Use common API to determine the `pause at' lba.
     */
    status = fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at(vd_object_id,
                                                                              percent_copied_before_pause,
                                                                              &checkpoint_to_pause_at);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: vd obj: 0x%x pause percent: %d lba: 0x%llx",
               __FUNCTION__, vd_object_id, percent_copied_before_pause, checkpoint_to_pause_at);

    /* Set the debug hook
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              checkpoint_to_pause_at,
                                              0,
                                              SCHEDULER_CHECK_VALS_GT,
                                              SCHEDULER_DEBUG_ACTION_PAUSE); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return checkpoint_to_pause_at;
}
/***********************************************
 * end bogel_set_rebuild_hook()
 ***********************************************/

/*!***************************************************************************
 *          bogel_wait_for_rebuild_hook()
 *****************************************************************************
 * @brief
 *  This waits for the previously set rebuild hook.
 *
 * @param   vd_object_id - object id of vd associated with hook
 * @param   checkpoint_to_pause_at - lba that hook was set for
 *
 * @return  None
 *
 * @author
 *  09/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static void bogel_wait_for_rebuild_hook(fbe_object_id_t vd_object_id, fbe_lba_t checkpoint_to_pause_at)
{
    fbe_status_t                    status = FBE_STATUS_OK;

    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                         FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                         SCHEDULER_CHECK_VALS_GT, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 
                                         checkpoint_to_pause_at,
                                         0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/*************************************************
 * end bogel_wait_for_rebuild_hook()
 *************************************************/

/*!***************************************************************************
 *          bogel_remove_rebuild_hook()
 *****************************************************************************
 * @brief
 *  This removes the rebuild hook.
 *
 * @param   vd_object_id - object id of vd associated with hook
 * @param   checkpoint_to_pause_at - lba that hook was set for
 *
 * @return  None
 *
 * @author
 *  09/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static void bogel_remove_rebuild_hook(fbe_object_id_t vd_object_id, fbe_lba_t checkpoint_to_pause_at)
{
    fbe_status_t                    status = FBE_STATUS_OK;

    /* remove the hook .*/
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                              FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                              checkpoint_to_pause_at, 
                                              0, 
                                              SCHEDULER_CHECK_VALS_GT, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/*************************************************
 * end bogel_remove_rebuild_hook()
 *************************************************/

/*!***************************************************************************
 *          bogel_get_valid_destination_drive()
 *****************************************************************************
 *
 * @brief   This method used the `unused_disk_set' from the raid group test
 *          configuration passed to locate a valid destination drive for a
 *          User Copy To request.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   dest_disk_p - Pointer to destination disk to use
 *
 * @return  status
 *
 * @author
 *  10/19/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t bogel_get_valid_destination_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_test_raid_group_disk_set_t *dest_disk_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               hs_index = 0;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_test_raid_group_disk_set_t          disk_to_skip = {1, 0, 7};
    fbe_u32_t                               local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
  
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Discover all the `unused drives'
     */
    fbe_test_sep_util_get_all_unused_pvd_location(&bogel_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    bogel_set_unused_drive_info(rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
                                              &disk_to_skip,
                                              1);

    /* Now locate the next available spare.
     */
    for (hs_index = 0; hs_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; hs_index++)
    {
        /* 0_0_0 Is a special drive location that designates the end of the
         * drive list.
         */
        if ((rg_config_p->unused_disk_set[hs_index].bus == 0)       &&
            (rg_config_p->unused_disk_set[hs_index].enclosure == 0) &&
            (rg_config_p->unused_disk_set[hs_index].slot == 0)         )
        {
            /* End of list reached*/
            status = FBE_STATUS_GENERIC_FAILURE;
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    
        /* Else populate the `destination' configuration information.
         */
        *dest_disk_p = rg_config_p->unused_disk_set[hs_index];
        break;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/******************************************************** 
 * end bogel_get_valid_destination_drive()
 ********************************************************/

/*!***************************************************************************
 * bogel_test_debug_fail_destination_during_swap()
 *****************************************************************************
 *
 * @brief
 *  We should fail the destination drive during the swap operation.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  3/8/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static void bogel_test_debug_fail_destination_during_swap(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 original_source_pvd_object_id;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &bogel_test_context[0];
    fbe_test_raid_group_disk_set_t  removed_drive_location;
    fbe_api_terminator_device_handle_t removed_drive_handle;
    fbe_api_get_block_edge_info_t   block_edge_info;
    fbe_lba_t                       hook_lba;

    mut_printf(MUT_LOG_TEST_STATUS, "== destination drive removal during proactive copy swap started ==\n");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, 
               rg_config_p->rg_disk_set[drive_pos].bus, 
               rg_config_p->rg_disk_set[drive_pos].enclosure, 
               rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Enable both the raid group and raid library debug flags.
     */
    bogel_enable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                    FBE_TRUE,   /* Enable the raid group debug flags */
                                                                    FBE_TRUE    /* Enable the raid library debug flags */);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &original_source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED, 
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* trigger the proactive copy operation using error injection on peer. */
    bogel_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                          SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                          FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                          SCHEDULER_CHECK_STATES, 
                                          SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* proactive spare gets swapped-in, get the spare edge information. */
    status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_object_lifecycle_state(block_edge_info.server_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object ready failed");
    status = fbe_api_provision_drive_get_location(block_edge_info.server_id,
                                                  &removed_drive_location.bus,
                                                  &removed_drive_location.enclosure,
                                                  &removed_drive_location.slot);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Remove destination drive immediately.
     */
    fbe_test_sep_drive_pull_drive(&removed_drive_location, &removed_drive_handle);

    /* remove hook to allow set config to complete.
     * The big part of this test is to hold up the set config while we remove the drive.
     * This causes us to "miss" the event since we are still in pass through  mode
     * when the event arrives for the destination going away.
     */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* The drive failing should cause the copy to abort.  Wait for the copy to stop.
     */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                          SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                          FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED,
                                          SCHEDULER_CHECK_STATES, 
                                          SCHEDULER_DEBUG_ACTION_LOG, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = bogel_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* Now remove the debug hook for abort copy granted.
     */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /*! @note Since EOL is set on the source drive a second proactive copy
     *        operation will automatically be initiated.
     */

    /* Cleanup removed, EOL drives*/
    spare_drive_location = rg_config_p->rg_disk_set[drive_pos];
    status = bogel_cleanup_reinsert_source_drive(rg_config_p, &spare_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    bogel_insert_hs_drive(&removed_drive_location, removed_drive_handle);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Now remove the rebuild hook and wait for the copy to complete
     */
    bogel_remove_rebuild_hook(vd_object_id, hook_lba);

    /* wait for the proactive copy completion. 
     */
    bogel_wait_for_proactive_copy_completion(vd_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /* Clear raid group and raid library debug flags for this test.
     */
    bogel_disable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                     FBE_TRUE,   /* Disable the raid group debug flags */
                                                                     FBE_TRUE,   /* Disable the raid library debug flags */
                                                                     FBE_TRUE   /* Clear the debug flags */);

    mut_printf(MUT_LOG_TEST_STATUS, "Destination drive removal during proactive copy swap test - completed!!!\n");
    return;
}
/******************************************************************
 * end bogel_test_debug_fail_destination_during_swap()
 ******************************************************************/

/*!***************************************************************************
 *          bogel_test_second_copy_on_same_raid_group()
 *****************************************************************************
 *
 * @brief   Attempt a second copy operation on a raid group that already has a
 *          copy operation in progress.  The second copy should fail.
 *
 * @param   rg_config_p - Pointer to test raid group configuration
 * @param   b_user_copy - FBE_TRUE - the first copy is a user copy
 *                        FBE_FALSE - The first copy is proactive copy
 *
 * @return  lba - The correct lba for the rebuild hook
 *
 * @author
 *  09/07/2012  Rob Foley   - Created.
 *
 *****************************************************************************/
static void bogel_test_second_copy_on_same_raid_group(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_bool_t b_user_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       drive_pos = 0; /* mbz */
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 first_vd_object_id;
    fbe_u32_t                       first_copy_pos;
    fbe_edge_index_t                first_source_edge_index;
    fbe_edge_index_t                first_dest_edge_index;
    fbe_u32_t                       second_copy_pos;
    fbe_object_id_t                 source_pvd_object_id;
    fbe_object_id_t                 second_vd_object_id;
    fbe_object_id_t                 second_source_pvd_object_id;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                second_source_edge_index;
    fbe_edge_index_t                second_dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &bogel_test_context[0];
    fbe_lba_t                       hook_lba = FBE_LBA_INVALID;
    fbe_provision_drive_copy_to_status_t        copy_status;
    fbe_bool_t b_is_msg_present;

    /* Determine first and second copy positions.
     */
    first_copy_pos = drive_pos;
    second_copy_pos = first_copy_pos + 1;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s started\n", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start %s operation on disk:%d_%d_%d",
               __FUNCTION__, (b_user_copy) ? "user copy" : "proactive copy",
               rg_config_p->rg_disk_set[first_copy_pos].bus, 
               rg_config_p->rg_disk_set[first_copy_pos].enclosure, rg_config_p->rg_disk_set[first_copy_pos].slot);

    /* The previous test changed things, so refresh the disk sets.
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, first_copy_pos, &first_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(first_vd_object_id, &first_source_edge_index, &first_dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, first_source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, first_dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[first_copy_pos].bus,
                                                            rg_config_p->rg_disk_set[first_copy_pos].enclosure,
                                                            rg_config_p->rg_disk_set[first_copy_pos].slot,
                                                            &source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = bogel_set_rebuild_hook(first_vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, first_vd_object_id, hook_lba);

    mut_printf(MUT_LOG_TEST_STATUS, "== Start a copy operation");
    if (b_user_copy == FBE_TRUE)
    {
        /* user initiated copy */
        status = fbe_api_provision_drive_user_copy(source_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR);
    }
    else
    {
        /* if not user initiated copy, we trigger the proactive copy operation using error injection. */
        bogel_io_error_trigger_proactive_sparing(rg_config_p, first_copy_pos, first_source_edge_index);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for swap in");
    bogel_wait_for_proactive_spare_swap_in(first_vd_object_id, first_dest_edge_index, &spare_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, first_vd_object_id, hook_lba);
    bogel_wait_for_rebuild_hook(first_vd_object_id, hook_lba);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, second_copy_pos, &second_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(second_vd_object_id, &second_source_edge_index, &second_dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, second_source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, second_dest_edge_index);

    /* Get the provision object for second copy position.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[second_copy_pos].bus,
                                                            rg_config_p->rg_disk_set[second_copy_pos].enclosure,
                                                            rg_config_p->rg_disk_set[second_copy_pos].slot,
                                                            &second_source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* A second copy operation should fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Try to start user copy:  It should fail.");
    status = fbe_api_provision_drive_user_copy(second_source_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_RG_COPY_ALREADY_IN_PROGRESS);

    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                         &b_is_msg_present, 
                                         SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS,
                                         rg_config_p->rg_disk_set[second_copy_pos].bus,
                                         rg_config_p->rg_disk_set[second_copy_pos].enclosure,
                                         rg_config_p->rg_disk_set[second_copy_pos].slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(b_is_msg_present, FBE_TRUE, "Copy in progress msg not found in event log");

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Triggering the copy should also fail.  Make sure the event log is present.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Trigger proactive copy.  It will report failure but keep trying.");
    bogel_io_error_trigger_proactive_sparing(rg_config_p, second_copy_pos, second_source_edge_index);

    /* We need to poll for the message since the copy is requested asynchronously from 
     * triggering the copy. 
     */
    status = fbe_api_wait_for_event_log_msg(FBE_TEST_WAIT_TIMEOUT_MS,
                                            FBE_PACKAGE_ID_SEP_0, 
                                            &b_is_msg_present, 
                                            SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS,
                                            rg_config_p->rg_disk_set[second_copy_pos].bus,
                                            rg_config_p->rg_disk_set[second_copy_pos].enclosure,
                                            rg_config_p->rg_disk_set[second_copy_pos].slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(b_is_msg_present, FBE_TRUE, "Copy in progress msg not found in event log");

    /* remove the hook .*/
    bogel_remove_rebuild_hook(first_vd_object_id, hook_lba);

    /* wait for the first source drive to swap-out.
     */
    bogel_wait_for_source_drive_swap_out(first_vd_object_id, first_source_edge_index);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(first_vd_object_id, first_dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /* wait for the second source drive to swap-out.
     */
    bogel_wait_for_source_drive_swap_out(second_vd_object_id, second_source_edge_index);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(second_vd_object_id, second_dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /* Make the view of the raid group disk set consistent for the next test.
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s completed \n", __FUNCTION__);
    return;
}
/********************************************************** 
 * end bogel_test_second_copy_on_same_raid_group()
 **********************************************************/

/*!***************************************************************************
 *          bogel_test_second_copy_on_same_virtual_drive()
 *****************************************************************************
 *
 * @brief   Attempt a second copy operation on a virtual drive that already has a
 *          copy operation in progress.  The second copy should fail.
 *
 * @param   rg_config_p - Pointer to test raid group configuration
 * @param   drive_pos - Position in raid group to start first copy on
 * @param   b_user_copy - FBE_TRUE - the first copy is a user copy
 *                        FBE_FALSE - The first copy is proactive copy
 *
 * @return  lba - The correct lba for the rebuild hook
 *
 * @author
 *  01/03/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void bogel_test_second_copy_on_same_virtual_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t  drive_pos,
                                                                  fbe_bool_t b_user_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 first_vd_object_id;
    fbe_u32_t                       first_copy_pos;
    fbe_edge_index_t                first_source_edge_index;
    fbe_edge_index_t                first_dest_edge_index;
    fbe_u32_t                       second_copy_pos;
    fbe_object_id_t                 source_pvd_object_id;
    fbe_object_id_t                 second_vd_object_id;
    fbe_object_id_t                 second_source_pvd_object_id;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                second_source_edge_index;
    fbe_edge_index_t                second_dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &bogel_test_context[0];
    fbe_lba_t                       hook_lba = FBE_LBA_INVALID;
    fbe_provision_drive_copy_to_status_t        copy_status;
    fbe_bool_t b_is_msg_present;

    /* Determine first and second copy positions.
     */
    first_copy_pos = drive_pos;
    second_copy_pos = first_copy_pos;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s started\n", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start %s operation on disk:%d_%d_%d",
               __FUNCTION__, (b_user_copy) ? "user copy" : "proactive copy",
               rg_config_p->rg_disk_set[first_copy_pos].bus, 
               rg_config_p->rg_disk_set[first_copy_pos].enclosure, rg_config_p->rg_disk_set[first_copy_pos].slot);

    /* The previous test changed things, so refresh the disk sets.
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, first_copy_pos, &first_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(first_vd_object_id, &first_source_edge_index, &first_dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, first_source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, first_dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[first_copy_pos].bus,
                                                            rg_config_p->rg_disk_set[first_copy_pos].enclosure,
                                                            rg_config_p->rg_disk_set[first_copy_pos].slot,
                                                            &source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = bogel_set_rebuild_hook(first_vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, first_vd_object_id, hook_lba);

    mut_printf(MUT_LOG_TEST_STATUS, "== Start a copy operation");
    if (b_user_copy == FBE_TRUE)
    {
        /* user initiated copy */
        status = fbe_api_provision_drive_user_copy(source_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR);
    }
    else
    {
        /* if not user initiated copy, we trigger the proactive copy operation using error injection. */
        bogel_io_error_trigger_proactive_sparing(rg_config_p, first_copy_pos, first_source_edge_index);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for swap in");
    bogel_wait_for_proactive_spare_swap_in(first_vd_object_id, first_dest_edge_index, &spare_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, first_vd_object_id, hook_lba);
    bogel_wait_for_rebuild_hook(first_vd_object_id, hook_lba);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, second_copy_pos, &second_vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(second_vd_object_id, &second_source_edge_index, &second_dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, second_source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, second_dest_edge_index);

    /* Get the provision object for second copy position.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[second_copy_pos].bus,
                                                            rg_config_p->rg_disk_set[second_copy_pos].enclosure,
                                                            rg_config_p->rg_disk_set[second_copy_pos].slot,
                                                            &second_source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* A second copy operation should fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Try to start user copy:  It should fail.");
    status = fbe_api_provision_drive_user_copy(second_source_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);

    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                         &b_is_msg_present, 
                                         SEP_ERROR_CODE_SPARE_OPERATION_IN_PROGRESS,
                                         rg_config_p->rg_disk_set[second_copy_pos].bus,
                                         rg_config_p->rg_disk_set[second_copy_pos].enclosure,
                                         rg_config_p->rg_disk_set[second_copy_pos].slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(b_is_msg_present, FBE_TRUE, "Copy in progress msg not found in event log");

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the hook .*/
    bogel_remove_rebuild_hook(first_vd_object_id, hook_lba);

    /* wait for the first source drive to swap-out.
     */
    bogel_wait_for_source_drive_swap_out(first_vd_object_id, first_source_edge_index);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(first_vd_object_id, first_dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /* Make the view of the raid group disk set consistent for the next test.
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s completed \n", __FUNCTION__);
    return;
}
/****************************************************************** 
 * end bogel_test_second_copy_on_same_virtual_drive()
 ******************************************************************/


/* Populate the io error record */
static fbe_status_t bogel_populate_io_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
                                                                         fbe_u32_t drive_pos,
                                                                         fbe_u8_t requested_scsi_command_to_trigger,
                                                                         fbe_protocol_error_injection_error_record_t *record_p,
                                                                         fbe_protocol_error_injection_record_handle_t *record_handle_p,
                                                                         fbe_scsi_sense_key_t scsi_sense_key,
                                                                         fbe_scsi_additional_sense_code_t scsi_additional_sense_code,
                                                                         fbe_scsi_additional_sense_code_qualifier_t scsi_additional_sense_code_qualifier)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    scsi_command_to_trigger = requested_scsi_command_to_trigger;
    fbe_object_id_t             drive_object_id = 0;
    fbe_object_id_t             pvd_object_id = 0;
    fbe_api_provision_drive_info_t provision_drive_info;

    /* Get the drive object id for the position specified.
     */
    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                              rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                              rg_config_p->rg_disk_set[drive_pos].slot, 
                                                              &drive_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Calculate the physical lba to inject to.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    bogel_default_offset = provision_drive_info.default_offset;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (bogel_b_is_DE542_fixed == FBE_FALSE)
    {
        if (requested_scsi_command_to_trigger != FBE_SCSI_VERIFY_16)
        {
            //mut_printf(MUT_LOG_LOW, "%s: Due to DE542 change opcode from: 0x%02x to: 0x%02x", 
            //           __FUNCTION__, requested_scsi_command_to_trigger, FBE_SCSI_VERIFY_16);
            scsi_command_to_trigger = FBE_SCSI_VERIFY_16;
        }
        MUT_ASSERT_TRUE(scsi_command_to_trigger == FBE_SCSI_VERIFY_16);
    }

    /* Invoke the method to initialize and populate the (1) error record
     */
    status = fbe_test_neit_utils_populate_protocol_error_injection_record(rg_config_p->rg_disk_set[drive_pos].bus,
                                                                          rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                                          rg_config_p->rg_disk_set[drive_pos].slot,
                                                                          record_p,
                                                                          record_handle_p,
                                                                          bogel_user_space_lba_to_inject_to,
                                                                          bogel_user_space_blocks_to_inject,
                                                                          FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI,
                                                                          scsi_command_to_trigger,
                                                                          scsi_sense_key,
                                                                          scsi_additional_sense_code,
                                                                          scsi_additional_sense_code_qualifier,
                                                                          FBE_PORT_REQUEST_STATUS_SUCCESS, 
                                                                          1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}

/*!***************************************************************************
 *          bogel_run_io_to_start_copy()
 *****************************************************************************
 *
 * @brief   If the test isn't running I/O but they want to trigger a proactive
 *          copy, this method is invoked to run the specified I/O type which
 *          will trigger the copy.
 *
 * @param   rg_config_p - Pointer to raid group array to locate pvds
 * @param   sparing_position - Position that is being copied
 * @param   source_edge_index - Source vd edge index to send I/O to
 * @param   requested_scsi_opcode_to_inject_on - SCSI operation that is being injected to
 * @param   io_size - Fixed number of blocks per I/O
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  10/12/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t bogel_run_io_to_start_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                             fbe_u32_t sparing_position,
                                                             fbe_edge_index_t source_edge_index,
                                                             fbe_u8_t requested_scsi_opcode_to_inject_on,
                                                             fbe_block_count_t io_size)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t        *rdgen_context_p = &bogel_test_context[0];
    fbe_u8_t                        scsi_opcode_to_inject_on = requested_scsi_opcode_to_inject_on;
    fbe_rdgen_operation_t           rdgen_operation = FBE_RDGEN_OPERATION_INVALID;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL; 
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_class_id_t                  class_id;
    fbe_lba_t                       user_space_start_lba = FBE_LBA_INVALID;
    fbe_block_count_t               blocks = 0;
    fbe_lba_t                       start_lba = FBE_LBA_INVALID;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                       default_offset = FBE_LBA_INVALID;

    /* We must inject to the same pvd block that the error was injected to.
     */
    user_space_start_lba = bogel_user_space_lba_to_inject_to;
    blocks = io_size;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (bogel_b_is_DE542_fixed == FBE_FALSE)
    {
        if (requested_scsi_opcode_to_inject_on != FBE_SCSI_VERIFY_16)
        {
            //mut_printf(MUT_LOG_LOW, "%s: Due to DE542 change opcode from: 0x%02x to: 0x%02x", 
            //           __FUNCTION__, requested_scsi_opcode_to_inject_on, FBE_SCSI_VERIFY_16);
            scsi_opcode_to_inject_on = FBE_SCSI_VERIFY_16;
        }
        MUT_ASSERT_TRUE(scsi_opcode_to_inject_on == FBE_SCSI_VERIFY_16);
    }

    /* Determine the rdgen operation based on the SCSI opcode
     */
    switch (scsi_opcode_to_inject_on)
    {
        case FBE_SCSI_READ_6:
        case FBE_SCSI_READ_10:
        case FBE_SCSI_READ_12:
        case FBE_SCSI_READ_16:
            /* Generate read only requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_READ_ONLY;
            break;
        case FBE_SCSI_VERIFY_10:
        case FBE_SCSI_VERIFY_16:
            /* Generate verify only */
            rdgen_operation = FBE_RDGEN_OPERATION_VERIFY;
            break;
        case FBE_SCSI_WRITE_6:
        case FBE_SCSI_WRITE_10:
        case FBE_SCSI_WRITE_12:
        case FBE_SCSI_WRITE_16:
            /* Generate write only */
            rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
            break;
        case FBE_SCSI_WRITE_SAME_10:
        case FBE_SCSI_WRITE_SAME_16:
            /* Generate zero requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_ZERO_ONLY;
            break;
        case FBE_SCSI_WRITE_VERIFY_10:
        case FBE_SCSI_WRITE_VERIFY_16:
            /* Generate write verify requests. */
            rdgen_operation = FBE_RDGEN_OPERATION_WRITE_VERIFY;
            break;
        default:
            /* Unsupported opcode */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported SCSI opcode: 0x%x",
                       __FUNCTION__, scsi_opcode_to_inject_on);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }


    /* Get the vd object id of the position to force into proactive copy
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    drive_to_spare_p = &rg_config_p->rg_disk_set[sparing_position];
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, sparing_position, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    /* Get the pvd to issue the I/O to
     */
    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE));

    /* Verify the class-id of the object to be provision drive
     */
    pvd_object_id = edge_info.server_id;
    status = fbe_api_get_object_class_id(pvd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);

    /*! @note PVD Sniff could automatically trigger copy.
     */
    if ((status != FBE_STATUS_OK)                  || 
        (class_id != FBE_CLASS_ID_PROVISION_DRIVE)    )  
    {
        if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
        {
            /* Sniff will trigger EOL.
            */
            start_lba = user_space_start_lba + bogel_default_offset;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                       __FUNCTION__, rdgen_operation, start_lba, blocks);
            return FBE_STATUS_OK;
        }
        MUT_ASSERT_TRUE((status == FBE_STATUS_OK) && (class_id == FBE_CLASS_ID_PROVISION_DRIVE));
    }

    /* Get default offset to locate lba to issue request to.
     */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    /*! @note PVD Sniff could automatically trigger copy.
     */
    if (status != FBE_STATUS_OK)
    {
        if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
        {
            /* Sniff will trigger EOL.
            */
            start_lba = user_space_start_lba + bogel_default_offset;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                       __FUNCTION__, rdgen_operation, start_lba, blocks);
            return FBE_STATUS_OK;
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    default_offset = provision_drive_info.default_offset;
    start_lba = user_space_start_lba + default_offset;

    /* Send a single I/O to each PVD.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Issue rdgen op: %d to pvd: 0x%x lba: 0x%llx blks: 0x%llx ", 
               __FUNCTION__, rdgen_operation, pvd_object_id, start_lba, blocks);
    status = fbe_api_rdgen_send_one_io(rdgen_context_p, 
                                       pvd_object_id,       /* Issue to a single pvd */
                                       FBE_CLASS_ID_INVALID,
                                       FBE_PACKAGE_ID_SEP_0,
                                       rdgen_operation,
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       start_lba, /* lba */
                                       blocks, /* blocks */
                                       FBE_RDGEN_OPTIONS_STRIPE_ALIGN,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    /*! @note PVD Sniff could automatically trigger copy.
     */
    if (status != FBE_STATUS_OK)
    {
        if (rdgen_operation == FBE_RDGEN_OPERATION_VERIFY)
        {
            /* Sniff will trigger EOL.
            */
            start_lba = user_space_start_lba + bogel_default_offset;
            mut_printf(MUT_LOG_TEST_STATUS, "%s: rdgen op: %d pvd Sniff to lba: 0x%llx blks: 0x%llx triggers copy.", 
                       __FUNCTION__, rdgen_operation, start_lba, blocks);
            return FBE_STATUS_OK;
        }
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Return the execution status.
     */
    return status;
}
/********************************************************
 * end bogel_run_io_to_start_copy()
 ********************************************************/

/*!***************************************************************************
 *          bogel_io_error_trigger_proactive_sparing()
 *****************************************************************************
 *
 * @brief   If the test isn't running I/O but they want to trigger a proactive
 *          copy, this method is invoked to run the specified I/O type which
 *          will trigger the copy.
 *
 * @param   rg_config_p - Pointer to raid group array to locate pvds
 * @param   sparing_position - Position that is being copied
 * @param   source_edge_index - Source vd edge index to send I/O to
 * @param   requested_scsi_opcode_to_inject_on - SCSI operation that is being injected to
 * @param   io_size - Fixed number of blocks per I/O
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  10/12/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void bogel_io_error_trigger_proactive_sparing(fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_u32_t        drive_pos,
                                                          fbe_edge_index_t source_edge_index)
{
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t    record_handle_p;
    fbe_object_id_t                                 rg_object_id;
    fbe_object_id_t                                 vd_object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_get_block_edge_info_t                   block_edge_info;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: inject scsi errors on bus:0x%x, encl:0x%x, slot:0x%x to trigger spare swap-in.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    /* Invoke the method to initialize and populate the (1) error record
     */
    status = bogel_populate_io_error_injection_record(rg_config_p,
                                                                  drive_pos,
                                                                  FBE_SCSI_WRITE_16,
                                                                  &record,
                                                                  &record_handle_p,
                                                                  FBE_SCSI_SENSE_KEY_RECOVERED_ERROR,
                                                                  FBE_SCSI_ASC_PFA_THRESHOLD_REACHED,
                                                                  FBE_SCSI_ASCQ_GENERAL_QUALIFIER);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* stop the job service recovery queue to hold the job service swap commands. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* inject the scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Start I/O to force EOL.*/
    status = bogel_run_io_to_start_copy(rg_config_p, drive_pos, source_edge_index,
                                                      FBE_SCSI_WRITE_16,
                                                      bogel_user_space_blocks_to_inject);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the eol bit gets set for the pdo and it gets propogated to virtual drive object. */
    status = fbe_api_wait_for_block_edge_path_attribute(vd_object_id, source_edge_index, FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE, FBE_PACKAGE_ID_SEP_0, 60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s:EOL bit is set on the edge between vd (0x%x) and pvd (0x%x)",
               __FUNCTION__, vd_object_id, block_edge_info.server_id);

    /* stop injecting scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* restart the job service queue to proceed with job service swap command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);
    return;
}
/************************************************************
 * end bogel_io_error_trigger_proactive_sparing()
 ************************************************************/

/*!***************************************************************************
 *          bogel_io_error_fault_drive()
 *****************************************************************************
 *
 * @brief   If the test isn't running I/O but they want to trigger a proactive
 *          copy, this method is invoked to run the specified I/O type which
 *          will trigger the copy.
 *
 * @param   rg_config_p - Pointer to raid group array to locate pvds
 * @param   sparing_position - Position that is being copied
 * @param   source_edge_index - Source vd edge index to send I/O to
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  03/13/2013  Rob Foley  - Created.
 *
 *****************************************************************************/
static void bogel_io_error_fault_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t        drive_pos,
                                                   fbe_edge_index_t source_edge_index)
{
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t    record_handle_p;
    fbe_object_id_t                                 rg_object_id;
    fbe_object_id_t                                 vd_object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_get_block_edge_info_t                   block_edge_info;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: inject scsi errors on bus:0x%x, encl:0x%x, slot:0x%x to trigger spare swap-in.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    /* Invoke the method to initialize and populate the (1) error record
     */
    status = bogel_populate_io_error_injection_record(rg_config_p,
                                                                  drive_pos,
                                                                  FBE_SCSI_WRITE_16,
                                                                  &record,
                                                                  &record_handle_p,
                                                                  FBE_SCSI_SENSE_KEY_MEDIUM_ERROR,
                                                                  FBE_SCSI_ASC_DEFECT_LIST_ERROR,
                                                                  FBE_SCSI_ASCQ_GENERAL_QUALIFIER);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* stop the job service recovery queue to hold the job service swap commands. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* inject the scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Start I/O to force EOL.*/
    status = bogel_run_io_to_start_copy(rg_config_p, drive_pos, source_edge_index,
                                                      FBE_SCSI_WRITE_16,
                                                      bogel_user_space_blocks_to_inject);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the eol bit gets set for the pdo and it gets propogated to virtual drive object. */
    status = fbe_api_wait_for_block_edge_path_attribute(vd_object_id, source_edge_index, 
                                                        (FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE | 
                                                         FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT), FBE_PACKAGE_ID_SEP_0, 60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s:EOL bit is set on the edge between vd (0x%x) and pvd (0x%x) object.",
               __FUNCTION__, vd_object_id, block_edge_info.server_id);

    /* stop injecting scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* restart the job service queue to proceed with job service swap command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);
    return;
}
/************************************************************** 
 * end bogel_io_error_fault_drive()
 **************************************************************/


void bogel_wait_for_proactive_spare_swap_in(fbe_object_id_t vd_object_id,
                                                   fbe_edge_index_t spare_edge_index,
                                                   fbe_test_raid_group_disk_set_t * spare_location_p)
{
    fbe_api_get_block_edge_info_t           block_edge_info;
    fbe_status_t                            status;
    fbe_virtual_drive_configuration_mode_t  expected_mirror_config_mode;
    fbe_virtual_drive_configuration_mode_t  current_configuration_mode;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;

    /* Determine active SP for copy.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If this is not the active change it.
     */
    if (this_sp != active_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    if(spare_edge_index == 0) {
        expected_mirror_config_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE;
    }
    else if(spare_edge_index == 1) {
        expected_mirror_config_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE;
    }

    /* verify that Proactive spare gets swapped-in and configuration mode gets changed to mirror. */
    status = fbe_api_wait_for_virtual_drive_configuration_mode(vd_object_id,
                                                               expected_mirror_config_mode, 
                                                               &current_configuration_mode,
                                                               BOGEL_TEST_SWAP_WAIT_TIME_MS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* proactive spare gets swapped-in, get the spare edge information. */
    status = fbe_api_get_block_edge_info(vd_object_id, spare_edge_index, &block_edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_object_lifecycle_state(block_edge_info.server_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object ready failed");
    status = fbe_api_provision_drive_get_location(block_edge_info.server_id,
                                                   &(spare_location_p->bus),
                                                   &(spare_location_p->enclosure),
                                                   &(spare_location_p->slot));
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If this is not the active change it back.
     */
    if (this_sp != active_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "%s: proactive spare swapped in - vd: 0x%x dest: %d pvd:0x%x(%d_%d_%d)",
               __FUNCTION__, vd_object_id, spare_edge_index, block_edge_info.server_id, 
               spare_location_p->bus, spare_location_p->enclosure, spare_location_p->slot);
    return;
}

/*!***************************************************************************
 *          bogel_wait_for_in_progress_request_to_complete()
 *****************************************************************************
 *
 * @brief   Wait for the previous copy operation to complete before proceeding.
 *
 * @param   vd_object_id - Virtual drive object id to wait for
 * @param   timeout_ms - Timeout in milliseconds
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  04/18/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t bogel_wait_for_in_progress_request_to_complete(fbe_object_id_t vd_object_id,
                                                                                 fbe_u32_t timeout_ms)
{
    fbe_status_t                    status;
    fbe_u32_t                       total_ms_waited = 0;
    fbe_u32_t                       wait_interval_ms = 100;
    fbe_api_virtual_drive_get_info_t vd_info;


    /* Wait until the request is complete or timeout
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    while ((((vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
             (vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    ) ||
            (vd_info.b_request_in_progress == FBE_TRUE)                                               ) &&
           (total_ms_waited < timeout_ms)                                                                    )
    {
        /* Wait until complete or timeout.
         */
        status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait 100ms
         */
        total_ms_waited += wait_interval_ms;
        fbe_api_sleep(wait_interval_ms);
    }

    /* Check if complete
     */
    if (((vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
         (vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    ) ||
        (vd_info.b_request_in_progress == FBE_TRUE)                                               )
    {
        /* Timedout
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s vd obj: 0x%x mode: %d in-progress: %d did not complete in: %d ms", 
                   __FUNCTION__, vd_object_id, vd_info.configuration_mode, vd_info.b_request_in_progress, total_ms_waited);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Success
     */
    return FBE_STATUS_OK;
}
/********************************************************************
 * end bogel_wait_for_in_progress_request_to_complete()
 ********************************************************************/

void bogel_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
                                                        fbe_edge_index_t source_edge_index)
{
    fbe_status_t                            status;
    fbe_virtual_drive_configuration_mode_t  expected_configuration_mode;
    fbe_virtual_drive_configuration_mode_t  current_configuration_mode;

    if(source_edge_index == 0) {
        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE;
    }
    else if(source_edge_index == 1) {
        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE;
    }
    else {
        /* source edge indes can be only zero or one. */
        MUT_ASSERT_TRUE(0);
    }

    /* verify that Proactive spare gets swapped-in and configuration mode gets changed to mirror. */
    status = fbe_api_wait_for_virtual_drive_configuration_mode(vd_object_id,
                                                               expected_configuration_mode,
                                                               &current_configuration_mode,
                                                               BOGEL_TEST_SWAP_WAIT_TIME_MS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_block_edge_path_state(vd_object_id, source_edge_index, FBE_PATH_STATE_INVALID, FBE_PACKAGE_ID_SEP_0, 5000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:source drive is swapped-out, vd_object_id:0x%x.\n", __FUNCTION__, vd_object_id);
    return;
}

static void bogel_wait_for_destination_drive_swap_out(fbe_object_id_t vd_object_id,
                                                    fbe_edge_index_t dest_edge_index)
{
    fbe_status_t   status;
    fbe_virtual_drive_configuration_mode_t  expected_configuration_mode;
    fbe_virtual_drive_configuration_mode_t  current_configuration_mode;

    if(dest_edge_index == 0) {
        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE;
    }
    else if(dest_edge_index == 1) {
        expected_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE;
    }
    else {
        /* source edge indes can be only zero or one. */
        MUT_ASSERT_TRUE(0);
    }

    /* verify that Proactive spare gets swapped-in and configuration mode gets changed to mirror. */
    status = fbe_api_wait_for_virtual_drive_configuration_mode(vd_object_id,
                                                               expected_configuration_mode,
                                                               &current_configuration_mode,
                                                               BOGEL_TEST_SWAP_WAIT_TIME_MS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_block_edge_path_state(vd_object_id, dest_edge_index, FBE_PATH_STATE_INVALID, FBE_PACKAGE_ID_SEP_0, 5000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:proactive spare drive is swapped-out, vd_object_id:0x%x.\n", __FUNCTION__, vd_object_id);
    return;
}

void bogel_wait_for_proactive_copy_completion(fbe_object_id_t    vd_object_id)
{
    fbe_status_t                    status;              
    fbe_u32_t                       retry_count;
    fbe_u32_t                       max_retries;         
    fbe_api_virtual_drive_get_info_t virtual_drive_info;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;


    /* set the max retries in a local variable. */
    max_retries = BOGEL_TEST_MAX_RETRIES;

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /*  loop until the number of chunks marked NR is 0 for the drive that we are rebuilding. */
    for (retry_count = 0; retry_count < max_retries; retry_count++)
    {
        /*  get the virtual drive information */
        status = fbe_api_virtual_drive_get_info(vd_object_id, &virtual_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*  Use the `is copy complete' flag to determine if the copy is complete
         */
        if ((virtual_drive_info.b_is_copy_complete == FBE_TRUE)   ||
            (virtual_drive_info.vd_checkpoint == FBE_LBA_INVALID)    )
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: vd obj: 0x%x chkpt: 0x%llx copy complete\n", 
                       __FUNCTION__, vd_object_id, virtual_drive_info.vd_checkpoint);
            return;
        }

        /*  wait before trying again. */
        fbe_api_sleep(BOGEL_TEST_RETRY_TIME);
    }

    /*  The rebuild operation did not finish within the time limit, assert here. */
    MUT_ASSERT_TRUE(0);
    return;
}

static fbe_status_t bogel_wait_for_copy_job_to_complete(fbe_object_id_t vd_object_id)
{
    fbe_status_t                    status;              
    fbe_u32_t                       retry_count;
    fbe_u32_t                       max_retries;         
    fbe_api_virtual_drive_get_info_t virtual_drive_info;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;


    /* set the max retries in a local variable. */
    max_retries = BOGEL_TEST_MAX_RETRIES;

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /*  loop until the number of chunks marked NR is 0 for the drive that we are rebuilding. */
    for (retry_count = 0; retry_count < max_retries; retry_count++)
    {
        /*  get the virtual drive information */
        status = fbe_api_virtual_drive_get_info(vd_object_id, &virtual_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*  Use the `job in progress' flag
         */
        if (virtual_drive_info.b_request_in_progress == FBE_FALSE)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: vd obj: 0x%x chkpt: 0x%llx copy job complete\n", 
                       __FUNCTION__, vd_object_id, virtual_drive_info.vd_checkpoint);
            return FBE_STATUS_OK;
        }

        /*  wait before trying again. */
        fbe_api_sleep(BOGEL_TEST_RETRY_TIME);
    }

    /*  The copy operation did not finish within the time limit, assert here. */
    return FBE_STATUS_GENERIC_FAILURE;
}


/******************************************* 
 * BOGEL TESTS START HERE !!
 ********************************************/


/*!***************************************************************************
 *          bogel_test_user_copy_with_expected_failure()
 ***************************************************************************** 
 * 
 * @brief   This test case is used to to make sure the spare validation is 
 *          always triggered while doing a swap.  Basically, we will try to 
 *          copy from a drive with larger capacity to a drive with smaller 
 *          capacity and we expected the spare validation will fail this job 
 *          and swap won't happen. Attempt a user copy on a broken raid group.  
 *          It should fail and generate the proper event log.
 *
 * @param   rg_config_p - Pointer to raid group to force to shutdown/broken
 * @param   drive_pos - raid group position to attempt copy on
 *
 * @return  status - FBE_STATUS_OK.
 *
 * @author
 *  10/26/2012  Arun Selvapillai    - Updated.
 *
 *****************************************************************************/
static void bogel_test_user_copy_with_expected_failure(fbe_test_rg_configuration_t *rg_config_p, 
                                                                     fbe_u32_t drive_pos)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         vd_object_id;
    fbe_object_id_t                         source_object_id;
    fbe_object_id_t                         dest_object_id;
    fbe_edge_index_t                        source_edge_index;
    fbe_edge_index_t                        dest_edge_index;
    fbe_api_rdgen_context_t                *rdgen_context_p = &bogel_test_context[0];
    fbe_provision_drive_copy_to_status_t    copy_status = FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR;
    fbe_test_raid_group_disk_set_t          source_drive_location;
    fbe_test_raid_group_disk_set_t          dest_drive_location;
    fbe_transport_connection_target_t       target_invoked_on = FBE_TRANSPORT_SP_A;
    fbe_transport_connection_target_t       peer_sp = FBE_TRANSPORT_SP_B;
    fbe_api_terminator_device_handle_t      pulled_drive_handle;
    fbe_api_provision_drive_info_t          provision_drive_info;
    fbe_u32_t                               retry_count = 0;

    /* Get the local SP 
     */
    target_invoked_on = fbe_api_transport_get_target_server();
    peer_sp = (target_invoked_on == FBE_TRANSPORT_SP_A) ? FBE_TRANSPORT_SP_B : FBE_TRANSPORT_SP_A;

    /* Get the source disk.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &source_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    source_drive_location = rg_config_p->rg_disk_set[drive_pos];

    /* Get the raid group and virtual drive information.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Test 1: Select a destination drive with a capacity that is too small.  
     *         Drive 1_0_7 is the one we inserted separately with a smaller 
     *         capacity than the source drive.
     */
    dest_drive_location.bus = 1;
    dest_drive_location.enclosure = 0;
    dest_drive_location.slot = 7;
    mut_printf(MUT_LOG_TEST_STATUS, "==%s Attempt User Copy To - expected failure: dest too small(%d_%d_%d) - start", 
               __FUNCTION__, dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                            dest_drive_location.enclosure,
                                                            dest_drive_location.slot,
                                                            &dest_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

    /* Test 1: Attempt a copy operation to a drive that is too small.
     */
    status = fbe_api_copy_to_replacement_disk(source_object_id, dest_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_CAPACITY_MISMATCH);
    mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);
    bogel_check_event_log_msg(SEP_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY,                  
                                          FBE_TRUE, /* Message contains destination location*/ 
                                          &dest_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &source_drive_location);
    mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since destination is too small. event: 0x%x",
               SEP_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY);
    
    /* Test 2: Attempt copy on destination drive with EOL set.
     */
    status = bogel_get_valid_destination_drive(rg_config_p, &dest_drive_location);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                            dest_drive_location.enclosure,
                                                            dest_drive_location.slot,
                                                            &dest_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "==%s Attempt User Copy To - expected failure: EOL(%d_%d_%d) - start", 
               __FUNCTION__, dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot); 

    /* Test 2: Here we are triggering proactive sparing by setting end of life 
     *         state for the given drive.
     */
    status = fbe_api_provision_drive_set_eol_state(dest_object_id, FBE_PACKET_FLAG_NO_ATTRIB);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(dest_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    while ((provision_drive_info.end_of_life_state == FBE_FALSE) &&
           (retry_count < 200)                                     )
    {
        /* Wait a short time.
         */
        fbe_api_sleep(100);
        status = fbe_api_provision_drive_get_info(dest_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        retry_count++;
    }
    MUT_ASSERT_TRUE(provision_drive_info.end_of_life_state == FBE_TRUE);
    retry_count = 0;
    status = fbe_api_copy_to_replacement_disk(source_object_id, dest_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_DESTINATION_DRIVE_NOT_HEALTHY);
    mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);
    bogel_check_event_log_msg(SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY,                  
                                          FBE_TRUE, /* Message contains destination location*/ 
                                          &dest_drive_location,                                  
                                          FBE_FALSE, /* Message does notcontains original drive location */
                                          &source_drive_location);
    mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since destination is not healthy. event: 0x%x",
               SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY);

    /* Test 2: Clear EOL.
     */
    bogel_clear_eol(&dest_drive_location);
    
    /* Test 3: Set a drive fault on the destination.
     */          
    mut_printf(MUT_LOG_TEST_STATUS, "==%s Attempt User Copy To - expected failure: Drive Fault(%d_%d_%d) - start", 
               __FUNCTION__, dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot); 
    status = fbe_test_sep_util_set_drive_fault(dest_drive_location.bus,
                                               dest_drive_location.enclosure, 
                                               dest_drive_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
    status = fbe_api_copy_to_replacement_disk(source_object_id, dest_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_DESTINATION_DRIVE_NOT_HEALTHY);
    mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);
    bogel_check_event_log_msg(SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY,                  
                                          FBE_TRUE, /* Message contains destination location*/ 
                                          &dest_drive_location,                                  
                                          FBE_FALSE, /* Message does notcontains original drive location */
                                          &source_drive_location);
    mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since destination is not healthy. event: 0x%x",
               SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY);

    /* Clear drive fault.
     */
    status = fbe_test_sep_util_clear_drive_fault(dest_drive_location.bus,
                                                 dest_drive_location.enclosure,
                                                 dest_drive_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 4: If this is a dualsp test, remove the destination drive on the peer.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode() == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "==%s Attempt User Copy To - expected failure: SLF(%d_%d_%d) - start", 
               __FUNCTION__, dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot); 

        /* Set the target server to the peer and remove the destination drive.
         */
        fbe_api_transport_set_target_server(peer_sp);
        if (fbe_test_util_is_simulation())
        {
            /* remove the drive. */
            status = fbe_test_pp_util_pull_drive(dest_drive_location.bus,
                                                 dest_drive_location.enclosure,
                                                 dest_drive_location.slot,
                                                 &pulled_drive_handle); 
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        else
        {
            /* remove the drive on actual hardware. */
            status = fbe_test_pp_util_pull_drive_hw(dest_drive_location.bus,
                                                    dest_drive_location.enclosure,
                                                    dest_drive_location.slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Now go back to original
         */
        fbe_api_transport_set_target_server(target_invoked_on);

        /* Validate that we go to SLF.
         */
        status = fbe_api_provision_drive_get_info(dest_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
        while ((provision_drive_info.slf_state == FBE_PROVISION_DRIVE_SLF_NONE) &&
               (retry_count < 200)                                                 )
        {
            /* Wait a short time.
             */
            fbe_api_sleep(100);
            status = fbe_api_provision_drive_get_info(dest_object_id, &provision_drive_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            retry_count++;
        }
        MUT_ASSERT_TRUE(provision_drive_info.slf_state == FBE_PROVISION_DRIVE_SLF_PEER_SP);   
        retry_count = 0;

        /* Attempt the copy.
         */
        status = fbe_api_copy_to_replacement_disk(source_object_id, dest_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_DESTINATION_DRIVE_NOT_HEALTHY);
        mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);
        bogel_check_event_log_msg(SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY,                  
                                          FBE_TRUE, /* Message contains destination location*/ 
                                          &dest_drive_location,                                  
                                          FBE_FALSE, /* Message does notcontains original drive location */
                                          &source_drive_location);
        mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since destination is not healthy. event: 0x%x",
                   SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY);

        /* Now re-insert the drive 
         */
        fbe_api_transport_set_target_server(peer_sp);
        if (fbe_test_util_is_simulation())
        {
            status = fbe_test_pp_util_reinsert_drive(dest_drive_location.bus, 
                                                     dest_drive_location.enclosure, 
                                                     dest_drive_location.slot,
                                                     pulled_drive_handle);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        else
        {
            status = fbe_test_pp_util_reinsert_drive_hw(dest_drive_location.bus, 
                                                        dest_drive_location.enclosure, 
                                                        dest_drive_location.slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Now go back to original
         */
        fbe_api_transport_set_target_server(target_invoked_on);

        /* Validate that SLF is cleared
         */
        status = fbe_api_provision_drive_get_info(dest_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
        while ((provision_drive_info.slf_state != FBE_PROVISION_DRIVE_SLF_NONE) &&
               (retry_count < 200)                                                 )
        {
            /* Wait a short time.
             */
            fbe_api_sleep(100);
            status = fbe_api_provision_drive_get_info(dest_object_id, &provision_drive_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            retry_count++;
        }
        MUT_ASSERT_TRUE(provision_drive_info.slf_state == FBE_PROVISION_DRIVE_SLF_NONE);   
        retry_count = 0;

    } /* end if this is a dualsp test */

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    mut_printf(MUT_LOG_TEST_STATUS, "==%s Completed successfully. ==", __FUNCTION__);
    return;
}
/****************************************************************
 * end bogel_test_user_copy_with_expected_failure()
 ****************************************************************/

/*!***************************************************************************
 *          bogel_test_user_copy_on_broken_raid_group()
 ***************************************************************************** 
 * 
 * @brief   Attempt a user copy on a broken raid group.  It should fail and
 *          generate the proper event log.
 *
 * @param   rg_config_p - Pointer to raid group to force to shutdown/broken
 *
 * @return  status - FBE_STATUS_OK.
 *
 * @author
 *  09/11/2012  Ron Proulx  - Created.
 *****************************************************************************/
static fbe_status_t bogel_test_user_copy_on_broken_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         vd_object_id;
    fbe_object_id_t                         dest_object_id;
    fbe_edge_index_t                        source_edge_index;
    fbe_edge_index_t                        dest_edge_index;
    fbe_api_rdgen_context_t                *rdgen_context_p = &bogel_test_context[0];
    fbe_object_id_t                         src_object_id;
    fbe_provision_drive_copy_to_status_t    copy_status;
    fbe_u32_t                               first_position_to_remove;
    fbe_u32_t                               second_position_to_remove;
    fbe_u32_t                               third_position_to_remove = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t                               num_of_pos_to_remove = 2;
    fbe_u32_t                               position_to_copy;
    fbe_test_raid_group_disk_set_t          dest_drive_location;
    fbe_transport_connection_target_t       target_invoked_on = FBE_TRANSPORT_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "User copy on broken raid group started!!!\n");

    /* Determine the position that will be removed.
     */
    if (((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) &&
         (rg_config_p->width     == 3)                            ) ||
        (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6)          )
    {
        first_position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        second_position_to_remove = first_position_to_remove + 1;
        third_position_to_remove = second_position_to_remove + 1;
        position_to_copy = third_position_to_remove + 1;
        num_of_pos_to_remove = 3;
    }
    else
    {
        first_position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        second_position_to_remove = first_position_to_remove + 1;
        position_to_copy = second_position_to_remove + 1;
    }

    /* For RAID-1 1 + 1 attempt to copy from one of the removed positions
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
    {
        position_to_copy = second_position_to_remove;
    }

    /* Get the copy information before we cause the raid group to go broken.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position_to_copy].bus,
                                                            rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                            rg_config_p->rg_disk_set[position_to_copy].slot,
                                                            &src_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Move all the hot-spares to the automatic sparing pool.
     */
    status = bogel_test_remove_hotspares_from_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now shutdown the raid group.
     */
    big_bird_remove_all_drives(rg_config_p, 1, num_of_pos_to_remove,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* raid group should be shutdown so we cannot write a background pattern*/

    /* Validate that the raid group is shutdown
     */
    MUT_ASSERT_TRUE(fbe_test_rg_is_broken(rg_config_p) == FBE_TRUE);

    /* Get a valid destination drive.
     */
    status = bogel_get_valid_destination_drive(rg_config_p, &dest_drive_location);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot,
                                                            &dest_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* generate a message */
    mut_printf(MUT_LOG_TEST_STATUS, "%s - Attempt Copy To from: %d_%d_%d (0x%x) to %d_%d_%d(0x%x) should fail.",
               __FUNCTION__,  
               rg_config_p->rg_disk_set[position_to_copy].bus, 
               rg_config_p->rg_disk_set[position_to_copy].enclosure, 
               rg_config_p->rg_disk_set[position_to_copy].slot, 
               src_object_id,
               dest_drive_location.bus,
               dest_drive_location.enclosure,
               dest_drive_location.slot,
               dest_object_id);

    /* trigger the copy back operation  */
    status = fbe_api_copy_to_replacement_disk(src_object_id, dest_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /*! @note On the RAID-1 we attempted to start a copy operation on a virutal
     *        that is not in the Ready state.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1)
    {
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN);
        mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);

        bogel_check_event_log_msg(SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY,                  
                                          FBE_FALSE, /* Message does not contain spare drive location */   
                                          &dest_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[position_to_copy]);                  

        mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the virtual drive is broken. event: 0x%x",
                   SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY);
    }
    else
    {
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_COPY_NOT_ALLOWED_TO_BROKEN_RAID_GROUP);
        mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);

        /*! @note Since the format of this event is:
         *        `Policy does not allow copy operations to a broken raid group disk:%d_%d_%d'
         *        There is an unexpected fourth parameter so we can use 
         *        `bogel_check_event_log_msg'.
         */
        bogel_check_event_log_msg(SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN,                  
                                              FBE_FALSE, /* Message does not contain spare drive location */   
                                              &dest_drive_location,                                  
                                              FBE_TRUE, /* Message contains original drive location */
                                              &rg_config_p->rg_disk_set[position_to_copy]);                  

        mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the raid group is broken. event: 0x%x",
                   SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN);
    }

    /* Move all the automatic spares back to the hot-spare pool.
     */
    status = bogel_test_add_hotspares_to_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Re-insert the removed drives.
     */
    big_bird_insert_all_drives(rg_config_p, 1, num_of_pos_to_remove,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, first_position_to_remove);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, second_position_to_remove);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* Since we forced the raid group to go broken we must wait for the luns to
     * become ready before attempting I/O.
     */
    target_invoked_on = fbe_api_transport_get_target_server();

    /* Wait for all lun objects of this raid group to be ready
     */
    status = fbe_test_sep_util_wait_for_all_lun_objects_ready(rg_config_p,
                                                              target_invoked_on);
    if (status != FBE_STATUS_OK)
    {
        /* Failure, break out and return the failure.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                   rg_config_p->raid_group_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /* Success.
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end bogel_test_user_copy_on_broken_raid_group()
 ************************************************************/

/*!***************************************************************************
 *          bogel_test_user_copy_on_degraded_raid_group()
 ***************************************************************************** 
 * 
 * @brief   Attempt a user copy on a degraded raid group.  It should fail and
 *          generate the proper event log.
 *
 * @param   rg_config_p - Pointer to raid group to degrade.
 *
 * @return  status - FBE_STATUS_OK.
 *
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 *****************************************************************************/
static fbe_status_t bogel_test_user_copy_on_degraded_raid_group(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         vd_object_id;
    fbe_object_id_t                         dest_object_id;
    fbe_edge_index_t                        source_edge_index;
    fbe_edge_index_t                        dest_edge_index;
    fbe_api_rdgen_context_t                *rdgen_context_p = &bogel_test_context[0];
    fbe_object_id_t                         src_object_id;
    fbe_provision_drive_copy_to_status_t    copy_status;
    fbe_u32_t                               position_to_remove;
    fbe_u32_t                               position_to_copy;
    fbe_test_raid_group_disk_set_t          dest_drive_location;
    fbe_raid_position_bitmask_t             expected_degraded_bitmask = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "User copy on degraded raid group started!!!\n");

    /* Determine the position that will be removed.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;
    expected_degraded_bitmask |= (1 << position_to_remove);

    /* Get the raid group information
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position_to_copy].bus,
                                                            rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                            rg_config_p->rg_disk_set[position_to_copy].slot,
                                                            &src_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Move all the hot-spares to the automatic sparing pool.
     */
    status = bogel_test_remove_hotspares_from_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now degraded the raid group.
     */
    big_bird_remove_all_drives(rg_config_p, 1, 1,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Validate that the raid group is degraded.
     */
    status = fbe_test_sep_util_wait_for_degraded_bitmask(rg_object_id, 
                                                         expected_degraded_bitmask, 
                                                         30000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get a valid destination drive.
     */
    status = bogel_get_valid_destination_drive(rg_config_p, &dest_drive_location);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus, 
                                                            dest_drive_location.enclosure, 
                                                            dest_drive_location.slot,
                                                            &dest_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* generate a message */

    mut_printf(MUT_LOG_TEST_STATUS, "%s - Attempt Copy To from: %d_%d_%d (0x%x) to %d_%d_%d(0x%x) should fail.",
               __FUNCTION__,  
               rg_config_p->rg_disk_set[position_to_copy].bus, 
               rg_config_p->rg_disk_set[position_to_copy].enclosure, 
               rg_config_p->rg_disk_set[position_to_copy].slot, 
               src_object_id,
               dest_drive_location.bus,
               dest_drive_location.enclosure,
               dest_drive_location.slot,
               dest_object_id);

    /* get the edge index of the source and destination drive based on configuration mode. */
    bogel_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* trigger the copy back operation  */
    status = fbe_api_copy_to_replacement_disk(src_object_id, dest_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED);
    mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);

    /*! @note Since the format of this event is:
     *        `Policy does not allow copy operations to a degraded raid group disk:%d_%d_%d'
     *        There is an unexpected fourth parameter so we can use 
     *        `bogel_check_event_log_msg'.
     */
    bogel_check_event_log_msg(SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED,                  
                                          FBE_FALSE, /* Message does not contain spare drive location */   
                                          &dest_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[position_to_copy]);                  

    mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the raid group is degraded. event: 0x%x",
               SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED);

    /* Move all the automatic spares back to the hot-spare pool.
     */
    status = bogel_test_add_hotspares_to_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Re-insert the removed drive.
     */
    big_bird_insert_all_drives(rg_config_p, 1, 1,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position_to_remove);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    bogel_verify_background_pattern(rdgen_context_p, BOGEL_TEST_ELEMENT_SIZE);

    /* Success.
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end bogel_test_user_copy_on_degraded_raid_group()
 ************************************************************/

/*!****************************************************************************
 * bogel_test_remove_hotspares_from_hotspare_pool()
 ******************************************************************************
 * @brief
 *  This method is used to temporarily remove the configured hot-spares from
 *  the hot-spare pool.
 *  
 *
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   num_of_spares_required - Number of spares that must be found
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 ******************************************************************************/
static fbe_status_t bogel_test_remove_hotspares_from_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                               fbe_u32_t num_of_spares_required)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_hs_configuration_t             hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t                               hs_index = 0;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_test_raid_group_disk_set_t          disk_to_skip = {1, 0, 7};
    fbe_u32_t                               local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
  
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Discover all the `unused drives'
     */
    fbe_test_sep_util_get_all_unused_pvd_location(&bogel_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    bogel_set_unused_drive_info(rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
                                              &disk_to_skip,
                                              num_of_spares_required);

    /* Now configure the available drives as `automatic spare' to remove
     * them from the hot-spare pool
     */
    for (hs_index = 0; hs_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; hs_index++)
    {
        /* 0_0_0 Is a special drive location that designates the end of the
         * drive list.
         */
        if ((rg_config_p->unused_disk_set[hs_index].bus == 0)       &&
            (rg_config_p->unused_disk_set[hs_index].enclosure == 0) &&
            (rg_config_p->unused_disk_set[hs_index].slot == 0)         )
        {
            /* End of list reached*/
            break;
        }
    
        /* Else populate the `hot-spare' configuration information.
         */
        hs_config[hs_index].block_size = 520;
        hs_config[hs_index].disk_set = rg_config_p->unused_disk_set[hs_index];
        hs_config[hs_index].hs_pvd_object_id = FBE_OBJECT_ID_INVALID;
    }
    if (hs_index < num_of_spares_required)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "*** %s rg type: %d no spares located ***", 
                   __FUNCTION__, rg_config_p->raid_type);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = fbe_test_sep_util_configure_automatic_spares(hs_config, hs_index);
    }

    return status;
}
/*******************************************************************
 * end bogel_test_remove_hotspares_from_hotspare_pool()
 *******************************************************************/

/*!****************************************************************************
 * bogel_test_add_hotspares_to_hotspare_pool()
 ******************************************************************************
 * @brief
 *  This method is used to place the hot-spares back into the hot-spare pool.
 *  
 *
 * @param   rg_config_p - Pointer to raid group add to unused array
 * @param   num_of_spares_required - Number of spare required.
 *
 * @return FBE_STATUS.
 *
 * @author
 *  5/12/2011 - Created. Vishnu Sharma
 ******************************************************************************/
static fbe_status_t bogel_test_add_hotspares_to_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                          fbe_u32_t num_of_spares_required)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_hs_configuration_t             hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t                               hs_index = 0;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_test_raid_group_disk_set_t          disk_to_skip = {1, 0, 7};
    fbe_u32_t                               local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
  
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Discover all the `unused drives'
     */
    fbe_test_sep_util_get_all_unused_pvd_location(&bogel_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    bogel_set_unused_drive_info(rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
                                              &disk_to_skip,
                                              num_of_spares_required);

    for (hs_index = 0; hs_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; hs_index ++)
    {
        /* 0_0_0 Is a special drive location that designates the end of the
         * drive list.
         */
        if ((rg_config_p->unused_disk_set[hs_index].bus == 0)       &&
            (rg_config_p->unused_disk_set[hs_index].enclosure == 0) &&
            (rg_config_p->unused_disk_set[hs_index].slot == 0)         )
        {
            /* End of list reached*/
            break;
        }

        /* Else populate the `hot-spare' configuration information.
         */
        hs_config[hs_index].block_size = 520;
        hs_config[hs_index].disk_set = rg_config_p->unused_disk_set[hs_index];
        hs_config[hs_index].hs_pvd_object_id = FBE_OBJECT_ID_INVALID;       
    }
    if (hs_index < num_of_spares_required)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "*** %s num spares required: %d spares found: %d***", 
                   __FUNCTION__, num_of_spares_required, hs_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = fbe_test_sep_util_reconfigure_hot_spares(hs_config, hs_index);
    }
    return status;
}

