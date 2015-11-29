/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file scoobert_drive_sparing_test.c
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains a scoobert hot spare selection algorithm  test.
 *
 * HISTORY
 *   7/30/2009:  Created. Dhaval Patel.
 *   8/30/2011 - Modified Vishnu Sharma.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_random.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_sim_transport.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_test_configurations.h"
#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_event.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "sep_rebuild_utils.h"


char * scoobert_short_desc = "drive sparing criterion";
char * scoobert_long_desc =
    "\n"
    "\n"
    "The Scoobert Scenario tests the criterion for selecting spare drives.\n"
    "\n"
    "It covers the following cases:\n"
    "\t- Spare drive selection based on same drive type(currently) need to modify.\n"
    "\t- after we get a complete performance tier table for drive types.\n"
    "\t- Spare drive selection based on bus proximity.\n"
    "\t- Spare drive selection based on drive capacity.\n"
    "\t- Spare drive selection based on enclosure proximity.\n"
    "\n"
    "Starting Config: \n"
    "\t[PP] armada board\n"
    "\t[PP] two SAS PMC ports (PORT-0 and PORT-1)\n"
    "\t[PP] three viper enclosures two on port 0 (ENCL-A and ENCL-B) and one on port 1(ENCL -C)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-A, slot-5 (PDO-A5)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-A, slot-6 (PDO-A6)\n"
    "\t[PP] a SAS drive with a capacity of 0x30000 blocks in ENCL-A, slot-7 (PDO-A7)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-A, slot-8 (PDO-A8)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-A, slot-9 (PDO-A9)\n"
    "\t[PP] a SAS drive with a capacity of 0x30000 blocks in ENCL-A, slot-14(PDO-A14)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-0 (PDO-B0)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-1 (PDO-B1)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-2 (PDO-B2)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-3 (PDO-B3)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-4 (PDO-B4)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-5 (PDO-B5)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-6 (PDO-B6)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-9 (PDO-B9)\n"
    "\t[PP] a SAS drive with a capacity of 0x40000 blocks in ENCL-B, slot-10(PDO-B10)\n"
    "\t[PP] a SAS drive with a capacity of 0x30000 blocks in ENCL-C, slot-6 (PDO-C6)\n"
    "\t[PP] logical drives for each of the physical drives\n"
    "\t[SEP] provision drives for each of the logical drives\n"
    "\t[SEP] a RAID-1 RG object with an I/O edge to VD-B5 and VD-B6\n"
    "\t[SEP] a RAID-5 RG object with an I/O edge to VD-A5, VD-A6, VD-A7, VD-A8 and VD-A9\n"
    "\t[SEP] a RAID-5 RG object with an I/O edge to VD-B0, VD-B1, VD-B2, VD-B3 and VD-B4\n"
    "\t[SEP] configure the PVD-B9 and PVD -C6 as Spare\n"
    "\n"
    "STEP 1: Make VD-B5 need a spare, Confirm no spare (PVD) will get swapped in and fails with error NO SUITABLE SPARE\n"
    "\t- Disable the sparing queue in the job service\n"
    "\t- Remove the physical drive (PDO-B5).\n"
    "\t- Verify whether PDO and LDO got destroyed after drive removal or not.\n"
    "\t- Verify that  virtual drive VD-B5 is in the FAIL state.\n"
    "\t- Enable the job service queue which allows job service to process the swap commands.\n"
    "\t- Verify that no spare gets swapped-in and spare operation fails with NO SUITABLE SPARE.\n"
    "\t- Reinsert the physical drive (PDO-B5).\n" 
    "\t- Verify whether PDO and LDO of B-5 is in the READY state.\n"
    "STEP 2: Make VD-A5 need a spare, Confirm PVD-B9 will get swapped in\n"
    "\t- Disable the sparing queue in the job service\n"
    "\t- Remove the physical drive (PDO-A5).\n"
    "\t- Verify whether PDO and LDO got destroyed after drive removal or not.\n"
    "\t- Verify that  virtual drive VD-A5 is in the FAIL state.\n"
    "\t- Enable the job service queue which allows job service to process the swap commands.\n"
    "\t- Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "\t- Get the spare edge information. Confirm desired HS has been swapped in.\n" 
    "\t- Verify that  virtual drive VD-A5 is in the READY state.\n"
    "STEP 3: Make VD-B9 need a spare, Confirm PVD-C6 will get swapped in\n"
    "\t- Disable the sparing queue in the job service\n"
    "\t- Remove the physical drive (PDO-B9).\n"
    "\t- Verify whether PDO and LDO got destroyed after drive removal or not.\n"
    "\t- Verify that  virtual drive VD-B9 is in the FAIL state.\n"
    "\t- Enable the job service queue which allows job service to process the swap commands.\n"
    "\t- Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "\t- Get the spare edge information. Confirm desired HS has been swapped in.\n" 
    "\t- Verify that  virtual drive VD-B9 is in the READY state.\n"
    "\t[SEP] configure the PVD-A14and PVD -B10 as Spare\n"
    "STEP 4: Make VD-B0 need a spare, Confirm PVD-A14 will get swapped in\n"
    "\t- Disable the sparing queue in the job service\n"
    "\t- Remove the physical drive (PDO-B0).\n"
    "\t- Verify whether PDO and LDO got destroyed after drive removal or not.\n"
    "\t- Verify that  virtual drive VD-B0 is in the FAIL state.\n"
    "\t- Enable the job service queue which allows job service to process the swap commands.\n"
    "\t- Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "\t- Get the spare edge information. Confirm desired HS has been swapped in.\n" 
    "\t- Verify that  virtual drive VD-B0 is in the READY state.\n"
    "STEP 5: Make VD-A14 need a spare, Confirm PVD-B10 will get swapped in\n"
    "\t- Disable the sparing queue in the job service\n"
    "\t- Remove the physical drive (PDO-A14).\n"
    "\t- Verify whether PDO and LDO got destroyed after drive removal or not.\n"
    "\t- Verify that  virtual drive VD-A14 is in the FAIL state.\n"
    "\t- Enable the job service queue which allows job service to process the swap commands.\n"
    "\t- Verify that permanent spare gets swapped-in and edge between VD object and newly swapped-in PVD object becomes enabled.\n"
    "\t- Get the spare edge information. Confirm desired HS has been swapped in.\n" 
    "\t- Verify that  virtual drive VD-A14 is in the READY state.\n"
    "STEP 6: Make VD-B10 need a spare, Confirm no PVD will get swapped in and fail with error no hot spare available\n"
    "\t- Disable the sparing queue in the job service\n"
    "\t- Remove the physical drive (PDO-B10).\n"
    "\t- Verify whether PDO and LDO got destroyed after drive removal or not.\n"
    "\t- Verify that  virtual drive VD-B10 is in the FAIL state.\n"
    "\t- Enable the job service queue which allows job service to process the swap commands.\n"
    "\t- Verify that no spare gets swapped-in and spare operation fails with NO SPARE AVAILABLE.\n"
    ;

/*!*******************************************************************
 * @def SCOOBERT_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the qual test. 
 *
 *********************************************************************/
#define SCOOBERT_TEST_LUNS_PER_RAID_GROUP                 1

/*!*******************************************************************
 * @def SCOOBERT_TEST_CHUNKS_PER_LUN
 *********************************************************************
 * @brief chunks per lun for the qual test. 
 *
 *********************************************************************/
#define SCOOBERT_TEST_CHUNKS_PER_LUN                      3


/*!*******************************************************************
 * @def SCOOBERT_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief raid group count for the sas 520 raid group. 
 *
 *********************************************************************/
#define SCOOBERT_TEST_RAID_GROUP_COUNT                    3

/*!*******************************************************************
 * @def SCOOBERT_NUM_HS_PER_TEST_CASE
 *********************************************************************
 * @brief it is used to test the configuration of hot spare pvd.
 *
 *********************************************************************/
#define SCOOBERT_NUM_HS_PER_TEST_CASE                     2

/*!*******************************************************************
 * @def SCOOBERT_MAX_NUM_REMOVED_DRIVES
 *********************************************************************
 * @brief Maximum number of removed drives is currently (2).
 *
 *********************************************************************/
#define SCOOBERT_MAX_NUM_REMOVED_DRIVES                   2

/*!*******************************************************************
 * @def SCOOBERT_NUM_REMOVED_DRIVES
 *********************************************************************
 * @brief Number of removed drives is currently (1).
 *
 *********************************************************************/
#define SCOOBERT_NUM_REMOVED_DRIVES                        1

/*!*******************************************************************
 * @def SCOOBERT_TOTAL_HS_TO_CONFIGURE
 *********************************************************************
 * @brief it is used to test the configuration of hot spare pvd.
 *
 *********************************************************************/
#define SCOOBERT_TOTAL_HS_TO_CONFIGURE                    4

/*!*******************************************************************
 * @def SCOOBERT_CONFIGURE_HS_TEST_COUNT
 *********************************************************************
 * @brief it is used to test the configuration of hot spare pvd.
 *
 *********************************************************************/
#define SCOOBERT_CONFIGURE_HS_TEST_COUNT                  1

/*!*******************************************************************
 * @def SCOOBERT_TEST_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SCOOBERT_TEST_WAIT_SEC 60

/*!*******************************************************************
 * @def SCOOBERT_TEST_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define SCOOBERT_TEST_WAIT_MSEC 1000 * SCOOBERT_TEST_WAIT_SEC

/*!*******************************************************************
 * @def     SCOOBERT_NUM_SYSTEM_DRIVE_SLOTS
 *********************************************************************
 * @brief   Simply define for the number of slots in the first
 *          enclosure reserved for system drives.
 *  
 * @todo    Write an API to get the number of drive slots reserved for
 *          system drives.
 *********************************************************************/
#define SCOOBERT_NUM_SYSTEM_DRIVE_SLOTS         (4)

/*!*******************************************************************
 * @def     SCOOBERT_INVALID_DISK_POSITION
 *********************************************************************
 *  @brief  Invalid disk position.  Used when searching for a disk 
 *          position and no disk is found that meets the criteria.
 *
 *********************************************************************/
#define SCOOBERT_INVALID_DISK_POSITION           ((fbe_u32_t) -1)

/*!*******************************************************************
 * @def SCOOBERT_INVALID_BUS_ENCL_SLOT
 *********************************************************************
 * @brief Invalid bus, enclosure, slot
 *********************************************************************/
#define SCOOBERT_INVALID_BUS_ENCL_SLOT FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX

typedef enum scoobert_test_case_enum_e {
    SCOOBERT_CONFIGURE_SPARE_TEST = 0, 
    SCOOBERT_NO_HS_AVAILABLE,      
    SCOOBERT_No_SUITABLE_HS_AVAILABLE,
    SCOOBERT_SPARE_LARGER_THAN_ORIGINAL,
    SCOOBERT_SPARE_SMALLER_THAN_ORIGINAL_BUT_BIG_ENOUGH,
    SCOOBERT_BUS_IS_HIGHER_PRIORITY_THAN_CAPACITY,
    SCOOBERT_CAPACITY_IS_HIGHER_PRIORITY_THAN_ENCL,
    SCOOBERT_BUS_PROXIMITY,
    SCOOBERT_ENCL_PROXIMITY,        
    SCOOBERT_SELECT_NON_SYSTEM_DRIVE,                   /*! @note This should be before the `select system drive' case.*/  
    SCOOBERT_SELECT_SYSTEM_DRIVE,
    SCOOBERT_SAME_BLOCK_SIZE,
    SCOOBERT_SAME_BLOCK_SIZE_UNAVAILABLE, 
    SCOOBERT_NO_LUNS_BOUND,
    SCOOBERT_TEST_CASE_LAST = SCOOBERT_NO_LUNS_BOUND,   /*! @note This must be the last case.*/
    SCOOBERT_NUM_TEST_CASES,
} scoobert_test_case_enum_t;

typedef struct scoobert_test_spare_lib_ns_context_s
{
    fbe_semaphore_t                 sem;
    fbe_object_id_t                 object_id;
    fbe_job_service_info_t          js_error_info;
}
scoobert_test_spare_lib_ns_context_t;

/*!*******************************************************************
 * @var scoobert_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t scoobert_raid_group_config[] = 
{
       
        /* width,   capacity    raid type,                  class,                 block size      RAID-id.    bandwidth.*/
        /* [1] RAID-5 4 + 1 Bound only on non-system drives */
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            0,          0,
            {{0, 2, 10}, {0, 2, 11}, {0, 2, 12}, {0, 2, 13}, {0, 2, 14}}},
        /* [0] RAID-5 4 + 1 Bound including (2) system drives */
        {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,          0,
            {{0, 0, 0}, {0, 0, 3}, {1, 0, 0}, {1, 0, 1}, {1, 0, 2}}},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    
};



/* shaggy rdgen context. */
fbe_api_rdgen_context_t   scoobert_test_context[SCOOBERT_TEST_LUNS_PER_RAID_GROUP * 2];

typedef struct scoobert_ns_context_s
{
    fbe_semaphore_t sem;
    fbe_u32_t slot;
    fbe_u32_t encl;
    fbe_u32_t port;
}
scoobert_ns_context_t;

/*scoobert test cases ( name and count) */
typedef struct scoobert_test_case_s 
{
    const char                  *test_case_name;
    scoobert_test_case_enum_t   test_case;
    fbe_bool_t                  b_configure_hot_spares;
    fbe_u32_t                   rg_index;
    fbe_u32_t                   hs_index_to_select;
    fbe_job_service_error_type_t expected_job_service_error;
    fbe_u32_t                   expected_event_message_id;
    fbe_u32_t                   test_rg_position[SCOOBERT_MAX_NUM_REMOVED_DRIVES];
    fbe_test_hs_configuration_t hs_config[SCOOBERT_NUM_HS_PER_TEST_CASE];
} scoobert_test_case_t;

