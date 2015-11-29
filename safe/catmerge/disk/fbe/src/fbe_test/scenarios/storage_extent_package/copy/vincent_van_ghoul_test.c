/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * vincent_van_ghoul_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains copy operation (System Proactive Copy, User
 *  Copy and User Copy To) with I/O with reboot (both in single and dual-sp).
 *
 * HISTORY
 *  06/21/2012  Ron Proulx - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "sep_tests.h"
#include "sep_test_background_ops.h"
#include "sep_rebuild_utils.h"
#include "neit_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_test_configurations.h"
#include "fbe_test.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "sep_hook.h"

char * vincent_van_ghoul_short_desc = "This scenario test Copy Operations with SP reboots.";
char * vincent_van_ghoul_long_desc = 
    "\n"
    "\n"
    "This test verifies that when any copy operation is interrupted by a SP reboot the current or alternate\n"  
    "SP will re-start and complete the copy operation.\n\n"
    "\t 1) User driven Proactive Copy operation(fbe_cli):-\n"
    "\t\t - This scenario will initiate a Proactive Copy operation for specific drive using fbe_cli command (PS).\n"
    "\t\t - It will send the control packet to Config Service to start the Proactive Copy operation.\n"
    "\t\t - It will verify that Drive Sparing Service receives Proactive Spare Swap-In request using fbe_cli command.\n"
    "\t\t - It will verify that Drive Sparing Service has found appropriate Proactive Spare for the swap-in.\n"
    "\t\t - It will verify that VD object starts actual Proactive copy operation.\n"
    "\t\t - It will verify that it updates checkpoint regularly on Proactive Spare drive for every chunk it copies.\n"
    "\t\t - It will also verify that Proactive Copy operation is performed in LBA order (Decision might change in future).\n"
    "\t\t - It will verify that at the end of Proactive Copy completion VD object sends notification to Drive Swa service.\n" 
    "\t\t - It will verify that at the end of proactive copy Drive Swap service will transition to Hot Sparing by removing\n"
    "\t\t   edge between VD object and PVD object for Proactive Candidate drive.\n\n"
    "\t 2) Specific I/O error triggers Proactive Copy operation:-\n"
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
    "\t\t - Verifies that Drive Swap Service transition to Hot Sparing by removing edge with Proactive Candidate drive.\n"
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
    "\t\t - Verifies that Drive Swap Service transition to Hot Sparing by removing edge with Proactive Candidate drive.\n"
    "\t\t - Verifies that Drive Swap Service updates required configuration database.\n"
"\n\
Test Specific Switches:\n\
          -abort_cases_only - Only run the abort tests.\n\
\n\
Description last updated: 06/21/2012.\n";

/*! @def VINCENT_VAN_GHOUL_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY
 *
 *  @brief Number of `extra' drives to reserve for copy operation.
 */
#define VINCENT_VAN_GHOUL_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY    2

/*! @def VINCENT_VAN_GHOUL_INDEX_RESERVED_FOR_COPY
 *
 *  @brief Position reserved for copy operation
 */
#define VINCENT_VAN_GHOUL_INDEX_RESERVED_FOR_COPY              0

/*! @def VINCENT_VAN_GHOUL_INVALID_DISK_POSITION
 *
 *  @brief Invalid disk position.  Used when searching for a disk position and no disk is 
 *         found that meets the criteria.
 */
#define VINCENT_VAN_GHOUL_INVALID_DISK_POSITION ((fbe_u32_t) -1)

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_WAIT_SEC 60

/*!*******************************************************************
 * @def BIG_BIRD_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_WAIT_MSEC (1000 * VINCENT_VAN_GHOUL_WAIT_SEC)

/*!*******************************************************************
 * @def     VINCENT_VAN_GHOUL_FIXED_PATTERN
 *********************************************************************
 * @brief   rdgen fixed pattern to use
 *
 * @todo    Currently the only fixed pattern that rdgen supports is
 *          zeros.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_FIXED_PATTERN     FBE_RDGEN_PATTERN_ZEROS

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief The number of LUNs in our raid group.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 * @note Since we want copy to run for some time set this fairly large
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_CHUNKS_PER_LUN      (5)

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_NUM_DRIVES_TO_FORCE_SHUTDOWN
 *********************************************************************
 * @brief Number of drives to remove to force a shutdown
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_NUM_DRIVES_TO_FORCE_SHUTDOWN    (3)

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_RDGEN_WAIT_SECS
 *********************************************************************
 * @brief Delay to run I/O before starting next phase.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_RDGEN_WAIT_SECS          5

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_DRIVE_WAIT_MSEC
 *********************************************************************
 * @brief Delay to run I/O before starting next phase.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_DRIVE_WAIT_MSEC          60000

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_TEST_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of our raid groups.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_ELEMENT_SIZE 128 

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_MAX_IO_CONTEXT
 *********************************************************************
 * @brief  Five I/O threads. lg and sm.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_MAX_IO_CONTEXT 5

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_MAX_WIDTH
 *********************************************************************
 * @brief Max number of drives we will test with.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_MAX_WIDTH 16

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Number of blocks in small io
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_SMALL_IO_SIZE_BLOCKS 1024  

/*!*******************************************************************
 * @def VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS (VINCENT_VAN_GHOUL_MAX_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE // 4096 

/*!*******************************************************************
 * @var vincent_van_ghoul_io_msec_short
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `short' period of time.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_SHORT_IO_TIME_SECS  5
static fbe_u32_t vincent_van_ghoul_io_msec_short = (VINCENT_VAN_GHOUL_SHORT_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var vincent_van_ghoul_io_msec_long
 *********************************************************************
 * @brief This variable defined the number of milliseconds to run I/O
 *        for a `long' period of time.
 *
 *********************************************************************/
#define VINCENT_VAN_GHOUL_LONG_IO_TIME_SECS  30
static fbe_u32_t vincent_van_ghoul_io_msec_long = (VINCENT_VAN_GHOUL_LONG_IO_TIME_SECS * 1000);

/*!*******************************************************************
 * @var vincent_van_ghoul_rdgen_originating_sp
 *********************************************************************
 * @brief   By default rdgen always checks that the data read originated
 *          (i.e. was written on) the same SP.  But since this test can
 *          and will write the data on a SP that is different than the
 *          SP it read from, we save the `write' (a.k.a. originating)
 *          SP when the background pattern is written.
 *
 *********************************************************************/
static fbe_rdgen_sp_id_t vincent_van_ghoul_rdgen_originating_sp = FBE_DATA_PATTERN_SP_ID_INVALID;

/*!*******************************************************************
 * @var vincent_van_ghoul_pause_params
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t vincent_van_ghoul_pause_params = { 0 };

/*!*******************************************************************
 * @var vincent_van_ghoul_debug1_hooks
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t vincent_van_ghoul_debug1_hooks = { 0 };

/*!*******************************************************************
 * @var vincent_van_ghoul_debug2_hooks
 *********************************************************************
 * @brief This variable contains any values that need to be passed to
 *        the reboot methods.
 *
 *********************************************************************/
static fbe_sep_package_load_params_t vincent_van_ghoul_debug2_hooks = { 0 };

/*!*******************************************************************
 * @var vincent_van_ghoul_b_disable_shutdown
 *********************************************************************
 * @brief This variable controls of shutdown tests are enabled for 
 *        certain or all raid groups.
 *
 *********************************************************************/
static fbe_bool_t vincent_van_ghoul_b_disable_shutdown = FBE_TRUE;

/*!***************************************************************************
 * @var     vincent_van_ghoul_b_are_non_simple_tests_disabled_due_to_DE572
 *****************************************************************************
 * @brief   Determines if only simple tests are enabled to to the fact that 
 *          other tests are incomplete.
 *
 * @todo    Fix the issues preventing other tests to execute.
 *
 *****************************************************************************/
static fbe_bool_t vincent_van_ghoul_b_are_non_simple_tests_disabled_due_to_DE572 = FBE_TRUE; /*! Currently advanced testing is disabled */

/*!***************************************************************************
 * @var     vincent_van_ghoul_b_does_reboot_cause_path_failure
 *****************************************************************************
 * @brief   Determines if reboots can cause virtual drive downstream (a.k.a
 *          pvd) path failures.  If this is the case we need to extend the
 *          `trigger spare timer' so that we don't abort the copy.
 *
 * @note    Fix sep_destroy() to take out the CMI before the topology.
 *
 *****************************************************************************/
static fbe_bool_t vincent_van_ghoul_b_does_reboot_cause_path_failure = FBE_TRUE;            /*! Currently reboots cause path failures */

/*!*******************************************************************
 * @var vincent_van_ghoul_threads
 *********************************************************************
 * @brief Number of threads we will run for I/O.
 *
 *********************************************************************/
fbe_u32_t vincent_van_ghoul_threads = 5;

/*!*******************************************************************
 * @var vincent_van_ghoul_light_threads
 *********************************************************************
 * @brief Number of threads we will run for light I/O.
 *
 *********************************************************************/
fbe_u32_t vincent_van_ghoul_light_threads = 2;

/*!*******************************************************************
 * @var vincent_van_ghoul_b_debug_enable
 *********************************************************************
 * @brief Determines if debug should be enabled or not
 *
 *********************************************************************/
fbe_bool_t vincent_van_ghoul_b_debug_enable = FBE_FALSE;

/*!*******************************************************************
 * @var vincent_van_ghoul_library_debug_flags
 *********************************************************************
 * @brief Default value of the raid library debug flags to set
 *
 *********************************************************************/
fbe_u32_t vincent_van_ghoul_library_debug_flags = (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING      | 
                                            FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING |
                                            FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING  |
                                            FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING  );

/*!*******************************************************************
 * @var vincent_van_ghoul_object_debug_flags
 *********************************************************************
 * @brief Default value of the raid group object debug flags to set
 *
 *********************************************************************/
fbe_u32_t vincent_van_ghoul_object_debug_flags = (FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING);

/*!*******************************************************************
 * @var vincent_van_ghoul_enable_state_trace
 *********************************************************************
 * @brief Default value of enabling state trace
 *
 *********************************************************************/
fbe_bool_t vincent_van_ghoul_enable_state_trace = FBE_TRUE;

/*!*******************************************************************
 * @var     vincent_van_ghoul_dest_array
 *********************************************************************
 * @brief   Arrays of destination positions to spare (for user copy).
 *
 *********************************************************************/
