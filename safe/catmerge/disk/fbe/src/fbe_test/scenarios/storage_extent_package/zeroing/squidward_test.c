/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file squidward_test.c
 ***************************************************************************
 *
 * @brief   This scenario sends I/O to LUN/Raid group object that hasn't completed background
 *          zeroing.  The result is that the PVD must handle Read and Write
 *          requests to ranges that do not have valid data.
 *          o For ZOD(Zero On Demand) Reads, the PVD returns valid zeroed data
 *          o For ZOD(Zero On Demand) Writes, the PVD initializes the area and then perform IO
 *
 * @version
 *  02/19/2010  Amit Dhaduk - Created.
 *  11/02/2010  Amit Dhaduk - Modified to add I/O test cases
 *  02/20/2012  Sandeep Chaudhari - Modified
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
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe_test_common_utils.h"
#include "sep_zeroing_utils.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe/fbe_api_system_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_random.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * squidward_short_desc = "Test Zero On Demand support in the PVD";
char * squidward_long_desc =
"\n"
"   The Squidward test validates Zero On Demand support in PVD object\n"
"       Zero on Demand is a state of provision drive object to determine that disk\n"
"       is not completely initialized. In this state it handles read and write IO request\n"
"       for the disk area which is not zeroed out. It is responsibility of provision drive object to\n"
"       initialize disk area if it is not while handling write IOs.\n"
"   ZOD Write sequence in PVD\n"
"   - Read the paged bitmap data\n"
"   - identify which chunks need to zero\n"
"   - perform zeroing\n"
"   - perform the original write request\n"
"   - update the paged bitmap data\n"
"   ZOD Read sequence in PVD\n"
"   - Read the paged bitmap data\n"
"   - identify if all chunks are initialized or not\n"
"   - if all chunks are initialized, perform the read IO\n"
"   - if all chunks are not initialized, fill out buffer with zero\n"
"   - if chunks are initialized partially \n"
"         perform the read IO for initialized chunks\n"
"         fill out zero buffer for uninitialized chunks \n"
"\n"
"Starting Config:\n"
"        [PP] armada board\n"
"        [PP] SAS PMC port\n"
"        [PP] viper enclosure\n"
"        [PP] 2 SAS drives\n"
"        [PP] 2 logical drive\n"
"        [SEP] 2 provision drive\n"
"        [SEP] 2 virtual drive\n"
"        [SEP] 1 parity raid group (raid 0 - 520)\n"
"        [SEP] 1 LUN\n"
"\n"
"   STEP 1: Bring up the initial topology.\n"
"        - Create and verify the initial physical package config.\n"
"        - Create provisioned drives and attach edges to logical drives.\n"
"        - Create virtual drives and attach edges to provisioned drives.\n"
"        - Create raid raid group objects and attach to virtual drives.\n\n"
"   STEP 2: ZOD Write IO test\n"
"        - This test performs write IO to uninitialized chunks and verify that write IO completed successfully.\n"
"   STEP 3: ZOD Read IO test\n"
"        - This test performs read IO to uninitialized chunks and verify that pvd returns valid zeroed buffer.\n"
"   STEP 4: ZOD Read IO test with unaligned IO requests\n"
"        - Perform ZOD unaligned read I/Os with various block size and verify that PVD \n"
"           returns valid zero data.\n"
"   STEP 5: ZOD read IO test for partial initialized chunks\n"
"        - PVD performs read IO to the disk for initialized chunks.\n"
"        - PVD fills out zero buffer for uninitialized chunks.\n"
"        - PVD combines read buffer with zero buffer and returns valid read data.\n"
"   STEP 6: User zero IO test\n"
"        - PVD updates the paged metadata.\n"
"        - For unaligned edges, pvd sends write same request to perform zeroing operation.\n"
"        - For the aligned chunks pvd updates bitmap so background zeroing or ZOD write IO\n"
"           will zeroing out these chunks before IO takes place.\n"
"   STEP 7: ZOD unaligned Write IO test\n"
"        - Perform ZOD unaligned write I/Os with various block size and verify that write completes successfully.\n"
"   STEP 8: ZOD Write IO test with rebind lun\n"
"        - Unbind and rebind the lun\n"
"        - perform write IO and validates it\n"
"        - Unbind and rebind the lun again\n"
"        - Perform write IO and validates it\n"
"   STEP 9: ZOD random IO test\n"
"        - Perform random IO test to uninitialized disk for the given time duration.\n"
"   STEP 10: ZOD write IO test while running background zeroing operation\n"
"        - Start random IO test.\n"
"        - Start disk zeroing for associate PVD objects.\n"
"        - Wait for disk zeroing complete.\n" 
"        - Stop random IO test.\n\n"
"   STEP 11: ZOD read IO test\n"
"        - Perform read IO test to post zeroed disk.\n"
"   STEP 12: ZOD write/read/zero IO test\n"
"        - Perform write/read/zero IO test to post zeroed disk.\n"
"   STEP 13: ZOD random read zero IO test\n"
"        - Perform random read zero IO test to post zeroed disk.\n"
"   STEP 14: ZOD bind LUNs and run random IO test\n"
"        - Perform LUN bind and then random IO test to post zeroed disk.\n"
"   STEP 15: ZOD unaligned IO test\n"
"        - Perform unaligned IO test to post zeroed disk.\n"
"\n";

static fbe_object_id_t                      expected_object_id = FBE_OBJECT_ID_INVALID;
static fbe_lifecycle_state_t                expected_state = FBE_LIFECYCLE_STATE_INVALID;
static fbe_semaphore_t                      sem;
static fbe_notification_registration_id_t   reg_id;

/*!*******************************************************************
 * @def SQUIDWARD_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs per RAID group.
 *
 *********************************************************************/
#define SQUIDWARD_TEST_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def SQUIDWARD_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of raid groups we will test with.
 *
 *********************************************************************/
#define SQUIDWARD_TEST_RAID_GROUP_COUNT 19

 /*!*******************************************************************
 * @def SQUIDWARD_TEST_RAID_GROUP_WIDTH
 *********************************************************************
 * @brief The number of drives in the mirror raid group.
 *
 *********************************************************************/
#define SQUIDWARD_TEST_RAID_GROUP_WIDTH       2 

/*!*******************************************************************
 * @def SQUIDWARD_POLLING_INTERVAL
 *********************************************************************
 * @brief polling interval time to check for disk zeroing status
 *
 *********************************************************************/
#define SQUIDWARD_POLLING_INTERVAL 100 /*ms*/

/*!*******************************************************************
 * @def SQUIDWARD_TEST_NS_TIMEOUT
 *********************************************************************
 * @brief  Notification to timeout value in milliseconds 
 *
 *********************************************************************/
#define SQUIDWARD_TEST_NS_TIMEOUT        25000 /*wait maximum of 25 seconds*/

#define SQUIDWARD_TEST_CHUNKS_PER_LUN    65 

