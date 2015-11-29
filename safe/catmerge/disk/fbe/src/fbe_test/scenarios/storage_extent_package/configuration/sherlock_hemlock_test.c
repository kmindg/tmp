/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sherlock_hemlock_test.c
 ***************************************************************************
 *
 * @brief
 *  This test validates the proper handling of attempts to create raid groups
 *  that are not valid.  For instance a RAID-5 with a width of (17), etc.
 *
 * @version
 *  10/12/2009  Ron Proulx  - Created
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
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_event.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_parity.h"
#include "fbe/fbe_mirror.h"
#include "fbe/fbe_striper.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_event_log_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char *sherlock_hemlock_short_desc = "Test of invalid raid group creation";
char *sherlock_hemlock_long_desc =
    "\n"
    "\n"
    "The Sherlock Hemlock Test validates the handling of attempts to create raid groups that are not valid.\n"
    "\n"
    "This includes:\n"
    "    - Attempt to create a stipe raid groups (R0) with (17) drives.\n"
    "    - Attempt to create a mirror raid group (R1) with (4) drives.\n"
    "    - Attempt to create a parity raid groups (R5) with (2) drives.\n"
    "\n"
    "\n"
    "Dependencies:\n"
    "    - Job Control Service (JCS) API solidified\n"
    "    - JCS creation queue operational\n"
    "    - Database (DB) Service able to persist database both locally and on the peer\n"
    "    - Ability to create RG using fbe_api\n"
    "\n"
    "Starting Config:\n"
    "    [PP] 1 - armada board\n"
    "    [PP] 1 - SAS PMC port\n"
    "    [PP] 3 - viper enclosures\n"
    "    [PP] 33 - SAS drives (PDOs)\n"
    "    [PP] 33 - Logical drives (LDs)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "    - For each of the drives:(PDO1-PDO2-PDO3)\n"
    "        - Verify that LDO and PVD are both in the READY state\n"
    "\n" 
    "STEP 2: Create RAID-0 and verify.\n"
    "    - Create a RG (0) of type R0 with 17 drives.\n"
    "    - Verify that the RG creation fails with the expected results.\n"
    "\n"
    "STEP 3: Create RAID-1 and verify.\n"
    "    - Create a RG (1) of type R1 with 1 more than max required (4).\n"
    "    - Verify that the Job fails with invalid configuration and no RG is created.\n"
    "\n"
    "STEP 4: Create RAID-5 and verify.\n"
    "    - Create a RG (2) of type R5 with one less than min required (2).\n"
    "    - Verify that the Job fails with invalid configuration and no RG is created.\n"
    "\n"
    "Description last updated: 10/04/2011.\n"
    "\n"
    "\n";