static fbe_test_raid_group_disk_set_t vincent_van_ghoul_dest_array[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = { 0 };

/*!*******************************************************************
 * @var vincent_van_ghoul_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t vincent_van_ghoul_test_contexts[VINCENT_VAN_GHOUL_LUNS_PER_RAID_GROUP * VINCENT_VAN_GHOUL_MAX_IO_CONTEXT];

/*!*******************************************************************
 * @var vincent_van_ghoul_raid_groups_extended
 *********************************************************************
 * @brief Test config for raid test level 1 and greater.
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifndef __SAFE__
fbe_test_rg_configuration_t vincent_van_ghoul_raid_groups_extended[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = 
#else
fbe_test_rg_configuration_t vincent_van_ghoul_raid_groups_extended[] = 
#endif /* __SAFE__ SAFEMESS - shrink table/executable size */
{
    /* width,                       capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT,  FBE_TEST_RG_CONFIG_RANDOM_TAG,    520,            0,          0},
    {3,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,            1,          0},
    {4,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,   520,            2,          1},
    {5,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID3,  FBE_CLASS_ID_PARITY,    520,            3,          1},
    {6,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,            4,          0},
    {5,                             0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            5,          1},
    /* RAID-0 configurations (not supported !).   */
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var vincent_van_ghoul_raid_groups_qual
 *********************************************************************
 * @brief Test config for raid test level 0 (default test level).
 *
 * @note  Currently we limit the number of different raid group
 *        configurations of each type due to memory constraints.
 *********************************************************************/
#ifndef __SAFE__
fbe_test_rg_configuration_t vincent_van_ghoul_raid_groups_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT] = 
#else
fbe_test_rg_configuration_t vincent_van_ghoul_raid_groups_qual[] = 
#endif /* __SAFE__ SAFEMESS - shrink table/executable size */
{
    /* width,                       capacity    raid type,                                  class,                  block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT,  FBE_TEST_RG_CONFIG_RANDOM_TAG,    520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************************
 *          vincent_van_ghoul_validate_active_sp()
 *****************************************************************************
 *
 * @brief   Since this is a reboot test the Active SP of the virtual drive can
 *          can change as the test execute.  Therefore each test should get
 *          the SP information at the start of the test.
 *
 * @param   rg_config_p - Pointer to firste raid group test information
 * @param   b_dualsp_p - Pointer to the dualsp bool
 * @param   this_sp_p - Pointer to this_sp
 * @param   other_sp_p - Pointer to other_sp
 * @param   active_sp_p - Pointer to active_sp
 * @param   passive_sp_p - Pointer to passive_sp
 *
 * @return  status
 *
 * @author
 *  07/05/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_get_sp_info(fbe_test_rg_configuration_t * const rg_config_p,
                                                  fbe_bool_t *b_dualsp_p,
                                                  fbe_sim_transport_connection_target_t *this_sp_p,
                                                  fbe_sim_transport_connection_target_t *other_sp_p,
                                                  fbe_sim_transport_connection_target_t *active_sp_p,
                                                  fbe_sim_transport_connection_target_t *passive_sp_p)

{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               position_to_copy = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         vd_object_id;
    
    /* Get the dualsp info */
    *b_dualsp_p = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @note All virtual drives must be active on the same SP as the job service.
     *        Therefore we only need to get the active SP for (1) virtual drive.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    position_to_copy = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
    status = fbe_test_sep_util_get_active_passive_sp(vd_object_id, active_sp_p, passive_sp_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* Get `this', `other' and `active' SP.
     */
    *this_sp_p = fbe_api_sim_transport_get_target_server();
    *other_sp_p = (*this_sp_p == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    return status;
}
/*********************************************
 * end vincent_van_ghoul_get_sp_info()
 *********************************************/

/*!**************************************************************
 * vincent_van_ghoul_set_trace_level()
 ****************************************************************
 * @brief
 *  Cleanup the scooby_doo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/09/2012  Ron Proulx  - Created
 *
 ****************************************************************/
static void vincent_van_ghoul_set_trace_level(fbe_trace_type_t trace_type, fbe_u32_t id, fbe_trace_level_t level)
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

/*!**************************************************************
 * vincent_van_ghoul_set_io_seconds()
 ****************************************************************
 * @brief
 *  Set the io seconds for this test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  5/19/2010 - Created. Rob Foley
 *
 ****************************************************************/
void vincent_van_ghoul_set_io_seconds(void)
{
    fbe_u32_t long_io_time_seconds = fbe_test_sep_util_get_io_seconds();

    if (long_io_time_seconds >= VINCENT_VAN_GHOUL_LONG_IO_TIME_SECS)
    {
        vincent_van_ghoul_io_msec_long = (long_io_time_seconds * 1000);
    }
    else
    {
        vincent_van_ghoul_io_msec_long  = ((fbe_random() % VINCENT_VAN_GHOUL_LONG_IO_TIME_SECS) + 1) * 1000;
    }
    vincent_van_ghoul_io_msec_short = ((fbe_random() % VINCENT_VAN_GHOUL_SHORT_IO_TIME_SECS) + 1) * 1000;
    return;
}
/******************************************
 * end vincent_van_ghoul_set_io_seconds()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_set_originating_sp()
 *****************************************************************************
 *
 * @brief   Set the SP where the write data originated from.
 *
 * @param   originating_sp - The target SP to write data from
 *
 * @return  status
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_set_originating_sp(fbe_sim_transport_connection_target_t originating_sp)
{
    fbe_status_t    status = FBE_STATUS_OK;

    switch(originating_sp) {
        case FBE_SIM_SP_A:
            vincent_van_ghoul_rdgen_originating_sp = FBE_DATA_PATTERN_SP_ID_A;
            break;
        case FBE_SIM_SP_B:
            vincent_van_ghoul_rdgen_originating_sp = FBE_DATA_PATTERN_SP_ID_B;
            break;
        default:
            MUT_ASSERT_TRUE_MSG((originating_sp == FBE_SIM_SP_A), "Invalid originating SP");
            return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_sim_transport_set_target_server(originating_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}
/********************************************* 
 * end vincent_van_ghoul_set_originating_sp()
 ********************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_read_set_rdgen_originating_sp()
 *****************************************************************************
 *
 * @brief   Since this test can read data from an SP that is different from the
 *          SP that the data was written on, we must tell rdgen which SP the
 *          data was written on (otherwise rdgen will incorrectly report a
 *          miscompare).
 *
 * @param   None
 *
 * @return  status
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_read_set_rdgen_originating_sp(void)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t    *context_p = &vincent_van_ghoul_test_contexts[0];

    /* Validate that we have properly set the originating SP.
     */
    switch(vincent_van_ghoul_rdgen_originating_sp) {
        case FBE_DATA_PATTERN_SP_ID_A:
        case FBE_DATA_PATTERN_SP_ID_B:
            vincent_van_ghoul_rdgen_originating_sp = FBE_DATA_PATTERN_SP_ID_A;
            break;
        default:
            MUT_ASSERT_TRUE_MSG((vincent_van_ghoul_rdgen_originating_sp == FBE_DATA_PATTERN_SP_ID_A), "Invalid originating SP");
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Tell rdgen which SP the I/O originated on.
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CHECK_ORIGINATING_SP);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_rdgen_io_specification_set_originating_sp(&context_p->start_io.specification, vincent_van_ghoul_rdgen_originating_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*******************************************************
 * end vincent_van_ghoul_read_set_rdgen_originating_sp()
 *******************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_wait_for_all_luns_ready()
 *****************************************************************************
 *
 * @brief   Wait for all luns to be ready before starting I/O.
 *
 * @param   rg_config_p - Pointer to array of raid groups with the luns to run
 *              run I/O against.  This method validates that all the luns are
 *              in the ready state.
 * @param   target_sp - The SP to wait for the the LUNs to be Ready on
 *
 * @return  status - Determine if test passed or not
 *
 * @note    In dualsp mode the `other' SP maybe shutdown.  Therefore for either
 *          single SP mode or dualsp mode we only wait for all the LUNs to be
 *          be ready on `this' SP.
 *
 * @author
 *  03/24/2011  Ron Proulx  - Created 
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_wait_for_all_luns_ready(fbe_test_rg_configuration_t *rg_config_p,
                                                              fbe_transport_connection_target_t target_sp)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t        *current_rg_config_p = NULL;
    fbe_u32_t                           rg_count = 0;
    fbe_u32_t                           rg_index = 0;
    fbe_time_t                          start_time_for_sp;
    fbe_time_t                          elapsed_time_for_sp;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate the target SP is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "wait for all luns ready: this_sp: %d target_sp: %d is not alive !!", 
                   this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* First iterate over the raid groups on `this' sp. Then if neccessary iterate
     * over the raid groups on the other sp.
     */
    current_rg_config_p = rg_config_p;
    start_time_for_sp = fbe_get_time();
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* Wait for all lun objects of this raid group to be ready
             */
            status = fbe_test_sep_util_wait_for_all_lun_objects_ready(current_rg_config_p,
                                                                      target_sp);
            if (status != FBE_STATUS_OK)
            {
                /* Failure, break out and return the failure.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "wait for all luns ready: raid_group_id: 0x%x failed to come ready", 
                           current_rg_config_p->raid_group_id);
                break;
            }
        }

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */
    elapsed_time_for_sp = fbe_get_time() - start_time_for_sp;
    mut_printf(MUT_LOG_TEST_STATUS, 
               "wait for all luns ready: on target sp: %d took: %lld ms with status: 0x%x", 
               target_sp, (long long)elapsed_time_for_sp, status);

    return status;
}
/***********************************************
 * end vincent_van_ghoul_wait_for_all_luns_ready()
 ***********************************************/

/*!**************************************************************
 * vincent_van_ghoul_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param   target_sp - The SP where the data will be written from  
 * @param   rg_config_p - Array of raid groups under test             
 *
 * @return  status
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from vincent_van_ghoul_write_background_pattern
 *
 ****************************************************************/
static fbe_status_t vincent_van_ghoul_write_background_pattern(fbe_sim_transport_connection_target_t target_sp,
                                                               fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status;
    fbe_api_rdgen_context_t            *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If not dualsp mode the target_sp is ignored.
     */
    if (b_is_dualsp_mode == FBE_FALSE) 
    {
        target_sp = this_sp;
    }

    /* Validate that the target sp is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    ) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s this_sp: %d target_sp: %d is not alive ==", 
               __FUNCTION__, this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
    }

    /* If single SP I/O is run on the local SP (i.e. target_sp is ignored)
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s called on: %d rdgen SP: %d rdgen op: %d ==", 
               __FUNCTION__, this_sp, target_sp, FBE_RDGEN_OPERATION_WRITE_ONLY);

    /* If the target sp is not this one wait for the LUNs to be ready.
     */
    if (target_sp != this_sp) 
    {
        status = vincent_van_ghoul_wait_for_all_luns_ready(rg_config_p, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Set the SP to the target.
     */
    status = vincent_van_ghoul_set_originating_sp(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            VINCENT_VAN_GHOUL_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            VINCENT_VAN_GHOUL_ELEMENT_SIZE);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Set the SP to original.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/**************************************************
 * end vincent_van_ghoul_write_background_pattern()
 **************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_write_fixed_pattern()
 *****************************************************************************
 *
 * @brief   Seed all the LUNs with a fixed (i.e. doesn't vary) pattern.
 *
 * @param   target_sp - The target SP for write             
 * @param   rg_config_p - Array of raid groups under test             
 *
 * @return  None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void vincent_van_ghoul_write_fixed_pattern(fbe_transport_connection_target_t target_sp,
                                           fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                status;
    fbe_api_rdgen_context_t    *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If not dualsp mode the target_sp is ignored.
     */
    if (b_is_dualsp_mode == FBE_FALSE) 
    {
        target_sp = this_sp;
    }

    /* Validate that the target sp is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    ) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s this_sp: %d target_sp: %d is not alive ==", 
               __FUNCTION__, this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
    }

    /* If single SP I/O is run on the local SP (i.e. target_sp is ignored)
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s called on: %d rdgen SP: %d rdgen op: %d ==", 
               __FUNCTION__, this_sp, target_sp, FBE_RDGEN_OPERATION_WRITE_ONLY);

    /* If the target sp is not this one wait for the LUNs to be ready.
     */
    if (target_sp != this_sp) 
    {
        status = vincent_van_ghoul_wait_for_all_luns_ready(rg_config_p, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Set the SP to the target.
     */
    status = vincent_van_ghoul_set_originating_sp(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*!  Write a fixed pattern pattern to the LUNs.
     *
     *   @todo Currently zeros is the only fixed pattern available.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             VINCENT_VAN_GHOUL_FIXED_PATTERN,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             VINCENT_VAN_GHOUL_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             VINCENT_VAN_GHOUL_FIXED_PATTERN,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             VINCENT_VAN_GHOUL_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* If the target sp is not this set back
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/******************************************
 * end vincent_van_ghoul_write_fixed_pattern()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_read_fixed_pattern()
 *****************************************************************************
 *
 * @brief   Read all lUNs and validate fixed pattern
 *
 * @param   target_sp - The SP to read from
 * @param   rg_config_p - Pointer to array of raid groups under test.      
 *
 * @return  status
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_read_fixed_pattern(fbe_transport_connection_target_t target_sp,
                                                         fbe_test_rg_configuration_t *rg_config_p)

{
    fbe_status_t                        status;
    fbe_api_rdgen_context_t            *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If not dualsp mode the target_sp is ignored.
     */
    if (b_is_dualsp_mode == FBE_FALSE) 
    {
        target_sp = this_sp;
    }

    /* Validate that the target sp is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    ) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s this_sp: %d target_sp: %d is not alive ==", 
               __FUNCTION__, this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
    }

    /* Log a message
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s from SP: %d ==", 
               __FUNCTION__, target_sp);
    
    /* Read back the pattern and check it was written OK.
     */ 
    if (target_sp != this_sp) 
    {
        status = vincent_van_ghoul_wait_for_all_luns_ready(rg_config_p, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Initialize the rdgen context.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             VINCENT_VAN_GHOUL_FIXED_PATTERN,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             VINCENT_VAN_GHOUL_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Set the correct originating SP.
     */
    status = vincent_van_ghoul_read_set_rdgen_originating_sp();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);    

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    if (target_sp != this_sp) 
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating fixed pattern - success ==", 
               __FUNCTION__);
    return status;
}
/********************************************
 * end vincent_van_ghoul_read_fixed_pattern()
 ********************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_write_zero_background_pattern()
 *****************************************************************************
 *
 * @brief   Seed all the LUNs with a zero pattern.
 *
 * @param   target_sp - The SP to write data from          
 * @param   rg_config_p - Array of raid groups under test             
 *
 * @return  status
 *
 * @author
 *  03/08/2011  Ruomu Gao  - Copied from big_bird_test.c
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_write_zero_background_pattern(fbe_transport_connection_target_t target_sp,
                                                                    fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status;
    fbe_api_rdgen_context_t            *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If not dualsp mode the target_sp is ignored.
     */
    if (b_is_dualsp_mode == FBE_FALSE) 
    {
        target_sp = this_sp;
    }

    /* Validate that the target sp is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    ) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s this_sp: %d target_sp: %d is not alive ==", 
               __FUNCTION__, this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
    }

    /* If single SP I/O is run on the local SP (i.e. target_sp is ignored)
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s called on: %d rdgen SP: %d rdgen op: %d ==", 
               __FUNCTION__, this_sp, target_sp, FBE_RDGEN_OPERATION_ZERO_ONLY);

    /* If the target sp is not this one wait for the LUNs to be ready.
     */
    if (target_sp != this_sp) 
    {
        status = vincent_van_ghoul_wait_for_all_luns_ready(rg_config_p, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Set the SP to the target.
     */
    status = vincent_van_ghoul_set_originating_sp(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*!  Write a zero background pattern to the LUNs.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_ZERO_ONLY,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             VINCENT_VAN_GHOUL_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             VINCENT_VAN_GHOUL_ELEMENT_SIZE    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    /* Set the SP back to original.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/*******************************************************
 * end vincent_van_ghoul_write_zero_background_pattern()
 *******************************************************/

/*!**************************************************************
 * vincent_van_ghoul_read_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param   target_sp - The target for the request 
 * @param   rg_config_p - Pointer to array of raid groups under test.
 *
 * @return  status
 *
 * @author
 *  02/22/2010  Ron Proulx  - Created from vincent_van_ghoul_read_background_pattern
 *
 ****************************************************************/
static fbe_status_t vincent_van_ghoul_read_background_pattern(fbe_transport_connection_target_t target_sp,
                                                              fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t                        status;
    fbe_api_rdgen_context_t            *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If not dualsp mode the target_sp is ignored.
     */
    if (b_is_dualsp_mode == FBE_FALSE) 
    {
        target_sp = this_sp;
    }

    /* Validate that the target sp is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    ) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s this_sp: %d target_sp: %d is not alive ==", 
               __FUNCTION__, this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
    }

    /* Log a message
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s from SP: %d ==", 
               __FUNCTION__, target_sp);
    
    /* Read back the pattern and check it was written OK.
     */ 
    if (target_sp != this_sp) 
    {
        status = vincent_van_ghoul_wait_for_all_luns_ready(rg_config_p, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Fill the rdgen context.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            VINCENT_VAN_GHOUL_ELEMENT_SIZE);
    /* Set the correct originating SP.
     */
    status = vincent_van_ghoul_read_set_rdgen_originating_sp();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate the data.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    if (target_sp != this_sp) 
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s Validating background pattern - success ==", 
               __FUNCTION__);
    return status;
}
/**************************************************
 * end vincent_van_ghoul_read_background_pattern()
 **************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_is_shutdown_allowed_for_raid_group()
 *****************************************************************************
 *
 * @brief   Simply determines if we all the specified raid group to be shutdown
 *          or not.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 *
 * @return  bool - FBE_TRUE - Allow raid group to be shutdown
 *                 FBE_FALSE - Not allowed to shutdown this raid group
 *
 * @author
 *  09/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_bool_t vincent_van_ghoul_is_shutdown_allowed_for_raid_group(fbe_test_rg_configuration_t *const rg_config_p)
{
    fbe_bool_t  b_allow_shutdown = FBE_TRUE;

    /* Currently al
     */
    if (vincent_van_ghoul_b_disable_shutdown == FBE_TRUE)
    {
        b_allow_shutdown = FBE_FALSE;
    }

    return(b_allow_shutdown);
}
/*****************************************************
 * end vincent_van_ghoul_is_shutdown_allowed_for_raid_group()
 *****************************************************/

/*!**************************************************************
 * vincent_van_ghoul_run_test_on_rg_config_with_extra_disk_with_time_save()
 ****************************************************************
 * @brief
 *  This function is wrapper of fbe_test_run_test_on_rg_config(). 
 *  This function populates extra disk count in rg config and then start 
 *  the test. It destroys RG config depending on the parameter passed to it.
 * 
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 * @param destroy_rg_config - FBE_TRUE: Does not destory the rg config and saves test execution time. FBE_FALSE: Destroys rg config.
 *
 * @return  None.
 *
 * @author
 *  02/21/2013 - Created. Preethi Poddatur
 *
 ****************************************************************/
static void vincent_van_ghoul_run_test_on_rg_config_with_extra_disk_with_time_save(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun,
                                    fbe_u32_t destroy_rg_config)
{
   if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the arry of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }

    /* populate the required extra disk count in rg config */
    fbe_test_rg_populate_extra_drives(rg_config_p, VINCENT_VAN_GHOUL_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);

    /* run the test */
    if (destroy_rg_config == FBE_TRUE) {
       fbe_test_run_test_on_rg_config(rg_config_p, context_p, test_fn,
                                      luns_per_rg,
                                      chunks_per_lun);
    }
    else {
       fbe_test_run_test_on_rg_config_with_time_save(rg_config_p, context_p, test_fn,
                                      luns_per_rg,
                                      chunks_per_lun,
                                      FBE_FALSE);
    }

    return;
}

/********************************************************
 * end vincent_van_ghoul_run_test_on_rg_config_with_extra_disk_with_time_save()
 ********************************************************/

/*!**************************************************************
 * vincent_van_ghoul_run_test_on_rg_config_with_extra_disk()
 ****************************************************************
 * @brief
 *  This function is wrapper of fbe_test_run_test_on_rg_config(). 
 *  This function populates extra disk count in rg config and then start 
 *  the test.
 * 
 *
 * @param rg_config_p - Array of raid group configs.
 * @param context_p - Context to pass to test function.
 * @param test_fn- Test function to use for every set of raid group configs uner test.
 * @param luns_per_rg - Number of LUs to bind per rg.
 * @param chunks_per_lun - Number of chunks in each lun.
 *
 * @return  None.
 *
 * @author
 *  06/01/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
static void vincent_van_ghoul_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun)
{
   vincent_van_ghoul_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p,context_p,test_fn,luns_per_rg,
                                                                          chunks_per_lun,
                                                                          FBE_TRUE);
   return;
}
/********************************************************
 * end vincent_van_ghoul_run_test_on_rg_config_with_extra_disk()
 ********************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_wait_for_raid_group_to_fail()
 *****************************************************************************
 *
 * @brief   Wait for the specified raid group to enter the FAIL state.
 *
 * @param   rg_config_p - Pointer to raid group to wait for
 *
 * @return  None
 *
 * @author
 *  09/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static void vincent_van_ghoul_wait_for_raid_group_to_fail(fbe_test_rg_configuration_t *const rg_config_p)
{
    fbe_status_t    status;
    fbe_object_id_t rg_object_id;

    /* Skip this RG if it is not configured for this test */
    if (!fbe_test_rg_config_is_enabled(rg_config_p))
    {
        return;
    }

    /* Get the object id associated with the raid group
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* If allowed to shutdown, wait for the raid group to enter FAIL within
     * 20 seconds.
     */
    if (vincent_van_ghoul_is_shutdown_allowed_for_raid_group(rg_config_p))
    {
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              rg_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}
/******************************************
 * end vincent_van_ghoul_wait_for_raid_group_to_fail()
 ******************************************/

/*!**************************************************************
 * vincent_van_ghoul_configure_extra_drives_as_hot_spares()
 ****************************************************************
 * @brief
 *  Insert all drives that were removed (but don't include the
 *  sparing drives)
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param drives_to_insert - Number of drives we are removing.
 * @param b_reinsert_same_drive 
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void vincent_van_ghoul_configure_extra_drives_as_hot_spares(fbe_test_rg_configuration_t *rg_config_p,
                                                            fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t                       index;
    fbe_test_rg_configuration_t     *current_rg_config_p = rg_config_p;
    fbe_test_raid_group_disk_set_t  *drive_to_spare_p = NULL;
    fbe_u32_t                       drive_number;
    fbe_u32_t                       num_removed_drives = 0;
    fbe_u32_t                       num_spared_drives = 0;
    fbe_u32_t                       spared_position;
    fbe_u32_t                       spared_index = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_u32_t                       index_to_spare = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;

    /*  Disable the recovery queue to delay spare swap-in for any RG at this moment untill all are available
     */
    fbe_test_sep_util_disable_recovery_queue(FBE_OBJECT_ID_INVALID);

    /* For every raid group, configure extra drives as hot-spares based on the number of drives removed. */
    for (index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the number of removed drives in the RG. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);

        /* This requires the test to privode enough extra drives for hotspare.
         * Subtrack any drives that are in the `spared' array.
         */
        num_spared_drives = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);
        MUT_ASSERT_CONDITION(num_spared_drives, <=,  current_rg_config_p->num_of_extra_drives);
        MUT_ASSERT_CONDITION(num_spared_drives, <=,  num_removed_drives);

        /* Configure automatic spares from the RG extra disk set. */
        spared_position = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {    
            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == VINCENT_VAN_GHOUL_INVALID_DISK_POSITION)    )
            {
                /* Skip over any spared drives */
                spared_position = fbe_test_sep_util_get_needing_spare_position_from_index(current_rg_config_p, drive_number);
                spared_index = drive_number;
                continue;
            }

            /* Get an extra drive to configure as hot spare. */
            index_to_spare = drive_number;
            drive_to_spare_p = &current_rg_config_p->extra_disk_set[index_to_spare];

            /* Validate that we haven't gone beyond the available spares
             */
            if ((drive_to_spare_p->bus == 0)       &&
                (drive_to_spare_p->enclosure == 0) &&
                (drive_to_spare_p->slot == 0)         )
            {
                /* Exceeded available sspares */
                status = FBE_STATUS_GENERIC_FAILURE;
                MUT_ASSERT_INTEGER_EQUAL_MSG(FBE_STATUS_OK, status, "Insufficient extra drives");
            }

            /* Configure this drive as a hot-spare.*/
            mut_printf(MUT_LOG_TEST_STATUS, "== %s sparing rg %d (%d_%d_%d). ==", 
                       __FUNCTION__, index,  
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Wait for the PVD to be ready.  We need to wait at a minimum for the object to be created,
             * but the PVD is not spareable until it is ready. 
             */
            status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                          drive_to_spare_p->enclosure, 
                                                          drive_to_spare_p->slot,
                                                          FBE_LIFECYCLE_STATE_READY,
                                                          VINCENT_VAN_GHOUL_DRIVE_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Also wait for the drive to become ready on the peer, if dual-SP test.
             */
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
                fbe_api_sim_transport_set_target_server(peer_sp);
                status = fbe_test_sep_util_wait_for_pvd_state(drive_to_spare_p->bus, 
                                                              drive_to_spare_p->enclosure, 
                                                              drive_to_spare_p->slot,
                                                              FBE_LIFECYCLE_STATE_READY,
                                                              VINCENT_VAN_GHOUL_DRIVE_WAIT_MSEC);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                fbe_api_sim_transport_set_target_server(local_sp);
            }

            /* Configure extra disk as a hot spare so that it will become part of the raid group permanently.
             */
            status = fbe_test_sep_util_configure_drive_as_hot_spare(drive_to_spare_p->bus, 
                                                                    drive_to_spare_p->enclosure, 
                                                                    drive_to_spare_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        }
        current_rg_config_p++;
   }

    /*  enable the recovery queue so that hot spare swap in occur concurrently at best suitable place
     */
    fbe_test_sep_util_enable_recovery_queue(FBE_OBJECT_ID_INVALID);


   return;
}
/*************************************************************
 * end vincent_van_ghoul_configure_extra_drives_as_hot_spares()
 *************************************************************/