/*initialize the scoobert test cases (name and count) */
static scoobert_test_case_t  test_cases[SCOOBERT_NUM_TEST_CASES] =
{
        /* disk set                index        test    block size  object-id               handle*/
    {"Configure spare test",                    SCOOBERT_CONFIGURE_SPARE_TEST,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        SCOOBERT_INVALID_DISK_POSITION, SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        0 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"No HS available",                         SCOOBERT_NO_HS_AVAILABLE,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, SEP_WARN_SPARE_NO_SPARES_AVAILABLE,
        0 /* Test position is 0 */, SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        1 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"No suitable HS available",                SCOOBERT_No_SUITABLE_HS_AVAILABLE,
        FBE_TRUE, 0, FBE_U32_MAX,  FBE_JOB_SERVICE_ERROR_NO_ERROR, SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE,  
        0 /* Test position is 0 */, SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        2 */
        {{{0,1,9},                                       520,        FBE_OBJECT_ID_INVALID},
         {{1,0,6},                                       520,        FBE_OBJECT_ID_INVALID}}, 
    },
    {"Replacement is larger than original",     SCOOBERT_SPARE_LARGER_THAN_ORIGINAL,
        FBE_TRUE, 0, FBE_U32_MAX,  FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,  
        0 /* Test position is 0 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        3 */
        {{{0,1,5},                                       520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}}, 
    },
    {"Replacement is smaller but sufficient",   SCOOBERT_SPARE_SMALLER_THAN_ORIGINAL_BUT_BIG_ENOUGH,
        FBE_TRUE, 0, FBE_U32_MAX,  FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,  
        1 /* Test position is 1 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        4 */
        {{{0,1,4},                                       520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}}, 
    },
    {"bus is higher priority than capacity",    SCOOBERT_BUS_IS_HIGHER_PRIORITY_THAN_CAPACITY,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        0 /* Test position is 0 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        5 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"capacity is higher priority than encl",   SCOOBERT_CAPACITY_IS_HIGHER_PRIORITY_THAN_ENCL,
        FBE_TRUE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        1 /* Test position is 1 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        6 */
        {{{0,0,14},                                      520,        FBE_OBJECT_ID_INVALID},
         {{0,1,10},                                      520,        FBE_OBJECT_ID_INVALID}},
    },
    {"bus proximity",                           SCOOBERT_BUS_PROXIMITY,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        2 /* Test position is 2 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        8 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"encl proximity",                          SCOOBERT_ENCL_PROXIMITY,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        3 /* Test position is 3 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        7 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"select non-system drive",                 SCOOBERT_SELECT_NON_SYSTEM_DRIVE,
        FBE_TRUE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        0 /* Test position is 0 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        9 */
        {{{0,0,1},                                       520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"select system drive",                     SCOOBERT_SELECT_SYSTEM_DRIVE,
        FBE_TRUE, 1, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        0 /* Test position is 0 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index       10 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"Same block size",                          SCOOBERT_SAME_BLOCK_SIZE,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        3 /* Test position is 3 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        12 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"Same Block size Unavailable",                          SCOOBERT_SAME_BLOCK_SIZE_UNAVAILABLE,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, FBE_U32_MAX,
        3 /* Test position is 3 */ ,SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index        7 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
    {"No LUNs bound",                           SCOOBERT_NO_LUNS_BOUND,
        FBE_FALSE, 0, FBE_U32_MAX, FBE_JOB_SERVICE_ERROR_NO_ERROR, SEP_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED,
        0 /* Test position is 0 */, SCOOBERT_INVALID_DISK_POSITION,
        /* Test config index       11 */
        {{{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID},
         {{SCOOBERT_INVALID_BUS_ENCL_SLOT},              520,        FBE_OBJECT_ID_INVALID}},
    },
   
};

/*************************
 *   FUNCTION PROTOTYPES
 *************************/
void scobbert_test_remove_hot_spare(fbe_object_id_t pvd_object_id,
                                    fbe_job_service_error_type_t * js_error_type_p);
void scoobert_run_tests(fbe_test_rg_configuration_t *rg_config_ptr,void * context_p);

fbe_bool_t scoobert_is_unused_drive(fbe_test_raid_group_disk_set_t *disk_set_p);
void scoobert_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun);
static fbe_bool_t scoobert_find_another_spare_for_block_size(fbe_test_rg_configuration_t *rg_config_p,
                                                             scoobert_test_case_t *const test_case_p,
                                                             fbe_test_discovered_drive_locations_t *drive_locations_p,
                                                             fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                             fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                             fbe_test_raid_group_disk_set_t *better_block_size_disk_set_p,
                                                             fbe_lba_t selected_disk_capacity,
                                                             fbe_lba_t selected_disk_offset);


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!****************************************************************************
 * scoobert_test_fill_removed_disk_set()
 ******************************************************************************
 * @brief
 *   This function fills the removed disk array by removed drives.
 *       
 * @param rg_config_p           - pointer to raid group
 * @param removed_disk_set_p    - pointer to array
 * @param position_p            - pointer to position
 * @param number_of_drives      - number of drives removed
 * 
 * @return none.
 *
 ******************************************************************************/
static void scoobert_test_fill_removed_disk_set(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                       fbe_u32_t * position_p,
                                       fbe_u32_t number_of_drives)
{

    fbe_u32_t disk_index =0;

    for (disk_index = 0;disk_index <number_of_drives;disk_index++)
    {
        removed_disk_set_p[disk_index] = rg_config_p->rg_disk_set[position_p[disk_index]];        
    }

}
/********************************************* 
 * end scoobert_test_fill_removed_disk_set()
 *********************************************/

/*!***************************************************************************
 *          scoobert_validate_test_results() 
 *****************************************************************************
 *
 * @brief   For the test case supplied, validate the expected results.
 *      
 * @param   test_case_p - Pointer to test case to validate
 * @param   job_service_error - The actual job service error code
 *
 * @return  status
 *
 * @author
 *  04/25/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t scoobert_validate_test_results(scoobert_test_case_t *test_case_p,
                                                   fbe_job_service_error_type_t job_service_error)

{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_event_found = FBE_FALSE;

    /* Validate the parameter.
     */
    if ((test_case_p == NULL)                               ||
        (test_case_p->test_case >  SCOOBERT_TEST_CASE_LAST)    )
    {
        /* Invalid parameter.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s Invalid test case pointer: 0x%p ot test case: %d",
                   __FUNCTION__, test_case_p, (test_case_p == NULL) ? 0 : test_case_p->test_case);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the expected job service error code.
     */
    if (job_service_error != test_case_p->expected_job_service_error)
    {
        /* Print a message and then fail the validation
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "validate test results: test case: %s (%d) job error: %d doesn't match expected: %d",
                   test_case_p->test_case_name, test_case_p->test_case,
                   job_service_error, test_case_p->expected_job_service_error);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* If an event message was expected, validate it.
     */
    if (test_case_p->expected_event_message_id != FBE_U32_MAX)
    {
        /*! @note Invoke method to find expected event message.
         * 
         *  @note Since we may need to wait for an entire monitor
         *        cycle to run, wait up to 5 seconds.
         */
        status = fbe_test_sep_event_validate_event_generated(test_case_p->expected_event_message_id,
                                                             &b_event_found,
                                                             FBE_TRUE, /* If the event is found reset logs*/
                                                             5000 /* Wait up to 5000ms for event*/);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (b_event_found == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                   "validate test results: test case: %s (%d) event message id: 0x%08X wasn't found",
                   test_case_p->test_case_name, test_case_p->test_case,
                   test_case_p->expected_event_message_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Return the status
     */
    return status;
}
/**************************************
 * end scoobert_validate_test_results()
 **************************************/

/*!****************************************************************************
 * scoobert_setup()
 ******************************************************************************
 * @brief
 *  Setup for a drive sparing test.
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  4/10/2010 - Created. Dhaval Patel
 *  24/08/2011 - Modified Vishnu Sharma
 ******************************************************************************/
void scoobert_setup(void)
{
    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &scoobert_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        scoobert_physical_config(520);

        elmo_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
        
    }

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************************************************
 * end scoobert_setup()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_cleanup()
 ******************************************************************************
 * @brief
 *  Setup for a drive sparing test.
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  
 *  24/08/2011 - Created Vishnu Sharma
 ******************************************************************************/

void scoobert_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!****************************************************************************
 * scoobert_4k_setup()
 ******************************************************************************
 * @brief
 *  Setup for a drive sparing test for 4k block size drives.
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  06/17/2014 - Created - Ashok Tamilarasan. 
 ******************************************************************************/
void scoobert_4k_setup(void)
{
    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &scoobert_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_rg_setup_block_sizes(rg_config_p, 4160);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        scoobert_physical_config(4160);

        elmo_load_sep_and_neit();

        /* Set the trace level to info. */
        elmo_set_trace_level(FBE_TRACE_LEVEL_INFO);
        
    }

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    return;
}
/******************************************************************************
 * end scoobert_4k_setup()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_4k_cleanup()
 ******************************************************************************
 * @brief
 *  Setup for a drive sparing test.
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  
 *  06/17/2014 - Created - Ashok Tamilarasan.
 ******************************************************************************/

void scoobert_4k_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}

/*!***************************************************************************
 *          scoobert_get_replacement_drive_required_capacity()
 *****************************************************************************
 *
 * @brief   Based on the raid group supplied, determine the minimum (exported)
 *          capacity of a replacement drive.  
 *
 * @param   rg_config_p - Poiner to raid group test configuration to use to
 *              determine the minimum required replacement drive capacity.
 * @param   position_to_replace - Raid group postion that will be spared.
 *
 * @return  capacity - Minimum provision drive exported capacity (in blocks)
 *              for a replacement drive.
 *
 * @note    This assumes only (1) user raid group per drive.
 * 
 * @author
 *  05/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_lba_t scoobert_get_replacement_drive_required_capacity(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t position_to_replace)

{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_get_info_t   virtual_drive_info;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_lba_t                       min_required_capacity = FBE_LBA_INVALID;
    fbe_spare_drive_info_t          spare_drive_info;
    fbe_u32_t                       position;                    

    /* Get the raid group and virtual drive information.  The current
     * requirement is that every raid group position have the same offset
     * and will use the same capacity.  Therefore simply use position 0.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(position_to_replace < rg_config_p->width);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_replace, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(0, vd_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_id);
    status = fbe_api_raid_group_get_info(vd_object_id, &virtual_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the current provision drive information.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position_to_replace].bus,
                                                            rg_config_p->rg_disk_set[position_to_replace].enclosure,
                                                            rg_config_p->rg_disk_set[position_to_replace].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine the minimum capacity of the replacement provisoned drive.
     *  o Minimum capacity required for replacement drive.  This includes:
     *      + Capacity consumed by any LUs bound on raid group
     *      + Any unused capacity on raid group
     *      + Raid group metadata capacity
     *      + Raid group journal capacity (depending on raid type)
     *      = Virtual drive metadata capacity (a mirror)
     *      - Provision drive/virtual drive offset.
     */
    min_required_capacity = raid_group_info.imported_blocks_per_disk;
    min_required_capacity += virtual_drive_info.paged_metadata_capacity;
    min_required_capacity += provision_drive_info.default_offset;

    /* Now use the same API as the spare selection library and compare the
     * two values.
     */
    status = fbe_api_virtual_drive_get_spare_info(vd_object_id, &spare_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(spare_drive_info.capacity_required != 0);
    MUT_ASSERT_TRUE(spare_drive_info.capacity_required != FBE_LBA_INVALID);
    MUT_ASSERT_TRUE(spare_drive_info.capacity_required >= min_required_capacity);
    MUT_ASSERT_TRUE(spare_drive_info.capacity_required >= (virtual_drive_info.paged_metadata_start_lba + virtual_drive_info.paged_metadata_capacity));
    MUT_ASSERT_TRUE(spare_drive_info.capacity_required <= spare_drive_info.configured_capacity);
    min_required_capacity = spare_drive_info.capacity_required;

    /* Now validate that all position require the same replacement capacity.
     */
    for (position = 0; position < rg_config_p->width; position++)
    {
        /* Skip the position being replaced.
         */
        if (position != position_to_replace)
        {
            /* Validate that all position require the same spare capacity.
             */
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(0, vd_object_id);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, vd_object_id);
            status = fbe_api_virtual_drive_get_spare_info(vd_object_id, &spare_drive_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_TRUE(spare_drive_info.capacity_required == min_required_capacity);
        }
    }
    /* Return the minimum required capacity.
     */
    return min_required_capacity;
}
/*********************************************************
 * end scoobert_get_replacement_drive_required_capacity()
 *********************************************************/

/*!****************************************************************************
 * scoobert_test_configure_hot_spare()
 ******************************************************************************
 * @brief
 *  This function is used to configure the pvd object as hot
 *  spare.
 * 
 * @param test_case_p - Pointer to test case information
 * @param pvd_object_id     - pvd object-id.
 * @param js_error_type_p   - returns error type of job service if any.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 08/24/2010 - Created. Dhaval Patel
 * 
 ******************************************************************************/
void scoobert_test_configure_hot_spare(scoobert_test_case_t *const test_case_p,
                                       fbe_object_id_t pvd_object_id,
                                       fbe_job_service_error_type_t * js_error_type_p)
{
    fbe_api_job_service_update_provision_drive_config_type_t    update_pvd_config_type;
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;
    fbe_api_provision_drive_info_t                              provision_drive_info;

    /* In order to make configuring particular PVDs as hot spares work, first
     * we need to set automatic hot spare not working (set to false). 
     */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* initialize error type as no error. */
    *js_error_type_p = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* get the pvd location */
    status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* configure the pvd as hot spare. */
    update_pvd_config_type.pvd_object_id = pvd_object_id;
    update_pvd_config_type.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE;

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_provision_drive_config_type(&update_pvd_config_type);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_config_type.job_number,
                                         SCOOBERT_TEST_WAIT_MSEC,
                                         js_error_type_p,
                                         &job_status,
                                         NULL);
    /* verify the job status. */
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out configuring hot spare");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "Configuring PVD as hot spare failed");

    /* Display a progress message
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "**** test case: `%-32s'-configured spare: pvd obj: 0x%x(%d_%d_%d) capacity: 0x%llx****",
               test_case_p->test_case_name, pvd_object_id,
               provision_drive_info.port_number, provision_drive_info.enclosure_number, provision_drive_info.slot_number,
               (unsigned long long)provision_drive_info.capacity);

    return;
}
/******************************************************************************
 * end scoobert_test_configure_hot_spare()
 ******************************************************************************/

/*!***************************************************************************
 *      scoobert_setup_unbind_all_luns()
 *****************************************************************************
 *
 * @brief   Unbind all the luns on the raid group specified.
 *
 * @param   test_case_p - Pointer to test case to setup
 * @param   rg_config_p - Pointer to raid group to unbind all luns on
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 *****************************************************************************/
static fbe_status_t scoobert_setup_unbind_all_luns(scoobert_test_case_t *const test_case_p,
                                                   fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       lu_index;
    fbe_test_logical_unit_configuration_t *logical_unit_configuration_list_p = NULL;

    /* Validate that this is the last test case.
     */
    MUT_ASSERT_TRUE(test_case_p->test_case == SCOOBERT_TEST_CASE_LAST);

    /* Unbind all the luns on this raid group.
     */
    status = fbe_test_sep_util_destroy_logical_unit_configuration(rg_config_p->logical_unit_configuration_list,
                                                                  rg_config_p->number_of_luns_per_rg);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of Logical units failed.");

    logical_unit_configuration_list_p = &rg_config_p->logical_unit_configuration_list[0];
    for (lu_index = 0; lu_index < rg_config_p->number_of_luns_per_rg; lu_index++)
    {
        fbe_zero_memory(logical_unit_configuration_list_p, sizeof(*logical_unit_configuration_list_p));
        logical_unit_configuration_list_p++;
    }
    rg_config_p->number_of_luns_per_rg = 0;

    return status;
}
/**************************************
 * end scoobert_setup_unbind_all_luns()
 **************************************/
/*!***************************************************************************
 *      scoobert_setup_bind_all_luns()
 *****************************************************************************
 *
 * @brief   Bind all the luns on the raid group specified.
 *
 * @param   test_case_p - Pointer to test case to setup
 * @param   rg_config_p - Pointer to raid group to unbind all luns on
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 *****************************************************************************/
static fbe_status_t scoobert_setup_bind_all_luns(scoobert_test_case_t *const test_case_p,
                                                 fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Validate that this is the last test case.
     */
    MUT_ASSERT_TRUE(test_case_p->test_case == SCOOBERT_TEST_CASE_LAST);
    MUT_ASSERT_TRUE(rg_config_p->number_of_luns_per_rg == 0);

    /* Bind all the luns on this raid group.
     */
    rg_config_p->number_of_luns_per_rg = SCOOBERT_TEST_LUNS_PER_RAID_GROUP;

    /* Create a set of LUNs for this RAID group. */
    status = fbe_test_sep_util_create_logical_unit_configuration(rg_config_p->logical_unit_configuration_list,
                                                                 rg_config_p->number_of_luns_per_rg);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical units failed.");

    return status;
}
/**************************************
 * end scoobert_setup_bind_all_luns()
 **************************************/

/*!***************************************************************************
 *          scoobert_is_system_drive()
 *****************************************************************************
 *
 * @brief   Simply determines if the proposed drive is in one of the system 
 *          drive slots or not.
 * 
 * @param   proposed_disk_p - Pointer to proposed spare location information
 *
 * @return  bool - FBE_TRUE - The proposed drive is a system drive.
 *
 * @author
 *  06/07/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_is_system_drive(fbe_test_raid_group_disk_set_t *proposed_disk_p)
{
    /* Simply check if this is a system drive or not.
     */
    if ((proposed_disk_p->bus == 0)                               &&
        (proposed_disk_p->enclosure == 0)                         &&
        (proposed_disk_p->slot < SCOOBERT_NUM_SYSTEM_DRIVE_SLOTS)     )
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/*********************************
 * end scoobert_is_system_drive()
 ********************************/

/*!***************************************************************************
 *          scoobert_is_better_bus_capacity()
 *****************************************************************************
 *
 * @brief   This function determines if the proposed drive is at a better bus
 *          location than the currently selected drive or not.
 * 
 * @param   proposed_disk_p - Pointer to proposed spare location information
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 *
 * @return  bool - FBE_TRUE - The proposed drive is at a better bus proximity
 *                  than the currently selected drive.
 *
 * @author
 *  06/12/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_is_better_bus_capacity(fbe_test_raid_group_disk_set_t *proposed_disk_p,
                                                   fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                   fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                   fbe_lba_t selected_disk_capacity,
                                                   fbe_lba_t selected_disk_offset)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      b_better_proximity = FBE_FALSE;
    fbe_object_id_t                 pvd_id;    
    fbe_api_provision_drive_info_t  pvd_info;
  
    /* Only look at `unconsumed' or `test spare' drives.
     */
    if (scoobert_is_unused_drive(proposed_disk_p) == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    /* Get the provision drive information.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(proposed_disk_p->bus,
                                                            proposed_disk_p->enclosure,
                                                            proposed_disk_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the drive capacity from pvd object.
     */
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    /* Check if the proposed spare is at a better bus proximity compared to
     * the currently selected spare.  The proposed capacity must be greater than
     * currently selected otherwise a `best fit' match could break this rule.
     */
    if ((pvd_info.capacity > selected_disk_capacity)      &&
        (pvd_info.default_offset == selected_disk_offset) &&
        (selected_drive_p->bus != failed_disk_p->bus)     &&
        (proposed_disk_p->bus  == failed_disk_p->bus)        )
    {
        b_better_proximity = FBE_TRUE;
    }

    /* Return if this is a better proximity or not.
     */
    return b_better_proximity;
}
/******************************************************************************
 * end scoobert_is_better_bus_capacity()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_is_better_enclosure_capacity()
 *****************************************************************************
 *
 * @brief   This function determines if the proposed drive is at a better
 *          enclosure (on the same bus) location than the currently selected 
 *          drive or not.
 * 
 * @param   proposed_disk_p - Pointer to proposed spare location information
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 *
 * @return  bool - FBE_TRUE - The proposed drive is at a better enclosure 
 *                  proximity than the currently selected drive.
 *
 * @author
 *  06/12/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_is_better_enclosure_capacity(fbe_test_raid_group_disk_set_t *proposed_disk_p,
                                                        fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                        fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                        fbe_lba_t selected_disk_capacity,
                                                        fbe_lba_t selected_disk_offset)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      b_better_proximity = FBE_FALSE;
    fbe_object_id_t                 pvd_id;    
    fbe_api_provision_drive_info_t  pvd_info;
  
    /* Only look at `unconsumed' or `test spare' drives.
     */
    if (scoobert_is_unused_drive(proposed_disk_p) == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    /* Get the provision drive information.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(proposed_disk_p->bus,
                                                            proposed_disk_p->enclosure,
                                                            proposed_disk_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the drive capacity from pvd object.
     */
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    /* Check if the proposed spare is at a better enclosure proximity compared to
     * the currently selected spare.  The proposed capacity must match the
     * currently selected otherwise a `best fit' match could break this rule.
     */
    if ((pvd_info.capacity < selected_disk_capacity)              &&
        (pvd_info.default_offset == selected_disk_offset)         &&
        (proposed_disk_p->bus  == failed_disk_p->bus)             &&
        (selected_drive_p->enclosure == failed_disk_p->enclosure) &&
        (proposed_disk_p->enclosure  != failed_disk_p->enclosure)    )
    {
        b_better_proximity = FBE_TRUE;
    }

    /* Return if this is a better proximity or not.
     */
    return b_better_proximity;
}
/******************************************************************************
 * end scoobert_is_better_enclosure_capacity()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_find_another_spare_for_capacity()
 *****************************************************************************
 *
 * @brief   This function adds exactly (1) proposed spare drive that is of 
 *          better capacity/proximity. Compared to the current replacement drive these 
 *          values must be the same:
 *              o Offset
 * 
 * @param   rg_config_p - pointer to raid group array
 * @param   test_case_p - pointer to test information
 * @param   drive_locations_p - Pointer to array of all disk locations
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   better_proximity_disk_set_p - Pointer to `better' to populate.
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 * @param   b_bus_proximity - FBE_TRUE - Rule being tested is bus proximity
 * @param   b_encl_proximity - FBE_TRUE - Rule being tested is enclosure proximity
 *
 * @return  bool - FBE_TRUE - Better capacity/location found
 *
 * @author
 *  06/12/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_find_another_spare_for_capacity(fbe_test_rg_configuration_t *rg_config_p,
                                                           scoobert_test_case_t *const test_case_p,
                                                           fbe_test_discovered_drive_locations_t *drive_locations_p,
                                                           fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                           fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                           fbe_test_raid_group_disk_set_t *better_proximity_disk_set_p,
                                                           fbe_lba_t selected_disk_capacity,
                                                           fbe_lba_t selected_disk_offset,
                                                           fbe_bool_t b_bus_proximity,
                                                           fbe_bool_t b_encl_proximity)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      b_better_proximity = FBE_FALSE;
    fbe_test_raid_group_disk_set_t *current_disk_set_p = NULL;
    fbe_u32_t                       drive_cnt =0;
    fbe_u32_t                       drive_index =0;
    fbe_object_id_t                 pvd_id;    
    fbe_job_service_error_type_t    job_service_error_type;
    fbe_test_block_size_t block_size_enum;

    /* Valdiate that only (1) of the following is FBE_TRUE:
     *  o b_bus_proximity
     *  o b_encl_proximity
     */
    if (b_bus_proximity == FBE_TRUE)
    {
        MUT_ASSERT_TRUE(b_encl_proximity == FBE_FALSE);
    }
    else if (b_encl_proximity == FBE_TRUE)
    {
        MUT_ASSERT_TRUE(b_bus_proximity == FBE_FALSE);
    }
    else
    {
        MUT_ASSERT_TRUE((b_bus_proximity == FBE_TRUE) || (b_encl_proximity == FBE_TRUE));
    }

    fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size_enum);
    /* Walk thru all the available drive and located a drive with a better proximity.
     */
    drive_cnt = drive_locations_p->drive_counts[block_size_enum][rg_config_p->drive_type];
    for (drive_index = 0; drive_index < drive_cnt ; drive_index++)
    {
        /* Get the locatin information.
         */
        current_disk_set_p = &drive_locations_p->disk_set[block_size_enum][rg_config_p->drive_type][drive_index];
        
        /* Based on the type of proximity desired, check if this drive is at a
         * better location.
         */
        if (b_bus_proximity == FBE_TRUE)
        {
            b_better_proximity = scoobert_is_better_bus_capacity(current_disk_set_p,
                                                                 failed_disk_p,
                                                                 selected_drive_p,
                                                                 selected_disk_capacity,
                                                                 selected_disk_offset);
        }
        else
        {
            b_better_proximity = scoobert_is_better_enclosure_capacity(current_disk_set_p,
                                                                       failed_disk_p,
                                                                       selected_drive_p,
                                                                       selected_disk_capacity,
                                                                       selected_disk_offset);
        }

        /* If we have located a drive with a better proximity configure it as
         * a `hot-spare'.
         */
        if (b_better_proximity == FBE_TRUE)
        {
            /* Set the better proximity.
             */
            *better_proximity_disk_set_p = *current_disk_set_p;
            status = fbe_api_provision_drive_get_obj_id_by_location(better_proximity_disk_set_p->bus,
                                                            better_proximity_disk_set_p->enclosure,
                                                            better_proximity_disk_set_p->slot,
                                                            &pvd_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Configure this drive as a `hot-spare'.
             */
            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "**** Best spare for capacity/proximity pvd: 0x%x(%d_%d_%d) as hot spare succeed. ****\n",
                       pvd_id, (int)better_proximity_disk_set_p->bus, (int)better_proximity_disk_set_p->enclosure, (int)better_proximity_disk_set_p->slot);
            return b_better_proximity;
        }

    } /* for all drives found */

    /* If we did not find a better match it is a failure.
     */
    MUT_ASSERT_TRUE(b_better_proximity == FBE_TRUE);

    /* Return the better proximity (if found).
     */
    return b_better_proximity;
}
/******************************************************************************
 * end scoobert_find_another_spare_for_capacity()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_test_block_size_sparing_test()
 *****************************************************************************
 *
 * @brief   This functions validates Block Size related rules
 * 
 * @param   rg_config_p - pointer to raid group array
 * @param   test_case_p - pointer to test information
 * @param   drive_locations_p - Pointer to array of all disk locations
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   failed_drive_capacity - Capacity of the failed drive
 * @param   failed_drive_offset - Default offset of the failed drive
 * @param   hs_index - Index to be used as HS
 * @param   configured_hs - HS disk set
 *
 *
 * @author
 *  06/12/2014  Ashok Tamilarasan   - Created
 * 
 *****************************************************************************/
static void scoobert_test_block_size_sparing_test(fbe_test_rg_configuration_t *rg_config_p,
                                             scoobert_test_case_t *const test_case_p,
                                             fbe_test_discovered_drive_locations_t *drive_locations_p,
                                             fbe_test_raid_group_disk_set_t *failed_disk_p,
                                             fbe_lba_t failed_drive_capacity,
                                             fbe_lba_t failed_drive_offset,
                                             fbe_u32_t *hs_index,
                                             fbe_test_hs_configuration_t *configured_hs)
{

    fbe_test_block_size_t next_block_size_enum;
    fbe_test_raid_group_disk_set_t          better_spare_for_block_size = { 0 };
    fbe_test_raid_group_disk_set_t         *better_spare_for_block_size_p = &better_spare_for_block_size;
    fbe_bool_t                              b_better_spare_for_block_size_found = FBE_FALSE;
    fbe_physical_drive_information_t drive_info;
    fbe_object_id_t pdo_id;
    fbe_test_block_size_t block_size_enum;
    fbe_u32_t                               next_drive_index = 0;
    fbe_api_provision_drive_info_t          pvd_info;
    fbe_object_id_t                         pvd_id;
    fbe_test_hs_configuration_t            *hs_config_p = NULL;
    fbe_u32_t                               drive_cnt_next_block_size = 0;
    fbe_test_raid_group_disk_set_t         *next_disk_set_p = NULL;
    fbe_status_t status;
    fbe_job_service_error_type_t job_service_error_type;

    /* The initial pointer to the hot-spare drive information is he first 
     * hot-spare for this test case.
     */
    hs_config_p = &test_case_p->hs_config[0];
    *hs_index = 0;

    fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size_enum);
    next_block_size_enum = block_size_enum == FBE_TEST_BLOCK_SIZE_520 ? FBE_TEST_BLOCK_SIZE_4160 : FBE_TEST_BLOCK_SIZE_520;
    drive_cnt_next_block_size = drive_locations_p->drive_counts[next_block_size_enum][rg_config_p->drive_type];

    for (next_drive_index = 0; next_drive_index < drive_cnt_next_block_size ; next_drive_index++)
    {
        next_disk_set_p = &drive_locations_p->disk_set[next_block_size_enum][rg_config_p->drive_type][next_drive_index];

        /* Only select this drive if it is unused.
         */
        if (scoobert_is_unused_drive(next_disk_set_p))
        {
            /*  get the pvd object id. */
            status = fbe_api_provision_drive_get_obj_id_by_location(next_disk_set_p->bus,
                                                                    next_disk_set_p->enclosure,
                                                                    next_disk_set_p->slot,
                                                                    &pvd_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /*  get the drive capacity from pvd object */
            status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);

            status = fbe_api_get_physical_drive_object_id_by_location(next_disk_set_p->bus, 
                                                                      next_disk_set_p->enclosure, 
                                                                      next_disk_set_p->slot, 
                                                                      &pdo_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK)

            status = fbe_api_physical_drive_get_drive_information(pdo_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            if ((pvd_info.capacity == failed_drive_capacity)            && 
                (pvd_info.default_offset == failed_drive_offset)        &&
                (failed_disk_p->enclosure == next_disk_set_p->enclosure) &&
                (failed_disk_p->bus == next_disk_set_p->bus)             &&
                (rg_config_p->block_size !=  drive_info.block_size  ))
            {
                /*  The `first' spare is not of the same block Size */
                hs_config_p->disk_set = *next_disk_set_p;
                if(test_case_p->test_case == SCOOBERT_SAME_BLOCK_SIZE)
                {
                    b_better_spare_for_block_size_found = scoobert_find_another_spare_for_block_size(rg_config_p, test_case_p, drive_locations_p,
                                                                                                     failed_disk_p, next_disk_set_p, better_spare_for_block_size_p, 
                                                                                                     failed_drive_capacity, failed_drive_offset);
    
                    /* We found another unused drive*/
                    if (b_better_spare_for_block_size_found == FBE_TRUE)
                    {
                        /*  We found a better block size match. Now configure the `first' drive found.
                        */
                        scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the unused pvd as hot spare succeed.    ****");
                        fbe_copy_memory(configured_hs, hs_config_p, sizeof(fbe_test_hs_configuration_t)) ;
                        (*hs_index)++;
                        /* We expect the spare selected to be the `second'*/
                        hs_config_p->disk_set = *better_spare_for_block_size_p;
                        test_case_p->hs_index_to_select = *hs_index;
                        configured_hs++;
                        fbe_copy_memory(configured_hs, hs_config_p, sizeof(fbe_test_hs_configuration_t)) ;
                        (*hs_index)++;
                        return;
                    }
                }
                else
                {
                    /* This test makes sure that if drive with same block size is not available then 
                     * a drive with different block size can swap assumining other rules satisfies
                     */
                    scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                    MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                    mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the unused pvd as hot spare succeed.    ****");
                    fbe_copy_memory(configured_hs, hs_config_p, sizeof(fbe_test_hs_configuration_t)) ;
                    test_case_p->hs_index_to_select = *hs_index;
                    (*hs_index)++;
                    return;
                }

            }
        }

    }
    return;
}
/*!***************************************************************************
 *          scoobert_find_another_spare_for_block_size()
 *****************************************************************************
 *
 * @brief   This function adds exactly (1) proposed spare drive that is of 
 *          the same block size 
 * 
 * @param   rg_config_p - pointer to raid group array
 * @param   test_case_p - pointer to test information
 * @param   drive_locations_p - Pointer to array of all disk locations
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   better_block_size_disk_set_p - Pointer to `better' to populate.
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 *
 * @return  bool - FBE_TRUE - Better capacity/location found
 *
 * @author
 *  06/12/2014  Ashok Tamilarasan  - Created
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_find_another_spare_for_block_size(fbe_test_rg_configuration_t *rg_config_p,
                                                             scoobert_test_case_t *const test_case_p,
                                                             fbe_test_discovered_drive_locations_t *drive_locations_p,
                                                             fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                             fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                             fbe_test_raid_group_disk_set_t *better_block_size_disk_set_p,
                                                             fbe_lba_t selected_disk_capacity,
                                                             fbe_lba_t selected_disk_offset)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_raid_group_disk_set_t *current_disk_set_p = NULL;
    fbe_u32_t                       drive_cnt =0;
    fbe_u32_t                       drive_index =0;
    fbe_object_id_t                 pvd_id;    
    fbe_job_service_error_type_t    job_service_error_type;
    fbe_test_block_size_t block_size_enum;
    fbe_api_provision_drive_info_t  pvd_info;
    fbe_physical_drive_information_t drive_info;
    fbe_object_id_t pdo_id;

    fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size_enum);
    /* Walk thru all the available drive and located a drive with a better proximity.
     */
    drive_cnt = drive_locations_p->drive_counts[block_size_enum][rg_config_p->drive_type];
    for (drive_index = 0; drive_index < drive_cnt ; drive_index++)
    {
        /* Get the locatin information.
         */
        current_disk_set_p = &drive_locations_p->disk_set[block_size_enum][rg_config_p->drive_type][drive_index];

         /* Only look at `unconsumed' or `test spare' drives.
          */
         if (!scoobert_is_unused_drive(current_disk_set_p))
         {
             continue;
         }
        /* Get the provision drive information.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(current_disk_set_p->bus,
                                                                current_disk_set_p->enclosure,
                                                                current_disk_set_p->slot,
                                                                &pvd_id);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK)
        status = fbe_api_get_physical_drive_object_id_by_location(current_disk_set_p->bus, 
                                                                  current_disk_set_p->enclosure, 
                                                                  current_disk_set_p->slot, 
                                                                  &pdo_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK)
                                    
        status = fbe_api_physical_drive_get_drive_information(pdo_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /*  Get the drive capacity from pvd object.
         */
        status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Check if the proposed spare is at a better block size compared to
         * the currently selected spare.  The proposed capacity must be greater than
         * currently selected otherwise a `best fit' match could break this rule.
         */
        if ((pvd_info.capacity >= selected_disk_capacity)      &&
            (pvd_info.default_offset == selected_disk_offset) &&
            (current_disk_set_p->bus == failed_disk_p->bus)  &&
            (current_disk_set_p->enclosure == failed_disk_p->enclosure) &&
            (drive_info.block_size == rg_config_p->block_size     ))
        {
            /* Set the better block size.
             */
            *better_block_size_disk_set_p = *current_disk_set_p;
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Configure this drive as a `hot-spare'.
             */
            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "**** Best spare for Block Size/proximity pvd: 0x%x(%d_%d_%d) as hot spare succeed. ****\n",
                       pvd_id, (int)better_block_size_disk_set_p->bus, (int)better_block_size_disk_set_p->enclosure, (int)better_block_size_disk_set_p->slot);
            return FBE_TRUE;
        }

    } /* for all drives found */

    /* If we did not find a better match it is a failure.
     */
    MUT_FAIL_MSG("Better Block Size not found\n");

    return FBE_FALSE;
}
/******************************************************************************
 * end scoobert_find_another_spare_for_block_size()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_is_better_bus_proximity()
 *****************************************************************************
 *
 * @brief   This function determines if the proposed drive is at a better bus
 *          location than the currently selected drive or not.
 * 
 * @param   proposed_disk_p - Pointer to proposed spare location information
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 *
 * @return  bool - FBE_TRUE - The proposed drive is at a better bus proximity
 *                  than the currently selected drive.
 *
 * @author
 *  06/07/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_is_better_bus_proximity(fbe_test_raid_group_disk_set_t *proposed_disk_p,
                                                   fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                   fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                   fbe_lba_t selected_disk_capacity,
                                                   fbe_lba_t selected_disk_offset)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      b_better_proximity = FBE_FALSE;
    fbe_object_id_t                 pvd_id;    
    fbe_api_provision_drive_info_t  pvd_info;
  
    /* Only look at `unconsumed' or `test spare' drives.
     */
    if (scoobert_is_unused_drive(proposed_disk_p) == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    /* Get the provision drive information.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(proposed_disk_p->bus,
                                                            proposed_disk_p->enclosure,
                                                            proposed_disk_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the drive capacity from pvd object.
     */
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    /* Check if the proposed spare is at a better bus proximity compared to
     * the currently selected spare.  The proposed capacity must match the
     * currently selected otherwise a `best fit' match could break this rule.
     */
    if ((pvd_info.capacity == selected_disk_capacity)     &&
        (pvd_info.default_offset == selected_disk_offset) &&
        (selected_drive_p->bus != failed_disk_p->bus)     &&
        (proposed_disk_p->bus  == failed_disk_p->bus)        )
    {
        b_better_proximity = FBE_TRUE;
    }

    /* Return if this is a better proximity or not.
     */
    return b_better_proximity;
}
/******************************************************************************
 * end scoobert_is_better_bus_proximity()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_is_better_enclosure_proximity()
 *****************************************************************************
 *
 * @brief   This function determines if the proposed drive is at a better
 *          enclosure (on the same bus) location than the currently selected 
 *          drive or not.
 * 
 * @param   proposed_disk_p - Pointer to proposed spare location information
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 *
 * @return  bool - FBE_TRUE - The proposed drive is at a better enclosure 
 *                  proximity than the currently selected drive.
 *
 * @author
 *  06/07/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_is_better_enclosure_proximity(fbe_test_raid_group_disk_set_t *proposed_disk_p,
                                                         fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                         fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                         fbe_lba_t selected_disk_capacity,
                                                         fbe_lba_t selected_disk_offset)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      b_better_proximity = FBE_FALSE;
    fbe_object_id_t                 pvd_id;    
    fbe_api_provision_drive_info_t  pvd_info;
  
    /* Only look at `unconsumed' or `test spare' drives.
     */
    if (scoobert_is_unused_drive(proposed_disk_p) == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    /* Get the provision drive information.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(proposed_disk_p->bus,
                                                            proposed_disk_p->enclosure,
                                                            proposed_disk_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the drive capacity from pvd object.
     */
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    /* Check if the proposed spare is at a better enclosure proximity compared to
     * the currently selected spare.  The proposed capacity must match the
     * currently selected otherwise a `best fit' match could break this rule.
     */
    if ((pvd_info.capacity == selected_disk_capacity)             &&
        (pvd_info.default_offset == selected_disk_offset)         &&
        (proposed_disk_p->bus  == failed_disk_p->bus)             &&
        (selected_drive_p->enclosure != failed_disk_p->enclosure) &&
        (proposed_disk_p->enclosure  == failed_disk_p->enclosure)    )
    {
        b_better_proximity = FBE_TRUE;
    }

    /* Return if this is a better proximity or not.
     */
    return b_better_proximity;
}
/******************************************************************************
 * end scoobert_is_better_enclosure_proximity()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_is_better_slot_proximity()
 *****************************************************************************
 *
 * @brief   This function determines if the proposed drive is at a better
 *          slot (on the same bus, in the same enclosure) location than the 
 *          currently selected drive or not.  This is used to test that a raid
 *          group bound on a system drive should give priority to the system
 *          drives when selecting a spare drive.
 * 
 * @param   proposed_disk_p - Pointer to proposed spare location information
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 *
 * @return  bool - FBE_TRUE - The proposed drive is at a better slot proximity
 *                  than the currently selected drive.
 *
 * @author
 *  06/07/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_is_better_slot_proximity(fbe_test_raid_group_disk_set_t *proposed_disk_p,
                                                    fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                    fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                    fbe_lba_t selected_disk_capacity,
                                                    fbe_lba_t selected_disk_offset)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      b_better_proximity = FBE_FALSE;
    fbe_object_id_t                 pvd_id;    
    fbe_api_provision_drive_info_t  pvd_info;
  
    /* Only look at `unconsumed' or `test spare' drives.
     */
    if (scoobert_is_unused_drive(proposed_disk_p) == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    /* Get the provision drive information.
     */
    status = fbe_api_provision_drive_get_obj_id_by_location(proposed_disk_p->bus,
                                                            proposed_disk_p->enclosure,
                                                            proposed_disk_p->slot,
                                                            &pvd_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the drive capacity from pvd object.
     */
    status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
      
    /* Check if the proposed spare is at a better slot proximity compared to
     * the currently selected spare.  The proposed capacity must match the
     * currently selected otherwise a `best fit' match could break this rule.
     */
    if ((pvd_info.capacity == selected_disk_capacity)               &&
        (proposed_disk_p->bus  == failed_disk_p->bus)               &&
        (proposed_disk_p->enclosure  == failed_disk_p->enclosure)   &&
        (selected_drive_p->slot >= SCOOBERT_NUM_SYSTEM_DRIVE_SLOTS) &&
        (proposed_disk_p->slot  <  SCOOBERT_NUM_SYSTEM_DRIVE_SLOTS)    )
    {
        b_better_proximity = FBE_TRUE;
    }

    /* Return if this is a better proximity or not.
     */
    return b_better_proximity;
}
/******************************************************************************
 * end scoobert_is_better_slot_proximity()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_find_another_spare_for_proximity()
 *****************************************************************************
 *
 * @brief   This function adds exactly (1) proposed spare drive that is of 
 *          better proximity. Compared to the current replacement drive these 
 *          values must be the same:
 *              o Capacity
 *              o Offset
 * 
 * @param   rg_config_p - pointer to raid group array
 * @param   test_case_p - pointer to test information
 * @param   drive_locations_p - Pointer to array of all disk locations
 * @param   failed_disk_p - Pointer to failed disk location information
 * @param   selected_drive_p - Pointer to the the currently selected drive
 *              location.
 * @param   better_proximity_disk_set_p - Pointer to disk set for `better'
 * @param   selected_disk_capacity - Capacity of the currently selected disk.
 *              We must use the same capacity so that the best fit rule(s) do
 *              not apply!
 * @param   selected_disk_offset - Offset of the currently selected disk.
 *              We must use the same offset so that the best offset rule(s) do
 *              not apply!
 * @param   b_bus_proximity - FBE_TRUE - Rule being tested is bus proximity
 * @param   b_encl_proximity - FBE_TRUE - Rule being tested is enclosure proximity
 * @param   b_slot_proximity - FBE_TRUE - Rule being tested is slot proximity
 *
 * @return  bool - FBE_TRUE - Found a better location 
 *
 * @author
 *  06/07/2012  Ron Proulx  - Re-written.
 * 
 *****************************************************************************/
static fbe_bool_t scoobert_find_another_spare_for_proximity(fbe_test_rg_configuration_t *rg_config_p,
                                                                          scoobert_test_case_t *const test_case_p,
                                                                          fbe_test_discovered_drive_locations_t *drive_locations_p,
                                                                          fbe_test_raid_group_disk_set_t *failed_disk_p,
                                                                          fbe_test_raid_group_disk_set_t *selected_drive_p,
                                                                          fbe_test_raid_group_disk_set_t *better_proximity_disk_set_p,
                                                                          fbe_lba_t selected_disk_capacity,
                                                                          fbe_lba_t selected_disk_offset,
                                                                          fbe_bool_t b_bus_proximity,
                                                                          fbe_bool_t b_encl_proximity,
                                                                          fbe_bool_t b_slot_proximity)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      b_better_proximity = FBE_FALSE;
    fbe_test_raid_group_disk_set_t *current_disk_set_p = NULL;
    fbe_u32_t                       drive_cnt =0;
    fbe_u32_t                       drive_index =0;
    fbe_object_id_t                 pvd_id;    
    fbe_job_service_error_type_t    job_service_error_type;
    fbe_test_block_size_t block_size_enum;

    /* Valdiate that only (1) of the following is FBE_TRUE:
     *  o b_bus_proximity
     *  o b_encl_proximity
     *  o b_slot_proximity
     */
    if (b_bus_proximity == FBE_TRUE)
    {
        MUT_ASSERT_TRUE((b_encl_proximity == FBE_FALSE) && (b_slot_proximity == FBE_FALSE));
    }
    else if (b_encl_proximity == FBE_TRUE)
    {
        MUT_ASSERT_TRUE((b_bus_proximity == FBE_FALSE) && (b_slot_proximity == FBE_FALSE));
    }
    else if (b_slot_proximity == FBE_TRUE)
    {
        MUT_ASSERT_TRUE((b_bus_proximity == FBE_FALSE) && (b_encl_proximity == FBE_FALSE));
    }
    else
    {
        MUT_ASSERT_TRUE((b_bus_proximity == FBE_TRUE) || (b_encl_proximity == FBE_TRUE) || (b_slot_proximity == FBE_TRUE));
    }

     fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size_enum);

    /* Walk thru all the available drive and located a drive with a better proximity.
     */
    drive_cnt = drive_locations_p->drive_counts[block_size_enum][rg_config_p->drive_type];
    for (drive_index = 0; drive_index < drive_cnt ; drive_index++)
    {
        /* Get the locatin information.
         */
        current_disk_set_p = &drive_locations_p->disk_set[block_size_enum][rg_config_p->drive_type][drive_index];
        
        /* Based on the type of proximity desired, check if this drive is at a
         * better location.
         */
        if (b_bus_proximity == FBE_TRUE)
        {
            b_better_proximity = scoobert_is_better_bus_proximity(current_disk_set_p,
                                                                  failed_disk_p,
                                                                  selected_drive_p,
                                                                  selected_disk_capacity,
                                                                  selected_disk_offset);
        }
        else if (b_encl_proximity == FBE_TRUE)
        {
            b_better_proximity = scoobert_is_better_enclosure_proximity(current_disk_set_p,
                                                                        failed_disk_p,
                                                                        selected_drive_p,
                                                                        selected_disk_capacity,
                                                                        selected_disk_offset);
        }
        else
        {
            b_better_proximity = scoobert_is_better_slot_proximity(current_disk_set_p,
                                                                   failed_disk_p,
                                                                   selected_drive_p,
                                                                   selected_disk_capacity,
                                                                   selected_disk_offset);
        }

        /* If we have located a drive with a better proximity configure it as
         * a `hot-spare'.
         */
        if (b_better_proximity == FBE_TRUE)
        {
            /* Set the better proximity.
             */
            *better_proximity_disk_set_p = *current_disk_set_p;
            status = fbe_api_provision_drive_get_obj_id_by_location(better_proximity_disk_set_p->bus,
                                                            better_proximity_disk_set_p->enclosure,
                                                            better_proximity_disk_set_p->slot,
                                                            &pvd_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Configure this drive as a `hot-spare'.
             */
            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "**** Best spare for proximity pvd: 0x%x (%d_%d_%d) as hot spare succeed.    ****\n",
                       pvd_id, better_proximity_disk_set_p->bus, better_proximity_disk_set_p->enclosure, better_proximity_disk_set_p->slot);
            return b_better_proximity;
        }

    } /* for all drives found */

    /* If we did not find a better match it is a failure.
     */
    MUT_ASSERT_TRUE(b_better_proximity == FBE_TRUE);

    /* Return the better proximity (if found).
     */
    return b_better_proximity;
}
/******************************************************************************
 * end scoobert_find_another_spare_for_proximity()
 ******************************************************************************/

/*!***************************************************************************
 *          scoobert_discover_all_drive_information()
 *****************************************************************************
 *
 * @brief   The exact same as fbe_test_sep_util_discover_all_drive_information()
 *          EXCEPT that this method allows the system drive to be spared!!
 * 
 * @param   drive_locations_p - pointer to array to populate with discovered drives.
 *
 * @author
 *  06/11/2012  Ron Proulx  -   Updated to allow system drives.
 * 
 *****************************************************************************/
static void scoobert_discover_all_drive_information(fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_handle = 0;
    fbe_u32_t                   port_idx = 0, drive_idx = 0;
    fbe_port_number_t           port_number;
    fbe_u32_t                   total_objects = 0;
    fbe_class_id_t              class_id;
    fbe_object_id_t            *obj_list = NULL;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_u32_t                   board_count = 0, port_count = 0, encl_count = 0, physical_drive_count = 0;
    fbe_drive_type_t            drive_type_enum;
    const fbe_u32_t             READY_STATE_WAIT_TIME_LOCAL = 3000;
    fbe_u32_t                   drive_type = 0;
    fbe_u32_t                   system_drive_missing_count = 0;
    fbe_u32_t                   drive_count_520 = 0, drive_count_512 = 0, drive_count_4096 = 0, drive_count_4160 = 0;

    
    #define MAX_DRIVES_TO_IGNORE 4
    #define MAX_MISSING_SYSTEM_DRIVE    (MAX_DRIVES_TO_IGNORE - (MAX_DRIVES_TO_IGNORE - 1))
    /*! @note First (4) drives are system drives
     */
    /* keep track of first N drives since we do not want to allow creation of configuration on these drives yet.
     */
    fbe_object_id_t pdo_object_id[MAX_DRIVES_TO_IGNORE]; 
    fbe_object_id_t pvd_object_id[MAX_DRIVES_TO_IGNORE]; 

    fbe_zero_memory(drive_locations_p, sizeof(fbe_test_discovered_drive_locations_t));
    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

    /* Today the board is always object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(class_id, FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* We should have exactly one port.
     */
    for (port_idx = 0; port_idx < 8; port_idx++)
    {
        /* For the first port wait for the port and enclosure to come up.
         */
        if (port_idx == 0)
        {
            fbe_u32_t           object_handle;
            /* Get the handle of the port and validate port exists*/
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state*/
            status = fbe_api_wait_for_object_lifecycle_state(object_handle,
                                                              FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME_LOCAL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Use the handle to get port number*/
            status = fbe_api_get_object_port_number(object_handle, &port_number);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(port_number == port_idx);

            /* Verify if the enclosure exists */
            status = fbe_api_get_enclosure_object_id_by_location(port_idx, 0, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_TRUE(object_handle != FBE_OBJECT_ID_INVALID);

            /* Use the handle to get lifecycle state of the enclosure */
            status = fbe_api_wait_for_object_lifecycle_state(object_handle, FBE_LIFECYCLE_STATE_READY, 
                                                             READY_STATE_WAIT_TIME_LOCAL, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            for (drive_idx = 0; drive_idx < MAX_DRIVES_TO_IGNORE; drive_idx++)
            {
                /* Get the pvd object id
                 */
                status = fbe_api_provision_drive_get_obj_id_by_location(port_idx, 0, drive_idx, &pvd_object_id[drive_idx]);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* Validate that at least (MAX_DRIVES_TO_IGNORE - MAX_MISSING_SYSTEM_DRIVE)
                 * are in the Ready state.
                 */
                status = fbe_test_pp_util_verify_pdo_state(port_idx, 0, drive_idx,
                                                           FBE_LIFECYCLE_STATE_READY, READY_STATE_WAIT_TIME_LOCAL);
                if (status != FBE_STATUS_OK)
                {
                    /* We allow (1) degraded system drive.
                     */
                    mut_printf(MUT_LOG_TEST_STATUS, "%s system drive: %d_%d_%d (pvd obj: 0x%x) not in Ready state.", 
                               __FUNCTION__, port_idx, 0, drive_idx, pvd_object_id[drive_idx]);
                    pdo_object_id[drive_idx] = FBE_OBJECT_ID_INVALID;
                    system_drive_missing_count++;
                    MUT_ASSERT_TRUE_MSG((system_drive_missing_count <= MAX_MISSING_SYSTEM_DRIVE), "TOO MANY SYSTEM DISKS MISSING");
                }
                else
                {
                    status = fbe_api_get_physical_drive_object_id_by_location(port_idx, 0, drive_idx, &pdo_object_id[drive_idx]);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                }
            }
        }
        else
        {
            /* Get the handle of the port and validate port exists or not
             */
            status = fbe_api_get_port_object_id_by_location(port_idx, &object_handle);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    /* We do not know exactly how many drives we have.  We assume more than 0 objects total.
     */
    MUT_ASSERT_INT_NOT_EQUAL(total_objects, 0);
    mut_printf(MUT_LOG_LOW, "Total number of object is %d", total_objects);

    obj_list = (fbe_object_id_t *)malloc(sizeof(fbe_object_id_t) * total_objects);
    MUT_ASSERT_TRUE(obj_list != NULL);

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Loop over all the objects in the physical package and
     * determine how many drives there are. 
     */
    for (obj_count = 0; obj_count < object_num; obj_count++)
    {
        fbe_physical_drive_information_t drive_info;
        fbe_test_block_size_t block_size_enum;
        fbe_object_id_t object_id = obj_list[obj_count];

        status = fbe_api_get_object_type(obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK)
        {
            /* If the object is in a bad state just skip it.
             */
            continue;
        }

        switch (obj_type)
        {
            case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
                ++board_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_PORT:
                ++port_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
                ++encl_count;
                break;
            case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
                ++physical_drive_count;

                 /* Attempt to bring the drive up
                  */
                status = fbe_test_sep_util_bring_drive_up(object_id, READY_STATE_WAIT_TIME_LOCAL);
                if (status == FBE_STATUS_OK) 
                {
                    fbe_u32_t drive_index;

                    /* Fetch the bus, enclosure, slot drive information.
                     */
                    status = fbe_api_physical_drive_get_drive_information(object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
                    if (status != FBE_STATUS_OK)
                    {
                        mut_printf(MUT_LOG_LOW, "%s unable to get pdo drive info for object id: 0x%x status: 0x%x", 
                                   __FUNCTION__, object_id, status);
                    }
                    /* Determine block size since we are counting how many of each block size we have, 
                     * and we are saving the b_e_s based on the drive block size. 
                     */
                    status = fbe_test_sep_util_get_block_size_enum(drive_info.block_size, &block_size_enum);
                    if (status != FBE_STATUS_OK) 
                    { 
                        mut_printf(MUT_LOG_LOW, "%s object id: 0x%x block size: %d unexpected", 
                                   __FUNCTION__, object_id, drive_info.block_size);
                    }
    
                    /* Determine drive type.  
                     */
                    drive_type_enum = drive_info.drive_type;
    
                    /*! @note This method does NOT skip the system drives!!!
                     */
                    drive_index = drive_locations_p->drive_counts[block_size_enum][drive_type_enum];
                    status = fbe_test_sep_util_get_pdo_location_by_object_id(object_id,
                                                                             &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].bus,
                                                                             &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].enclosure,
                                                                             &drive_locations_p->disk_set[block_size_enum][drive_type_enum][drive_index].slot);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    drive_locations_p->drive_counts[block_size_enum][drive_type_enum]++;
                }
                break;
            default:
                break;
        }
    }

    mut_printf(MUT_LOG_LOW, "Total number of board is %d", board_count);
    mut_printf(MUT_LOG_LOW, "Total number of port(s) is %d", port_count);
    mut_printf(MUT_LOG_LOW, "Total number of enclosure(s) is %d", encl_count);

    /* Get the total number of 520 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_520 = drive_count_520 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_520][drive_type];
    }

    /* Get the total number of 512 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_512 = drive_count_512 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_512][drive_type];
    } 
    
    /* Get the total number of 4096 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_4096 = drive_count_4096 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4096][drive_type];
    }

    /* Get the total number of 4160 drives */
    for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        drive_count_4160 = drive_count_4160 + drive_locations_p->drive_counts[FBE_TEST_BLOCK_SIZE_4160][drive_type];
    }       
     
    mut_printf(MUT_LOG_LOW, "Total number of physical drives is %d", physical_drive_count);
    mut_printf(MUT_LOG_LOW, "Total number of non-db physical drives by type:  520: %d 512: %d 4096: %d 4160: %d", 
               drive_count_520, 
               drive_count_512,
               drive_count_4096,
               drive_count_4160);

    free(obj_list);

    mut_printf(MUT_LOG_LOW, "=== Configuration verification (with system drives) successful ===");
    return;
}
/******************************************************************************
 * end scoobert_find_another_spare_for_proximity()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_setup_hotspare_for_test_case()
 ******************************************************************************
 * @brief
 *  Try to configure the hot spare with few error cases.
 *
 * @param test_case_p - Pointer to test case to setup
 *
 * @return hs_config_p - Pointer to hs config info
 * 30/08/2011 - Modified Vishnu Sharma
 ******************************************************************************/
static fbe_test_hs_configuration_t *scoobert_setup_hotspare_for_test_case(scoobert_test_case_t *const test_case_p,
                                                                          fbe_test_rg_configuration_t *rg_config_p,
                                                                          fbe_u32_t                   position)
{
    fbe_u32_t                               configured_hs_index = 0;
    static fbe_test_hs_configuration_t      configured_hs[SCOOBERT_TOTAL_HS_TO_CONFIGURE] = { -1 };
    fbe_test_hs_configuration_t            *hs_config_p = NULL;
    fbe_test_discovered_drive_locations_t   drive_locations;
    fbe_drive_type_t                        drive_type;
    fbe_u32_t                               drive_cnt = 0;
    fbe_lba_t                               failed_drive_capacity;
    fbe_lba_t                               failed_drive_offset;
    fbe_lba_t                               required_capacity;
    fbe_test_raid_group_disk_set_t         *disk_set_p = NULL;
    fbe_test_raid_group_disk_set_t          better_spare_for_capacity = { 0 };
    fbe_test_raid_group_disk_set_t         *better_spare_for_capacity_p = &better_spare_for_capacity;
    fbe_test_raid_group_disk_set_t          better_spare_for_location = { 0 };
    fbe_test_raid_group_disk_set_t         *better_spare_for_location_p = &better_spare_for_location;
    fbe_bool_t                              b_better_spare_for_location_found = FBE_FALSE;
    fbe_test_raid_group_disk_set_t         *failed_disk_set_p = NULL;
    fbe_u32_t                               drive_index = 0;
    fbe_api_provision_drive_info_t          pvd_info;
    fbe_object_id_t                         pvd_id;
    fbe_object_id_t                         hs_pvd_id;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_error_type_t            job_service_error_type;
    fbe_test_block_size_t block_size_enum;
   

    /* Validate test index
     */
    MUT_ASSERT_TRUE(position < rg_config_p->width);
    MUT_ASSERT_TRUE(test_case_p->test_case > 0);
    MUT_ASSERT_TRUE(test_case_p->test_case <= SCOOBERT_NUM_TEST_CASES);

    /* The initial pointer to the hot-spare drive information is the first 
     * hot-spare for this test case.
     */
    hs_config_p = &test_case_p->hs_config[0];
    
    /* unconfigure all spare drives from the system 
    */
    if(test_case_p->test_case > SCOOBERT_No_SUITABLE_HS_AVAILABLE)
    {
        fbe_test_sep_util_unconfigure_all_spares_in_the_system();
    }

    /* Determine the following information about the failed drive:
     *  o Minimum capacity required for replacement drive.  This includes:
     *      + Capacity consumed by any LUs bound on raid group
     *      + Any unused capacity on raid group
     *      + Raid group metadata capacity
     *      + Raid group journal capacity (depending on raid type)
     *      = Virtual drive metadata capacity
     *      - Must subtract out metadata capacity required for provision drive
     */
    required_capacity = scoobert_get_replacement_drive_required_capacity(rg_config_p, test_case_p->test_rg_position[0]);
    drive_type = rg_config_p->drive_type;    
    failed_disk_set_p = &rg_config_p->rg_disk_set[position];
    fbe_test_sep_util_get_drive_capacity(failed_disk_set_p, &failed_drive_capacity);
    fbe_test_sep_util_get_drive_offset(failed_disk_set_p, &failed_drive_offset);

    fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size_enum);

    /* Locate all the unused drives in the system.
     */
    scoobert_discover_all_drive_information(&drive_locations);
    drive_cnt = drive_locations.drive_counts[block_size_enum][drive_type];
    for (drive_index = 0; drive_index < drive_cnt ; drive_index++)
    {
        disk_set_p = &drive_locations.disk_set[block_size_enum][drive_type][drive_index];
                
        /* Only select this drive if it is unused.
         */
        if (scoobert_is_unused_drive(disk_set_p))
        {

            /* get the pvd object id. */
            status = fbe_api_provision_drive_get_obj_id_by_location(disk_set_p->bus,
                                                                    disk_set_p->enclosure,
                                                                    disk_set_p->slot,
                                                                    &pvd_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* get the drive capacity from pvd object */
            status = fbe_api_provision_drive_get_info(pvd_id, &pvd_info);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            switch(test_case_p->test_case)
            {
                case SCOOBERT_No_SUITABLE_HS_AVAILABLE:
                     if (pvd_info.capacity < required_capacity)
                     {
                         scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                         MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                         mut_printf(MUT_LOG_TEST_STATUS, "**** No suitable spare - succeed.    ****\n");
                         hs_config_p->disk_set = *disk_set_p;
                         test_case_p->hs_index_to_select = configured_hs_index; 
                         configured_hs[configured_hs_index++] = *hs_config_p; 
                     }
                     break;
                        
                case SCOOBERT_SPARE_LARGER_THAN_ORIGINAL:
                    if ((pvd_info.capacity > failed_drive_capacity)                             &&
                        (pvd_info.default_offset == failed_drive_offset)                        &&
                        (disk_set_p->bus == test_case_p->hs_config[0].disk_set.bus)             &&
                        (disk_set_p->enclosure == test_case_p->hs_config[0].disk_set.enclosure) &&
                        (disk_set_p->slot == test_case_p->hs_config[0].disk_set.slot)              )


                    {
                        scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                        mut_printf(MUT_LOG_TEST_STATUS, "**** Spare larger than original succeed.    ****\n");
                        test_case_p->hs_index_to_select = configured_hs_index ;
                        configured_hs[configured_hs_index++] = *hs_config_p;
                        hs_config_p->disk_set = *disk_set_p;
                        configured_hs[configured_hs_index++] = *hs_config_p;
                    }
                    break;

                case SCOOBERT_SPARE_SMALLER_THAN_ORIGINAL_BUT_BIG_ENOUGH:
                    /*! @note This case must follow SCOOBERT_SPARE_LARGER_THAN_ORIGINAL.
                     */
                    if ((pvd_info.capacity < failed_drive_capacity)                             &&
                        (pvd_info.default_offset == failed_drive_offset)                        &&
                        (pvd_info.capacity >= required_capacity)                                &&
                        (disk_set_p->bus == test_case_p->hs_config[0].disk_set.bus)             &&
                        (disk_set_p->enclosure == test_case_p->hs_config[0].disk_set.enclosure) &&
                        (disk_set_p->slot == test_case_p->hs_config[0].disk_set.slot)              )
                    {
                        scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                        mut_printf(MUT_LOG_TEST_STATUS, "**** Spare spare smaller than original but sufficient succeed.    ****\n");
                        test_case_p->hs_index_to_select = configured_hs_index ;
                        configured_hs[configured_hs_index++] = *hs_config_p;
                        hs_config_p->disk_set = *disk_set_p;
                        configured_hs[configured_hs_index++] = *hs_config_p;
                    }
                    break;

                case SCOOBERT_BUS_IS_HIGHER_PRIORITY_THAN_CAPACITY:
                    if ((pvd_info.capacity < failed_drive_capacity)      &&
                        (pvd_info.default_offset == failed_drive_offset) &&
                        (pvd_info.capacity >= required_capacity)         && 
                        (failed_disk_set_p->bus != disk_set_p->bus)         )
                    {
                        /* We expect the spare with the better location to be selected.*/
                        better_spare_for_capacity_p = disk_set_p;
                        hs_config_p->disk_set = *better_spare_for_capacity_p;
                        b_better_spare_for_location_found = scoobert_find_another_spare_for_capacity(rg_config_p, test_case_p, &drive_locations,
                                                                    failed_disk_set_p, better_spare_for_capacity_p, better_spare_for_location_p,
                                                                    failed_drive_capacity, failed_drive_offset,
                                                                    FBE_TRUE,    /* Find a better bus match*/
                                                                    FBE_FALSE); 
                           
                        /*We found another unused drive*/
                        if (b_better_spare_for_location_found == FBE_TRUE)
                        {
                            /* Configure the `first' matching as hot-spare */
                            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                            mut_printf(MUT_LOG_TEST_STATUS, "**** Bus is higher priority than capacity - succeed.    ****\n");
                            configured_hs[configured_hs_index++] = *hs_config_p;
                            /* The spare we should select is the second.*/
                            hs_config_p->disk_set = *better_spare_for_location_p;
                            test_case_p->hs_index_to_select = configured_hs_index;
                            configured_hs[configured_hs_index++] = *hs_config_p;
                        }
                        
                    }
                    break;    
                
                case SCOOBERT_CAPACITY_IS_HIGHER_PRIORITY_THAN_ENCL:
                    if ((pvd_info.capacity >= failed_drive_capacity)            &&
                        (pvd_info.default_offset == failed_drive_offset)        &&
                        (pvd_info.capacity >= required_capacity)                && 
                        (failed_disk_set_p->bus == disk_set_p->bus)             &&
                        (failed_disk_set_p->enclosure == disk_set_p->enclosure)    )
                    {
                        /* We expect the spare with the better capacity to be selected.*/
                        better_spare_for_capacity_p = disk_set_p;
                        hs_config_p->disk_set = *better_spare_for_capacity_p;
                        b_better_spare_for_location_found = scoobert_find_another_spare_for_capacity(rg_config_p, test_case_p, &drive_locations,
                                                                    failed_disk_set_p, better_spare_for_capacity_p, better_spare_for_location_p,
                                                                    failed_drive_capacity, failed_drive_offset,
                                                                    FBE_FALSE,
                                                                    FBE_TRUE     /* Capacity is higher priority than enclosure*/);

                        /*We found another unused drive*/
                        if (b_better_spare_for_location_found == FBE_TRUE)
                        {
                            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                            mut_printf(MUT_LOG_TEST_STATUS, "**** Capacity is higher priority than enclosure - succeed.    ****\n");
                            /* We select the `first' which has better capacity*/
                            test_case_p->hs_index_to_select = configured_hs_index ;
                            configured_hs[configured_hs_index++] = *hs_config_p;
                            hs_config_p->disk_set = *better_spare_for_location_p;
                            configured_hs[configured_hs_index++] = *hs_config_p;
                        }
                        
                    }
                    break;
                        
                case SCOOBERT_BUS_PROXIMITY:
                    if ((pvd_info.capacity == failed_drive_capacity)     && 
                        (pvd_info.default_offset == failed_drive_offset) &&
                        (failed_disk_set_p->bus != disk_set_p->bus)         )
                    {
                        /* The first spare we find is not on the same bus. */
                        hs_config_p->disk_set = *disk_set_p;
                        b_better_spare_for_location_found = scoobert_find_another_spare_for_proximity(rg_config_p, test_case_p, &drive_locations,
                                                                                      failed_disk_set_p, disk_set_p, better_spare_for_location_p,
                                                                                      failed_drive_capacity, failed_drive_offset,
                                                                                      FBE_TRUE,    /* Find a better bus match*/
                                                                                      FBE_FALSE,    
                                                                                      FBE_FALSE);

                        /* We found another unused drive*/
                        if (b_better_spare_for_location_found == FBE_TRUE)
                        {
                            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                            mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the unused pvd as hot spare succeed. ****");
                            configured_hs[configured_hs_index++] = *hs_config_p;
                            hs_config_p->disk_set = *better_spare_for_location_p;
                            /* We expect the `second' to be better.*/
                            test_case_p->hs_index_to_select = configured_hs_index;
                            configured_hs[configured_hs_index++] = *hs_config_p;
                        }
                        
                    }
                    break;     
                     
                case SCOOBERT_ENCL_PROXIMITY:
                    if ((pvd_info.capacity == failed_drive_capacity)            && 
                        (pvd_info.default_offset == failed_drive_offset)        &&
                        (failed_disk_set_p->enclosure != disk_set_p->enclosure) &&
                        (failed_disk_set_p->bus == disk_set_p->bus)                )
                    {
                        /* The `first' spare is not in the same enclosure */
                        hs_config_p->disk_set = *disk_set_p;
                        b_better_spare_for_location_found = scoobert_find_another_spare_for_proximity(rg_config_p, test_case_p, &drive_locations,
                                                                                      failed_disk_set_p, disk_set_p, better_spare_for_location_p, 
                                                                                      failed_drive_capacity, failed_drive_offset,
                                                                                      FBE_FALSE,
                                                                                      FBE_TRUE,    /* Find a better encl match*/
                                                                                      FBE_FALSE);

                        /* We found another unused drive*/
                        if (b_better_spare_for_location_found == FBE_TRUE)
                        {
                            /* We found a better enclosure match. Now configure the `first' drive found.
                             */
                            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                            mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the unused pvd as hot spare succeed.    ****");
                            configured_hs[configured_hs_index++] = *hs_config_p;
                            /* We expect the spare selected to be the `second'*/
                            hs_config_p->disk_set = *better_spare_for_location_p;
                            test_case_p->hs_index_to_select = configured_hs_index;
                            configured_hs[configured_hs_index++] = *hs_config_p;
                        }
                        
                    }
                    break;

                case SCOOBERT_SELECT_NON_SYSTEM_DRIVE:
                    if ((pvd_info.capacity >= failed_drive_capacity)            && 
                        (failed_disk_set_p->bus == disk_set_p->bus)             &&
                        (disk_set_p->enclosure  == 0)                           &&
                        (disk_set_p->slot >= SCOOBERT_NUM_SYSTEM_DRIVE_SLOTS)      )
                    {
                        /* First configure the test case (0_0_1) as a hot-spare.*/
                        status = fbe_api_provision_drive_get_obj_id_by_location(hs_config_p->disk_set.bus,
                                                                                hs_config_p->disk_set.enclosure,
                                                                                hs_config_p->disk_set.slot,
                                                                                &hs_pvd_id);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        scoobert_test_configure_hot_spare(test_case_p, hs_pvd_id, &job_service_error_type);
                        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the system pvd as hot spare succeed.    ****\n");
                        configured_hs[configured_hs_index++] = *hs_config_p;

                        /* The `first' spare found is not a system drive */
                        scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the non-system pvd as hot spare succeed.    ****\n");
                        /* We expect the non-system drive to be selected. */
                        test_case_p->hs_index_to_select = configured_hs_index;
                        hs_config_p->disk_set = *disk_set_p;
                        configured_hs[configured_hs_index++] = *hs_config_p;
                    }
                    break;

                case SCOOBERT_SELECT_SYSTEM_DRIVE:
                    if ((block_size_enum == FBE_TEST_BLOCK_SIZE_520) &&
                        (pvd_info.capacity == failed_drive_capacity)            && 
                        (failed_disk_set_p->bus == disk_set_p->bus)             &&
                        (failed_disk_set_p->enclosure == disk_set_p->enclosure) &&
                        (disk_set_p->slot >= SCOOBERT_NUM_SYSTEM_DRIVE_SLOTS)      )
                    {
                        /* The `first' spare found is not a system drive */
                        hs_config_p->disk_set = *disk_set_p;
                        b_better_spare_for_location_found = scoobert_find_another_spare_for_proximity(rg_config_p, test_case_p, &drive_locations,
                                                                                      failed_disk_set_p, disk_set_p, better_spare_for_location_p,
                                                                                      failed_drive_capacity, failed_drive_offset,
                                                                                      FBE_FALSE,    
                                                                                      FBE_FALSE,    
                                                                                      FBE_TRUE     /* Find a better slot match*/);

                        /*We found another unused drive*/
                        if (b_better_spare_for_location_found == FBE_TRUE)
                        {
                            scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                            MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                            mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the unused pvd as hot spare succeed.    ****\n");
                            configured_hs[configured_hs_index++] = *hs_config_p;
                            /* We expecte the `second' spare to be selected*/
                            hs_config_p->disk_set = *better_spare_for_location_p;
                            test_case_p->hs_index_to_select = configured_hs_index;
                            configured_hs[configured_hs_index++] = *hs_config_p;
                        }
                        
                    }
                    break;

                case SCOOBERT_NO_LUNS_BOUND:
                    /* Unbind all the lun on the raid group.
                     */
                     if (pvd_info.capacity >= failed_drive_capacity)
                     {
                         scoobert_test_configure_hot_spare(test_case_p, pvd_id, &job_service_error_type);
                         MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
                         hs_config_p->disk_set = *disk_set_p;
                         test_case_p->hs_index_to_select = configured_hs_index; 
                         configured_hs[configured_hs_index++] = *hs_config_p;
                         status = scoobert_setup_unbind_all_luns(test_case_p, rg_config_p);                        
                         MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                         mut_printf(MUT_LOG_TEST_STATUS, "**** No luns bound - succeed.    ****\n");
                     }
                     break;

                case SCOOBERT_SAME_BLOCK_SIZE:
                case SCOOBERT_SAME_BLOCK_SIZE_UNAVAILABLE:
                    scoobert_test_block_size_sparing_test(rg_config_p, test_case_p, &drive_locations,
                                                          failed_disk_set_p, failed_drive_capacity, 
                                                          failed_drive_offset, &configured_hs_index,
                                                          &configured_hs[0]);
                    break;
                default:    
                    /* Unsupported test case.*/  
                    status = FBE_STATUS_GENERIC_FAILURE;                                
                    mut_printf(MUT_LOG_TEST_STATUS, "*** %s Unsupported test case: %d(%s) ****",
                               __FUNCTION__, test_case_p->test_case, test_case_p->test_case_name);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return NULL;               

            }//End of Switch

            /*We have found the desired unused drives to configure as spare
             So break the  loop*/
            if (configured_hs_index > 0)
            {
                    break;
            }
        }//End of if
        
    }//End of for loop
    
    
    /* Else validate spare selection index
     */
    if (test_case_p->hs_index_to_select != FBE_U32_MAX)
    {
        hs_config_p = &configured_hs[test_case_p->hs_index_to_select];
        MUT_ASSERT_FALSE(hs_config_p->disk_set.bus == FBE_U32_MAX);
        MUT_ASSERT_FALSE(hs_config_p->disk_set.enclosure == FBE_U32_MAX);
        MUT_ASSERT_FALSE(hs_config_p->disk_set.slot == FBE_U32_MAX);
    }
    else
    {
        return NULL;
    }

    /* Return the hs selected
     */
    return(hs_config_p);
}
/***********************************************
 * end scoobert_setup_hotspares_for_test_case()
 ***********************************************/

