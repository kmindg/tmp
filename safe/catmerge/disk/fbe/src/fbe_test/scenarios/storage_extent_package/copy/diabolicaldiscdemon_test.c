/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * diabolicdiscaldemon_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains a diabolicdiscaldemon simple proactive copy test.
 *
 * HISTORY
 *   8/26/2009:  Created. Dhaval Patel.
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
#include "fbe/fbe_api_encryption_interface.h"
#include "fbe/fbe_api_drive_configuration_service_interface.h"

char * diabolicaldiscdemon_short_desc = "This scenario will test copy operations on a random redundant raid type";
char * diabolicaldiscdemon_long_desc = 
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
#define DIABOLICDISCDEMON_TEST_LUNS_PER_RAID_GROUP                  3 

/* Element size of our LUNs.
 */
#define DIABOLICDISCDEMON_TEST_ELEMENT_SIZE                       128

/* Maximum number of block we will test with.
 */
#define DIABOLICDISCDEMON_MAX_IO_SIZE_BLOCKS                        1024

/* Number of hot-spare to configure is the max raid group width */
#define DIABOLICDISCDEMON_TEST_HS_COUNT                             12

/* Max number of raid groups we will test with.
 */
#define DIABOLICDISCDEMON_TEST_RAID_GROUP_COUNT                      1
#define DIABOLICDISCDEMON_TEST_MAX_RETRIES                         300  // retry count - number of times to loop
#define DIABOLICDISCDEMON_TEST_RETRY_TIME                         1000  // wait time in ms, in between retries 
#define DIABOLICDISCDEMON_TEST_EVENT_LOG_WAIT_TIME                 100
#define DIABOLICDISCDEMON_TEST_SWAP_WAIT_TIME_MS                 60000  // Swap-In and Swap-Out wait time is (60) seconds    
#define DIABOLICDISCDEMON_TEST_ZERO_DISK_TIME_MS                120000  // Wait for disk zeroing to complete  

/*********************************************************************
 * @def     DIABOLICDISCDEMON_TEST_CHUNKS_PER_LUN
 * 
 * @note    This needs to be fairly big so that the copy operations runs 
 *          for some time. 
 *********************************************************************/
#define DIABOLICDISCDEMON_TEST_CHUNKS_PER_LUN                       10

/*********************************************************************
 * @def     DIABOLICDISCDEMON_RAID_GROUP_DEFAULT_DEBUG_FLAGS
 * 
 * @note    Default raid group debug flags (if enabled) for virtual 
 *          drive object specified. 
 *********************************************************************/
#define DIABOLICDISCDEMON_RAID_GROUP_DEFAULT_DEBUG_FLAGS            (FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR)

/*********************************************************************
 * @def     DIABOLICDISCDEMON_RAID_LIBRARY_DEFAULT_DEBUG_FLAGS
 * 
 * @note    Default raid group library flags (if enabled) for virtual 
 *          drive object specified. 
 *********************************************************************/
#define DIABOLICDISCDEMON_RAID_LIBRARY_DEFAULT_DEBUG_FLAGS          (FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING             | \
                                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING        | \
                                                                     FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_PAGED_MD_WRITE_DATA     )

/*!*******************************************************************
 * @var     diabolicaldiscdemon_raid_group_debug_flags
 *********************************************************************
 * @brief   These are the debugs flags to add if enabled.
 *
 *********************************************************************/          /*! @note Re-enable flags if needed. */
fbe_raid_group_debug_flags_t    diabolicaldiscdemon_raid_group_debug_flags  = 0; /* DIABOLICDISCDEMON_RAID_GROUP_DEFAULT_DEBUG_FLAGS;*/
                                                                        
/*!*******************************************************************
 * @var     diabolicaldiscdemon_raid_library_debug_flags
 *********************************************************************
 * @brief   These are the debugs flags to add if enabled.
 *
 *********************************************************************/           /*! @note Re-enable flags if needed. */
fbe_raid_library_debug_flags_t  diabolicaldiscdemon_raid_library_debug_flags = 0; /* DIABOLICDISCDEMON_RAID_LIBRARY_DEFAULT_DEBUG_FLAGS; */

/* RDGEN test context. */
static fbe_api_rdgen_context_t      diabolicaldiscdemon_test_context[DIABOLICDISCDEMON_TEST_LUNS_PER_RAID_GROUP * DIABOLICDISCDEMON_TEST_RAID_GROUP_COUNT];

/*!*******************************************************************
 * @var diabolicaldiscdemon_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_lba_t diabolicaldiscdemon_user_space_lba_to_inject_to          =   0x0ULL; /* lba 0 on first LUN. Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var diabolicaldiscdemon_user_space_lba_to_inject_to
 *********************************************************************
 * @brief This is the default user space lba to start the error injection
 *        to.
 *
 *********************************************************************/
static fbe_block_count_t diabolicaldiscdemon_user_space_blocks_to_inject  = 0x7C0ULL; /* Must be aligned to 4K block size.*/

/*!*******************************************************************
 * @var diabolicaldiscdemon_default_offset
 *********************************************************************
 * @brief This is the default user space offset.
 *
 *********************************************************************/
static fbe_lba_t diabolicaldiscdemon_default_offset                     = 0x10000ULL; /* Offset used maybe different */

/*!***************************************************************************
 * @var diabolicaldiscdemon_b_is_DE542_fixed
 *****************************************************************************
 * @brief DE542 - Due to the fact that protocol error are injected on the
 *                way to the disk, the data is never sent to or read from
 *                the disk.  Therefore until this defect is fixed b_start_io
 *                MUST be True.
 *
 * @todo  Fix defect DE542.
 *
 *****************************************************************************/
static fbe_bool_t diabolicaldiscdemon_b_is_DE542_fixed                  = FBE_FALSE;

/*!***************************************************************************
 * @var diabolicaldiscdemon_b_is_AR564750_fixed
 *****************************************************************************
 * @brief AR564750 - There is a small window where (only during the `complete 
 *          copy transaction') where if the destination (after the configuration 
 *          mode is set to pass-thru and the source drive is swapped out) if the 
 *          drive is `glitched' (in this case by clearing EOL) the virtual drive 
 *          will remain in the failed state. This is because there is a job in 
 *          progress in the failed state yet the virtual drive must be in the 
 *          Ready state to set the checkpoint to the end marker.
 *
 * @todo  Fix defect AR 564750.
 *
 *****************************************************************************/
static fbe_bool_t diabolicaldiscdemon_b_is_AR564750_fixed               = FBE_FALSE;

/*!***************************************************************************
 * @var     diabolicaldiscdemon_discovered_drive_locations
 *****************************************************************************
 * @brief   For each test iteration (only (1) raid group is tested per 
 *          iteration) we discover all the disk information and save it here.
 *          There is no need to rediscover the the disk for each test.
 *
 * @todo  Fix defect DE542.
 *
 *****************************************************************************/
static fbe_test_discovered_drive_locations_t diabolicaldiscdemon_discovered_drive_locations = { 0 };

/*!*******************************************************************
 * @var diabolicdiscdemon_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t diabolicdiscdemon_raid_group_config[DIABOLICDISCDEMON_TEST_RAID_GROUP_COUNT + 1] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG,         0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TYPE_REDUNDANT,  FBE_TEST_RG_CONFIG_RANDOM_TAG,    520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var diabolicdiscdemon_raid_group_config_encryption
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t diabolicdiscdemon_raid_group_config_encryption[DIABOLICDISCDEMON_TEST_RAID_GROUP_COUNT + 1] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG,         0xE000,     FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY,  FBE_TEST_RG_CONFIG_RANDOM_TAG,    520,            0,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var diabolicdiscdemon_raid_group_config_timeout_errors
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t diabolicdiscdemon_raid_group_config_timeout_errors[3] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,         0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0},
    {5,         0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


typedef enum diabolicdiscdemon_test_case_e {    
    DIABOLICDISCDEMON_SIMPLE_PROACTIVE_COPY_FIRST = 0,
    DIABOLICDISCDEMON_SIMPLE_USER_PROACTIVE_COPY,
    DIABOLICDISCDEMON_USER_COPY,
    DIABOLICDISCDEMON_PROACTIVE_COPY_START_LONG_MARK_NR_SUCCEEDS,
    DIABOLICDISCDEMON_REMOVE_SOURCE_DRIVE_DURING_COPY,
    DIABOLICDISCDEMON_DEST_DRIVE_REMOVAL_DURING_COPY,
    DIABOLICDISCDEMON_REMOVE_SOURCE_DURING_COPY_START,
    DIABOLICDISCDEMON_PROACTIVE_COPY_AFTER_JOIN,
    DIABOLICDISCDEMON_REMOVE_SOURCE_VALIDATE_DELAY,
    DIABOLICDISCDEMON_REMOVE_SOURCE_DURING_COPY_COMPLETE,
    DIABOLICDISCDEMON_DEST_AND_SOURCE_DRIVE_REMOVAL_DURING_COPY,
    DIABOLICDISCDEMON_REMOVE_DEST_DURING_COPY_COMPLETE,
    DIABOLICDISCDEMON_EOL_SET_ON_DEST_DURING_PROACTIVE_COPY,
    DIABOLICDISCDEMON_USER_COPY_DEST_DIES_DURING_SWAP,
    DIABOLICDISCDEMON_SOURCE_DRIVE_FAULT_DURING_COPY_START,   /*! @note This MUST follow DIABOLICDISCDEMON_USER_COPY_DEST_DIES_DURING_SWAP !!*/
    DIABOLICDISCDEMON_USER_COPY_COMPLETE_SET_CHECKPOINT_TIMES_OUT,
    DIABOLICDISCDEMON_NUMBER_OF_ENABLED_TEST_CASES,

    DIABOLICDISCDEMON_FIRST_DISABLED_TEST_CASE = DIABOLICDISCDEMON_NUMBER_OF_ENABLED_TEST_CASES,   

    DIABOLICDISCDEMON_NUMBER_OF_TEST_CASES,
} diabolicdiscdemon_test_case_t;

typedef struct diabolicdiscdemon_test_information_s
{
    fbe_bool_t                      is_user_initiated_copy;
    fbe_u32_t                       drive_pos;
    diabolicdiscdemon_test_case_t   test_case;
    fbe_u32_t                       number_of_spare;
} diabolicdiscdemon_test_information_t;


diabolicdiscdemon_test_information_t diabolicdiscdemon_test_info[DIABOLICDISCDEMON_NUMBER_OF_TEST_CASES] =
{/* user initiated copy?    drive position  test case                                                   Number of spares required*/
    {FBE_FALSE,              0,             DIABOLICDISCDEMON_SIMPLE_PROACTIVE_COPY_FIRST,                  1},
    {FBE_TRUE,               1,             DIABOLICDISCDEMON_SIMPLE_USER_PROACTIVE_COPY,                   1},
    {FBE_TRUE,               0,             DIABOLICDISCDEMON_USER_COPY,                                    1},
    {FBE_FALSE,              1,             DIABOLICDISCDEMON_PROACTIVE_COPY_START_LONG_MARK_NR_SUCCEEDS,   1},
    {FBE_FALSE,              0,             DIABOLICDISCDEMON_REMOVE_SOURCE_DRIVE_DURING_COPY,              1},
    {FBE_FALSE,              0,             DIABOLICDISCDEMON_DEST_DRIVE_REMOVAL_DURING_COPY,               1},
    {FBE_FALSE,              0,             DIABOLICDISCDEMON_REMOVE_SOURCE_DURING_COPY_START,              1},
    {FBE_FALSE,              1,             DIABOLICDISCDEMON_PROACTIVE_COPY_AFTER_JOIN,                    1},
    {FBE_TRUE,               1,             DIABOLICDISCDEMON_REMOVE_SOURCE_VALIDATE_DELAY,                 1},
    {FBE_FALSE,              0,             DIABOLICDISCDEMON_REMOVE_SOURCE_DURING_COPY_COMPLETE,           1},
    {FBE_FALSE,              0,             DIABOLICDISCDEMON_DEST_AND_SOURCE_DRIVE_REMOVAL_DURING_COPY,    1},
    {FBE_TRUE,               1,             DIABOLICDISCDEMON_REMOVE_DEST_DURING_COPY_COMPLETE,             1},
    {FBE_FALSE,              0,             DIABOLICDISCDEMON_EOL_SET_ON_DEST_DURING_PROACTIVE_COPY,        1},
    {FBE_TRUE,               1,/*SameNext*/ DIABOLICDISCDEMON_USER_COPY_DEST_DIES_DURING_SWAP,              1},
    {FBE_FALSE,              1,/*SamePrev*/ DIABOLICDISCDEMON_SOURCE_DRIVE_FAULT_DURING_COPY_START,         1}, /*! @note This test cleans up for the previous test.*/
    {FBE_TRUE,               0,             DIABOLICDISCDEMON_USER_COPY_COMPLETE_SET_CHECKPOINT_TIMES_OUT,  1},
};

/*****************************
 *   LOCAL FUNCTION PROTOTYPES
 *****************************/
static void diabolicdiscdemon_io_error_trigger_proactive_sparing(fbe_test_rg_configuration_t *rg_config_p,
                                                          fbe_u32_t        drive_pos,
                                                          fbe_edge_index_t source_edge_index);

static void diabolicdiscdemon_io_error_fault_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                   fbe_u32_t        drive_pos,
                                                   fbe_edge_index_t source_edge_index);

/* Performs a proactive copy on the specified drive position in the raid group */
void diabolicaldiscdemon_test_simple_proactive_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t  drive_pos,
                                                    fbe_bool_t is_user_initiated_copy);

/* it performs test which removes destination drive (proactive spare) in the middle of the proactive copy operation. */
static void diabolicaldiscdemon_test_remove_destination_drive(fbe_test_rg_configuration_t * rg_config_p,
                                                              fbe_u32_t   drive_pos,
                                                              fbe_bool_t  is_user_initiated_copy);

/* Remove the destination drive during the copy complete job. */
static void diabolicaldiscdemon_test_remove_destination_drive_during_copy_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                                                   fbe_u32_t  drive_pos);

/*  EOL is set on the destination after proactive copy starts. */
static void diabolicaldiscdemon_test_eol_set_on_destination_during_proactive_copy(fbe_test_rg_configuration_t * rg_config_p,
                                                              fbe_u32_t   drive_pos,
                                                              fbe_bool_t  is_user_initiated_copy);

/* remove destination then source drive during proactive copy.  validate copy fails*/
static void diabolicaldiscdemon_test_remove_destination_and_source_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t   drive_pos,
                                                        fbe_bool_t  b_use_user_copy);

/* it performs test which removes source drive in the middle of the proactive copy operation. */
static void diabolicaldiscdemon_test_remove_source_drive_during_copy(fbe_test_rg_configuration_t * rg_config_p,
                                                                  fbe_u32_t    drive_pos);

/* Remove the source immediately after the user copy operation is initiated */
static void diabolicaldiscdemon_test_remove_source_drive_during_copy_start(fbe_test_rg_configuration_t *rg_config_p,
                                                                           fbe_u32_t  drive_pos);

/* Initiate the proactive copy after the join */
static void diabolicaldiscdemon_test_proactive_copy_after_join(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t  drive_pos);

/* Remove source drive and validate that we wait the `trigger spare time' before exiting the failed state*/
static void diabolicaldiscdemon_test_remove_source_drive_validate_delay(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_u32_t  drive_pos);

/* Remove the source drive at the start of the copy complete job. */
static void diabolicaldiscdemon_test_remove_source_drive_during_copy_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                                              fbe_u32_t  drive_pos);

/* Wait for the proactive spare to swap-in. */
void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t  vd_object_id,
                                                        fbe_edge_index_t spare_edge_index,
                                                        fbe_test_raid_group_disk_set_t * spare_location_p);

/* It waits for the proactive copy to complete. */
void diabolicdiscdemon_wait_for_proactive_copy_completion(fbe_object_id_t vd_object_id);

/* Wait for the copy job to complete. */
static fbe_status_t diabolicdiscdemon_wait_for_copy_job_to_complete(fbe_object_id_t vd_object_id);

/* wait for the source edge to swap-out. */
void diabolicdiscdemon_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
                                                                    fbe_edge_index_t source_edge_index);

/* wait for the destination drive to swap-out. */
static void diabolicdiscdemon_wait_for_destination_drive_swap_out(fbe_object_id_t vd_object_id,
                                                                fbe_edge_index_t dest_edge_index);

/* it gets source and destination edge index for the virtual drive object. */
void diabolicdiscdemon_get_source_destination_edge_index(fbe_object_id_t  vd_object_id,
                                                         fbe_edge_index_t * source_edge_index_p,
                                                         fbe_edge_index_t * dest_edge_index_p);

/* verify the background pattern from all the luns of the raid group. */
void diabolicdiscdemon_verify_background_pattern(fbe_api_rdgen_context_t *in_test_context_p,
                                                 fbe_u32_t in_element_size);

void diabolicaldiscdemon_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);

void diabolicaldiscdemon_run_timeout_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);

/* Wait for the previous copy test to complete */
static fbe_status_t diabolicaldiscdemon_wait_for_in_progress_request_to_complete(fbe_object_id_t vd_object_id,
                                                                                 fbe_u32_t timeout_ms);

static fbe_status_t diabolicdiscdemon_test_remove_hotspares_from_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                               fbe_u32_t num_of_spares_required);

static fbe_status_t diabolicdiscdemon_test_add_hotspares_to_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                          fbe_u32_t num_of_spares_required);
/* Mark NR times-out (takes a long time). Copy should continue */
static void diabolicaldiscdemon_test_proactive_copy_start_long_mark_nr_succeeds(fbe_test_rg_configuration_t *rg_config_p,
                                                                                fbe_u32_t  drive_pos);

/* test user copy */
static void diabolicaldiscdemon_test_user_copy(fbe_test_rg_configuration_t * rg_config_p,
                                               fbe_u32_t  drive_pos);

static void diabolicaldiscdemon_test_debug_fail_destination_during_swap(fbe_test_rg_configuration_t *rg_config_p,
                                                                        fbe_u32_t  drive_pos);

static void diabolicaldiscdemon_test_fail_destination_during_swap(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t  drive_pos);

/* Drive fault on source drive immediately after copy starts. */
static void diabolicaldiscdemon_test_source_drive_fault_during_copy_start(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_u32_t drive_pos);

static void diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_active(fbe_test_rg_configuration_t *rg_config_p,
                                                                             fbe_u32_t drive_pos);

static void diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_passive(fbe_test_rg_configuration_t *rg_config_p,
                                                                             fbe_u32_t drive_pos);

/* Set checkpoint to end marker times out during copy complete.*/
static void diabolicaldiscdemon_test_user_copy_complete_set_checkpoint_times_out(fbe_test_rg_configuration_t *rg_config_p,
                                                                                 fbe_u32_t  drive_pos);

static void diabolicaldiscdemon_validate_rg_info(fbe_object_id_t rg_id, fbe_object_id_t src_pvd, fbe_object_id_t dest_pvd);

static fbe_status_t diabolicdiscdemon_populate_io_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
                                                                         fbe_u32_t drive_pos,
                                                                         fbe_u8_t requested_scsi_command_to_trigger,
                                                                         fbe_protocol_error_injection_error_record_t *record_p,
                                                                         fbe_protocol_error_injection_record_handle_t *record_handle_p,
                                                                         fbe_scsi_sense_key_t scsi_sense_key,
                                                                         fbe_scsi_additional_sense_code_t scsi_additional_sense_code,
                                                                         fbe_scsi_additional_sense_code_qualifier_t scsi_additional_sense_code_qualifier);

static fbe_status_t diabolicdiscdemon_populate_timeout_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
                                                                              fbe_u32_t drive_pos,
                                                                              fbe_protocol_error_injection_error_record_t *record_p,
                                                                              fbe_protocol_error_injection_record_handle_t *record_handle_p);