/*!**************************************************************
 * vincent_van_ghoul_wait_extra_drives_swap_in()
 ****************************************************************
 * @brief
 *  Insert all drives that were removed (but don't include the
 *  sparing drives)
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void vincent_van_ghoul_wait_extra_drives_swap_in(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t raid_group_count)
{
    fbe_status_t status;
    fbe_u32_t                       index;
    fbe_u32_t                       num_removed_drives = 0;
    fbe_u32_t                       num_spared_drives = 0;
    fbe_test_raid_group_disk_set_t *unused_drive_to_insert_p;
    fbe_test_rg_configuration_t     *current_rg_config_p = NULL;
    fbe_u32_t                       position_to_insert;
    fbe_u32_t                       drive_number;
    fbe_u32_t                       spared_position;
    fbe_u32_t                       spared_index = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    #define MAX_RAID_GROUPS 20
    #define MAX_POSITIONS_TO_REINSERT 2
    fbe_u32_t saved_position_to_reinsert[MAX_RAID_GROUPS][MAX_POSITIONS_TO_REINSERT];
    fbe_u32_t saved_num_removed_drives[MAX_RAID_GROUPS];

    if (raid_group_count > MAX_RAID_GROUPS)
    {
        MUT_FAIL_MSG("unexpected number of raid groups");
    }
    /* For every raid group ensure spare drives have swapped in for failed drives. */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it. */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* This requires the test to privode enough extra drives for hotspare.
         * Subtrack any drives that are in the `spared' array.
         */
        num_spared_drives = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);

        /* Get the number of removed drives. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);
        if ((num_removed_drives - num_spared_drives) > MAX_POSITIONS_TO_REINSERT)
        {
            MUT_FAIL_MSG("unexpected number of positions to reinsert");
        }
        saved_num_removed_drives[index] = num_removed_drives;

        /* Confirm hot-spares swapped-in for the removed drives in the RG. */
        spared_position = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < num_removed_drives; drive_number++)
        {
            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == VINCENT_VAN_GHOUL_INVALID_DISK_POSITION)    )
            {
                /* Skip over any spared drives */
                spared_position = fbe_test_sep_util_get_needing_spare_position_from_index(current_rg_config_p, drive_number);
                saved_position_to_reinsert[index][drive_number] = spared_position;
                spared_index = drive_number;
                continue;
            }
    
            /* Get the next position to insert a new drive. */
            position_to_insert = fbe_test_sep_util_get_next_position_to_insert(current_rg_config_p);
            saved_position_to_reinsert[index][drive_number] = position_to_insert;

            /*! @note Do not remove from the removed array since that will keep track
             *        of the drives that we need ot re-insert.
             */
            //fbe_test_sep_util_removed_position_inserted(current_rg_config_p, position_to_insert); 

            /* Add this position to the spare array (for rebuild) */
            fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_insert);

            /* Get a pointer to the position in the RG disk set of the removed drive. */
            unused_drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];
        
            if (drive_number < current_rg_config_p->num_of_extra_drives)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "== %s wait permanent swap-in for rg index %d, position %d. ==", 
                           __FUNCTION__, index, position_to_insert);
            
                /* Wait for an extra drive to be permanent swap-in for the failed position. */
                fbe_test_sep_drive_wait_permanent_spare_swap_in(current_rg_config_p, position_to_insert);
            
                mut_printf(MUT_LOG_TEST_STATUS, "== %s swap in complete for rg index %d, position %d. ==", 
                           __FUNCTION__, index, position_to_insert);
            }
        }

        current_rg_config_p++;
    }

    /* No need to validate spared drives*/
    if (num_spared_drives != 0)
    {
        return;
    }

    /* For every raid group ensure spare drives have swapped in for failed drives. */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        /* If the current RG is not configured for this test, skip it. */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* This requires the test to privode enough extra drives for hotspare.
         * Subtrack any drives that are in the `spared' array.
         */
        num_spared_drives = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);

        /* Get the number of removed drives. */
        num_removed_drives = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);
        if ((num_removed_drives - num_spared_drives) > MAX_POSITIONS_TO_REINSERT)
        {
            MUT_FAIL_MSG("unexpected number of positions to reinsert");
        }

        /* Confirm hot-spares swapped-in for the removed drives in the RG. */
        spared_position = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
        for (drive_number = 0; drive_number < saved_num_removed_drives[index]; drive_number++)
        {
            /* Get the next position to insert a new drive. */
            position_to_insert = saved_position_to_reinsert[index][drive_number];

            /* The spared drive is always prior to the removed */
            if ((num_spared_drives > 0)                               &&
                (spared_position == VINCENT_VAN_GHOUL_INVALID_DISK_POSITION)    )
            {
                /* Skip over any spared drives */
                spared_position = position_to_insert;
                continue;
            }

            /* Get a pointer to the position in the RG disk set of the removed drive. */
            unused_drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];

            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting unused drive rg index %d (%d_%d_%d). ==", 
                       __FUNCTION__, index, 
                       unused_drive_to_insert_p->bus, unused_drive_to_insert_p->enclosure, unused_drive_to_insert_p->slot);

            /* Insert removed drive back. */
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_reinsert_drive(unused_drive_to_insert_p->bus, 
                                                         unused_drive_to_insert_p->enclosure, 
                                                         unused_drive_to_insert_p->slot,
                                                         current_rg_config_p->drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
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
                    status = fbe_test_pp_util_reinsert_drive(unused_drive_to_insert_p->bus, 
                                                             unused_drive_to_insert_p->enclosure, 
                                                             unused_drive_to_insert_p->slot,
                                                             current_rg_config_p->peer_drive_handle[position_to_insert]);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /*  Set the target server back to the local SP. */
                    fbe_api_sim_transport_set_target_server(local_sp);
                }
            }
            else
            {
                status = fbe_test_pp_util_reinsert_drive_hw(unused_drive_to_insert_p->bus, 
                                                            unused_drive_to_insert_p->enclosure, 
                                                            unused_drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

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
                    status = fbe_test_pp_util_reinsert_drive_hw(unused_drive_to_insert_p->bus, 
                                                                unused_drive_to_insert_p->enclosure, 
                                                                unused_drive_to_insert_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /*  Set the target server back to the local SP. */
                    fbe_api_sim_transport_set_target_server(local_sp);
                }
            }
        
            /* Wait for the unused pvd object to be in ready state. */
            status = fbe_test_sep_util_wait_for_pvd_state(unused_drive_to_insert_p->bus, 
                                                          unused_drive_to_insert_p->enclosure, 
                                                          unused_drive_to_insert_p->slot,
                                                          FBE_LIFECYCLE_STATE_READY,
                                                          VINCENT_VAN_GHOUL_DRIVE_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    return;
}
/*************************************************************
 * vincent_van_ghoul_wait_extra_drives_swap_in()
 *************************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_get_next_position_to_insert()
 *****************************************************************************
 * 
 * @brief   Find the next position to insert using the removed array.
 *
 * @param   rg_config_p - Pointer to raid group config          
 *
 * @return  fbe_u32_t - Position to insert
 *
 *****************************************************************************/
static fbe_u32_t vincent_van_ghoul_get_next_position_to_insert(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t   position_to_insert = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    fbe_u32_t   rg_index;

    /*! @note This assumes that drives have been removed
     */
    MUT_ASSERT_TRUE((fbe_s32_t)rg_config_p->num_removed > 0);
    MUT_ASSERT_TRUE(rg_config_p->num_removed <= rg_config_p->width); 
     
    /* Walk thru the removed array (skipping over the copy index) and
     * locate first valid position.
     */
    for (rg_index = 0; rg_index < rg_config_p->width; rg_index++)
    {
        /* Skip over the copy index
         */
        if (rg_index == VINCENT_VAN_GHOUL_INDEX_RESERVED_FOR_COPY)
        {
            continue;
        }

        /* Check if not invalid.
         */
        if (rg_config_p->drives_removed_array[rg_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
        {
            position_to_insert = rg_config_p->drives_removed_array[rg_index];
            break;
        }
    }
    MUT_ASSERT_TRUE(position_to_insert < rg_config_p->width); 
    return position_to_insert;
}
/*************************************************************
 * ehnd vincent_van_ghoul_get_next_position_to_insert()
 *************************************************************/

/*!**************************************************************
 * vincent_van_ghoul_insert_drives()
 ****************************************************************
 * @brief
 *  insert drives, one per raid group.
 * 
 * @param rg_config_p - raid group config array.
 * @param raid_group_count
 * @param b_transition_pvd - True to not use terminator to reinsert.
 *
 * @return None.
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 ****************************************************************/
static void vincent_van_ghoul_insert_drives(fbe_test_rg_configuration_t *rg_config_p,
                                     fbe_u32_t raid_group_count,
                                     fbe_bool_t b_transition_pvd)
{
    fbe_u32_t index;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_status_t status;
    fbe_object_id_t pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t position_to_insert;

    /* For every raid group insert one drive.
     */
    for ( index = 0; index < raid_group_count; index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        position_to_insert = vincent_van_ghoul_get_next_position_to_insert(current_rg_config_p);
        fbe_test_sep_util_removed_position_inserted(current_rg_config_p, position_to_insert);
        drive_to_insert_p = &current_rg_config_p->rg_disk_set[position_to_insert];

        mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting rg %d position %d (%d_%d_%d). ==", 
                   __FUNCTION__, index, position_to_insert, 
                   drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);

        if (b_transition_pvd)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus,
                                                                    drive_to_insert_p->enclosure,
                                                                    drive_to_insert_p->slot,
                                                                    &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_set_object_to_state(pvd_object_id, FBE_LIFECYCLE_STATE_ACTIVATE, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else 
        {
            /* inserting the same drive back */
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure, 
                                                     drive_to_insert_p->slot,
                                                     current_rg_config_p->drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                             drive_to_insert_p->enclosure, 
                                                             drive_to_insert_p->slot,
                                                             current_rg_config_p->peer_drive_handle[position_to_insert]);
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
        }
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end vincent_van_ghoul_insert_drives()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_start_io()
 *****************************************************************************
 *
 * @brief   Since vincent van ghoul is a reboot test we only start I/O on (1) 
 *          SP.  This test starts I/O on the SP specified.
 *
 * @param   target_sp - The target SP to start rdgen on
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   rdgen_operation - The rdgen operation (i.e. w/r/c, z/r/c, etc)
 * @param   ran_abort_msecs - Random abort (if enabled) msecs
 * @param   threads - How many threads (per LUN)
 * @param   io_size_blocks - The maximum I/O size in blocks
 *
 * @return  status
 *
 * @note    If this is not a dualsp test (i.e. there is only a single SP)
 *          then this method simply writes the background pattern (i.e. I/O
 *          does not run continuously since we will reboot this SP).
 *
 * @author
 *  02/13/2010  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_start_io(fbe_sim_transport_connection_target_t target_sp,
                                               fbe_test_rg_configuration_t *const rg_config_p,
                                               fbe_rdgen_operation_t rdgen_operation,
                                               fbe_test_random_abort_msec_t ran_abort_msecs,
                                               fbe_u32_t threads,
                                               fbe_u32_t io_size_blocks)
{
    fbe_status_t                        status;
    fbe_api_rdgen_context_t            *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_bool_t                          b_ran_aborts_enabled;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If not dualsp mode the target_sp is ignored.
     */
    if (b_is_dualsp_mode == FBE_FALSE) 
    {
        target_sp = this_sp;
    }

    /* Validate that the target sp is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    ) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s this_sp: %d target_sp: %d is not alive ==", 
               __FUNCTION__, this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
    }

    /* If single SP I/O is run on the local SP (i.e. target_sp is ignored)
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s called on: %d rdgen SP: %d rdgen op: %d ==", 
               __FUNCTION__, this_sp, target_sp, rdgen_operation);

    /* Setup the abort mode to false
     */
    b_ran_aborts_enabled = (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID) ? FBE_FALSE : FBE_TRUE;
    status = fbe_test_sep_io_configure_abort_mode(b_ran_aborts_enabled, ran_abort_msecs);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  

    /* If dualsp test mode is enabled then set the target_sp
     */
    if (b_is_dualsp_mode)
    {
        /* If the target sp is not this one wait for the LUNs to be ready.
         */
        if (target_sp != this_sp) 
        {
            status = vincent_van_ghoul_wait_for_all_luns_ready(rg_config_p, target_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Run random I/O ONLY on the SP specified.  We cannot run on both SPs
         * because this is a reboot test and I/O will fail.
         */
        status = vincent_van_ghoul_set_originating_sp(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                           FBE_OBJECT_ID_INVALID,
                                           FBE_CLASS_ID_LUN,
                                           rdgen_operation,
                                           FBE_LBA_INVALID,    /* use capacity */
                                           0,    /* run forever*/ 
                                           fbe_test_sep_io_get_threads(threads), /* threads */
                                           io_size_blocks,
                                           b_ran_aborts_enabled, /* Inject aborts if requested*/
                                           FBE_FALSE  /* No Peer I/O  */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

        /* inject random aborts if we are asked to do so
         */
        if (ran_abort_msecs!= FBE_TEST_RANDOM_ABORT_TIME_INVALID)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s random aborts inserted %d msecs", __FUNCTION__, ran_abort_msecs);
            status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification, ran_abort_msecs);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Run our I/O test.
         */
        status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the target back to the original
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);        
    }
    else
    {
        /* Else this is a single SP test, therefore we cannot run I/O 
         * continuously.  Simply write the background pattern.
         */
        status = vincent_van_ghoul_write_background_pattern(target_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
    }

    /* Return the execution status
     */
    return status;
}
/******************************************
 * end vincent_van_ghoul_start_io()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_stop_io()
 *****************************************************************************
 *
 * @brief   Since vincent van ghoul is a reboot test we only stop I/O on (1) 
 *          SP.
 *
 * @param   target_sp - The SP to issue the stop I/O on
 * @param   rg_config_p - Pointer to raid groups under test
 * @param   rdgen_operation - The rdgen operation (i.e. w/r/c, z/r/c, etc)
 * @param   ran_abort_msecs - Random abort (if enabled) msecs
 *
 * @return  status
 *
 * @note    If this is not a dualsp test (i.e. there is only a single SP)
 *          then this method simply writes the background pattern (i.e. I/O
 *          does not run continuously since we will reboot this SP).
 *
 * @author
 *  02/13/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_stop_io(fbe_transport_connection_target_t   target_sp,
                                              fbe_test_rg_configuration_t *const rg_config_p,
                                              fbe_rdgen_operation_t rdgen_operation,
                                              fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                        status;
    fbe_api_rdgen_context_t            *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_bool_t                          b_is_dualsp_mode;
    fbe_transport_connection_target_t   this_sp;
    fbe_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t          cmi_info;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If not dualsp mode the target_sp is ignored.
     */
    if (b_is_dualsp_mode == FBE_FALSE) 
    {
        target_sp = this_sp;
    }

    /* Validate that the target sp is alive.
     */
    if ((target_sp != this_sp)             &&
        (cmi_info.peer_alive == FBE_FALSE)    ) 
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s this_sp: %d target_sp: %d is not alive ==", 
               __FUNCTION__, this_sp, target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);  
    }

    /* If single SP I/O is run on the local SP (i.e. target_sp is ignored)
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== %s called on: %d rdgen SP: %d rdgen op: %d ==", 
               __FUNCTION__, this_sp, target_sp, rdgen_operation);
    
    /* Dualssp test can run I/O constantly.  Single SP jusdt validates the
     * background pattern
     */
    if (b_is_dualsp_mode)
    {
        /* Issue stop on target SP.
         */
        status = fbe_api_sim_transport_set_target_server(target_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

        /* Stop the I/O
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
        status = fbe_api_rdgen_stop_tests(context_p, 1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

        /* If no aborts were specified, we do not expect errors.
         */
        if (ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
            MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        }

        /* Now set the SP back.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
    }
    else
    {
        /* Else single SP test we validate the background pattern
         */
        status = vincent_van_ghoul_read_background_pattern(this_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Return the status
     */
    return status;
}
/******************************************
 * end vincent_van_ghoul_stop_io()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_check_if_all_threads_started()
 *****************************************************************************
 *
 * @brief      Check all threads has i/o count > 1 
 *
 * @param      object_info_p 
 * @param      thread_info_p
 *
 * @return     FBE_TRUE if i/o count > 0 for all threads
 *             otherwise FBE_FALSE
 *
 * @author
 *  09/20/2011  Maya Dagon  - Created
 *
 ****************************************************************/
static fbe_bool_t vincent_van_ghoul_check_if_all_threads_started(fbe_api_rdgen_get_object_info_t  *object_info_p,                          
                                                  fbe_api_rdgen_get_thread_info_t *thread_info_p)
{

    fbe_u32_t thread_index;
    fbe_u32_t object_index;
    fbe_api_rdgen_thread_info_t *current_thread_info_p = NULL;
    fbe_api_rdgen_object_info_t *current_object_info_p = NULL;
    fbe_u32_t found_threads = 0;

    for (object_index = 0; object_index < object_info_p->num_objects; object_index++)
    {
        current_object_info_p = &object_info_p->object_info_array[object_index];

        /* If we have a fldb, display it if there are outstanding threads.
         */
        if ( current_object_info_p->active_ts_count != 0 )
        {        
            found_threads = 0;

            /* Loop over all the threads and stop when we have found the number we were 
             * looking for. 
             */
            for ( thread_index = 0; 
                  ((thread_index < thread_info_p->num_threads) && (found_threads < current_object_info_p->active_ts_count)); 
                  thread_index++)
            {
                current_thread_info_p = &thread_info_p->thread_info_array[thread_index];
                if (current_thread_info_p->object_handle == current_object_info_p->object_handle)
                {
                    if(current_thread_info_p->io_count < 1)
                    {
                        return FBE_FALSE;
                    }
                    found_threads++;
                }
            }
        }
    }
    return FBE_TRUE;
}
/******************************************
 * end vincent_van_ghoul_check_if_all_threads_started()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_wait_for_rdgen_start()
 *****************************************************************************
 *
 * @brief  Wait for all threads to start i/o to luns
 *
 * @param  wait_seconds - max wait time in seconds               
 *
 * @return None.
 *
 * @author
 *  09/20/2011  Maya Dagon  - Created
 *
 ****************************************************************/
static void vincent_van_ghoul_wait_for_rdgen_start(fbe_u32_t wait_seconds)
{
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t stats;
    fbe_rdgen_filter_t filter;
    fbe_api_rdgen_get_object_info_t *object_info_p = NULL;
    fbe_api_rdgen_get_request_info_t *request_info_p = NULL;
    fbe_api_rdgen_get_thread_info_t *thread_info_p = NULL;


    /* Get the stats, then allocate memory.
     */
    fbe_api_rdgen_filter_init(&filter, 
                              FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_LUN, 
                              FBE_PACKAGE_ID_SEP_0,
                              0 /* edge index */);
    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if ((stats.objects > 0) && (stats.requests > 0))
    {
        /* Allocate memory to get objects, requests and threads.
         */
        object_info_p = (fbe_api_rdgen_get_object_info_t*)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_get_object_info_t) * stats.objects);
        MUT_ASSERT_TRUE(object_info_p != NULL);
        
        request_info_p = (fbe_api_rdgen_get_request_info_t*)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_get_request_info_t) * stats.requests);
        if(request_info_p == NULL)
        {
            fbe_api_free_memory(object_info_p);
            MUT_ASSERT_TRUE(request_info_p != NULL);
        }
        

        thread_info_p = (fbe_api_rdgen_get_thread_info_t*)fbe_api_allocate_memory(sizeof(fbe_api_rdgen_get_thread_info_t) * stats.threads);
        if (thread_info_p == NULL)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            MUT_ASSERT_TRUE(thread_info_p != NULL);
        }

        /* Fetch info on objects, requests, and threads.
         */
        status = fbe_api_rdgen_get_object_info(object_info_p, stats.objects);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            fbe_api_free_memory(thread_info_p);
            MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK,"cannot get object info\n");
        }

        status = fbe_api_rdgen_get_request_info(request_info_p, stats.requests);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            fbe_api_free_memory(thread_info_p);
            MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK, "cannot get request info \n");
        }
        status = fbe_api_rdgen_get_thread_info(thread_info_p, stats.threads);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_free_memory(object_info_p);
            fbe_api_free_memory(request_info_p);
            fbe_api_free_memory(thread_info_p);
            MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK,"cannot get thread info\n");
        }
    

        /* Keep looping until we hit the target wait milliseconds.
         */
        do
        {
            /* refresh thread info
             */
            status = fbe_api_rdgen_get_thread_info(thread_info_p, stats.threads);
            if (status != FBE_STATUS_OK)
            {
                fbe_api_free_memory(object_info_p);
                fbe_api_free_memory(request_info_p);
                fbe_api_free_memory(thread_info_p);
                MUT_ASSERT_INT_EQUAL_MSG(status,FBE_STATUS_OK,"cannot get thread info\n");
            }

            if (vincent_van_ghoul_check_if_all_threads_started(object_info_p,thread_info_p))
            {
                fbe_api_free_memory(object_info_p);
                fbe_api_free_memory(request_info_p);
                fbe_api_free_memory(thread_info_p);
                return ;
            }

            fbe_api_sleep(500);
            elapsed_msec += 500;
        }while (elapsed_msec < target_wait_msec);
    }

    fbe_api_free_memory(object_info_p);
    fbe_api_free_memory(request_info_p);
    fbe_api_free_memory(thread_info_p);
    /* fail the test 
    */
    MUT_ASSERT_INT_EQUAL_MSG(1,0,"not all rdgen threads started\n");
}
/******************************************
 * end vincent_van_ghoul_wait_for_rdgen_start()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_start_fixed_io()
 *****************************************************************************
 *
 * @brief   Run a fixed I/O pattern.  The reason the pattern is fixed is the
 *          fact that the raid group will be shutdown during I/O.  Since some
 *          I/O will be dropped we cannot use a sequence tag in the data since
 *          we want to be able to validate not media errors when the raid group
 *          are brought back.    
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  03/20/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static void vincent_van_ghoul_start_fixed_io(fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_status_t status;

    mut_printf(MUT_LOG_TEST_STATUS, 
           "== %s Start fixed I/O ==", 
           __FUNCTION__);
    
    /*! @todo Currently the only fixed pattern that rdgen supports is
     *        zeros.
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                             VINCENT_VAN_GHOUL_FIXED_PATTERN,
                                             0,    /* Run forever */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             fbe_test_sep_io_get_threads(vincent_van_ghoul_threads),    /* threads */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,    /* Number of stripes to write. */
                                             VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    /* If peer I/O is enabled - send I/O through peer
     */
    
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s dual sp .set I/O to go thru peer", __FUNCTION__);
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    if (ran_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s random aborts inserted %d msecs", __FUNCTION__, ran_abort_msecs);
        status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification, ran_abort_msecs);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end vincent_van_ghoul_start_fixed_io()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_wait_for_rebuild_start()
 *****************************************************************************
 *
 * @brief   Wait for the specified raid group to `start' rebuilding.  Then
 *          perform any needed cleanup.
 *
 * @param   rg_config_p - Pointer to raid group configurations
 * @param   raid_group_count - Number of raid group configurations
 * @param   rg_object_id_to_wait_for - Raid group object id to wait for rebuild
 *                          to start
 *
 * @return  None.
 *
 * @author
 *  01/12/201   Ron Proulx - Created
 *
 *****************************************************************************/
static void vincent_van_ghoul_wait_for_rebuild_start(fbe_test_rg_configuration_t * const rg_config_p,
                                         const fbe_u32_t raid_group_count,
                                         const fbe_object_id_t rg_object_id_to_wait_for)
{
    fbe_u32_t       position_to_rebuild;
    fbe_u32_t       num_positions_to_rebuild;
    fbe_u32_t       rg_index, index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;

    /* First wait for the specified raid group to start the rebuild
     */
    fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(rg_object_id_to_wait_for);
    
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_wait_for_raid_group_to_start_rebuild(rg_object_id_to_wait_for);
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }

    /* Since we are not wait for the rebuild to complete remove from spare array
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* We use the `drives_needing_spare' array to determine which positions
         * need to be rebuilt.  We assume that there is at least (1) position
         * that needs to be rebuilt.
         */
        num_positions_to_rebuild = fbe_test_sep_util_get_num_needing_spare(current_rg_config_p);
        MUT_ASSERT_TRUE(num_positions_to_rebuild > 0);
        for (index = 0; index < num_positions_to_rebuild; index++)
        {
            /* If this is the index reserved for copy operations skip it.*/
            if (index == VINCENT_VAN_GHOUL_INDEX_RESERVED_FOR_COPY)
            {
                continue;
            }

            /* Get the next position to rebuild
             */
            position_to_rebuild = fbe_test_sep_util_get_needing_spare_position_from_index(current_rg_config_p, index);

            /* If this is the position */

            /* Remove the position from the `needing spare' array
             */
            fbe_test_sep_util_delete_needing_spare_position(current_rg_config_p, position_to_rebuild);

        } /* end for all positions that need to be rebuilt */

        current_rg_config_p++;

    } /* end for all raid groups */

    return;
}
/******************************************
 * end vincent_van_ghoul_wait_for_rebuild_start()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_wait_for_rebuild_completion()
 *****************************************************************************
 *
 * @brief   Wait for the required number of positions to complete
 *          rebuild.
 *
 * @param   rg_config_p - Pointer to raid group configuraiton to wait for
 * @param   b_wait_for_all_degraded_positions_to_rebuild - FBE_TRUE - Wait for
 *                  all positions to be fully rebuilt
 *                                                       - FBE_FALSE - Only wait
 *                  for (1) position to rebuild
 * @return  None.
 *
 * @author
 * 03/12/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void vincent_van_ghoul_wait_for_rebuild_completion(fbe_test_rg_configuration_t * const rg_config_p,
                                                   fbe_bool_t b_wait_for_all_degraded_positions_to_rebuild)
{
    fbe_object_id_t rg_object_id;
    fbe_u32_t       position_to_rebuild;
    fbe_u32_t       num_positions_to_rebuild;
    fbe_u32_t       index;

    /* There is no need to wait if the rg_config is not enabled
    */
    if (!fbe_test_rg_config_is_enabled(rg_config_p))
    {            
        return;
    }

    /* Get the riad group object id
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* We use the `drives_needing_spare' array to determine which positions
     * need to be rebuilt.  We assume that there is at least (1) position
     * that needs to be rebuilt.
     */
    num_positions_to_rebuild = fbe_test_sep_util_get_num_needing_spare(rg_config_p);
    MUT_ASSERT_TRUE(num_positions_to_rebuild > 0);

    /* Walk all the `needing spare' positions, wait for them to rebuild then
     * remove them from the `needing spare' array.
     */
    for (index = 0; index < num_positions_to_rebuild; index++)
    {
        fbe_status_t status;

        /* If this is the index reserved for copy operations skip it.*/
        if (index == VINCENT_VAN_GHOUL_INDEX_RESERVED_FOR_COPY)
        {
            continue;
        }

        /* Get the next position to rebuild
         */
        position_to_rebuild = fbe_test_sep_util_get_needing_spare_position_from_index(rg_config_p, index);

        /* If we need to wait for all positions or this is the position we
         * need to wait for.
         */
        if (b_wait_for_all_degraded_positions_to_rebuild == FBE_TRUE)
        {
            /* Wait for the rebuild to complete
             */
            status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position_to_rebuild,
                                                  FBE_TEST_SEP_UTIL_RB_WAIT_SEC);           
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position_to_rebuild,
                                                  FBE_TEST_SEP_UTIL_RB_WAIT_SEC);           
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }

        /* Remove the position from the `needing spare' array
         */
        fbe_test_sep_util_delete_needing_spare_position(rg_config_p, position_to_rebuild);

    } /* end for all positions that need to be rebuilt */

    return;
}
/*********************************************
 * end vincent_van_ghoul_wait_for_rebuild_completion()
 **********************************************/