/*!****************************************************************************
 *          scoobert_reinsert_removed_disks()
 ******************************************************************************
 * @brief
 *   This function reinsert the removed  drive and waits for the logical and physical
 *   drive objects to be ready.  It does not check the states of the VD .
 *     *
 * @param rg_config_p           - pointer to raid group
 * @param removed_disk_set_p    - pointer to array
 * @param position_p            - pointer to position
 * @param number_of_drives      - number of drives removed
 * 
 * @return none.
 *
 ******************************************************************************/
static void scoobert_reinsert_removed_disks(fbe_test_rg_configuration_t *rg_config_p,
                                            fbe_test_raid_group_disk_set_t*removed_disk_set_p,
                                            fbe_u32_t *position_p,
                                            fbe_u32_t  number_of_drives)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_object_id_t rg_object_id;
    fbe_u32_t       drive_index = 0;
    fbe_u32_t       removed_position;
    
    /* get the raid group object id*/
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* For all removed positions.
     */
    for ( drive_index = 0; drive_index < number_of_drives; drive_index++ )
    {

        /* validate that this is a valid position*/
        removed_position = position_p[drive_index];
        MUT_ASSERT_TRUE((removed_position != SCOOBERT_INVALID_DISK_POSITION) &&
                        (removed_position < rg_config_p->width)                 );

        mut_printf(MUT_LOG_TEST_STATUS, "== %s pos: %d removed drive (%d_%d_%d). ==", 
                   __FUNCTION__, removed_position,
                   removed_disk_set_p[drive_index].bus, 
                   removed_disk_set_p[drive_index].enclosure,
                   removed_disk_set_p[drive_index].slot);

        /* insert unused drive back */
        if(fbe_test_util_is_simulation())
        {
            status = fbe_test_pp_util_reinsert_drive(removed_disk_set_p[drive_index].bus, 
                                            removed_disk_set_p[drive_index].enclosure, 
                                            removed_disk_set_p[drive_index].slot,
                                            rg_config_p->drive_handle[position_p[drive_index]]);
            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                status = fbe_test_pp_util_reinsert_drive(removed_disk_set_p[drive_index].bus, 
                                                        removed_disk_set_p[drive_index].enclosure, 
                                                        removed_disk_set_p[drive_index].slot,
                                                        rg_config_p->peer_drive_handle[position_p[drive_index]]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
            }
        }
        else
        {
            status = fbe_test_pp_util_reinsert_drive_hw(removed_disk_set_p[drive_index].bus, 
                                            removed_disk_set_p[drive_index].enclosure, 
                                            removed_disk_set_p[drive_index].slot);
        }
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* wait for the unused pvd object to be in ready state */
        status = fbe_test_sep_util_wait_for_pvd_state(removed_disk_set_p[drive_index].bus, 
                                removed_disk_set_p[drive_index].enclosure, 
                                removed_disk_set_p[drive_index].slot,
                                FBE_LIFECYCLE_STATE_READY,
                                SCOOBERT_TEST_WAIT_MSEC);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for the drive to be rebuilt. */
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish ==", __FUNCTION__);
        if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_api_base_config_downstream_object_list_t downstream_object_list;

            /* For Raid 10, we need to get the object id of the mirror.
             * Issue the control code to RAID group to get its downstream edge list. 
             */  
            fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

            /* The first and second positions to remove will be considered to land in the first pair.
             */
            status = fbe_test_sep_util_wait_rg_pos_rebuild(downstream_object_list.downstream_object_list[removed_position / 2], 
                                                           removed_position % 2,
                                                           SCOOBERT_TEST_WAIT_MSEC   /* Time to wait */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            mut_printf(MUT_LOG_TEST_STATUS, "waiting for rebuild rg_id: %d obj: 0x%x position: %d",
                       rg_config_p->raid_group_id, rg_object_id, removed_position);
            status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, removed_position,
                                                           SCOOBERT_TEST_WAIT_MSEC /* Time to wait */);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish - Successful ==", __FUNCTION__);
    }

}
/***********************************************
 * end scoobert_reinsert_removed_disks()
 ***********************************************/