static fbe_status_t diabolicaldiscdemon_run_io_to_start_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                             fbe_u32_t sparing_position,
                                                             fbe_edge_index_t source_edge_index,
                                                             fbe_u8_t requested_scsi_opcode_to_inject_on,
                                                             fbe_block_count_t io_size);


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void diabolicaldiscdemon_set_trace_level(fbe_trace_type_t trace_type, fbe_u32_t id, fbe_trace_level_t level)
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

void diabolicaldiscdemon_cleanup(void)
{
    fbe_test_common_cleanup_notifications(FBE_FALSE /* This is a single SP test*/);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

void diabolicaldiscdemon_encrypted_cleanup(void)
{
    fbe_test_common_cleanup_notifications(FBE_FALSE /* This is a single SP test*/);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms();
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 *          diabolicaldiscdemon_get_vd_and_rg_object_ids()
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
static fbe_status_t diabolicaldiscdemon_get_vd_and_rg_object_ids(fbe_object_id_t original_rg_object_id,
                                                                 fbe_u32_t position,
                                                                 fbe_object_id_t *vd_object_id_p,
                                                                 fbe_object_id_t *rg_object_id_p)
{
    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_raid_group_get_info_t get_info;
    fbe_u32_t  current_time = 0;

    MUT_ASSERT_TRUE(position < FBE_MAX_DOWNSTREAM_OBJECTS);

    while (current_time < DIABOLICDISCDEMON_TEST_SWAP_WAIT_TIME_MS)
    {
        status = fbe_api_raid_group_get_info(original_rg_object_id, &get_info, FBE_PACKET_FLAG_EXTERNAL);

        if (status == FBE_STATUS_OK)
        {
            break;
        }
        current_time = current_time + DIABOLICDISCDEMON_TEST_EVENT_LOG_WAIT_TIME;
        fbe_api_sleep(DIABOLICDISCDEMON_TEST_EVENT_LOG_WAIT_TIME);
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
 * end diabolicaldiscdemon_get_vd_and_rg_object_ids()
 *****************************************************/

/*!****************************************************************************
 *          diabolicaldiscdemon_remove_drive()
 ******************************************************************************
 *
 * @brief   It is used to remove the specific drive in raid group.
 *
 * @param   rg_config_p - Pointer to raid group
 * @param   position - Position in raid group
 * @param   drive_handle_p - Address of local drive handle to update
 * @param   peer_drive_handle_p - Address of peer drive handle to update
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
static void diabolicaldiscdemon_remove_drive(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t position,
                                             fbe_test_raid_group_disk_set_t *drive_location_p,
                                             fbe_api_terminator_device_handle_t *drive_handle_p,
                                             fbe_api_terminator_device_handle_t *peer_drive_handle_p,
                                             fbe_bool_t b_wait_for_pvd_fail,
                                             fbe_bool_t b_wait_for_vd_fail)
{
    fbe_status_t    status;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t vd_object_id;
    fbe_object_id_t pvd_object_id;
    fbe_bool_t      b_is_rg_member = FBE_FALSE;
    
    /* get the vd object id. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);

    /* Get the PVD object id by location. */
    status = fbe_api_provision_drive_get_obj_id_by_location(drive_location_p->bus,
                                                            drive_location_p->enclosure,
                                                            drive_location_p->slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
  
    /* Prnt the action */
    mut_printf(MUT_LOG_TEST_STATUS, "%s:remove the drive port: %d encl: %d slot: %d",
                __FUNCTION__,
               drive_location_p->bus,        
               drive_location_p->enclosure,
               drive_location_p->slot);

    /* If this drive is a member of the raid group then we must use the
     * raid group configuration array.
     */
    if ((drive_location_p->bus == rg_config_p->rg_disk_set[position].bus)             &&
        (drive_location_p->enclosure == rg_config_p->rg_disk_set[position].enclosure) &&
        (drive_location_p->slot == rg_config_p->rg_disk_set[position].slot)              )
    {
        b_is_rg_member = FBE_TRUE;
    }

    /* Remove the drive
     */
    if (fbe_test_util_is_simulation())
    {
        if (b_is_rg_member)
        {
            fbe_test_sep_util_add_removed_position(rg_config_p, position); 
        }
        /* remove the drive. */
        status = fbe_test_pp_util_pull_drive(drive_location_p->bus,         
                                             drive_location_p->enclosure,   
                                             drive_location_p->slot, 
                                             drive_handle_p);       
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
            status = fbe_test_pp_util_pull_drive(drive_location_p->bus,           
                                                 drive_location_p->enclosure,     
                                                 drive_location_p->slot,          
                                                 peer_drive_handle_p);                 
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /*  Set the target server back to the local SP. */
            fbe_api_sim_transport_set_target_server(local_sp);
        }
    }
    else
    {
        /* remove the drive on actual hardware. */
        status = fbe_test_pp_util_pull_drive_hw(drive_location_p->bus,        
                                                drive_location_p->enclosure,  
                                                drive_location_p->slot);      
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
 * end diabolicaldiscdemon_remove_drive()
 ******************************************************************************/

/*!****************************************************************************
 *          diabolicaldiscdemon_reinsert_drive()
 ******************************************************************************
 *
 * @brief   It is used to re-insert the specific drive in raid group.
 *
 * @param   rg_config_p - Pointer to raid group
 * @param   removed_position - Position in raid group (if it was part of a riad group)
 * @param   reinsert_disk_p - Location of drive to re-insert
 * @param   drive_handle - The local drive handle of the removed drive
 * @param   peer_drive_handle - The peer drive handle of the removed drive
 * @param   b_wait_for_pvd_ready - Wait for the provision drive to goto the 
 *              Ready state before returning.
 * @param   b_wait_for_vd_ready - Wait for the virtual drive to goto the 
 *              Ready state before returning.
 *
 * @return None.
 *
 * @author
 *  07/31/2015  Ron Proulx  - Created
 ******************************************************************************/
static void diabolicaldiscdemon_reinsert_drive(fbe_test_rg_configuration_t *rg_config_p,
                                               fbe_u32_t removed_position,
                                               fbe_test_raid_group_disk_set_t *reinsert_disk_p,
                                               fbe_api_terminator_device_handle_t drive_handle,
                                               fbe_api_terminator_device_handle_t peer_drive_handle, 
                                               fbe_bool_t b_wait_for_pvd_ready,
                                               fbe_bool_t b_wait_for_vd_ready)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   removed_index;
    fbe_object_id_t             vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                  b_is_rg_member = FBE_FALSE;
    fbe_u32_t                   wait_attempts = 0;
    fbe_api_base_config_downstream_object_list_t downstream_object_list; 

    mut_printf(MUT_LOG_TEST_STATUS, "%s: re-insert removed pos: %d disk %d_%d_%d",
               __FUNCTION__, removed_position,
               reinsert_disk_p->bus, reinsert_disk_p->enclosure, reinsert_disk_p->slot);

    /* First checked if the drive is removed or not.
     */
    for (removed_index = 0; removed_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; removed_index++)
    {
        if (rg_config_p->drives_removed_array[removed_index] == removed_position)
        {
            b_is_rg_member = FBE_TRUE;
            break;
        }
    }

    /* Process the removed drive.
     */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, removed_position, &vd_object_id);

    /* If this drive was a member of the raid group we must update the
     * confiiguration information.
     */
    if (b_is_rg_member)
    {
        fbe_test_sep_util_removed_position_inserted(rg_config_p, removed_position);
    }

    /* inserting the same drive back */
    if (fbe_test_util_is_simulation())
    {
        status = fbe_test_pp_util_reinsert_drive(reinsert_disk_p->bus, 
                                                 reinsert_disk_p->enclosure, 
                                                 reinsert_disk_p->slot,
                                                 drive_handle);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /*! @todo This only works if Active is SPA !!*/
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            status = fbe_test_pp_util_reinsert_drive(reinsert_disk_p->bus, 
                                                     reinsert_disk_p->enclosure, 
                                                     reinsert_disk_p->slot,
                                                     peer_drive_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
    }
    else
    {
        status = fbe_test_pp_util_reinsert_drive_hw(reinsert_disk_p->bus, 
                                                    reinsert_disk_p->enclosure, 
                                                    reinsert_disk_p->slot);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            /* Set the target server to SPB and insert the drive there. */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
            status = fbe_test_pp_util_reinsert_drive_hw(reinsert_disk_p->bus, 
                                                        reinsert_disk_p->enclosure, 
                                                        reinsert_disk_p->slot);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Set the target server back to SPA. */
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
    }

    /* If requested wait for PVD to become Ready
     */
    if (b_wait_for_pvd_ready)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for rg %d position %d (%d_%d_%d) to become ready ==", 
                       __FUNCTION__, rg_config_p->raid_group_id, removed_position, 
                       reinsert_disk_p->bus, reinsert_disk_p->enclosure, reinsert_disk_p->slot);
        while (((status != FBE_STATUS_OK)                ||
                (pvd_object_id == FBE_OBJECT_ID_INVALID)    ) &&
                (wait_attempts < 20)                              )
        {
            /* Wait until we can get pvd object id.
             */
            status = fbe_api_provision_drive_get_obj_id_by_location(reinsert_disk_p->bus,
                                                                    reinsert_disk_p->enclosure,
                                                                    reinsert_disk_p->slot,
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

        status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for %d_%d_%d to become ready - complete ==", 
               __FUNCTION__,
               reinsert_disk_p->bus, reinsert_disk_p->enclosure, reinsert_disk_p->slot);
    } /* end if b_wait_for_pvd_ready */

    /* If request wait for vd Ready*/
    if (b_wait_for_vd_ready)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for rg %d position %d vd obj: 0x%x to become ready ==", 
                   __FUNCTION__, rg_config_p->raid_group_id, removed_position, vd_object_id);
        status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for vd obj: 0x%xto become ready - complete ==", 
               __FUNCTION__, vd_object_id);
    }

    return;
}
/******************************************************************************
 * end diabolicaldiscdemon_reinsert_drive()
 ******************************************************************************/

/*!****************************************************************************
 *          diabolicaldiscdemon_set_unused_drive_info()
 ******************************************************************************
 *
 * @brief   This function is used to set the unused drive info in the raid group
 *          structure.
 *
 * @param   rg_config_p - Pointer to raid group test configuration to populate
 * @param   unused_drive_count - Array of unused drive counters
 * @param   drive_locations_p - Pointer to unused drive locations
 * @param   num_of_spares_required - How many spares must be discovered
 *
 * @return  None
 *
 * @author
 *  10/19/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_set_unused_drive_info(fbe_test_rg_configuration_t *rg_config_p,
                                                      fbe_u32_t unused_drive_count[][FBE_DRIVE_TYPE_LAST],
                                                      fbe_test_discovered_drive_locations_t *drive_locations_p,
                                                      fbe_u32_t num_of_spares_required)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       no_of_entries = FBE_RAID_MAX_DISK_ARRAY_WIDTH;
    fbe_test_block_size_t           block_size;
    fbe_test_raid_group_disk_set_t *disk_set_p = NULL;
    fbe_drive_type_t                drive_type = rg_config_p->drive_type;
    fbe_u32_t                       unused_drive_cnt = 0;
    fbe_u32_t                       unused_drive_index = 0;
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

        /*Initialise the array with zero values
         */
        fbe_zero_memory(rg_config_p->unused_disk_set,
                        no_of_entries * sizeof(fbe_test_raid_group_disk_set_t));

        /* Get the pointer of the first available drive
         */
        disk_set_p = &drive_locations_p->disk_set[block_size][drive_type][0];

        /* For all available drives.
         */
        for (unused_drive_index = 0; unused_drive_index < unused_drive_cnt; unused_drive_index++)
        {
            /* Validate that the drive is a healthy spare.
             */
            b_is_healthy_spare = fbe_test_sep_is_spare_drive_healthy(disk_set_p);
            MUT_ASSERT_TRUE(b_is_healthy_spare == FBE_TRUE);

            /* If this is not the `skip' drive, add it to the unused array.
             */
            rg_config_p->unused_disk_set[unused_drive_index] = *disk_set_p;

            /* Goto next drive
             */
            disk_set_p++;
        }
    }

    /* Validate that we found sufficient spare drives.
     */
    MUT_ASSERT_TRUE(unused_drive_cnt >= num_of_spares_required);
    return;
}
/************************************************** 
 * end diabolicaldiscdemon_set_unused_drive_info()
 **************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_enable_debug_flags_for_virtual_drive_object()
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
static void diabolicaldiscdemon_enable_debug_flags_for_virtual_drive_object(fbe_object_id_t vd_object_id,
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
                                                          (raid_group_debug_flags | diabolicaldiscdemon_raid_group_debug_flags));
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If the raid group flags for the associated virtual drive are to be
     * enabled, enable then.
     */
    if (b_enable_raid_library_flags)
    {
        fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", (fbe_u32_t *)&raid_library_debug_flags);
        status = fbe_api_raid_group_set_library_debug_flags(vd_object_id, 
                                                            (raid_library_debug_flags | diabolicaldiscdemon_raid_library_debug_flags));

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
               (b_enable_raid_group_flags) ? (raid_group_debug_flags | diabolicaldiscdemon_raid_group_debug_flags) : raid_group_debug_flags,
               (b_enable_raid_library_flags) ? (raid_library_debug_flags | diabolicaldiscdemon_raid_library_debug_flags) : b_enable_raid_library_flags);
   
    return;
}
/***********************************************************************
 * end diabolicaldiscdemon_enable_debug_flags_for_virtual_drive_object()
 ***********************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object()
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
static void diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object(fbe_object_id_t vd_object_id,
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
 * end diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object()
 ************************************************************************/

/*!****************************************************************************
 * diabolicaldiscdemon_test_config_hotspares()
 ******************************************************************************
 * @brief
 *  This test is used to init the hs config
 *  .
 * @param   current_rg_config_p - Pointer to raid group test configuration for this 
 *              iteration.
 * @param   num_of_spares_required - How many spares must be discovered
 * @param   num_spares_available_p - Address of the number of spares we found
 *
 * @return FBE_STATUS.
 *
 * @author
 *  10/19/2012  Ron Proulx  - Updated.
 ******************************************************************************/
static fbe_status_t diabolicaldiscdemon_test_config_hotspares(fbe_test_rg_configuration_t *current_rg_config_p,  
                                                              fbe_u32_t num_of_spares_required,
                                                              fbe_u32_t *num_spares_available_p)               
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_test_rg_configuration_t    *rg_config_p = &diabolicdiscdemon_raid_group_config[0];
    fbe_test_hs_configuration_t     hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t                       unused_drive_index;
    fbe_u32_t                       hs_index = 0;
    fbe_test_discovered_drive_locations_t unused_pvd_locations;
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
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, DIABOLICDISCDEMON_TEST_RAID_GROUP_COUNT);

    /* Discover all the `unused drives'
     */
    *num_spares_available_p = 0;
    fbe_test_sep_util_discover_all_drive_information(&diabolicaldiscdemon_discovered_drive_locations);
    fbe_test_sep_util_get_all_unused_pvd_location(&diabolicaldiscdemon_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    diabolicaldiscdemon_set_unused_drive_info(current_rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
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
            *num_spares_available_p = unused_drive_index;
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
 * end diabolicaldiscdemon_test_config_hotspares()
 *************************************************/

/*!***************************************************************************
 * diabolicaldiscdemon_clear_eol()
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
static void diabolicaldiscdemon_clear_eol(fbe_test_raid_group_disk_set_t *source_disk_p)
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
 * end diabolicaldiscdemon_clear_eol()
 **************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_zero_swapped_out_drive()
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
static fbe_status_t diabolicaldiscdemon_zero_swapped_out_drive(fbe_object_id_t pvd_object_id)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       retry_count = 0;

    /* Wait for the drive to be ready.
     */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          pvd_object_id,
                                                                          FBE_LIFECYCLE_STATE_READY, 
                                                                          DIABOLICDISCDEMON_TEST_SWAP_WAIT_TIME_MS, 
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

    fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, DIABOLICDISCDEMON_TEST_ZERO_DISK_TIME_MS);
    mut_printf(MUT_LOG_TEST_STATUS, "== Zeroing of pvd obj: 0x%x complete==",  pvd_object_id);

    return status;
}
/**************************************************
 * end diabolicaldiscdemon_zero_swapped_out_drive()
 **************************************************/

/*!***************************************************************************
 *          diabolicdiscdemon_insert_hs_drive()
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
static void diabolicdiscdemon_insert_hs_drive(fbe_test_raid_group_disk_set_t *drive_to_insert_p,
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
    status = diabolicaldiscdemon_zero_swapped_out_drive(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/*****************************************
 * end diabolicdiscdemon_insert_hs_drive()
 *****************************************/

/*!***************************************************************************
 * diabolicaldiscdemon_cleanup_reinsert_source_drive()
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
static fbe_status_t diabolicaldiscdemon_cleanup_reinsert_source_drive(fbe_test_rg_configuration_t *rg_config_p, 
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
            diabolicaldiscdemon_clear_eol(drive_to_insert_p);
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
    status = diabolicaldiscdemon_zero_swapped_out_drive(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return FBE_STATUS_OK;
}
/*****************************************************************************
 * end diabolicaldiscdemon_cleanup_reinsert_source_drive()
 *****************************************************************************/

/*!****************************************************************************
 * diabolicaldiscdemon_test()
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
void diabolicaldiscdemon_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &diabolicdiscdemon_raid_group_config[0];

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,diabolicaldiscdemon_run_tests,
                                   DIABOLICDISCDEMON_TEST_LUNS_PER_RAID_GROUP,
                                   DIABOLICDISCDEMON_TEST_CHUNKS_PER_LUN);
    return;    
}
/********************************
 * end diabolicaldiscdemon_test()
 ********************************/


/*!****************************************************************************
 * diabolicaldiscdemon_encryption_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups with encryption.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 ******************************************************************************/
void diabolicaldiscdemon_encryption_test(void)
{
    diabolicaldiscdemon_test();
    return;    
}
/********************************
 * end diabolicaldiscdemon_encryption_test()
 ********************************/

/*!****************************************************************************
 * diabolicaldiscdemon_dualsp_test()
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
void diabolicaldiscdemon_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &diabolicdiscdemon_raid_group_config[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_run_test_on_rg_config(rg_config_p,"Dummy",diabolicaldiscdemon_run_tests,
                                   DIABOLICDISCDEMON_TEST_LUNS_PER_RAID_GROUP,
                                   DIABOLICDISCDEMON_TEST_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end diabolicaldiscdemon_dualsp_test()
 ******************************************************************************/
void diabolicaldiscdemon_encryption_dualsp_test(void)
{
    diabolicaldiscdemon_dualsp_test();
}

static void diabolicaldiscdemon_set_num_of_extra_drives(fbe_test_rg_configuration_t *rg_config_p)
{        
    fbe_u32_t num_extra_drives = 0;

    fbe_test_sep_util_get_rg_type_num_extra_drives(rg_config_p->raid_type,
                                                   rg_config_p->width,
                                                   &num_extra_drives);
    num_extra_drives = FBE_RAID_MAX_DISK_ARRAY_WIDTH - num_extra_drives;
    fbe_test_rg_populate_extra_drives(rg_config_p, num_extra_drives);
    return;
}

/*!****************************************************************************
 * diabolicaldiscdemon_timeout_errors_dualsp_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing timeout test across different RAID groups.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *   10/25/2015:  Created. Deanna Heng.
 ******************************************************************************/
void diabolicaldiscdemon_timeout_errors_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &diabolicdiscdemon_raid_group_config_timeout_errors[0];

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    fbe_test_run_test_on_rg_config(rg_config_p,"Dummy",diabolicaldiscdemon_run_timeout_tests,
                                   DIABOLICDISCDEMON_TEST_LUNS_PER_RAID_GROUP,
                                   DIABOLICDISCDEMON_TEST_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    
    return;    

}
/******************************************************************************
 * end diabolicaldiscdemon_timeout_errors_dualsp_test()
 ******************************************************************************/