/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID0_INVALID_MIN_WIDTH
 *********************************************************************
 * @brief Invalid minimum width for a RAID-0 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID0_INVALID_MIN_WIDTH    (0)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID0_INVALID_MAX_WIDTH
 *********************************************************************
 * @brief Invalid maximum width for a RAID-0 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID0_INVALID_MAX_WIDTH    (FBE_STRIPER_MAX_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID1_INVALID_MIN_WIDTH
 *********************************************************************
 * @brief Invalid minimum width for a RAID-1 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID1_INVALID_MIN_WIDTH    (FBE_MIRROR_MIN_WIDTH - 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID1_INVALID_MAX_WIDTH
 *********************************************************************
 * @brief Invalid maximum width for a RAID-1 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID1_INVALID_MAX_WIDTH    (FBE_MIRROR_MAX_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID5_INVALID_MIN_WIDTH
 *********************************************************************
 * @brief Invalid minimum width for a RAID-5 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID5_INVALID_MIN_WIDTH    (FBE_PARITY_R5_MIN_WIDTH - 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID5_INVALID_WIDTH
 *********************************************************************
 * @brief Invalid width for a RAID-5 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID5_INVALID_WIDTH        (FBE_PARITY_R5_MIN_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID5_INVALID_MAX_WIDTH
 *********************************************************************
 * @brief Invalid maximum width for a RAID-5 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID5_INVALID_MAX_WIDTH    (FBE_PARITY_MAX_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID3_INVALID_MIN_WIDTH
 *********************************************************************
 * @brief Invalid minimum width for a RAID-3 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID3_INVALID_MIN_WIDTH    (2)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID3_INVALID_WIDTH
 *********************************************************************
 * @brief Invalid width for a RAID-3 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID3_INVALID_WIDTH        (FBE_PARITY_R5_MIN_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID3_INVALID_MAX_WIDTH
 *********************************************************************
 * @brief Invalid maximum width for a RAID-3 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID3_INVALID_MAX_WIDTH    (FBE_PARITY_MAX_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID6_INVALID_MIN_WIDTH
 *********************************************************************
 * @brief Invalid minimum width for a RAID-6 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID6_INVALID_MIN_WIDTH    (FBE_PARITY_R6_MIN_WIDTH - 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID6_INVALID_WIDTH
 *********************************************************************
 * @brief Invalid width for a RAID-6 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID6_INVALID_WIDTH        (FBE_PARITY_R6_MIN_WIDTH + 1)
/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID6_INVALID_MAX_WIDTH
 *********************************************************************
 * @brief Invalid maximum width for a RAID-6 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID6_INVALID_MAX_WIDTH    (FBE_PARITY_MAX_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID10_INVALID_MIN_WIDTH
 *********************************************************************
 * @brief Invalid minimum width for a RAID-10 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID10_INVALID_MIN_WIDTH   (FBE_STRIPER_MIN_WIDTH)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID10_INVALID_WIDTH
 *********************************************************************
 * @brief Invalid width for a RAID-10 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID10_INVALID_WIDTH       (4 + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID10_INVALID_MAX_WIDTH
 *********************************************************************
 * @brief Invalid maximum width for a RAID-10 raid group
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID10_INVALID_MAX_WIDTH   (FBE_STRIPER_MAX_WIDTH + 1)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_RAID0_INVALID_CAPACITY
 *********************************************************************
 * @brief Invalid capacity for a single drive raid group.  Note that
 *        `FBE_LBA_INVALID' means `Use the entire capacity available'.
 *        Therefore we need to subtract some amount from that value.
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID0_INVALID_CAPACITY     (FBE_LBA_INVALID - (fbe_u64_t)0xFFFF)

/*!*******************************************************************
 * @def     SHERLOCK_HEMLOCK_RAID5_INVALID_CAPACITY
 *********************************************************************
 * @brief   This invalid capacity takes into account all the overhead
 *          associated with a parity raid group:
 *              o Default offset (0x10000)
 *              o Metadata capacity
 *              o Journal log capacity
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_RAID5_INVALID_CAPACITY     ((TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY - 0x20000) * 2)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_INVALID_RAID_TYPE
 *********************************************************************
 * @brief Invalid raid type
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_INVALID_RAID_TYPE          (FBE_RAID_GROUP_TYPE_UNKNOWN)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_NS_TIMEOUT                 25000 /*wait maximum of 25 seconds*/

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_LUNS_PER_RAID_GROUP        3

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_CHUNKS_PER_LUN             3

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_NUM_DRIVES_IN_FIRST_ENCLOSURE
 *********************************************************************
 * @brief   Number of drives in the first enclosure.
 *
 * @note    Must be at least (4) for system drives
 *********************************************************************/
#define SHERLOCK_HEMLOCK_NUM_DRIVES_IN_FIRST_ENCLOSURE  4

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_MAX_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_MAX_DRIVES_PER_ENCL        25

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_DRIVES_PER_ENCL
 *********************************************************************
 * @brief Allows us to change how many drives per enclosure we have.
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_DRIVES_PER_ENCL            (SHERLOCK_HEMLOCK_MAX_DRIVES_PER_ENCL)

/*!*******************************************************************
 * @def     SHERLOCK_HEMLOCK_B_IS_4160_BLOCK_SIZE_SUPPORTED
 *********************************************************************
 * @brief   Determines if 4160-bps drives are supported or not.
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_B_IS_4160_BLOCK_SIZE_SUPPORTED     (FBE_FALSE)     /*! @todo Need to add support for 4160-bps drives to provision drive. */

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 2 encl + + 4 pdo + 26 pdo)
 *
 *                                                      
 *
 *********************************************************************/
#define SHERLOCK_HEMLOCK_NUMBER_OF_PHYSICAL_OBJECTS (1 + 1 + 2 + \
                                                     (SHERLOCK_HEMLOCK_NUM_DRIVES_IN_FIRST_ENCLOSURE + \
                                                      (SHERLOCK_HEMLOCK_DRIVES_PER_ENCL - 1 + SHERLOCK_HEMLOCK_B_IS_4160_BLOCK_SIZE_SUPPORTED)))

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_GET_SAS_ENCLOSURE_SAS_ADDRESS
 *********************************************************************
 * @brief Default number of blocks to adjust capacity to.
 *********************************************************************/
#define  SHERLOCK_HEMLOCK_GET_SAS_ENCLOSURE_SAS_ADDRESS(BN, EN) \
    (0x5000097A79000000 + ((fbe_u64_t)(EN) << 16) + BN)

/*!*******************************************************************
 * @def SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT
 *********************************************************************
 * @brief Default number of blocks to adjust capacity to.
 *********************************************************************/
#define SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT 0x10000


/*!*******************************************************************
 * @enum    sherlock_hemlock_failure_type_t
 *********************************************************************
 * @brief   Defines the types of failures expected
 *
 *********************************************************************/
typedef enum sherlock_hemlock_failure_type_e
{
    SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID   = 0,
    SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,
    SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_CAPACITY,
    SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_RAID_TYPE,
    SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH,
    SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_FAIL,
    SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_EOL,

} sherlock_hemlock_failure_type_t;

/*!*******************************************************************
 * @struct  sherlock_hemlock_test_case_t
 *********************************************************************
 * @brief   Defines the test information for each raid group tested
 *
 * @note    These tests do not validate the notification data since
 *          the caller will look at the job error code.
 * 
 *********************************************************************/
typedef struct sherlock_hemlock_test_case_s
{
    sherlock_hemlock_failure_type_t failure_type;               /*! The type of failure to create */
    fbe_status_t                    expected_create_status;     /*! Expected create request status */
    fbe_status_t                    expected_job_status;        /*! Expected create job status */
    fbe_job_service_error_type_t    expected_job_service_error; /*! The expected job service error code */
    fbe_u32_t                       expected_event_code;        /*! Expected event code generated */

} sherlock_hemlock_test_case_t;


/*!*******************************************************************
 * @enum    sherlock_hemlock_drive_state_t
 *********************************************************************
 * @brief   Defines the drive state
 *
 *********************************************************************/
typedef enum sherlock_hemlock_drive_state_e
{
    SHERLOCK_HEMLOCK_DRIVE_STATE_READY  = 0,
    SHERLOCK_HEMLOCK_DRIVE_STATE_FAIL,
    SHERLOCK_HEMLOCK_DRIVE_STATE_EOL,

} sherlock_hemlock_drive_state_t;

/*!*******************************************************************
 * @struct  sherlock_hemlock_drive_info_t
 *********************************************************************
 * @brief   Defines the drive information for each drive configured
 *
 *********************************************************************/
typedef struct sherlock_hemlock_drive_info_s
{
    fbe_lba_t                       drive_capacity;
    fbe_sas_drive_type_t            drive_type;
    sherlock_hemlock_drive_state_t  drive_state;

} sherlock_hemlock_drive_info_t;

/*!*******************************************************************
 * @var     sherlock_hemlock_encl0_0_info
 *********************************************************************
 * @brief   This is the first enclosure that only contains the system
 *          drives.
 *
 * @note    Currently we do not use the system drives to test raid
 *          group creates.
 *********************************************************************/
static sherlock_hemlock_drive_info_t sherlock_hemlock_encl0_0_info[SHERLOCK_HEMLOCK_NUM_DRIVES_IN_FIRST_ENCLOSURE] = { 0 };

/*!*******************************************************************
 * @var     sherlock_hemlock_encl0_1_info
 *********************************************************************
 * @brief   This is the second enclosure that contains the drives that
 *          we will test.
 *
 * @note    Currently we do not use the system drives to test raid
 *          group creates.
 *********************************************************************/
static sherlock_hemlock_drive_info_t sherlock_hemlock_encl0_1_info[SHERLOCK_HEMLOCK_MAX_DRIVES_PER_ENCL] = { 0 };


/*************************    
 *   FUNCTION DEFINITIONS     
 *************************/   

/*!**************************************************************
 * sherlock_hemlock_physical_config_populate_drive_info()
 ****************************************************************
 * @brief
 *  Populate the array of drive capacitys for sherlock_hemlock test().
 *
 * @param None.              
 *
 * @return None.
 *
 * @author
 *  06/22/2012  Ron Proulx  - Created.
 ****************************************************************/
static void sherlock_hemlock_physical_config_populate_drive_info(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lba_t min_sd_cap  = TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT;
    fbe_lba_t min_nsd_cap = TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;
    fbe_lba_t std_nsd_cap = (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY + SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t sml_nsd_cap = std_nsd_cap - (1 * SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t med_nsd_cap = std_nsd_cap + (1 * SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t lrg_nsd_cap = std_nsd_cap + (2 * SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);
    fbe_lba_t big_nsd_cap = std_nsd_cap + (3 * SHERLOCK_HEMLOCK_DEFAULT_DRIVE_CAPACITY_ADJUSTMENT);

    /* Validate minimum system drive and non-system drive capacities. */
    if ((min_sd_cap == FBE_LBA_INVALID) ||
        (min_sd_cap <= min_nsd_cap    )    )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: unexpected system cap: 0x%llx", 
                   __FUNCTION__, min_sd_cap);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Validate minimum system drive and non-system drive capacities. */
    if ((min_nsd_cap == FBE_LBA_INVALID) ||
        (min_nsd_cap <= 0x10000    )        )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: unexpected non-sytem cap: 0x%llx", 
                   __FUNCTION__, min_nsd_cap);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Simply a visual way to see the capacities that we populate.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: system cap: 0x%llx standard cap: 0x%llx small cap: 0x%llx", 
               __FUNCTION__, min_sd_cap, std_nsd_cap, sml_nsd_cap);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s: medium cap: 0x%llx large cap: 0x%llx big cap: 0x%llx", 
               __FUNCTION__, med_nsd_cap, lrg_nsd_cap, big_nsd_cap);

/****Start System drives *****/
    /* 0_0_slot:                                        Capacity    Type                    State                               Location */
    sherlock_hemlock_encl0_0_info[ 0].drive_capacity = min_sd_cap;                                                              /* 0_0_0*/
    sherlock_hemlock_encl0_0_info[ 0].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_0_0*/
    sherlock_hemlock_encl0_0_info[ 0].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_0_0*/
      
    sherlock_hemlock_encl0_0_info[ 1].drive_capacity = min_sd_cap;                                                              /* 0_0_1*/ 
    sherlock_hemlock_encl0_0_info[ 1].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_0_1*/ 
    sherlock_hemlock_encl0_0_info[ 1].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_0_1*/ 

    sherlock_hemlock_encl0_0_info[ 2].drive_capacity = min_sd_cap;                                                              /* 0_0_2*/ 
    sherlock_hemlock_encl0_0_info[ 2].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_0_2*/ 
    sherlock_hemlock_encl0_0_info[ 2].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_0_2*/ 
      
    sherlock_hemlock_encl0_0_info[ 3].drive_capacity = min_sd_cap;                                                              /* 0_0_3*/ 
    sherlock_hemlock_encl0_0_info[ 3].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_0_3*/ 
    sherlock_hemlock_encl0_0_info[ 3].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_0_3*/ 
/****End System drives *****/

/****Start (17) drives that can bind a user LUN: 0_1_0 thru 0_1_16 */
    sherlock_hemlock_encl0_1_info[ 0].drive_capacity = std_nsd_cap;                                                             /* 0_1_0*/
    sherlock_hemlock_encl0_1_info[ 0].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_0*/
    sherlock_hemlock_encl0_1_info[ 0].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_0*/

    sherlock_hemlock_encl0_1_info[ 1].drive_capacity = std_nsd_cap;                                                             /* 0_1_1*/ 
    sherlock_hemlock_encl0_1_info[ 1].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_1*/ 
    sherlock_hemlock_encl0_1_info[ 1].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_1*/ 

    sherlock_hemlock_encl0_1_info[ 2].drive_capacity = std_nsd_cap;                                                             /* 0_1_2*/ 
    sherlock_hemlock_encl0_1_info[ 2].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_2*/ 
    sherlock_hemlock_encl0_1_info[ 2].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_2*/
     
    sherlock_hemlock_encl0_1_info[ 3].drive_capacity = std_nsd_cap;                                                             /* 0_1_3*/
    sherlock_hemlock_encl0_1_info[ 3].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_3*/
    sherlock_hemlock_encl0_1_info[ 3].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_3*/

    sherlock_hemlock_encl0_1_info[ 4].drive_capacity = std_nsd_cap;                                                             /* 0_1_4*/ 
    sherlock_hemlock_encl0_1_info[ 4].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_4*/ 
    sherlock_hemlock_encl0_1_info[ 4].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_4*/ 

    sherlock_hemlock_encl0_1_info[ 5].drive_capacity = std_nsd_cap;                                                             /* 0_1_5*/ 
    sherlock_hemlock_encl0_1_info[ 5].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_5*/ 
    sherlock_hemlock_encl0_1_info[ 5].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_5*/
     
    sherlock_hemlock_encl0_1_info[ 6].drive_capacity = std_nsd_cap;                                                             /* 0_1_6*/
    sherlock_hemlock_encl0_1_info[ 6].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_6*/
    sherlock_hemlock_encl0_1_info[ 6].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_6*/

    sherlock_hemlock_encl0_1_info[ 7].drive_capacity = std_nsd_cap;                                                             /* 0_1_7*/ 
    sherlock_hemlock_encl0_1_info[ 7].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_7*/ 
    sherlock_hemlock_encl0_1_info[ 7].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_7*/ 

    sherlock_hemlock_encl0_1_info[ 8].drive_capacity = std_nsd_cap;                                                             /* 0_1_8*/ 
    sherlock_hemlock_encl0_1_info[ 8].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_8*/ 
    sherlock_hemlock_encl0_1_info[ 8].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_8*/ 

    sherlock_hemlock_encl0_1_info[ 9].drive_capacity = std_nsd_cap;                                                             /* 0_1_9*/
    sherlock_hemlock_encl0_1_info[ 9].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_9*/
    sherlock_hemlock_encl0_1_info[ 9].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_9*/

    sherlock_hemlock_encl0_1_info[10].drive_capacity = std_nsd_cap;                                                             /* 0_1_10*/ 
    sherlock_hemlock_encl0_1_info[10].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_10*/ 
    sherlock_hemlock_encl0_1_info[10].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_10*/

    sherlock_hemlock_encl0_1_info[11].drive_capacity = std_nsd_cap;                                                             /* 0_1_11*/ 
    sherlock_hemlock_encl0_1_info[11].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_11*/ 
    sherlock_hemlock_encl0_1_info[11].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_11*/

    sherlock_hemlock_encl0_1_info[12].drive_capacity = std_nsd_cap;                                                             /* 0_1_12*/ 
    sherlock_hemlock_encl0_1_info[12].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_12*/ 
    sherlock_hemlock_encl0_1_info[12].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_12*/ 

    sherlock_hemlock_encl0_1_info[13].drive_capacity = std_nsd_cap;                                                             /* 0_1_13*/
    sherlock_hemlock_encl0_1_info[13].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_13*/
    sherlock_hemlock_encl0_1_info[13].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_13*/

    sherlock_hemlock_encl0_1_info[14].drive_capacity = std_nsd_cap;                                                             /* 0_1_14*/ 
    sherlock_hemlock_encl0_1_info[14].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_14*/ 
    sherlock_hemlock_encl0_1_info[14].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_14*/

    sherlock_hemlock_encl0_1_info[15].drive_capacity = std_nsd_cap;                                                             /* 0_1_15*/ 
    sherlock_hemlock_encl0_1_info[15].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_15*/ 
    sherlock_hemlock_encl0_1_info[15].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_15*/

    sherlock_hemlock_encl0_1_info[16].drive_capacity = std_nsd_cap;                                                             /* 0_1_16*/ 
    sherlock_hemlock_encl0_1_info[16].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_16*/ 
    sherlock_hemlock_encl0_1_info[16].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_16*/
/****End (17) drives that can bind a user LUN: 0_1_0 thru 0_1_16 */ 

/****Start random capacity, drive type and drive state*/
    /* too small */
    sherlock_hemlock_encl0_1_info[17].drive_capacity = sml_nsd_cap;                                                             /* 0_1_17*/ 
    sherlock_hemlock_encl0_1_info[17].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_17*/ 
    sherlock_hemlock_encl0_1_info[17].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_17*/

    /* drive not in the ready state */
    sherlock_hemlock_encl0_1_info[18].drive_capacity = std_nsd_cap;                                                             /* 0_1_18*/ 
    sherlock_hemlock_encl0_1_info[18].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_18*/ 
    sherlock_hemlock_encl0_1_info[18].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_FAIL;  /* 0_1_22*/ 

    /* EOL set */
    sherlock_hemlock_encl0_1_info[19].drive_capacity = std_nsd_cap;                                                             /* 0_1_19*/ 
    sherlock_hemlock_encl0_1_info[19].drive_type     =              FBE_SAS_DRIVE_SIM_520;                                      /* 0_1_19*/ 
    sherlock_hemlock_encl0_1_info[19].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_EOL;   /* 0_1_19*/

    /* wrong drive type - sas flash */
    sherlock_hemlock_encl0_1_info[20].drive_capacity = std_nsd_cap;                                                             /* 0_1_20*/
    sherlock_hemlock_encl0_1_info[20].drive_type     =              FBE_SAS_DRIVE_SIM_520_FLASH_HE;                                /* 0_1_20*/
    sherlock_hemlock_encl0_1_info[20].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_20*/ 

    /* wrong drive type - sas nl */
    sherlock_hemlock_encl0_1_info[21].drive_capacity = std_nsd_cap;                                                             /* 0_1_21*/ 
    sherlock_hemlock_encl0_1_info[21].drive_type     =              FBE_SAS_NL_DRIVE_SIM_520;                                   /* 0_1_21*/ 
    sherlock_hemlock_encl0_1_info[21].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_21*/

#if (SHERLOCK_HEMLOCK_B_IS_4160_BLOCK_SIZE_SUPPORTED == FBE_TRUE)
    /* wrong block size - 4160 */
    sherlock_hemlock_encl0_1_info[22].drive_capacity = std_nsd_cap;                                                             /* 0_1_22*/
    sherlock_hemlock_encl0_1_info[22].drive_type     =              FBE_SAS_DRIVE_UNICORN_4160;                                 /* 0_1_22*/
    sherlock_hemlock_encl0_1_info[22].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_22*/
#else
    /* wrong drive type - sas nl */
    sherlock_hemlock_encl0_1_info[22].drive_capacity = std_nsd_cap;                                                             /* 0_1_22*/
    sherlock_hemlock_encl0_1_info[22].drive_type     =              FBE_SAS_NL_DRIVE_SIM_520;                                   /* 0_1_22*/
    sherlock_hemlock_encl0_1_info[22].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_22*/
#endif
/****End random capacity, drive type and drive state*/

/****Start sas flash raid group with (1) drive too small */
    sherlock_hemlock_encl0_1_info[23].drive_capacity = std_nsd_cap;                                                             /* 0_1_23*/ 
    sherlock_hemlock_encl0_1_info[23].drive_type     =              FBE_SAS_DRIVE_SIM_520_FLASH_HE;                                /* 0_1_23*/ 
    sherlock_hemlock_encl0_1_info[23].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_23*/
     
    /* too small */
    sherlock_hemlock_encl0_1_info[24].drive_capacity = sml_nsd_cap;                                                             /* 0_1_24*/ 
    sherlock_hemlock_encl0_1_info[24].drive_type     =              FBE_SAS_DRIVE_SIM_520_FLASH_HE;                                /* 0_1_24*/ 
    sherlock_hemlock_encl0_1_info[24].drive_state    =                                      SHERLOCK_HEMLOCK_DRIVE_STATE_READY; /* 0_1_24*/ 
/****End sas flash raid group with (1) drive too small */

/****Start sas drives available for future use (must change capacity, drive type and state as required)*/

/****End sas drives available for future use */

    MUT_ASSERT_TRUE((24 + 1) <= SHERLOCK_HEMLOCK_DRIVES_PER_ENCL);

    return;
}
/************************************************************
 * end sherlock_hemlock_physical_config_populate_drive_info()
 ************************************************************/

/*!**************************************************************
 * sherlock_hemlock_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the sherlock hemlock test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 * 
 * @note    Number of objects we will create.
 *        (1 board + 1 port + 1 encl + 25 pdo ) = SHERLOCK_HEMLOCK_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 * 
 * @author
 *  01/10/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
static void sherlock_hemlock_load_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_boards = 0;
    fbe_u32_t                           total_ports = 0;
    fbe_u32_t                           total_enclosures = 0;
    fbe_u32_t                           total_pdos = 0;
    fbe_u32_t                           total_expected_objects = 0; 
    fbe_u32_t                           actual_total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port0_handle;
    fbe_u8_t uid[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE] = {0};
    fbe_sas_address_t                   new_sas_address;
    fbe_api_terminator_device_handle_t  encl0_0_handle;
    fbe_api_terminator_device_handle_t  encl0_1_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t                           slot;
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    total_boards++;

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port0_handle);
    total_ports += 1;

    /* We use enclosure 0 for the system drives which are currently not used
     * for this test
     */
    uid[0] = 0;
    new_sas_address = SHERLOCK_HEMLOCK_GET_SAS_ENCLOSURE_SAS_ADDRESS(0, 0);
    status = fbe_test_pp_util_insert_sas_enclosure(0, 0,
                                                   uid,
                                                   new_sas_address,
                                                   FBE_SAS_ENCLOSURE_TYPE_VIPER,
                                                   port0_handle,
                                                   &encl0_0_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    total_enclosures += 1;

    /* We use enclosure 1 for all the drives under test
     */
    uid[0] = 1;
    new_sas_address = SHERLOCK_HEMLOCK_GET_SAS_ENCLOSURE_SAS_ADDRESS(0, 1);
    status = fbe_test_pp_util_insert_sas_enclosure(0, 1,
                                                   uid,
                                                   new_sas_address,
                                                   FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
                                                   port0_handle,
                                                   &encl0_1_handle);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    total_enclosures += 1;

    /* Initialize the drive capacities. */
    sherlock_hemlock_physical_config_populate_drive_info();

    /* Insert drives for system enclosure
     */
    for (slot = 0; slot < SHERLOCK_HEMLOCK_NUM_DRIVES_IN_FIRST_ENCLOSURE; slot++)
    {
        /* Use the proper method to insert the drive
         */
        switch(sherlock_hemlock_encl0_0_info[slot].drive_type)
        {
            case FBE_SAS_DRIVE_SIM_520:
                status = fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, sherlock_hemlock_encl0_0_info[slot].drive_capacity, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            case FBE_SATA_DRIVE_SIM_512:
                /*! @note SATA and 512-bps no longer supported !
                 *
                 */
                MUT_ASSERT_TRUE_MSG(512 == 520, "SATA and drive block sizes other than 520 or 4160 are not supported");
                break;

            case FBE_SAS_NL_DRIVE_SIM_520:
                /*! @note Over-ride default drive type.
                 */
                new_sas_address = fbe_test_pp_util_get_unique_sas_address();
                status = fbe_test_pp_util_insert_sas_drive_extend(0, 0, slot, 520, sherlock_hemlock_encl0_0_info[slot].drive_capacity,
                                                         new_sas_address, FBE_SAS_NL_DRIVE_SIM_520, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            case FBE_SAS_DRIVE_SIM_520_FLASH_HE:
                status = fbe_test_pp_util_insert_sas_flash_drive(0, 0, slot, 520, sherlock_hemlock_encl0_0_info[slot].drive_capacity, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            case FBE_SAS_DRIVE_UNICORN_4160:
                /*! @note Over-ride default drive type.
                 */
                new_sas_address = fbe_test_pp_util_get_unique_sas_address();
                status = fbe_test_pp_util_insert_sas_drive_extend(0, 0, slot, 4160, sherlock_hemlock_encl0_0_info[slot].drive_capacity,
                                                         new_sas_address, FBE_SAS_DRIVE_UNICORN_4160, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported drive type: %d ", 
                           __FUNCTION__, sherlock_hemlock_encl0_0_info[slot].drive_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return;
        }
    }

    /* Insert drives for test enclosure
     */
    for (slot = 0; slot < SHERLOCK_HEMLOCK_DRIVES_PER_ENCL; slot++)
    {
        /* Use the proper method to insert the drive
         */
        switch(sherlock_hemlock_encl0_1_info[slot].drive_type)
        {
            case FBE_SAS_DRIVE_SIM_520:
                status = fbe_test_pp_util_insert_sas_drive(0, 1, slot, 520, sherlock_hemlock_encl0_1_info[slot].drive_capacity, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            case FBE_SATA_DRIVE_SIM_512:
                /*! @note SATA and 512-bps no longer supported !
                 *
                 */
                MUT_ASSERT_TRUE_MSG(512 == 520, "SATA and drive block sizes other than 520 or 4160 are not supported");
                break;

            case FBE_SAS_NL_DRIVE_SIM_520:
                /*! @note Over-ride default drive type.
                 */
                new_sas_address = fbe_test_pp_util_get_unique_sas_address();
                status = fbe_test_pp_util_insert_sas_drive_extend(0, 1, slot, 520, sherlock_hemlock_encl0_1_info[slot].drive_capacity,
                                                         new_sas_address, FBE_SAS_NL_DRIVE_SIM_520, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            case FBE_SAS_DRIVE_SIM_520_FLASH_HE:
                status = fbe_test_pp_util_insert_sas_flash_drive(0, 1, slot, 520, sherlock_hemlock_encl0_1_info[slot].drive_capacity, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            case FBE_SAS_DRIVE_UNICORN_4160:
                /*! @note Over-ride default drive type.
                 */
                new_sas_address = fbe_test_pp_util_get_unique_sas_address();
                status = fbe_test_pp_util_insert_sas_drive_extend(0, 1, slot, 4160, sherlock_hemlock_encl0_1_info[slot].drive_capacity,
                                                         new_sas_address, FBE_SAS_DRIVE_UNICORN_4160, &drive_handle);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                total_pdos += 1;
                break;

            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s: Unsupported drive type: %d ", 
                           __FUNCTION__, sherlock_hemlock_encl0_1_info[slot].drive_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return;
        }
    }

    /* validate the total_expected objects against the previously calculated constant */
    total_expected_objects = total_boards + total_ports + total_enclosures + total_pdos;
    if (total_expected_objects != SHERLOCK_HEMLOCK_NUMBER_OF_PHYSICAL_OBJECTS)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Actual total: %d doesnt match expected: %d change SHERLOCK_HEMLOCK_NUMBER_OF_PHYSICAL_OBJECTS",
                   __FUNCTION__, total_expected_objects, SHERLOCK_HEMLOCK_NUMBER_OF_PHYSICAL_OBJECTS);
        mut_printf(MUT_LOG_TEST_STATUS, "=== Actual boards: %d ports: %d enclosures: %d pdos: %d",
                   total_boards, total_ports, total_enclosures, total_pdos);
    }

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the expected number of objects */
    mut_printf(MUT_LOG_LOW, "=== %s waiting for %d physical objects start ===",__FUNCTION__, total_expected_objects);
    status = fbe_api_wait_for_number_of_objects(total_expected_objects, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&actual_total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(actual_total_objects == total_expected_objects);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    return;
}
/**********************************************
 * end sherlock_hemlock_load_physical_config()
 **********************************************/

/*!*******************************************************************
 * @var     sherlock_hemlock_test_case_qual
 *********************************************************************
 * @brief   Expected job status and event code
 *
 *********************************************************************/
sherlock_hemlock_test_case_t sherlock_hemlock_test_case_qual[] = 
{
    /* Failure Type                                  Create Status  Job status                  Job service error code                       expected event code generated                           */
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,    FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT,   SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,    FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION, SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,    FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION, SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH, FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_DRIVE_TYPES, SEP_ERROR_CREATE_RAID_GROUP_INCOMPATIBLE_DRIVE_TYPES},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_CAPACITY, FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY, SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
};

/*!*******************************************************************
 * @var sherlock_hemlock_raid_group_invalid_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t sherlock_hemlock_raid_group_invalid_config_qual[] = 
{
    /* width,                                   capacity, raid type,                 class,                block size, RAID-id, bandwidth */
    {SHERLOCK_HEMLOCK_RAID0_INVALID_MAX_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID0, FBE_CLASS_ID_STRIPER, 520,        0,       0},
    {SHERLOCK_HEMLOCK_RAID1_INVALID_MAX_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR,  520,        1,       0},
    {SHERLOCK_HEMLOCK_RAID5_INVALID_MIN_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,  520,        2,       0},
    {4,                                         0xE000,   FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY,  520,        3,       0},
    {3,        SHERLOCK_HEMLOCK_RAID5_INVALID_CAPACITY,   FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,  520,        4,       0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*!*******************************************************************
 * @var     sherlock_hemlock_test_case_extended
 *********************************************************************
 * @brief   Expected job status and event code
 *
 *********************************************************************/
sherlock_hemlock_test_case_t sherlock_hemlock_test_case_extended[] = 
{
    /* test case failure type                         Create Status  Job status                  Job service error code                                     expected event code generated                           */
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,     FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT,                 SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,     FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION,               SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,     FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION,               SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,     FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT,                 SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,     FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION,               SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,     FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT,                 SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH,     FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION,               SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_CAPACITY,  FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY,  SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_RAID_TYPE, FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_TYPE,             SEP_ERROR_CREATE_RAID_GROUP_INVALID_RAID_TYPE},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH, FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_DRIVE_TYPES,          SEP_ERROR_CREATE_RAID_GROUP_INCOMPATIBLE_DRIVE_TYPES},
    {SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_CAPACITY,  FBE_STATUS_OK, FBE_STATUS_GENERIC_FAILURE, FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY,  SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX},
};

/*!*******************************************************************
 * @var sherlock_hemlock_raid_group_invalid_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t sherlock_hemlock_raid_group_invalid_config_extended[] = 
{
    /* width,                                   capacity  raid type,                 class,               block size, RAID-id, bandwidth */
    {SHERLOCK_HEMLOCK_RAID0_INVALID_MAX_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID0, FBE_CLASS_ID_STRIPER, 520,        0,       0},
    {SHERLOCK_HEMLOCK_RAID1_INVALID_MAX_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR,  520,        1,       0},
    {SHERLOCK_HEMLOCK_RAID5_INVALID_MIN_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,  520,        2,       0},
    {SHERLOCK_HEMLOCK_RAID5_INVALID_MAX_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,  520,        3,       0},
    {SHERLOCK_HEMLOCK_RAID6_INVALID_WIDTH,      0xE000,   FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY,  520,        4,       0},
    {SHERLOCK_HEMLOCK_RAID3_INVALID_MAX_WIDTH,  0xE000,   FBE_RAID_GROUP_TYPE_RAID3, FBE_CLASS_ID_PARITY,  520,        5,       0},
    {SHERLOCK_HEMLOCK_RAID10_INVALID_WIDTH,     0xE000,   FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER, 520,       6,       0},
    {1,        SHERLOCK_HEMLOCK_RAID0_INVALID_CAPACITY,   FBE_RAID_GROUP_TYPE_RAID0, FBE_CLASS_ID_STRIPER, 520,        7,       0},
    {2,                                         0xE000,   SHERLOCK_HEMLOCK_INVALID_RAID_TYPE, FBE_CLASS_ID_MIRROR, 520, 8,      0},
    {4,                                         0xE000,   FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY,  520,        9,       0},
    {3,        SHERLOCK_HEMLOCK_RAID5_INVALID_CAPACITY,   FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY,  520,       10,       0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!***************************************************************************
 *          sherlock_hemlock_find_drive_with_failure_type()
 *****************************************************************************
 *
 * @brief   Based on the type of error expected, locate (1) drive that meets 
 *          the criteria.  Then place that drive into the state required.
 *
 * @param   failure_type - The drive failure type to locate
 * @param   original_drive_location_p - Pointer to drive location for the drive 
 *              to be replaced.
 * @param   failed_drive_location_p - Pointer to drive location for drive 
 *              failure type expected.               
 *
 * @return  status
 *
 * @author
 *  02/26/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t sherlock_hemlock_find_drive_with_failure_type(sherlock_hemlock_failure_type_t  failure_type,
                                                                  fbe_test_raid_group_disk_set_t  *original_drive_location_p,
                                                                  fbe_test_raid_group_disk_set_t  *failed_drive_location_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       bus = 0;        /* Location of drives is 0_1_xx */
    fbe_u32_t                       enclosure = 1;  /* Location of drives is 0_1_xx */
    fbe_u32_t                       slot = 0;
    fbe_bool_t                      b_orig_drive_found = FBE_FALSE;
    sherlock_hemlock_drive_info_t  *orig_drive_info_p = NULL;
    fbe_bool_t                      b_failure_drive_found = FBE_FALSE;
    sherlock_hemlock_drive_info_t  *drive_info_p = NULL;
    fbe_object_id_t                 failed_drive_object_id = FBE_OBJECT_ID_INVALID;

    /* We only support the following drive failure types
     */
    switch(failure_type)
    {
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_FAIL:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_EOL:
            /* Currently only the above error types are supported
             */
            break;
        
        default:
            /* Unsupported
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s Unsupported drive failure type: %d",
                       __FUNCTION__, failure_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Walk the array of drives and locate the original drive inforation
     */
    for (slot = 0; slot < SHERLOCK_HEMLOCK_DRIVES_PER_ENCL; slot++)
    {
        orig_drive_info_p = &sherlock_hemlock_encl0_1_info[slot];

        /* Check for a match
         */
        if ((original_drive_location_p->bus == bus)             &&
            (original_drive_location_p->enclosure == enclosure) &&
            (original_drive_location_p->slot == slot)              )
        {
            /* Found the orignal
             */
            b_orig_drive_found = FBE_TRUE;
            break;
        }
    }

    /* If the original drive was not found report the failure
     */
    if (b_orig_drive_found == FBE_FALSE)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s failure type: %d could not locate original drive: %d_%d_%d",
                   __FUNCTION__, failure_type, original_drive_location_p->bus,
                   original_drive_location_p->enclosure,
                   original_drive_location_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Walk the array of drives and locate the failure type requested
     */
    for (slot = 0; 
         (slot < SHERLOCK_HEMLOCK_DRIVES_PER_ENCL) && (b_failure_drive_found == FBE_FALSE);
         slot++)
    {
        drive_info_p = &sherlock_hemlock_encl0_1_info[slot];

        /* Check for the error type requested.
         */
        switch(failure_type)
        {
            case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH:
                /* Check if the drive is in the Ready state but the type does
                 * does not match the original.
                 */
                if ((drive_info_p->drive_type != orig_drive_info_p->drive_type)       &&
                    (drive_info_p->drive_state == SHERLOCK_HEMLOCK_DRIVE_STATE_READY)    )
                {
                    /* Found a drive type that does match remainder of raid group.
                     */
                    failed_drive_location_p->bus = bus;
                    failed_drive_location_p->enclosure = enclosure;
                    failed_drive_location_p->slot = slot;
                    b_failure_drive_found = FBE_TRUE;
                }
                break;

            case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_FAIL:
                /* Locate a drive that should be in the failed state
                 */
                if ((drive_info_p->drive_type == orig_drive_info_p->drive_type)      &&
                    (drive_info_p->drive_state == SHERLOCK_HEMLOCK_DRIVE_STATE_FAIL)    )
                {
                    /* Place the drive into the failed state
                     */
                    failed_drive_location_p->bus = bus;
                    failed_drive_location_p->enclosure = enclosure;
                    failed_drive_location_p->slot = slot;
                    b_failure_drive_found = FBE_TRUE;
                    status = fbe_test_sep_util_set_drive_fault(bus, enclosure, slot);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                }
                break;

            case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_EOL:
                /* Locate a drive that should be marked EOL
                 */
                if ((drive_info_p->drive_type == orig_drive_info_p->drive_type)     &&
                    (drive_info_p->drive_state == SHERLOCK_HEMLOCK_DRIVE_STATE_EOL)    )
                {
                    /* Mark the PVD EOL
                     */
                    failed_drive_location_p->bus = bus;
                    failed_drive_location_p->enclosure = enclosure;
                    failed_drive_location_p->slot = slot;
                    b_failure_drive_found = FBE_TRUE;
                    status = fbe_api_provision_drive_get_obj_id_by_location(bus,
                                                                            enclosure,
                                                                            slot,
                                                                            &failed_drive_object_id);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    status = fbe_api_provision_drive_set_eol_state(failed_drive_object_id, FBE_PACKET_FLAG_NO_ATTRIB);  
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
                break;

            default:
                /* Unsupported
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s Unsupported drive failure type: %d slot: %d",
                           __FUNCTION__, failure_type, slot);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
        }

    } /* end for all drives and not found */

    /* If we could not locate the drive report the failure.
     */
    if (b_failure_drive_found == FBE_FALSE)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s failure type: %d could not locate failed drive for original: %d_%d_%d",
                   __FUNCTION__, failure_type, original_drive_location_p->bus,
                   original_drive_location_p->enclosure,
                   original_drive_location_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
        return status;
    }

    /* Return the status
     */
    return status;
}
/*****************************************************
 * end sherlock_hemlock_find_drive_with_failure_type()
 *****************************************************/

/*!***************************************************************************
 *          sherlock_hemlock_cleanup_drive_with_failure_type()
 *****************************************************************************
 *
 * @brief   Based on the type of error cleanup any inserted errors (i.e. place
 *          the drive backinto the Ready state.
 *
 * @param   failure_type - The drive failure type to locate
 * @param   failed_drive_location_p - Pointer to drive location for drive 
 *              failure type expected.  
 *
 * @return  status
 *
 * @author
 *  02/26/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t sherlock_hemlock_cleanup_drive_with_failure_type(sherlock_hemlock_failure_type_t  failure_type,
                                                                     fbe_test_raid_group_disk_set_t  *failed_drive_location_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       bus = 0;        /* Location of drives is 0_1_xx */
    fbe_u32_t                       enclosure = 1;  /* Location of drives is 0_1_xx */
    sherlock_hemlock_drive_info_t  *drive_info_p = NULL;

    /* We only support the following drive failure types
     */
    switch(failure_type)
    {
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_FAIL:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_EOL:
            /* Currently only the above error types are supported
             */
            break;
        
        default:
            /* Unsupported
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s Unsupported drive failure type: %d",
                       __FUNCTION__, failure_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Validate the failed drive location
     */
    if ((failed_drive_location_p->bus != bus)                               ||
        (failed_drive_location_p->enclosure != enclosure)                   ||
        (failed_drive_location_p->slot >= SHERLOCK_HEMLOCK_DRIVES_PER_ENCL)    )
    {
        /* Invalid location
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s failure type: %d invalid failed drive location: %d_%d_%d",
                   __FUNCTION__, failure_type, failed_drive_location_p->bus,
                   failed_drive_location_p->enclosure,
                   failed_drive_location_p->slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Get the drive information
     */
    drive_info_p = &sherlock_hemlock_encl0_1_info[failed_drive_location_p->slot];

    /* If the failure type doesn't match something went wrong
     */
    switch(failure_type)
    {
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH:
            /* Validate that the drive is in the Ready state and
             */
            if (drive_info_p->drive_state != SHERLOCK_HEMLOCK_DRIVE_STATE_READY)
            {
                /* Drive state should be Ready
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s failure type: %d failed drive location: %d_%d_%d is in: %d state (not Ready)",
                           __FUNCTION__, failure_type, failed_drive_location_p->bus,
                           failed_drive_location_p->enclosure,
                           failed_drive_location_p->slot,
                           drive_info_p->drive_state);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
            break;

        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_FAIL:
            /* Drive state should be failed
             */
            if (drive_info_p->drive_state != SHERLOCK_HEMLOCK_DRIVE_STATE_FAIL)
            {
                /* Drive state should be failed
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s failure type: %d failed drive location: %d_%d_%d is in: %d state (not Failed)",
                           __FUNCTION__, failure_type, failed_drive_location_p->bus,
                           failed_drive_location_p->enclosure,
                           failed_drive_location_p->slot,
                           drive_info_p->drive_state);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }

            /* Now clear drive fault
             */
            status = fbe_test_sep_util_clear_drive_fault(failed_drive_location_p->bus, 
                                                         failed_drive_location_p->enclosure, 
                                                         failed_drive_location_p->slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_EOL:
            /* Drive state should be EOL
             */
            if (drive_info_p->drive_state != SHERLOCK_HEMLOCK_DRIVE_STATE_EOL)
            {
                /* Drive state should be EOL
                 */
                /* Drive state should be failed
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "%s failure type: %d failed drive location: %d_%d_%d is in: %d state (not Failed)",
                           __FUNCTION__, failure_type, failed_drive_location_p->bus,
                           failed_drive_location_p->enclosure,
                           failed_drive_location_p->slot,
                           drive_info_p->drive_state);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }

            /* Now clear EOL
             */
            status = fbe_test_sep_util_clear_disk_eol(failed_drive_location_p->bus,
                                                      failed_drive_location_p->enclosure,
                                                      failed_drive_location_p->slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* Unsupported
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s Unsupported drive failure type: %d slot: %d",
                       __FUNCTION__, failure_type, failed_drive_location_p->slot);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;

    } /* end based on drive failure type */

    /* Return the status
     */
    return status;
}
/********************************************************
 * end sherlock_hemlock_cleanup_drive_with_failure_type()
 ********************************************************/

/*!***************************************************************************
 *          sherlock_hemlock_setup_raid_group_for_error_case()
 *****************************************************************************
 *
 * @brief   Based on the type of error expected we will place the drives, etc
 *          into the proper state.
 *
 * @param   rg_config_p - Pointer to raid group configuration to test  
 * @param   test_case_p - Pointer to test case for this raid group  
 * @param   original_drive_location_p - Pointer to original drive location               
 * @param   failed_drive_location_p - Pointer to original drive location               
 *
 * @return  status - status != FBE_STATUS_OK expected!!
 *
 * @author
 *  02/26/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t sherlock_hemlock_setup_raid_group_for_error_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                     sherlock_hemlock_test_case_t *test_case_p,
                                                                     fbe_test_raid_group_disk_set_t *original_drive_location_p,
                                                                     fbe_test_raid_group_disk_set_t *failed_drive_location_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Switch on the error type.
     */
    switch(test_case_p->failure_type)
    {
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_CAPACITY:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_RAID_TYPE:
            /* Nothing to do */
            *failed_drive_location_p = *original_drive_location_p;
            break;

        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_EOL:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_FAIL:
            /* Invoke the method to replace the first drive of the raid group
             * with one that creates the failure case.
             */
            status = sherlock_hemlock_find_drive_with_failure_type(test_case_p->failure_type,
                                                                   original_drive_location_p,
                                                                   failed_drive_location_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Now replace the failed drive location with the failed drive
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "== sherlock hemlock setup for raid group id: %d failure type: %d ==",
                       rg_config_p->raid_group_id, test_case_p->failure_type);
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "== sherlock hemlock setup for failure type: %d replace drive: %d_%d_%d with: %d_%d_%d ==",
                       test_case_p->failure_type,
                       original_drive_location_p->bus,
                       original_drive_location_p->enclosure,
                       original_drive_location_p->slot,
                       failed_drive_location_p->bus,
                       failed_drive_location_p->enclosure,
                       failed_drive_location_p->slot);
            break;

        default:
            /* Unsupported failure type
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s raid group id: %d failure type: %d unsupported",
                       __FUNCTION__, rg_config_p->raid_group_id, test_case_p->failure_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Print some info.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s raid group id: %d failure type: %d success",
               __FUNCTION__, rg_config_p->raid_group_id, test_case_p->failure_type);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/********************************************************
 * end sherlock_hemlock_setup_raid_group_for_error_case()
 ********************************************************/

/*!***************************************************************************
 *          sherlock_hemlock_cleanup_raid_group_for_error_case()
 *****************************************************************************
 *
 * @brief   Based on the type of error expected we will place the drives, etc
 *          cleanup after the failure case.
 *
 * @param   rg_config_p - Pointer to raid group configuration to test  
 * @param   test_case_p - Pointer to test case for this raid group 
 * @param   original_drive_location_p - Pointer to original drive location               
 * @param   failed_drive_location_p - Pointer to original drive location                   
 *
 * @return  status - status != FBE_STATUS_OK expected!!
 *
 * @author
 *  02/26/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t sherlock_hemlock_cleanup_raid_group_for_error_case(fbe_test_rg_configuration_t *rg_config_p,
                                                                       sherlock_hemlock_test_case_t *test_case_p,
                                                                       fbe_test_raid_group_disk_set_t *original_drive_location_p,
                                                                       fbe_test_raid_group_disk_set_t *failed_drive_location_p)

{
    fbe_status_t                    status = FBE_STATUS_OK;

    /* Switch on the error type.
     */
    switch(test_case_p->failure_type)
    {
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_WIDTH:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_CAPACITY:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_INVALID_RAID_TYPE:
            /* Nothing to do */
            break;

        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_TYPE_MISMATCH:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_EOL:
        case SHERLOCK_HEMLOCK_FAILURE_TYPE_DRIVE_FAIL:
            /* Invoke the method to replace the first drive of the raid group
             * with one that creates the failure case.
             */
            status = sherlock_hemlock_cleanup_drive_with_failure_type(test_case_p->failure_type,
                                                                      failed_drive_location_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Now replace the failed position with the original disk
             */
            /* Now replace the failed drive location with the failed drive
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "==sherlock hemlock cleanup for raid group id: %d failure type: %d ==",
                       rg_config_p->raid_group_id, test_case_p->failure_type);
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "==sherlock hemlock cleanup for failure type: %d change drive: %d_%d_%d to: %d_%d_%d ==",
                       test_case_p->failure_type,
                       failed_drive_location_p->bus,
                       failed_drive_location_p->enclosure,
                       failed_drive_location_p->slot,
                       original_drive_location_p->bus,
                       original_drive_location_p->enclosure,
                       original_drive_location_p->slot);
            break;

        default:
            /* Unsupported failure type
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s raid group id: %d failure type: %d unsupported",
                       __FUNCTION__, rg_config_p->raid_group_id, test_case_p->failure_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Print some info.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "%s raid group id: %d failure type: %d success",
               __FUNCTION__, rg_config_p->raid_group_id, test_case_p->failure_type);

    /* Return success
     */
    return FBE_STATUS_OK;
}
/********************************************************
 * end sherlock_hemlock_cleanup_raid_group_for_error_case()
 ********************************************************/

/*!***************************************************************************
 * sherlock_hemlock_create_raid_group()
 *****************************************************************************
 *
 * @brief   Create a single raid group.  Expectation is that the raid group
 *          creation will fail.  If it succeeds its an error.
 *
 * @param   rg_config_p - Pointer to raid group configuration to test
 * @param   test_case_p - Pointer to test case for this raid group                 
 *
 * @return  status - status != FBE_STATUS_OK expected!!
 *
 * @author
 *  02/22/2013  Ron Proulx  - Updated
 *
 *****************************************************************************/
static fbe_status_t sherlock_hemlock_create_raid_group(fbe_test_rg_configuration_t *rg_config_p,
                                                       sherlock_hemlock_test_case_t *test_case_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_job_service_raid_group_create_t fbe_raid_group_create_req;
    fbe_u32_t                               position = 0;
    const fbe_char_t                       *raid_type_string_p = NULL;
    fbe_u32_t                               valid_width;

    /*! @note Do not exceed maximum number of positions
     */
    valid_width = FBE_MIN(FBE_RAID_MAX_DISK_ARRAY_WIDTH, rg_config_p->width);

    /*! @note We only perform the minimum validations.  Those are to
     *        prevent un-expected errors (access violation etc).
     */
    MUT_ASSERT_NOT_NULL_MSG(rg_config_p, "User has provided RAID group configuration as NULL.");
    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);

    /* Display info
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Create a RAID Group with RAID group ID: %d type: %s width: %d", 
               rg_config_p->raid_group_id, raid_type_string_p,
               rg_config_p->width);

    /* Create a RAID group object with RAID group create API. 
     */
    fbe_zero_memory(&fbe_raid_group_create_req, sizeof(fbe_api_job_service_raid_group_create_t));
    fbe_raid_group_create_req.b_bandwidth = rg_config_p->b_bandwidth;
    fbe_raid_group_create_req.capacity = rg_config_p->capacity;
    //fbe_raid_group_create_req.explicit_removal = 0;
    fbe_raid_group_create_req.power_saving_idle_time_in_seconds = FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME;
    fbe_raid_group_create_req.is_private = FBE_TRI_FALSE;
    fbe_raid_group_create_req.max_raid_latency_time_is_sec = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    fbe_raid_group_create_req.raid_group_id = rg_config_p->raid_group_id;
    fbe_raid_group_create_req.raid_type = rg_config_p->raid_type;
    fbe_raid_group_create_req.drive_count = rg_config_p->width;
    fbe_raid_group_create_req.job_number = 0;
    fbe_raid_group_create_req.wait_ready  = FBE_FALSE;
    fbe_raid_group_create_req.ready_timeout_msec = SHERLOCK_HEMLOCK_NS_TIMEOUT;

    /* Fill up the drive information for the RAID group. */
    for (position = 0; position < valid_width; ++position)
    {
        fbe_raid_group_create_req.disk_array[position].bus = rg_config_p->rg_disk_set[position].bus;
        fbe_raid_group_create_req.disk_array[position].enclosure = rg_config_p->rg_disk_set[position].enclosure;
        fbe_raid_group_create_req.disk_array[position].slot = rg_config_p->rg_disk_set[position].slot;
    }

    /*! @note Create a RAID group using Job service API to create a RAID group. 
     *        Since the creation is asynchronous we expect the creation `request'
     *        to succeed!!
    */
    status = fbe_api_job_service_raid_group_create(&fbe_raid_group_create_req);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group create job sent successfully! ==");
    mut_printf(MUT_LOG_TEST_STATUS, "Job number 0x%llX", fbe_raid_group_create_req.job_number);

    /* update the job number for the */
    rg_config_p->job_number = fbe_raid_group_create_req.job_number;

    return status;
}
/********************************************
 * end sherlock_hemlock_create_raid_group()
 ********************************************/

/*!**************************************************************
 * sherlock_hemlock_get_number_of_test_cases()
 ****************************************************************
 * @brief
 *  Get the number of raid_groups in the array.
 *  Loops until it hits the terminator.
 *
 * @param config_p - array of raid_groups.               
 *
 * @return fbe_u32_t number of raid_groups.  
 *
 ****************************************************************/
static fbe_u32_t sherlock_hemlock_get_number_of_test_cases(sherlock_hemlock_test_case_t *test_case_p)
{
    fbe_u32_t   number_of_test_cases = 0;

    /* Loop until we find the table number or we hit the terminator.
     */
    while (test_case_p->failure_type != FBE_U32_MAX)
    {
        number_of_test_cases++;
        test_case_p++;
    }
    return number_of_test_cases;
}
/*************************************************
 * end sherlock_hemlock_get_number_of_test_cases()
 *************************************************/

/*!***************************************************************************
 *          sherlock_hemlock_validate_job_results()
 *****************************************************************************
 *
 * @brief   For the test validate:
 *              o Create raid group status is as expected
 *              o Job status is as expected
 *              o Job error code is as expected
 *              o Event code is as expected
 *
 * @param   rg_config_p - Pointer to single raid group configuration to check
 * @param   test_case_p - Pointer to test case to validate results
 * @param   test_index - The test index
 * @param   create_status - The actual create status
 * @param   job_status - The actual job status
 * @param   job_error_code - The actual error code
 *
 * @return  status  - Status of validation
 *
 * @author
 *  02/15/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t sherlock_hemlock_validate_job_results(fbe_test_rg_configuration_t *rg_config_p,
                                                          sherlock_hemlock_test_case_t *test_case_p,
                                                          fbe_u32_t test_index,
                                                          fbe_status_t create_status,
                                                          fbe_status_t job_status,
                                                          fbe_job_service_error_type_t job_error_code)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_event_found = FBE_FALSE;

    /* Simply validate the results
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== sherlock hemlock validate: test index: %d error type: %d expected create status: %d job status: %d job error: %d ==",
               test_index, test_case_p->failure_type, test_case_p->expected_create_status,
               test_case_p->expected_job_status, test_case_p->expected_job_service_error);
    mut_printf(MUT_LOG_TEST_STATUS, 
               "== sherlock hemlock validate: actual create status: %d job status: %d job error: %d ==",
               create_status, job_status, job_error_code);

    /* If all (3) status don't match then fail.
     */
    if ((create_status  != test_case_p->expected_create_status)     ||
        (job_status     != test_case_p->expected_job_status)        ||
        (job_error_code != test_case_p->expected_job_service_error)    )
    {
        /* Fail the test
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "== sherlock hemlock validate: test index: %d error type: %d failed ==",
                   test_index, test_case_p->failure_type);
        MUT_ASSERT_TRUE((create_status  == test_case_p->expected_create_status)     &&
                        (job_status     != test_case_p->expected_job_status)        &&
                        (job_error_code != test_case_p->expected_job_service_error)    );
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Now validate the event (if applicable)
     */
    if (create_status == FBE_STATUS_OK)
    {
        /* Validate the proper event log is generated
         */
        status = fbe_test_sep_event_validate_event_generated(test_case_p->expected_event_code,
                                                             &b_event_found,
                                                             FBE_TRUE, /* If the event is found reset logs*/
                                                             5000 /* Wait up to 5000ms for event*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_TRUE, b_event_found);
    }

    /* Return status
     */
    return status;
}
/*********************************************
 * end sherlock_hemlock_validate_job_results()
 *********************************************/

/*!***************************************************************************
 * sherlock_hemlock_run_test_for_config()
 *****************************************************************************
 *
 * @brief   For each invalid configuration, validate that the response is
 *          as expected.
 *
 * @param   rg_config_p - Pointer to raid group configuration to test 
 * @param   context_p - Opaque pointer to the test cases
 *
 * @return  status  - Status of validation
 *
 * @author
 *  01/10/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
static void sherlock_hemlock_run_test_for_config(fbe_test_rg_configuration_t *rg_config_p,
                                                 void *context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    sherlock_hemlock_test_case_t   *current_test_case_p = (sherlock_hemlock_test_case_t *)context_p;
    fbe_u32_t                       raid_group_count;
    fbe_u32_t                       test_case_count;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                       rg_index;
    fbe_job_service_error_type_t    job_error_code;
    fbe_status_t                    job_status;
    const fbe_char_t               *raid_type_string_p = NULL;
    fbe_u32_t                       failed_drive_position = 0;
    fbe_test_raid_group_disk_set_t  original_drive_location;
    fbe_test_raid_group_disk_set_t  failed_drive_location;

    /* Validate that the number of test raid group configurations match the 
     * number of test cases.
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    test_case_count = sherlock_hemlock_get_number_of_test_cases(current_test_case_p);
    MUT_ASSERT_TRUE(raid_group_count == test_case_count);

    /* go through all the RG for which user has provided configuration data. */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Get the raid type string
         */
        fbe_test_sep_util_get_raid_type_string(current_rg_config_p->raid_type, &raid_type_string_p);

        if (current_rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(current_rg_config_p);
        }
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            current_test_case_p++;
            continue;
        }

        /* Now modify the drive(s) etc to generate the expected error code.
         */
        original_drive_location = current_rg_config_p->rg_disk_set[failed_drive_position];
        status = sherlock_hemlock_setup_raid_group_for_error_case(current_rg_config_p, current_test_case_p,
                                                                  &original_drive_location,
                                                                  &failed_drive_location);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p->rg_disk_set[failed_drive_position] = failed_drive_location;
 
        /* Attempt to create the raid group and validate that it fails
         */
        status = sherlock_hemlock_create_raid_group(current_rg_config_p, current_test_case_p);
        job_error_code = FBE_JOB_SERVICE_ERROR_INVALID;
        job_status = FBE_STATUS_INVALID;

        /* There are (2) cases:
         *  1. Create request was never issued to job service: status = failure
         *  2. Create request was issued to job service: status = success
         */
        if (status == FBE_STATUS_OK)
        {
            /* Create request was successfully sent to job service.  Wait for the
             * create request to complete in error.
             */

            status = fbe_api_common_wait_for_job(current_rg_config_p->job_number,
                                                 SHERLOCK_HEMLOCK_NS_TIMEOUT,
                                                 &job_error_code,
                                                 &job_status,
                                                 NULL);

            /* Validate that the job completed (i.e. shouldn't be `timed out')
             */
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Validate that the create request failed whit expected results
             */
            status = sherlock_hemlock_validate_job_results(current_rg_config_p, current_test_case_p, rg_index, 
                                                           status, job_status, job_error_code);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, job_status);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR, job_error_code);
            mut_printf(MUT_LOG_TEST_STATUS, "=== RAID group create job number: 0x%llx for raid type: %s ==",
                       current_rg_config_p->job_number, raid_type_string_p);
            mut_printf(MUT_LOG_TEST_STATUS, "\t Failed as expected. job_status: %d job_error_code: %d",
                       job_status, job_error_code);
            if (job_error_code == FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "\t DE168 - FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR should be: `REQUEST_BEYOND_' ");
            }
        }
        else
        {
            /* Validate that the create request failed whit expected results
             */
            status = sherlock_hemlock_validate_job_results(current_rg_config_p, current_test_case_p, rg_index,
                                                           status, job_status, job_error_code);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Validate that there is no such raid group
         */
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

        /* Perform any cleanup neccessary
         */
        status = sherlock_hemlock_cleanup_raid_group_for_error_case(current_rg_config_p, current_test_case_p,
                                                                    &original_drive_location,
                                                                    &failed_drive_location);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p->rg_disk_set[failed_drive_position] = original_drive_location;

        /* Goto next
         */
        current_rg_config_p++;
        current_test_case_p++;
    }

    /* If we made it this far the status is success
     */
    return;
}
/********************************************
 * end sherlock_hemlock_run_test_for_config()
 ********************************************/

/*!**************************************************************
 * sherlock_hemlock_setup_rg_config()
 ****************************************************************
 * @brief
 *  Given an input raid group an an input set of disks,
 *  determine which disks to use for this raid group configuration.
 *  This allows us to use a physical configuration, which is not
 *  configurable.
 *
 *  Note that if the number of disks is not sufficient for the 
 *  raid group config, then we simply enable the configs that we can.
 * 
 *  A later call to this function will enable the remaining configs
 *  to be tested.
 *
 * @param rg_config_p - current raid group config to evaluate.
 * @param drive_locations_p - The set of drives we discovered.
 *
 * @return -   
 *
 * @author
 *  02/27/2013  Ron Proulx  - Updated to suppport sherlock hemlock
 *
 ****************************************************************/
static fbe_status_t sherlock_hemlock_setup_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index = 0;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_u32_t                       raid_group_count = 0;
    fbe_test_block_size_t           block_size;
    fbe_drive_type_t                drive_type;

    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations_p->drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));

    /* Get the total entries in the array so we loop over all entries. 
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    /* For the give drive counts, disable any configs not valid for this 
     * physical config. 
     * In other words if we do not have enough drives total to create the 
     * given raid group, then disable it. 
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_test_sep_util_get_block_size_enum(current_rg_config_p->block_size, &block_size);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

        if (!current_rg_config_p->b_already_tested)
        {
            /* Check if we can find enough drives of same drive type
             */
            current_rg_config_p->b_create_this_pass = FBE_FALSE;
            current_rg_config_p->b_valid_for_physical_config = FBE_FALSE;

            for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
            {
                /* consider extra drives into account if count is set by consumer test
                 */
                if ((current_rg_config_p->width + current_rg_config_p->num_of_extra_drives) <= local_drive_counts[block_size][drive_type])
                {
                    /* We will use this in the current pass. 
                     * Mark it as used so that the next time through we will not create it. 
                     */
                    current_rg_config_p->b_create_this_pass = FBE_TRUE;
                    current_rg_config_p->b_already_tested = FBE_TRUE;
                    current_rg_config_p->b_valid_for_physical_config = FBE_TRUE;

                    /*! @note This is sherlock_hemlock specific code.  We do
                     *        remove the drives for this raid group from the 
                     *        counts since none of the raid groups will actually
                     *        be created.
                     */
                    //local_drive_counts[block_size][drive_type] -= current_rg_config_p->width;
                    //local_drive_counts[block_size][drive_type] -= current_rg_config_p->num_of_extra_drives;
                    break;
                }
            }
        }
        else
        {
            current_rg_config_p->b_create_this_pass = FBE_FALSE;
        }

        current_rg_config_p++;
    }

    /*! @note This is sherlock_hemlock specfic also.  We re-use the same drives
     *        for each raid group.  Therefore we only configure (1) raid group
     *        at a time.
     */
    current_rg_config_p = rg_config_p;
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t   original_width;
        fbe_u32_t   discover_width;

        /* Temporarily over-ride the width
         */
        original_width = current_rg_config_p->width;
        discover_width = FBE_MIN(FBE_RAID_MAX_DISK_ARRAY_WIDTH, original_width);
        current_rg_config_p->width = discover_width;

        /* Run the appropriet method (simulator or hardware)
         */
        if (fbe_test_util_is_simulation())
        {
            status = fbe_test_sep_util_discover_raid_group_disk_set(current_rg_config_p,
                                                                    1,
                                                                    drive_locations_p);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            /* Consider drive capacity into account while running the test on hardware.
             */

            /*! @todo: When we will change the spare selection algorithm to select spare drive based
             *   on minimum raid group drive capacity then we do not need this special handling.
            */
            status = fbe_test_sep_util_discover_raid_group_disk_set_with_capacity(current_rg_config_p,
                                                                                  1,
                                                                                  drive_locations_p);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }

        /* Restore the width
         */
        current_rg_config_p->width = original_width;

        /* Goto next
         */
        current_rg_config_p++;
    }

    /*! @note sherlock_hemlock specific; not need to set unused
     */
    // fbe_test_sep_util_set_unused_drive_info(rg_config_p,raid_group_count,local_drive_counts, drive_locations_p);

    return status;
}
/******************************************
 * end sherlock_hemlock_setup_rg_config()
 ******************************************/

/*!**************************************************************
 * sherlock_hemlock_run_test_on_rg_config_no_create()
 ****************************************************************
 * @brief
 *  Given a raid group config, determine which disks to use 
 *  and run the test on the applicable configurations.
 *
 * @param rg_config_p - current raid group config to evaluate.
 * @param test_fn - The test function to run
 *
 * @return -   
 *
 * @author
 *  02/27/2013  Ron Proulx  - Updated.
 *
 ****************************************************************/
static void sherlock_hemlock_run_test_on_rg_config_no_create(fbe_test_rg_configuration_t *rg_config_p,
                                              void * context_p,
                                              fbe_test_rg_config_test test_fn)
{
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_u32_t                               raid_group_count_this_pass = 0;
    fbe_u32_t                               raid_group_count = 0;

    /* Initialize the raid groups
     */
    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* Get the raid group config for the first pass.
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    sherlock_hemlock_setup_rg_config(rg_config_p, &drive_locations);
    raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);
    MUT_ASSERT_TRUE(raid_group_count_this_pass == raid_group_count);
    mut_printf(MUT_LOG_LOW, "testing %d raid groups \n", raid_group_count_this_pass);

    /* Run the tests.
     */
    test_fn(rg_config_p, context_p);

    fbe_test_sep_util_report_created_rgs(rg_config_p);
    return;
}
/*********************************************************
 * end sherlock_hemlock_run_test_on_rg_config_no_create()
 *********************************************************/

/*!**************************************************************
 * sherlock_hemlock_test()
 ****************************************************************
 * @brief
 *  For each invalid configuration, validate that the response is
 *  as expected.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/12/2010  Ron Proulx  - Created
 *
 ****************************************************************/

void sherlock_hemlock_test(void)
{
    fbe_test_rg_configuration_t    *rg_config_p = NULL;
    sherlock_hemlock_test_case_t   *test_case_p = NULL;
    fbe_u32_t                       test_level;

    /* Get the riad group config array and associated test cases*/
    test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    if (test_level > 0)
    {
        rg_config_p = &sherlock_hemlock_raid_group_invalid_config_extended[0];
        test_case_p = &sherlock_hemlock_test_case_extended[0];
    }
    else
    {
        rg_config_p = &sherlock_hemlock_raid_group_invalid_config_qual[0];
        test_case_p = &sherlock_hemlock_test_case_qual[0];
    }

    /*! @note We don't expect `Error' level traces.  So please don't disable them.
     */
    //fbe_test_sep_util_disable_trace_limits();

    /* Execute the method that attempts to create the raid group and expects
    * a failure.
    */
    sherlock_hemlock_run_test_on_rg_config_no_create(rg_config_p, 
                                             (void *)test_case_p, 
                                             sherlock_hemlock_run_test_for_config);


    return;
}
/******************************************
 * end sherlock_hemlock_test()
 ******************************************/

/*!**************************************************************
 * sherlock_hemlock_setup()
 ****************************************************************
 * @brief
 *  Setup for invalid raid group configuration test.  Note!! this
 *  method ONLY creates the drive neccessary to test the configurations.
 *  It doesn't actually create the raid groups!!
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/12/2010  Ron Proulx  - Created
 *
 ****************************************************************/
void sherlock_hemlock_setup(void)
{   
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (test_level > 0)
        {
            rg_config_p = &sherlock_hemlock_raid_group_invalid_config_extended[0];
        }
        else
        {
            rg_config_p = &sherlock_hemlock_raid_group_invalid_config_qual[0];
        }

        /* Only create the and load the physical configuration (i.e. don't create
         * the raid groups).
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        sherlock_hemlock_load_physical_config();
        elmo_load_sep_and_neit();
    }
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end sherlock_hemlock_setup()
 **************************************/

/*!**************************************************************
 * sherlock_hemlock_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the sherlock hemlock.  We cannot use standard cleanup
 *  since we purposfully induced errors
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/12/2010  Ron Proulx  - Created
 *
 ****************************************************************/

void sherlock_hemlock_cleanup(void)
{
    if (fbe_test_util_is_simulation())
    {
        mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

        /* Destroy: SEP, NEIT, Physical and terminator.
         */
        fbe_test_sep_util_destroy_neit_sep_physical();
        
        mut_printf(MUT_LOG_LOW, "=== %s completes ===\n", __FUNCTION__);
    }
    return;
}
/******************************************
 * end sherlock_hemlock_cleanup()
 ******************************************/

/***********************************
 * end file sherlock_hemlock_test.c
 ***********************************/