/*!**************************************************************
 * scoobert_discover_raid_group_disk_set()
 ****************************************************************
 * @brief
 *  Given an input raid group an an input set of disks,
 *  determine which disks to use for this raid group configuration.
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
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t scoobert_discover_raid_group_disk_set(fbe_test_rg_configuration_t * raid_configuration_list_p,
                                                        fbe_u32_t raid_group_count,
                                                        fbe_test_discovered_drive_locations_t *drive_locations_p)
{
   /* For a given configuration fill in the actual drives we will
    * be using.  This function makes it easy to create flexible
    * configurations without having to know exactly which disk goes
    * in which raid group.  We first create a set of disk sets for
    * the 512 and 520 disks.  Then we call this function and
    * pick drives from those disk sets to use in our raid groups.
     */
    fbe_status_t status;
    fbe_u32_t rg_index;
    fbe_u32_t rg_disk_index;
    fbe_u32_t local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_drive_type_t        drive_type;
    fbe_test_block_size_t   block_size;
    fbe_bool_t              b_disks_found;
    fbe_test_raid_group_disk_set_t * disk_set_p = NULL;
    fbe_bool_t              b_is_drive_in_use = FBE_FALSE;

    fbe_copy_memory(&local_drive_counts[0][0], &drive_locations_p->drive_counts[0][0], (sizeof(fbe_u32_t) * FBE_TEST_BLOCK_SIZE_LAST * FBE_DRIVE_TYPE_LAST));


    /* For every raid group, fill in the entire disk set by using our static 
     * set or disk sets for 512 and 520. 
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_test_sep_util_get_block_size_enum(raid_configuration_list_p[rg_index].block_size, &block_size);
        if (raid_configuration_list_p[rg_index].magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
        {
            /* Must initialize the rg configuration structure.
             */
            fbe_test_sep_util_init_rg_configuration(&raid_configuration_list_p[rg_index]);
        }
        if (!fbe_test_rg_config_is_enabled(&raid_configuration_list_p[rg_index]))
        {
            continue;
        }

        /* Discover the Drive type we will use for this RG
         */
        b_disks_found = false;
        for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
        {
            if ((raid_configuration_list_p[rg_index].width + raid_configuration_list_p[rg_index].num_of_extra_drives) 
                                                                            <= local_drive_counts[block_size][drive_type])
            {
                raid_configuration_list_p[rg_index].drive_type = drive_type;
                for (rg_disk_index = 0; rg_disk_index < raid_configuration_list_p[rg_index].width; rg_disk_index++)
                {
                    disk_set_p = NULL;
                    //local_drive_counts[block_size][drive_type]--;

                    /* The drives we want to use have been populated
                     */
                    //raid_configuration_list_p[rg_index].rg_disk_set[rg_disk_index] = *disk_set_p;
                }

                /* populate required extra disk set if extra drive count is set by consumer test
                 */
                for (rg_disk_index = 0; rg_disk_index < raid_configuration_list_p[rg_index].num_of_extra_drives; rg_disk_index++)
                {
                    /* Skip over in-use drives
                     */
                    b_is_drive_in_use = FBE_TRUE;
                    while ((b_is_drive_in_use)                              &&
                           (local_drive_counts[block_size][drive_type] > 0)    )
                    {
                        disk_set_p = NULL;
                        local_drive_counts[block_size][drive_type]--;

                        /* Fill in the next drive.
                         */
                        status = fbe_test_sep_util_get_next_disk_in_set(local_drive_counts[block_size][drive_type], 
                                                                        drive_locations_p->disk_set[block_size][drive_type],
                                                                        &disk_set_p);
                        if (status != FBE_STATUS_OK)
                        {
                            return status;
                        }

                        /* Check if this drive is in use for any of the configured
                         * raid groups.
                         */
                        if (scoobert_is_unused_drive(disk_set_p) == FBE_TRUE)
                        {
                            raid_configuration_list_p[rg_index].extra_disk_set[rg_disk_index] = *disk_set_p;
                            b_disks_found = true;
                            b_is_drive_in_use = FBE_FALSE;
                        }
                    } /* while the selected drive is in-use*/
                } /* for all the extra disks*/

            } /* if there enough unused drives*/

        } /* for all drive types */

        /* if we didn't find sufficient drives report an error */
        if (b_disks_found == false)
        {
            /* We did make sure we had enough drives before entering this function
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}
/***********************************************
 * end scoobert_discover_raid_group_disk_set()
 ***********************************************/

/*!**************************************************************
 * scoobert_setup_rg_config()
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
 *  3/30/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t scoobert_setup_rg_config(fbe_test_rg_configuration_t * rg_config_p,
                                               fbe_test_discovered_drive_locations_t *drive_locations_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   rg_index = 0;
    fbe_test_rg_configuration_t * const         orig_rg_config_p = rg_config_p;
    fbe_u32_t local_drive_counts[FBE_TEST_BLOCK_SIZE_LAST][FBE_DRIVE_TYPE_LAST];
    fbe_u32_t raid_group_count = 0;
    fbe_test_block_size_t   block_size;
    fbe_drive_type_t        drive_type;

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
        status = fbe_test_sep_util_get_block_size_enum(rg_config_p->block_size, &block_size);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

        if (!rg_config_p->b_already_tested)
        {
            /* Check if we can find enough drives of same drive type
             */
            rg_config_p->b_create_this_pass = FBE_FALSE;
            rg_config_p->b_valid_for_physical_config = FBE_FALSE;

            for (drive_type = 0; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
            {
                /* consider extra drives into account if count is set by consumer test
                 */
                if ((rg_config_p->width + rg_config_p->num_of_extra_drives) <= local_drive_counts[block_size][drive_type])
                {
                    /* We will use this in the current pass. 
                     * Mark it as used so that the next time through we will not create it. 
                     */
                    rg_config_p->b_create_this_pass = FBE_TRUE;
                    rg_config_p->b_already_tested = FBE_TRUE;
                    rg_config_p->b_valid_for_physical_config = FBE_TRUE;
                    local_drive_counts[block_size][drive_type] -= rg_config_p->width;
                    local_drive_counts[block_size][drive_type] -= rg_config_p->num_of_extra_drives;
                    break;
                }
            }
        }
        else
        {
            rg_config_p->b_create_this_pass = FBE_FALSE;
        }

        rg_config_p++;
    }

    if (fbe_test_util_is_simulation())
    {
        status = scoobert_discover_raid_group_disk_set(orig_rg_config_p,
                                                    raid_group_count,
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
        status = FBE_STATUS_GENERIC_FAILURE;
        //fbe_test_sep_util_discover_raid_group_disk_set_with_capacity(orig_rg_config_p,
        //                                            raid_group_count,
        //                                            drive_locations_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /*! @todo really we should change the below function to take a fbe_test_drive_locations_t structure.
     */

    /* This function will update unused drive info for those raid groups
       which are going to be used in this pass
    */
    fbe_test_sep_util_set_unused_drive_info(orig_rg_config_p,
                                            raid_group_count,
                                            local_drive_counts,
                                            drive_locations_p);

    return status;
}
/******************************************
 * end scoobert_setup_rg_config()
 ******************************************/

/*!**************************************************************
 * scoobert_run_test_on_rg_config()
 ****************************************************************
 * @brief
 *  Run a test on a set of raid group configurations.
 *  This function creates the configs, runs the test and destroys
 *  the configs.
 * 
 *  We will run this using whichever drives we find in the system.
 *  If there are not enough drives to create all the configs, then
 *  we will create a set of configs, run the test, destroy the configs
 *  and then run another test.
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
 *  5/16/2011 - Created. Rob Foley
 *
 ****************************************************************/
static void scoobert_run_test_on_rg_config(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun)
{
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_u32_t raid_group_count_this_pass = 0;
    fbe_u32_t raid_group_count = 0;

    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the arry of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* Get the raid group config for the first pass.
     */
    scoobert_setup_rg_config(rg_config_p, &drive_locations);
    raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);

    /* Continue to loop until we have no more raid groups.
     */
    while (raid_group_count_this_pass > 0)
    {
        fbe_u32_t debug_flags;
        fbe_u32_t trace_level = FBE_TRACE_LEVEL_INFO;
        mut_printf(MUT_LOG_LOW, "testing %d raid groups \n", raid_group_count_this_pass);
        /* Create the raid groups.
         */
        raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

        /* Setup the lun capacity with the given number of chunks for each lun.
         */
        fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, raid_group_count,
                                                          chunks_per_lun, 
                                                          luns_per_rg);

        /* Kick of the creation of all the raid group with Logical unit configuration.
         */
        fbe_test_sep_util_create_raid_group_configuration(rg_config_p, raid_group_count);
        
        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_trace_level", &trace_level))
        {
            fbe_test_sep_util_set_rg_trace_level_both_sps(rg_config_p, (fbe_trace_level_t) trace_level);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_group_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_debug_flags_both_sps(rg_config_p, (fbe_raid_group_debug_flags_t)debug_flags);
        }
        if (fbe_test_sep_util_get_cmd_option_int("-raid_library_debug_flags", &debug_flags))
        {
            fbe_test_sep_util_set_rg_library_debug_flags_both_sps(rg_config_p, (fbe_raid_library_debug_flags_t)debug_flags);
        }
        /* With this option we will skip the test, we just create/destroy the configs.
         */
        mut_pause();
        if (fbe_test_sep_util_get_cmd_option("-just_create_config"))
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s -just_create_config selected, skipping test\n", __FUNCTION__);
        }
        else
        {
            fbe_u32_t repeat_count = 1;
            fbe_u32_t loop_count = 0;
            /* The default number of times to run is 1, -repeat_count can alter this.
             */
            fbe_test_sep_util_get_cmd_option_int("-repeat_count", &repeat_count);
            /* Run the tests some number of times.
             * Note it is expected that a 0 repeat count will cause us to not run the test.
             */
            while (loop_count < repeat_count)
            {
                test_fn(rg_config_p, context_p);
                loop_count++;
                /* Only trace if the user supplied a repeat count.
                 */
                if (repeat_count > 0)
                {
                    mut_printf(MUT_LOG_TEST_STATUS, "test iteration [%d of %d] completed", loop_count, repeat_count);
                }
            }
        }

        /* Check for errors and destroy the rg config.
         */
        fbe_test_sep_util_destroy_raid_group_configuration(rg_config_p, raid_group_count);

        /* Setup the rgs for the next pass.
         */
        scoobert_setup_rg_config(rg_config_p, &drive_locations);
        raid_group_count_this_pass = fbe_test_get_rg_count(rg_config_p);
    }

    fbe_test_sep_util_report_created_rgs(rg_config_p);

    /* Done testing rgconfigs. Mark them invalid.
     */
    fbe_test_rg_config_mark_invalid(rg_config_p);
    return;
}
/******************************************
 * end scoobert_run_test_on_rg_config()
 ******************************************/