/*!*******************************************************************
 * @var squidward_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t squidward_test_contexts[SQUIDWARD_TEST_LUNS_PER_RAID_GROUP * SQUIDWARD_TEST_RAID_GROUP_COUNT];


/*!*******************************************************************
 * @var squidward_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t squidward_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                      block size      RAID-id.    bandwidth.*/
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            0,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            1,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            2,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            3,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            4,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            5,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            6,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            7,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            8,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            9,          0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            10,         0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            11,         0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            12,         0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            13,         0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            14,         0},
    {2,         0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            15,         0},
    {2,         0x2000,     FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            16,         0},
    {2,         0x22000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,        520,            18,         0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

#define SQUIDWARD_MAX_IO_COUNT              40

#define SQUIDWARD_CHUNK_SIZE        0x800
#define SQUIDWARD_CHUNK_SIZE_HALF   0x400
#define SQUIDWARD_CHUNK_SIZE_FULL   0x800
#define SQUIDWARD_CHUNK_SIZE_ONEH   0xC00
#define SQUIDWARD_CHUNK_SIZE_TWO    0x1000
#define SQUIDWARD_CHUNK_SIZE_TWOH   0x1400
#define SQUIDWARD_CHUNK_SIZE_THREE  0x1800
#define SQUIDWARD_CHUNK_SIZE_FIVE   0x2800
#define SQUIDWARD_CHUNK_SIZE_SEVEN  0x3800

#define SQUIDWARD_ZERO_WAIT_TIME    200000


/*!*******************************************************************
 * @enum squidward_test_index_e
 *********************************************************************
 * @brief This is an test index enum for the squidward.
 *
 *********************************************************************/
typedef enum squidward_test_index_e
{
    SQUIDWARD_TEST_ZERO_ON_DEMAND_FIRST = 0,

    SQUIDWARD_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST = 0,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_READ_IO_TEST = 1,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_READ_EDGE_IO_TEST = 2,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_PARTIAL_ZERO_READ_IO_TEST = 3,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_ZERO_IO_TEST = 4,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_WRITE_EDGE_IO_TEST = 5,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_UNBIND_BIND_WRITE_IO_TEST = 6,    
    SQUIDWARD_TEST_ZERO_ON_DEMAND_RANDOM_IO_TEST = 7,    
    SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_WITH_ZOD_IO_TEST = 8,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_AFTER_WRITE_TEST = 9,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_READ_IO_TEST = 10,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_WRITE_READ_ZERO_IO_TEST = 11,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_RANDOM_READ_ZERO_IO_TEST = 12,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_BIND_LUNS_RUN_IO_TEST = 13,
    SQUIDWARD_TEST_HIBERNATION_TEST = 14,
#if 0    /* PVD does NOT quiesce I/O for ZOD any more */ 
    SQUIDWARD_TEST_ZERO_ON_DEMAND_EXIT_QUIESCE_IO_TEST = 15,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_QUIESCE_IO_TEST = 16,
#endif
    SQUIDWARD_TEST_ZERO_ON_DEMAND_UNALIGNED_IO_TEST = 15,
    SQUIDWARD_TEST_ZERO_ON_DEMAND_CHECK_ZEROED_IO_TEST = 16,

    /*!@note Add new test index here.*/
    SQUIDWARD_TEST_ZERO_ON_DEMAND_LAST,
}
squidward_test_index_t;


/*!*******************************************************************
 * @struct squidward_io_configuration_s
 *********************************************************************
 * @brief This is an i/o configuration of particular test.
 *
 *********************************************************************/
typedef struct squidward_io_configuration_s
{
    fbe_lba_t                   start_lba;
    fbe_block_count_t           block_count;
    fbe_rdgen_operation_t       rdgen_operation;
    fbe_rdgen_pattern_t         rdgen_pattern;
}
squidward_io_configuration_t;

/*!*******************************************************************
 * @struct squidward_test_configuration_s
 *********************************************************************
 * @brief This is a single test case for this test.
 *
 *********************************************************************/
typedef struct squidward_test_configuration_s
{
    squidward_test_index_t          test_index;
    fbe_raid_group_number_t         raid_group_id;
    squidward_io_configuration_t    rdgen_io_config[SQUIDWARD_MAX_IO_COUNT];
}
squidward_test_configuration_t;

#define SQUIDWARD_INVALID_IO_CONFIGURATION {FBE_LBA_INVALID, 0x0, FBE_RDGEN_OPERATION_INVALID, FBE_RDGEN_PATTERN_INVALID}


/*!*******************************************************************
 * @struct squidward_test_configuration
 *********************************************************************
 * @brief squidward IO configuration table
 *
 *********************************************************************/
squidward_test_configuration_t squidward_test_configuration[SQUIDWARD_TEST_ZERO_ON_DEMAND_LAST] =
{

     
    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST, 0,
     /* normal ZOD IO test with different block size */   
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */  
      {0x0,          SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0xC00,        SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x1400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x2000,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x2400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x3C00,       SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x3400,       SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x4C00,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x4400,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x5900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x6900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x5C00,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x6900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x7000,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x8C00,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x9200,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x9900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x9200,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     }, 
    },
    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_READ_IO_TEST, 1,
     /* ZOD IO test with partial zeroed lba range */
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */           
      {0x0,             SQUIDWARD_CHUNK_SIZE / 2,       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x400,           SQUIDWARD_CHUNK_SIZE,           FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS}, // Z NZ
      {0x1000,          SQUIDWARD_CHUNK_SIZE / 2,       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0xC00,           SQUIDWARD_CHUNK_SIZE,           FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS}, // NZ Z
      {0x2000,          SQUIDWARD_CHUNK_SIZE / 2,       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x1400,          SQUIDWARD_CHUNK_SIZE * 2,       FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS}, // Z NZ Z
      {0x1C00,          SQUIDWARD_CHUNK_SIZE * 2,       FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS}, // NZ Z NZ
      {0x5000,          SQUIDWARD_CHUNK_SIZE,           FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x5647,          SQUIDWARD_CHUNK_SIZE,           FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS},
      {0x5647,          SQUIDWARD_CHUNK_SIZE,           FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS},
      {0x5647,          SQUIDWARD_CHUNK_SIZE,           FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS},
      {0x5647,          SQUIDWARD_CHUNK_SIZE,           FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_ZEROS},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_READ_EDGE_IO_TEST, 2,

     /* non aligned read IO test for non zeroed lba range */
     {

     
      /* start lba   block count                        rdgen operation                         rdgen pattern */                     

      /*  ZOD read IO with pre edge */
      {0x0,          SQUIDWARD_CHUNK_SIZE_HALF,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /*  ZOD read IO with pre edge */
      {0xC00,        SQUIDWARD_CHUNK_SIZE_HALF,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /*  ZOD read IO with pre-post edge */
      {0x1200,       SQUIDWARD_CHUNK_SIZE_HALF,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO aligned request */  
      {0x1800,       SQUIDWARD_CHUNK_SIZE_FULL,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO pre-post edge */  
      {0x2400,       SQUIDWARD_CHUNK_SIZE_FULL,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO with post edge */
      {0x3000,       SQUIDWARD_CHUNK_SIZE_ONEH ,    FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO with pre edge */
      {0x4400,       SQUIDWARD_CHUNK_SIZE_ONEH,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO with pre-post edge */
      {0x5200,       SQUIDWARD_CHUNK_SIZE_ONEH,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD aligned read IO */
      {0x6000,       SQUIDWARD_CHUNK_SIZE_TWO,      FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO  with pre-post edge */      
      {0x7400,       SQUIDWARD_CHUNK_SIZE_TWO,      FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO  with post edge */      
      {0x8800,       SQUIDWARD_CHUNK_SIZE_TWOH ,    FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO  with pre edge */      
      {0xA400,       SQUIDWARD_CHUNK_SIZE_TWOH,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO  with pre-post edge */      
      {0xBA00,       SQUIDWARD_CHUNK_SIZE_TWOH,     FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},

       /* next start lba - 0xD000 */ 
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_PARTIAL_ZERO_READ_IO_TEST, 3,
     /* non aligned read IO test for partial zeroed lba range */
     {

      /* ZOD read IO (2 chunk size, first chunk zeroed and second chunk not zeroed )   */             
      {0x0,             SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x0,             SQUIDWARD_CHUNK_SIZE_TWO,   FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO (2 chunk size, first chunk not zeroed and second chunk zeroed )   */
      {0x1800,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x1000,          SQUIDWARD_CHUNK_SIZE_TWO,   FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO (1 and half chunk size, first chunk zeroed and next half chunk not zeroed )   */        
      {0x2000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x2000,          SQUIDWARD_CHUNK_SIZE_ONEH,  FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO (1 and half chunk size, first half chunk not zeroed and next chunk zeroed )   */                
      {0x3800,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x3400,          SQUIDWARD_CHUNK_SIZE_ONEH,  FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO (3 chunk size, first chunk zeroed, second chunk not zeroed and third chunk zeroed)   */                
      {0x4000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x5000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x4000,          SQUIDWARD_CHUNK_SIZE_THREE, FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO (3 chunk size, first chunk not zeroed, second chunk zeroed and third chunk not zeroed)   */                        
      {0x6000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x5800,          SQUIDWARD_CHUNK_SIZE_THREE, FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO - pre-post edge (2 chunk size, first half chunk not zeroed, middle 1 chunk zeroed and last half chunk not zeroed   */                        
      {0x7800,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x7400,          SQUIDWARD_CHUNK_SIZE_TWO,   FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO - pre-post edge (2 chunk size, first half chunk not zeroed, middle 1 chunk zeroed and last half chunk not zeroed   */
      {0x9000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x8800,          SQUIDWARD_CHUNK_SIZE_TWOH,  FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO - pre-edge (2 and half chunk size, first half chunk not zeroed, middle 1 chunk zeroed and last chunk not zeroed   */        
      {0xA800,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0xA400,          SQUIDWARD_CHUNK_SIZE_TWOH,  FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
        /* ZOD read IO (5 chunk size, first, third and fifth chunk zeroed, second and fourth chunk not zeroed */        
      {0xB800,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0xC800,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0xD800,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0xB800,          SQUIDWARD_CHUNK_SIZE_FIVE,  FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD read IO - pre-edge (7 chunk size, second, fourth and sixth chunk zeroed and rest of chunks are not zeroed */        
      {0xE800,         SQUIDWARD_CHUNK_SIZE,        FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0xF800,         SQUIDWARD_CHUNK_SIZE,        FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x10800,        SQUIDWARD_CHUNK_SIZE,        FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0xE000,         SQUIDWARD_CHUNK_SIZE_SEVEN,  FBE_RDGEN_OPERATION_READ_CHECK,         FBE_RDGEN_PATTERN_ZEROS},

       /* next start lba - 0x11800 */ 
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_ZERO_IO_TEST, 4,
     /* non aligned zero IO test */
     {
      /* ZOD zero IO - post edge */
      {0x0,          SQUIDWARD_CHUNK_SIZE_HALF,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre edge */
      {0xC00,        SQUIDWARD_CHUNK_SIZE_HALF,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre-post edge */
      {0x1200,      SQUIDWARD_CHUNK_SIZE_HALF,      FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - aligned */          
      {0x1800,       SQUIDWARD_CHUNK_SIZE_FULL,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre-post edge */          
      {0x2400,       SQUIDWARD_CHUNK_SIZE_FULL,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - post edge */                    
      {0x3000,       SQUIDWARD_CHUNK_SIZE_ONEH ,    FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre edge */                    
      {0x4400,       SQUIDWARD_CHUNK_SIZE_ONEH,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre-post edge */                    
      {0x5200,       SQUIDWARD_CHUNK_SIZE_ONEH,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - aligned */                    
      {0x6000,       SQUIDWARD_CHUNK_SIZE_TWO,      FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre-post edge */                    
      {0x7400,       SQUIDWARD_CHUNK_SIZE_TWO,      FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - post edge */                    
      {0x8800,       SQUIDWARD_CHUNK_SIZE_TWOH ,    FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre edge */                    
      {0xA400,       SQUIDWARD_CHUNK_SIZE_TWOH,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      /* ZOD zero IO - pre-post edge */                    
      {0xBA00,       SQUIDWARD_CHUNK_SIZE_TWOH,     FBE_RDGEN_OPERATION_ZERO_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},

       /* next start lba - 0xD000 */ 
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_WRITE_EDGE_IO_TEST, 5,
     /* non aligned ZOD write IO test */
     {
      /* ZOD write IO - post edge */
      {0x0,          SQUIDWARD_CHUNK_SIZE_HALF,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - post edge */
      {0xC00,        SQUIDWARD_CHUNK_SIZE_HALF,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - pre-post edge */
      {0x1200,      SQUIDWARD_CHUNK_SIZE_HALF,      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - aligned */          
      {0x1800,       SQUIDWARD_CHUNK_SIZE_FULL,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - pre-post edge */
      {0x2400,       SQUIDWARD_CHUNK_SIZE_FULL,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - post edge */          
      {0x3000,       SQUIDWARD_CHUNK_SIZE_ONEH ,    FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - pre edge */
      {0x4400,       SQUIDWARD_CHUNK_SIZE_ONEH,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - pre-post edge */
      {0x5200,       SQUIDWARD_CHUNK_SIZE_ONEH,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - aligned */          
      {0x6000,       SQUIDWARD_CHUNK_SIZE_TWO,      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - pre-post edge */
      {0x7400,       SQUIDWARD_CHUNK_SIZE_TWO,      FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - post edge */         
      {0x8800,       SQUIDWARD_CHUNK_SIZE_TWOH ,    FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - pre edge */
      {0xA400,       SQUIDWARD_CHUNK_SIZE_TWOH,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      /* ZOD write IO - pre-post edge */
      {0xBA00,       SQUIDWARD_CHUNK_SIZE_TWOH,     FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},

       /* next start lba - 0xD000 */ 
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_UNBIND_BIND_WRITE_IO_TEST, 6,
     /* normal ZOD IO test with different block size */   
     {
      /* start lba   block count                        rdgen operation                         rdgen pattern */  
      {0x0,          SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0xC00,        SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x1400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x2000,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x2400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x3C00,       SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x3400,       SQUIDWARD_CHUNK_SIZE / 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x4C00,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x4400,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x5900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x6900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x5C00,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x6900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x7000,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x8C00,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x9200,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x9900,       SQUIDWARD_CHUNK_SIZE / 4,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x9200,       SQUIDWARD_CHUNK_SIZE * 2,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     }, 
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_RANDOM_IO_TEST, 7,
      /* ZOD random IO test */
     {
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
     
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_WITH_ZOD_IO_TEST, 8,
      /* ZOD write random IO test while background zeroing runnning */
     {
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },
    
    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_AFTER_WRITE_TEST, 9,
      /* ZOD write unaligned IO test */
     {
      {0x1400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_LBA_PASS},
      {0x1400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_READ_CHECK,          FBE_RDGEN_PATTERN_LBA_PASS},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_READ_IO_TEST, 10,
      /* ZOD write unaligned IO test */
     {
      {0x0,          SQUIDWARD_CHUNK_SIZE_HALF,       FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x1400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x2400,       SQUIDWARD_CHUNK_SIZE_TWO,        FBE_RDGEN_OPERATION_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_WRITE_READ_ZERO_IO_TEST, 11,
      /* ZOD write unaligned IO test */
     {
      {0x0,          SQUIDWARD_CHUNK_SIZE_HALF,       FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x1400,       SQUIDWARD_CHUNK_SIZE,            FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      {0x2400,       SQUIDWARD_CHUNK_SIZE_TWO,        FBE_RDGEN_OPERATION_WRITE_READ_CHECK,    FBE_RDGEN_PATTERN_ZEROS},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_RANDOM_READ_ZERO_IO_TEST, 12,
      /* ZOD random read IO test */
     {
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_BIND_LUNS_RUN_IO_TEST, 13,
      /* ZOD random read IO test */
     {
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_HIBERNATION_TEST, 14,
      /* ZOD quiesce IO test */
     {
      {0x10000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_ONLY,    FBE_RDGEN_PATTERN_ZEROS},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

#if 0    /* PVD does NOT quiesce I/O for ZOD any more */ 
    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_EXIT_QUIESCE_IO_TEST, 15,
      /* ZOD quiesce IO test */
     {
      {0x10000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_ONLY,    FBE_RDGEN_PATTERN_CLEAR},
      {0x10000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_ONLY,    FBE_RDGEN_PATTERN_CLEAR},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_QUIESCE_IO_TEST, 16,
      /* ZOD quiesce IO test */
     {
      {0x10000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_ONLY,    FBE_RDGEN_PATTERN_CLEAR},
      {0x10000,          SQUIDWARD_CHUNK_SIZE,       FBE_RDGEN_OPERATION_ZERO_ONLY,    FBE_RDGEN_PATTERN_CLEAR},
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },
#endif

    /* IO test name index                       ,Raid Group number */
    {SQUIDWARD_TEST_ZERO_ON_DEMAND_UNALIGNED_IO_TEST, 17,
      /* ZOD write unaligned IO test */
     {
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },

    {SQUIDWARD_TEST_ZERO_ON_DEMAND_CHECK_ZEROED_IO_TEST, 18,
    /* ZOD check zeroed IO test */
     {
      SQUIDWARD_INVALID_IO_CONFIGURATION,
     },
    },
    
};


/*
 * FORWARD REFERENCES:
 */
static void squidward_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p);
void squidward_test_run_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p, fbe_bool_t wait_for_completion);
static fbe_status_t squidward_test_wait_for_disk_zeroing_complete(fbe_object_id_t object_id, 
                                                        fbe_u32_t timeout_ms);
static fbe_status_t squidward_test_perform_bgz_with_io(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_api_rdgen_context_t * in_test_context_p, 
                                                        squidward_test_index_t test_index);
static void squidward_test_perform_io(fbe_api_rdgen_context_t * test_context_p, 
                                                        squidward_test_index_t test_index);
static void squidward_test_perform_single_io(fbe_api_rdgen_context_t * test_context_p,
                                                squidward_test_index_t test_index,
                                                fbe_u32_t index,
                                                fbe_bool_t *invalid_pattern);
static void squidward_test_perform_zod_write_edge_IO(fbe_api_rdgen_context_t * in_test_context_p, 
                                                        squidward_test_index_t test_index);
static void squidward_test_perform_random_io(fbe_api_rdgen_context_t * in_test_context_p, 
                                                        squidward_test_index_t test_index);
static void squidward_random_io_start(fbe_api_rdgen_context_t *context_p, 
                                                        fbe_rdgen_operation_t io_operation, 
                                                        fbe_object_id_t lu_object_id, 
                                                        fbe_u32_t test_duration,
                                                        fbe_rdgen_pattern_t pattern);
static fbe_status_t squidward_test_calculate_unaligned_edges_lba_range(fbe_lba_t start_lba,
                                                        fbe_block_count_t block_count,
                                                        fbe_u32_t chunk_size,
                                                        fbe_lba_t * pre_edge_start_lba_p,
                                                        fbe_block_count_t * pre_edge_block_count_p,
                                                        fbe_lba_t * post_edge_start_lba_p,
                                                        fbe_block_count_t * post_edge_block_count_p,
                                                        fbe_u32_t * number_of_edges_p);
static fbe_status_t squidward_test_calculate_chunk_range(fbe_lba_t start_lba,
                                                        fbe_block_count_t block_count,
                                                        fbe_u32_t chunk_size,
                                                        fbe_chunk_index_t * start_chunk_index_p,
                                                        fbe_chunk_count_t * chunk_count_p);
static void squidward_test_setup_random_io_rdgen_test_context(
                                                        fbe_api_rdgen_context_t *context_p,
                                                        fbe_object_id_t object_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_rdgen_operation_t rdgen_operation,
                                                        fbe_lba_t capacity,
                                                        fbe_u32_t max_passes,
                                                        fbe_u32_t threads,
                                                        fbe_u32_t io_size_blocks,
                                                        fbe_rdgen_pattern_t pattern);
static void squidward_test_create_lun( fbe_raid_group_number_t rg_id, fbe_lun_number_t   lun_number, fbe_lba_t capacity);
static void squidward_test_destroy_lun(fbe_lun_number_t   lun_number);
static void squidward_test_unbind_bind_write_io(fbe_api_rdgen_context_t * test_context_p, 
                                                        squidward_test_index_t test_index);
static fbe_status_t squidward_test_unaligned_io(fbe_test_rg_configuration_t *in_rg_config_p, fbe_api_rdgen_context_t * test_context_p,
                                                squidward_test_index_t test_index);
static fbe_status_t squidward_test_check_zeroed_io(fbe_test_rg_configuration_t *in_rg_config_p, fbe_api_rdgen_context_t * test_context_p,
                                                squidward_test_index_t test_index);
void squidward_test_bgz_after_zod_write(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_api_rdgen_context_t * test_context_p,
                                        squidward_test_index_t test_index);
void squidward_test_bgz_then_read(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_api_rdgen_context_t * test_context_p,
                                        squidward_test_index_t test_index,
                                        fbe_bool_t random_read);
void squidward_test_bgz_bind_luns_run_io(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_api_rdgen_context_t * test_context_p,
                                        squidward_test_index_t test_index);
void squidward_test_bgz_zod_quiesce(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_api_rdgen_context_t * test_context_p,
                                    squidward_test_index_t test_index);
void squidward_test_bgz_exit_zod_quiesce(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_api_rdgen_context_t * test_context_p,
                                        squidward_test_index_t test_index);
void squidward_hibernation_test(fbe_test_rg_configuration_t *in_rg_config_p,fbe_api_rdgen_context_t * test_context_p,
                                squidward_test_index_t test_index);
void squidward_find_test_index(squidward_test_index_t *test_index,fbe_raid_group_number_t raid_group_id);
void squidward_write_background_pattern(void);
static void squidward_check_for_and_reset_sep_error_counter(fbe_u32_t count);
static void squidward_test_perform_single_io_to_lun(fbe_lun_number_t lun_number, fbe_api_rdgen_context_t * test_context_p, squidward_test_index_t test_index, fbe_u32_t index, fbe_bool_t *invalid_pattern);
static void squidward_test_perform_single_io_to_pvd(fbe_object_id_t object_id, fbe_api_rdgen_context_t * test_context_p, squidward_test_index_t test_index, fbe_u32_t index, fbe_bool_t *invalid_pattern);
static void squidward_wait_for_io_to_complete(fbe_u32_t rg_index, fbe_api_rdgen_context_t *context_p);
static void squidward_command_response_callback_function (update_object_msg_t * update_object_msg, void *context);
/*!****************************************************************************
 *  squidward_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the squidward test.  
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk 
 *
 *****************************************************************************/

void squidward_setup(void)
{    

    mut_printf(MUT_LOG_LOW, "=== squidward setup ===\n");   
    if (fbe_test_util_is_simulation())
    {

        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t num_raid_groups = 0;
        
        rg_config_p = &squidward_raid_group_config[0];
       
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        
        /* Load the physical package and create the physical configuration. 
         */
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        /* Load the logical packages. 
         */
        sep_config_load_sep_and_neit();

    } 
    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
    
}
/******************************************
 * end squidward_setup()
 ******************************************/


/*!****************************************************************************
 *  squidward_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the squidward test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk
 *
 *****************************************************************************/
void squidward_test(void)
{
    
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    
    rg_config_p = &squidward_raid_group_config[0];
    
    /*!@todo This is not fool proof. Before we give this command
       some PVDs could have started BZ */
    fbe_test_sep_drive_disable_background_zeroing_for_all_pvds();

    fbe_test_run_test_on_rg_config(rg_config_p,NULL,squidward_run_tests,
                                   SQUIDWARD_TEST_LUNS_PER_RAID_GROUP,
                                   SQUIDWARD_TEST_CHUNKS_PER_LUN);
           
    return;
 
 }
/******************************************
 * end squidward_test()
 ******************************************/

/*!****************************************************************************
 * @fn squidward_run_tests()
 ******************************************************************************
 * @brief
 *  This function initiate the IO test based on test index.
 *
 *
 * @param  test_index - test index to run IO.
 *
 * @return  none
 *
 * @author
 *   11/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static void squidward_run_tests(fbe_test_rg_configuration_t *rg_config_ptr ,void * context_p)
{
    fbe_u32_t num_raid_groups = 0;
    fbe_u32_t index = 0;
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    squidward_test_index_t test_index = SQUIDWARD_TEST_ZERO_ON_DEMAND_LAST;
    
    mut_printf(MUT_LOG_LOW, "=== squidward test started ===\n");

    num_raid_groups = fbe_test_get_rg_array_length(rg_config_ptr);

    rg_config_p = rg_config_ptr;
    for(index = 0; index < num_raid_groups; index++)
    {

        if(fbe_test_rg_config_is_enabled(rg_config_p))
        {
            squidward_find_test_index(&test_index,rg_config_p->raid_group_id);

            switch(test_index)
            {
                case SQUIDWARD_TEST_ZERO_ON_DEMAND_WRITE_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD Write IO test ===\n");
                    squidward_test_perform_io(&squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_READ_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD Read IO test ===\n");
                    squidward_test_perform_io(&squidward_test_contexts[test_index], test_index);
                    break;
                
                case SQUIDWARD_TEST_ZERO_ON_DEMAND_READ_EDGE_IO_TEST :
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD Read edge IO test ===\n");
                    squidward_test_perform_io(&squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_PARTIAL_ZERO_READ_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD partial zero read IO test ===\n");
                    squidward_test_perform_io(&squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_ZERO_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD zero IO test ===\n");
                    squidward_test_perform_io(&squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_WRITE_EDGE_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD Write edge IO test ===\n");
                    squidward_test_perform_zod_write_edge_IO(&squidward_test_contexts[test_index], test_index);
                    break;
             
                case SQUIDWARD_TEST_ZERO_ON_DEMAND_UNBIND_BIND_WRITE_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD rebind lun write IO test ===\n");
                    squidward_test_unbind_bind_write_io(&squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_RANDOM_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward random IO test ===\n");
                    squidward_test_perform_random_io(&squidward_test_contexts[test_index], test_index);
                    break;
            
                case SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_WITH_ZOD_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD background zeroing with ZOD IO test ===\n");
                    squidward_test_perform_bgz_with_io(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_AFTER_WRITE_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward background zeroing after ZOD write test ===\n");
                    squidward_test_bgz_after_zod_write(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_READ_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward background zeroing read zero IO test ===\n");
                    squidward_test_bgz_then_read(rg_config_p, &squidward_test_contexts[test_index], test_index, FBE_FALSE);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_WRITE_READ_ZERO_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward background zeroing write / read zero IO test ===\n");
                    squidward_test_bgz_then_read(rg_config_p, &squidward_test_contexts[test_index], test_index, FBE_FALSE);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_RANDOM_READ_ZERO_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward background zeroing random read zero IO test ===\n");
                    squidward_test_bgz_then_read(rg_config_p, &squidward_test_contexts[test_index], test_index, FBE_TRUE);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_BGZ_BIND_LUNS_RUN_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward background zeroing bind luns run IO test ===\n");
                    squidward_test_bgz_bind_luns_run_io(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_UNALIGNED_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward Unaligned IO test ===\n");
                    squidward_test_unaligned_io(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;

#if 0    /* PVD does NOT quiesce I/O for ZOD any more */ 

                    // @todo: the quiesce tests have issues and need to be looked into.  The IOs never complete
                case SQUIDWARD_TEST_ZERO_ON_DEMAND_EXIT_QUIESCE_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD exit quiesce IO test ===\n");
                    squidward_test_bgz_exit_zod_quiesce(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;
            
                case SQUIDWARD_TEST_ZERO_ON_DEMAND_QUIESCE_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward ZOD Quiesce IO test ===\n");
                    squidward_test_bgz_zod_quiesce(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;
#endif

                case SQUIDWARD_TEST_ZERO_ON_DEMAND_CHECK_ZEROED_IO_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward Check Zeroed IO test ===\n");
                    squidward_test_check_zeroed_io(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;
            
                case SQUIDWARD_TEST_HIBERNATION_TEST:
                    mut_printf(MUT_LOG_LOW, "=== squidward Hibernation test ===\n");
                    squidward_hibernation_test(rg_config_p, &squidward_test_contexts[test_index], test_index);
                    break;
            
                default:
                    mut_printf(MUT_LOG_LOW, "=== squidward Invalid test index  ===\n");
                    break;

            }
        }
        rg_config_p++;
    }

    mut_printf(MUT_LOG_LOW, "=== squidward test completed ===\n");
    return;
}
/******************************************
 * end squidward_run_tests()
 ******************************************/
void squidward_find_test_index(squidward_test_index_t *test_index,fbe_raid_group_number_t raid_group_id)
{
    squidward_test_index_t iterator;
    squidward_test_index_t start_test_index;
    squidward_test_index_t end_test_index;

    start_test_index = SQUIDWARD_TEST_ZERO_ON_DEMAND_FIRST;
    end_test_index = SQUIDWARD_TEST_ZERO_ON_DEMAND_LAST;
    
        
    for(iterator = start_test_index; iterator < end_test_index; iterator++)
    {
        if(squidward_test_configuration[iterator].raid_group_id == raid_group_id)
        {
            *test_index = iterator;
            return;
        }
    }
}

/*!****************************************************************************
 * @fn squidward_test_perform_io()
 ******************************************************************************
 * @brief
 *  This function perform the normal IO test based on given test index.
 *
 *
 * @param test_context_p - IO test context to run rdgen
 * @param test_index     - test index to run IO
 *
 * @return none
 *
 * @author
 *   11/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static void squidward_test_perform_io(fbe_api_rdgen_context_t * test_context_p, squidward_test_index_t test_index)
{
    fbe_u32_t                   io_op_index ;
    fbe_bool_t                  invalid_pattern;

    /*  start the i/o operation for particular test.*/
    for(io_op_index = 0; io_op_index < SQUIDWARD_MAX_IO_COUNT; io_op_index++)
    {

        squidward_test_perform_single_io(test_context_p, test_index, io_op_index, &invalid_pattern);
        if (invalid_pattern)
        {
            /* break the loop if we find the invalid i/o operation for particular test. */
            break;
        }
    }

    return;
}
/******************************************
 * end squidward_test_perform_io()
 ******************************************/

/*!****************************************************************************
 * @fn squidward_test_perform_single_io()
 ******************************************************************************
 * @brief
 *  This function perform the normal IO test based on given test index.
 *
 *
 * @param test_context_p  - IO test context to run rdgen
 * @param test_index      - test index to run IO
 * @param index           - the index of the IO to perform
 * @param invalid_pattern - the invalid pattern was hit
 *
 * @return none
 *
 * @author
 *   11/17/2011 - Created. Jason White
 *
 ******************************************************************************/
static void squidward_test_perform_single_io(fbe_api_rdgen_context_t * test_context_p, squidward_test_index_t test_index, fbe_u32_t index, fbe_bool_t *invalid_pattern)
{

    fbe_object_id_t             raid_group_object_id;
    fbe_status_t                status;

    *invalid_pattern = FBE_FALSE;

    /*  Get the RG object id from the RG number  */
    fbe_api_database_lookup_raid_group_by_number(squidward_test_configuration[test_index].raid_group_id, &raid_group_object_id);

    if((squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
       (squidward_test_configuration[test_index].rdgen_io_config[index].start_lba != FBE_LBA_INVALID))
    {
        // Perform the requested I/O and make sure there were no errors
        status = fbe_api_rdgen_send_one_io(test_context_p,
                                           raid_group_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_operation,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_pattern,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].start_lba,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].block_count,
                                           FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(test_context_p->start_io.statistics.error_count, 0);
    }
    else
    {
        /* break the loop if we find the invalid i/o operation for particular test. */
        *invalid_pattern = FBE_TRUE;
    }

    return;
}
/******************************************
 * end squidward_test_perform_single_io()
 ******************************************/

/*!****************************************************************************
 * @fn squidward_test_perform_single_io_to_lun()
 ******************************************************************************
 * @brief
 *  This function perform the normal IO test based on given test index.
 *
 * @param lun_number      - lun number
 * @param test_context_p  - IO test context to run rdgen
 * @param test_index      - test index to run IO
 * @param index           - the index of the IO to perform
 * @param invalid_pattern - the invalid pattern was hit
 *
 * @return none
 *
 * @author
 *   02/17/2012 - Created. Sandeep Chaudhari
 *
 ******************************************************************************/
static void squidward_test_perform_single_io_to_lun(fbe_lun_number_t lun_number, fbe_api_rdgen_context_t * test_context_p, squidward_test_index_t test_index, fbe_u32_t index, fbe_bool_t *invalid_pattern)
{

    fbe_object_id_t             lun_object_id;
    fbe_status_t                status;

    *invalid_pattern = FBE_FALSE;

    /*  Get the LUN object id from the LUN number  */
    fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);

    if((squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
       (squidward_test_configuration[test_index].rdgen_io_config[index].start_lba != FBE_LBA_INVALID))
    {
        // Perform the requested I/O and make sure there were no errors
        status = fbe_api_rdgen_send_one_io(test_context_p,
                                           lun_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_operation,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_pattern,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].start_lba,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].block_count,
                                           FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(test_context_p->start_io.statistics.error_count, 0);
    }
    else
    {
        /* break the loop if we find the invalid i/o operation for particular test. */
        *invalid_pattern = FBE_TRUE;
    }

    return;
}

/*!****************************************************************************
 * @fn squidward_test_perform_single_io_to_pvd()
 ******************************************************************************
 * @brief
 *  This function perform the normal IO test based on given test index.
 *
 *
 * @param test_context_p  - IO test context to run rdgen
 * @param test_index      - test index to run IO
 * @param index           - the index of the IO to perform
 * @param invalid_pattern - the invalid pattern was hit
 *
 * @return none
 *
 * @author
 *   11/17/2011 - Created. Jason White
 *
 ******************************************************************************/
static void squidward_test_perform_single_io_to_pvd(fbe_object_id_t object_id, fbe_api_rdgen_context_t * test_context_p, squidward_test_index_t test_index, fbe_u32_t index, fbe_bool_t *invalid_pattern)
{

    fbe_status_t                status;

    *invalid_pattern = FBE_FALSE;

    if((squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
       (squidward_test_configuration[test_index].rdgen_io_config[index].start_lba != FBE_LBA_INVALID))
    {
        // Perform the requested I/O and make sure there were no errors
        status = fbe_api_rdgen_send_one_io_asynch(test_context_p,
                                           object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_operation,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].rdgen_pattern,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].start_lba,
                                           squidward_test_configuration[test_index].rdgen_io_config[index].block_count,
                                           FBE_RDGEN_OPTIONS_INVALID);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(test_context_p->start_io.statistics.error_count, 0);
    }
    else
    {
        /* break the loop if we find the invalid i/o operation for particular test. */
        *invalid_pattern = FBE_TRUE;
    }

    return;
}
/******************************************
 * end squidward_test_perform_single_io_to_pvd()
 ******************************************/

/*!****************************************************************************
 * @fn squidward_test_perform_zod_write_edge_IO()
 ******************************************************************************
 * @brief
 *  This function is used to perform zero on demand IO. It also performs
 *  read verification if the IO has unaligned edge to verify the ZOD logic at PVD
 *  level.
 *
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static void squidward_test_perform_zod_write_edge_IO(fbe_api_rdgen_context_t* in_test_context_p, squidward_test_index_t  test_index)
{

    fbe_object_id_t             raid_group_object_id;
    fbe_status_t                status;
    fbe_u32_t                   io_op_index = 0;
    fbe_lba_t                   start_lba;
    fbe_lba_t                   pre_edge_chunk_start_lba;
    fbe_block_count_t           pre_edge_chunk_block_count;
    fbe_lba_t                   post_edge_chunk_start_lba;
    fbe_block_count_t           post_edge_chunk_block_count;
    fbe_chunk_count_t           number_of_edge_chunks_need_zero;

    /*  Get the RG object id from the RG number  */
    fbe_api_database_lookup_raid_group_by_number(squidward_test_configuration[test_index].raid_group_id, &raid_group_object_id);

    /*  start the i/o operation for particular test. */
    for(io_op_index = 0; io_op_index < SQUIDWARD_MAX_IO_COUNT; io_op_index++)
    {

        if((squidward_test_configuration[test_index].rdgen_io_config[io_op_index].rdgen_operation != FBE_RDGEN_OPERATION_INVALID) &&
           (squidward_test_configuration[test_index].rdgen_io_config[io_op_index].start_lba != FBE_LBA_INVALID))
        {

            /* Perform the requested I/O and make sure there were no errors with READ check on 
             * same write area
             */
            status = fbe_api_rdgen_send_one_io(in_test_context_p, 
                                        raid_group_object_id, 
                                        FBE_CLASS_ID_INVALID, 
                                        FBE_PACKAGE_ID_SEP_0, 
                                        squidward_test_configuration[test_index].rdgen_io_config[io_op_index].rdgen_operation, 
                                        squidward_test_configuration[test_index].rdgen_io_config[io_op_index].rdgen_pattern,
                                        squidward_test_configuration[test_index].rdgen_io_config[io_op_index].start_lba,
                                        squidward_test_configuration[test_index].rdgen_io_config[io_op_index].block_count,
                                        FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                        FBE_API_RDGEN_PEER_OPTIONS_INVALID);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);

            /* find out if the previous IO has pre edge or post edge 
             * pre edge - start lba is unaligned with chunk size
             * post edge -(start lba + block count) is unaligned
             */
            start_lba = squidward_test_configuration[test_index].rdgen_io_config[io_op_index].start_lba;
            status = squidward_test_calculate_unaligned_edges_lba_range(start_lba,
                                        squidward_test_configuration[test_index].rdgen_io_config[io_op_index].block_count,
                                        SQUIDWARD_CHUNK_SIZE,
                                        &pre_edge_chunk_start_lba,
                                        &pre_edge_chunk_block_count,
                                        &post_edge_chunk_start_lba,
                                        &post_edge_chunk_block_count,
                                        &number_of_edge_chunks_need_zero);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


            /* check if there is need to perform additional read verification on edge chunk with
             * relation of previous performed IO
             */
            if( number_of_edge_chunks_need_zero != 0)
            {
                /* check which side need to perform READ additionally */
                if(pre_edge_chunk_start_lba != FBE_LBA_INVALID)
                {

                    /* perform READ verification on pre edge to make sure zeroing done */

                    /* Perform the requested I/O and make sure there were no errors */
                    status = fbe_api_rdgen_send_one_io(in_test_context_p, 
                                            raid_group_object_id, 
                                            FBE_CLASS_ID_INVALID, 
                                            FBE_PACKAGE_ID_SEP_0, 
                                            FBE_RDGEN_OPERATION_READ_ONLY, 
                                            FBE_RDGEN_PATTERN_ZEROS,
                                            pre_edge_chunk_start_lba,
                                            pre_edge_chunk_block_count,
                                            FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                            FBE_API_RDGEN_PEER_OPTIONS_INVALID);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);
                }


                /* perform READ verification on post edge to make sure zeroing done */
                if (post_edge_chunk_start_lba != FBE_LBA_INVALID)
                {
                    /* Perform the requested I/O and make sure there were no errors */
                    status = fbe_api_rdgen_send_one_io(in_test_context_p, 
                                            raid_group_object_id, 
                                            FBE_CLASS_ID_INVALID, 
                                            FBE_PACKAGE_ID_SEP_0, 
                                            FBE_RDGEN_OPERATION_READ_ONLY, 
                                            FBE_RDGEN_PATTERN_ZEROS,
                                            post_edge_chunk_start_lba,
                                            post_edge_chunk_block_count,
                                            FBE_RDGEN_LBA_SPEC_FIXED, 0, 0,
                                            FBE_API_RDGEN_PEER_OPTIONS_INVALID);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    MUT_ASSERT_INT_EQUAL(in_test_context_p->start_io.statistics.error_count, 0);
                }
            }


        }
        else
        {
            /* break the loop if we find the invalid i/o operation for particular test. */
            break;
        }
    }
    

    return;
}
/************************************************
 * end squidward_test_perform_zod_write_edge_IO()
 *************************************************/
 

/*!****************************************************************************
 * @fn squidward_test_perform_random_io()
 ******************************************************************************
 * @brief
 *  This function is used to start random IO tests.
 *
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static void squidward_test_perform_random_io(fbe_api_rdgen_context_t * in_test_context_p, squidward_test_index_t test_index)
{
    fbe_object_id_t             raid_group_object_id;

    /*  Get the RG object id from the RG number 
     */
    fbe_api_database_lookup_raid_group_by_number(squidward_test_configuration[test_index].raid_group_id, &raid_group_object_id);

    squidward_random_io_start(in_test_context_p,FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,raid_group_object_id, 20000, FBE_RDGEN_PATTERN_ZEROS);
    squidward_random_io_start(in_test_context_p,FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,raid_group_object_id, 20000, FBE_RDGEN_PATTERN_LBA_PASS);
    squidward_random_io_start(in_test_context_p,FBE_RDGEN_OPERATION_ZERO_READ_CHECK,raid_group_object_id, 20000, FBE_RDGEN_PATTERN_LBA_PASS);
    squidward_random_io_start(in_test_context_p,FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,raid_group_object_id, 20000, FBE_RDGEN_PATTERN_LBA_PASS);

    return;

} 
/************************************************
 * end squidward_test_perform_random_io()
 *************************************************/

/*!****************************************************************************
 * @fn squidward_random_io_start()
 ******************************************************************************
 * @brief
 *  This function is performs random IO tests.
 *
 *
 * @param context_p             - rdgen test context
 * @param io_operation          - IO operation type
 * @param raid_group_object_id  - raid group object id
 * @param test_duration-        - duration to run the test
 *
 * @return  none
 *
 * @author
 *   11/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static void squidward_random_io_start(fbe_api_rdgen_context_t *context_p, 
                    fbe_rdgen_operation_t io_operation, 
                    fbe_object_id_t raid_group_object_id, 
                    fbe_u32_t test_duration,
                    fbe_rdgen_pattern_t pattern
                    )
{
    fbe_status_t                                status;  
    
    /* Set up for issuing random IO  
     */
    squidward_test_setup_random_io_rdgen_test_context(  context_p,
                                                raid_group_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                io_operation,
                                                FBE_LBA_INVALID,    /* use capacity */
                                                0,                  /* run forever*/ 
                                                3,                  /* threads per lun */
                                                1024,
                                                pattern);

    /* Run I/O test
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for I/O complete for given duration  
     */
    fbe_thread_delay(test_duration);/*let IO run for a while*/

    /* stop the I/O 
      */
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;
    
}
/************************************************
 * end squidward_random_io_start()
 *************************************************/
 

/*!****************************************************************************
 * @fn squidward_test_perform_bgz_with_io()
 ******************************************************************************
 * @brief
 *  This function performs ZOD write IO while running backgroun zeroing operation.
 *
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 *! @todo
 * future extended test implementation
 * BGZ + read
 * BGZ + Zero IO
 *
 *
 * @author
 *   07/14/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t squidward_test_perform_bgz_with_io(fbe_test_rg_configuration_t *rg_config_p,
                                                        fbe_api_rdgen_context_t * test_context_p,
                                                        squidward_test_index_t test_index)
{
    fbe_object_id_t             raid_group_object_id;
    fbe_status_t                status;

    /*  Get the RG object id from the RG number 
     */
    fbe_api_database_lookup_raid_group_by_number(squidward_test_configuration[test_index].raid_group_id, &raid_group_object_id);


     /* Set up for issuing random IO WRITE 
     */
    squidward_test_setup_random_io_rdgen_test_context(test_context_p,
                                                raid_group_object_id,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_RDGEN_OPERATION_WRITE_READ_CHECK ,
                                                FBE_LBA_INVALID,    /* use capacity */
                                                0,      /* run forever*/ 
                                                3,      /* threads per lun */
                                                1024,
                                                FBE_RDGEN_PATTERN_LBA_PASS);


    /* Run our I/O test
     */
    status = fbe_api_rdgen_start_tests(test_context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* start disk zeroing and wait to complete it*/
    squidward_test_run_disk_zeroing(rg_config_p, FBE_TRUE);

    /* stop the I/O test */
    status = fbe_api_rdgen_stop_tests( test_context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


    return FBE_STATUS_OK;

} 
/************************************************
 * end squidward_test_perform_bgz_with_io()
 *************************************************/

/*!****************************************************************************
 * @fn squidward_test_unaligned_io()
 ******************************************************************************
 * @brief
 *  This function performs unaligned IO to a PVD.
 *
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 *! @todo
 * future extended test implementation
 * BGZ + read
 * BGZ + Zero IO
 *
 *
 * @author
 *   11/15/2010 - Created. Jason White
 *
 ******************************************************************************/
static fbe_status_t squidward_test_unaligned_io(fbe_test_rg_configuration_t *in_rg_config_p,
                                                fbe_api_rdgen_context_t * test_context_p,
                                                squidward_test_index_t test_index)
{
    fbe_status_t                status;
    fbe_object_id_t                       object_id;
    fbe_payload_block_operation_t               block_operation;
    fbe_block_transport_block_operation_info_t  block_operation_info;
    fbe_raid_verify_error_counts_t              verify_err_counts={0};
    fbe_sg_element_t * sg_list;
    fbe_lba_t lba;
    fbe_u32_t block_count;
    fbe_u8_t * buffer;
    fbe_api_provision_drive_info_t  provision_drive_info;
    
    /* start disk zeroing */
    squidward_test_run_disk_zeroing(in_rg_config_p, FBE_TRUE);

    /* get the pvd object-id for this position. */
    status = fbe_api_provision_drive_get_obj_id_by_location(in_rg_config_p->rg_disk_set[0].bus,
                                                            in_rg_config_p->rg_disk_set[0].enclosure,
                                                            in_rg_config_p->rg_disk_set[0].slot,
                                                            &object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* Get provision drive info
     */
    status = fbe_api_provision_drive_get_info(object_id, &provision_drive_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    lba = provision_drive_info.default_offset + 0x3250;
    block_count = 0x4b;
    buffer = fbe_api_allocate_memory(block_count * 520 + 2*sizeof(fbe_sg_element_t));
    sg_list = (fbe_sg_element_t *)buffer;

    MUT_ASSERT_INT_NOT_EQUAL(lba % 64, 0);
    MUT_ASSERT_INT_NOT_EQUAL((lba + block_count) % 64, 0);

    /* Send I/O */
    sg_list[0].address = buffer + 2*sizeof(fbe_sg_element_t);
    sg_list[0].count = block_count * 520;

    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    fbe_payload_block_build_operation(&block_operation,
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO,
                                     lba,
                                     block_count,
                                     512,
                                     64,
                                     NULL);

    block_operation_info.block_operation = block_operation;
    block_operation_info.verify_error_counts = verify_err_counts;

    mut_printf(MUT_LOG_LOW, "=== sending unaligned IO\n");

    status = fbe_api_block_transport_send_block_operation(object_id, FBE_PACKAGE_ID_SEP_0, &block_operation_info, sg_list);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
//    @todo: these work when running a single test thread but return invalid when running multiple test threads.  This needs to be investigated.
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST);

    fbe_api_free_memory(buffer);

    return FBE_STATUS_OK;

}
/************************************************
 * end squidward_test_unaligned_io()
 *************************************************/

/*!****************************************************************************
 * @fn squidward_test_check_zeroed_io()
 ******************************************************************************
 * @brief
 *  This function performs a check zero IO on both a zeroed and non zeroed chunk.
 *
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   12/8/2011 - Created. Jason White
 *
 ******************************************************************************/
static fbe_status_t squidward_test_check_zeroed_io(fbe_test_rg_configuration_t *in_rg_config_p,
                                                    fbe_api_rdgen_context_t * test_context_p,
                                                    squidward_test_index_t test_index)
{
    fbe_status_t                status;
    fbe_object_id_t                       object_id;
    fbe_payload_block_operation_t               block_operation;
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_block_transport_block_operation_info_t  block_operation_info;
    fbe_raid_verify_error_counts_t              verify_err_counts={0};
    fbe_sg_element_t * sg_list;
    fbe_lba_t lba;
    fbe_u32_t block_count;
    fbe_u32_t i;
    fbe_u8_t * buffer;
    fbe_api_provision_drive_info_t  pvd_info;

    /* get the pvd object-id for this position. */
    status = fbe_api_provision_drive_get_obj_id_by_location(in_rg_config_p->rg_disk_set[0].bus,
                                                            in_rg_config_p->rg_disk_set[0].enclosure,
                                                            in_rg_config_p->rg_disk_set[0].slot,
                                                            &object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    /* disable background zeroing otherwise it can run on given LBA before we get the block status and can get the 
     * different qualifier status _QUALIFIER_NOT_ZEROED
     */
    fbe_api_base_config_disable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);

    status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
    lba = pvd_info.default_offset;
    block_count = SQUIDWARD_CHUNK_SIZE;

    buffer = fbe_api_allocate_memory(block_count * 520 + 2*sizeof(fbe_sg_element_t));
    sg_list = (fbe_sg_element_t *)buffer;

    /* Send I/O */
    sg_list[0].address = buffer + 2*sizeof(fbe_sg_element_t);
    sg_list[0].count = block_count * 520;

    sg_list[1].address = NULL;
    sg_list[1].count = 0;

        
    /* Send the block operation twice... The first one will zero the lba, the
    * 2nd one will determine that it is already zeroed
    */
    for (i=0; i<2; i++)
    {
        if(i == 0)
        {
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO;
        }
        else
        {
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED;
        }

        fbe_payload_block_build_operation(&block_operation,
                                                            block_opcode,
                                                            lba,
                                                            block_count,
                                                            520,
                                                            1,
                                                            NULL);

        block_operation_info.block_operation = block_operation;
        block_operation_info.verify_error_counts = verify_err_counts;

        status = fbe_api_block_transport_send_block_operation(object_id, FBE_PACKAGE_ID_SEP_0, &block_operation_info, sg_list);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

        if(i == 0)
        {
            MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED);
        }
    }

    /* Send the block operation twice... The first one will write the data, the
    * 2nd one will determine that it is not zeroed
    */
    for (i=0; i<2; i++)
    {
        if(i == 0)
        {
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
        }
        else
        {
            block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED;
        }

        fbe_payload_block_build_operation(&block_operation,
                                                            block_opcode,
                                                            lba,
                                                            block_count,
                                                            520,
                                                            1,
                                                            NULL);

        block_operation_info.block_operation = block_operation;
        block_operation_info.verify_error_counts = verify_err_counts;

        status = fbe_api_block_transport_send_block_operation(object_id, FBE_PACKAGE_ID_SEP_0, &block_operation_info, sg_list);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);

        if(i==0)
        {
            MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(block_operation_info.block_operation.status_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED);
        }
    }

    fbe_api_free_memory(buffer);

    return FBE_STATUS_OK;
}
/************************************************
 * end squidward_test_check_zeroed_io()
 *************************************************/

/*!****************************************************************************
 * @fn squidward_test_run_disk_zeroing()
 ******************************************************************************
 * @brief
 *  This function starts disk zeroing on the associate provision drives and wait
 *  for its completion.
 *
 *
 * @param test_index            - test index to run IO test
 * @param wait_for_completion   - if its FBE_TRUE then wait till disk zeroing gets
 *                                get completed else just start disk zeroing and 
 *                                return.
 * @return  none
 *
 * @author
 *   07/14/2010 - Created. Amit Dhaduk
 *   02/20/2012 - Modified Sandeep Chaudhari
 ******************************************************************************/
void squidward_test_run_disk_zeroing(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_bool_t wait_for_completion)
{
    fbe_u32_t           position;
    fbe_object_id_t     pvd_object_id[SQUIDWARD_TEST_RAID_GROUP_WIDTH];
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE; 

    /* initiate disk zeroing on all rg disk set */
    for(position = 0; position < SQUIDWARD_TEST_RAID_GROUP_WIDTH; position++)
    {
        /* get the pvd object-id for this position. */
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position].bus,
                                                                rg_config_p->rg_disk_set[position].enclosure,
                                                                rg_config_p->rg_disk_set[position].slot,
                                                                &pvd_object_id[position]);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id[position]);

        status = fbe_api_base_config_enable_background_operation(pvd_object_id[position], FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_LOW, "=== disk zeroing started for bus %d, enc %d, slot %d (be patient, it will take some time) \n",
                            rg_config_p->rg_disk_set[position].bus,
                            rg_config_p->rg_disk_set[position].enclosure,
                            rg_config_p->rg_disk_set[position].slot);
    }


    if (wait_for_completion)
    {
        /* wait for disk zeroing to complete on all rg disk set */
        for(position = 0; position < SQUIDWARD_TEST_RAID_GROUP_WIDTH; position++)
        {
            /* wait for disk zeroing complete */
            status = squidward_test_wait_for_disk_zeroing_complete(pvd_object_id[position], 200000);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }
    return;
}
/***************************************************************
 * end squidward_test_run_disk_zeroing()
 ***************************************************************/

/*!****************************************************************************
 *  squidward_test_wait_for_disk_zeroing_complete
 ******************************************************************************
 *
 * @brief
 *   This function checks disk zeroing status in loop and returns the control when disk 
 *   zeroing completed.
 *    
 * @param object_id               - provision drive object id
 * @param timeout_ms              - max timeout value to check for disk zeroing  complete
 * 
 * @return fbe_status_t              fbe status 
 *
 * @author
 *   07/14/2010 - Created. Amit Dhaduk
 *
 *****************************************************************************/
static fbe_status_t squidward_test_wait_for_disk_zeroing_complete(fbe_object_id_t object_id, 
                                                                    fbe_u32_t   timeout_ms)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_lba_t               zero_checkpoint;

    if(!fbe_test_util_is_simulation())
    {
        fbe_api_provision_drive_info_t provision_drive_info;
        // get  provision drive info
        status = fbe_api_provision_drive_get_info( object_id, &provision_drive_info );
        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get the pvd info failed with status %d\n", __FUNCTION__, status);          
            return status;
        }
        zero_checkpoint = provision_drive_info.capacity - (40 * SQUIDWARD_CHUNK_SIZE);
        status = fbe_api_provision_drive_set_zero_checkpoint( object_id, zero_checkpoint);
        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Set the check point failed with status %d\n", __FUNCTION__, status);          
            return status;
        }
    }
    
    while (current_time < timeout_ms){

        current_time = current_time + SQUIDWARD_POLLING_INTERVAL;
        fbe_api_sleep (SQUIDWARD_POLLING_INTERVAL);

        /* get the current disk zeroing checkpoint */
        status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &zero_checkpoint);        
        

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Get the check point failed with status %d\n", __FUNCTION__, status);          
            return status;
        }

        /* check disk zeroing status to see if its complete
         */
        if (zero_checkpoint == FBE_LBA_INVALID){
        
            return status;
        }
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object_id %d checkpoint-0x%llx, %d ms!\n", 
                  __FUNCTION__, object_id,
		  (unsigned long long)zero_checkpoint, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/***************************************************************
 * end squidward_test_wait_for_disk_zeroing_complete()
 ***************************************************************/


/*!****************************************************************************
 * squidward_test_calculate_unaligned_edges_lba_range()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the lba range for the edges (pre and post)
 *  which is not aligned to chunk.
 *
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param pre_edge_start_lba_p          - pre edge start lba.
 * @param pre_edge_block_count_p        - pre edge block count.
 * @param post_edge_start_lba_p         - post edge start lba.
 * @param post_edge_block_count_p       - post edge block count.
 * @param number_of_edges_need_zero_p   - no of edges not covered part of the
 *                                        chunk boundary and need zero(out).
 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  11/02/2009 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t squidward_test_calculate_unaligned_edges_lba_range(fbe_lba_t start_lba,
                                                      fbe_block_count_t block_count,
                                                      fbe_u32_t chunk_size,
                                                      fbe_lba_t * pre_edge_start_lba_p,
                                                      fbe_block_count_t * pre_edge_block_count_p,
                                                      fbe_lba_t * post_edge_start_lba_p,
                                                      fbe_block_count_t * post_edge_block_count_p,
                                                      fbe_u32_t * number_of_edges_p)
{
    fbe_chunk_index_t   start_chunk_index =  FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_count_t   chunk_count = 0;

    /* initialize number of edges which is not aligned as zero. */
    *number_of_edges_p = 0;
    *pre_edge_start_lba_p = FBE_LBA_INVALID;
    *pre_edge_block_count_p = 0;
    *post_edge_start_lba_p = FBE_LBA_INVALID;
    *post_edge_block_count_p = 0;

    /* If lba range is invalid then return error. */
    if(start_lba == FBE_LBA_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if block count less than or equal zero then return error. */
    if(block_count <= 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* determine the chunk range from the given lba range. */
    squidward_test_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);


    /* if start lba is not aligned to chunk size then calculate the pre edge range. */
    if(start_lba % chunk_size)
    {
        *pre_edge_start_lba_p = start_lba -(start_lba % chunk_size) ;
        *pre_edge_block_count_p = (fbe_block_count_t)(start_lba - *pre_edge_start_lba_p); 
        (*number_of_edges_p)++;
    }

    /* if start lba + block_count is not aligned then calculate the post edge range. */
    if((start_lba + block_count) % chunk_size)
    {
        *post_edge_start_lba_p = start_lba + block_count ;
        *post_edge_block_count_p = (fbe_block_count_t) (chunk_size - ((start_lba + block_count) % chunk_size));
        (*number_of_edges_p)++;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_utils_calculate_unaligned_edges_lba_range()
 ******************************************************************************/



/*!****************************************************************************
 * squidward_test_calculate_chunk_range()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the chunk range for the given lba range
 *  for the i/o operation, it rounds the edged on the bounday which is not aligned
 *  and consider it as full chunk.
 *
 * @param start_lba                     - start lba of the range.
 * @param block_count                   - block count of i/o operation.
 * @param chunk_size                    - chunk size.
 * @param start_chunk_index_p           - pointer to the start chunk index (out).
 * @param chunk_count_p                 - pointer to the number of chunks (out).

 *
 * @return fbe_status_t                 - status of the operation.
 *  
 * @author
 *  10/14/2009 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t squidward_test_calculate_chunk_range(fbe_lba_t start_lba,
                                                fbe_block_count_t block_count,
                                                fbe_u32_t chunk_size,
                                                fbe_chunk_index_t * start_chunk_index_p,
                                                fbe_chunk_count_t * chunk_count_p)
{
    fbe_chunk_index_t   start_chunk_index;
    fbe_chunk_index_t   end_chunk_index;

    /* initialize ths start chunk indes as invaid. */
    /* @todo need to update header to get it from fbe_types.h */
    *start_chunk_index_p = FBE_CHUNK_INDEX_INVALID;

    /* If lba range is invalid then return error. */
    if((start_lba == FBE_LBA_INVALID) || (block_count == 0))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the chunk index for the given start lba and end lba. */
    start_chunk_index = (start_lba - (start_lba % chunk_size)) / chunk_size;

    /* get the end chunk index to find the stripe lock range. */
    if((start_lba + block_count) % chunk_size)
    {
        end_chunk_index = (start_lba + block_count + chunk_size - ((start_lba + block_count) % chunk_size)) / chunk_size;
    }
    else
    {
        end_chunk_index = (start_lba + block_count) / chunk_size;
    }

    /* get the chunk count. */
    *chunk_count_p = (fbe_chunk_count_t) (end_chunk_index - start_chunk_index);

    if(*chunk_count_p)
    {
        /* store the start chunk index as return value. */
        *start_chunk_index_p = start_chunk_index;
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end squidward_test_calculate_chunk_range()
 ******************************************************************************/


/*!****************************************************************************
 * @fn squidward_test_setup_random_io_rdgen_test_context()
 ******************************************************************************
 * @brief
 *  This function is used to setup rdgen context to perform random I/O.
 *
 * @note We have created new packet to send the i/o operation, it is required
 *  since curent operation in master packet is metadata operation.  
 *
 * @param context_p             - pointer to rdgen context
 * @param object_id             - object ID
 * @param class_id              - class Id
 * @param rdgen_operation       - rdgen operation
 * @param capacity              - capacity 
 * @param max_passes            - Max pass
 * @param threads               - number of threads to kick off in parallel.
 * @param io_size_blocks        - I/O block size
 *
 * @return  none
 *
 * @author
 *   11/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static void squidward_test_setup_random_io_rdgen_test_context(  fbe_api_rdgen_context_t *context_p,
                                                    fbe_object_id_t object_id,
                                                    fbe_class_id_t class_id,
                                                    fbe_rdgen_operation_t rdgen_operation,
                                                    fbe_lba_t capacity,
                                                    fbe_u32_t max_passes,
                                                    fbe_u32_t threads,
                                                    fbe_u32_t io_size_blocks,
                                                    fbe_rdgen_pattern_t pattern)
{
    fbe_status_t status;

    status = fbe_api_rdgen_test_context_init(   context_p, 
                                                object_id, 
                                                class_id,
                                                FBE_PACKAGE_ID_SEP_0,
                                                rdgen_operation,
                                                pattern,
                                                max_passes,
                                                0, /* io count not used */
                                                0, /* time not used*/
                                                threads,
                                                FBE_RDGEN_LBA_SPEC_RANDOM,
                                                0,    /* Start lba*/
                                                0,    /* Min lba */
                                                capacity, /* max lba */
                                                FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                1,    /* Min blocks */
                                                io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return;

} 
/******************************************************************************
 * end squidward_test_setup_random_io_rdgen_test_context()
 ******************************************************************************/

/*!****************************************************************************
 * @fn squidward_test_unbind_bind_write_io()
 ******************************************************************************
 * @brief
 *  This function is used to perform write IO with different unbind-rebind use cases.
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/02/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
void squidward_test_unbind_bind_write_io(fbe_api_rdgen_context_t * test_context_p, 
                                                        squidward_test_index_t test_index)
{

    fbe_u32_t lun_number;
    fbe_u32_t rg_id;

    lun_number = squidward_raid_group_config[test_index].logical_unit_configuration_list[0].lun_number;
    rg_id =  squidward_raid_group_config[test_index].raid_group_id;

    /* rebind the lun 
     * there was not any IO performed to this lun and we need to cover this usecase to unbind and rebind the 
     * lun without performing any IO on it also. So we will test this use case first as disk is still uninintialized
     */    
    squidward_test_destroy_lun(lun_number);
    squidward_test_create_lun(rg_id, lun_number, 0x20000);

    /* perform the IO test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_io(test_context_p, test_index);

    /* unbidn and rebind the lun */
    squidward_test_destroy_lun(lun_number);
    squidward_test_create_lun(rg_id, lun_number, 0x20000);

    /* perform the IO test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_io(test_context_p, test_index);

    return;

}

/*!****************************************************************************
 * @fn squidward_test_bgz_bind_luns_run_io()
 ******************************************************************************
 * @brief
 *  This function is used to test a bgz and then bind luns and run IO.
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/17/2011 - Created. Jason White
 *
 ******************************************************************************/
void squidward_test_bgz_bind_luns_run_io(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_api_rdgen_context_t * test_context_p,
                                        squidward_test_index_t test_index)
{

    fbe_u32_t lun_number;
    fbe_u32_t rg_id;
    fbe_lba_t capacity;

    rg_id =  squidward_raid_group_config[test_index].raid_group_id;

    squidward_test_run_disk_zeroing(rg_config_p, FBE_TRUE);

    lun_number = squidward_raid_group_config[test_index].logical_unit_configuration_list[0].lun_number;
    capacity = squidward_raid_group_config[test_index].logical_unit_configuration_list[0].capacity;

    /* rebind the lun
     * there was not any IO performed to this lun and we need to cover this usecase to unbind and rebind the
     * lun without performing any IO on it also. So we will test this use case first as disk is still uninintialized
     */
    squidward_test_destroy_lun(lun_number);
    squidward_test_create_lun(rg_id, lun_number, capacity);

    squidward_write_background_pattern();

    squidward_test_perform_random_io(test_context_p, test_index);

    return;

}

/*!****************************************************************************
 * @fn squidward_test_bgz_bind_lun_read_write_zero_io()
 ******************************************************************************
 * @brief
 *  This function is used to test a bgz after a zod write.  The purpose of this
 *  test is to ensure that a zod write before running disk zeroing is not zeroed
 *  out from the disk zeroing.
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/17/2011 - Created. Jason White
 *
 ******************************************************************************/
void squidward_test_bgz_after_zod_write(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_api_rdgen_context_t * test_context_p,
                                        squidward_test_index_t test_index)
{

    fbe_bool_t invalid_pattern;

    /* perform the IO test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_single_io(test_context_p, test_index, 0, &invalid_pattern);
    MUT_ASSERT_FALSE(invalid_pattern);

    squidward_test_run_disk_zeroing(rg_config_p, FBE_TRUE);

    /* Read the IO back to ensure that it is still there */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_single_io(test_context_p, test_index, 1, &invalid_pattern);
    MUT_ASSERT_FALSE(invalid_pattern);

    return;

}

/*!****************************************************************************
 * @fn squidward_test_bgz_zod_quiesce()
 ******************************************************************************
 * @brief
 *  This function is used to test a bgz IO quiesce after a zod write.
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/29/2011 - Created. Jason White
 *
 ******************************************************************************/
void squidward_test_bgz_zod_quiesce(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_api_rdgen_context_t * test_context_p,
                                    squidward_test_index_t test_index)
{

    fbe_bool_t invalid_pattern;
    fbe_object_id_t pvd_object_id;
    fbe_status_t status;
    fbe_scheduler_debug_hook_t            hook;
    fbe_u32_t first_context = 0;
    fbe_u32_t second_context = first_context + 1;

    squidward_test_run_disk_zeroing(rg_config_p, FBE_TRUE);

    /* get the pvd object-id for this position. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                            rg_config_p->rg_disk_set[0].enclosure,
                                                            rg_config_p->rg_disk_set[0].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // set a debug hook
    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

    status = fbe_api_scheduler_add_debug_hook(pvd_object_id,
                                        SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE,
                                        FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY,
                                        NULL,
                                        NULL,
                                        SCHEDULER_CHECK_STATES,
                                        SCHEDULER_DEBUG_ACTION_PAUSE);

    /* perform the IO test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_single_io_to_pvd(pvd_object_id, &test_context_p[first_context], test_index, 0, &invalid_pattern);
    MUT_ASSERT_FALSE(invalid_pattern);

    hook.object_id = pvd_object_id;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE;
    hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    // wait for hook to be hit
    status = sep_zeroing_utils_wait_for_hook(&hook, SQUIDWARD_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* perform the IO test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_single_io_to_pvd(pvd_object_id, &test_context_p[second_context], test_index, 1, &invalid_pattern);
    MUT_ASSERT_FALSE(invalid_pattern);

    /* delete the debug hook */
    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

    status = fbe_api_scheduler_del_debug_hook(pvd_object_id,
                                        SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE,
                                        FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY,
                                        NULL,
                                        NULL,
                                        SCHEDULER_CHECK_STATES,
                                        SCHEDULER_DEBUG_ACTION_PAUSE);

    /* Destroy the sempahore if done unless there is a need 
     * to use the same context for sending another IO.. 
     */
    squidward_wait_for_io_to_complete(test_index, &test_context_p[first_context]);
    mut_printf(MUT_LOG_TEST_STATUS, "=== IOs completed and Semphore (First) Destroyed ===\n");

    squidward_wait_for_io_to_complete(test_index, &test_context_p[second_context]);
    mut_printf(MUT_LOG_TEST_STATUS, "=== IOs completed and Semphore (Second) Destroyed ===\n");

    return;
}

/*!****************************************************************************
 * @fn squidward_test_bgz_exit_zod_quiesce()
 ******************************************************************************
 * @brief
 *  This function is used to test a bgz IO quiesce coming out of ZOD.
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/29/2011 - Created. Jason White
 *
 ******************************************************************************/
void squidward_test_bgz_exit_zod_quiesce(fbe_test_rg_configuration_t *rg_config_p,
                                        fbe_api_rdgen_context_t * test_context_p,
                                        squidward_test_index_t test_index)
{

    fbe_bool_t invalid_pattern;
    fbe_object_id_t pvd_object_id;
    fbe_status_t status;
    fbe_scheduler_debug_hook_t            hook;
    fbe_u32_t first_context = 0;
    fbe_u32_t second_context = first_context + 1;

    /* get the pvd object-id for this position. */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                            rg_config_p->rg_disk_set[0].enclosure,
                                                            rg_config_p->rg_disk_set[0].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // set a debug hook
    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

    status = fbe_api_scheduler_add_debug_hook(pvd_object_id,
                                        SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE,
                                        FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY,
                                        NULL,
                                        NULL,
                                        SCHEDULER_CHECK_STATES,
                                        SCHEDULER_DEBUG_ACTION_PAUSE);


    /* start disk zeroing */
    status = fbe_api_base_config_enable_background_operation(pvd_object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    hook.object_id = pvd_object_id;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE;
    hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    // wait for hook to be hit
    status = sep_zeroing_utils_wait_for_hook(&hook, SQUIDWARD_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* perform the IO test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_single_io_to_pvd(pvd_object_id, &test_context_p[first_context], test_index, 0, &invalid_pattern);
    MUT_ASSERT_FALSE(invalid_pattern);

    /* perform the IO test */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
    squidward_test_perform_single_io_to_pvd(pvd_object_id, &test_context_p[second_context], test_index, 1, &invalid_pattern);
    MUT_ASSERT_FALSE(invalid_pattern);

    // set a debug hook
    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

    status = fbe_api_scheduler_del_debug_hook(pvd_object_id,
                                        SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE,
                                        FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY,
                                        NULL,
                                        NULL,
                                        SCHEDULER_CHECK_STATES,
                                        SCHEDULER_DEBUG_ACTION_PAUSE);

    /* Destroy the sempahore if done unless there is a need 
     * to use the same context for sending another IO.. 
     */
    squidward_wait_for_io_to_complete(test_index, &test_context_p[first_context]);
    mut_printf(MUT_LOG_TEST_STATUS, "=== IOs completed and Semphore (First) Destroyed ===\n");

    squidward_wait_for_io_to_complete(test_index, &test_context_p[second_context]);
    mut_printf(MUT_LOG_TEST_STATUS, "=== IOs completed and Semphore (Second) Destroyed ===\n");

    return;
}

/*!****************************************************************************
 * @fn squidward_test_bgz_then_read()
 ******************************************************************************
 * @brief
 *  This function is used to test a bgz and then read 1/2 chunk, 1 chunk,
 *  2 chunk sizes and verify they they are zeroed.
 *
 * @param in_test_context_p - test context to run rdgen
 * @param test_index        - test index to run IO test
 *
 * @return  none
 *
 * @author
 *   11/17/2011 - Created. Jason White
 *
 ******************************************************************************/
void squidward_test_bgz_then_read(fbe_test_rg_configuration_t *rg_config_p,
                                    fbe_api_rdgen_context_t * test_context_p,
                                    squidward_test_index_t test_index,
                                    fbe_bool_t random_read)
{
    fbe_object_id_t             raid_group_object_id;

    /*  Get the RG object id from the RG number
     */
    fbe_api_database_lookup_raid_group_by_number(squidward_test_configuration[test_index].raid_group_id, &raid_group_object_id);

    squidward_test_run_disk_zeroing(rg_config_p, FBE_TRUE);

    if (random_read)
    {
        squidward_random_io_start(test_context_p,FBE_RDGEN_OPERATION_READ_CHECK,raid_group_object_id, 20000, FBE_RDGEN_PATTERN_ZEROS);
    }
    else
    {
        /* perform the IO test */
        mut_printf(MUT_LOG_TEST_STATUS, "=== Start performing IOs ===\n");
        squidward_test_perform_io(test_context_p, test_index);
    }

    return;

}

/*!****************************************************************************
 *  squidward_hibernation_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to test out the interaction between disk zeroing and
 *   hibernation.
 *
 *
 * @param   rg_config_p - pointer to the RG configuration
 * @param   context_p   - test context to run rdgen
 * @param   test_index  - test index to run IO test
 *
 * @return  None
 *
 *****************************************************************************/
void squidward_hibernation_test(fbe_test_rg_configuration_t *rg_config_p, fbe_api_rdgen_context_t * test_context_p,
                                squidward_test_index_t test_index)
{
    fbe_object_id_t                     pdo_object_id, vd_object_id, lu_object_id, rg_object_id, pvd_object_id[SQUIDWARD_TEST_RAID_GROUP_WIDTH];
    fbe_status_t                        status;
    fbe_bool_t                          invalid_pattern;
    fbe_u32_t                           timeout_ms = 30000;
    fbe_u32_t                           disk_index = 0;
    DWORD                               dwWaitResult;
    
    mut_printf(MUT_LOG_LOW, "=== Disk zeroing interaction test with hibernation ===\n");

    fbe_semaphore_init(&sem, 0, 1);
    status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_LUN |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP |
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE,
                                                                  squidward_command_response_callback_function,
                                                                  &sem,
                                                                  &reg_id);

        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* enable system wide power save */
    status  = fbe_api_enable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    // @todo need to add support for running on hardware

    status = fbe_api_database_lookup_lun_by_number(rg_config_p->logical_unit_configuration_list->lun_number, &lu_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_set_raid_group_power_save_idle_time(rg_object_id, 10);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    for (disk_index = 0; disk_index < rg_config_p->width; disk_index++) {
        status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[disk_index].bus,
                                                                rg_config_p->rg_disk_set[disk_index].enclosure,
                                                                rg_config_p->rg_disk_set[disk_index].slot,
                                                                &pvd_object_id[disk_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Wait for the pvd to come online */
        status = fbe_test_sep_drive_verify_pvd_state(rg_config_p->rg_disk_set[disk_index].bus,
                                                     rg_config_p->rg_disk_set[disk_index].enclosure,
                                                     rg_config_p->rg_disk_set[disk_index].slot,
                                                     FBE_LIFECYCLE_STATE_READY,
                                                     timeout_ms);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* set PVD object idle time */
        status = fbe_api_set_object_power_save_idle_time(pvd_object_id[disk_index], 10);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, disk_index, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* set VD object idle time */
        status = fbe_api_set_object_power_save_idle_time(vd_object_id, 10);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "===Enabling power saving on RG(should propagate to all objects) ===\n");

    status = fbe_api_get_physical_drive_object_id_by_location(rg_config_p->rg_disk_set[0].bus,
                                                              rg_config_p->rg_disk_set[0].enclosure,
                                                              rg_config_p->rg_disk_set[0].slot,
                                                              &pdo_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    expected_object_id = pvd_object_id[0];/*we want to get notification from this PVD*/
    expected_state = FBE_LIFECYCLE_STATE_HIBERNATE;
    
    /* Enable power saving on RG */
    status = fbe_api_enable_raid_group_power_save(rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== waiting up to 300 sec. for system to power save ===\n");

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", pvd_object_id[0]);

    squidward_test_run_disk_zeroing(rg_config_p, FBE_TRUE);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", pvd_object_id[0]);

    mut_printf(MUT_LOG_LOW, "=== waiting for hibernation state for PVD obj id 0x%x\n", pvd_object_id[0]);

    /* Verify the SEP objects is in expected lifecycle State  */
    dwWaitResult = fbe_semaphore_wait_ms(&sem, 300000);
    MUT_ASSERT_TRUE(dwWaitResult != FBE_STATUS_TIMEOUT);
    mut_printf(MUT_LOG_TEST_STATUS, "===  Verified PVD obj id 0x%x is saving power \n", pvd_object_id[0]);

    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id[0],
                                                     FBE_LIFECYCLE_STATE_HIBERNATE,
                                                     60000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== sending zero IO to RG\n");

    squidward_test_perform_single_io_to_lun(rg_config_p->logical_unit_configuration_list[0].lun_number,test_context_p, test_index, 0, &invalid_pattern);
    MUT_ASSERT_FALSE(invalid_pattern);

    mut_printf(MUT_LOG_LOW, "=== waiting for ready state for PVD obj id 0x%x\n", pvd_object_id[0]);

    /* Verify the SEP objects is in expected lifecycle State  */
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id[0],
                                                     FBE_LIFECYCLE_STATE_READY,
                                                     60000,
                                                     FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*we want to clean and go out w/o leaving the power saving persisted on the disks for next test*/
    mut_printf(MUT_LOG_TEST_STATUS, " %s: Disabling system wide power saving \n", __FUNCTION__);
    status  = fbe_api_disable_system_power_save();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    fbe_semaphore_destroy(&sem);
    return;
}
/***************************************************************
 * end squidward_hibernation_test()
 ***************************************************************/

/*!****************************************************************************
 *  squidward_test_create_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for creating a LUN.
 *
 * @param rg id             - raid group number
 * @param lun_number   - number of LUN to create
 * @param capacity        - lun capacity to create
 *
 * @return  None 
 *
 *****************************************************************************/
static void squidward_test_create_lun( fbe_raid_group_number_t rg_id, fbe_lun_number_t   lun_number, fbe_lba_t capacity)
{
    fbe_status_t                       status;
    fbe_api_lun_create_t               fbe_lun_create_req;
    fbe_object_id_t                    object_id;
    fbe_job_service_error_type_t       job_error_type;      
   
    /* Create a LUN */
    fbe_zero_memory(&fbe_lun_create_req, sizeof(fbe_api_lun_create_t));
    fbe_lun_create_req.raid_type        = FBE_RAID_GROUP_TYPE_RAID1;
    fbe_lun_create_req.raid_group_id    = rg_id;
    fbe_lun_create_req.lun_number       = lun_number;
    fbe_lun_create_req.capacity         = capacity;
    fbe_lun_create_req.placement        = FBE_BLOCK_TRANSPORT_BEST_FIT;
    fbe_lun_create_req.ndb_b            = FBE_FALSE;
    fbe_lun_create_req.noinitialverify_b = FBE_FALSE;
    fbe_lun_create_req.addroffset       = FBE_LBA_INVALID;
    fbe_lun_create_req.world_wide_name.bytes[0] = fbe_random() & 0xf;
    fbe_lun_create_req.world_wide_name.bytes[1] = fbe_random() & 0xf;

    status = fbe_api_create_lun(&fbe_lun_create_req, FBE_TRUE, SQUIDWARD_TEST_NS_TIMEOUT, &object_id, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "=== %s: LUN 0x%X created successfully! ===\n", __FUNCTION__, object_id);

    return;

}  
/**************************************************
 * end yabbadoo_test_create_lun()
 **************************************************/

/*!****************************************************************************
 *  squidward_test_destroy_lun
 ******************************************************************************
 *
 * @brief
 *   This is the test function for destroying a LUN.  
 *
 * @param   lun_number - name of LUN to destroy
 *
 * @return  None 
 *
 *****************************************************************************/
static void squidward_test_destroy_lun(fbe_lun_number_t   lun_number)
{
    fbe_status_t                            status;
    fbe_api_lun_destroy_t       fbe_lun_destroy_req;
    fbe_job_service_error_type_t job_error_type;
    
    fbe_lun_destroy_req.lun_number = lun_number;

    /* Destroy a LUN */
    status = fbe_api_destroy_lun(&fbe_lun_destroy_req, FBE_TRUE,  SQUIDWARD_TEST_NS_TIMEOUT, &job_error_type);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    mut_printf(MUT_LOG_TEST_STATUS, "=== LUN destroyed successfully! ===\n");

    return;

}  

/*!**************************************************************
 * squidward_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the Squidward test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/22/2010 - Created. Amit Dhaduk
 *
 ****************************************************************/
void squidward_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== squidward destroy starts ===\n");
    if (fbe_test_util_is_simulation())
    {
        squidward_check_for_and_reset_sep_error_counter(1);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    mut_printf(MUT_LOG_LOW, "=== squidward destroy completed ===\n");
    return;
}
/******************************************
 * end squidward_cleanup()
 ******************************************/

/*!**************************************************************
 * squidward_write_background_pattern()
 ****************************************************************
 * @brief
 *  Seed all the LUNs with a pattern.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  11/17/2011 - Created. Jason White
 *
 ****************************************************************/

void squidward_write_background_pattern(void)
{
    fbe_api_rdgen_context_t *context_p = &squidward_test_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.io_count, 0);

    /* Read back the pattern and check it was written OK.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            128);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end squidward_write_background_pattern()
 ******************************************/

/*********************************************************************
 * squidward_check_for_and_reset_sep_error_counter()
 *********************************************************************
 * @brief
 * checks for the specified number of errors in the SEP package and
 * then resets it.
 *
 * @param - count - The expected error count
 *
 * @return none
 *
 * @author
 * 11/18/2011 - Created. Jason White
 *
 *********************************************************************/
static void squidward_check_for_and_reset_sep_error_counter(fbe_u32_t count)
{
    fbe_api_trace_error_counters_t error_counters;
    fbe_status_t status;

    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_EQUAL(error_counters.trace_critical_error_counter, 0);

    MUT_ASSERT_FALSE(error_counters.trace_error_counter > count);

    /* reset the error counters and remove all error records */
    status = fbe_api_system_reset_failure_info(FBE_PACKAGE_NOTIFICATION_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * squidward_wait_for_io_to_complete()
 ****************************************************************
 * @brief
 *  Wait for the IO's to complete and check the status 
 *
 * @param
 *  fbe_u32_t rg_index
 *
 * @return None.
 *
 ****************************************************************/
static void squidward_wait_for_io_to_complete(fbe_u32_t rg_index, fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t status;
    fbe_u32_t   lun_index;

    for(lun_index = 0; lun_index < SQUIDWARD_TEST_LUNS_PER_RAID_GROUP; lun_index++)
    {
        /* Wait for the IO's to complete before checking the status */
        status = fbe_api_rdgen_wait_for_ios(context_p, FBE_PACKAGE_ID_NEIT, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /* Get the status from the context structure. 
         * Make sure no errors were encountered. 
         */
        status = fbe_api_rdgen_get_status(context_p, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        
    
        /* Destroy all the contexts */
        status = fbe_api_rdgen_test_context_destroy(context_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        context_p ++;
    }
    
    return;
}

static void squidward_command_response_callback_function (update_object_msg_t * update_object_msg, void *context)
{
    fbe_semaphore_t *sem_p = (fbe_semaphore_t *)context;
    fbe_lifecycle_state_t   state;

    fbe_notification_convert_notification_type_to_state(update_object_msg->notification_info.notification_type, &state);
    
    if (update_object_msg->object_id == expected_object_id && expected_state == state) {
        fbe_semaphore_release(sem_p, 0, 1 ,FALSE);
    }
}
/*************************
 * end file squidward_test.c
 *************************/