/*!**************************************************************
 * vincent_van_ghoul_remove_drives()
 ****************************************************************
 * @brief
 *  Remove drives, one per raid group.
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param copy_position - The position that is or will be used for the copy
 *                          operation.
 * @param b_transition_pvd_fail - FBE_TRUE - Force PVD to fail 
 * @param b_pull_drive - FBE_TRUE - Pull and re-insert same drive
 *                       FBE_FALSE - Replace drive 
 * @param removal_mode - random or sequential
 *
 * @return status - Typically FBE_STATUS_OK,  Othersie we failed to
 *          locate a position to remove
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *  05/23/2011 - Modified. Amit Dhaduk
 *
 ****************************************************************/
static fbe_status_t vincent_van_ghoul_get_next_position_to_remove(fbe_test_rg_configuration_t *rg_config_p,
                                                           fbe_u32_t copy_position,
                                                           fbe_test_drive_removal_mode_t removal_mode,
                                                           fbe_u32_t *next_position_to_remove_p)
{
    fbe_status_t    status = FBE_STATUS_FAILED;
    fbe_u32_t       remove_index = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_u32_t       position_to_remove = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_u32_t       can_be_removed[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t       random_position = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_u32_t       removed_position;

    /* Initialize the position to remove to invalid.
     */
    *next_position_to_remove_p = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;

    /* Check if there any position that haven't been removed.
     */
    if ((rg_config_p->width - rg_config_p->num_removed) == 0)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: width: %d minus num removed: %d is 0", 
                   __FUNCTION__, rg_config_p->width, rg_config_p->num_removed);

        /* There are no drives available to be removed.
        */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the copy_position.
     */
    if (copy_position >= rg_config_p->width)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: copy pos: %d is greater or equal width: %d", 
                   __FUNCTION__, copy_position, rg_config_p->width);

        /* There are no drives available to be removed.
        */
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /*! @note Currently we prevent the removal of the copy position.  Therefore
     *        validate that it isn't removed.
     */
    for (remove_index = 0; remove_index < rg_config_p->num_removed; remove_index++)
    {

        /* Sanity check that the removed position is present in the array.
         */
        MUT_ASSERT_TRUE(remove_index < rg_config_p->width)
        MUT_ASSERT_TRUE(rg_config_p->drives_removed_array[remove_index] != VINCENT_VAN_GHOUL_INVALID_DISK_POSITION);

        removed_position = rg_config_p->drives_removed_array[remove_index];
        MUT_ASSERT_TRUE(removed_position < rg_config_p->width)

        /* Validate tha this isn't the copy positin.
         */
        MUT_ASSERT_INT_NOT_EQUAL(copy_position, removed_position);
    }

    /* Generate the can be removed array
     * First initialize all the positions to being present (but mark the
     * copy position as removed).
     */
    for (remove_index = 0; remove_index < rg_config_p->width; remove_index++)
    {
        if (remove_index == copy_position)
        {
            can_be_removed[remove_index] = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
        }
        else
        {
            can_be_removed[remove_index] = remove_index;
        }
    }

    /* For each position found on the drives removed array, set that position to
     * removed in our local array.
     */
    for (remove_index = 0; remove_index < rg_config_p->num_removed; remove_index++)
    {
        fbe_u32_t   removed_position;

        /* Sanity check that the removed position is present in the array.
         */
        MUT_ASSERT_TRUE(remove_index < rg_config_p->width)
        MUT_ASSERT_TRUE(rg_config_p->drives_removed_array[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION);

        removed_position = rg_config_p->drives_removed_array[remove_index];
        MUT_ASSERT_TRUE(removed_position < rg_config_p->width)

        can_be_removed[removed_position] = FBE_TEST_SEP_UTIL_INVALID_POSITION;
    }

    /* Check which removal mode is requested.
     */
    switch (removal_mode)
    {
        case FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL:
            /* Loop through the 'can be removed' array to find the first position that can be removed.
             */
            remove_index = 0;
            while (remove_index != rg_config_p->width)
            {
                if (can_be_removed[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
                {
                    /* Found a position to be removed.
                     * Set position_to_remove to the position to be removed.
                     */
                    position_to_remove = can_be_removed[remove_index];
                    break;
                }
                remove_index++;
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        case FBE_DRIVE_REMOVAL_MODE_RANDOM:
            /* Pick a random number from the number of drives available to remove.
             * The number of drives available should be nonzero.
             */
            MUT_ASSERT_TRUE((rg_config_p->width - rg_config_p->num_removed) != 0);
            random_position = fbe_random() % (rg_config_p->width - rg_config_p->num_removed);

            /* Loop through until and skip 'random_position' number of available drives to
             * return a random drive.
             */
            remove_index = 0;
            while (remove_index != rg_config_p->width)
            {
                if (can_be_removed[remove_index] != FBE_TEST_SEP_UTIL_INVALID_POSITION)
                {
                    if (random_position == 0)
                    {
                        /* Found a position to be removed.
                         * Set position_to_remove to the position to be removed.
                         */
                        position_to_remove = can_be_removed[remove_index];
                        break;
                    }
                    else
                    {
                        random_position--;
                    }
                }
                remove_index++;
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        case FBE_DRIVE_REMOVAL_MODE_SPECIFIC:
            /* Loop through the 'can be removed' array to find the first position that can be removed.
             */
            remove_index = 0;
            for ( remove_index = 0; remove_index < rg_config_p->width; remove_index++)
            {                
                /* Found a position to be removed.
                 * Set position_to_remove to the position to be removed.
                 */
                position_to_remove = rg_config_p->specific_drives_to_remove[remove_index];                    
                if(position_to_remove == rg_config_p->drives_removed_array[remove_index])
                {                    
                    continue;
                }

               break;                               
            }

            /* Should not have reached the end of the array since there
             * should be a drive available to remove.
             */
            MUT_ASSERT_TRUE(remove_index != rg_config_p->width);
            MUT_ASSERT_TRUE(position_to_remove != FBE_TEST_SEP_UTIL_INVALID_POSITION);
            break;

        default:
            /* Not a valid algorithm.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Invalid removal algorithm %d\n", 
                       __FUNCTION__, removal_mode);
            MUT_ASSERT_TRUE(FBE_FALSE);
            break;
    }

    MUT_ASSERT_TRUE(position_to_remove < rg_config_p->width);
    *next_position_to_remove_p = position_to_remove;

    return FBE_STATUS_OK;
}
/**********************************************
 * end vincent_van_ghoul_get_next_position_to_remove()
 **********************************************/

/*!**************************************************************
 * vincent_van_ghoul_remove_drives()
 ****************************************************************
 * @brief
 *  Remove drives, one per raid group.
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param copy_position - The position that is or will be used for the copy
 *                          operation.
 * @param b_transition_pvd_fail - FBE_TRUE - Force PVD to fail 
 * @param b_pull_drive - FBE_TRUE - Pull and re-insert same drive
 *                       FBE_FALSE - Replace drive 
 * @param removal_mode - random or sequential
 *
 * @return None.
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *  05/23/2011 - Modified. Amit Dhaduk
 *
 ****************************************************************/
static void vincent_van_ghoul_remove_drives(fbe_test_rg_configuration_t *rg_config_p, 
                                     fbe_u32_t raid_group_count,
                                     fbe_u32_t copy_position,
                                     fbe_bool_t b_transition_pvd_fail,
                                     fbe_bool_t b_pull_drive,
                                     fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_remove_p = NULL;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t       position_to_remove; 

    /* For every raid group remove one drive.
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Skip copy position
         */
        status = vincent_van_ghoul_get_next_position_to_remove(current_rg_config_p,
                                                        copy_position,
                                                        removal_mode,
                                                        &position_to_remove);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_test_sep_util_add_removed_position(current_rg_config_p, position_to_remove);        
        drive_to_remove_p = &current_rg_config_p->rg_disk_set[position_to_remove];

        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Removing drive raid group: %d position: 0x%x (%d_%d_%d)", 
                   __FUNCTION__, rg_index, position_to_remove, 
                   drive_to_remove_p->bus, drive_to_remove_p->enclosure, drive_to_remove_p->slot);

        if (b_transition_pvd_fail)
        {
            fbe_object_id_t pvd_id;
            status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_remove_p->bus,
                                                                    drive_to_remove_p->enclosure,
                                                                    drive_to_remove_p->slot,
                                                                    &pvd_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_set_object_to_state(pvd_id, FBE_LIFECYCLE_STATE_FAIL, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else 
        {

           /* We are going to pull a drive.
             */
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure, 
                                                     drive_to_remove_p->slot,
                                                     &current_rg_config_p->drive_handle[position_to_remove]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_pull_drive(drive_to_remove_p->bus, 
                                                         drive_to_remove_p->enclosure, 
                                                         drive_to_remove_p->slot,
                                                         &current_rg_config_p->peer_drive_handle[position_to_remove]);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
            else
            {
                status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                     drive_to_remove_p->enclosure, 
                                                     drive_to_remove_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    /* Set the target server to SPB and remove the drive there. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_pull_drive_hw(drive_to_remove_p->bus, 
                                                            drive_to_remove_p->enclosure, 
                                                            drive_to_remove_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
        }
        current_rg_config_p++;
    }

    /* Now wait for the drives to be destroyed.
     * We intentionally separate out the waiting from the actions to allow all the 
     * transitioning of drives to occur in parallel. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        /* The last drive removed for this raid group is at the end of the removed drive array.
         */
        position_to_remove = current_rg_config_p->drives_removed_array[current_rg_config_p->num_removed - 1];

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              vd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              VINCENT_VAN_GHOUL_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end vincent_van_ghoul_remove_drives()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_remove_all_drives()
 *****************************************************************************
 *
 * @brief   For all raid groups, remove a the number of drives specified but
 *          skip the copy position.
 *
 * @param   rg_config_p - Our configuration.
 * @param   raid_group_count - Number of rgs in config.
 * @param   copy_position - The position that is or will be used for the copy
 *                          operation.
 * @param   position_to_remove - The position to remove
 * @param   drives_to_remove - Number of drives we are removing for each raid group
 * @param   msecs_between_removals - Milliseconds to wait in between removals.
 * @param   removal_mode - random or sequential
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *  5/23/2011 - Modified. Amit Dhaduk
 *
 *****************************************************************************/
void vincent_van_ghoul_remove_all_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                  fbe_u32_t raid_group_count, 
                                  fbe_u32_t copy_position,
                                  fbe_u32_t drives_to_remove,
                                  fbe_bool_t b_reinsert_same_drive,
                                  fbe_u32_t msecs_between_removals,
                                  fbe_test_drive_removal_mode_t removal_mode)
{
    fbe_u32_t drive_number;

    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);


    if (b_reinsert_same_drive == FBE_FALSE)
    {
        /* Refresh the locations of the all extra drives in each raid group. 
         * This info can change as we swap in spares. 
         */
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);
    }

    /* For each raid group remove the specified number of drives.
     */
    for (drive_number = 0; drive_number < drives_to_remove; drive_number++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive index %d of %d. ==", __FUNCTION__, drive_number, drives_to_remove);
        vincent_van_ghoul_remove_drives(rg_config_p, raid_group_count, copy_position,
                                 FBE_FALSE,    /* don't fail pvd */
                                 b_reinsert_same_drive,
                                 removal_mode);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s removing drive index %d of %d. Completed. ==", __FUNCTION__, drive_number, drives_to_remove);
    }
    return;
}
/******************************************
 * end vincent_van_ghoul_remove_all_drives()
 ******************************************/

/*!**************************************************************
 * vincent_van_ghoul_insert_all_drives()
 ****************************************************************
 * @brief
 *  Remove all the drives for this test.
 *
 * @param rg_config_p - Our configuration.
 * @param raid_group_count - Number of rgs in config.
 * @param drives_to_insert - Number of drives we are removing.
 *
 * @return None.
 *
 * @author
 *  6/30/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void vincent_van_ghoul_insert_all_drives(fbe_test_rg_configuration_t * rg_config_p, 
                                         fbe_u32_t raid_group_count, 
                                         fbe_u32_t drives_to_insert,
                                         fbe_bool_t b_reinsert_same_drive)
{
    fbe_u32_t drive_number;
    fbe_u32_t num_to_insert = 0;
    fbe_u32_t rg_index;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;

    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            break;
        }
        current_rg_config_p++;
    }
    MUT_ASSERT_NOT_NULL(current_rg_config_p);

    /* Assumption is that all raid groups under test have the same number of
     * drives removed.
     */
    num_to_insert = fbe_test_sep_util_get_num_removed_drives(current_rg_config_p);

    /* If we are inserting different drive use the `extra_disk_set'.
     */
    if (b_reinsert_same_drive == FBE_FALSE)
    {
        /* Populate the number of extra drive to allow rebuild.
         */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Refresh the location of the all drives in each raid group. 
         * This info can change as we swap in spares. 
         */
        fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, raid_group_count);

        /* Refresh the locations of the all extra drives in each raid group. 
         * This info can change as we swap in spares. 
         */
        fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

        /* to insert new drive, configure extra drive as spare 
         */
         fbe_test_sep_drive_configure_extra_drives_as_hot_spares(rg_config_p, raid_group_count);

        /* make sure that hs swap in complete 
         */        
         fbe_test_sep_drive_wait_extra_drives_swap_in(rg_config_p, raid_group_count);


         /* set target server to SPA (test expects SPA will be left as target server and previous call may change that) 
          */
         fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    }
    else
    {

        /* insert the same drive back 
         */
        for (drive_number = 0; drive_number < num_to_insert; drive_number++)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting drive index %d of %d. ==", __FUNCTION__, drive_number, num_to_insert);
            big_bird_insert_drives(rg_config_p, 
                                   raid_group_count, 
                                   FBE_FALSE);    /* don't fail pvd */

            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting drive index %d of %d. Complete. ==", __FUNCTION__, drive_number, num_to_insert);
        }
    }

    return;
}
/******************************************
 * end vincent_van_ghoul_insert_all_drives()
 ******************************************/

/*!**************************************************************
 *          vincent_van_ghoul_set_debug_flags()
 ****************************************************************
 *
 * @brief   Set any debug flags for all the raid groups specified
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed            
 *
 * @return  None.
 *
 * @author
 * 03/16/2010   Ron Proulx - Created
 *
 ****************************************************************/
void vincent_van_ghoul_set_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                           fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;
    
    /* If debug is not enabled, simply return
     */
    if (vincent_van_ghoul_b_debug_enable == FBE_FALSE)
    {
        return;
    }
    
    /* Populate the raid group debug flags to the value desired.
     * (there can only be 1 set of debug flag override)
     */
    raid_group_debug_flags = vincent_van_ghoul_object_debug_flags;
        
    /* Populate the raid library debug flags to the value desired.
     */
    raid_library_debug_flags = fbe_test_sep_util_get_raid_library_debug_flags(vincent_van_ghoul_library_debug_flags);
    
    /* For all the raid groups specified set the raid library debug flags
     * to the desired value.
     */
    for (rg_index_to_change = 0; rg_index_to_change < raid_group_count; rg_index_to_change++)
    {
        fbe_object_id_t                 rg_object_id;
        fbe_api_trace_level_control_t   control;
    
    /* Set the debug flags for raid groups that we are interested in.
     * Currently is the is the the 3-way non-optimal raid group.
     */
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    
     /* Set the raid group debug flags
      */
     status = fbe_api_raid_group_set_group_debug_flags(rg_object_id, 
                                                       raid_group_debug_flags);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     mut_printf(MUT_LOG_TEST_STATUS, "== %s set raid group debug flags to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_group_debug_flags, rg_object_id);
   
     /* Set the raid library debug flags
      */
     status = fbe_api_raid_group_set_library_debug_flags(rg_object_id, 
                                                         raid_library_debug_flags);
     MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

     mut_printf(MUT_LOG_TEST_STATUS, "== %s set raid library debug flags to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_library_debug_flags, rg_object_id);

     /* Set the state trace flags for this object (if enabled)
      */
     if (vincent_van_ghoul_enable_state_trace == FBE_TRUE)
     {
         control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
         control.fbe_id = rg_object_id;
         control.trace_level = FBE_TRACE_LEVEL_INVALID;
         control.trace_flag = FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE;
         status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);
         mut_printf(MUT_LOG_TEST_STATUS, "%s enabled lifecyclee trace flags for object: %d", 
                    __FUNCTION__, rg_object_id);
     }

        /* Goto to next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups specified */

    return;
}
/******************************************
 * end vincent_van_ghoul_set_debug_flags()
 ******************************************/

/*!**************************************************************
 *          vincent_van_ghoul_clear_debug_flags()
 ****************************************************************
 *
 * @brief   Clear any debug flags for all the raid groups specified
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed           
 *
 * @return  None.
 *
 * @author
 * 03/16/2010   Ron Proulx - Created
 *
 ****************************************************************/
void vincent_van_ghoul_clear_debug_flags(fbe_test_rg_configuration_t * const rg_config_p,
                             fbe_u32_t raid_group_count)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index_to_change;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_library_debug_flags_t  raid_library_debug_flags;
    fbe_test_rg_configuration_t *   current_rg_config_p = rg_config_p;

    /* If debug is not enabled, simply return
     */
    if (vincent_van_ghoul_b_debug_enable == FBE_FALSE)
    {
        return;
    }

    /* For all the raid groups specified clear the raid library debug flags
     */
    for (rg_index_to_change = 0; rg_index_to_change < raid_group_count; rg_index_to_change++)
    {
        fbe_object_id_t                 rg_object_id;
    fbe_api_trace_level_control_t   control;

    /* Clear the debug flags for raid groups that we are interested in.
     */
    fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Get the current value of the raid group debug flags.
     */
    status = fbe_api_raid_group_get_group_debug_flags(rg_object_id,
                                                      &raid_group_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear the raid group debug flags
     */
    status = fbe_api_raid_group_set_group_debug_flags(rg_object_id,
                                                      0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s raid group debug flags changed from: 0x%08x to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_group_debug_flags, 0, rg_object_id);

    /* Get the current value of the raid library debug flags.
     */
    status = fbe_api_raid_group_get_library_debug_flags(rg_object_id,
                                                        &raid_library_debug_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Clear the raid library debug flags
     */
    status = fbe_api_raid_group_set_library_debug_flags(rg_object_id,
                                                        0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s raid library debug flags changed from: 0x%08x to 0x%08x for rg_object_id: %d == ", 
               __FUNCTION__, raid_library_debug_flags, 0, rg_object_id);

    /* Clear the state trace (if enabled)
     */
    if (vincent_van_ghoul_enable_state_trace == FBE_TRUE)
    {
        control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
        control.fbe_id = rg_object_id;
        control.trace_level = FBE_TRACE_LEVEL_INVALID;
        control.trace_flag = 0;
        status = fbe_api_trace_set_flags(&control, FBE_PACKAGE_ID_SEP_0);
        mut_printf(MUT_LOG_TEST_STATUS, "%s disabled lifecycle trace flags for object: %d", 
                   __FUNCTION__, rg_object_id);
    }

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups */

    return;
}
/******************************************
 * end vincent_van_ghoul_clear_debug_flags()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_populate_destination_drives()
 *****************************************************************************
 *
 * @brief   First disable `automatic' sparing.  Then determine which free drives 
 *          will be used for each of the raid groups under test and then add 
 *          them to the `extra' information for each of the raid groups under 
 *          test.  
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   dest_array_pp - Address of pointer to array of destination drive
 *
 * @return  FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  03/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_populate_destination_drives(fbe_test_rg_configuration_t *rg_config_p,   
                                                           fbe_u32_t raid_group_count,
                                                           fbe_test_raid_group_disk_set_t **dest_array_pp)           
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t       rg_index;
    fbe_u32_t       dest_index = 0;

    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_RG_CONFIG_ARRAY_MAX_RG_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Refresh the `extra' disk information.
     */
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    /* Step 2: Disable `automatic' sparing selection.  We need to do this since
     *         the user copy API must specify a specific destination PVD.
     */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* Step 3: Walk thru all the raid groups and add the `extra' drive to
     *         the array of destination drives.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Validate that there is at least (1) extra disk.
         */
        MUT_ASSERT_TRUE(current_rg_config_p->num_of_extra_drives > 0);

        /* Add (1) `extra' drive per raid group to the `destination' array.
         */
        vincent_van_ghoul_dest_array[dest_index] = current_rg_config_p->extra_disk_set[0];

        /* Goto next raid group.
         */
        current_rg_config_p++;
        dest_index++;
    }

    /* Update the address passed with the array of destination drives.
     */
    *dest_array_pp = &vincent_van_ghoul_dest_array[0];

    /* Return success
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end vincent_van_ghoul_populate_destination_drives()
 ***************************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_merge_debug_hooks()
 *****************************************************************************
 *
 * @brief   Merge the debug hooks of (2) sep_params.
 *
 * @param   sep_params1_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 * @param   sep_params2_p - Pointer to second hook array
 * @param   sep_params_out_p - The merged arrays
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  11/13/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_merge_debug_hooks(fbe_sep_package_load_params_t *sep_params1_p,
                                                        fbe_sep_package_load_params_t *sep_params2_p,
                                                        fbe_sep_package_load_params_t *sep_params_out_p)

{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       out_hook_index = 0;
    fbe_u32_t       hook_index;
    
    /* Initialize out
     */
    fbe_zero_memory(sep_params_out_p, sizeof(*sep_params_out_p));

    /* Copy the first `other' parameters.
     */
    sep_params_out_p->raid_library_debug_flags = sep_params1_p->raid_library_debug_flags;
    sep_params_out_p->raid_group_debug_flags = sep_params1_p->raid_group_debug_flags;
    sep_params_out_p->virtual_drive_debug_flags = sep_params1_p->virtual_drive_debug_flags;
    sep_params_out_p->pvd_class_debug_flags = sep_params1_p->pvd_class_debug_flags;
	sep_params_out_p->lifecycle_class_debug_flags = sep_params1_p->lifecycle_class_debug_flags;
    sep_params_out_p->default_trace_level = sep_params1_p->default_trace_level;
    sep_params_out_p->error_limit = sep_params1_p->error_limit;
    sep_params_out_p->cerr_limit = sep_params1_p->cerr_limit;
    
    /* Now copy any hooks.
     */
    for (hook_index = 0; hook_index < MAX_SCHEDULER_DEBUG_HOOKS; hook_index++)
    {
        /* Check the source1
         */
        if ((sep_params1_p->scheduler_hooks[hook_index].object_id != 0)                     &&
            (sep_params1_p->scheduler_hooks[hook_index].object_id != FBE_OBJECT_ID_INVALID)    )
        {
            MUT_ASSERT_TRUE(out_hook_index < MAX_SCHEDULER_DEBUG_HOOKS);
            sep_params_out_p->scheduler_hooks[out_hook_index++] = sep_params1_p->scheduler_hooks[hook_index];
        }

        /* Check the source2
         */
        if ((sep_params2_p->scheduler_hooks[hook_index].object_id != 0)                     &&
            (sep_params2_p->scheduler_hooks[hook_index].object_id != FBE_OBJECT_ID_INVALID)    )
        {
            MUT_ASSERT_TRUE(out_hook_index < MAX_SCHEDULER_DEBUG_HOOKS);
            sep_params_out_p->scheduler_hooks[out_hook_index++] = sep_params2_p->scheduler_hooks[hook_index];
        }
    }

    /* Return
     */
    return status;
}
/*******************************************
 * end vincent_van_ghoul_merge_debug_hooks()
 *******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_reboot()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   rg_config_p - Array of raid groups to re-create
 * @param   sep_params_p - Pointer to `sep params' for the reboot of either this
 *              SP or the other SP.
 * @param   sep_params2_p - Pointer to second hook array
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_reboot(fbe_test_rg_configuration_t * const rg_config_p,
                                             fbe_sep_package_load_params_t *sep_params1_p,
                                             fbe_sep_package_load_params_t *sep_params2_p)
{   
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sep_package_load_params_t           sep_params = {0,};
    fbe_bool_t                              b_single_sp_fast_reboot = FBE_FALSE;
    fbe_bool_t                              b_dualsp_fast_reboot = FBE_TRUE;
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are in dualsp mode (the normal mode on the array), shutdown
     * the current SP to transfer control to the other SP.
     */
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        /* If both SPs are enabled shutdown this SP and validate that the other 
         * SP takes over.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Dualsp: %d shutdown SP: %d make current SP: %d ==", 
                   __FUNCTION__, b_is_dualsp_mode, this_sp, other_sp);

        /* Only use the `quick' destroy if `preserve data' is not set.
         */
        if (b_dualsp_fast_reboot == FBE_TRUE)
        {
            fbe_api_sim_transport_destroy_client(this_sp);
            fbe_test_sp_sim_stop_single_sp(this_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);
        }
        else
        {
            if (fbe_test_sep_util_get_encryption_test_mode()) {
                fbe_test_sep_util_destroy_kms();
            }
            fbe_test_sep_util_destroy_neit_sep();
        }

        /* Set the transport server to the correct SP.
         */
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        this_sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(this_sp, other_sp);
    }
    else
    {
        /* Else simply reboot this SP. (currently no specific reboot params for NEIT).
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot current SP: %d ==", 
                   __FUNCTION__, this_sp);
        if (b_single_sp_fast_reboot == FBE_TRUE) 
        {
            fbe_api_sim_transport_destroy_client(this_sp);
            fbe_test_sp_sim_stop_single_sp(this_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);
            fbe_test_boot_sp(rg_config_p, this_sp);
        }
        else
        {
            status = vincent_van_ghoul_merge_debug_hooks(sep_params1_p, sep_params2_p, &sep_params);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_common_reboot_this_sp(&sep_params, NULL /* No params for NEIT*/); 
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }  

    return status;
}
/******************************************
 * end vincent_van_ghoul_reboot()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_reboot_cleanup()
 *****************************************************************************
 *
 * @brief   Depending if the test in running in single SP or dualsp mode, this
 *          method will either reboot `this' SP (single SP mode) or will reboot
 *          the `other' SP.
 *
 * @param   rg_config_p - Array of raid groups to create
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  06/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_reboot_cleanup(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_bool_t                              b_dualsp_fast_reboot = FBE_TRUE;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* Determine if we are in dualsp mode or not.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /* If we are single SP mode reboot this SP.
     */
    if (b_is_dualsp_mode == FBE_FALSE)
    {
        /* If we are in single SP mode there currently is no cleanup neccessary.
         */
    }
    else
    {
        /* Else we are in dualsp mode. Boot the `other' SP and use/set the same
         * debug hooks that we have for this sp.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Boot other SP: %d ==", 
                   __FUNCTION__, other_sp);

        /* Only use the `quick' create if `preserve data' is not set.
         */
        if (b_dualsp_fast_reboot == FBE_TRUE)
        {
            status = fbe_api_sim_transport_set_target_server(other_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            fbe_test_boot_sp(rg_config_p, other_sp);
            status = fbe_api_sim_transport_set_target_server(this_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        else
        {
            status = fbe_test_common_boot_sp(other_sp, 
                                             NULL,  /* No sep params*/ 
                                             NULL   /* No neit params*/);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
    }  

    return status;
}
/******************************************
 * end vincent_van_ghoul_reboot_cleanup()
 ******************************************/

/*!****************************************************************************
 *  vincent_van_ghoul_dual_sp_crash
 ******************************************************************************
 *
 * @brief
 *   Shutdown sep and neit on both sps
 *
 * @param   sep_params_p - pointer to the parameters (debug hooks) to start the sps with
 * @param   rg_config_p   - pointer to the RG config
 * @param   raid_group_count - Count of raid groups under test
 *
 * @return  None
 *
 *****************************************************************************/
static void vincent_van_ghoul_dual_sp_crash(fbe_sep_package_load_params_t *sep_params_p,
                                            fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_u32_t raid_group_count)
{
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_status_t                            status;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Dualsp crash==", __FUNCTION__);

    /* shutdown both sps 
     */
    fbe_api_sim_transport_set_target_server(this_sp);
    fbe_test_sep_util_destroy_neit_sep();
    fbe_api_sim_transport_set_target_server(other_sp);
    fbe_test_sep_util_destroy_neit_sep();

    /* boot up both sps 
     */
    status = fbe_test_common_boot_sp(this_sp, 
                                     sep_params_p,
                                     NULL   /* No neit params*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_common_boot_sp(other_sp, 
                                     sep_params_p,
                                     NULL   /* No neit params*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_util_wait_for_database_service(50000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_common_util_test_setup_init();
    fbe_api_sleep(20000);

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/**************************************
 * end vincent_van_ghoul_dual_sp_crash()
 **************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_proactive_shutdown_step1()
 *****************************************************************************
 *
 * @brief   This test populates all raid groups (LUNs) with a fixed pattern.  
 *          Then with fixed I/O running it initiates a proactive copy for each 
 *          raid group.  It then `pulls' sufficient drives to force the raid
 *          group to go shutdown.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group   
 *
 * @return  None.
 *
 * @author
 *  03/14/2012   Ron Proulx - Created
 *
 *****************************************************************************/
static void vincent_van_ghoul_proactive_shutdown_step1(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t luns_per_rg,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t            *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_status_t                        status;
    fbe_u32_t                           index;
    fbe_test_rg_configuration_t        *current_rg_config_p = NULL;
    fbe_u32_t                           position_to_spare;
    fbe_transport_connection_target_t   this_sp;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */

    /* First write the fixed pattern to the entire extent of all the LUNs
     */
    vincent_van_ghoul_write_fixed_pattern(this_sp, rg_config_p);

    /* Now start fixed I/O
     */
    vincent_van_ghoul_start_fixed_io(ran_abort_msecs);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Set debug flags.
     */
    vincent_van_ghoul_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    vincent_van_ghoul_wait_for_rdgen_start(VINCENT_VAN_GHOUL_RDGEN_WAIT_SECS);

    /* Force (1) drive for each raid group to enter proactive copy
     */
    position_to_spare = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Forcing drives into proactive copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_spare,
                                                              FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                              NULL /* The system selects the destination drives*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Started proactive copy successfully. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* No pull the first drive in each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing sufficient drives to shutdown raid group ==", __FUNCTION__);
    vincent_van_ghoul_remove_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 VINCENT_VAN_GHOUL_NUM_DRIVES_TO_FORCE_SHUTDOWN, 
                                 FBE_TRUE, /* We plan on re-inserting this drive later. */ 
                                 0, /* msecs between removals */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        vincent_van_ghoul_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }
    
    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors. we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }
    
    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all  drives. ==", __FUNCTION__);
    vincent_van_ghoul_insert_drives(rg_config_p, raid_group_count,
                             FBE_FALSE /* Don't force transition*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        
        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
        
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
 
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);
    
    /* Make sure the fixed pattern can be read still.
     */
    status = vincent_van_ghoul_read_fixed_pattern(this_sp, rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        vincent_van_ghoul_wait_for_rebuild_completion(current_rg_config_p,
                                               FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. Completed.==", __FUNCTION__);

    /* Restart fixed I/O
     */
    vincent_van_ghoul_start_fixed_io(ran_abort_msecs);

    /* Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_spare,
                                                                          FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                          FBE_FALSE, /* Don't wait for swap out*/
                                                                          FBE_TRUE   /* Skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Wait for the proactive copy to cleanup
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy cleanup. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_copy_operation_cleanup(rg_config_p, 
                                                                raid_group_count,
                                                                position_to_spare,
                                                                FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                FBE_FALSE /* Cleanup has not been performed */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy cleanup. ==", __FUNCTION__);

    /* Clear debug flags.
     */
    /*! @todo Currently need to leave them on here
    vincent_van_ghoul_clear_debug_flags();
    */

    return;
}
/******************************************
 * end vincent_van_ghoul_proactive_shutdown_step1()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_proactive_shutdown_step2()
 *****************************************************************************
 *
 * @brief   This test validates that pulling drives during a differential 
 *          rebuild doesn't cause error (rebuild request that hit dead positions
 *          would cause a deadlock with the monitor at one point in time).
 *          First we validate that rebuilds are active then we pull all drives
 *          to force the raid groups to be shutdown.  We then validate that
 *          the rebuilds complete without error.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group    
 *
 * @return  None.
 *
 * @author
 *  03/25/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void vincent_van_ghoul_proactive_shutdown_step2(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t luns_per_rg,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   position_to_spare;
    fbe_transport_connection_target_t   this_sp;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */

    /* First write the fixed pattern to the entire extent of all the LUNs
     */
    vincent_van_ghoul_write_fixed_pattern(this_sp, rg_config_p);

    /* Now start fixed I/O
     */
    vincent_van_ghoul_start_fixed_io(ran_abort_msecs);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Set debug flags.
     */
    vincent_van_ghoul_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    vincent_van_ghoul_wait_for_rdgen_start(VINCENT_VAN_GHOUL_RDGEN_WAIT_SECS);

    /* Force (1) drive for each raid group to enter proactive copy
     */
    position_to_spare = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Forcing drives into proactive copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_spare,
                                                              FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                              NULL /* The system selects the destination drives*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Started proactive copy successfully. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* No pull the first drive in each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing sufficient drives to shutdown raid group ==", __FUNCTION__);
    vincent_van_ghoul_remove_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 VINCENT_VAN_GHOUL_NUM_DRIVES_TO_FORCE_SHUTDOWN, 
                                 FBE_TRUE, /* We plan on re-inserting this drive later. */ 
                                 0, /* msecs between removals */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        vincent_van_ghoul_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors. we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all  drives. ==", __FUNCTION__);
    vincent_van_ghoul_insert_drives(rg_config_p, raid_group_count,
                             FBE_FALSE  /* Don't force transition*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {

        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Make sure the fixed pattern can be read still.
     */
    status = vincent_van_ghoul_read_fixed_pattern(this_sp, rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        vincent_van_ghoul_wait_for_rebuild_completion(current_rg_config_p, 
                                               FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. Completed.==", __FUNCTION__);

    /* Restart fixed I/O
     */
    vincent_van_ghoul_start_fixed_io(ran_abort_msecs);

    /* Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_spare,
                                                                          FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    /*! @todo Currently need to leave them on here
    vincent_van_ghoul_clear_debug_flags();
    */
}
/*******************************************
 * end vincent_van_ghoul_proactive_shutdown_step2()
 *******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_proactive_shutdown_step3()
 *****************************************************************************
 *
 * @brief   This test validates that there are no media error after a raid group
 *          is shutdown with I/O in progress.  It also validates that a raid
 *          group can be quiesced with a differential rebuild active.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group   
 *
 * @return  None.
 *
 * @author
 *  03/25/2010   Ron Proulx - Created
 *
 *****************************************************************************/
static void vincent_van_ghoul_proactive_shutdown_step3(fbe_test_rg_configuration_t * const rg_config_p,
                                          fbe_u32_t raid_group_count,
                                          fbe_u32_t luns_per_rg,
                                          fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t    *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_status_t                status;
    fbe_u32_t                   index;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t                   position_to_spare;
    fbe_transport_connection_target_t   this_sp;

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Step 1: Validate that no metadata errors are induced (including 512-bps
     *         raid groups) when all drives are sequentially removed from a
     *         mirror raid group.
     */

    /* First write the fixed pattern to the entire extent of all the LUNs
     */
    vincent_van_ghoul_write_fixed_pattern(this_sp, rg_config_p);

    /* Now start fixed I/O
     */
    vincent_van_ghoul_start_fixed_io(ran_abort_msecs);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Set debug flags.
     */
    vincent_van_ghoul_set_debug_flags(rg_config_p, raid_group_count);

    /* Wait for  I/O to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rdgen to start. ==", __FUNCTION__);
    vincent_van_ghoul_wait_for_rdgen_start(VINCENT_VAN_GHOUL_RDGEN_WAIT_SECS);

    /* Force (1) drive for each raid group to enter proactive copy
     */
    position_to_spare = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Forcing drives into proactive copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_spare,
                                                              FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                              NULL /* The system selects the destination drives*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Started proactive copy successfully. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* No pull the first drive in each raid group.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing sufficient drives to shutdown raid group ==", __FUNCTION__);
    vincent_van_ghoul_remove_all_drives(rg_config_p, raid_group_count, position_to_spare,
                                 VINCENT_VAN_GHOUL_NUM_DRIVES_TO_FORCE_SHUTDOWN, 
                                 FBE_TRUE, /* We plan on re-inserting this drive later. */ 
                                 0, /* msecs between removals */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* Make sure the raid groups goto fail.
     */
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++ )
    {
        vincent_van_ghoul_wait_for_raid_group_to_fail(current_rg_config_p);
        current_rg_config_p++;
    }

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors. we can expect errors when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Now re-insert the same drives and validate that the rg becomes ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives. ==", __FUNCTION__);
    vincent_van_ghoul_insert_drives(rg_config_p, raid_group_count,
                           FBE_FALSE /* Don't force transition*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-inserting all drives successful. ==", __FUNCTION__);

    /* Wait for all objects to become ready including raid groups.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {

        fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            fbe_test_sep_util_wait_for_all_objects_ready(current_rg_config_p);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }

        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Make sure the fixed pattern can be read still.
     */
    status = vincent_van_ghoul_read_fixed_pattern(this_sp, rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    current_rg_config_p = rg_config_p;
    for ( index = 0; index < raid_group_count; index++)
    {
        vincent_van_ghoul_wait_for_rebuild_completion(current_rg_config_p,
                                               FBE_TRUE /* Wait for all positions to rebuild*/);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. Completed.==", __FUNCTION__);

    /* Restart fixed I/O
     */
    vincent_van_ghoul_start_fixed_io(ran_abort_msecs);

    /* Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_spare,
                                                                          FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND,
                                                                          FBE_TRUE, /* Wait for swap out */
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);

    /* Allow I/O to continue for a few seconds.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Clear debug flags.
     */
    /*! @todo Currently need to leave them on here
    vincent_van_ghoul_clear_debug_flags();
    */
}
/******************************************
 * end vincent_van_ghoul_proactive_shutdown_step3()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_wait_for_database_rollback_to_complete()
 *****************************************************************************
 *
 * @brief   Wait for the database rollback to put the virtual drive back into
 *          mirror mode with (2) edges.
 *
 * @param   vd_object_id - Virtual drive object id to wait for
 * @param   timeout_ms - Timeout in milliseconds
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  06/03/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t vincent_van_ghoul_wait_for_database_rollback_to_complete(fbe_object_id_t vd_object_id,
                                                                             fbe_u32_t timeout_ms)
{
    fbe_status_t                    status;
    fbe_api_virtual_drive_get_info_t vd_info;

    /* Display the virtual drive information prior to the rollback
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "wait for db rollback start - vd obj: 0x%x mode: %d chkpt: 0x%llx swap edge: %d",
               vd_object_id, vd_info.configuration_mode, 
               vd_info.vd_checkpoint, vd_info.swap_edge_index);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "wait for db rollback start - source obj: 0x%x dest obj: 0x%x request-in-progress: %d copy complete: %d",
               vd_info.original_pvd_object_id, vd_info.spare_pvd_object_id,
               vd_info.b_request_in_progress, vd_info.b_is_copy_complete);

    /* Wait for the debug hook when the Virtual drive is in mirror mode.
     */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0,
                                                 0); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the database to be ready.
     */
    status = fbe_test_sep_util_wait_for_database_service_ready(timeout_ms);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the debug hook
     */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET,
                                            0,
                                            0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Display the virtual drive information after to the rollback
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "wait for db rollback end   - vd obj: 0x%x mode: %d chkpt: 0x%llx swap edge: %d",
               vd_object_id, vd_info.configuration_mode, 
               vd_info.vd_checkpoint, vd_info.swap_edge_index);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "wait for db rollback end   - source obj: 0x%x dest obj: 0x%x request-in-progress: %d copy complete: %d",
               vd_info.original_pvd_object_id, vd_info.spare_pvd_object_id,
               vd_info.b_request_in_progress, vd_info.b_is_copy_complete);

    /* Success
     */
    return FBE_STATUS_OK;
}
/********************************************************************
 * end vincent_van_ghoul_wait_for_database_rollback_to_complete()
 ********************************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_run_shutdown_test()
 *****************************************************************************
 *
 * @brief   This test executes the following:
 *          1.  Validate that no metadata errors are induced (including 512-bps
 *              raid groups) when all drives are sequentially removed from a
 *              mirror raid group.
 *              o Write a fixed pattern to all blocks
 *              o Start fixed I/O at queue depth
 *              o Pull all drives for each raid group (delay between each pull)
 *              o Re-insert all drives (delay between each insert)
 *              o Wait for raid group to become ready (but don't wait for rebuild)
 *              o Validate that all data can be read without error (even if stale)
 *              o Validate that rebuilds start
 *          2.  Validate that drive pulls during rebuild don't cause issues
 *              o Previously the raid group monitor would deadlock when a rebuild
 *                request hit a new dead position (since the monitor is waiting
 *                for the rebuild request it cannot process the edge state change
 *                yet the raid algorithms are waiting for the dead acknowledgement
 *                from the object).
 *              o Wait for rebuilds to complete
 *          3.  Validate no media errors with normal I/O with shutdown
 *              o Start normal I/O at queue depth
 *              o Pull all drives for each raid group (delay between each pull)
 *              o Re-insert all drives (delay between each insert)
 *              o Wait for raid group to become ready
 *              o Wait for rebuilds to complete (so that we can destroy the environment
 *                without any outstanding requests (considered a memory leak)
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group   
 *
 * @return  None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from vincent_van_ghoul_test.c
 *
 *****************************************************************************/

static void vincent_van_ghoul_run_shutdown_test(fbe_test_rg_configuration_t * const rg_config_p,
                                    fbe_u32_t raid_group_count,
                                    fbe_u32_t luns_per_rg,
                                    fbe_test_random_abort_msec_t ran_abort_msecs)
{

    /* Step 1: Validate that no I/O errors are induced when a procative copy
     *         operation is interrupted with a shutdown.
     */
    vincent_van_ghoul_proactive_shutdown_step1(rg_config_p, raid_group_count, luns_per_rg,
                                        ran_abort_msecs);

    /* Step 2:  Validate that drive pulls during rebuild don't cause issues
     */
    /*! @todo this test needs to be re-worked */
    /*vincent_van_ghoul_degraded_shutdown_step2(rg_config_p, raid_group_count, luns_per_rg,
                                                            ran_abort_msecs);*/

    /* Step 3:  Validate no media errors with normal I/O with shutdown
     */
    /*! @todo this test needs to be re-worked */
    /*vincent_van_ghoul_degraded_shutdown_step3(rg_config_p, raid_group_count, luns_per_rg,
                                                            ran_abort_msecs);*/
   
    return;
}
/******************************************
 * end vincent_van_ghoul_run_shutdown_test()
 ******************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_test_degraded_user_copy()
 *****************************************************************************
 *
 * @brief   Raid groups are already degraded.  Start a user copy then
 *          insert the drives.
 *              1) Start I/O
 *              2) Start a user copy operation
 *              3) Replace the previously removed drives (i.e. full rebuild)
 *              4) Wait for copy operation to complete
 *              5) Wait for rebuilds to complete
 *              6) Stop I/O and validate no errors
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group 
 *
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 *
 * @param   rdgen_operation - The type of operation (w/r/c or z/r/c etc)
 *
 * @param   ran_abort_msecs - If not 0, the time in milliseconds to abort  
 *
 * @return None.
 *
 * @note    All copy operations to degraded raid groups will fail.  Since the
 *          background operation utility used doesn't allow a failure, we only
 *          degraded the raid group after the copy operation is started.
 *
 * @author
 * 04/22/2012   Ron Proulx - Created.
 *
 *****************************************************************************/
static void vincent_van_ghoul_test_degraded_user_copy(fbe_test_rg_configuration_t * const rg_config_p,
                                            fbe_u32_t raid_group_count,
                                            fbe_u32_t luns_per_rg,
                                            fbe_bool_t b_use_original_drives,
                                            fbe_rdgen_operation_t rdgen_operation,
                                            fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t                *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_u32_t                               remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_spare_swap_command_t                copy_type = FBE_SPARE_INITIATE_USER_COPY_COMMAND;
    fbe_u32_t                               position_to_copy;
    fbe_u32_t                               position_to_remove;
    fbe_bool_t                              b_is_dualsp_test;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
  
    /* Set the SP to the active for the virtual drive.
     */
    status = vincent_van_ghoul_get_sp_info(rg_config_p, &b_is_dualsp_test,
                                           &this_sp, &other_sp, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting degraded I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, vincent_van_ghoul_threads, VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS);
    status = vincent_van_ghoul_start_io(passive_sp, rg_config_p,
                                        rdgen_operation, ran_abort_msecs,
                                        vincent_van_ghoul_threads, VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting degraded I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);
    
    /* Step 2: Start a user copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting user copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_copy,
                                                              copy_type,
                                                              NULL /* Don't specify destination */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy started - successfully. ==", __FUNCTION__);

    /* Step 3: Remove drives.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    vincent_van_ghoul_remove_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 1, 
                                 b_use_original_drives, /* We plan on re-inserting this drive later. */ 
                                 remove_delay, /* msecs between removals */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);


    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Replace the previously removed drives (i.e. force a full rebuild)
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives ==", __FUNCTION__);
    vincent_van_ghoul_insert_all_drives(rg_config_p, raid_group_count, 1,
                                 FBE_FALSE /* Replace failed drives */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives: Successful. ==", __FUNCTION__);

    /* Step 6: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Set debug flags.
     */
    vincent_van_ghoul_set_debug_flags(rg_config_p, raid_group_count);

    /* Step 7: Wait for the proactive copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for user copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_copy,
                                                                          copy_type,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy complete. ==", __FUNCTION__);

    /* Allow I/O to continue for some time.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_long);   

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O: Successful ==", __FUNCTION__);

    /* Step 8: Validate no errors. We can expect abort failures if we have injected them
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    return;
}
/****************************************************
 * end vincent_van_ghoul_test_degraded_user_copy()
 ****************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_test_reboot_during_copy_rebuild_start()
 *****************************************************************************
 *
 * @brief   Write a background pattern to the raid group, start proactive copy,
 *          pull drives, make sure we can read the pattern back degraded.
 *              1) Start proactive copy
 *              2) Wait for the virtual drive paged rebuild to start
 *              3) Shutdown active sp
 *              4) Wait for the virtual drive paged rebuild to start on the now active SP
 *              5) Remove the rebuild hook and validate copy completes
 *              6) Wait for proactive copy cleanup (clear EOL)
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group 
 *
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 *
 * @param   background_operation - The type of operation (w/r/c or z/r/c etc)
 *
 * @param   ran_abort_msecs - If not 0, the time in milliseconds to abort  
 *
 * @return None.
 *
 * @note    All copy operations to degraded raid groups will fail.  Since the
 *          background operation utility used doesn't allow a failure, we only
 *          degraded the raid group after the copy operation is started.
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void vincent_van_ghoul_test_reboot_during_copy_rebuild_start(fbe_test_rg_configuration_t * const rg_config_p,
                                                      fbe_u32_t raid_group_count,
                                                      fbe_u32_t luns_per_rg,
                                                      fbe_bool_t b_use_original_drives,
                                                      fbe_rdgen_operation_t background_operation,
                                                      fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                            status;
    fbe_spare_swap_command_t                copy_type = FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND;
    fbe_u32_t                               position_to_remove = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_u32_t                               position_to_copy = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_object_id_t                         source_pvd_object_id;
    fbe_test_sep_background_copy_state_t    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID;
    fbe_test_sep_background_copy_state_t    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID; 
    fbe_test_sep_background_copy_event_t    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_INVALID;
    fbe_bool_t                              b_is_dualsp_test;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_base_config_encryption_mode_t       encryption_mode;
    fbe_bool_t                              b_is_encryption_enabled = FBE_FALSE;
  
    /* Set the SP to the active for the virtual drive.
     */
    status = vincent_van_ghoul_get_sp_info(rg_config_p, &b_is_dualsp_test,
                                           &this_sp, &other_sp, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine the copy position
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;

    /* If encryption is enabled on the source, set the scrub hooks.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position_to_copy].bus,
                                                            rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                            rg_config_p->rg_disk_set[position_to_copy].slot,
                                                            &source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_base_config_get_encryption_mode(source_pvd_object_id, &encryption_mode);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting encryption mode failed\n");
    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)      ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)                 ) {
        b_is_encryption_enabled = FBE_TRUE;
    }

    /*! @note Due to the fact that the pvds can enter the failed state due to a 
     *        reboot we need to increase the sparing timer so that reboots don't
     *        unexpectedly cause the source and/or destination to fail (which
     *        will result in the test failing).
     */
    if (vincent_van_ghoul_b_does_reboot_cause_path_failure == FBE_TRUE)
    {
        fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);
    }

    /* Step 1: Write the background pattern.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        status = vincent_van_ghoul_write_zero_background_pattern(this_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = vincent_van_ghoul_write_background_pattern(this_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 2: Start the proactive copy and `pause' when the virtual drive starts
     *         the rebuild of the paged metadata.
     */

    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &vincent_van_ghoul_pause_params,
                                                               0,
                                                               FBE_TRUE, /* This is a reboot test*/
                                                               NULL      /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy started - successfully. ==", __FUNCTION__); 

    /* Step 3: Wait for rebuild start hook*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for metadata rebuild start hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &vincent_van_ghoul_pause_params,
                                                         FBE_FALSE /* Only wait on the active SP */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4:  Reboot and validate that the proactive proceeds.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP ==", __FUNCTION__);
    status = vincent_van_ghoul_reboot(rg_config_p,
                                      &vincent_van_ghoul_pause_params,
                                      &vincent_van_ghoul_debug1_hooks);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP - complete ==", __FUNCTION__);

    /* Step 5: Wait for virtual drive to start rebuilding.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_METADATA_REBUILD_START;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &vincent_van_ghoul_pause_params,
                                                               0,
                                                               FBE_FALSE,   /* Only remove the hook on the active*/
                                                               NULL         /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate metadata rebuild start on new active - successful. ==", __FUNCTION__); 

    /* Step 6: Make sure the background pattern can be read still.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        status = vincent_van_ghoul_read_fixed_pattern(passive_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = vincent_van_ghoul_read_background_pattern(passive_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 6a: If encryption is enabled wait for all the virtual drives to get their
     *          keys before we reboot.
     */
    if (b_is_encryption_enabled) {
        status = fbe_test_sep_background_pause_wait_for_encryption_state(rg_config_p, raid_group_count, position_to_copy,
                                                                         FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_REMOVED);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 7:  Perform any reboot cleanup required.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = vincent_van_ghoul_reboot_cleanup(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /* Step 8: Make sure the background pattern can be read still.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        status = vincent_van_ghoul_read_fixed_pattern(active_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = vincent_van_ghoul_read_background_pattern(active_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 9: Need to perform cleanup (clear EOL etc).
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &vincent_van_ghoul_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test*/
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__); 

    /*! @note If we increased the `trigger spare timer' due to reboot causes
     *        path failures, set it back.
     */
    if (vincent_van_ghoul_b_does_reboot_cause_path_failure == FBE_TRUE)
    {
        fbe_test_sep_util_update_permanent_spare_trigger_timer(1);
    }

    return;
}
/************************************************************
 * end vincent_van_ghoul_test_reboot_during_copy_rebuild_start()
 ************************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_test_reboot_during_copy_start_mark_nr()
 *****************************************************************************
 *
 * @brief   I/O runs differently depending on dualsp mode:
 *              o Single SP - Write then verify background pattern after reboot
 *              o Dual SP - Run I/O during the reboot on the Passive only
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group 
 *
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 *
 * @param   rdgen_operation - The type of operation (w/r/c or z/r/c etc)
 *
 * @param   ran_abort_msecs - If not 0, the time in milliseconds to abort  
 *
 * @return None.
 *
 * @note    All copy operations to degraded raid groups will fail.  Since the
 *          background operation utility used doesn't allow a failure, we only
 *          degraded the raid group after the copy operation is started.
 *
 * @author
 *  02/13/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void vincent_van_ghoul_test_reboot_during_copy_start_mark_nr(fbe_test_rg_configuration_t *const rg_config_p,
                                                                   fbe_u32_t raid_group_count,
                                                                   fbe_u32_t luns_per_rg,
                                                                   fbe_bool_t b_use_original_drives,
                                                                   fbe_rdgen_operation_t rdgen_operation,
                                                                   fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                            status;
    fbe_spare_swap_command_t                copy_type = FBE_SPARE_INITIATE_USER_COPY_COMMAND;
    fbe_u32_t                               position_to_copy = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_test_sep_background_copy_state_t    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID;
    fbe_test_sep_background_copy_state_t    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID; 
    fbe_test_sep_background_copy_event_t    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_INVALID;
    fbe_bool_t                              b_is_dualsp_test;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
  
    /* Set the SP to the active for the virtual drive.
     */
    status = vincent_van_ghoul_get_sp_info(rg_config_p, &b_is_dualsp_test,
                                           &this_sp, &other_sp, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note Due to the fact that the pvds can enter the failed state due to a 
     *        reboot we need to increase the sparing timer so that reboots don't
     *        unexpectedly cause the source and/or destination to fail (which
     *        will result in the test failing).
     */
    if (vincent_van_ghoul_b_does_reboot_cause_path_failure == FBE_TRUE)
    {
        fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);
    }


    /* Step 1: Start I/O on the passive SP
     */
    position_to_copy = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    status = vincent_van_ghoul_start_io(passive_sp, rg_config_p,
                                        rdgen_operation, ran_abort_msecs,
                                        vincent_van_ghoul_threads, VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 2: Set a debug hook when mark NR starts
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set mark NR start hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &vincent_van_ghoul_debug1_hooks,
                                                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                                               FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE /* Only set the hook on the local */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Start the user copy and `pause' when the virtual drive is set
     *         to mirror mode.
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &vincent_van_ghoul_pause_params,
                                                               0,
                                                               FBE_TRUE, /* This is a reboot test*/
                                                               NULL      /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy started - successfully. ==", __FUNCTION__); 
  
    /* Step 4: Wait for mirror mode the active SP then remove the hook.
     */
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_MODE_SET_TO_MIRROR;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &vincent_van_ghoul_pause_params,
                                                               0,
                                                               FBE_TRUE, /* This is a reboot test*/
                                                               NULL      /* No destination required*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate mirror mode - successful. ==", __FUNCTION__);
    
    /* Step 5: Wait for eval mark NR hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for mark NR start hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &vincent_van_ghoul_debug1_hooks,
                                                         FBE_FALSE /* Only wait on the active SP */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6:  Remove the `local' debug hook so that the copy proceeds
     *          after the reboot.
     */
    fbe_zero_memory(&vincent_van_ghoul_debug1_hooks, sizeof(fbe_sep_package_load_params_t));
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP ==", __FUNCTION__);
    status = vincent_van_ghoul_reboot(rg_config_p,
                                      &vincent_van_ghoul_pause_params,
                                      &vincent_van_ghoul_debug1_hooks);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP - complete ==", __FUNCTION__);

    /* Step 7: Need to perform cleanup
     */
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &vincent_van_ghoul_pause_params,
                                                               0,
                                                               FBE_FALSE, /* This is not a reboot test*/
                                                               NULL      /* No destination required*/);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__);
    
    /* Step 8: Stop I/O and validate status.
     */
    status = vincent_van_ghoul_stop_io(passive_sp, rg_config_p, 
                                       rdgen_operation, ran_abort_msecs);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9:  Perform any reboot cleanup required.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = vincent_van_ghoul_reboot_cleanup(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /*! @note If we increased the `trigger spare timer' due to reboot causes
     *        path failures, set it back.
     */
    if (vincent_van_ghoul_b_does_reboot_cause_path_failure == FBE_TRUE)
    {
        fbe_test_sep_util_update_permanent_spare_trigger_timer(1);
    }

    return;
}
/**************************************************************
 * end vincent_van_ghoul_test_reboot_during_copy_start_mark_nr()
 **************************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_test_user_copy_to_with_full_rebuild()
 *****************************************************************************
 *
 * @brief   Start with raid groups non-degraded.  Degraded the raid groups then
 *          start a user copy.  Re-insert the removed drives.  First wait for
 *          rebuild to complete then wait for copy to complete.
 *              1) Start I/O
 *              2) Start a user copy operation
 *              3) Remove drives
 *              4) Replace removed drives (i.e. full rebuild)
 *              5) Wait for rebuild to complete
 *              6) Wait for the user copy operation to complete
 *              7) Validate no errors
 *              8) Move the spares back to the automatic mode
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group  
 *
 * @param   rdgen_operation - The type of operation (w/r/c or z/r/c etc)  
 *
 * @param   ran_abort_msecs - If not 0, the time in milliseconds to abort      
 *
 * @return None.
 *
 * @author
 * 04/22/2012   Ron Proulx - Created.
 *
 *****************************************************************************/
static void vincent_van_ghoul_test_user_copy_to_with_full_rebuild(fbe_test_rg_configuration_t * const rg_config_p,
                                        fbe_u32_t raid_group_count,
                                        fbe_u32_t luns_per_rg,
                                        fbe_rdgen_operation_t rdgen_operation,
                                        fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t                *context_p = &vincent_van_ghoul_test_contexts[0];
    fbe_u32_t                               remove_delay = fbe_test_sep_util_get_drive_insert_delay(FBE_U32_MAX);
    fbe_spare_swap_command_t                copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    fbe_u32_t                               position_to_remove;
    fbe_u32_t                               position_to_copy;
    fbe_test_raid_group_disk_set_t         *dest_array_p = NULL;
    fbe_bool_t                              b_is_dualsp_test;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
  
    /* Set the SP to the active for the virtual drive.
     */
    status = vincent_van_ghoul_get_sp_info(rg_config_p, &b_is_dualsp_test,
                                           &this_sp, &other_sp, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 1: Start I/O
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O threads: %d blks: 0x%x ==", 
               __FUNCTION__, vincent_van_ghoul_threads, VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS);
    status = vincent_van_ghoul_start_io(passive_sp, rg_config_p,
                                        rdgen_operation, ran_abort_msecs,
                                        vincent_van_ghoul_threads, VINCENT_VAN_GHOUL_MAX_IO_SIZE_BLOCKS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting I/O - successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Step 2: Start a user copy request.
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;
    status = vincent_van_ghoul_populate_destination_drives(rg_config_p, raid_group_count, &dest_array_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting user copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p,
                                                              raid_group_count,
                                                              position_to_copy,
                                                              copy_type,
                                                              dest_array_p /* Specify the destination drives */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy started - successfully. ==", __FUNCTION__);

    /* Step 3: Remove drives.
     */

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives. ==", __FUNCTION__);
    vincent_van_ghoul_remove_all_drives(rg_config_p, raid_group_count, position_to_copy,
                                 1, 
                                 FBE_FALSE, /* This is a full rebuild test: replace drives*/
                                 remove_delay, /* msecs between removals */
                                 FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Removing first drives: Successful. ==", __FUNCTION__);

    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Step 4: Validate that the raid groups are degraded.
     */
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, raid_group_count,
                                                 position_to_remove);

    /* Step 5: Replace the previously removed drives (i.e. force a full rebuild)
     */
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives ==", __FUNCTION__);
    //vincent_van_ghoul_insert_all_removed_drives(rg_config_p, raid_group_count, 1,
    vincent_van_ghoul_insert_all_drives(rg_config_p, raid_group_count, 1,
                                 FBE_FALSE /* Replace failed drives */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Replacing drives: Successful. ==", __FUNCTION__);

    /* Step 6: Wait for the rebuilds to finish for ALL possible degraded positions
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);
    
    /* Step 7: Wait for the user copy to complete
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for proactive copy complete. ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_wait_for_copy_operation_complete(rg_config_p, 
                                                                          raid_group_count,
                                                                          position_to_copy,
                                                                          copy_type,
                                                                          FBE_TRUE, /* Wait for swap out*/
                                                                          FBE_FALSE /* Do not skip cleanup */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Proactive copy complete. ==", __FUNCTION__);
  
    /* Allow I/O to continue for a second.
     */
    fbe_api_sleep(vincent_van_ghoul_io_msec_short);

    /* Step 7: Stop I/O and validate no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(7) ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O(7): Successful ==", __FUNCTION__);

    /* Validate no errors. We can expect error when we inject random aborts
     */
    if(ran_abort_msecs == FBE_TEST_RANDOM_ABORT_TIME_INVALID)
    {
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    }

    /* Clear debug flags.
     */
    vincent_van_ghoul_clear_debug_flags(rg_config_p, raid_group_count);

    /* Step 9: Move all the drive to the automatic spare for the next test.
     */
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/*************************************************************
 * end vincent_van_ghoul_test_user_copy_to_with_full_rebuild()
 *************************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_test_reboot_during_copy_complete()
 *****************************************************************************
 *
 * @brief   Write the background pattern to all LUNs.  Start a user copy
 *          and wait for the source drive to be swapped out.  If encryption
 *          is enabled validate that zeroing (scrubbing) starts on the swapped
 *          out source drive.  Wait for the `set checkpoint to end marker' hook.
 *          Shutdown the active SP.  The database on the peer will now:
 *              1. Rollback any updated objects (since we have not persisted).
 *
 *              2. Re-start the copy complete job from the start. 
 *
 *          The test blocks the recovery queue on the peer and waits for the
 *          virtual drive to be in mirror mode.  Then it validates the 
 *          background pattern.  Then it removes the destination drive and
 *          re-validates the back ground pattern.
 *
 * @param   rg_config_p - Pointer to list of raid groups to run the
 *                        rebuild tests against.
 * @param   raid_group_count - Number of raid groups in the configuration
 *                             passed 
 * @param   luns_per_rg - Number of LUNs in each raid group 
 *
 * @param   b_use_original_drives - FBE_TRUE - Pull the orginal drives
 *                                  FBE_FALSE - Remove the drives
 *
 * @param   rdgen_operation - The type of operation (w/r/c or z/r/c etc)
 *
 * @param   ran_abort_msecs - If not 0, the time in milliseconds to abort  
 *
 * @return None.
 *
 * @author
 *  05/27/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void vincent_van_ghoul_test_reboot_during_copy_complete(fbe_test_rg_configuration_t * const rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t luns_per_rg,
                                                               fbe_bool_t b_use_original_drives,
                                                               fbe_rdgen_operation_t background_operation,
                                                               fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                            status;
    fbe_spare_swap_command_t                copy_type = FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND;
    fbe_u32_t                               position_to_remove = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_u32_t                               position_to_copy = VINCENT_VAN_GHOUL_INVALID_DISK_POSITION;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         vd_object_id;
    fbe_object_id_t                         source_pvd_object_id;
    fbe_test_sep_background_copy_state_t    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID;
    fbe_test_sep_background_copy_state_t    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID; 
    fbe_test_sep_background_copy_event_t    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_INVALID;
    fbe_test_raid_group_disk_set_t         *dest_array_p = NULL;
    fbe_bool_t                              b_is_dualsp_test;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_base_config_encryption_mode_t       encryption_mode;
    fbe_bool_t                              b_is_encryption_enabled = FBE_FALSE;
    fbe_api_virtual_drive_get_info_t        vd_info;
    fbe_test_raid_group_disk_set_t          removed_drive_location;
    fbe_api_terminator_device_handle_t      removed_drive_handle;
    fbe_api_provision_drive_get_swap_pending_t drive_offline_info;
  
    /* Set the SP to the active for the virtual drive.
     */
    status = vincent_van_ghoul_get_sp_info(rg_config_p, &b_is_dualsp_test,
                                           &this_sp, &other_sp, &active_sp, &passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note This is currently a dualsp only test.
     */
    if (!b_is_dualsp_test) {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s is a dualsp only test  ==", 
               __FUNCTION__);
        return;
    }

    /*! @note Due to the fact that the pvds can enter the failed state due to a 
     *        reboot we need to increase the sparing timer so that reboots don't
     *        unexpectedly cause the source and/or destination to fail (which
     *        will result in the test failing).
     */
    if (vincent_van_ghoul_b_does_reboot_cause_path_failure == FBE_TRUE) {
        fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);
    }

    /* Get the provision object to set debug hook, etc. 
     */
    position_to_remove = fbe_test_sep_util_get_next_position_to_remove(rg_config_p, FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
    position_to_copy = position_to_remove + 1;
    status = vincent_van_ghoul_populate_destination_drives(rg_config_p, raid_group_count, &dest_array_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position_to_copy].bus,
                                                            rg_config_p->rg_disk_set[position_to_copy].enclosure,
                                                            rg_config_p->rg_disk_set[position_to_copy].slot,
                                                            &source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note We do NOT use background operation `pause' since those APIs 
     *        disable the swap confirmation!
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If encryption is enabled on the source, set the scrub hooks.
     */
    status = fbe_api_base_config_get_encryption_mode(source_pvd_object_id, &encryption_mode);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting encryption mode failed\n");
    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)      ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)                 ) {
        b_is_encryption_enabled = FBE_TRUE;
    }

    /* Step 1: Write the background pattern.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Write background pattern: %d  ==", 
               __FUNCTION__, background_operation);
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        status = vincent_van_ghoul_write_zero_background_pattern(this_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    } else {
        status = vincent_van_ghoul_write_background_pattern(this_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 2: Set a debug hook when the virtual drive initiates the copy
     *         complete request.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set initiate copy complete hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &vincent_van_ghoul_pause_params,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE /* Only set the hook on the local */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 3: Start the user copy
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting user copy ==", __FUNCTION__);
    status = fbe_test_sep_background_ops_start_copy_operation(rg_config_p, raid_group_count, position_to_copy,
                                                              copy_type,
                                                              dest_array_p /* Specify the destination drives */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s User copy started - successfully. ==", __FUNCTION__);

    /* Step 4: Wait for initiate copy complete hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for initiate copy complete hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &vincent_van_ghoul_pause_params,
                                                         FBE_FALSE /* Only wait on the active SP */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5: Make sure the background pattern can be read still.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK) {
        status = vincent_van_ghoul_read_fixed_pattern(passive_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    } else{
        status = vincent_van_ghoul_read_background_pattern(passive_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 6: Add a hook when the swap-out completes
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set swap-out complete hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &vincent_van_ghoul_debug1_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE /* Only set the hook on the local */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6a: If encryption is enabled stop when the swap-out is complete.
     */
    if (b_is_encryption_enabled) {
        /* Validate that the source drive is scrubbed after it is swapped out.
         */
        status = fbe_test_add_debug_hook_active(source_pvd_object_id,
                                                SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                                FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START,  
                                                NULL, NULL, 
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    } /* end if encryption is enabled set additional hooks*/

    /* Step 7: Add a hook when the virtual drive set the checkpoint to the end
     *         marker
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Set copy complete set checkpoint hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               &vincent_van_ghoul_debug2_hooks,
                                                               SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                                               FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE_SET_CHECKPOINT,
                                                               0, 0,
                                                               SCHEDULER_CHECK_STATES,
                                                               SCHEDULER_DEBUG_ACTION_PAUSE,
                                                               FBE_FALSE /* Only set the hook on the local */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8: Remove the initiate copy complete hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove initiate copy complete hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                  &vincent_van_ghoul_pause_params,
                                                                  FBE_FALSE /* Do not wait on passive*/);

    /* Step 9: Wait for swap-out complete hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for swap-out complete hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &vincent_van_ghoul_debug1_hooks,
                                                         FBE_FALSE /* Only wait on the active SP */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 10: Remove the swap-out complete hook.  This needs to be done BEFORE
     *         we check for scrubbing since it is the next step in the copy
     *         complete job (update source pvd config to `unconsumed') that
     *         triggers the scrubbing.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Remove swap-out complete hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_remove_copy_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                  &vincent_van_ghoul_debug1_hooks,
                                                                  FBE_FALSE /* Do not wait on passive*/);

    /* Step 11: Wait for set checkpoint to end marker hook
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for set checkpoint hook ==", __FUNCTION__);
    status = fbe_test_sep_background_pause_wait_for_hook(rg_config_p, raid_group_count, position_to_copy,
                                                         &vincent_van_ghoul_debug2_hooks,
                                                         FBE_FALSE /* Only wait on the active SP */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 12: Disable the recovery queue on the passive SP.  The database 
     *          will restart the copy complete job automatically but we
     *          want to validate the data before it starts.
     */
    status = fbe_api_sim_transport_set_target_server(passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 13:  Reboot and validate that the proactive proceeds.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP ==", __FUNCTION__);
    status = vincent_van_ghoul_reboot(rg_config_p,
                                      &vincent_van_ghoul_pause_params,
                                      &vincent_van_ghoul_debug1_hooks);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Rebooting SP - complete ==", __FUNCTION__);

    //Set a debug hook when the Virtual drive is in mirror mode.
    status = fbe_api_scheduler_add_debug_hook(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET,
                                            0,
                                            0,
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 14: Wait until the database rollback is complete.
     */
    status = vincent_van_ghoul_wait_for_database_rollback_to_complete(vd_object_id, VINCENT_VAN_GHOUL_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 15: Remove the destination drive.
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_location(vd_info.spare_pvd_object_id,
                                                  &removed_drive_location.bus,
                                                  &removed_drive_location.enclosure,
                                                  &removed_drive_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_drive_pull_drive(&removed_drive_location,
                                           &removed_drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 16: Add hook that the copy complete fails and an abort copy is issued.
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED, 
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 17: Now enable the recovery queue and wait for the job to be aborted.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);
    status = fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* Step 18: Validate the that copy job is aborted.
     */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                          SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                          FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED,
                                          SCHEDULER_CHECK_STATES, 
                                          SCHEDULER_DEBUG_ACTION_LOG, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 19: Wait for the virtual drive to become ready and the parent
     *          raid group to perform any rebuilds.
     */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(vd_object_id, 
                                                                        passive_sp,
                                                                        FBE_LIFECYCLE_STATE_READY, 
                                                                        VINCENT_VAN_GHOUL_WAIT_MSEC, 
                                                                        FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position_to_copy);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* Step 20: Now validate the data.
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        status = vincent_van_ghoul_read_fixed_pattern(passive_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = vincent_van_ghoul_read_background_pattern(passive_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 21: Need to perform cleanup
     */
    current_copy_state = FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_COMPLETE;
    copy_state_to_pause_at = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;
    validation_event = FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Current state: %d pause state: %d event: %d ==", 
               __FUNCTION__, current_copy_state, copy_state_to_pause_at, validation_event);
    status = fbe_test_sep_background_copy_operation_with_pause(rg_config_p, raid_group_count, position_to_copy, copy_type,
                                                               &current_copy_state,
                                                               copy_state_to_pause_at,
                                                               validation_event,
                                                               &vincent_van_ghoul_pause_params,
                                                               0,
                                                               FBE_TRUE, /* This is a reboot test*/
                                                               dest_array_p /* Destination drive array */);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Copy Cleanup - successful. ==", __FUNCTION__);

    /*! Step 21a: If encryption is enabled wait for the scrub to start.
     *
     *  @todo The original source drive has been marked offline.  We must manually
     *        bring it back.
     */
    if (b_is_encryption_enabled) {
        fbe_u32_t   bus;        
        fbe_u32_t   enclosure;  
        fbe_u32_t   slot;
        
        /* Get the location of the now swapped-out source
         */      
        status = fbe_api_provision_drive_get_location(source_pvd_object_id, 
                                                      &bus,
                                                      &enclosure,
                                                      &slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that the source drive is offline.
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              source_pvd_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                              20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_swap_pending(source_pvd_object_id,
                                                          &drive_offline_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(drive_offline_info.get_swap_pending_reason == FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY);

        /* Now clear the `mark offline' and validate that the original source drive
         * comes back online.
         */
        status = fbe_api_provision_drive_clear_swap_pending(source_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                              source_pvd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                              20000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_swap_pending(source_pvd_object_id,
                                                          &drive_offline_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(drive_offline_info.get_swap_pending_reason == FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_NONE);

        /* Validate that the source drive is scrubbed after it is swapped out.
         */
        status = fbe_test_wait_for_debug_hook_active(source_pvd_object_id, 
                                                     SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                                     FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START, 
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_LOG,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now wait for the zeroing to start.
         */
        status = fbe_test_sep_drive_check_for_disk_zeroing_event(source_pvd_object_id, 
                                                                 SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED,  
                                                                 bus, enclosure, slot); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now remove the scrub start hook.
         */
        status = fbe_test_del_debug_hook_active(source_pvd_object_id, 
                                                SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                                FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START, 
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    } /* end if encryption is enabled check and clear additional hooks*/

    /* Step 22: Re-insert any removed drives.
     */
    status = fbe_test_sep_drive_reinsert_drive(&removed_drive_location, removed_drive_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 23:  Perform any reboot cleanup required.
     */
    fbe_zero_memory(&vincent_van_ghoul_debug2_hooks, sizeof(fbe_sep_package_load_params_t));
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup ==", __FUNCTION__);
    status = vincent_van_ghoul_reboot_cleanup(rg_config_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Reboot cleanup - complete ==", __FUNCTION__);

    /*! @note If we increased the `trigger spare timer' due to reboot causes
     *        path failures, set it back.
     */
    if (vincent_van_ghoul_b_does_reboot_cause_path_failure == FBE_TRUE)
    {
        fbe_test_sep_util_update_permanent_spare_trigger_timer(1);
    }

    return;
}
/************************************************************
 * end vincent_van_ghoul_test_reboot_during_copy_complete()
 ************************************************************/

/*!***************************************************************************
 *          vincent_van_ghoul_determine_random_abort_msecs()
 *****************************************************************************
 *
 * @brief   Based on the input time, determine the random abort
 *          time.
 *
 * @param   ran_abort_msecs_in - Setting of random abort msecs.
 *              FBE_TEST_RANDOM_ABORT_TIME_INVALID - Random aborts are disabled.          
 *
 * @return  random_abort_msecs - FBE_TEST_RANDOM_ABORT_TIME_INVALID
 *
 * @author
 *  03/09/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_test_random_abort_msec_t vincent_van_ghoul_determine_random_abort_msecs(fbe_test_random_abort_msec_t ran_abort_msecs_in)
{
    fbe_u32_t                       random_msecs;
    fbe_test_random_abort_msec_t    random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;

    if (ran_abort_msecs_in == FBE_TEST_RANDOM_ABORT_TIME_INVALID) {
        return FBE_TEST_RANDOM_ABORT_TIME_INVALID;
    }

    random_msecs = fbe_random() % 5;
    switch(random_msecs) {
        case 0:
            random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_ONE_TENTH_MSEC;
            break;

        case 1:
        case 2:
            random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC;
            break;

        case 3:
        default:
            random_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;
            break;
    }

    return random_abort_msecs;
}
/******************************************************
 * end vincent_van_ghoul_determine_random_abort_msecs()
 ******************************************************/

/*!**************************************************************
 *          vincent_van_ghoul_run_tests_for_config()
 ****************************************************************
 * @brief
 *  Run the set of tests that make up the big bird test.
 *
 * @param rg_config_p - Config to create.
 * @param raid_group_count - Total number of raid groups.
 * @param luns_per_rg - Luns to create per rg.          
 *
 * @return none
 *
 * @author
 *  03/09/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
void vincent_van_ghoul_run_tests_for_config(fbe_test_rg_configuration_t * rg_config_p, 
                                     fbe_u32_t raid_group_count, 
                                     fbe_u32_t luns_per_rg,
                                     fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_rdgen_operation_t       background_operation = FBE_RDGEN_OPERATION_ZERO_READ_CHECK;
    fbe_rdgen_operation_t       rdgen_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
    fbe_sim_transport_connection_target_t this_sp;

    /* Refresh the extra disk information before configuring the spares.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    /*! @note Since this is a degraded test suite we assume that we will be
     *        degrading the raid groups.  Since the test assume a specific
     *        drive will be spared, disable automatic sparing and then move
     *        all the spare drives to the automatic spare pool.
     */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_remove_hotspares_from_hotspare_pool(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize out `short' and `long' I/O times
     */
    vincent_van_ghoul_set_io_seconds();

    /* We write the entire raid group and read it all back after the 
     * rebuild has finished. 
     */
    if (background_operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        status = vincent_van_ghoul_write_zero_background_pattern(this_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = vincent_van_ghoul_write_background_pattern(this_sp, rg_config_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Speed up VD hot spare during testing */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    mut_printf(MUT_LOG_TEST_STATUS, "%s Running for test level %d", __FUNCTION__, test_level);

    /* If we are doing shutdown only, then skip the degraded tests.
     */
    if (!fbe_test_sep_util_get_cmd_option("-shutdown_only"))
    {
        /* Test 1: Reboot the active SP just as the virutal drive marks NR for
         *         the destination drive.
         */
        vincent_van_ghoul_test_reboot_during_copy_start_mark_nr(rg_config_p, raid_group_count, luns_per_rg,
                                                                FBE_FALSE, 
                                                                rdgen_operation, 
                                                                vincent_van_ghoul_determine_random_abort_msecs(ran_abort_msecs));

        /* Test 2: Reboot the active SP after the copy starts to rebuild
         *         the user area.
         */
        vincent_van_ghoul_test_reboot_during_copy_rebuild_start(rg_config_p, raid_group_count, luns_per_rg,
                                                                FBE_FALSE, 
                                                                background_operation, 
                                                                vincent_van_ghoul_determine_random_abort_msecs(ran_abort_msecs));

        /*! Test 3:  User copy rolls back after swap out.
         *
         *  After the swap-out of the source reboot which should cause a rollback??
         *  @note Only (1) raid group is used for this test
         */
         vincent_van_ghoul_test_reboot_during_copy_complete(rg_config_p, 1 /*! @note Only first raid group is tested. */, luns_per_rg,
                                                            FBE_FALSE, 
                                                            rdgen_operation, 
                                                            vincent_van_ghoul_determine_random_abort_msecs(ran_abort_msecs));

        /* Test 4: User Copy with I/O active
         *
         * With the raid groups already degraded start I/O, then start and user
         * copy.
         */
        /*! @todo This test is not yet implemented
         vincent_van_ghoul_test_degraded_user_copy(rg_config_p, raid_group_count, luns_per_rg,
                                                   FBE_FALSE, rdgen_operation, ran_abort_msecs);
         */

         /* Test 5: Full Rebuild with User Copy To
          *
          * Invoke method that will run the rebuild tests for all active raid groups
          */
         /*! @todo This test is not yet implemented
         vincent_van_ghoul_test_user_copy_to_with_full_rebuild(rg_config_p, raid_group_count, luns_per_rg, 
                                                               FBE_FALSE, rdgen_operation, ran_abort_msecs); 
          */
    }
 
    /*! @todo Other tests are disabled.
     */
    if (vincent_van_ghoul_b_are_non_simple_tests_disabled_due_to_DE572 == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Shutdown test disabled due to DE572", __FUNCTION__);
    }
    else
    {
        /* Test 4: Shutdown raid groups with I/O
         *
         * Invoke method that will run I/O and then force a shutdown
         */
        vincent_van_ghoul_run_shutdown_test(rg_config_p, raid_group_count, luns_per_rg,
                                            vincent_van_ghoul_determine_random_abort_msecs(ran_abort_msecs));
    }

    /* Restore the spare swap-in timer back to default of 5 minutes.
     */
	fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    return;
}
/******************************************
 * end vincent_van_ghoul_run_tests_for_config()
 ******************************************/

/*!**************************************************************
 *          vincent_van_ghoul_run_tests()
 ****************************************************************
 * @brief
 *  We create a config and run the rebuild tests.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  08/20/2010  Ron Proulx  - Created from big_bird_run_tests
 *
 ****************************************************************/
static void vincent_van_ghoul_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_status_t    status;
    fbe_u32_t       raid_group_count = 0;
    fbe_u32_t       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_random_abort_msec_t ran_abort_msecs = FBE_TEST_RANDOM_ABORT_TIME_INVALID;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Configure the dualsp mode.
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_TRUE, FBE_TEST_SEP_IO_DUAL_SP_MODE_PEER_ONLY);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {       
        status = fbe_test_sep_io_configure_dualsp_io_mode(FBE_FALSE, FBE_TEST_SEP_IO_DUAL_SP_MODE_INVALID);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Determine if we should run with aborts.
     */
    if (test_level > 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, " Test started with random aborts msecs : %d", 
                   FBE_TEST_RANDOM_ABORT_TIME_TWO_SEC);
    }

    /* Always run the standard test.
     */
    vincent_van_ghoul_run_tests_for_config(rg_config_p, raid_group_count, 
                                           VINCENT_VAN_GHOUL_LUNS_PER_RAID_GROUP, 
                                           ran_abort_msecs);

    return;
}
/**************************************
 * end vincent_van_ghoul_run_tests()
 **************************************/

/*!**************************************************************
 * vincent_van_ghoul_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from vincent_van_ghoul_test.c
 *
 ****************************************************************/

void vincent_van_ghoul_test(void)
{
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Test all raid groups at the same time.
     */
    if (test_level > 0)
    {
        rg_config_p = &vincent_van_ghoul_raid_groups_extended[0];
    }
    else
    {
        rg_config_p = &vincent_van_ghoul_raid_groups_qual[0];
    }

    /* Run the test.
     */
    vincent_van_ghoul_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL, vincent_van_ghoul_run_tests,
                                          VINCENT_VAN_GHOUL_LUNS_PER_RAID_GROUP,
                                          VINCENT_VAN_GHOUL_CHUNKS_PER_LUN,
                                          FBE_FALSE);

    return;
}
/******************************************
 * end vincent_van_ghoul_test()
 ******************************************/

/*!**************************************************************
 * vincent_van_ghoul_encrypted_test()
 ****************************************************************
 * @brief
 *  vincent van ghoul sp encryption test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void vincent_van_ghoul_encrypted_test(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    vincent_van_ghoul_test();
}
/*************************************************
 * end vincent_van_ghoul_dualsp_encrypted_test()
 *************************************************/

/*!**************************************************************
 * vincent_van_ghoul_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/
void vincent_van_ghoul_setup(void)
{    
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        /* Randomize which raid type to use.
         */
        if (test_level > 0)
        {
            /* Extended.
             */
            rg_config_p = &vincent_van_ghoul_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            rg_config_p = &vincent_van_ghoul_raid_groups_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Initialize the number of extra drives required by RG. 
         * Used when create physical configuration for RG in simulation. 
         */
        fbe_test_rg_populate_extra_drives(rg_config_p, VINCENT_VAN_GHOUL_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        sep_config_load_sep_and_neit();
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            sep_config_load_kms(NULL);
            fbe_test_sep_util_enable_kms_encryption();
        }
    }
    
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
}
/**************************************
 * end vincent_van_ghoul_setup()
 **************************************/

/*!**************************************************************
 * vincent_van_ghoul_encrypted_setup()
 ****************************************************************
 * @brief
 *  vincent van ghoul sp encryption setup
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void vincent_van_ghoul_encrypted_setup(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    vincent_van_ghoul_setup();
}
/*************************************************
 * end vincent_van_ghoul_dualsp_encrypted_setup()
 *************************************************/

/*!**************************************************************
 * vincent_van_ghoul_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the scooby_doo test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 * 02/22/2010   Ron Proulx - Created from vincent_van_ghoul_test.c
 *
 ****************************************************************/

void vincent_van_ghoul_cleanup(void)
{
    //fbe_status_t status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        if (fbe_test_sep_util_get_encryption_test_mode()) {
            fbe_test_sep_util_destroy_kms();
        }
        fbe_test_sep_util_destroy_neit_sep_physical();
    }

    fbe_test_sep_util_set_encryption_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end vincent_van_ghoul_cleanup()
 ******************************************/

/*!**************************************************************
 * vincent_van_ghoul_encrypted_cleanup()
 ****************************************************************
 * @brief
 *  vincent van ghoul sp encryption setup
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void vincent_van_ghoul_encrypted_cleanup(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    vincent_van_ghoul_cleanup();
}
/*************************************************
 * end vincent_van_ghoul_dualsp_encrypted_cleanup()
 *************************************************/

/*!**************************************************************
 * vincent_van_ghoul_dualsp_test()
 ****************************************************************
 * @brief
 *  Run test on both SPs.
 *
 * @param  None             
 *
 * @return None.   
 *
 *
 ****************************************************************/

void vincent_van_ghoul_dualsp_test(void)
{
    fbe_u32_t                       test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t    *rg_config_p = NULL;

    /* Test all the raid groups at the same time.
     */
    if (test_level > 0)
    {
        rg_config_p = &vincent_van_ghoul_raid_groups_extended[0];
    }
    else
    {
        rg_config_p = &vincent_van_ghoul_raid_groups_qual[0];
    }

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Run the test.
     */
    vincent_van_ghoul_run_test_on_rg_config_with_extra_disk_with_time_save(rg_config_p, NULL, vincent_van_ghoul_run_tests,
                                          VINCENT_VAN_GHOUL_LUNS_PER_RAID_GROUP,
                                          VINCENT_VAN_GHOUL_CHUNKS_PER_LUN,
                                          FBE_FALSE);

    /* Always disable dualsp I/O after running the test
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end vincent_van_ghoul_dualsp_test()
 ******************************************/

/*!**************************************************************
 * vincent_van_ghoul_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 1 I/O test.
 *
 * @param None.               
 * 
 * @return None.   
 * 
 * @note    Must run in dual-sp mode
 * 
 ****************************************************************/
void vincent_van_ghoul_dualsp_setup(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        /* Randomize which raid type to use.
         */
        if (test_level > 0)
        {
            /* Extended.
             */
            rg_config_p = &vincent_van_ghoul_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            rg_config_p = &vincent_van_ghoul_raid_groups_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Initialize the number of extra drives required by RG. 
         * Used when create physical configuration for RG in simulation. 
         */
        fbe_test_rg_populate_extra_drives(rg_config_p, VINCENT_VAN_GHOUL_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, num_raid_groups);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

    }    
    
    return;
}
/**************************************
 * end vincent_van_ghoul_dualsp_setup()
 **************************************/

/*!**************************************************************
 * vincent_van_ghoul_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup vincent van ghoul dual sp test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/

void vincent_van_ghoul_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

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
 * end vincent_van_ghoul_dualsp_cleanup()
 ******************************************/

/*!**************************************************************
 *          vincent_van_ghoul_dualsp_encrypted_setup()
 ****************************************************************
 * @brief
 *  Setup for a copy with reboot with encryption enabled tests.
 *
 * @param None.               
 * 
 * @return None.   
 * 
 * @note    Must run in dual-sp mode
 * 
 ****************************************************************/
void vincent_van_ghoul_dualsp_encrypted_setup(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        /* Randomize which raid type to use.
         */
        if (test_level > 0)
        {
            /* Extended.
             */
            rg_config_p = &vincent_van_ghoul_raid_groups_extended[0];
        }
        else
        {
            /* Qual.
             */
            rg_config_p = &vincent_van_ghoul_raid_groups_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* Initialize the number of extra drives required by RG. 
         * Used when create physical configuration for RG in simulation. 
         */
        fbe_test_rg_populate_extra_drives(rg_config_p, VINCENT_VAN_GHOUL_NUM_OF_DRIVES_TO_RESERVE_FOR_COPY);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */
        fbe_test_create_physical_config_for_rg(rg_config_p, num_raid_groups);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

        /* Determine and load the physical config and populate all the 
         * raid groups.
         */        
        fbe_test_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();

        /* Enable encryption.
         */
		sep_config_load_kms_both_sps(NULL);

        status = fbe_test_sep_util_enable_kms_encryption();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }    
    
    return;
}
/*************************************************
 * end vincent_van_ghoul_dualsp_encrypted_setup()
 *************************************************/

/*!**************************************************************
 * vincent_van_ghoul_dualsp_encrypted_test()
 ****************************************************************
 * @brief
 *  vincent van ghoul dual sp encryption test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void vincent_van_ghoul_dualsp_encrypted_test(void)
{
    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    vincent_van_ghoul_dualsp_test();
}
/*************************************************
 * end vincent_van_ghoul_dualsp_encrypted_test()
 *************************************************/

/*!**************************************************************
 * vincent_van_ghoul_dualsp_encrypted_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup vincent van ghoul dual sp encryption test.
 *
 * @param None.               
 *
 * @return None.
 *
 ****************************************************************/
void vincent_van_ghoul_dualsp_encrypted_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Re-enable automatic sparing*/
    fbe_api_control_automatic_hot_spare(FBE_TRUE);

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
		fbe_test_sep_util_destroy_kms_both_sps();
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    fbe_test_sep_util_set_encryption_test_mode(FBE_TRUE);
    return;
}
/***************************************************
 * end vincent_van_ghoul_dualsp_encrypted_cleanup()
 ***************************************************/


/************************************ 
 * end file vincent_van_ghoul_test.c
 ************************************/