/*!**************************************************************
 * scoobert_run_test_on_rg_config_with_extra_disk()
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
void scoobert_run_test_on_rg_config_with_extra_disk(fbe_test_rg_configuration_t *rg_config_p,
                                    void * context_p,
                                    fbe_test_rg_config_test test_fn,
                                    fbe_u32_t luns_per_rg,
                                    fbe_u32_t chunks_per_lun)
{

    if (rg_config_p->magic != FBE_TEST_RG_CONFIGURATION_MAGIC)
    {
        /* Must initialize the arry of rg configurations.
         */
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
    }

    /* populate the required extra disk count in rg config */
    fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
     
    /* Generally, the extra disks on rg will be configured as hot spares. In order to make configuring 
    particular PVDs(extra disks) as hot spares work, we need to set set automatic hot spare not working (set to false). */    
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* run the test */
    scoobert_run_test_on_rg_config(rg_config_p, context_p, test_fn,
                                   luns_per_rg,
                                   chunks_per_lun);

    return;
}
/******************************************************
 * end scoobert_run_test_on_rg_config_with_extra_disk()
 ******************************************************/

/*!****************************************************************************
 * scoobert_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups to test the selection algo.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  24/08/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void scoobert_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &scoobert_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,scoobert_run_tests,
                                   SCOOBERT_TEST_LUNS_PER_RAID_GROUP,
                                   SCOOBERT_TEST_CHUNKS_PER_LUN);
    return;    

}
/****************************
 * end scoobert_test()
 ****************************/