/*!****************************************************************************
 * diabolicaldiscdemon_setup()
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

void diabolicaldiscdemon_setup(void)
{
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t   num_raid_groups =0;
        
        /* Randomize which raid type to use.
         */
        rg_config_p = &diabolicdiscdemon_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
         */
        diabolicaldiscdemon_set_num_of_extra_drives(rg_config_p);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
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
 * end diabolicaldiscdemon_setup()
 *********************************/
/*!****************************************************************************
 * diabolicaldiscdemon_encrypted_setup()
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
void diabolicaldiscdemon_encrypted_setup(void)
{
    fbe_status_t status;

    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        /* Randomize which raid type to use.
         */
        rg_config_p = &diabolicdiscdemon_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
         */
        diabolicaldiscdemon_set_num_of_extra_drives(rg_config_p);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();

        //fbe_api_enable_system_encryption();
        sep_config_load_kms(NULL);

        status = fbe_test_sep_util_enable_kms_encryption();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* After sep is loaded setup notifications.
         */
        fbe_test_common_setup_notifications(FBE_FALSE /* This is a single SP test*/);

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
    }
    
    return;
}
/********************************* 
 * end diabolicaldiscdemon_encrypted_setup()
 *********************************/

/*!**************************************************************
 * diabolicaldiscdemon_dualsp_setup()
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
void diabolicaldiscdemon_dualsp_setup(void)
{
    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;

        fbe_test_rg_configuration_t *rg_config_p = NULL;
        
        /* Randomize which raid type to use.
         */
        rg_config_p = &diabolicdiscdemon_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        diabolicaldiscdemon_set_num_of_extra_drives(rg_config_p);

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

        /* After sep is load setup notifications */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dualsp test */);

    }
    return;
}
/******************************************
 * end diabolicaldiscdemon_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * diabolicaldiscdemon_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the diabolicaldiscdemon test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *     8/26/2009:  Created. Dhaval Patel.
 *
 ****************************************************************/
void diabolicaldiscdemon_dualsp_cleanup(void)
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
 * end diabolicaldiscdemon_dualsp_cleanup()
 ******************************************/

/*!**************************************************************
 *          diabolicaldiscdemon_run_timeout_tests()
 ****************************************************************
 * @brief
 *  Run timeout error tests against 2 raid groups. These tests
 *  rely on the fact that the raid group 0 is active on sp A
 *  and raid group 1 is active on sp B
 *
 * @param   rg_config_p - Pointer to array of raid groups under
 *              test. 
 *
 * @return None.
 *
 * @author
 *  10/25/2015  Deanna Heng    - Created
 *
 ****************************************************************/
void diabolicaldiscdemon_run_timeout_tests(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   rg_index;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_u32_t                   num_spares_required = 1;
    fbe_u32_t                   num_spares_available = 0;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Diabolicdiscdemon timeout errors test started!!.\n\n", __FUNCTION__);

    /* Set the trace level as information. */
    diabolicaldiscdemon_set_trace_level(FBE_TRACE_TYPE_DEFAULT, FBE_OBJECT_ID_INVALID, FBE_TRACE_LEVEL_INFO);

    /* We are injecting timeout errors during the copy. It is possible the vd
     * will see errors during the paged metadata verify in copy
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    /* Speed up VD hot spare */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Determine the */
    /* write background pattern to all the luns before we start copy operation. */
    diabolicdiscdemon_write_background_pattern(&diabolicaldiscdemon_test_context[0]);

    /* For all raid groups under test.
     */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* Always configure the hot-spares.*/
            status =  diabolicaldiscdemon_test_config_hotspares(current_rg_config_p, num_spares_required, &num_spares_available);

                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                mut_printf(MUT_LOG_TEST_STATUS, "Copy timeout test case (%d) started on rg type: %d!!!", 
                           rg_index, current_rg_config_p->raid_type);
                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
    
                if (rg_index == 0)
                {
                    /* Drive timeout errors on source drive immediately after copy starts. 
                     * This test case validates that when timeout errors get set during rebuild logging
                     * appropriate actions are taken so that the raid group does not get stuck in mark nr
                     * when the source drives eventually faults
                     */
                    diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_active(current_rg_config_p, 0);
                }

                if (rg_index == 1)
                {
                    /* Drive timeout errors on source drive immediately after copy starts.
                     * This test case validates that when timeout errors get set during rebuild logging
                     * appropriate actions are taken so that the raid group does not get stuck in clear rebuild logging
                     */
                    diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_passive(current_rg_config_p, 1);
                }

                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                mut_printf(MUT_LOG_TEST_STATUS, "Copy timeout test case (%d) completed successfully!!!", rg_index);
                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");

        } /* end if this raid group is enabled for test*/

        status  = fbe_api_protocol_error_injection_remove_all_records();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups under test*/

    /* Set the sector trace stop on error off.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_FALSE);

    fbe_test_sep_util_unconfigure_all_spares_in_the_system();

    /* Set the trigger spare timer to the default value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:diabolicdiscdemon timeout errors test completed successfully!!.\n", __FUNCTION__);
    return;
}
/****************************************** 
 * end diabolicaldiscdemon_run_timeout_tests()
 ******************************************/