/*!****************************************************************************
 * scoobert_4k_test()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups to test the selection algo.
 *  for 4K block size drives.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  06/17/2014 - Created. Ashok Tamilarasan
 ******************************************************************************/
void scoobert_4k_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &scoobert_raid_group_config[0];
    
    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,scoobert_run_tests,
                                   SCOOBERT_TEST_LUNS_PER_RAID_GROUP,
                                   SCOOBERT_TEST_CHUNKS_PER_LUN);
    return;    

}
/****************************
 * end scoobert_4k_test()
 ****************************/

/*!****************************************************************************
 * scoobert_test_spare_library_callback_function()
 ******************************************************************************
 * @brief
 *  Callback function for the spare library notification.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/29/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void scoobert_test_spare_library_callback_function(update_object_msg_t * update_object_msg,
                                                  void *context2)
{
    scoobert_test_spare_lib_ns_context_t *    spare_lib_ns_context_p = (scoobert_test_spare_lib_ns_context_t * )context2;

    /* Release the semaphore only when specific vd object id sends job
     * state change notification.
     */
    if(spare_lib_ns_context_p->object_id == update_object_msg->object_id)
    {
        /* update the error information before releasing the semaphore. */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: received spare library notification, vd_obj_id:0x%x.",
                   __FUNCTION__, update_object_msg->object_id);
        spare_lib_ns_context_p->js_error_info = update_object_msg->notification_info.notification_data.job_service_error_info;
        fbe_semaphore_release(&spare_lib_ns_context_p->sem, 0, 1, FALSE);
    }
    return ;
}
/******************************************************************************
 * end scoobert_test_spare_library_callback_function()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_register_spare_lib_notification()
 ******************************************************************************
 * @brief
 *  It is used to register the spare library notification.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/29/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void scoobert_test_register_spare_lib_notification(fbe_object_id_t vd_object_id,
                                                 scoobert_test_spare_lib_ns_context_t * spare_lib_ns_context_p,
                                                 fbe_notification_registration_id_t * notification_reg_id_p)
{
    fbe_status_t                                status;

    /* register with notification service for the job state change. */
    spare_lib_ns_context_p->object_id = vd_object_id;
    fbe_semaphore_init(&(spare_lib_ns_context_p->sem), 0, 1);

    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE,
                                                                  scoobert_test_spare_library_callback_function,
                                                                  spare_lib_ns_context_p,
                                                                  notification_reg_id_p);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    return;
}
/******************************************************************************
 * end scoobert_test_register_spare_lib_notification()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_wait_for_spare_lib_notification()
 ******************************************************************************
 * @brief
 *  It is used to wait for the spare library notification.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
*  09/29/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void scoobert_test_wait_for_spare_lib_notification(scoobert_test_spare_lib_ns_context_t * spare_lib_ns_context_p)
{
    EMCPAL_STATUS status;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Waiting for the spare library to send notification.", __FUNCTION__);
    status = fbe_semaphore_wait(&(spare_lib_ns_context_p->sem), NULL);
    MUT_ASSERT_INT_EQUAL(status, EMCPAL_STATUS_SUCCESS);
    return;
}
/******************************************************************************
 * end scoobert_test_wait_for_spare_lib_notification()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_unregister_spare_lib_notification()
 ******************************************************************************
 * @brief
 *  It is used to unregister the spare library notification.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/29/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void scoobert_test_unregister_spare_lib_notification(fbe_object_id_t vd_object_id,
                                                   struct scoobert_test_spare_lib_ns_context_s * spare_lib_ns_context_p,
                                                   fbe_notification_registration_id_t notification_reg_id)
{
    fbe_status_t                                status;

    status = fbe_api_notification_interface_unregister_notification(scoobert_test_spare_library_callback_function,
                                                                    notification_reg_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_semaphore_destroy(&spare_lib_ns_context_p->sem); /* SAFEBUG - needs destroy */

    return;
}
/******************************************************************************
 * end scoobert_test_unregister_spare_lib_notification()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_spare_config_callback_function()
 ******************************************************************************
 * @brief
 *  Callback function for the spare configuration notification.
 *
 * @param update_object_msg_p   - pointer to the object that sent notification 
 * @param context_p             - pointer to the notification context 
 *
 * @return None.
 *
 * @author
 *  03/2011 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static void scoobert_test_spare_config_callback_function(update_object_msg_t*    update_object_msg_p,
                                                void*                   context_p)
{
    scoobert_test_spare_lib_ns_context_t*    spare_config_ns_context_p;

    /* Get a pointer to the context */
    spare_config_ns_context_p = (scoobert_test_spare_lib_ns_context_t * )context_p;

    /* Release the semaphore only when specific object id sends configuration change notification */
    if(spare_config_ns_context_p->object_id == update_object_msg_p->object_id)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: received spare configuration change notification, obj_id:0x%x.",
                   __FUNCTION__, update_object_msg_p->object_id);
        fbe_semaphore_release(&spare_config_ns_context_p->sem, 0, 1, FALSE);
    }

    return;
}
/******************************************************************************
 * end scoobert_test_spare_config_callback_function()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_wait_for_config_notification()
 ******************************************************************************
 * @brief
 *  It is used to wait for the Configuration Service notification.
 *
 * @param spare_config_ns_context_p   - pointer to the context  
 *
 * @return None.
 *
 * @author
*  03/2011 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static void scoobert_test_wait_for_config_notification(scoobert_test_spare_lib_ns_context_t * spare_config_ns_context_p)
{
    EMCPAL_STATUS status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Waiting for the Configuration Service to send notification.", __FUNCTION__);
    status = fbe_semaphore_wait(&(spare_config_ns_context_p->sem), NULL);
    MUT_ASSERT_INT_EQUAL(status, EMCPAL_STATUS_SUCCESS);

    return;
}
/******************************************************************************
 * end scoobert_test_wait_for_config_notification()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_unregister_config_notification()
 ******************************************************************************
 * @brief
 *  It is used to unregister the Configuration Service notification.
 *
 * @param notification_reg_id   - notification registration ID  
 *
 * @return None.
 *
 * @author
 *  03/2011 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static void scoobert_test_unregister_config_notification(fbe_notification_registration_id_t  notification_reg_id)
{
    fbe_status_t    status;

    status = fbe_api_notification_interface_unregister_notification(scoobert_test_spare_config_callback_function,
                                                                    notification_reg_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
}
/******************************************************************************
 * end scoobert_test_unregister_config_notification()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_setup_hot_spare()
 ******************************************************************************
 * @brief
 *  It is used to setup hot spare.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 ******************************************************************************/
static void scoobert_test_setup_hot_spare(fbe_test_hs_configuration_t * hs_config_p)
{  
    scoobert_test_spare_lib_ns_context_t  spare_config_ns_context;
    fbe_notification_registration_id_t  ns_registration_id;
    fbe_status_t                        status;


    /* get the object id of the pvd. */
    status = fbe_api_provision_drive_get_obj_id_by_location(hs_config_p->disk_set.bus,
                                                            hs_config_p->disk_set.enclosure,
                                                            hs_config_p->disk_set.slot,
                                                            &(hs_config_p->hs_pvd_object_id));
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Register for notification from the Configuration Service on this config change */
    spare_config_ns_context.object_id = hs_config_p->hs_pvd_object_id;
    fbe_semaphore_init(&(spare_config_ns_context.sem), 0, 2);

    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
                                                                  scoobert_test_spare_config_callback_function,
                                                                  &spare_config_ns_context,
                                                                  &ns_registration_id);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);


    /* In order to make configuring particular PVDs as hot spares work, first
       we need to set automatic hot spare not working (set to false). */
    fbe_api_control_automatic_hot_spare(FBE_FALSE);

    /* create a pvd configured as a hot spare */
    fbe_test_sep_util_configure_pvd_as_hot_spare(hs_config_p->hs_pvd_object_id, NULL, 0, FBE_FALSE);

    /* wait for the notification from the Configuration Service */
    scoobert_test_wait_for_config_notification(&spare_config_ns_context);

    /* unregister the notification */
    scoobert_test_unregister_config_notification(ns_registration_id);

    /* wait for the new pvd to become ready */
    status = fbe_api_wait_for_object_lifecycle_state(hs_config_p->hs_pvd_object_id,
                                                     FBE_LIFECYCLE_STATE_READY,
                                                     20000, FBE_PACKAGE_ID_SEP_0); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "%s:drive %d-%d-%d configured as hot spare, obj-id:0x%x.\n",
               __FUNCTION__,
               hs_config_p->disk_set.bus,
               hs_config_p->disk_set.enclosure,
               hs_config_p->disk_set.slot,
               hs_config_p->hs_pvd_object_id);

    fbe_semaphore_destroy(&(spare_config_ns_context.sem)); /* SAFEBUG - needs destroy */

    return;
}
/******************************************************************************
 * end scoobert_test_setup_hot_spare()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_test_degraded_permanent_sparing_test()
 ******************************************************************************
 * @brief
 *  Degraded permanent sparing test.
 *
 * @return None.
 *
 * @author
 *  09/08/2010 - Created. Dhaval Patel
 * 5/12/2011 - Modified Vishnu Sharma Modified to run on Hardware
 ******************************************************************************/
static void scoobert_test_degraded_permanent_sparing_test(fbe_test_rg_configuration_t * rg_config_p,
                                                 fbe_u32_t * position_p,
                                                 fbe_u32_t number_of_drives,
                                                 fbe_test_hs_configuration_t * hs_config_p,
                                                 fbe_job_service_error_type_t * js_error_type_p,
                                                 fbe_bool_t b_wait_for_spare_notification)
{
    fbe_u32_t                           index = 0;
    fbe_object_id_t                     rg_object_id;
    fbe_object_id_t                     vd_object_id;
    scoobert_test_spare_lib_ns_context_t  spare_lib_ns_context;
    //shaggy_test_vd_ns_context_t         vd_ns_context;
    fbe_notification_registration_id_t  ns_registration_id;
    //fbe_notification_registration_id_t  ns_registration_id1;
    fbe_test_raid_group_disk_set_t      spare_drive_location;
    fbe_object_id_t                     pvd_id[2] = {0};

    /* get the vd object-id for this position. */
    fbe_test_sep_util_get_raid_group_object_id(rg_config_p, &rg_object_id);
    fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, *position_p, &vd_object_id);

    /* register for the notification with spare library. */
    scoobert_test_register_spare_lib_notification(vd_object_id, &spare_lib_ns_context, &ns_registration_id);

    if(hs_config_p != NULL)
    {
        /* configure the hot spare. */
        for(index = 0; index < number_of_drives; index++)
        {
            scoobert_test_setup_hot_spare(hs_config_p++);
        }
    }

    /* register for the notification with VD object. */
    //shaggy_test_register_vd_object_notification(vd_object_id, &vd_ns_context, &ns_registration_id1); 

    for(index = 0; index < number_of_drives; index++)
    {
        /* get the pvd object id before we remove the drive. */
        shaggy_test_get_pvd_object_id_by_position(rg_config_p, position_p[index], &pvd_id[index]);
        /* pull the specific drive from the enclosure. */
        shaggy_remove_drive(rg_config_p,
                            position_p[index],
                            &rg_config_p->drive_handle[position_p[index]]);
    }

    /* verify the lifecycle state of the raid group object after drive removal. */
    shaggy_test_verify_raid_object_state_after_drive_removal(rg_config_p, position_p, number_of_drives);   

   /* If we were told to not wait for the spare library notification, it means
    * that a spare request will not occur.
    */
    if (b_wait_for_spare_notification == FBE_FALSE)
    {
        *js_error_type_p = FBE_JOB_SERVICE_ERROR_NO_ERROR;
        scoobert_test_unregister_spare_lib_notification(vd_object_id, &spare_lib_ns_context, ns_registration_id);
        return;
    }

    /* wait for the notification from the spare library. */
    scoobert_test_wait_for_spare_lib_notification(&spare_lib_ns_context);
    *js_error_type_p = spare_lib_ns_context.js_error_info.error_code;

    if(*js_error_type_p != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "Job service request from vd (0x%x) object to swap-in permanent spare failed, error:%d",
                   vd_object_id, *js_error_type_p);
        scoobert_test_unregister_spare_lib_notification(vd_object_id, &spare_lib_ns_context, ns_registration_id);
        return;
    }

    /* unregister the notification. */
    scoobert_test_unregister_spare_lib_notification(vd_object_id, &spare_lib_ns_context, ns_registration_id);

    /* unregister the notification with VD object. */
    //shaggy_test_unregister_vd_object_notification(vd_object_id, ns_registration_id1);

    /* wait for the permanent spare to swap-in. */
    for(index = 0; index < number_of_drives; index++)
    {
        shaggy_test_wait_permanent_spare_swap_in(rg_config_p, position_p[index]);
        
        spare_drive_location.bus = rg_config_p->rg_disk_set[position_p[index]].bus;
        spare_drive_location.enclosure = rg_config_p->rg_disk_set[position_p[index]].enclosure;
        spare_drive_location.slot = rg_config_p->rg_disk_set[position_p[index]].slot;

        shaggy_test_check_event_log_msg(SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN,
                                        &spare_drive_location,
                                        pvd_id[index]);
    }

    /* wait for the rebuild completion to permanent spare. */
    for(index = 0; index < number_of_drives; index++)
    {
        sep_rebuild_utils_wait_for_rb_comp(rg_config_p, position_p[index]);
    }

    sep_rebuild_utils_check_bits(rg_object_id);

    /* wait for the rebuild finish. */
}
/******************************************************************************
 * end scoobert_test_degraded_permanent_sparing_test()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_run_spare_configuration_test()
 ******************************************************************************
 * @brief
 *  Try to configure the hot spare with few error cases.
 *
 * @param   None
 *
 * @return  None.
 *
 * @author
 *  4/10/2010 - Created. Dhaval Patel
 *  24/08/2011 - Modified Vishnu Sharma
 ******************************************************************************/