/*!**************************************************************
 *          diabolicaldiscdemon_run_tests()
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
void diabolicaldiscdemon_run_tests(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   test_index;
    fbe_u32_t                   rg_index;
    fbe_u32_t                   hs_index = 0;
    fbe_u32_t                   num_raid_groups = 0;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   num_spares_required = 0;
    fbe_u32_t                   num_spares_available = 0;
    fbe_bool_t                  b_is_dualsp = FBE_FALSE;
    fbe_u32_t                   current_test_case;
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Diabolicdiscdemon test started!!.\n\n", __FUNCTION__);

    b_is_dualsp = fbe_test_sep_util_get_dualsp_test_mode();

    /* Set the trace level as information. */
    diabolicaldiscdemon_set_trace_level(FBE_TRACE_TYPE_DEFAULT, FBE_OBJECT_ID_INVALID, FBE_TRACE_LEVEL_INFO);

    /* Since these test do not generate corrupt crc etc, we don't expect
     * ANY errors.  So stop the system if any sector errors occur.
     */
    fbe_test_sep_util_set_sector_trace_stop_on_error(FBE_TRUE);

    /* Speed up VD hot spare */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Determine the */
    /* write background pattern to all the luns before we start copy operation. */
    diabolicdiscdemon_write_background_pattern(&diabolicaldiscdemon_test_context[0]);

    /* For all raid groups under test.
     */
    for (rg_index = 0; rg_index < num_raid_groups; rg_index++)
    {
        num_spares_required = 0;
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            /* Always configure the hot-spares.*/
            num_spares_required = diabolicdiscdemon_test_info[DIABOLICDISCDEMON_SIMPLE_PROACTIVE_COPY_FIRST].number_of_spare;
            status =  diabolicaldiscdemon_test_config_hotspares(current_rg_config_p, num_spares_required, &num_spares_available);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            hs_index = 0;

            /* Run all the tests. */
            current_test_case = 1;
            for (test_index = DIABOLICDISCDEMON_SIMPLE_PROACTIVE_COPY_FIRST; test_index < DIABOLICDISCDEMON_NUMBER_OF_ENABLED_TEST_CASES; test_index++, current_test_case <<= 1) 
            {
                /* Run the test if enabled */
                if ((current_rg_config_p->test_case_bitmask != 0 ) &&
                     !(current_rg_config_p->test_case_bitmask & current_test_case))
                {
                    continue;
                }

                /* If the test position is greater than the number pf positions
                 * skip the test.
                 */
                if (diabolicdiscdemon_test_info[test_index].drive_pos >= current_rg_config_p->width)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                    mut_printf(MUT_LOG_TEST_STATUS, "Copy test case (%d) rg type: %d drive_pos: %d not valid for width: %d skip.", 
                               test_index, current_rg_config_p->raid_type, diabolicdiscdemon_test_info[test_index].drive_pos, current_rg_config_p->width);
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                }
                else
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                    mut_printf(MUT_LOG_TEST_STATUS, "Copy test case (%d) started on rg type: %d!!!", 
                               test_index, current_rg_config_p->raid_type);
                    mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
    
                    switch(diabolicdiscdemon_test_info[test_index].test_case)
                    {
                        case DIABOLICDISCDEMON_SIMPLE_PROACTIVE_COPY_FIRST:
                        case DIABOLICDISCDEMON_SIMPLE_USER_PROACTIVE_COPY:
                            /* it tests simple proactive copy operation. */
                            diabolicaldiscdemon_test_simple_proactive_copy(current_rg_config_p,
                                                                           diabolicdiscdemon_test_info[test_index].drive_pos,
                                                                           diabolicdiscdemon_test_info[test_index].is_user_initiated_copy);
                            break;
                        case DIABOLICDISCDEMON_PROACTIVE_COPY_START_LONG_MARK_NR_SUCCEEDS:
                            /* Proactive copy starts and mark NR takes a long time.  The copy job
                             * should proceed and NOT wait for the marking of NR in the paged. */
                            diabolicaldiscdemon_test_proactive_copy_start_long_mark_nr_succeeds(current_rg_config_p,
                                                                                                diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_DEST_DRIVE_REMOVAL_DURING_COPY:
                            /* it tests destination drive removal during proactive copy operation. */
                            diabolicaldiscdemon_test_remove_destination_drive(current_rg_config_p,
                                                                              diabolicdiscdemon_test_info[test_index].drive_pos,
                                                                              diabolicdiscdemon_test_info[test_index].is_user_initiated_copy);
                            break;
                        case DIABOLICDISCDEMON_REMOVE_DEST_DURING_COPY_COMPLETE:
                            /* Remove the destination drive immediately after the copy complete job starts. */
                            diabolicaldiscdemon_test_remove_destination_drive_during_copy_complete(current_rg_config_p,
                                                                              diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_EOL_SET_ON_DEST_DURING_PROACTIVE_COPY:
                            /* EOL is set on the destination after proactive copy starts. */
                            diabolicaldiscdemon_test_eol_set_on_destination_during_proactive_copy(current_rg_config_p,
                                                                              diabolicdiscdemon_test_info[test_index].drive_pos,
                                                                              diabolicdiscdemon_test_info[test_index].is_user_initiated_copy);
                            break;
                        case DIABOLICDISCDEMON_DEST_AND_SOURCE_DRIVE_REMOVAL_DURING_COPY:
                            /* Test destination and source removal during proactive copy operation. */
                            diabolicaldiscdemon_test_remove_destination_and_source_drives(current_rg_config_p,
                                                                                        diabolicdiscdemon_test_info[test_index].drive_pos,
                                                                                        diabolicdiscdemon_test_info[test_index].is_user_initiated_copy);
                            break;
                        case DIABOLICDISCDEMON_REMOVE_SOURCE_DRIVE_DURING_COPY:
                            /* Remove the source drive with the `trigger spare 
                             * timer' set to a small value.
                             */
                            diabolicaldiscdemon_test_remove_source_drive_during_copy(current_rg_config_p,
                                                                             diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_REMOVE_SOURCE_DURING_COPY_START:
                            /* Validates that removing source drive during the `copy start' 
                             * job does not affect the job execution.
                             */
                            diabolicaldiscdemon_test_remove_source_drive_during_copy_start(current_rg_config_p,
                                                                             diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_PROACTIVE_COPY_AFTER_JOIN:
                            /* Validates that if proactive copy is initiated immediately after join
                             * (which can result in clear rl being invoked after the destination edge is swapped
                             *  in but BEFORE we set the mode to mirror) we will not clear rl until mark NR
                             *  is complete)
                             */
                            diabolicaldiscdemon_test_proactive_copy_after_join(current_rg_config_p,
                                                                               diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_REMOVE_SOURCE_VALIDATE_DELAY:
                            /* Validates that we don't exist the failed state 
                             * until the `spare trigger delay' has occurred.
                             */
                            diabolicaldiscdemon_test_remove_source_drive_validate_delay(current_rg_config_p,
                                                                             diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;   
                        case DIABOLICDISCDEMON_REMOVE_SOURCE_DURING_COPY_COMPLETE:
                            /* Validates that removing source drive during the `copy complete' 
                             * job does not affect the job execution.
                             */
                            diabolicaldiscdemon_test_remove_source_drive_during_copy_complete(current_rg_config_p,
                                                                             diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_USER_COPY:
                            /* it tests user copy.*/ 
                            diabolicaldiscdemon_test_user_copy(current_rg_config_p,
                                                               diabolicdiscdemon_test_info[test_index].drive_pos);
                            
                            break; 
                        case DIABOLICDISCDEMON_USER_COPY_DEST_DIES_DURING_SWAP:
                            /* Destination is removed immediately after copy starts.*/
                            diabolicaldiscdemon_test_fail_destination_during_swap(current_rg_config_p,
                                                                                  diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_SOURCE_DRIVE_FAULT_DURING_COPY_START:
                            /* Drive fault on source drive immediately after copy starts. */
                            diabolicaldiscdemon_test_source_drive_fault_during_copy_start(current_rg_config_p,                                
                                                                                    diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;
                        case DIABOLICDISCDEMON_USER_COPY_COMPLETE_SET_CHECKPOINT_TIMES_OUT:
                            /* User copy completes by the non-paged write of checkpoints times-out. Job is rolled-back.*/
                            diabolicaldiscdemon_test_user_copy_complete_set_checkpoint_times_out(current_rg_config_p,
                                                                                                 diabolicdiscdemon_test_info[test_index].drive_pos);
                            break;

                    } /* end switch test case index */

                } /* end else drive position for test is valid */

                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");
                mut_printf(MUT_LOG_TEST_STATUS, "Copy test case (%d) completed successfully!!!", test_index);
                mut_printf(MUT_LOG_TEST_STATUS, "########################################################");

                /* increment the hot spare index with number of spares for the particular test. */
                hs_index += diabolicdiscdemon_test_info[test_index].number_of_spare;
                if (hs_index >= num_spares_available) {
                    /* Configure hot-spares*/
                    status =  diabolicaldiscdemon_test_config_hotspares(current_rg_config_p, num_spares_required, &num_spares_available);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    hs_index = 0;
                }
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

    mut_printf(MUT_LOG_TEST_STATUS, "%s:diabolicdiscdemon test completed successfully!!.\n", __FUNCTION__);
    return;
}
/****************************************** 
 * end diabolicaldiscdemon_run_tests()
 ******************************************/

/*!***************************************************************************
 *          diabolicdiscdemon_check_event_log_msg()
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
void diabolicdiscdemon_check_event_log_msg(fbe_u32_t message_code,
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
    max_retries = DIABOLICDISCDEMON_TEST_MAX_RETRIES;

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
        fbe_api_sleep(DIABOLICDISCDEMON_TEST_EVENT_LOG_WAIT_TIME);
    }
    
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, b_is_msg_present, "Event log msg is not present!");
 
    return;
}
/*********************************************
 * end diabolicdiscdemon_check_event_log_msg()
 **********************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_set_rebuild_hook()
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
static fbe_lba_t diabolicaldiscdemon_set_rebuild_hook(fbe_object_id_t vd_object_id, fbe_u32_t percent_copied_before_pause)
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
 * end diabolicaldiscdemon_set_rebuild_hook()
 ***********************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_wait_for_rebuild_hook()
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
static void diabolicaldiscdemon_wait_for_rebuild_hook(fbe_object_id_t vd_object_id, fbe_lba_t checkpoint_to_pause_at)
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
 * end diabolicaldiscdemon_wait_for_rebuild_hook()
 *************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_remove_rebuild_hook()
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
static void diabolicaldiscdemon_remove_rebuild_hook(fbe_object_id_t vd_object_id, fbe_lba_t checkpoint_to_pause_at)
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
 * end diabolicaldiscdemon_remove_rebuild_hook()
 *************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_get_valid_destination_drive()
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
static fbe_status_t diabolicaldiscdemon_get_valid_destination_drive(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_test_raid_group_disk_set_t *dest_disk_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               hs_index = 0;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_u32_t                               local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
  
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Discover all the `unused drives'
     */
    fbe_test_sep_util_get_all_unused_pvd_location(&diabolicaldiscdemon_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    diabolicaldiscdemon_set_unused_drive_info(rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
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
 * end diabolicaldiscdemon_get_valid_destination_drive()
 ********************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_simple_proactive_copy()
 ***************************************************************************** 
 * 
 * @brief   Test `simple' proactive copy request.
 *
 * @param   rg_config_p - Pointer to single raid group under test
 * @param   drive_pos - Raid group position to start copy on
 * @param   is_user_initiated_copy - FBE_TRUE Issue user copy instead of
 *              proactive copy.
 *
 * @return  none
 *
 * @author
 *  08/09/2009  Dhaval Patel    - Created.
 *
 *****************************************************************************/
void diabolicaldiscdemon_test_simple_proactive_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                    fbe_u32_t  drive_pos,
                                                    fbe_bool_t is_user_initiated_copy)
{
    fbe_status_t                            status;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         vd_object_id;
    fbe_edge_index_t                        source_edge_index;
    fbe_edge_index_t                        dest_edge_index;
    fbe_test_raid_group_disk_set_t          source_drive_location;
    fbe_test_raid_group_disk_set_t          spare_drive_location;
    fbe_object_id_t							spare_pvd_object_id;
    fbe_api_rdgen_context_t                *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_object_id_t                         pvd_object_id;
    fbe_provision_drive_copy_to_status_t    copy_status;
    fbe_notification_info_t                 notification_info;
    fbe_database_pvd_info_t					pvd_info;

    mut_printf(MUT_LOG_TEST_STATUS, "Simple proactive copy test started!!!\n");

    source_drive_location = rg_config_p->rg_disk_set[drive_pos];
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on object 0x%x with bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, pvd_object_id, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    if(is_user_initiated_copy)
    {
        /* user initiate copy to. */
        status = fbe_api_provision_drive_user_copy(pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        /* if not user initiated copy, we trigger the proactive copy operation using error injection. */
        diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);
    }

    /* Set a debug hook after we switch to mirror mode but before we start to 
     * rebuild the paged metadata.
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* Wait for rebuild logging to be clear for the destination. */
    status = sep_rebuild_utils_wait_for_rb_logging_clear_for_position(vd_object_id, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that NR is set on the destination drive*/
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

    /* Validate that no bits are set in the parent raid group*/
    sep_rebuild_utils_check_bits(rg_object_id);

    /* Validate in the rg info we can see the destination drive being part of RG now*/
    spare_pvd_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_location.bus,
                                                            spare_drive_location.enclosure,
                                                            spare_drive_location.slot,
                                                            &spare_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, spare_pvd_object_id);

    diabolicaldiscdemon_validate_rg_info(rg_object_id, pvd_object_id, spare_pvd_object_id);
    status = fbe_api_database_get_pvd(spare_pvd_object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL_MSG(0, pvd_info.rg_count, "simple_proactive_copy: dest drive reports upstream RG, not expected");
    

    /* Remove the rebuild start hook. */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* stop the job service recovery queue to hold the job service swap command. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);
    
    /* wait for the proactive copy completion. */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* Verify event log message for system initiated proactive copy operation  */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_COPY_OPERATION_COMPLETED,
                                          FBE_TRUE, /* Message contains spare drive location */
                                          &spare_drive_location,
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]);

    
    /* Wait for notification that swap-out is successful*/
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                          FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                          FBE_NOTIFICATION_TYPE_SWAP_INFO,
                                                          FBE_STATUS_OK,
                                                          FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* start the job service recovery queue to process the swap-out command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* Wait for the notification. */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note There is no job status associated with a swap notification.
     *        But the command in the swap notification must be valid.
     */
    MUT_ASSERT_INT_EQUAL(FBE_SPARE_COMPLETE_COPY_COMMAND, notification_info.notification_data.swap_info.command_type);
    MUT_ASSERT_INT_EQUAL(vd_object_id, notification_info.notification_data.swap_info.vd_object_id);

    /* wait for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);
        
    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &spare_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]);                  

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &source_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Simple proactive copy test completed successfully!!!\n");
    return;
}
/******************************************************
 * end diabolicaldiscdemon_test_simple_proactive_copy()
 ******************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_remove_destination_drive()
 ***************************************************************************** 
 * 
 * @brief   Remove the destination drive while a proactive copy is in progress.
 *
 * @param   rg_config_p - Pointer to single raid group under test
 * @param   drive_pos - Raid group position to start copy on
 * @param   is_user_initiated_copy - FBE_TRUE Issue user copy instead of
 *              proactive copy.
 *
 * @return  none
 *
 * @author
 *  08/09/2009  Dhaval Patel    - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_remove_destination_drive(fbe_test_rg_configuration_t * rg_config_p,
                                                        fbe_u32_t   drive_pos,
                                                        fbe_bool_t  is_user_initiated_copy)
{
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_status_t                    status;
    fbe_test_raid_group_disk_set_t  removed_drive_location;
    fbe_test_raid_group_disk_set_t  source_drive_location;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_api_terminator_device_handle_t removed_drive_handle;
    fbe_lba_t                       hook_lba = 0;

    
    mut_printf(MUT_LOG_TEST_STATUS, "Destination drive removal during proactive copy test started!!!\n");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    source_drive_location = rg_config_p->rg_disk_set[drive_pos];

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &removed_drive_location);

    /*! @note Move all the hot-spares to the automatic sparing pool.
     *
     */
    status = diabolicdiscdemon_test_remove_hotspares_from_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable the automatic spare selection so that a spare isn't automatically swapped in */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for copy to start before removing the destination drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /* Add and debug hook when the virutal drive initiates pass-thru*/
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add and debug hook when the virtual drive aborts the copy operation.
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the hook .*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* remove the spare drive. */
    fbe_test_sep_drive_pull_drive(&removed_drive_location, &removed_drive_handle);

    /* wait for intiatation of swap-out */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the hook .*/
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for swap-out complete */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* stop the job service recovery queue to hold the job service swap commands. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* remove the hook .*/
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @todo Move all the automatic spares back to the hot-spare pool.
     *
     *  @todo A better solution is to add a switch hook to stop the swap-in 
     *        process.
     */
    status = diabolicdiscdemon_test_add_hotspares_to_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* enable the recovery queue to allow the swap-in to proceed */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* stop the job service recovery queue to hold the job service swap commands. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);
    
    /* wait for the proactive copy completion. */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* restart the job service queue to proceed with job service swap command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* wait for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* wait for the proactive copy operation to complete. */
    diabolicdiscdemon_wait_for_copy_job_to_complete(vd_object_id);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Re-insert the removed drive */
    diabolicdiscdemon_insert_hs_drive(&removed_drive_location, removed_drive_handle);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup the source drive.
     */
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &source_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Proactive spare removal during proactive copy test completed successfully!!!\n");
    return;
}
/***************************************************************
 * end diabolicaldiscdemon_test_remove_destination_drive()
 ***************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_remove_destination_drive_during_copy_complete()
 *****************************************************************************
 *
 * @brief   This test removes the destination drive during a `copy complete' operation.
 *          It removes the destination drive immediately after the `change configuration
 *          mode to pass-thru' database transaction is started.  The expectation
 *          is that removing the destination disk after the copy is completed
 *          should not prevent the copy complete job from completing successfully.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @note    Although the virtual drive is in the failed state it does not
 *          prevent the copy from completing.  This is due to the fact that
 *          to `complete' the copy we access the non-paged drives not the
 *          source or destination drives.
 *
 * @author
 *  06/25/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_remove_destination_drive_during_copy_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                                                   fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 source_pvd_object_id;
    fbe_object_id_t                 dest_pvd_object_id;
    fbe_test_raid_group_disk_set_t  dest_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_lba_t                       hook_lba = FBE_LBA_INVALID;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_api_terminator_device_handle_t removed_drive_handle;
    fbe_base_config_encryption_mode_t encryption_mode;
    fbe_bool_t                      b_is_encryption_enabled = FBE_FALSE;
    fbe_bool_t                      b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_u32_t                       source_pvd_bus;       
    fbe_u32_t                       source_pvd_enclosure;  
    fbe_u32_t                       source_pvd_slot;
    fbe_sim_transport_connection_target_t active_sp = fbe_api_sim_transport_get_target_server();
    fbe_sim_transport_connection_target_t source_pvd_active_sp;
    fbe_sim_transport_connection_target_t source_pvd_passive_sp;

    mut_printf(MUT_LOG_TEST_STATUS, "Remove destination drive during copy complete job!!!\n");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start user copy operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the source drive information.
     */
    source_pvd_bus = rg_config_p->rg_disk_set[drive_pos].bus;  
    source_pvd_enclosure = rg_config_p->rg_disk_set[drive_pos].enclosure;
    source_pvd_slot = rg_config_p->rg_disk_set[drive_pos].slot;
    status = fbe_api_provision_drive_get_obj_id_by_location(source_pvd_bus,
                                                            source_pvd_enclosure,
                                                            source_pvd_slot,
                                                            &source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
    /* Get the active SP for the original source drive.
     */
    status = fbe_test_sep_util_get_active_passive_sp(source_pvd_object_id,
                                                     &source_pvd_active_sp,
                                                     &source_pvd_passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If encryption is enabled on the source, set the scrub hooks.
     */
    status = fbe_api_base_config_get_encryption_mode(source_pvd_object_id, &encryption_mode);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting encryption mode failed\n");
    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)      ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)                 ) {
        b_is_encryption_enabled = FBE_TRUE;
    }

    /* Get a valid destination drive.
     */
    status = diabolicaldiscdemon_get_valid_destination_drive(rg_config_p, &dest_drive_location);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                            dest_drive_location.enclosure,
                                                            dest_drive_location.slot,
                                                            &dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Add debug hook at 5% rebuild hook
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* Start user copy */
    status = fbe_api_copy_to_replacement_disk(source_pvd_object_id, dest_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &dest_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /* Add a debug hook when the request to change to mirror is received
     * but before we have switched to mirror mode. 
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure that the job completes.  I.E. No rollback.
     */
    status = fbe_test_add_job_service_hook(vd_object_id,                   
                                           FBE_JOB_TYPE_DRIVE_SWAP,                    
                                           FBE_JOB_ACTION_STATE_COMMIT,           
                                           FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START, 
                                           FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the hook .*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* Wait for the copy complete change config mode hook
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for copy complete start hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove destination drive.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s remove destination drive during copy complete vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    fbe_test_sep_drive_pull_drive(&dest_drive_location, &removed_drive_handle);

    /* Sleep for a short period
     */
    fbe_api_sleep(100);

    /* If encryption is enabled stop when the swap-out is complete.
     */
    if (b_is_encryption_enabled) {
        /* Add a debug hook when the copy complete request has finished.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                                0, 0,  
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

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

    /* Remove the copy complete change configuration mode debug hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the source drive to swap-out (and the configuration mode to be
     * changed to pass-thru).
     */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Wait for the virtual drive to goto the failed state.
     */
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);

    /* If encryption is enabled wait for re-zeroing (with new keys) of the
     * swapped out source drive to start.
     */
    if (b_is_encryption_enabled) {
        /* Wait for the copy complete operation to complete.
         */
        status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                     SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                     FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now remove the swap complete hook.
         */
        status = fbe_test_del_debug_hook_active(vd_object_id, 
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Validate that the source drive is re-zeroed after being swapped out.
         */
        status = fbe_test_wait_for_debug_hook_active(source_pvd_object_id,
                                                     SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                                     FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START,  
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_LOG,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now remove the re-zero hook.
         */
        status = fbe_test_del_debug_hook_active(source_pvd_object_id, 
                                                SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                                FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START, 
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now wait for the zeroing to start.
         */
        if ((b_is_dualsp_test)                  &&
            (source_pvd_active_sp != active_sp)    ) {
            status = fbe_api_sim_transport_set_target_server(source_pvd_active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        status = fbe_test_sep_drive_check_for_disk_zeroing_event(source_pvd_object_id, 
                                                                 SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED,  
                                                                 source_pvd_bus, source_pvd_enclosure, source_pvd_slot); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if ((b_is_dualsp_test)                  &&
            (source_pvd_active_sp != active_sp)    ) {
            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

    } /* end if encryption is enabled check and clear additional hooks*/

    /* Validate that the job completed successfully.
     */
    status = fbe_test_wait_for_job_service_hook(vd_object_id,                   
                                                FBE_JOB_TYPE_DRIVE_SWAP,                    
                                                FBE_JOB_ACTION_STATE_COMMIT,           
                                                FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START, 
                                                FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,                
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &dest_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]); 

    /* Remove the commit validation hook
     */
    status = fbe_test_del_job_service_hook(vd_object_id,                   
                                           FBE_JOB_TYPE_DRIVE_SWAP,                    
                                           FBE_JOB_ACTION_STATE_COMMIT,           
                                           FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START, 
                                           FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now wait for the virtual drive to become Ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Re-insert the removed drive */
    diabolicdiscdemon_insert_hs_drive(&dest_drive_location, removed_drive_handle);

    mut_printf(MUT_LOG_TEST_STATUS, "Remove destination drive during copy complete job - completed!!!\n");
    return;
}
/*******************************************************************************
 * end diabolicaldiscdemon_test_remove_destination_drive_during_copy_complete()
 *******************************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_eol_set_on_destination_during_proactive_copy()
 ***************************************************************************** 
 * 
 * @brief   Set EOL AFTER (the sparing rules will not allow a destination drive
 *          with EOL set to be used) the proactive copy starts.  There are (2)
 *          test cases:
 *              1. Set EOL on the destination after 5% has been copied.  Validate
 *                 that the copy is aborted and that the destination drive is
 *                 swapped out.  We swap out the destination because the source
 *                 drive is still present and contain all valid data.
 *
 *              2. Set EOL on the destination after the copy complete job has
 *                 started.  In this case the destination drive is fully rebuilt
 *                 and is (most likely) healthier than the source drive so 
 *                 expect the source drive to be swapped out.
 *
 * @param   rg_config_p - Pointer to single raid group under test
 * @param   drive_pos - Raid group position to start copy on
 * @param   is_user_initiated_copy - FBE_TRUE Issue user copy instead of
 *              proactive copy.
 *
 * @return  none
 *
 * @author
 *  06/05/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_eol_set_on_destination_during_proactive_copy(fbe_test_rg_configuration_t * rg_config_p,
                                                        fbe_u32_t   drive_pos,
                                                        fbe_bool_t  is_user_initiated_copy)
{
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_status_t                    status;
    fbe_test_raid_group_disk_set_t  dest_drive_location;
    fbe_test_raid_group_disk_set_t  source_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_object_id_t                 dest_object_id;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_lba_t                       hook_lba = 0;
    fbe_api_provision_drive_info_t  pvd_info;
    fbe_api_virtual_drive_get_info_t vd_info;
    fbe_u32_t                       retry_count;
    
    mut_printf(MUT_LOG_TEST_STATUS, "Set EOL on destination after proactive copy starts - started!!!\n");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    source_drive_location = rg_config_p->rg_disk_set[drive_pos];

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Change the spare library `confirmation timeout' to a smaller value.
     * (Should be at least (10) seconds so that the virtual drive monitor can
     * run).
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(15);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &dest_drive_location);

    /*! @note Move all the hot-spares to the automatic sparing pool.
     *
     */
    status = diabolicdiscdemon_test_remove_hotspares_from_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable the automatic spare selection so that a spare isn't automatically swapped in */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for copy to start before removing the destination drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /* Test 1: Add debug hooks when the virtual drive issues the request to
     *         abort the copy operation (due to the destination being marked
     *         EOL) and when the swap out is complete.
     */
    /* Add and debug hook when the virtual drive initiates pass-thru*/
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add and debug hook when the virtual drive aborts the copy operation.
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the rebuild hook .*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* Set EOL on the destination drive. */
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                            dest_drive_location.enclosure,
                                                            dest_drive_location.slot,
                                                            &dest_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_set_eol_state(dest_object_id, FBE_PACKET_FLAG_NO_ATTRIB);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(dest_object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    retry_count = 0;
    while ((pvd_info.end_of_life_state == FBE_FALSE) &&
           (retry_count < 200)                                     )
    {
        /* Wait a short time.
         */
        fbe_api_sleep(100);
        status = fbe_api_provision_drive_get_info(dest_object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        retry_count++;
    }
    MUT_ASSERT_TRUE(pvd_info.end_of_life_state == FBE_TRUE);
    retry_count = 0;

    /* wait for intiatation of swap-out */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Confirm that the destination drive will be swapped out.*/
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(vd_info.b_request_in_progress == FBE_FALSE);    /* We haven't started the `abort copy' job yet. */
    MUT_ASSERT_TRUE(vd_info.b_is_copy_complete == FBE_FALSE);
    MUT_ASSERT_TRUE(vd_info.spare_pvd_object_id == dest_object_id);
    MUT_ASSERT_TRUE(vd_info.swap_edge_index == dest_edge_index);

    /* remove the hook .*/
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for swap-out complete */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* stop the job service recovery queue to hold the job service swap commands. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* remove the hook .*/
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear EOL on the destination drive
     */
    diabolicaldiscdemon_clear_eol(&dest_drive_location);

    /*! @todo Move all the automatic spares back to the hot-spare pool.
     *
     *  @todo A better solution is to add a switch hook to stop the swap-in 
     *        process.
     */
    status = diabolicdiscdemon_test_add_hotspares_to_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Test 2: Add debug hook when all the data is copied and when we complete
     *         the swap out.
     */
    /* Add and debug hook when the copy completes. */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Add and debug hook when the virtual drive completes the swap-out
     * (We expect the source drive to be swapped out)
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* enable the recovery queue to allow the swap-in to proceed */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &dest_drive_location);

    /* stop the job service recovery queue to hold the job service swap commands. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* wait for the proactive copy completion. */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* restart the job service queue to proceed with job service swap command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* Wait for the copy to complete */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                          SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                          FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE,
                                          SCHEDULER_CHECK_STATES, 
                                          SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the destination object id */
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                            dest_drive_location.enclosure,
                                                            dest_drive_location.slot,
                                                            &dest_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*! @note There is a known issue where we cannot `complete' the copy
     *        operation if the destination drive is `glitched' (by clearing EOL).
     *        The outcome is the virtual drive is stuck in the failed state
     *        (until the job times-out) and then the job tries to rollback
     *        (which is worse).
     */
    if (diabolicaldiscdemon_b_is_AR564750_fixed == FBE_TRUE)
    {
        status = fbe_api_provision_drive_set_eol_state(dest_object_id, FBE_PACKET_FLAG_NO_ATTRIB);  
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_provision_drive_get_info(dest_object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        retry_count = 0;
        while ((pvd_info.end_of_life_state == FBE_FALSE) &&
               (retry_count < 200)                                     )
        {
            /* Wait a short time.
             */
            fbe_api_sleep(100);
            status = fbe_api_provision_drive_get_info(dest_object_id, &pvd_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            retry_count++;
        }
        MUT_ASSERT_TRUE(pvd_info.end_of_life_state == FBE_TRUE);
        retry_count = 0;
    } /* end if diabolicaldiscdemon_b_is_AR564750_fixed */

    /* remove the copy complete hook */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the swap out to start */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                          SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                          FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                          SCHEDULER_CHECK_STATES, 
                                          SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Confirm that the source drive will be swapped out.*/
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(vd_info.b_request_in_progress == FBE_TRUE);
    MUT_ASSERT_TRUE(vd_info.b_is_copy_complete == FBE_TRUE);
    MUT_ASSERT_TRUE(vd_info.original_pvd_object_id == dest_object_id);
    MUT_ASSERT_TRUE(vd_info.swap_edge_index == source_edge_index);

    /* Clear EOL on the destination drive
     */
    if (diabolicaldiscdemon_b_is_AR564750_fixed == FBE_TRUE)
    {
        diabolicaldiscdemon_clear_eol(&dest_drive_location);
    } /* end if diabolicaldiscdemon_b_is_AR564750_fixed */

    /* remove the swap out debug hook */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Wait for job to timeout
     */
    if (diabolicaldiscdemon_b_is_AR564750_fixed == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Set EOL on destination after proactive copy complete wait (2) minutes for job to fail!!!\n");
        fbe_api_sleep((60 * 2) * 1000);

        //mut_printf(MUT_LOG_TEST_STATUS, "Set EOL on destination after proactive copy complete wait (5) minutes to connect to debugger!!!\n");
        //fbe_api_sleep((60 * 5) * 1000);
    }

    /* wait for the proactive copy operation to complete. */
    diabolicdiscdemon_wait_for_copy_job_to_complete(vd_object_id);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &source_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Restore the spare library `confirmation timeout' to the default value.
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Set EOL on destination after proactive copy starts - completed successfully!!!\n");
    return;
}
/*****************************************************************************
 * end diabolicaldiscdemon_test_eol_set_on_destination_during_proactive_copy()
 *****************************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_remove_destination_and_source_drives()
 *****************************************************************************
 *
 * @brief   Add (1) drive to hot-spare pool, remove destination then source
 *          drives.  Validate virtual drive exists mirror mode and copy
 *          generates `failed' notification and event.  Validate raid group
 *          attempts rebuild but gets `no spares'.  Add spare to spare pool.
 *          Validate spare swaps-in and rebuilds.
 *
 * @param   rg_config_p - Pointer to raid group under test
 * @param   drive_pos - Raid group position to use for proactive copy
 * @param   b_use_user_copy - FBE_TRUE - Use User Copy To instead of Proactive
 *              copy.
 *
 * @return  None
 *
 * @author
 *  09/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_remove_destination_and_source_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_u32_t   drive_pos,
                                                        fbe_bool_t  b_use_user_copy)
{
    fbe_status_t                    status;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 source_pvd_object_id;
    fbe_test_raid_group_disk_set_t  source_drive_location;
    fbe_test_raid_group_disk_set_t  removed_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_api_terminator_device_handle_t removed_drive_handle;
    fbe_api_terminator_device_handle_t source_drive_handle;
    fbe_notification_info_t         notification_info;
    fbe_lba_t                       hook_lba;
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s start on raid type: %d", __FUNCTION__, rg_config_p->raid_type);

    source_drive_location = rg_config_p->rg_disk_set[drive_pos];
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_obj_id_by_location(source_drive_location.bus,
                                                            source_drive_location.enclosure,
                                                            source_drive_location.slot,
                                                            &source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &removed_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /* Change the `trigger spare time' to a large value so that we don't 
     * immediately swap-in a replacement drive for the failed destination drive.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(5);

    /* Wait for notification that swap-out is successful*/
    status = fbe_test_common_set_notification_to_wait_for(vd_object_id,
                                                          FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                          FBE_NOTIFICATION_TYPE_SWAP_INFO,
                                                          FBE_STATUS_OK,
                                                          FBE_JOB_SERVICE_ERROR_NO_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* disable the automatic spare selection so that a spare isn't automatically swapped in */
    status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* remove the hook .*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* remove the destination drive. */
    fbe_test_sep_drive_pull_drive(&removed_drive_location, &removed_drive_handle);

    /* Immediately pull the source drive*/
    fbe_test_sep_drive_pull_drive(&source_drive_location, &source_drive_handle);

    /* Wait for the notification. */
    status = fbe_test_common_wait_for_notification(__FUNCTION__, __LINE__,
                                                   30000, 
                                                   &notification_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note There is no job status associated with a swap notification.
     *        But since we have removed both the destination and source we
     *        expect the last copy command to be `abort copy'.
     */
    MUT_ASSERT_INT_EQUAL(FBE_SPARE_ABORT_COPY_COMMAND, notification_info.notification_data.swap_info.command_type);
    MUT_ASSERT_INT_EQUAL(vd_object_id, notification_info.notification_data.swap_info.vd_object_id);

    /* wait for the source drive swap-out after drive removal. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* validate raid group is degraded */
    mut_printf(MUT_LOG_TEST_STATUS, "== Validate raid groups are degraded ==");
    fbe_test_sep_rebuild_validate_rg_is_degraded(rg_config_p, 1,
                                                 drive_pos);
    mut_printf(MUT_LOG_TEST_STATUS, "== Validate raid groups are degraded - successful ==");

    /*! @todo Move all the automatic spares back to the hot-spare pool.
     *
     *  @todo A better solution is to add a switch hook to stop the swap-in 
     *        process.
     */
    status = diabolicdiscdemon_test_add_hotspares_to_hotspare_pool(rg_config_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for rebuild logging to be clear for the destination position.
     * (i.e. the replacement drive should be inserted into the destination position)
     */
    fbe_test_sep_rebuild_utils_wait_for_rebuild_logging_clear_for_position(vd_object_id, dest_edge_index);

    /* Wait for rebuild logging to be clear in the parent raid group.
     */
    sep_rebuild_utils_verify_not_rb_logging(rg_config_p, drive_pos);

    /* Validate that the destination position is not marked NR */
    mut_printf(MUT_LOG_TEST_STATUS, "== Validate NR cleared on permanent spare ==");
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);
    mut_printf(MUT_LOG_TEST_STATUS, "== Validate NR cleared on permanent spare - successful ==");

    /* Wait for the rebuilds to finish for ALL possible degraded positions
     * Since we never `removed' using big_bird_remove drives, we must set the
     * removed history.
     */
    rg_config_p->drives_removed_history[0] = drive_pos;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, 1, 1);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);

    /* Re-insert the removed drives.*/
    diabolicdiscdemon_insert_hs_drive(&removed_drive_location, removed_drive_handle);
    fbe_test_sep_drive_reinsert_drive(&source_drive_location, source_drive_handle);
    status = fbe_test_sep_util_clear_disk_eol(source_drive_location.bus, source_drive_location.enclosure, source_drive_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = diabolicaldiscdemon_zero_swapped_out_drive(source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Set `trigger spare timer' back to small value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s - Success ===", __FUNCTION__);
    return;
}
/*******************************************************************
 * end diabolicaldiscdemon_test_remove_destination_and_source_drives()
 *******************************************************************/


/*!***************************************************************************
 *          diabolicaldiscdemon_test_remove_source_drive_during_copy()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a proactive copy operation.
 *          It validates that the virtual drive goes to the failed state and
 *          that the source drive is swapped out.  Then it attempts a user
 *          initiated copy and expects it to fail because the source drive is
 *          degraded.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  10/12/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_remove_source_drive_during_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 original_source_pvd_object_id;
    fbe_object_id_t                 original_dest_pvd_object_id;
    fbe_object_id_t                 user_copy_source_pvd_object_id;
    fbe_object_id_t                 user_copy_dest_pvd_object_id;
    fbe_test_raid_group_disk_set_t  original_drive_location;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_test_raid_group_disk_set_t  dest_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_lba_t                       hook_lba = FBE_LBA_INVALID;
    fbe_provision_drive_copy_to_status_t copy_status;

    mut_printf(MUT_LOG_TEST_STATUS, "Source drive removal during proactive copy test started!!!\n");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &original_source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /* Get the destination object id.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_location.bus,
                                                            spare_drive_location.enclosure,
                                                            spare_drive_location.slot,
                                                            &original_dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* stop the job service recovery queue to hold the job service swap command. */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* remove the hook .*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* remove the source drive. */
    original_drive_location = rg_config_p->rg_disk_set[drive_pos];
    diabolicaldiscdemon_remove_drive(rg_config_p, drive_pos, &original_drive_location,
                                     &rg_config_p->drive_handle[drive_pos],
                                     &rg_config_p->peer_drive_handle[drive_pos],
                                     FBE_TRUE,  /* Wait for pvd to Fail */
                                     FBE_TRUE   /* Wait for vd to Fail */ );

    /* restart the job service recovery queue to hold the job service swap command. */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* wait for the source drive to swap-out. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Force a proactive copy immediately after the swap-out is complete.
     * We expect the copy to fail with either:
     *  o "raid group degraded"
     *      OR
     *  o "source drive degraded"
     */
    user_copy_source_pvd_object_id = original_dest_pvd_object_id;
    status = diabolicaldiscdemon_get_valid_destination_drive(rg_config_p, &dest_drive_location);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus, 
                                                            dest_drive_location.enclosure, 
                                                            dest_drive_location.slot,
                                                            &user_copy_dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* generate a message */

    mut_printf(MUT_LOG_TEST_STATUS, "%s - Attempt Copy To from: %d_%d_%d (0x%x) to %d_%d_%d(0x%x) should fail.",
               __FUNCTION__,  
               spare_drive_location.bus, 
               spare_drive_location.enclosure, 
               spare_drive_location.slot, 
               user_copy_source_pvd_object_id,
               dest_drive_location.bus,
               dest_drive_location.enclosure,
               dest_drive_location.slot,
               user_copy_dest_pvd_object_id);

    /* trigger the copy back operation  */
    status = fbe_api_copy_to_replacement_disk(user_copy_source_pvd_object_id, user_copy_dest_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /*! @note We try the copy request again.  There are (3) possible outcomes:
     *
     *      1. The copy request is received while the virtual drive is still in
     *         the failed state.  The validation status returned is:
     *              o FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN
     *
     *      2. The copy request is received before the parent raid group has set
     *         the virtual drive checkpoint to the end marker.  Since the virtual
     *         drive validation is performed prior to the parent raid group
     *         validation the status returned is:
     *              o FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY
     *
     *      3. Else the parent has set the virtual drive checkpoint to the end
     *         marker but the copy position is still degraded.  The status
     *         returned is:
     *              o FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED
     */
    switch(copy_status)
    {
        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN:
            /* The copy request is received while the virtual drive is still in
             * the failed state.  The validation status returned is:
             * o FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN
             */
            MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_VIRTUAL_DRIVE_BROKEN);
            mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);
            diabolicdiscdemon_check_event_log_msg(SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY,                  
                                                  FBE_FALSE, /* Message does not contain spare drive location */   
                                                  &dest_drive_location,                                  
                                                  FBE_TRUE, /* Message contains original drive location */
                                                  &spare_drive_location);                  
    
            mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the source drive is broken. event: 0x%x",
                       SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY);
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY:
            /* The copy request is received before the parent raid group has set
             * the virtual drive checkpoint to the end marker.  Since the virtual
             * drive validation is performed prior to the parent raid group
             * validation the status returned is:
             *              o FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY
             */
            MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY);
            mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);
            diabolicdiscdemon_check_event_log_msg(SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED,                  
                                                  FBE_FALSE, /* Message does not contain spare drive location */   
                                                  &dest_drive_location,                                  
                                                  FBE_TRUE, /* Message contains original drive location */
                                                  &spare_drive_location);                  
    
            mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the source drive is degraded. event: 0x%x",
                       SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY);
            break;

        case FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED:
            /* Else the parent has set the virtual drive checkpoint to the end
             * marker but the copy position is still degraded.  The status
             * returned is:
             * o FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED
             */
            MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED);
            mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);
            diabolicdiscdemon_check_event_log_msg(SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED,                  
                                              FBE_FALSE, /* Message does not contain spare drive location */   
                                              &dest_drive_location,                                  
                                              FBE_TRUE, /* Message contains original drive location */
                                              &spare_drive_location);                  
    
            mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the parent raid group is degraded. event: 0x%x",
                       SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED);
            break;

        default:
            /* Else the status is unexpected.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "User copy completed with unexpected copy_status: %d", copy_status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return;
    }

    /* Now wait for the virtual drive to become ready and then try it again.
     */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object ready failed");

    /* trigger the copy back operation  */
    status = fbe_api_copy_to_replacement_disk(user_copy_source_pvd_object_id, user_copy_dest_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);

    /*! @note We have waited for the virtual drive to exit the failed state and 
     *        enter the Ready state.  We try the copy request again.  There are
     *        (2) possible outcomes:
     *      1. The copy request is received before the parent raid group has set
     *         the virtual drive checkpoint to the end marker.  Since the virtual
     *         drive validation is performed prior to the parent raid group
     *         validation the status returned is:
     *              o FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY
     *
     *      2. Else the parent has set the virtual drive checkpoint to the end
     *         marker but the copy position is still degraded.  The status
     *         returned is:
     *              o FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED
     */
    if (copy_status == FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY)
    {
        /* Case 1. above: The copy request is received before the parent raid 
         *         group has set the virtual drive checkpoint to the end marker.
         */
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_SOURCE_BUSY);
        mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);

        diabolicdiscdemon_check_event_log_msg(SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED,                  
                                              FBE_FALSE, /* Message does not contain spare drive location */   
                                              &dest_drive_location,                                  
                                              FBE_TRUE, /* Message contains original drive location */
                                              &spare_drive_location);                  

        mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the source drive is degraded. event: 0x%x",
                   SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED);

    }
    else
    {
        /* Else Case 2. above: The parent has set the virtual drive checkpoint 
         *              to the end marker but the copy position is still degraded.
         */
        MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_RAID_GROUP_IS_DEGRADED);
        mut_printf(MUT_LOG_TEST_STATUS, "User copy with expected copy_status: %d completed.", copy_status);

        diabolicdiscdemon_check_event_log_msg(SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED,                  
                                          FBE_FALSE, /* Message does not contain spare drive location */   
                                          &dest_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &spare_drive_location);                  

        mut_printf(MUT_LOG_TEST_STATUS, "We validated failure since the parent raid group is degraded. event: 0x%x",
                   SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED);
    }

    /* Wait for rebuild to start */
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_pos);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &original_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Source drive removal during proactive copy test - completed!!!\n");
    return;
}
/*************************************************************
 * end diabolicaldiscdemon_test_remove_source_drive_during_copy()
 *************************************************************/

/*!***************************************************************************
 * diabolicaldiscdemon_test_debug_fail_destination_during_swap()
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
static void diabolicaldiscdemon_test_debug_fail_destination_during_swap(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 original_source_pvd_object_id;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
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
    diabolicaldiscdemon_enable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                    FBE_TRUE,   /* Enable the raid group debug flags */
                                                                    FBE_TRUE    /* Enable the raid library debug flags */);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
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
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

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
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
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
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /*! @note Since EOL is set on the source drive a second proactive copy
     *        operation will automatically be initiated.
     */

    /* Cleanup removed, EOL drives*/
    spare_drive_location = rg_config_p->rg_disk_set[drive_pos];
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &spare_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    diabolicdiscdemon_insert_hs_drive(&removed_drive_location, removed_drive_handle);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Now remove the rebuild hook and wait for the copy to complete
     */
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* wait for the proactive copy completion. 
     */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Clear raid group and raid library debug flags for this test.
     */
    diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                     FBE_TRUE,   /* Disable the raid group debug flags */
                                                                     FBE_TRUE,   /* Disable the raid library debug flags */
                                                                     FBE_TRUE   /* Clear the debug flags */);

    mut_printf(MUT_LOG_TEST_STATUS, "Destination drive removal during proactive copy swap test - completed!!!\n");
    return;
}
/******************************************************************
 * end diabolicaldiscdemon_test_debug_fail_destination_during_swap()
 ******************************************************************/

/*!***************************************************************************
 * diabolicaldiscdemon_test_fail_destination_during_swap()
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
static void diabolicaldiscdemon_test_fail_destination_during_swap(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 original_source_pvd_object_id;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_test_raid_group_disk_set_t  removed_drive_location;
    fbe_api_terminator_device_handle_t removed_drive_handle;
    fbe_api_get_block_edge_info_t   block_edge_info;

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

    /* Enable only the raid group debug flags.
     */
    diabolicaldiscdemon_enable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                    FBE_TRUE,   /* Enable the raid group debug flags */
                                                                    FBE_TRUE    /* Enable raid library debug flags */);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
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
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

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

    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Re-insert the removed drive */
    diabolicdiscdemon_insert_hs_drive(&removed_drive_location, removed_drive_handle);

    /*! @note Do not clear the debug flags until the next test completes.
     */
    /*
    diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                     FBE_TRUE,
                                                                     FBE_TRUE,
                                                                     FBE_TRUE);
    */

    mut_printf(MUT_LOG_TEST_STATUS, "Destination drive removal during proactive copy swap test - completed!!!\n");
    return;
}
/*************************************************************
 * end diabolicaldiscdemon_test_fail_destination_during_swap()
 *************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_remove_source_drive_during_copy_start()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a `copy start' operation.
 *          It removes the source drive immediately after the `Database Create
 *          edge' database transaction is started.  The expectation
 *          is that removing the source disk after the copy is started
 *          should not prevent the copy start job from completing successfully.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  06/21/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_remove_source_drive_during_copy_start(fbe_test_rg_configuration_t *rg_config_p,
                                                                           fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 source_object_id;
    fbe_test_raid_group_disk_set_t  source_drive_location;
    fbe_test_raid_group_disk_set_t  dest_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];

    mut_printf(MUT_LOG_TEST_STATUS, "Remove source drive during copy start job!!!\n");

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &source_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, 
               rg_config_p->rg_disk_set[drive_pos].bus, 
               rg_config_p->rg_disk_set[drive_pos].enclosure, 
               rg_config_p->rg_disk_set[drive_pos].slot);

    /* Add a debug hook when the user copy job swaps-in the destination edge.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* Wait for the copy start to swap in the destination drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for copy start swap-in hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the source drive and wait for it to goto fail.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s remove source drive during copy start vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    source_drive_location = rg_config_p->rg_disk_set[drive_pos];
    diabolicaldiscdemon_remove_drive(rg_config_p, drive_pos, &source_drive_location,
                                     &rg_config_p->drive_handle[drive_pos],
                                     &rg_config_p->peer_drive_handle[drive_pos],
                                     FBE_TRUE,  /* Wait for pvd to Fail */
                                     FBE_TRUE   /* Wait for vd to Fail */ );

    /* Remove the swap-in debug hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the source drive to swap-out (and the configuration mode to be
     * changed to pass-thru).
     */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,                
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &dest_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]); 
                     
    /* Now wait for the virtual drive to become Ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &source_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Remove source drive during copy start job - completed!!!\n");
    return;
}
/*************************************************************************
 * end diabolicaldiscdemon_test_remove_source_drive_during_copy_start()
 *************************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_proactive_copy_after_join()
 *****************************************************************************
 *
 * @brief   Validates that if proactive copy is initiated immediately after join
 *          (which can result in clear rl being invoked after the destination edge 
 *           is swapped in but BEFORE we set the mode to mirror) we will not clear 
 *           rl until mark NR is complete !
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  07/31/2015  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_proactive_copy_after_join(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t  drive_pos)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id;
    fbe_object_id_t                     vd_object_id;
    fbe_object_id_t                     source_object_id;
    fbe_test_raid_group_disk_set_t      source_drive_location;
    fbe_test_raid_group_disk_set_t      dest_drive_location;
    fbe_edge_index_t                    source_edge_index;
    fbe_edge_index_t                    dest_edge_index;
    fbe_api_rdgen_context_t            *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_api_get_block_edge_info_t       dest_edge_info;
    fbe_object_id_t                     dest_pvd_object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "Initiate proactive copy immediately after join !!!\n");

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    source_drive_location = rg_config_p->rg_disk_set[drive_pos];
    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &source_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on %d_%d_%d !!",
               __FUNCTION__, 
               rg_config_p->rg_disk_set[drive_pos].bus, 
               rg_config_p->rg_disk_set[drive_pos].enclosure, 
               rg_config_p->rg_disk_set[drive_pos].slot);

    /* Add a debug hook when the copy job swaps-in the destination edge.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note Do NOT add a hook at FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE since it
     *        runs in the raid group rotary AND it occurs PRIOR to clear rl etc.!!
     */

    /* Set a debug hook after we switch to mirror mode but before we start to 
     * rebuild the paged metadata.
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START, 
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* Wait for the copy start to swap in the destination drive BEFORE removing
     * the source drive...Otherwise the copy operation will fail
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for swap-in hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add a debug hook when the clear rl condition runs in the Ready rotary.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED, 
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);

    /* Simulate clear rl after join
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s set clear rl after join vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_sep_raid_group_set_debug_state(vd_object_id,
                                                     0x0,           /* No need to set FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST */
                                                     0x0,           /* Don't set any clustered flags */
                                                     7              /* Set FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING */);

    /* Wait for the clear rl condition in the Ready rotary
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for clear rl hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                                 FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the clear rl hook
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s remove clear rl hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the swap-in debug hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the copy start to swap in the destination drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for copy start hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for rebuild logging to be clear for the destination. */
    status = sep_rebuild_utils_wait_for_rb_logging_clear_for_position(vd_object_id, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that NR is set on the destination drive*/
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

    /* Remove the copy debug hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the source drive to swap-out (and the configuration mode to be
     * changed to pass-thru).
     */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,                
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &dest_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]); 
                     
    /* Now wait for the virtual drive to become Ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* Display the PVD paged metadata for the destination drive */
    status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &dest_edge_info, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    dest_pvd_object_id = dest_edge_info.server_id;
    fbe_test_provision_drive_get_paged_metadata_summary(dest_pvd_object_id, FBE_TRUE);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &source_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Initiate proactive copy after join - completed!!!\n");
    return;
}
/*************************************************************************
 * end diabolicaldiscdemon_test_proactive_copy_after_join()
 *************************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_remove_source_drive_validate_delay()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a user copy operation.
 *          It validates that the virtual drive goes to the failed state and
 *          STAYS in the failed state until the `trigger spare timer' is
 *          reached.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  10/17/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_remove_source_drive_validate_delay(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       seconds_to_wait_before_validating_failed_state = 0;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 original_source_pvd_object_id;
    fbe_test_raid_group_disk_set_t  original_drive_location;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_lba_t                       hook_lba = FBE_LBA_INVALID;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_scheduler_debug_hook_t      hook;

    /* Set the trigger spare timer to the default value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);
    MUT_ASSERT_TRUE(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME >= 10);
    seconds_to_wait_before_validating_failed_state = (FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME / 10);
    if (seconds_to_wait_before_validating_failed_state > 10)
    {
        seconds_to_wait_before_validating_failed_state = 10;
    }
    
    /* Print the test start message
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "==%s Validate that virtual drive waits: %d (%d is trigger time) seconds before existing failed ==",
               __FUNCTION__, seconds_to_wait_before_validating_failed_state,
               FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &original_source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add debug hook at 5% rebuilt before proceeding.
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* start a user copy to disk */
    status = fbe_api_provision_drive_user_copy(original_source_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /* Set a debug hook when the virtual drive enters the failed state */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set a debug hook to log the vd if it enters the activate state */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the source drive. */
    original_drive_location = rg_config_p->rg_disk_set[drive_pos];
    diabolicaldiscdemon_remove_drive(rg_config_p, drive_pos, &original_drive_location,
                                     &rg_config_p->drive_handle[drive_pos],
                                     &rg_config_p->peer_drive_handle[drive_pos],
                                     FBE_TRUE,  /* Wait for pvd to Fail */
                                     FBE_TRUE   /* Wait for vd to Fail */ );

    /* wait for intiatation of swap-out */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the rebuild hook*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* remove the failed hook */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now wait for some time to validate we stay in the failed state*/
    mut_printf(MUT_LOG_TEST_STATUS, 
               "==%s Validate that vd obj: 0x%x stays in failed state for: %d seconds ==",
               __FUNCTION__, vd_object_id, seconds_to_wait_before_validating_failed_state);
    fbe_api_sleep((seconds_to_wait_before_validating_failed_state * 1000));

    /* Validate that we did not enter activate.
     */
    status = fbe_test_get_debug_hook(vd_object_id,
                                        SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                        FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY, 
                                        SCHEDULER_CHECK_STATES, 
                                        SCHEDULER_DEBUG_ACTION_PAUSE,
                                        0,0, &hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(hook.counter == 0);

    /* Now change the `trigger spare timer' to a small value and validate
     * that we do enter the activate state.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "==%s Set spare trigger to %d seconds and wait for source swap-out ==",
               __FUNCTION__, seconds_to_wait_before_validating_failed_state);
    fbe_test_sep_util_update_permanent_spare_trigger_timer(seconds_to_wait_before_validating_failed_state);

    /* wait for the source drive to swap-out. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* wait for virtual drive to transition to activate */
    status = fbe_test_wait_for_debug_hook(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                         FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                         SCHEDULER_CHECK_STATES, 
                                         SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now remove the activate hook */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE,
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY,
                                              0, 0, 
                                              SCHEDULER_CHECK_STATES, 
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now wait for the virtual drive to become ready and then try it again.
     */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object ready failed");

    /* Wait for rebuild to start */
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_pos);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &original_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set the trigger spare timer back to a small value*/
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1);

    mut_printf(MUT_LOG_TEST_STATUS, 
               "==%s Complete==",__FUNCTION__);
    return;
}
/*******************************************************************
 * end diabolicaldiscdemon_test_remove_source_drive_validate_delay()
 *******************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_remove_source_drive_during_copy_complete()
 *****************************************************************************
 *
 * @brief   This test removes the source drive during a `copy complete' operation.
 *          It removes the source drive immediately after the `change configuration
 *          mode to pass-thru' database transaction is started.  The expectation
 *          is that removing the source disk after the copy is completed
 *          should not prevent the copy complete job from completing successfully.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  06/20/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_remove_source_drive_during_copy_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                                              fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 original_source_pvd_object_id;
    fbe_object_id_t                 original_dest_pvd_object_id;
    fbe_test_raid_group_disk_set_t  original_drive_location;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_lba_t                       hook_lba = FBE_LBA_INVALID;

    mut_printf(MUT_LOG_TEST_STATUS, "Remove source drive during copy complete job!!!\n");

    /* Change the spare library `confirmation timeout' to a smaller value.
     * (Should be at least (10) seconds so that the virtual drive monitor can
     * run).
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(15);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, 
               rg_config_p->rg_disk_set[drive_pos].bus, 
               rg_config_p->rg_disk_set[drive_pos].enclosure, 
               rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &original_source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add debug hook at 5% rebuild hook
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /* Get the destination object id.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_location.bus,
                                                            spare_drive_location.enclosure,
                                                            spare_drive_location.slot,
                                                            &original_dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add a debug hook after the virtual drive changes configuration mode from
     * mirror to pass-thru (this is the first transaction for the copy complete
     * job).
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* remove the hook .*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* Wait for the copy complete change config mode hook
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for copy complete start hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note Remove the drive but do NOT wait for the virtual drive to goto the 
     *        Failed state.  Since the set config hook is prior to the eval rl 
     *        condition.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s remove source drive during copy complete vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    original_drive_location = rg_config_p->rg_disk_set[drive_pos];
    diabolicaldiscdemon_remove_drive(rg_config_p, drive_pos, &original_drive_location,
                                     &rg_config_p->drive_handle[drive_pos],
                                     &rg_config_p->peer_drive_handle[drive_pos],
                                     FBE_TRUE,  /* Wait for pvd to Fail */
                                     FBE_FALSE  /* Do not wait for vd to Fail */);

    /* Sleep for a short period
     */
    fbe_api_sleep(100);

    /* Remove the copy complete change configuration mode debug hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SET_CONFIG_MODE,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the virtual drive to goto the failed state.
     */
    shaggy_verify_sep_object_state(vd_object_id, FBE_LIFECYCLE_STATE_FAIL);

    /* wait for the source drive to swap-out (and the configuration mode to be
     * changed to pass-thru).
     */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,                
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &spare_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]); 

    /* Now wait for the virtual drive to become Ready.
     */
    status = fbe_api_wait_for_object_lifecycle_state(vd_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                     
    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &original_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Restore the spare library `confirmation timeout' to the default value.
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Remove source drive during copy complete job - completed!!!\n");
    return;
}
/*************************************************************************
 * end diabolicaldiscdemon_test_remove_source_drive_during_copy_complete()
 *************************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_source_drive_fault_during_copy_start()
 *****************************************************************************
 *
 * @brief   This test marks the source drive `Faulted' immediately after the
 *          proactive copy operation starts. It validates that the virtual drive 
 *          goes to the failed state and that the source drive is swapped out.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  04/11/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_source_drive_fault_during_copy_start(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_u32_t drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 pvd_object_id;
    fbe_object_id_t                 pdo_object_id;
    fbe_object_id_t                 original_source_pvd_object_id;
    fbe_test_raid_group_disk_set_t  original_source_location;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_api_virtual_drive_get_info_t vd_info;

    mut_printf(MUT_LOG_TEST_STATUS, "Drive Fault on source drive during proactive copy start test - started!!!\n");

    /* Set the trigger spare timer to the default value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Get virtual drive information
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine if we need to wait for the previous copy opertion to complete
     * or not.
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (((vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
           (vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    ) ||
          (vd_info.b_request_in_progress == FBE_TRUE)                                               )
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s vd obj: 0x%x mode: %d in-progress: %d wait for previous copy to complete",
                   __FUNCTION__, vd_object_id, vd_info.configuration_mode, vd_info.b_request_in_progress);
        status = diabolicaldiscdemon_wait_for_in_progress_request_to_complete(vd_object_id, 30000);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Wait for verify to complete.
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &original_source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    original_source_location = rg_config_p->rg_disk_set[drive_pos];

    /* Set a debug hook to stop the copy once the paged metadata is rebuilt.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add and hook when the virtual drive goes broken.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* trigger the proactive copy operation using error injection. 
     */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* wait for copy to start before setting drive fault in source drive.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for copy rebuild paged complete hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Trigger a drive fault my injecting an error
     */          
    mut_printf(MUT_LOG_TEST_STATUS, "==%s Trigger Drive Fault on source drive (%d_%d_%d) ", 
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, 
               rg_config_p->rg_disk_set[drive_pos].enclosure, 
               rg_config_p->rg_disk_set[drive_pos].slot); 
    diabolicdiscdemon_io_error_fault_drive(rg_config_p, drive_pos, source_edge_index);

    /* Wait for virtual drive downstream health broken
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for downstream health broken vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the destination drive was not rebuilt.
     */
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

    /* Set a hook to validate that the parent raid group has marked NR for
     * this position.
     */
    status = fbe_test_add_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the source drive removed hook
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the source drive to swap-out. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Wait for clear rebuild logging hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for clear rl hook rg obj: 0x%x ==", 
               __FUNCTION__, rg_object_id);
    status = fbe_test_wait_for_debug_hook_active(rg_object_id,
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                                 FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the position was marked NR.
     */
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(rg_object_id, drive_pos);

    /* Remove the clear rebuild logging hook.
     */
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for rebuild to start */
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_pos);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Remove the user data copy start
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note Must clear drive fault directly (cannot use test sep util)
     *        Get the pvd and pdo object ids
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(original_source_location.bus,
                                                            original_source_location.enclosure,
                                                            original_source_location.slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_get_physical_drive_object_id_by_location(original_source_location.bus,
                                                              original_source_location.enclosure,
                                                              original_source_location.slot, 
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* First clear the EOL path attribute
     */
    status = fbe_api_physical_drive_clear_eol(pdo_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now Clear the `drive fault'
     */
    status = fbe_test_sep_util_clear_drive_fault(original_source_location.bus,
                                                 original_source_location.enclosure,
                                                 original_source_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now clear EOL
     */
    status = fbe_test_sep_util_clear_disk_eol(original_source_location.bus,       
                                              original_source_location.enclosure, 
                                              original_source_location.slot);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Speed up VD hot spare */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Clear raid group and raid library debug flags for this test.
     */
    diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                     FBE_TRUE,   /* Disable the raid group debug flags */
                                                                     FBE_TRUE,   /* Disable the raid library debug flags */
                                                                     FBE_TRUE   /* Clear the debug flags */);

    mut_printf(MUT_LOG_TEST_STATUS, "Drive Fault on source drive during proactive copy start test - completed!!!\n");
    return;
}
/***************************************************************
 * end diabolicaldiscdemon_test_source_drive_fault_during_copy_start()
 ****************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_active()
 *****************************************************************************
 *
 * @brief   This test injects the source drive timeout errors during the 
 *          proactive copy operation where the rg and vd are active on the same SP.
 *          It validates that if the vd sets timeout errors while the raid group 
 *          is already rebuild logging the raid group will reevaluate its downstream
 *          edges.
 *
 *          Note: This solves the case where the raid group gets stuck in mark nr.
 *          The timeout attribute would get set on the rg-vd edge while the raid 
 *          group was rebuild logging. When the source drive died timeout errors were 
 *          still set and mark nr would skip the edge not allowing the vd to 
 *          complete with the swap out
 *          
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  10/27/2015  Deanna Heng  - Created from diabolicaldiscdemon_test_source_drive_fault_during_copy_start
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_active(fbe_test_rg_configuration_t *rg_config_p,
                                                                             fbe_u32_t drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 pvd_object_id;
    fbe_object_id_t                 pdo_object_id;
    fbe_test_raid_group_disk_set_t  original_source_location;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_api_virtual_drive_get_info_t vd_info;
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t    record_handle_p;

    mut_printf(MUT_LOG_TEST_STATUS, "Source drive timeout errors during proactive copy active test - started!!!\n");

    /* Set the trigger spare timer to the default value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Get virtual drive information
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine if we need to wait for the previous copy opertion to complete
     * or not.
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (((vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
           (vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    ) ||
          (vd_info.b_request_in_progress == FBE_TRUE)                                               )
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s vd obj: 0x%x mode: %d in-progress: %d wait for previous copy to complete",
                   __FUNCTION__, vd_object_id, vd_info.configuration_mode, vd_info.b_request_in_progress);
        status = diabolicaldiscdemon_wait_for_in_progress_request_to_complete(vd_object_id, 30000);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Refresh the raid group disk set information. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Wait for verify to complete.
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    /* get the edge index of the source and destination drive based on configuration mode. 
     */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. 
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    original_source_location = rg_config_p->rg_disk_set[drive_pos];

    status = fbe_api_get_physical_drive_object_id_by_location(original_source_location.bus,
                                                              original_source_location.enclosure,
                                                              original_source_location.slot, 
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set a debug hook to stop the copy once the paged metadata is rebuilt.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add and hook when the virtual drive goes broken.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* trigger the proactive copy operation using error injection. 
     */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. 
     */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* wait for copy to start before setting drive fault in source drive.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for copy rebuild paged complete hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Enable HC and clear error counts. 
    */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_TRUE /*enable*/);
    fbe_api_drive_configuration_set_service_timeout(500); //ms

    /* stop the job service recovery queue to hold the job service swap commands. 
     */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

     /* Invoke the method to initialize and populate the timeout error record
     */
    status =  diabolicdiscdemon_populate_timeout_error_injection_record(rg_config_p,
                                                                        drive_pos,
                                                                        &record,
                                                                        &record_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* inject the scsi errors. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Start I/O to force EOL.
     */
    status = diabolicaldiscdemon_run_io_to_start_copy(rg_config_p, drive_pos, source_edge_index,
                                                      FBE_SCSI_WRITE_16,
                                                      diabolicaldiscdemon_user_space_blocks_to_inject);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* restart the job service queue to proceed with job service swap command. 
     */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* Remove the user data copy start and allow copy to hit the errors
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for virtual drive downstream health broken
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for downstream health broken vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the destination drive was not rebuilt.
     */
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);


    /* Set a debug hook to hold the virtual drive in swap out 
     */
    status = fbe_api_scheduler_add_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set the debug hook that would clear timeout errors 
     */
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,
                                              FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY, 
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

     /* Set a hook to validate that the parent raid group has marked NR for
     * this position.
     */
    status = fbe_test_add_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the source drive removed hook
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the clear timeout errors hook
     * Previously, clear errors condition was not set. This hook validates that when
     * timeout errors are set on the edge we will clear them in the raid group even
     * if we are rebuild logging.
     */
    status = fbe_test_wait_for_debug_hook_active(rg_object_id,
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,
                                                 FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY,              
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the timeout errors to be set on the raid group object. 
     */
    status = fbe_api_wait_for_block_edge_path_attribute(rg_object_id, drive_pos, 
                                                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS , FBE_PACKAGE_ID_SEP_0, 60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s timeout flags set on raid group: 0x%x ==", 
               __FUNCTION__, rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Clear the timeout errors hook
     */
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,
                                              FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY, 
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set the `drive fault' in the pdo.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "==%s Trigger Drive Fault on source drive (%d_%d_%d) ", 
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, 
               rg_config_p->rg_disk_set[drive_pos].enclosure, 
               rg_config_p->rg_disk_set[drive_pos].slot); 

    status = fbe_api_physical_drive_update_drive_fault(pdo_object_id, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the `drive fault' state to be set in the associated pvd.
     */
    status = fbe_test_sep_drive_verify_pvd_state(original_source_location.bus,
                                                 original_source_location.enclosure,
                                                 original_source_location.slot,
                                                 FBE_LIFECYCLE_STATE_FAIL,
                                                 30000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the drive fault state is True.
     */
    status = fbe_test_sep_util_validate_drive_fault_state(original_source_location.bus,
                                                          original_source_location.enclosure,
                                                          original_source_location.slot,
                                                          FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Display some information.                 
     */                                          
    mut_printf(MUT_LOG_TEST_STATUS, "%s Drive Fault for pvd: 0x%x disk: 0x%x(%d_%d_%d) ", 
               __FUNCTION__, pvd_object_id, pdo_object_id,
               original_source_location.bus,
               original_source_location.enclosure,
               original_source_location.slot);

    /* Ensure that we will proceed through the swap out condition
     */
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

     /* Remove the swap out hook
      */
    status = fbe_api_scheduler_del_debug_hook(vd_object_id,
                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the timeout errors to be cleared on the raid group object. 
     */
    status = fbe_api_wait_for_block_edge_path_attribute_cleared(rg_object_id, drive_pos, 
                                                                FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS, 
                                                                FBE_PACKAGE_ID_SEP_0, 60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s timeout flags no longer set on raid group after swap out: 0x%x ==", 
               __FUNCTION__, rg_object_id);

    /* wait for the source drive to swap-out. 
     */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Wait for clear rebuild logging hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for clear rl hook rg obj: 0x%x ==", 
               __FUNCTION__, rg_object_id);
    status = fbe_test_wait_for_debug_hook_active(rg_object_id,
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                                 FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the position was marked NR.
     */
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(rg_object_id, drive_pos);

    /* Remove the clear rebuild logging hook.
     */
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                                            FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);

    /* Wait for rebuild to start */
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_pos);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* wait until raid completes rebuild to the spare drive. */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* First clear the EOL path attribute
     */
    status = fbe_api_physical_drive_clear_eol(pdo_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now Clear the `drive fault'
     */
    status = fbe_test_sep_util_clear_drive_fault(original_source_location.bus,
                                                 original_source_location.enclosure,
                                                 original_source_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now clear EOL
     */
    status = fbe_test_sep_util_clear_disk_eol(original_source_location.bus,       
                                              original_source_location.enclosure, 
                                              original_source_location.slot);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the raid group disk set information. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Speed up VD hot spare 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Clear raid group and raid library debug flags for this test.
     */
    diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                     FBE_TRUE,   /* Disable the raid group debug flags */
                                                                     FBE_TRUE,   /* Disable the raid library debug flags */
                                                                     FBE_TRUE   /* Clear the debug flags */);

    mut_printf(MUT_LOG_TEST_STATUS, "Source drive timeout errors during proactive copy active test - completed!!!\n");
    return;
}
/***************************************************************
 * end diabolicaldiscdemon_test_source_drive_errors_during_copy_active()
 ****************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_passive()
 *****************************************************************************
 *
 * @brief   This test injects the source drive timeout errors during the 
 *          proactive copy operation where the vd is active on SP A and the 
 *          raid group is active on SP B. 
 *          This validates that if the vd sets timeout errors on the upstream edge
 *          while the raid group is already rebuild logging, then we will reevaluate
 *          the downstream edges on the raid group. 
 * 
 *          Note: This solves the case where the raid group would get stuck in clear 
 *          rebuild logging since the raid group's passive side still has 
 *          timeout errors on the edge
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  10/27/2015  Deanna Heng  - Created from diabolicaldiscdemon_test_source_drive_fault_during_copy_start
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_source_drive_timeout_errors_during_copy_passive(fbe_test_rg_configuration_t *rg_config_p,
                                                                             fbe_u32_t drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_object_id_t                 pvd_object_id;
    fbe_object_id_t                 pdo_object_id;
    fbe_test_raid_group_disk_set_t  original_source_location;
    fbe_test_raid_group_disk_set_t  spare_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_api_virtual_drive_get_info_t vd_info;
    fbe_protocol_error_injection_error_record_t     record;
    fbe_protocol_error_injection_record_handle_t    record_handle_p;

    mut_printf(MUT_LOG_TEST_STATUS, "Source drive timeout errors during proactive copy passive test - started!!!\n");

    /* Set the trigger spare timer to the default value.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(FBE_SPARE_DEFAULT_SWAP_IN_TRIGGER_TIME);

    /* Get virtual drive information
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine if we need to wait for the previous copy opertion to complete
     * or not.
     */
    status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (((vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
           (vd_info.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    ) ||
          (vd_info.b_request_in_progress == FBE_TRUE)                                               )
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s vd obj: 0x%x mode: %d in-progress: %d wait for previous copy to complete",
                   __FUNCTION__, vd_object_id, vd_info.configuration_mode, vd_info.b_request_in_progress);
        status = diabolicaldiscdemon_wait_for_in_progress_request_to_complete(vd_object_id, 30000);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Refresh the raid group disk set information. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Wait for verify to complete.
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start proactive sparing operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    /* get the edge index of the source and destination drive based on configuration mode. 
     */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. 
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    original_source_location = rg_config_p->rg_disk_set[drive_pos];

    status = fbe_api_get_physical_drive_object_id_by_location(original_source_location.bus,
                                                              original_source_location.enclosure,
                                                              original_source_location.slot, 
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set a debug hook to stop the copy once the paged metadata is rebuilt.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add and hook when the virtual drive goes broken.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* trigger the proactive copy operation using error injection. 
     */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. 
     */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* wait for copy to start before setting drive fault in source drive.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for copy rebuild paged complete hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Enable HC and clear error counts. 
    */
    fbe_api_drive_configuration_set_control_flag(FBE_DCS_HEALTH_CHECK_ENABLED, FBE_TRUE /*enable*/);
    fbe_api_drive_configuration_set_service_timeout(500); //ms

     /* Invoke the method to initialize and populate the (1) timeout error record
     */
    status =  diabolicdiscdemon_populate_timeout_error_injection_record(rg_config_p,
                                                                        drive_pos,
                                                                        &record,
                                                                        &record_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* inject the scsi errors. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Start I/O to force EOL.
     */
    status = diabolicaldiscdemon_run_io_to_start_copy(rg_config_p, drive_pos, source_edge_index,
                                                      FBE_SCSI_WRITE_16,
                                                      diabolicaldiscdemon_user_space_blocks_to_inject);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Remove the user data copy start and allow copy to hit the errors
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_USER_DATA_START,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for virtual drive downstream health broken
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for downstream health broken vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the destination drive was not rebuilt.
     */
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

    /* Make sure we go through this condition when the vd sets timeout errors and we are already 
     * rebuild logging. Previously we were not clearing these timeout errors if the vd had set timeout errors
     * and the raid group was already rebuild logging for this position. 
     */
    status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,
                                              FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY, 
                                              0, 0,              
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the source drive removed hook
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED, 
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the condition that would clear errors on the raid group's downstream edges
     */
    status = fbe_test_wait_for_debug_hook(rg_object_id,
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,
                                          FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY, 
                                          SCHEDULER_CHECK_STATES,
                                          SCHEDULER_DEBUG_ACTION_PAUSE,
                                          0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the timeout errors to be set on the raid group object. 
     */
    status = fbe_api_wait_for_block_edge_path_attribute(rg_object_id, drive_pos, 
                                                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS , FBE_PACKAGE_ID_SEP_0, 60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s timeout flags set on raid group: 0x%x ==", 
               __FUNCTION__, rg_object_id);

    /* Delete the hook for the function that would clear timeout errors on the raid group
     */
    status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                              SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_ERRORS,
                                              FBE_RAID_GROUP_SUBSTATE_CLEAR_ERRORS_ENTRY, 
                                              0, 0,
                                              SCHEDULER_CHECK_STATES,
                                              SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /* Wait for the timeout errors to be cleared on the raid group object. 
     */
    status = fbe_api_wait_for_block_edge_path_attribute_cleared(rg_object_id, drive_pos, 
                                                                FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS, 
                                                                FBE_PACKAGE_ID_SEP_0, 60000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s timeout flags no longer set on raid group after swap out: 0x%x ==", 
               __FUNCTION__, rg_object_id);

    /* Wait for rebuild to start 
     */
    sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_pos);

    /* wait until raid completes rebuild to the spare drive. 
     */
    sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_pos);
    sep_rebuild_utils_check_bits(rg_object_id);

    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* Validate that the destination position is not marked NR 
     */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. 
     */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* First clear the EOL path attribute
     */
    status = fbe_api_physical_drive_clear_eol(pdo_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now clear EOL
     */
    status = fbe_test_sep_util_clear_disk_eol(original_source_location.bus,       
                                              original_source_location.enclosure, 
                                              original_source_location.slot);    
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Refresh the raid group disk set information. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Speed up VD hot spare 
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

    /* Clear raid group and raid library debug flags for this test.
     */
    diabolicaldiscdemon_disable_debug_flags_for_virtual_drive_object(vd_object_id,
                                                                     FBE_TRUE,   /* Disable the raid group debug flags */
                                                                     FBE_TRUE,   /* Disable the raid library debug flags */
                                                                     FBE_TRUE   /* Clear the debug flags */);

    mut_printf(MUT_LOG_TEST_STATUS, "Source drive timeout errors during proactive copy passive test - completed!!!\n");
    return;
}
/***************************************************************
 * end diabolicaldiscdemon_test_source_drive_errors_during_copy_passive()
 ****************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_user_copy_complete_set_checkpoint_times_out()
 *****************************************************************************
 *
 * @brief   This test a time-out while updating the non-paged information
 *          after a user copy completed.  We expect that copy complete job to
 *          fail and the job be rolled back `successfully'.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @note    User copy is complete and source drive is swapped out.  The last
 *          step in the copy complete job is to set the destination drive
 *          checkpoint to the end marker.  The non-paged write takes to long
 *          so we rollback the job.
 *
 * @author
 *  05/20/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_user_copy_complete_set_checkpoint_times_out(fbe_test_rg_configuration_t *rg_config_p,
                                                                                 fbe_u32_t  drive_pos)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id;
    fbe_object_id_t                 vd_object_id;
    fbe_bool_t                      b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    fbe_u32_t                       source_pvd_bus;       
    fbe_u32_t                       source_pvd_enclosure;  
    fbe_u32_t                       source_pvd_slot;
    fbe_object_id_t                 source_pvd_object_id;
    fbe_object_id_t                 dest_pvd_object_id;
    fbe_test_raid_group_disk_set_t  dest_drive_location;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_lba_t                       hook_lba = FBE_LBA_INVALID;
    fbe_provision_drive_copy_to_status_t copy_status;
    fbe_base_config_encryption_mode_t encryption_mode;
    fbe_bool_t                      b_is_encryption_enabled = FBE_FALSE;
    fbe_bool_t                      b_event_found = FBE_FALSE;
    fbe_api_provision_drive_get_swap_pending_t swap_pending_info;
    fbe_api_provision_drive_info_t  pvd_info;
    fbe_sim_transport_connection_target_t active_sp = fbe_api_sim_transport_get_target_server();
    fbe_sim_transport_connection_target_t source_pvd_active_sp;
    fbe_sim_transport_connection_target_t source_pvd_passive_sp;

    mut_printf(MUT_LOG_TEST_STATUS, "User Copy completes but set checkpoint to end times-out!!!\n");

    /* Change the spare library `confirmation timeout' to a smaller value.
     * (Should be at least (10) seconds so that the virtual drive monitor can
     * run).
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(15);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: start user copy operation on bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               __FUNCTION__, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Get the provision object to set debug hook, etc. */
    source_pvd_bus = rg_config_p->rg_disk_set[drive_pos].bus;  
    source_pvd_enclosure = rg_config_p->rg_disk_set[drive_pos].enclosure;
    source_pvd_slot = rg_config_p->rg_disk_set[drive_pos].slot;
    status = fbe_api_provision_drive_get_obj_id_by_location(source_pvd_bus,
                                                            source_pvd_enclosure,
                                                            source_pvd_slot,
                                                            &source_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the active SP for the original source drive.
     */
    status = fbe_test_sep_util_get_active_passive_sp(source_pvd_object_id,
                                                     &source_pvd_active_sp,
                                                     &source_pvd_passive_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If encryption is enabled on the source, set the scrub hooks.
     */
    status = fbe_api_base_config_get_encryption_mode(source_pvd_object_id, &encryption_mode);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Getting encryption mode failed\n");
    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)      ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)                 ) {
        b_is_encryption_enabled = FBE_TRUE;
    }

    if (b_is_encryption_enabled)
    {
        /*TODO: We do not know how to handle the rollback scenario when the drives are encrypted*/
        mut_printf(MUT_LOG_TEST_STATUS, "User copy complete set checkpoint times out - completed!!!\n");
        return;
    }

    /* Get a valid destination drive.
     */
    status = diabolicaldiscdemon_get_valid_destination_drive(rg_config_p, &dest_drive_location);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                            dest_drive_location.enclosure,
                                                            dest_drive_location.slot,
                                                            &dest_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Add debug hook at 5% rebuild hook
     */
    hook_lba = diabolicaldiscdemon_set_rebuild_hook(vd_object_id, 5);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s added rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);

    /* Start user copy */
    status = fbe_api_copy_to_replacement_disk(source_pvd_object_id, dest_pvd_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &dest_drive_location);

    /* wait for copy to start before removing the source drive. */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s wait for rebuild hook vd obj: 0x%x lba: 0x%llx ==", 
               __FUNCTION__, vd_object_id, hook_lba);
    diabolicaldiscdemon_wait_for_rebuild_hook(vd_object_id, hook_lba);

    /*! @note Add a hook for the `set checkpoint to end marker condition'. 
     *        Although the virtual drive is in the failed state it does not
     *        prevent the copy from completing.  This is due to the fact that
     *        to `complete' the copy we access the non-paged drives not the
     *        source or destination drives.
     * 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Add copy complete set checkpoint hook vd obj: 0x%x ==", 
               __FUNCTION__, vd_object_id);
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE_SET_CHECKPOINT,
                                            0, 0,              
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add a job service hook when we start the rollback.
     */
    status = fbe_test_add_job_service_hook(vd_object_id,                   
                                           FBE_JOB_TYPE_DRIVE_SWAP,                    
                                           FBE_JOB_ACTION_STATE_ROLLBACK,           
                                           FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START, 
                                           FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add a debug hook when the copy complete operation completes.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                            0, 0,  
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If encryption is enabled stop when the swap-out is complete.
     */
    if (b_is_encryption_enabled) {
        /* Add a debug hook when the swap-out is complete.
         */
        status = fbe_test_add_debug_hook_active(vd_object_id,
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE, 
                                                0, 0,  
                                                SCHEDULER_CHECK_STATES, 
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

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
     
    /* remove the hook .*/
    diabolicaldiscdemon_remove_rebuild_hook(vd_object_id, hook_lba);

    /* wait for the source drive to swap-out (and the configuration mode to be
     * changed to pass-thru).
     */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);

    /* Disable the recovery queue so that no new jobs can start.
     */
    fbe_test_sep_util_disable_recovery_queue(vd_object_id);

    /* If encryption is enabled stop when the swap-out is complete.
     */
    if (b_is_encryption_enabled) {
        /* Wait for the swap-out to complete.
         */
        status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                     SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                                     FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,  
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_PAUSE,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_sleep(3000);
        status = fbe_api_provision_drive_get_swap_pending(source_pvd_object_id,
                                                          &swap_pending_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(swap_pending_info.get_swap_pending_reason == FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY);
        status = fbe_api_provision_drive_get_info(source_pvd_object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(pvd_info.swap_pending == FBE_TRUE);

        /* Now clear the hook so that the set checkpoint can timeout.
         */
        status = fbe_test_del_debug_hook_active(vd_object_id, 
                                                SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                                FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE, 
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    } /* end if encryption is enabled check and clear additional hooks*/

    /* Wait for the copy complete set checkpoint to end marker hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Wait for copy complete set checkpoint hook vd obj: 0x%x ==", 
               vd_object_id);
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE_SET_CHECKPOINT,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the rollback to start.
     */
    status = fbe_test_wait_for_job_service_hook(vd_object_id,                   
                                                FBE_JOB_TYPE_DRIVE_SWAP,                    
                                                FBE_JOB_ACTION_STATE_ROLLBACK,           
                                                FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START, 
                                                FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now wait for the virtual drive to become Ready.
     */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* perform the i/o after the user copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Wait for the swap-out to complete.
     */
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                                 FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If encryption is enabled validate that the source drive is still failed.
     */
    if (b_is_encryption_enabled) {
        status = fbe_api_provision_drive_get_swap_pending(source_pvd_object_id,
                                                          &swap_pending_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(swap_pending_info.get_swap_pending_reason == FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY);
        status = fbe_api_provision_drive_get_info(source_pvd_object_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(pvd_info.swap_pending == FBE_TRUE);

    } /* end if encryption is enabled check and clear additional hooks*/

    /* Remove the `copy complete set checkpoint' hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE_SET_CHECKPOINT,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the job service rollback hook.
     */
    status = fbe_test_del_job_service_hook(vd_object_id,                   
                                           FBE_JOB_TYPE_DRIVE_SWAP,                    
                                           FBE_JOB_ACTION_STATE_ROLLBACK,           
                                           FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START, 
                                           FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now remove the swap complete hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id, 
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE,
                                            0, 0,
                                            SCHEDULER_CHECK_STATES,
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Verify `unexpected error' (i.e. timeout) event log. Although message 
     * contains original location first parameter is job error.  Therefore we
     * cannot use `diabolicdiscdemon_check_event_log_msg'.
     */
    status = fbe_test_sep_event_validate_event_generated(SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR,
                                                         &b_event_found,
                                                         FBE_TRUE, /* If the event is found reset logs*/
                                                         30000 /* Wait up to 30 seconds for event*/);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);

    /* Enable the recovery queue and we expect the copy complete to proceed.
     */
    fbe_test_sep_util_enable_recovery_queue(vd_object_id);

    /* Wait for the user copy to complete.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "==%s vd obj: 0x%x wait for user copy to complete ==",
                   __FUNCTION__, vd_object_id);
    status = diabolicaldiscdemon_wait_for_in_progress_request_to_complete(vd_object_id, 30000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Now wait for the virtual drive to become Ready.
     */
    status = fbe_test_common_util_wait_for_object_lifecycle_state_all_sps(fbe_test_sep_util_get_dualsp_test_mode(),
                                                                          vd_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                          20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* There should be not bits marked in parent */
    sep_rebuild_utils_check_bits(rg_object_id);

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /*! @todo The original source drive has been marked offline.  We must manually
     *        bring it back.
     */
    if (b_is_encryption_enabled) {

        status = fbe_api_provision_drive_get_swap_pending(source_pvd_object_id,
                                                          &swap_pending_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(swap_pending_info.get_swap_pending_reason == FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_NONE);

        /* Validate that the source drive is re-zeroed after being swapped out.
         */
        status = fbe_test_wait_for_debug_hook_active(source_pvd_object_id,
                                                     SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                                     FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START,  
                                                     SCHEDULER_CHECK_STATES,
                                                     SCHEDULER_DEBUG_ACTION_LOG,
                                                     0, 0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now remove the re-zero hook.
         */
        status = fbe_test_del_debug_hook_active(source_pvd_object_id, 
                                                SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE,
                                                FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START, 
                                                0, 0,
                                                SCHEDULER_CHECK_STATES,
                                                SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now wait for the zeroing to start.
         */
        if ((b_is_dualsp_test)                  &&
            (source_pvd_active_sp != active_sp)    ) {
            status = fbe_api_sim_transport_set_target_server(source_pvd_active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        status = fbe_test_sep_drive_check_for_disk_zeroing_event(source_pvd_object_id, 
                                                                 SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED,  
                                                                 source_pvd_bus, source_pvd_enclosure, source_pvd_slot); 
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if ((b_is_dualsp_test)                  &&
            (source_pvd_active_sp != active_sp)    ) {
            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

    } /* end if encryption is enabled check and clear additional hooks*/

    /* perform the i/o after the proactive copy to verify that rebuild gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Restore the spare library `confirmation timeout' to the default value.
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "User copy complete set checkpoint times out - completed!!!\n");
    return;
}
/*******************************************************************************
 * end diabolicaldiscdemon_test_user_copy_complete_set_checkpoint_times_out()
 *******************************************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_proactive_copy_start_long_mark_nr_succeeds()
 *****************************************************************************
 *
 * @brief   Marking NR in the paged metadta can take some time (with heavy load
 *          over 7 minutes has bene observed).  Therefore the copy start request
 *          should NOT be waiting for the marking of NR in the paged metadata to
 *          complete.
 *
 * @param   rg_config_p - Raid group test configuration pointer
 * @param   drive_pos - The position in the raid group to test
 *
 * @return  None
 *
 * @author
 *  09/22/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void diabolicaldiscdemon_test_proactive_copy_start_long_mark_nr_succeeds(fbe_test_rg_configuration_t *rg_config_p,
                                                                                fbe_u32_t  drive_pos)
{
    fbe_status_t                            status;
    fbe_object_id_t                         rg_object_id;
    fbe_object_id_t                         vd_object_id;
    fbe_edge_index_t                        source_edge_index;
    fbe_edge_index_t                        dest_edge_index;
    fbe_test_raid_group_disk_set_t          source_drive_location;
    fbe_test_raid_group_disk_set_t          spare_drive_location;
    fbe_object_id_t							spare_pvd_object_id;
    fbe_api_rdgen_context_t                *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_object_id_t                         pvd_object_id;
    fbe_scheduler_debug_hook_t              hook;
    fbe_job_service_debug_hook_t            job_hook;
    fbe_u32_t                               wait_for_timeout_secs = (10 * 2);

    mut_printf(MUT_LOG_TEST_STATUS, "Proactive copy start long mark NR time - start\n");

    source_drive_location = rg_config_p->rg_disk_set[drive_pos];
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Long mark NR start proactive sparing operation on object 0x%x with bus:0x%x, encl:0x%x, slot:0x%x!!!.",
               pvd_object_id, rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = diabolicaldiscdemon_get_vd_and_rg_object_ids(rg_object_id, drive_pos, &vd_object_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Change the spare library `confirmation timeout' to a smaller value.
     * (Should be at least (10) seconds so that the virtual drive monitor can
     * run).
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(10);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Set a debug hook when we start to mark NR for the destination
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                            FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED, 
                                            0, 0,  
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add a debug hook when the copy complete operation completes.
     * (We should not encounter this)
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                            0, 0,  
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add a job service hook when we start the rollback.
     * (We should not encounter this)
     */
    status = fbe_test_add_job_service_hook(vd_object_id,                   
                                           FBE_JOB_TYPE_DRIVE_SWAP,                    
                                           FBE_JOB_ACTION_STATE_ROLLBACK,           
                                           FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START, 
                                           FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* proactive copy, we trigger the proactive copy operation using error injection. */
    diabolicdiscdemon_io_error_trigger_proactive_sparing(rg_config_p, drive_pos, source_edge_index);

    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* Wait for the eval mark NR hook in the virtual drive.
     */
    status = fbe_test_wait_for_debug_hook_active(vd_object_id,
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                                 FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED,  
                                                 SCHEDULER_CHECK_STATES,
                                                 SCHEDULER_DEBUG_ACTION_PAUSE,
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable error trace limits since the rollback will fail since the
     * transaction is complete.
     */
    fbe_test_sep_util_disable_trace_limits();

    /* Now wait 15 second to cause a timeout.*/
    mut_printf(MUT_LOG_TEST_STATUS, "Long mark NR - wait: %d seconds to confirm no timeout", wait_for_timeout_secs);
    fbe_api_sleep((wait_for_timeout_secs * 1000));
    mut_printf(MUT_LOG_TEST_STATUS, "Long mark NR - now confirm copy continued without timeout");

    /* Validate that we did not wait for the swap request complete condition.
     */
    status = fbe_test_get_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE,
                                            0,0, &hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(hook.counter == 0);

    /* Validate that we did not attempt to rollback the job.
     */
    status = fbe_test_get_job_service_hook(vd_object_id,                     
                                           FBE_JOB_TYPE_DRIVE_SWAP,                      
                                           FBE_JOB_ACTION_STATE_ROLLBACK,             
                                           FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START,   
                                           FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG,
                                           &job_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(job_hook.hit_counter == 0);

    /* Re-enable error trace limits.*/
    fbe_test_sep_util_enable_trace_limits(FBE_TRUE);

    /* Restore the spare library `confirmation timeout' to the default value.
     */
    status = fbe_test_sep_util_set_spare_library_confirmation_timeout(FBE_JOB_SERVICE_DEFAULT_SPARE_OPERATION_TIMEOUT_SECONDS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the copy complete condition hook
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP, 
                                            FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_COMPLETE, 
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the rollback hook
     */
    status = fbe_test_del_job_service_hook(vd_object_id,
                                           FBE_JOB_TYPE_DRIVE_SWAP,                      
                                           FBE_JOB_ACTION_STATE_ROLLBACK,             
                                           FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START,   
                                           FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove the virtual drive eval mark NR hook.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                                            FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED, 
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for rebuild logging to be clear for the destination. */
    status = sep_rebuild_utils_wait_for_rb_logging_clear_for_position(vd_object_id, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* */
    /* Validate that NR is set on the destination drive*/
    fbe_test_sep_rebuild_utils_check_bits_set_for_position(vd_object_id, dest_edge_index);

    /* Validate that no bits are set in the parent raid group*/
    sep_rebuild_utils_check_bits(rg_object_id);

    /* Validate in the rg info we can see the destination drive being part of RG now*/
    spare_pvd_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_provision_drive_get_obj_id_by_location(spare_drive_location.bus,
                                                            spare_drive_location.enclosure,
                                                            spare_drive_location.slot,
                                                            &spare_pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, spare_pvd_object_id);

    /* wait for the proactive copy completion. */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* Verify event log message for system initiated proactive copy operation  */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_COPY_OPERATION_COMPLETED,
                                          FBE_TRUE, /* Message contains spare drive location */
                                          &spare_drive_location,
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]);
    
    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &spare_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]);  
    
    /* wait for the proactive copy operation to complete. */
    diabolicdiscdemon_wait_for_copy_job_to_complete(vd_object_id);

    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Cleanup removed, EOL drives*/
    status = diabolicaldiscdemon_cleanup_reinsert_source_drive(rg_config_p, &source_drive_location, drive_pos);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Proactive copy start long mark NR time complete - success");
    return;
}
/*******************************************************************************
 * end diabolicaldiscdemon_test_proactive_copy_start_long_mark_nr_succeeds
 *******************************************************************************/


/* Populate the io error record */
static fbe_status_t diabolicdiscdemon_populate_io_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
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
    diabolicaldiscdemon_default_offset = provision_drive_info.default_offset;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (diabolicaldiscdemon_b_is_DE542_fixed == FBE_FALSE)
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
                                                                          diabolicaldiscdemon_user_space_lba_to_inject_to,
                                                                          diabolicaldiscdemon_user_space_blocks_to_inject,
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
 *          diabolicdiscdemon_populate_timeout_error_injection_record()
 *****************************************************************************
 *
 * @brief   Populate the timeout protocol error injection record
 *
 * @param   rg_config_p - Pointer to raid group array
 * @param   drive_pos - Position to inject timeout errors on
 * @param   record_p - protocol error injection record
 * @param   record_handle_p - handle to the protocol error injection record
 *
 * @return  status - FBE_STATUS_OK
 *
 * @author
 *  10/25/2015  Deanna Heng  - Created.
 *
 *****************************************************************************/
static fbe_status_t diabolicdiscdemon_populate_timeout_error_injection_record(fbe_test_rg_configuration_t *rg_config_p,
                                                                              fbe_u32_t drive_pos,
                                                                              fbe_protocol_error_injection_error_record_t *record_p,
                                                                              fbe_protocol_error_injection_record_handle_t *record_handle_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_object_id_t             drive_object_id = 0;
    fbe_object_id_t             pvd_object_id = 0;
    fbe_api_provision_drive_info_t provision_drive_info;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;
    fbe_lba_t                   start_lba = FBE_LBA_INVALID;
    fbe_lba_t                   end_lba = FBE_LBA_INVALID;

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
    default_offset = provision_drive_info.default_offset;
    start_lba = diabolicaldiscdemon_user_space_lba_to_inject_to + default_offset;
    end_lba = default_offset + diabolicaldiscdemon_user_space_lba_to_inject_to + (diabolicaldiscdemon_user_space_blocks_to_inject - 1);
    if (end_lba >= provision_drive_info.capacity) {
        end_lba = provision_drive_info.capacity - 1;
    }

    /* We need to convert to the proper physical drive block to inject to.
     */
    status = fbe_test_neit_utils_convert_to_physical_drive_range(drive_object_id,
                                                                 start_lba,
                                                                 end_lba,
                                                                 &start_lba,
                                                                 &end_lba);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize the record
     */
    fbe_test_neit_utils_init_error_injection_record(record_p);

    /* Populate the record
     */
    record_p->lba_start  = start_lba;
    record_p->lba_end    = FBE_LBA_INVALID;//end_lba;
    record_p->object_id  = drive_object_id;
    record_p->protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT;
    record_p->protocol_error_injection_error.protocol_error_injection_port_error.scsi_command[0] = FBE_SCSI_READ_16;
    record_p->protocol_error_injection_error.protocol_error_injection_port_error.scsi_command[1] = FBE_SCSI_WRITE_16;
    record_p->protocol_error_injection_error.protocol_error_injection_port_error.scsi_command[2] = FBE_SCSI_VERIFY_16;
    record_p->protocol_error_injection_error.protocol_error_injection_port_error.port_status = FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT;
    record_p->protocol_error_injection_error.protocol_error_injection_port_error.check_ranges = FBE_TRUE;
    record_p->num_of_times_to_insert = 0xF000;
    record_p->secs_to_reactivate = FBE_U32_MAX; /* Don't re-activate.*/

    /* Add the error injection record to the service, which also returns 
     * the record handle for later use.
     */
    status = fbe_api_protocol_error_injection_add_record(record_p, record_handle_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Print some information about the record
     */
    mut_printf(MUT_LOG_TEST_STATUS, "inject timeout error record for obj: 0x%x lba: 0x%llx end: 0x%llx times: %d",
               drive_object_id, 
               (unsigned long long)start_lba, (unsigned long long)end_lba, 
               record_p->num_of_times_to_insert);
    

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}
/********************************************************
 * end diabolicdiscdemon_populate_timeout_error_injection_record()
 ********************************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_run_io_to_start_copy()
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
static fbe_status_t diabolicaldiscdemon_run_io_to_start_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                             fbe_u32_t sparing_position,
                                                             fbe_edge_index_t source_edge_index,
                                                             fbe_u8_t requested_scsi_opcode_to_inject_on,
                                                             fbe_block_count_t io_size)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t        *rdgen_context_p = &diabolicaldiscdemon_test_context[0];
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
    user_space_start_lba = diabolicaldiscdemon_user_space_lba_to_inject_to;
    blocks = io_size;

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed force the
     *                SCSI opcode to inject on to VERIFY.
     */
    if (diabolicaldiscdemon_b_is_DE542_fixed == FBE_FALSE)
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
            start_lba = user_space_start_lba + diabolicaldiscdemon_default_offset;
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
            start_lba = user_space_start_lba + diabolicaldiscdemon_default_offset;
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
            start_lba = user_space_start_lba + diabolicaldiscdemon_default_offset;
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
 * end diabolicaldiscdemon_run_io_to_start_copy()
 ********************************************************/

/*!***************************************************************************
 *          diabolicdiscdemon_io_error_trigger_proactive_sparing()
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
static void diabolicdiscdemon_io_error_trigger_proactive_sparing(fbe_test_rg_configuration_t *rg_config_p,
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
    status = diabolicdiscdemon_populate_io_error_injection_record(rg_config_p,
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
    status = diabolicaldiscdemon_run_io_to_start_copy(rg_config_p, drive_pos, source_edge_index,
                                                      FBE_SCSI_WRITE_16,
                                                      diabolicaldiscdemon_user_space_blocks_to_inject);
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
 * end diabolicdiscdemon_io_error_trigger_proactive_sparing()
 ************************************************************/

/*!***************************************************************************
 *          diabolicdiscdemon_io_error_fault_drive()
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
static void diabolicdiscdemon_io_error_fault_drive(fbe_test_rg_configuration_t *rg_config_p,
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
    status = diabolicdiscdemon_populate_io_error_injection_record(rg_config_p,
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
    status = diabolicaldiscdemon_run_io_to_start_copy(rg_config_p, drive_pos, source_edge_index,
                                                      FBE_SCSI_WRITE_16,
                                                      diabolicaldiscdemon_user_space_blocks_to_inject);
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
 * end diabolicdiscdemon_io_error_fault_drive()
 **************************************************************/


void diabolicdiscdemon_wait_for_proactive_spare_swap_in(fbe_object_id_t vd_object_id,
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
                                                               DIABOLICDISCDEMON_TEST_SWAP_WAIT_TIME_MS);
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
 *          diabolicaldiscdemon_wait_for_in_progress_request_to_complete()
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
static fbe_status_t diabolicaldiscdemon_wait_for_in_progress_request_to_complete(fbe_object_id_t vd_object_id,
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
 * end diabolicaldiscdemon_wait_for_in_progress_request_to_complete()
 ********************************************************************/

void diabolicdiscdemon_wait_for_source_drive_swap_out(fbe_object_id_t vd_object_id,
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
                                                               DIABOLICDISCDEMON_TEST_SWAP_WAIT_TIME_MS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_block_edge_path_state(vd_object_id, source_edge_index, FBE_PATH_STATE_INVALID, FBE_PACKAGE_ID_SEP_0, 5000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:source drive is swapped-out, vd_object_id:0x%x.\n", __FUNCTION__, vd_object_id);
    return;
}

static void diabolicdiscdemon_wait_for_destination_drive_swap_out(fbe_object_id_t vd_object_id,
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
                                                               DIABOLICDISCDEMON_TEST_SWAP_WAIT_TIME_MS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_block_edge_path_state(vd_object_id, dest_edge_index, FBE_PATH_STATE_INVALID, FBE_PACKAGE_ID_SEP_0, 5000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:proactive spare drive is swapped-out, vd_object_id:0x%x.\n", __FUNCTION__, vd_object_id);
    return;
}

void diabolicdiscdemon_wait_for_proactive_copy_completion(fbe_object_id_t    vd_object_id)
{
    fbe_status_t                    status;              
    fbe_u32_t                       retry_count;
    fbe_u32_t                       max_retries;         
    fbe_api_virtual_drive_get_info_t virtual_drive_info;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;


    /* set the max retries in a local variable. */
    max_retries = DIABOLICDISCDEMON_TEST_MAX_RETRIES;

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
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
        fbe_api_sleep(DIABOLICDISCDEMON_TEST_RETRY_TIME);
    }

    /*  The rebuild operation did not finish within the time limit, assert here. */
    MUT_ASSERT_TRUE(0);
    return;
}

static fbe_status_t diabolicdiscdemon_wait_for_copy_job_to_complete(fbe_object_id_t vd_object_id)
{
    fbe_status_t                    status;              
    fbe_u32_t                       retry_count;
    fbe_u32_t                       max_retries;         
    fbe_api_virtual_drive_get_info_t virtual_drive_info;
    fbe_edge_index_t                source_edge_index;
    fbe_edge_index_t                dest_edge_index;


    /* set the max retries in a local variable. */
    max_retries = DIABOLICDISCDEMON_TEST_MAX_RETRIES;

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
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
        fbe_api_sleep(DIABOLICDISCDEMON_TEST_RETRY_TIME);
    }

    /*  The copy operation did not finish within the time limit, assert here. */
    return FBE_STATUS_GENERIC_FAILURE;
}

void diabolicdiscdemon_write_background_pattern(fbe_api_rdgen_context_t *  in_test_context_p)
{
    fbe_status_t    status;

    /*  Set up the context to do a write (fill) operation. */
    fbe_test_sep_io_setup_fill_rdgen_test_context(in_test_context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN, 
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID,
                                            DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);
                                            
    /*  Write a background pattern to all of the LUNs in the entire configuration. */
    status = fbe_api_rdgen_test_context_run_all_tests(in_test_context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);
    return;
}


void diabolicdiscdemon_verify_background_pattern(fbe_api_rdgen_context_t *  in_test_context_p,
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

void diabolicdiscdemon_get_source_destination_edge_index(fbe_object_id_t     vd_object_id,
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


/******************************************* 
 * DIABOLICDISCDEMON TESTS START HERE !!
 ********************************************/

/*!***************************************************************************
 *          diabolicaldiscdemon_test_user_copy()
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
 *
 * @return  status - FBE_STATUS_OK.
 *
 * @author
 *  08/26/2009:  Created. Dhaval Patel.
 *****************************************************************************/
static void diabolicaldiscdemon_test_user_copy(fbe_test_rg_configuration_t *rg_config_p, 
                                               fbe_u32_t drive_pos)
{
    fbe_object_id_t                             rg_object_id;
    fbe_object_id_t                             vd_object_id;
    fbe_object_id_t                             src_object_id;
    fbe_object_id_t                             dest_object_id;
    fbe_object_id_t                             temp_object_id;
    fbe_status_t                                status;
    fbe_edge_index_t                            source_edge_index;
    fbe_edge_index_t                            dest_edge_index;
    fbe_test_raid_group_disk_set_t              dest_drive_location;
    fbe_test_raid_group_disk_set_t              spare_drive_location;
    fbe_api_rdgen_context_t *                   rdgen_context_p = &diabolicaldiscdemon_test_context[0];
    fbe_provision_drive_copy_to_status_t        copy_status;

    /* Get the source drive object id */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[drive_pos].bus,
                                                            rg_config_p->rg_disk_set[drive_pos].enclosure,
                                                            rg_config_p->rg_disk_set[drive_pos].slot,
                                                            &src_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, drive_pos, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get a valid destination drive.
     */
    status = diabolicaldiscdemon_get_valid_destination_drive(rg_config_p, &dest_drive_location);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_location.bus,
                                                            dest_drive_location.enclosure,
                                                            dest_drive_location.slot,
                                                            &dest_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    mut_printf(MUT_LOG_TEST_STATUS, "==%s Start first user copy from: 0x%x(%d_%d_%d) to 0x%x(%d_%d_%d) ==",
               __FUNCTION__, src_object_id, 
               rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot,
               dest_object_id,
               dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot);

    /* Step 1: Start the first user copy to operation  
     */
    status = fbe_api_copy_to_replacement_disk(src_object_id, dest_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* Verify event log message for system initiated proactive copy operation  */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_USER_COPY_INITIATED,                 
                                          FBE_TRUE, /* Message contains spare drive location */   
                                          &spare_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]);                  
    
    /* wait for the proactive copy completion. */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* wait for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);
        
    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,                
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &spare_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]); 
                     
    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Step 2: Start the second user copy to operation.  Switch source and
     *         destination information.  
     */
    temp_object_id = src_object_id;
    src_object_id = dest_object_id;
    dest_object_id = temp_object_id;
    status = fbe_api_wait_for_object_lifecycle_state(dest_object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for object ready failed");
    status = fbe_api_provision_drive_get_location(dest_object_id, 
                                                  &dest_drive_location.bus, 
                                                  &dest_drive_location.enclosure, 
                                                  &dest_drive_location.slot);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* get the edge index of the source and destination drive based on configuration mode. */
    diabolicdiscdemon_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    mut_printf(MUT_LOG_TEST_STATUS, "==%s Start second user copy from: 0x%x(%d_%d_%d) to 0x%x(%d_%d_%d) ==",
               __FUNCTION__, src_object_id, 
               rg_config_p->rg_disk_set[drive_pos].bus, rg_config_p->rg_disk_set[drive_pos].enclosure, rg_config_p->rg_disk_set[drive_pos].slot,
               dest_object_id,
               dest_drive_location.bus, dest_drive_location.enclosure, dest_drive_location.slot);

    status = fbe_api_copy_to_replacement_disk(src_object_id, dest_object_id, &copy_status);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* wait for the proactive spare to swap-in. */
    diabolicdiscdemon_wait_for_proactive_spare_swap_in(vd_object_id, dest_edge_index, &spare_drive_location);

    /* Verify event log message for system initiated proactive copy operation  */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_USER_COPY_INITIATED,                 
                                          FBE_TRUE, /* Message contains spare drive location */   
                                          &spare_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]);                  
    
    /* wait for the proactive copy completion. */
    diabolicdiscdemon_wait_for_proactive_copy_completion(vd_object_id);

    /* wait for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_wait_for_source_drive_swap_out(vd_object_id, source_edge_index);
        
    /* Verify event log message for the swap-out of the drive after proactive copy completion. */
    diabolicdiscdemon_check_event_log_msg(SEP_INFO_SPARE_DRIVE_SWAP_OUT,                
                                          FBE_FALSE, /* Message contains spare drive location */   
                                          &spare_drive_location,                                  
                                          FBE_TRUE, /* Message contains original drive location */
                                          &rg_config_p->rg_disk_set[drive_pos]); 
                     
    /* Validate that the destination position is not marked NR */
    fbe_test_sep_rebuild_utils_check_bits_clear_for_position(vd_object_id, dest_edge_index);

    /* perform the i/o after the proactive copy to verify that proactive copy gets completed successfully. */
    diabolicdiscdemon_verify_background_pattern(rdgen_context_p, DIABOLICDISCDEMON_TEST_ELEMENT_SIZE);

    /* Refresh the raid group disk set information. */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "==%s - complete ==",__FUNCTION__);
    return;
}
/******************************************
 * end diabolicaldiscdemon_test_user_copy()
 ******************************************/

/*!****************************************************************************
 * diabolicdiscdemon_test_remove_hotspares_from_hotspare_pool()
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
static fbe_status_t diabolicdiscdemon_test_remove_hotspares_from_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                               fbe_u32_t num_of_spares_required)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_hs_configuration_t             hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t                               hs_index = 0;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_u32_t                               local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
  
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Discover all the `unused drives'
     */
    fbe_test_sep_util_get_all_unused_pvd_location(&diabolicaldiscdemon_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    diabolicaldiscdemon_set_unused_drive_info(rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
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
 * end diabolicdiscdemon_test_remove_hotspares_from_hotspare_pool()
 *******************************************************************/

/*!****************************************************************************
 * diabolicdiscdemon_test_add_hotspares_to_hotspare_pool()
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
static fbe_status_t diabolicdiscdemon_test_add_hotspares_to_hotspare_pool(fbe_test_rg_configuration_t *rg_config_p,
                                                                          fbe_u32_t num_of_spares_required)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_test_hs_configuration_t             hs_config[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};
    fbe_u32_t                               hs_index = 0;
    fbe_test_discovered_drive_locations_t   unused_pvd_locations;
    fbe_u32_t                               local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
  
    /* Refresh the location of the all drives in each raid group. 
     * This info can change as we swap in spares. 
     */
    fbe_test_sep_util_raid_group_refresh_disk_info(rg_config_p, 1);

    /* Discover all the `unused drives'
     */
    fbe_test_sep_util_get_all_unused_pvd_location(&diabolicaldiscdemon_discovered_drive_locations, &unused_pvd_locations);
    fbe_copy_memory(&local_drive_counts[0][0], &unused_pvd_locations.drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* This function will update unused drive info for those raid groups
     * which are going to be used in this pass.
     */
    diabolicaldiscdemon_set_unused_drive_info(rg_config_p,
                                              local_drive_counts,
                                              &unused_pvd_locations,
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

/*!***************************************************************************
 *          diabolicaldiscdemon_validate_rg_info()
 ***************************************************************************** 
 * 
 * @brief   Validate the rg info should include the source drive but not include the destination drive
 *             before the copy operation completes.
 *
 * @param   rg_id - raid group id
 * @param   src_pvd - source drive's pvd id
 * @param   dest_pvd - destination drive's pvd id
 *
 * @return none.
 *
 * @author
 *  5/21/2013 - Created. Zhipeng Hu
 *****************************************************************************/
static void diabolicaldiscdemon_validate_rg_info(fbe_object_id_t rg_id, fbe_object_id_t src_pvd, fbe_object_id_t dest_pvd)
{
    fbe_status_t status;
    fbe_u32_t	 index;
    fbe_database_raid_group_info_t	rg_info;
    fbe_bool_t	hit_src_drive = FBE_FALSE;
    fbe_bool_t	hit_dest_drive = FBE_FALSE;
    
    status = fbe_api_database_get_raid_group(rg_id, &rg_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for(index = 0; index < rg_info.pvd_count; index++)
    {
        if(src_pvd == rg_info.pvd_list[index])
        {
            hit_src_drive = FBE_TRUE;
        }
        else if(dest_pvd == rg_info.pvd_list[index])
        {
            hit_dest_drive = FBE_TRUE;
        }
    }

    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, hit_src_drive, "validate_rg_info: do not hit src drive in rg info, note expected");

    MUT_ASSERT_INT_NOT_EQUAL_MSG(FBE_TRUE, hit_dest_drive, "validate_rg_info: hit dest drive in rg info, not expected");

}


/*!**************************************************************
 * diabolicaldiscdemon_encrypted_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a Proactive Sparing test for encryption.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *     01/26/2014:  Created.
 *
 ****************************************************************/
void diabolicaldiscdemon_encrypted_dualsp_setup(void)
{
    fbe_status_t status;

    if (fbe_test_util_is_simulation())
    { 
        fbe_u32_t num_raid_groups;

        fbe_test_rg_configuration_t *rg_config_p = NULL;
        
        /* Randomize which raid type to use.
         */
        rg_config_p = &diabolicdiscdemon_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        diabolicaldiscdemon_set_num_of_extra_drives(rg_config_p);

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

        //fbe_api_enable_system_encryption();
        sep_config_load_kms_both_sps(NULL);

        status = fbe_test_sep_util_enable_kms_encryption();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* After sep is load setup notifications */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dualsp test */);

    }
    return;
}
/******************************************
 * end diabolicaldiscdemon_encrypted_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * diabolicaldiscdemon_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the diabolicaldiscdemon test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *     01/26/2014:  Created.
 *
 ****************************************************************/
void diabolicaldiscdemon_encrypted_dualsp_cleanup(void)
{
    fbe_test_common_cleanup_notifications(FBE_TRUE /* This is a dualsp test*/);

    fbe_test_sep_util_print_dps_statistics();

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_kms_both_sps();
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end diabolicaldiscdemon_dualsp_cleanup()
 ******************************************/