static void scoobert_run_spare_configuration_test(void)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    scoobert_test_case_t           *test_case_p = NULL;
    fbe_object_id_t                 pvd_obj_id;
    fbe_job_service_error_type_t    job_service_error_type;
    fbe_test_rg_configuration_t    *rg_config_p = &scoobert_raid_group_config[0];
    fbe_test_rg_configuration_t    *current_rg_config_p = NULL;
    fbe_u32_t                       raid_group_count;
    fbe_u32_t                       rg_index;

    mut_printf(MUT_LOG_TEST_STATUS, "**** Scoobert configure hotspare test started          ****\n");
    test_case_p = &test_cases[SCOOBERT_CONFIGURE_SPARE_TEST];

    /* For all enabled raid groups.
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Only run test on enabled raid groups.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* test case 1: configure the hot spare pvd. */
        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the unused pvd as hot spare             ****\n");
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->extra_disk_set[0].bus,
                                                                current_rg_config_p->extra_disk_set[0].enclosure,
                                                                current_rg_config_p->extra_disk_set[0].slot,
                                                                &pvd_obj_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        scoobert_test_configure_hot_spare(test_case_p, pvd_obj_id, &job_service_error_type);
        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the unused pvd as hot spare succeed.    ****\n");
        /* test case 1: configure the hot spare pvd. */


        /* test case 2: try to configure the hot spare for the already configured hot spare pvd. */
        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the existing spare pvd as spare         ****\n");
        scoobert_test_configure_hot_spare(test_case_p, pvd_obj_id, &job_service_error_type);
        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE);
        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the spare pvd as spare failed(expected) ****\n");
        /* test case 2: try to configure the hot spare for the already configured hot spare pvd. */

        /* test case 3: Remove the configured hotspare. */
        mut_printf(MUT_LOG_TEST_STATUS, "**** Removing the configured hotspare    ****\n");
        scobbert_test_remove_hot_spare(pvd_obj_id, &job_service_error_type);
        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_NO_ERROR);
        mut_printf(MUT_LOG_TEST_STATUS, "**** Removed the configured hotspare(expected)  ****\n");

        /* test case 3: try to configure the hot spare for the already configured pvd part of raid object. */
        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the existing RAID group pvd as spare    ****\n");
        status = fbe_api_provision_drive_get_obj_id_by_location(current_rg_config_p->rg_disk_set[0].bus,
                                                                current_rg_config_p->rg_disk_set[0].enclosure,
                                                                current_rg_config_p->rg_disk_set[0].slot,
                                                                &pvd_obj_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        scoobert_test_configure_hot_spare(test_case_p, pvd_obj_id, &job_service_error_type);
        MUT_ASSERT_INT_EQUAL(job_service_error_type, FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP);
        mut_printf(MUT_LOG_TEST_STATUS, "**** Configure the RAID pvd as spare failed(expected)  ****\n");

        /* Goto next raid group.
         */
        current_rg_config_p++;

    } /* for all configured raid groups.*/

    mut_printf(MUT_LOG_TEST_STATUS, "**** Scoobert configure hotspare test completed        ****\n");
    return;
}
/******************************************************************************
 * end scoobert_run_spare_configuration_test()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_run_tests()
 ******************************************************************************
 * @brief
 *  Run drive sparing test across different RAID groups.
 *
 * @param   rg_config_p - Pointer to array of raid groups to test.
 *
 * @return None.
 *
 * @author
 *  11/24/2011 - Created. Vishnu Sharma
 ******************************************************************************/
void scoobert_run_tests(fbe_test_rg_configuration_t *rg_config_p,void * context_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       position_to_test = SCOOBERT_INVALID_DISK_POSITION;
    fbe_test_rg_configuration_t    *current_rg_config_p = NULL;
    fbe_test_hs_configuration_t    *hs_config_p = NULL;
    fbe_u32_t                       test_case_index = 0;
    fbe_u32_t                       num_drives_removed = SCOOBERT_NUM_REMOVED_DRIVES;
    fbe_job_service_error_type_t    job_service_error_type = FBE_JOB_SERVICE_ERROR_INVALID;
    scoobert_test_case_t *          test_case_p = NULL;
    fbe_u32_t                       num_raid_groups = 0;
    fbe_test_raid_group_disk_set_t  removed_disk_set[SCOOBERT_MAX_NUM_REMOVED_DRIVES] = {0};
    fbe_bool_t                      b_wait_for_spare_notification = FBE_TRUE;
    
    num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s:Scoobert test Start ...\n", __FUNCTION__);

    /* The `configure hot-spare test' use the pre-configured hot-spares.
     * Therefore it doesn fit the format defined below.
     */
    scoobert_run_spare_configuration_test();
    
    /* We only test (1) raid group at a time.
     */

    /* permanent sparing degraded - single/double fault test. */
    for (test_case_index = SCOOBERT_NO_HS_AVAILABLE; test_case_index < SCOOBERT_NUM_TEST_CASES; test_case_index++)
    {
        /*get the test case name*/
        test_case_p = &test_cases[test_case_index];
        MUT_ASSERT_TRUE(test_case_p->rg_index < num_raid_groups); 
        current_rg_config_p = &rg_config_p[test_case_p->rg_index];
        b_wait_for_spare_notification = FBE_TRUE;
        position_to_test = test_case_p->test_rg_position[0];
       
        /* Only continue if the associated raid group is enabled
         */
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            MUT_ASSERT_TRUE(test_case_p->test_rg_position[0] < current_rg_config_p->width);
            mut_printf(MUT_LOG_TEST_STATUS, "**** Scoobert spare selection test case (%d), raid group number (%d) started to test the case %s ****\n", 
                       test_case_index, current_rg_config_p->raid_group_id, test_case_p->test_case_name);

            /* There are some test case specific actions.
            */
            switch(test_case_index)
            {
                case SCOOBERT_NO_HS_AVAILABLE:
                    /* Don't configure a hot spare.  `No spare' no longer 
                     * generates a notification.
                     */
                    b_wait_for_spare_notification = FBE_FALSE;
                    break;

                case SCOOBERT_No_SUITABLE_HS_AVAILABLE:
                case SCOOBERT_NO_LUNS_BOUND:
                    /* Don't wait for spare notification.
                     */
                    b_wait_for_spare_notification = FBE_FALSE;
                    // Fall thru
                default:
                    /* configure hotspare*/
                    hs_config_p = scoobert_setup_hotspare_for_test_case(test_case_p, current_rg_config_p, test_case_p->test_rg_position[0]);
    
                    /*If no unused drive is found in the system
                     to configure as spare go to the next test case*/
                    if(hs_config_p == NULL)
                    {
                        mut_printf(MUT_LOG_TEST_STATUS, "**** Can not test this test case with current config,test case (%d), raid group number (%d) started to test the case %s ****\n", 
                              test_case_index, current_rg_config_p->raid_group_id, test_case_p->test_case_name);
                        continue;
                    }
                    break;
            }
         
            scoobert_test_fill_removed_disk_set(current_rg_config_p,removed_disk_set,&test_case_p->test_rg_position[0],num_drives_removed);       

            /* 1. RG2 start degraded permanent sparing test -- for no HS available NO_SPARE . */   
            /* 2. RG0 start degraded permanent sparing test -- for available HS capacity lesser than the desired capacity. */
            /* 3. RG1 start degraded permanent sparing test -- to test bus is higher priority than capacity. */
            /* 4. RG2 start degraded permanent sparing test -- to test capacity is higher priority than encl. */
            /* 5. RG2 start degraded permanent sparing test -- to test encl proximity. */
            /* 6. RG1 start degraded permanent sparing test -- to test bus proximity. */
            scoobert_test_degraded_permanent_sparing_test(current_rg_config_p,
                                                          &test_case_p->test_rg_position[0],
                                                          num_drives_removed,
                                                          NULL,
                                                          &job_service_error_type,
                                                          b_wait_for_spare_notification);

            /* Validate the exepcted results
            */
            status = scoobert_validate_test_results(test_case_p, job_service_error_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Only if the status is success do we need additional tests.
            */
            if (test_case_p->expected_job_service_error == FBE_JOB_SERVICE_ERROR_NO_ERROR)
            {
                /* If we need to validate a specific drive is selected
                 */
                if ((b_wait_for_spare_notification == FBE_TRUE)                          &&
                    (test_case_p->test_rg_position[0] != SCOOBERT_INVALID_DISK_POSITION)    )
                {
                    /* Confirm that we selected a better proximity.
                     */
                    position_to_test = test_case_p->test_rg_position[0];
                    MUT_ASSERT_TRUE(hs_config_p != NULL);
                    MUT_ASSERT_INT_EQUAL(hs_config_p->disk_set.bus, current_rg_config_p->rg_disk_set[position_to_test].bus); 
                    MUT_ASSERT_INT_EQUAL(hs_config_p->disk_set.enclosure, current_rg_config_p->rg_disk_set[position_to_test].enclosure); 
                    MUT_ASSERT_INT_EQUAL(hs_config_p->disk_set.slot, current_rg_config_p->rg_disk_set[position_to_test].slot); 
                }
            }

            /*Reinsert the removed drive*/
            scoobert_reinsert_removed_disks(current_rg_config_p,removed_disk_set,&test_case_p->test_rg_position[0],num_drives_removed);

            /* Need to rebind the luns
             */
            if (test_case_index == SCOOBERT_NO_LUNS_BOUND)
            {
                status = scoobert_setup_bind_all_luns(test_case_p, current_rg_config_p);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            mut_printf(MUT_LOG_TEST_STATUS, "**** Scoobert spare selection test case (%d) completed ****\n", test_case_index);

        } /* end if raid group test configuration is enabled */

    } /* end for all test cases */
    
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Completed successfully ****", __FUNCTION__);
    return;
}
/******************************************************************************
 * end scoobert_test()
 ******************************************************************************/

/*!****************************************************************************
 * scobbert_test_remove_hot_spare()
 ******************************************************************************
 * @brief
 *  This function is used to remove the hotspare pvd object configuration.
 * 
 * @param pvd_object_id     - pvd object-id.
 * @param js_error_type_p   - returns error type of job service if any.
 *
 * @return fbe_status - status of disable request
 *
 * @author
 * 10/7/2010 - Created. Shanmugam
 * 
 ******************************************************************************/
void scobbert_test_remove_hot_spare(fbe_object_id_t pvd_object_id,
                                    fbe_job_service_error_type_t * js_error_type_p)
{
    fbe_api_job_service_update_provision_drive_config_type_t    update_pvd_config_type;
    fbe_status_t                                                status;
    fbe_status_t                                                job_status;

    /* initialize error type as no error. */
    *js_error_type_p = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    /* configure the pvd as hot spare. */
    update_pvd_config_type.pvd_object_id = pvd_object_id;
    update_pvd_config_type.config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED;

    /* update the provision drive config type as hot spare. */
    status = fbe_api_job_service_update_provision_drive_config_type(&update_pvd_config_type);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for the notification from the job service. */
    status = fbe_api_common_wait_for_job(update_pvd_config_type.job_number,
                                         SCOOBERT_TEST_WAIT_MSEC,
                                         js_error_type_p,
                                         &job_status,
                                         NULL);
    /* verify the job status. */
    MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out configuring hot spare");
    MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "Configuring PVD as hot spare failed");
    return;
}
/******************************************************************************
 * end scoobert_test_configure_hot_spare()
 ******************************************************************************/

/*!****************************************************************************
 * scoobert_is_unused_drive()
 ******************************************************************************
 * @brief
 *  Check if the drive indicated by the location passed is in use by any of the
 *  raid groups under test.
 * 
 *  @param disk_set_p      - pointer to disk.
 *
 *  @return fbe_bool_t      - true or false.
 *
 * @author
 * 08/30/2011 - Created. Vishnu Sharma
 * 
 ******************************************************************************/
fbe_bool_t scoobert_is_unused_drive(fbe_test_raid_group_disk_set_t *disk_set_p)
{
    fbe_u32_t                       raid_group_count;
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *rg_config_p = &scoobert_raid_group_config[0];
    fbe_test_rg_configuration_t    *current_rg_config_p;
    fbe_u32_t                       position;

    /* For all enabled raid groups.
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Only check if the drive is in-use if this raid group is enabled.
         */
        if (fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            for (position = 0; position < current_rg_config_p->width; position++)
            {
                /* Assuming we have bound only (1) raid group simply check all the
                 * positions against the drive supplied.
                 */
                if ((disk_set_p->bus == current_rg_config_p->rg_disk_set[position].bus)             &&
                    (disk_set_p->enclosure == current_rg_config_p->rg_disk_set[position].enclosure) &&
                    (disk_set_p->slot == current_rg_config_p->rg_disk_set[position].slot)              )
                {
                    return FBE_FALSE;
                }
            }
        }

        /* Goto next raid group */
        current_rg_config_p++;
    
    } /* for all raid groups */

    /* If we are here then the drive was not in use.
     */
    return FBE_TRUE;
}
/******************************************************************************
 * end scoobert_is_unused_drive()
 ******************************************************************************/

/*!**************************************************************
 * evie_dualsp_test()
 ****************************************************************
 * @brief
 *  Run I/O to a set of raid 5 objects.  The setting of sep dualsp
 *  mode determines if the test will run I/Os on both SPs or not.
 *  sep dualsp has been configured in the setup routine.
 *
 * @param  None.           
 *
 * @return None.
 *
 * @author
 *  04/17/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
void scoobert_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = &scoobert_raid_group_config[0];

    scoobert_run_test_on_rg_config_with_extra_disk(rg_config_p,NULL,scoobert_run_tests,
                                   SCOOBERT_TEST_LUNS_PER_RAID_GROUP,
                                   SCOOBERT_TEST_CHUNKS_PER_LUN);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end scoobert_dualsp_test()
 ******************************************/

/*!**************************************************************
 * evie_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  04/17/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
void scoobert_dualsp_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &scoobert_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        scoobert_physical_config(520);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        scoobert_physical_config(520);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    
        /* After sep is loaded setup notifications
         */
        fbe_test_common_setup_notifications(FBE_TRUE /* This is a dual SP test*/);   

    }
    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();
    
    //fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();
    //fbe_test_sep_drive_disable_sniff_verify_for_all_pvds();


    return;
}
/**************************************
 * end scoobert_dualsp_setup()
 **************************************/

/*!**************************************************************
 * scoobert_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  04/17/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
void scoobert_dualsp_cleanup(void)
{
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
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end scoobert_dualsp_cleanup()
 ******************************************/
/*****************************************
 * end file scoobert_test.c
 *****************************************/



