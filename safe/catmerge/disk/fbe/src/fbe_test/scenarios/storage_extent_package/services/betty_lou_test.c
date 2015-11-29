/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file betty_lou_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains sector tracing test.
 *  It injects sector errors and checks whether proper information is traced. 
 *
 * @version
 *   06/17/2010 - Created. Omer Miranda
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
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "neit_utils.h"
#include "pp_utils.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_sector_trace_interface.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * betty_lou_short_desc = "sector errors tracing test.";
char * betty_lou_long_desc ="\
The betty_lou scenario tests tracing of sector related errors.\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] viper enclosure\n\
        [PP] SAS drives and SATA drives\n\
        [PP] logical drive\n\
        [SEP] provision drive\n\
        [SEP] virtual drive\n\
        [SEP] raid groups (520)\n\
        [SEP] LUNs\n\
\n"
"STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Create provisioned drives and attach edges to logical drives.\n\
        - Create virtual drives and attach edges to provisioned drives.\n\
        - Create raid 0 - 520 raid group object.\n\
           - raid group attributes:  width: 6 drives  capacity: 0x24000 blocks\n\
        - Attach raid group to 5 provisioned drives for 520 SAS drives.\n\
        - Create and attach 3 LUNs to the raid 0 object.\n\
           - Each LUN will have capacity of 0x9000 blocks and element size of 128.\n\
        - Create raid 5 - 520 raid group object.\n\
           - raid group attributes:  width: 5 drives  capacity: 0x18000 blocks\n\
        - Attach raid group to 5 provisioned drives for 520 SAS drives.\n\
        - Create and attach 3 LUNs to the raid 0 object.\n\
           - Each LUN will have capacity of 0x6000 blocks and element size of 128.\n\
        - Create raid 1 - 520 raid group object.\n\
           - raid group attributes:  width: 6 drives  capacity: 0x6000 blocks\n\
        - Attach raid group to 5 provisioned drives for 520 SAS drives.\n\
        - Create and attach 3 LUNs to the raid 0 object.\n\
           - Each LUN will have capacity of 0x1800 blocks and element size of 128.\n\
    \n"
"STEP 2: Enable rderr uncorrectable table.\n\
    \n"
"STEP 3: Write initial data pattern\n\
        - Use rdgen to write an initial data pattern to all LUN objects.\n\
    \n"
"STEP 4: Inject RAID stripe errors.\n\
        - Use rdgen to inject errors one LUN of each RAID object.\n\
    \n"
"STEP 5: Initiate read-only verify operation.\n\
        - Validate that the LU verify report properly reflects the completed\n\
         verify operation.\n\
    \n"
"STEP 6: Initiate a write verify operation.\n\
        - Validate that the LU verify report properly reflects the completed\n\
          verify operation.\n\
        - Verify that all correctable errors from the prior read-only verify\n\
          operation were not corrected.\n\
        - Display sector trace counters\n\
    \n"
"STEP 7: Initiate a second write verify operation.\n\
        - Validate that the LU verify report properly reflects the completed\n\
          verify operation.\n\
        - Verify that all correctable errors from the prior write verify\n\
          operation were corrected.\n\
    \n"
"STEP 8: Repeat step 4 to step 7 for all raid types\n\
        - Perform the same on R5 and R6 raid groups\n\
    \n"
"Outstanding issue: r10 is not tested\n\
    \n"
"Description last updated: 11/14/2013.\n";

/*!*******************************************************************
 * @def BETTY_LOU_NUM_RAID_GROUPS_TESTED
 *********************************************************************
 * @brief Number of raid groups that are tested
 *********************************************************************/
#define BETTY_LOU_NUM_RAID_GROUPS_TESTED    4

/*!*******************************************************************
 * @def BETTY_LOU_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Number of LUNs in our raid group.
 *********************************************************************/
#define BETTY_LOU_LUNS_PER_RAID_GROUP   1 

/*!*******************************************************************
 * @def BETTY_LOU_ELEMENT_SIZE
 *********************************************************************
 * @brief Element size of the LUNs.
 *********************************************************************/
#define BETTY_LOU_ELEMENT_SIZE          128 

/*!*******************************************************************
 * @def BETTY_LOU_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define BETTY_LOU_CHUNKS_PER_LUN	    3

/*!*******************************************************************
 * @def BETTY_LOU_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define BETTY_LOU_WAIT_MSEC             120000

/*!*******************************************************************
 * @def BETTY_LOU_ERROR_RECORD_COUNT
 *********************************************************************
 * @brief Number of logical error injections.  Minimum required (too
 *        validate certain errors are sector traced):
 *          o CRC errors
 *          o LBA stamp errors
 *          o Coherency errors
 *********************************************************************/
#define BETTY_LOU_ERROR_RECORD_COUNT    3

/*!*******************************************************************
 * @struct betty_lou_error_case_t
 *********************************************************************
 * @brief Definition of logical error insertion case.
 *
 *********************************************************************/
typedef struct betty_lou_error_case_s
{
    char                                       *description;           // error case description
    fbe_raid_group_type_t                       raid_types_supported[BETTY_LOU_NUM_RAID_GROUPS_TESTED];
    fbe_api_logical_error_injection_record_t    record;                // logical error injection record
    fbe_lun_verify_counts_t                     report;
} betty_lou_error_case_t;

/*!*******************************************************************
 * @def betty_lou_error_raid0_record_array
 *********************************************************************
 * @brief Global array that determines which raid group should
 *        inject logical errors.  This table works for all raid groups
 *        with at least (2) positiosn.
 *
 *********************************************************************/
static betty_lou_error_case_t betty_lou_error_raid0_record_array[BETTY_LOU_ERROR_RECORD_COUNT] =
{
    {
        " Inject CRC error on raid0",
        {
            FBE_RAID_GROUP_TYPE_RAID0, FBE_RAID_GROUP_TYPE_UNKNOWN, FBE_RAID_GROUP_TYPE_UNKNOWN, FBE_RAID_GROUP_TYPE_UNKNOWN
        },
        {
            /* logical error injection record */
            0x1,    /*!< Bitmap for position 0 */
            0x10,   /*!< Maximum array width */
            0x0,    /*!< Physical address to begin errs    */
            0x1,    /*!< Blocks to extend the errs for     */
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,   /*!< Type of errors, see above      */
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT,     /*!< Mode of error injection, see above */
            0x0,    /*!< Count of errs injected so far  */
            0x2,    /*!< Limit of errors to inject  (direct I/O and retry) */
            0x0,    /*!< Limit of errors to skip          */
            0x15,   /*!< Limit of errors to inject         */
            0x1,    /*!< Error adjacency bitmask           */
            0x0,    /*!< Starting bit of an error          */
            0x0,    /*!< Number of bits of an error        */
            0x0,    /*!< Whether erroneous bits adjacent   */
            0x0,    /*!< Whether error is CRC detectable   */
            0x0     /*!< If not 0 or invalid, only inject for this opcode. */
        },
        {
            /* expected verify report counters
             */
            0,      /* correctable_single_bit_crc */
            0,      /* uncorrectable_single_bit_crc */
            0,      /* correctable_multi_bit_crc */
            1,      /* uncorrectable_multi_bit_crc */
            0,      /* correctable_write_stamp */
            0,      /* uncorrectable_write_stamp */
            0,      /* correctable_time_stamp */
            0,      /* uncorrectable_time_stamp */
            0,      /* correctable_shed_stamp */
            0,      /* uncorrectable_shed_stamp */
            0,      /* correctable_lba_stamp */
            0,      /* uncorrectable_lba_stamp */
            0,      /* correctable_coherency */
            0,      /* uncorrectable_coherency */
            0,      /* uncorrectable_media */
            0,      /* correctable_media */
            0       /* correctable_soft_media */
        },
    },
    {
        " Inject LBA Stamp for raid0",
        {
            FBE_RAID_GROUP_TYPE_RAID0, FBE_RAID_GROUP_TYPE_UNKNOWN, FBE_RAID_GROUP_TYPE_UNKNOWN, FBE_RAID_GROUP_TYPE_UNKNOWN
        },
        {
            /* logical error injection record */
            0x2,    /*!< Bitmap for position 1 */
            0x10,   /*!< Maximum array width */
            0x800,  /*!< Physical address to begin errs    */
            0x1,    /*!< Blocks to extend the errs for     */
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, /*!< Type of errors, see above      */
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT,         /*!< Mode of error injection, see above */
            0x0,    /*!< Count of errs injected so far     */
            0x2,    /*!< Limit of errors to inject (direct I/O and retry) */
            0x0,    /*!< Limit of errors to skip          */
            0x15,   /*!< Limit of errors to inject         */
            0x1,    /*!< Error adjacency bitmask           */
            0x0,    /*!< Starting bit of an error          */
            0x0,    /*!< Number of bits of an error        */
            0x0,    /*!< Whether erroneous bits adjacent   */
            0x0,    /*!< Whether error is CRC detectable   */
            0x0     /*!< If not 0 or invalid, only inject for this opcode. */
        },
        {
            /* expected verify report counters
             */
            0,      /* correctable_single_bit_crc */
            0,      /* uncorrectable_single_bit_crc */
            0,      /* correctable_multi_bit_crc */
            0,      /* uncorrectable_multi_bit_crc */
            0,      /* correctable_write_stamp */
            0,      /* uncorrectable_write_stamp */
            0,      /* correctable_time_stamp */
            0,      /* uncorrectable_time_stamp */
            0,      /* correctable_shed_stamp */
            0,      /* uncorrectable_shed_stamp */
            0,      /* correctable_lba_stamp */
            1,      /* uncorrectable_lba_stamp */
            0,      /* correctable_coherency */
            0,      /* uncorrectable_coherency */
            0,      /* uncorrectable_media */
            0,      /* correctable_media */
            0       /* correctable_soft_media */
        },
    },
    {
        " Inject media error for raid0",
        {
            FBE_RAID_GROUP_TYPE_RAID0, FBE_RAID_GROUP_TYPE_UNKNOWN, FBE_RAID_GROUP_TYPE_UNKNOWN, FBE_RAID_GROUP_TYPE_UNKNOWN
        },
        {
            /* logical error injection record */
            0x2,    /*!< Bitmap for position 1 */
            0x10,   /*!< Maximum array width */
            0x1000, /*!< Physical address to begin errs    */
            0x1,    /*!< Blocks to extend the errs for     */
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR,   /*!< Type of errors, see above      */
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT,     /*!< Mode of error injection, see above */
            0x0,    /*!< Count of errs injected so far     */
            0x2,    /*!< Limit of errors to inject (direct I/O and retry) */
            0x0,    /*!< Limit of errors to skip          */
            0x15,   /*!< Limit of errors to inject         */
            0x1,    /*!< Error adjacency bitmask           */
            0x0,    /*!< Starting bit of an error          */
            0x0,    /*!< Number of bits of an error        */
            0x0,    /*!< Whether erroneous bits adjacent   */
            0x0,    /*!< Whether error is CRC detectable   */
            0x0     /*!< If not 0 or invalid, only inject for this opcode. */
        },
        {
            /* expected verify report counters
             */
            0,      /* correctable_single_bit_crc */
            0,      /* uncorrectable_single_bit_crc */
            0,      /* correctable_multi_bit_crc */
            0,      /* uncorrectable_multi_bit_crc */
            0,      /* correctable_write_stamp */
            0,      /* uncorrectable_write_stamp */
            0,      /* correctable_time_stamp */
            0,      /* uncorrectable_time_stamp */
            0,      /* correctable_shed_stamp */
            0,      /* uncorrectable_shed_stamp */
            0,      /* correctable_lba_stamp */
            0,      /* uncorrectable_lba_stamp */
            0,      /* correctable_coherency */
            0,      /* uncorrectable_coherency */
            1,      /* uncorrectable_media */
            0,      /* correctable_media */
            0       /* correctable_soft_media */
        },
    },
};
/********************************************** 
 * end betty_lou_error_raid0_record_array[]
 **********************************************/

/*!*******************************************************************
 * @def betty_lou_error_redundant_record_array
 *********************************************************************
 * @brief Global array that determines which raid group should
 *        inject logical errors.  This table works for all raid groups
 *        with at least (2) positiosn.
 *
 *********************************************************************/
static betty_lou_error_case_t betty_lou_error_redundant_record_array[BETTY_LOU_ERROR_RECORD_COUNT] =
{
    {
        " Inject CRC error for all raid types",
        {
            FBE_RAID_GROUP_TYPE_RAID1, FBE_RAID_GROUP_TYPE_RAID5, FBE_RAID_GROUP_TYPE_RAID6, FBE_RAID_GROUP_TYPE_UNKNOWN
        },
        {
            /* logical error injection record */
            0x2,    /*!< Bitmap for position 1 */
            0x10,   /*!< Maximum array width */
            0x0,    /*!< Physical address to begin errs    */
            0x1,    /*!< Blocks to extend the errs for     */
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC,   /*!< Type of errors, see above      */
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT,     /*!< Mode of error injection, see above */
            0x0,    /*!< Count of errs injected so far     */
            0x2,    /*!< Limit of errors to inject (direct I/O and retry) */
            0x0,    /*!< Limit of errors to skip          */
            0x15,   /*!< Limit of errors to inject         */
            0x1,    /*!< Error adjacency bitmask           */
            0x0,    /*!< Starting bit of an error          */
            0x0,    /*!< Number of bits of an error        */
            0x0,    /*!< Whether erroneous bits adjacent   */
            0x0,    /*!< Whether error is CRC detectable   */
            0x0     /*!< If not 0 or invalid, only inject for this opcode. */
        },
        {
            /* expected verify report counters
             */
            0,      /* correctable_single_bit_crc */
            0,      /* uncorrectable_single_bit_crc */
            1,      /* correctable_multi_bit_crc */
            0,      /* uncorrectable_multi_bit_crc */
            0,      /* correctable_write_stamp */
            0,      /* uncorrectable_write_stamp */
            0,      /* correctable_time_stamp */
            0,      /* uncorrectable_time_stamp */
            0,      /* correctable_shed_stamp */
            0,      /* uncorrectable_shed_stamp */
            0,      /* correctable_lba_stamp */
            0,      /* uncorrectable_lba_stamp */
            0,      /* correctable_coherency */
            0,      /* uncorrectable_coherency */
            0,      /* uncorrectable_media */
            0,      /* correctable_media */
            0       /* correctable_soft_media */
        },
    },
    {
        " Inject LBA Stamp error for all raid types",
        {
            FBE_RAID_GROUP_TYPE_RAID1, FBE_RAID_GROUP_TYPE_RAID5, FBE_RAID_GROUP_TYPE_RAID6, FBE_RAID_GROUP_TYPE_UNKNOWN
        },
        {
            /* logical error injection record */
            0x2,    /*!< Bitmap for position 1 (cannot be parity position) */
            0x10,   /*!< Maximum array width */
            0x800,  /*!< Physical address to begin errs    */
            0x1,    /*!< Blocks to extend the errs for     */
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP, /*!< Type of errors, see above      */
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT,         /*!< Mode of error injection, see above */
            0x0,    /*!< Count of errs injected so far     */
            0x2,    /*!< Limit of errors to inject (direct I/O and retry) */
            0x0,    /*!< Limit of errors to skip          */
            0x15,   /*!< Limit of errors to inject         */
            0x1,    /*!< Error adjacency bitmask           */
            0x0,    /*!< Starting bit of an error          */
            0x0,    /*!< Number of bits of an error        */
            0x0,    /*!< Whether erroneous bits adjacent   */
            0x0,    /*!< Whether error is CRC detectable   */
            0x0     /*!< If not 0 or invalid, only inject for this opcode. */
        },
        {
            /* expected verify report counters
             */
            0,      /* correctable_single_bit_crc */
            0,      /* uncorrectable_single_bit_crc */
            0,      /* correctable_multi_bit_crc */
            0,      /* uncorrectable_multi_bit_crc */
            0,      /* correctable_write_stamp */
            0,      /* uncorrectable_write_stamp */
            0,      /* correctable_time_stamp */
            0,      /* uncorrectable_time_stamp */
            0,      /* correctable_shed_stamp */
            0,      /* uncorrectable_shed_stamp */
            1,      /* correctable_lba_stamp */
            0,      /* uncorrectable_lba_stamp */
            0,      /* correctable_coherency */
            0,      /* uncorrectable_coherency */
            0,      /* uncorrectable_media */
            0,      /* correctable_media */
            0       /* correctable_soft_media */
        },
    },
    {
        " Inject coherency error for redundant raid types",
        {
            FBE_RAID_GROUP_TYPE_RAID1, FBE_RAID_GROUP_TYPE_RAID5, FBE_RAID_GROUP_TYPE_RAID6, FBE_RAID_GROUP_TYPE_UNKNOWN
        },
        {
            /* logical error injection record */
            0x1,    /*!< Bitmap for position 0 */
            0x10,   /*!< Maximum array width */
            0x0,    /*!< Physical(logical) address to begin (for coherency this MBZ !)    */
            0x1,    /*!< Blocks to extend the errs for     */
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP,   /*!< Type of errors, see above      */
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT,     /*!< Mode of error injection, see above */
            0x0,    /*!< Count of errs injected so far     */
            0x1,    /*!< Limit of errors to inject         */
            0x0,    /*!< Limit of errors to skip           */
            0x15,   /*!< Limit of errors to inject         */
            0x1,    /*!< Error adjacency bitmask           */
            0x0,    /*!< Starting bit of an error          */
            0x0,    /*!< Number of bits of an error        */
            0x0,    /*!< Whether erroneous bits adjacent   */
            0x0,    /*!< Whether error is CRC detectable   */
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE /*!< If not 0 or invalid, only inject for this opcode. */
        },
        {
            /* expected verify report counters
             */
            0,      /* correctable_single_bit_crc */
            0,      /* uncorrectable_single_bit_crc */
            0,      /* correctable_multi_bit_crc */
            0,      /* uncorrectable_multi_bit_crc */
            0,      /* correctable_write_stamp */
            0,      /* uncorrectable_write_stamp */
            0,      /* correctable_time_stamp */
            0,      /* uncorrectable_time_stamp */
            0,      /* correctable_shed_stamp */
            0,      /* uncorrectable_shed_stamp */
            0,      /* correctable_lba_stamp */
            0,      /* uncorrectable_lba_stamp */
            1,      /* correctable_coherency */
            0,      /* uncorrectable_coherency */
            0,      /* uncorrectable_media */
            0,      /* correctable_media */
            0       /* correctable_soft_media */
        },
    },
};
/********************************************** 
 * end betty_lou_error_redundant_record_array[]
 **********************************************/

/*!*******************************************************************
 * @def BETTY_LOU_VERIFY_AFTER_WRITE_RECORD_COUNT
 *********************************************************************
 * @brief Number of logical error injections.  Minimum required (too
 *        validate certain errors are sector traced):
 *          o Coherency errors
 *********************************************************************/
#define BETTY_LOU_VERIFY_AFTER_WRITE_RECORD_COUNT   1

/*!*******************************************************************
 * @def betty_lou_verify_after_write_record_array
 *********************************************************************
 * @brief Global array that determines which raid group should
 *        inject logical errors.  This table works for all raid groups
 *        with at least (2) positiosn.
 *
 *********************************************************************/
static betty_lou_error_case_t betty_lou_verify_after_write_record_array[BETTY_LOU_VERIFY_AFTER_WRITE_RECORD_COUNT] =
{
    {
        " Inject coherency error for redundant raid-5 types",
        {
            FBE_RAID_GROUP_TYPE_RAID5, FBE_RAID_GROUP_TYPE_UNKNOWN
        },
        {
            /* logical error injection record */
            0x1,    /*!< Bitmap for position 0 */
            0x10,   /*!< Maximum array width */
            0x0,    /*!< Physical(logical) address to begin (for coherency this MBZ !)    */
            0x1,    /*!< Blocks to extend the errs for     */
            FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP,   /*!< Type of errors, see above      */
            FBE_LOGICAL_ERROR_INJECTION_MODE_COUNT,     /*!< Mode of error injection, see above */
            0x0,    /*!< Count of errs injected so far     */
            0x1,    /*!< Limit of errors to inject         */
            0x0,    /*!< Limit of errors to skip           */
            0x15,   /*!< Limit of errors to inject         */
            0x1,    /*!< Error adjacency bitmask           */
            0x0,    /*!< Starting bit of an error          */
            0x0,    /*!< Number of bits of an error        */
            0x0,    /*!< Whether erroneous bits adjacent   */
            0x0,    /*!< Whether error is CRC detectable   */
            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE /*!< If not 0 or invalid, only inject for this opcode. */
        },
        {
            /* expected verify report counters
             */
            0,      /* correctable_single_bit_crc */
            0,      /* uncorrectable_single_bit_crc */
            0,      /* correctable_multi_bit_crc */
            0,      /* uncorrectable_multi_bit_crc */
            0,      /* correctable_write_stamp */
            0,      /* uncorrectable_write_stamp */
            0,      /* correctable_time_stamp */
            0,      /* uncorrectable_time_stamp */
            0,      /* correctable_shed_stamp */
            0,      /* uncorrectable_shed_stamp */
            0,      /* correctable_lba_stamp */
            0,      /* uncorrectable_lba_stamp */
            1,      /* correctable_coherency */
            0,      /* uncorrectable_coherency */
            0,      /* uncorrectable_media */
            0,      /* correctable_media */
            0       /* correctable_soft_media */
        },
    },
};
/********************************************** 
 * end betty_lou_verify_after_write_record_array[]
 **********************************************/

/*!*******************************************************************
 * @var fbe_clifford_test_context_g
 *********************************************************************
 * @brief This contains the context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t betty_lou_test_context_g[BETTY_LOU_LUNS_PER_RAID_GROUP];

/*!*******************************************************************
 * @var betty_lou_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t betty_lou_raid_group_config_qual[BETTY_LOU_NUM_RAID_GROUPS_TESTED + 1] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
	{3,         0x32000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
    {2,         0x32000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},
    {5,         0x32000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {6,         0x32000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
	{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var betty_lou_raid_groups
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t betty_lou_raid_group_config_extended[BETTY_LOU_NUM_RAID_GROUPS_TESTED + 1] = 
{
    /* width,   capacity     raid type,                  class,                 block size      RAID-id.    bandwidth.*/
	{3,         0x32000,      FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
    {2,         0x32000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,   520,            1,          0},
    {5,         0x32000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            2,          0},
    {6,         0x32000,      FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,   520,            3,          0},
	{FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!**************************************************************
 * betty_lou_inject_error_record()
 ****************************************************************
 * @brief   Injects an error record for the specified RG.
 * 
 * @param   rg_config_p - Pointer to raid group test config
 * @param   error_record_p - Pointer to error record to inject
 *
 * @return None.
 *
 * @author
 *  11/14/2013  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t betty_lou_inject_error_record(fbe_test_rg_configuration_t *rg_config_p,
                                                  const fbe_api_logical_error_injection_record_t *const error_record_p)
{
    fbe_status_t                                status;
    fbe_api_logical_error_injection_get_stats_t stats;
    fbe_object_id_t                             rg_object_id;
    fbe_api_logical_error_injection_record_t    local_error_record;

    /* Disable error injection 
     */
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Disable error injection records in the error injection table (initializes records).
     */
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Make a copy of the record since it will be modified.
     */
    local_error_record = *error_record_p;

    /*! @note Handle special case based on raid type.
     */
    switch(rg_config_p->raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID6:
            /*! @note For RAID-6 we don't want to inject these types to a 
             *        parity position.
             */
            switch (error_record_p->err_type)
            {
                case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP:
                case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_COH:
                    /*! @note Simply shift (2) to skip over parity.
                     */
                    local_error_record.pos_bitmap <<= 2;
                    break;

                default:
                    /* No need to modify.
                     */
                    break;
            }
            break;

        default:
            /* No need to modify.
             */
            break;
    }

    /* Now create the record using the local copy.
     */
    status = fbe_api_logical_error_injection_create_record(&local_error_record);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Enable the error injection service
     */
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Enable error injection on the RG.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_api_logical_error_injection_enable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Check error injection stats
     */
    status = fbe_api_logical_error_injection_get_stats(&stats);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(stats.b_enabled, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(stats.num_records, 1);
    MUT_ASSERT_INT_EQUAL(stats.num_objects_enabled, 1);

    return FBE_STATUS_OK;
}
/******************************************
 * end betty_lou_inject_error_record()
 ******************************************/

/*!**************************************************************
 * betty_lou_wait_for_error_injection_complete()
 ****************************************************************
 *
 * @brief   Wait for the error injection to complete
 * 
 * @param   rg_config_p - Pointer to raid group test config
 * @param   error_record_p - Pointer to error record to inject
 *
 * @return  status
 *
 * @author
 *  11/14/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t betty_lou_wait_for_error_injection_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                       const fbe_api_logical_error_injection_record_t *const error_record_p)
{
    fbe_status_t                                        status;
    fbe_u32_t                                           sleep_count;
    fbe_u32_t                                           sleep_period_ms;
    fbe_u32_t                                           max_sleep_count;
    fbe_api_logical_error_injection_get_stats_t         stats; 
    fbe_object_id_t                                     rg_object_id;

    /*  Intialize local variables.
     */
    sleep_period_ms = 100;
    max_sleep_count = BETTY_LOU_WAIT_MSEC / sleep_period_ms;

    /*  Wait for errors to be injected on the RG's PVD objects.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all errors to be injected(detected) ==\n", __FUNCTION__);

    for (sleep_count = 0; sleep_count < max_sleep_count; sleep_count++)
    {
        status = fbe_api_logical_error_injection_get_stats(&stats);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(stats.num_errors_injected >= error_record_p->err_limit)
        {
            /* Break out of the loop.
             */
            mut_printf(MUT_LOG_TEST_STATUS,
		       "== %s found %llu errors injected ==\n", __FUNCTION__,
		       (unsigned long long)stats.num_errors_injected);
            break;
        }
                        
        fbe_api_sleep(sleep_period_ms);
    }

    /*  The error injection took too long and timed out.
     */
    if (sleep_count >= max_sleep_count)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection failed ==\n", __FUNCTION__);
        MUT_ASSERT_TRUE(sleep_count < max_sleep_count);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s Error injection successful ==\n", __FUNCTION__);

    /* Disable error injection on the RG.
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Disable error injection: on RG ==", __FUNCTION__);
    status = fbe_api_logical_error_injection_disable_object(rg_object_id, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Disable the error injection service.
     */
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
} 
/***************************************************
 * end betty_lou_wait_for_error_injection_complete()
 ***************************************************/

/*!***************************************************************************
 *          betty_lou_validate_sector_counts()
 *****************************************************************************
 *
 * @brief   This routine validates the sector trace service has observed and
 *          recorded the expected sector trace invocations.
 * 
 * @param in_index    
 *
 * @return  status
 *
 * @note    The test must reset the sector counters before they are validated.
 *          Currently the exact count is not validated (as long as the count
 *          is equal or greater than the expected).
 *
 * @author
 *  11/12/2013  Ron Proulx  - Created.
 * 
 *****************************************************************************/
static fbe_status_t betty_lou_validate_sector_counts(fbe_u32_t expected_invocations,
                                                     fbe_u32_t expected_total_errors_traced,
                                                     fbe_u32_t expected_total_sectors_traced,
                                                     fbe_sector_trace_error_level_t expected_error_level,
                                                     fbe_u32_t expected_error_level_count,
                                                     fbe_sector_trace_error_flags_t expected_error_flag,
                                                     fbe_u32_t expected_error_count)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       num_bits_set;
    fbe_u32_t       error_type_index;
    fbe_api_sector_trace_get_counters_t sector_trace_counters;

    /* Get the sector trace counters.
     */
    status = fbe_api_sector_trace_get_counters(&sector_trace_counters);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (status != FBE_STATUS_OK)
    {
        return(status);
    }

    /* No errors are expected return success.
     */
    if (expected_total_errors_traced == 0)
    {
        return FBE_STATUS_OK;
    }

    /* Validate level
     */
    if (expected_error_level >= FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT)
    {
        MUT_ASSERT_TRUE(expected_error_level < FBE_SECTOR_TRACE_ERROR_LEVEL_COUNT);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate error flag
     */
    if (expected_error_flag > FBE_SECTOR_TRACE_ERROR_FLAG_MAX)
    {
        MUT_ASSERT_TRUE(expected_error_level <= FBE_SECTOR_TRACE_ERROR_FLAG_MAX);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that error flag has only (1) bit set.
     */
    num_bits_set = fbe_test_sep_utils_get_bitcount((fbe_u32_t)expected_error_flag);
    if (num_bits_set != 1)
    {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s - expected_error_flag: 0x%x doesn't have 1 bit set",
                   __FUNCTION__, expected_error_flag);
        MUT_ASSERT_INT_EQUAL(1, num_bits_set);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Convert the flag to the error type index.
     */
    status = fbe_api_sector_trace_flag_to_index(expected_error_flag, &error_type_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Check each counter.
     */
    if ((sector_trace_counters.total_invocations < expected_invocations)                                ||
        (sector_trace_counters.total_errors_traced < expected_total_errors_traced)                      ||
        (sector_trace_counters.total_sectors_traced < expected_total_sectors_traced)                    ||
        (sector_trace_counters.error_level_counters[expected_error_level] < expected_error_level_count) ||
        (sector_trace_counters.error_type_counters[error_type_index] < expected_error_count)            )
    {
        /* First display the expected values.
         */
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s - expected_invocations: %d expected_total_errors_traced: %d ",
                   __FUNCTION__, expected_invocations, expected_total_errors_traced);
        mut_printf(MUT_LOG_TEST_STATUS,
                   "     expected_total_sectors_traced: %d expected_error_level: %d expected_error_level_count: %d ",
                   expected_total_sectors_traced, expected_error_level, expected_error_level_count);
        mut_printf(MUT_LOG_TEST_STATUS,
                   "     expected_error_flag: 0x%x expected_error_count: %d ",
                   expected_error_flag, expected_error_count);

        /* Now display the counters.
         */
        status = fbe_api_sector_trace_display_counters(FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now assert on the first failure.
         */
        MUT_ASSERT_TRUE(sector_trace_counters.total_invocations >= expected_invocations);
        MUT_ASSERT_TRUE(sector_trace_counters.total_errors_traced >= expected_total_errors_traced);
        MUT_ASSERT_TRUE(sector_trace_counters.total_sectors_traced >= expected_total_sectors_traced);
        MUT_ASSERT_TRUE(sector_trace_counters.error_level_counters[expected_error_level] >= expected_error_level_count);
        MUT_ASSERT_TRUE(sector_trace_counters.error_type_counters[error_type_index] >= expected_error_count);

        /* Return failure.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/*******************************************
 * end of betty_lou_validate_sector_counts()
 *******************************************/

/*!***************************************************************************
 *          betty_lou_validate_raid0_verify_counts()
 *****************************************************************************
 *
 * @brief   This routine validates the expected type and number of error were
 *          detected in the current verify report.
 * 
 * @param   error_case_p - Pointer to error case 
 * @param   verify_report_p - Pointer to verify report with actual counts
 * @param   expected_error_level_p - Address of expected error level to populate
 * @param   expected_error_flag_p - Address of expected error flag to populate
 * @param   expected_error_count_p - Address of expected number of errors
 * @param   expected_sectors_traced_p - Address of expected number of sectors traced
 *
 * @return  status
 *
 * @author
 *  11/12/2013  Ron Proulx  - Created.
 * 
 *****************************************************************************/
static fbe_status_t betty_lou_validate_raid0_verify_counts(betty_lou_error_case_t *error_case_p,
                                                           fbe_lun_verify_report_t *verify_report_p,
                                                           fbe_sector_trace_error_level_t *expected_error_level_p,
                                                           fbe_sector_trace_error_flags_t *expected_error_flag_p,
                                                           fbe_u32_t *expected_error_count_p,
                                                           fbe_u32_t *expected_sectors_traced_p)
{
    /* Initialize the ouputs ot invalid.
     */
    *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_MAX;
    *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;
    *expected_error_count_p = 0;
    *expected_sectors_traced_p = 0;

    /* Determine the error expected.
     */
    switch(error_case_p->record.err_type)
    {
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC:
            /* Validate un-correctable CRC count.
             */
            if ((error_case_p->report.uncorrectable_multi_bit_crc == 0)                                                  ||
                (verify_report_p->current.uncorrectable_multi_bit_crc != error_case_p->report.uncorrectable_multi_bit_crc)    )
            {
                /* Print a message and then fail.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,
                           "%s: Actual uncorrectable_multi_bit_crc: %d does not match expected: %d",
                           __FUNCTION__, verify_report_p->current.uncorrectable_multi_bit_crc,
                           error_case_p->report.uncorrectable_multi_bit_crc);
                MUT_ASSERT_INT_NOT_EQUAL(0, error_case_p->report.uncorrectable_multi_bit_crc);
                MUT_ASSERT_INT_EQUAL(error_case_p->report.uncorrectable_multi_bit_crc, verify_report_p->current.uncorrectable_multi_bit_crc);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* The counts match.  Populate the sector trace information.
             */
            *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
            *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_UCRC;
            *expected_error_count_p = error_case_p->record.err_limit;
            *expected_sectors_traced_p = error_case_p->record.err_limit;
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP:
            /* Validate un-correctable LBA stamp count.
             */
            if ((error_case_p->report.uncorrectable_lba_stamp == 0)                                                ||
                (verify_report_p->current.uncorrectable_lba_stamp != error_case_p->report.uncorrectable_lba_stamp)    )
            {
                /* Print a message and then fail.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,
                           "%s: Actual uncorrectable_lba_stamp: %d does not match expected: %d",
                           __FUNCTION__, verify_report_p->current.uncorrectable_lba_stamp,
                           error_case_p->report.uncorrectable_lba_stamp);
                MUT_ASSERT_INT_NOT_EQUAL(0, error_case_p->report.uncorrectable_lba_stamp);
                MUT_ASSERT_INT_EQUAL(error_case_p->report.uncorrectable_lba_stamp, verify_report_p->current.uncorrectable_lba_stamp);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* The counts match.  Populate the sector trace information.
             */
            *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
            *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_LBA;
            *expected_error_count_p = error_case_p->record.err_limit;
            *expected_sectors_traced_p = error_case_p->record.err_limit;
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_MEDIA_ERROR:
            /* Validate uncorrectable media error count.
             */
            if ((error_case_p->report.uncorrectable_media == 0)                                            ||
                (verify_report_p->current.uncorrectable_media != error_case_p->report.uncorrectable_media)    )
            {
                /* Print a message and then fail.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,
                           "%s: Actual uncorrectable_media: %d does not match expected: %d",
                           __FUNCTION__, verify_report_p->current.uncorrectable_media,
                           error_case_p->report.uncorrectable_media);
                MUT_ASSERT_INT_NOT_EQUAL(0, error_case_p->report.uncorrectable_media);
                MUT_ASSERT_INT_EQUAL(error_case_p->report.uncorrectable_media, verify_report_p->current.uncorrectable_media);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /*! @note No errors or sectors should be traced for media errors.
             *        Populate the sector trace information.
             */
            *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
            *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;
            *expected_error_count_p = 0;
            *expected_sectors_traced_p = 0;
            break;

        default:
            /* Unexpected/unsupported error type.
             */
            mut_printf(MUT_LOG_TEST_STATUS,
                       "%s: err type: 0x%x is not supported !",
                       __FUNCTION__, error_case_p->record.err_type);
            MUT_ASSERT_INT_EQUAL(FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, error_case_p->record.err_type);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**************************************************
 * end betty_lou_validate_raid0_verify_counts()
 **************************************************/

/*!***************************************************************************
 *          betty_lou_validate_redundant_verify_counts()
 *****************************************************************************
 *
 * @brief   This routine validates the expected type and number of error were
 *          detected in the current verify report.
 * 
 * @param   rg_config_p - Pointer to test raid group config to inject to
 * @param   error_case_p - Pointer to error case 
 * @param   verify_report_p - Pointer to verify report with actual counts
 * @param   expected_error_level_p - Address of expected error level to populate
 * @param   expected_error_flag_p - Address of expected error flag to populate
 * @param   expected_error_count_p - Address of expected number of errors
 * @param   expected_sectors_traced_p - Address of expected number of sectors traced
 *
 * @return  status
 *
 * @author
 *  11/12/2013  Ron Proulx  - Created.
 * 
 *****************************************************************************/
static fbe_status_t betty_lou_validate_redundant_verify_counts(fbe_test_rg_configuration_t *rg_config_p,
                                                               betty_lou_error_case_t *error_case_p,
                                                               fbe_lun_verify_report_t *verify_report_p,
                                                               fbe_sector_trace_error_level_t *expected_error_level_p,
                                                               fbe_sector_trace_error_flags_t *expected_error_flag_p,
                                                               fbe_u32_t *expected_error_count_p,
                                                               fbe_u32_t *expected_sectors_traced_p)
{
    /* Initialize the ouputs ot invalid.
     */
    *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_MAX;
    *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_NONE;
    *expected_error_count_p = 0;
    *expected_sectors_traced_p = 0;

    /* Determine the error expected.
     */
    switch(error_case_p->record.err_type)
    {
        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC:
            /* Validate correctable CRC count.
             */
            if ((error_case_p->report.correctable_multi_bit_crc == 0)                                                  ||
                (verify_report_p->current.correctable_multi_bit_crc != error_case_p->report.correctable_multi_bit_crc)    )
            {
                /* Print a message and then fail.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,
                           "%s: Actual correctable_multi_bit_crc: %d does not match expected: %d",
                           __FUNCTION__, verify_report_p->current.correctable_multi_bit_crc,
                           error_case_p->report.correctable_multi_bit_crc);
                MUT_ASSERT_INT_NOT_EQUAL(0, error_case_p->report.correctable_multi_bit_crc);
                MUT_ASSERT_INT_EQUAL(error_case_p->report.correctable_multi_bit_crc, verify_report_p->current.correctable_multi_bit_crc);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* The counts match.  Populate the sector trace information.
             */
            *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
            *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_UCRC;
            *expected_error_count_p = error_case_p->record.err_limit;
            *expected_sectors_traced_p = error_case_p->record.err_limit;
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_LBA_STAMP:
            /* Validate correctable LBA stamp count.
             */
            if ((error_case_p->report.correctable_lba_stamp == 0)                                                  ||
                (verify_report_p->current.correctable_lba_stamp != error_case_p->report.correctable_lba_stamp)    )
            {
                /* Print a message and then fail.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,
                           "%s: Actual correctable_lba_stamp: %d does not match expected: %d",
                           __FUNCTION__, verify_report_p->current.correctable_lba_stamp,
                           error_case_p->report.correctable_lba_stamp);
                MUT_ASSERT_INT_NOT_EQUAL(0, error_case_p->report.correctable_lba_stamp);
                MUT_ASSERT_INT_EQUAL(error_case_p->report.correctable_lba_stamp, verify_report_p->current.correctable_lba_stamp);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* The counts match.  Populate the sector trace information.
             */
            *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
            *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_LBA;
            *expected_error_count_p = error_case_p->record.err_limit;
            *expected_sectors_traced_p = error_case_p->record.err_limit;
            break;

        case FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP:
            /* Validate correctable coherency error count.
             */
            if ((error_case_p->report.correctable_coherency == 0)                                                  ||
                (verify_report_p->current.correctable_coherency != error_case_p->report.correctable_coherency)    )
            {
                /* Print a message and then fail.
                 */
                mut_printf(MUT_LOG_TEST_STATUS,
                           "%s: Actual correctable_coherency: %d does not match expected: %d",
                           __FUNCTION__, verify_report_p->current.correctable_coherency,
                           error_case_p->report.correctable_coherency);
                MUT_ASSERT_INT_NOT_EQUAL(0, error_case_p->report.correctable_coherency);
                MUT_ASSERT_INT_EQUAL(error_case_p->report.correctable_coherency, verify_report_p->current.correctable_coherency);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* The counts match.  Populate the sector trace information.
             */
            *expected_error_level_p = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
            *expected_error_flag_p = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
            *expected_error_count_p = error_case_p->record.err_limit;
            *expected_sectors_traced_p = error_case_p->record.err_limit * rg_config_p->width;
            break;

        default:
            /* Unexpected/unsupported error type.
             */
            mut_printf(MUT_LOG_TEST_STATUS,
                       "%s: err type: 0x%x is not supported !",
                       __FUNCTION__, error_case_p->record.err_type);
            MUT_ASSERT_INT_EQUAL(FBE_API_LOGICAL_ERROR_INJECTION_TYPE_CRC, error_case_p->record.err_type);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**************************************************
 * end betty_lou_validate_redundant_verify_counts()
 **************************************************/

/*!**************************************************************************
 * betty_lou_write_background_pattern()
 ***************************************************************************
 * @brief
 *  This function writes a data pattern to all the specified LUN.
 * 
 * @param   lun_object_id - Lun object to write to
 * @param   blocks_per_io - Blocks per request
 *
 * @return void
 *
 ***************************************************************************/
static void betty_lou_write_background_pattern(fbe_object_id_t lun_object_id,
                                               fbe_block_count_t blocks_per_io)
{
    fbe_api_rdgen_context_t *context_p = &betty_lou_test_context_g[0];

    /* First fill the raid group with a pattern.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  lun_object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                  FBE_LBA_INVALID, /* use max capacity */
                                                  blocks_per_io);
    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return;
}
/*******************************************
 * end betty_lou_write_background_pattern()
 *******************************************/

/*!**************************************************************************
 * betty_lou_inject_silent_drop()
 ***************************************************************************
 * @brief   This function executes a single write to generate the `silent'
 *          dropping of a write request.
 * 
 * @param   error_case_p - Pointer to error case
 * @param   lun_object_id - Lun object to write to
 *
 * @return  status
 *
 ***************************************************************************/
static fbe_status_t betty_lou_inject_silent_drop(betty_lou_error_case_t *error_case_p,
                                                 fbe_object_id_t lun_object_id)
{
    fbe_status_t            status;
    fbe_api_rdgen_context_t *context_p = &betty_lou_test_context_g[0];

    /* Execute the I/O specified in the error record
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             lun_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             0,    /* Pass not used */
                                             1,    /* num ios used*/
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_FIXED,
                                             error_case_p->record.lba,    /* Start lba*/
                                             error_case_p->record.lba,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             error_case_p->record.blocks,    /* Number of stripes to write. */
                                             error_case_p->record.blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);


    return status;
}
/*******************************************
 * end betty_lou_inject_silent_drop()
 *******************************************/

/*!**************************************************************************
 *          betty_lou_test_inject_error_validate()
 ****************************************************************************
 *
 * @brief   Inject the error described in the error record and validate
 *          the sector counters are as expected.
 * 
 * @param   rg_config_p - Pointer to test raid group config to inject to
 * @param   error_case_p - Pointer to error case 
 *
 * @return  status
 *
 * @author
 *  11/14/2013  Ron Proulx  - Created
 *
 ***************************************************************************/
static fbe_status_t betty_lou_test_inject_error_validate(fbe_test_rg_configuration_t *rg_config_p,
                                                         betty_lou_error_case_t *error_case_p)
{
    fbe_status_t                            status;
    fbe_object_id_t                         rg_object_id;
    fbe_api_raid_group_get_info_t           raid_group_info;
    fbe_u32_t                               type_index;
    fbe_object_id_t                         lun_object_id;              /* LUN object to test */
    fbe_bool_t                              b_record_valid_for_raid_type = FBE_FALSE;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;
    fbe_u32_t                               monitor_substate;
    fbe_lun_verify_report_t                 verify_report;
    fbe_api_lun_get_status_t                lun_verify_status;
    fbe_sector_trace_error_level_t          expected_error_level;
    fbe_sector_trace_error_flags_t          expected_error_flag;
    fbe_u32_t                               expected_error_count;
    fbe_u32_t                               expected_sectors_traced;

    /* Make sure the raid group does not shoot drives when it sees the errors since it will prevent the verifies 
     * from making progress.
     */
    clifford_disable_sending_pdo_usurper(rg_config_p);

    /* Trace the stat of this record
     */
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: raid type: %d err type: 0x%x",
               __FUNCTION__, rg_config_p->raid_type, error_case_p->record.err_type);

    /* Get the raid group object id so that we can check for the verify completion.
     * Get the raid group info so that we can write full stripes.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run test against first LUN.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);

    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    /* Step 0: Let LUN reach ready state and system verify complete before we proceed.
     */
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         3000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    /* Step 1: Write the entire LUN to clear any errors left from previous test.
     */
    betty_lou_write_background_pattern(lun_object_id, raid_group_info.sectors_per_stripe);

    /* Step 2: Reset the sector trace counters before we start injecting 
     *         errors. Initialize the verify report data.
     */
    status = fbe_api_sector_trace_reset_counters();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

    /* Step 3: Validate that this record is applicable for this raid type.
     */
    for (type_index = 0; type_index < BETTY_LOU_NUM_RAID_GROUPS_TESTED; type_index++)
    {
        if (error_case_p->raid_types_supported[type_index] == rg_config_p->raid_type)
        {
            b_record_valid_for_raid_type = FBE_TRUE;
            break;
        }
    }
    if (b_record_valid_for_raid_type == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s: Error record with err type: 0x%x is not valid for raid type: %d", 
                   __FUNCTION__, error_case_p->record.err_type, rg_config_p->raid_type);
        return FBE_STATUS_OK;
    }

    /* Step 4: Populate and enable the error injection.
     */
    status = betty_lou_inject_error_record(rg_config_p, &error_case_p->record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4a: If this is a coherency error (silent drop) issue to write that
     *          result in a coherency error.
     */
    if (error_case_p->record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP)
    {
        /* Inject a `silent' drop error
         */
        status = betty_lou_inject_silent_drop(error_case_p, lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Error are injected on writes.  Wait for all the errors 
         * to be injected.
         */
        status = betty_lou_wait_for_error_injection_complete(rg_config_p, &error_case_p->record);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 4b: Set a debug hook so that we don't miss the verify start.
     */
    fbe_test_verify_get_verify_start_substate(FBE_LUN_VERIFY_TYPE_USER_RW, &monitor_substate);
    status = fbe_test_add_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5: Run a R/W verify.  This will cause the errors to be detected.
     */
    fbe_test_sep_util_lun_initiate_verify(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW,
                                          FBE_TRUE, /* Verify the entire lun */   
                                          FBE_LUN_VERIFY_START_LBA_LUN_START,     
                                          FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5a: Wait for the raid group to start the verify.
     */ 
    status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                 monitor_substate,
                                                 SCHEDULER_CHECK_STATES, 
                                                 SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5b: Wait for the lun verify to start.
     */
    status = fbe_test_sep_util_wait_for_verify_start(lun_object_id, 
                                                     FBE_LUN_VERIFY_TYPE_USER_RW,
                                                     &lun_verify_status);                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 5c: Remove the raid group debug hook.
     */
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6: Validate that the error was inject and disable error injection.
     */
    if (error_case_p->record.err_type != FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP)
    {
        status = betty_lou_wait_for_error_injection_complete(rg_config_p, &error_case_p->record);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 7: Wait for the R/W Verify to complete.
     */
    status = fbe_test_verify_wait_for_verify_completion(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS, FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_get_bgverify_status(lun_object_id, &lun_verify_status, FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: raid type: %d err type: 0x%x lun obj: 0x%x R/W Verify chkpt: 0x%llx percent: %d", 
               __FUNCTION__, rg_config_p->raid_type, 
               error_case_p->record.err_type, lun_object_id,
               lun_verify_status.checkpoint, lun_verify_status.percentage_completed);

    /* Step 8: Validate the expected error count.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        status = betty_lou_validate_raid0_verify_counts(error_case_p,
                                                        &verify_report,
                                                        &expected_error_level,
                                                        &expected_error_flag,
                                                        &expected_error_count,
                                                        &expected_sectors_traced);
    }
    else
    {
        status = betty_lou_validate_redundant_verify_counts(rg_config_p, error_case_p,
                                                            &verify_report,
                                                            &expected_error_level,
                                                            &expected_error_flag,
                                                            &expected_error_count,
                                                            &expected_sectors_traced);
    }
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9: Validate the sector trace counters.
     */
    status = betty_lou_validate_sector_counts(expected_error_count,
                                              expected_error_count,
                                              expected_sectors_traced,
                                              expected_error_level,
                                              expected_error_count,
                                              expected_error_flag,
                                              expected_error_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Step 10: Clear the lun verify counters (report) and the sector trace
     *         counters.
     */
    status = fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = fbe_api_sector_trace_reset_counters();
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return FBE_STATUS_OK;
}
/*******************************************
 * end betty_lou_test_inject_error_validate()
 *******************************************/

/*!**************************************************************************
 * betty_lou_issue_write_with_verify()
 ***************************************************************************
 * @brief   This function executes a single write to generate a `verify after
 *          write'.
 * 
 * @param   error_case_p - Pointer to error case
 * @param   rg_object_id - Raid group object id
 * @param   lun_object_id - Lun object to write to
 *
 * @return  status
 *
 ***************************************************************************/
static fbe_status_t betty_lou_issue_write_with_verify(betty_lou_error_case_t *error_case_p,
                                                      fbe_object_id_t rg_object_id,
                                                      fbe_object_id_t lun_object_id)
{
    fbe_status_t                    status;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_raid_group_map_info_t       original_map_info;
    fbe_raid_group_map_info_t       new_map_info;
    fbe_api_rdgen_context_t        *context_p = &betty_lou_test_context_g[0];
    fbe_lba_t                       lba_to_inject = FBE_LBA_INVALID;

    /* Determine the position from the lba.
     */
    original_map_info.lba = error_case_p->record.lba;
    status = fbe_api_raid_group_map_lba(rg_object_id, &original_map_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /*! @note Currently this only works for RAID-5 or RAID-3.
     */
    if ((raid_group_info.raid_type != FBE_RAID_GROUP_TYPE_RAID3) && 
        (raid_group_info.raid_type != FBE_RAID_GROUP_TYPE_RAID5)    ) {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s: raid type: %d not supported. Must be RAID-3 or RAID-5.", 
                   __FUNCTION__, raid_group_info.raid_type);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Simply add the element size to the current lba.
     */
    lba_to_inject = error_case_p->record.lba + raid_group_info.element_size;
    new_map_info.lba = lba_to_inject;
    status = fbe_api_raid_group_map_lba(rg_object_id, &new_map_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(new_map_info.data_pos != original_map_info.data_pos)
    MUT_ASSERT_TRUE(new_map_info.pba == original_map_info.pba)
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: changed err type: 0x%x lba: 0x%llx pba: 0x%llx pos: %d to lba: 0x%llx pba: 0x%llx pos: %d", 
               __FUNCTION__, error_case_p->record.err_type, 
               original_map_info.lba, original_map_info.pba, original_map_info.data_pos,
               new_map_info.lba, new_map_info.pba, new_map_info.data_pos);

    /* Execute the I/O specified in the error record
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             lun_object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             0,    /* Pass not used */
                                             1,    /* num ios used*/
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_FIXED,
                                             lba_to_inject,    /* Start lba*/
                                             lba_to_inject,    /* Min lba */
                                             FBE_LBA_INVALID,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             error_case_p->record.blocks,    /* Number of stripes to write. */
                                             error_case_p->record.blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Run our I/O test.
     */
    fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return status;
}
/*******************************************
 * end betty_lou_issue_write_with_verify()
 *******************************************/

/*!**************************************************************************
 *          betty_lou_test_verify_after_write_validate()
 ****************************************************************************
 *
 * @brief   Inject the error described in the error record and validate
 *          that the expected number of coherency errors (detected by the
 *          automatic verify of `verify after write').
 * 
 * @param   rg_config_p - Pointer to test raid group config to inject to
 * @param   error_case_p - Pointer to error case
 *
 * @return  status
 *
 * @author
 *  11/14/2013  Ron Proulx  - Created
 *
 ***************************************************************************/
static fbe_status_t betty_lou_test_verify_after_write_validate(fbe_test_rg_configuration_t *rg_config_p,
                                                               betty_lou_error_case_t *error_case_p)
{
    fbe_status_t                            status;
    fbe_object_id_t                         rg_object_id;
    fbe_api_raid_group_get_info_t           raid_group_info;
    fbe_u32_t                               type_index;
    fbe_object_id_t                         lun_object_id;              /* LUN object to test */
    fbe_bool_t                              b_record_valid_for_raid_type = FBE_FALSE;
    fbe_test_logical_unit_configuration_t  *logical_unit_configuration_p = NULL;
    fbe_u32_t                               pass_count = 0;
    fbe_lun_verify_report_t                 verify_report;
    fbe_lun_verify_report_t                 *verify_report_p = &verify_report;
    fbe_sector_trace_error_level_t          expected_error_level;
    fbe_sector_trace_error_flags_t          expected_error_flag;
    fbe_u32_t                               expected_error_count;
    fbe_u32_t                               expected_sectors_traced;
    fbe_u32_t                               monitor_substate;
    fbe_api_lun_get_status_t                lun_verify_status;

    /* Trace the stat of this record
     */
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: raid type: %d err type: 0x%x",
               __FUNCTION__, rg_config_p->raid_type, error_case_p->record.err_type);

    /* Get the raid group object id so that we can check for the verify completion.
     * Get the raid group info so that we can write full stripes.
     */
    status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);


    /* This test is not valid for RAID-0.
     */
    if (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s: raid id: %d rg obj: 0x%x err type: RAID-0 is not supported",
                   __FUNCTION__, rg_config_p->raid_group_id, rg_object_id);
        return FBE_STATUS_OK;
    }

    /* Run test against first LUN.
     */
    logical_unit_configuration_p = rg_config_p->logical_unit_configuration_list;
    MUT_ASSERT_NOT_NULL(logical_unit_configuration_p);
       
    fbe_api_database_lookup_lun_by_number(logical_unit_configuration_p->lun_number, &lun_object_id);

    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);

    /* Step 0: Let LUN reach ready state and system verify complete before we proceed.
     */
    status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                     3000, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, verify_report_p, pass_count);

    /* Step 1: Write the entire LUN to clear any errors left from previous test.
     */
    betty_lou_write_background_pattern(lun_object_id, raid_group_info.sectors_per_stripe);

    /* Step 2: Reset the sector trace counters before we start injecting 
     *         errors. Initialize the verify report data.
     */
    status = fbe_api_sector_trace_reset_counters();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    fbe_test_sep_util_lun_clear_verify_report(lun_object_id);

    /* Step 3: Validate that this record is applicable for this raid type.
     */
    for (type_index = 0; type_index < BETTY_LOU_NUM_RAID_GROUPS_TESTED; type_index++) {
        if (error_case_p->raid_types_supported[type_index] == rg_config_p->raid_type) {
            b_record_valid_for_raid_type = FBE_TRUE;
            break;
        }
    }
    if (b_record_valid_for_raid_type == FBE_FALSE) {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "%s: Error record with err type: 0x%x is not valid for raid type: %d", 
                   __FUNCTION__, error_case_p->record.err_type, rg_config_p->raid_type);
        return FBE_STATUS_OK;
    }

    /* Step 4: Populate and enable the error injection.
     */
    status = betty_lou_inject_error_record(rg_config_p, &error_case_p->record);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 4a: If this is a coherency error (silent drop) issue to write that
     *          result in a coherency error.
     */
    if (error_case_p->record.err_type == FBE_API_LOGICAL_ERROR_INJECTION_TYPE_SILENT_DROP) {
        /* Inject a `silent' drop error
         */
        status = betty_lou_inject_silent_drop(error_case_p, lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Error are injected on writes.  Wait for all the errors 
         * to be injected.
         */
        status = betty_lou_wait_for_error_injection_complete(rg_config_p, &error_case_p->record);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Step 5: Verify that no sector trace has occurred and validate that no verify is marked.
     */
    status = fbe_test_verify_wait_for_verify_completion(rg_object_id,
                                                        10000,
                                                        FBE_LUN_VERIFY_TYPE_ERROR);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_verify_wait_for_verify_completion(rg_object_id,
                                                        10000,
                                                        FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = betty_lou_validate_sector_counts(0,
                                              0,
                                              0,
                                              FBE_SECTOR_TRACE_ERROR_LEVEL_INFO,
                                              0,
                                              FBE_SECTOR_TRACE_ERROR_FLAG_NONE,
                                              0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 6:  Enable `verify after write' debug flag.
     */
    status = fbe_api_raid_group_set_group_debug_flags(rg_object_id, 
                                                      FBE_RAID_GROUP_DEBUG_FLAG_FORCE_WRITE_VERIFY);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 7: Using the error record, issue a write that should not touch the
     *         the previous write position.
     */
    status = betty_lou_issue_write_with_verify(error_case_p, rg_object_id, lun_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 8:  Validate the expected coherency counts.
     */
    expected_error_level = FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR;
    expected_error_flag = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
    expected_error_count = error_case_p->record.err_limit;
    expected_sectors_traced = error_case_p->record.err_limit * rg_config_p->width;
    status = betty_lou_validate_sector_counts(expected_error_count,
                                              expected_error_count,
                                              expected_sectors_traced,
                                              expected_error_level,
                                              expected_error_count,
                                              expected_error_flag,
                                              expected_error_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9: Validate that there are no residual errors by running a verify
     *         for the lun capacity.
     */

    /* Step 9a: Set a debug hook so that we don't miss the verify start.
     */
    fbe_test_verify_get_verify_start_substate(FBE_LUN_VERIFY_TYPE_USER_RW, &monitor_substate);
    status = fbe_test_add_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9b: Run a R/W verify.  This will cause the errors to be detected.
     */
    fbe_test_sep_util_lun_initiate_verify(lun_object_id, FBE_LUN_VERIFY_TYPE_USER_RW,
                                          FBE_TRUE, /* Verify the entire lun */   
                                          FBE_LUN_VERIFY_START_LBA_LUN_START,     
                                          FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9c: Wait for the raid group to start the verify.
     */ 
    status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                 monitor_substate,
                                                 SCHEDULER_CHECK_STATES, 
                                                 SCHEDULER_DEBUG_ACTION_PAUSE, 
                                                 0, 0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9d: Wait for the lun verify to start.
     */
    status = fbe_test_sep_util_wait_for_verify_start(lun_object_id, 
                                                     FBE_LUN_VERIFY_TYPE_USER_RW,
                                                     &lun_verify_status);                
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9e: Remove the raid group debug hook.
     */
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, 
                                            SCHEDULER_CHECK_STATES, 
                                            SCHEDULER_DEBUG_ACTION_PAUSE);  
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 9f: Wait for the R/W Verify to complete.
     */
    status = fbe_test_verify_wait_for_verify_completion(rg_object_id, FBE_TEST_WAIT_TIMEOUT_MS, FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_lun_get_bgverify_status(lun_object_id, &lun_verify_status, FBE_LUN_VERIFY_TYPE_USER_RW);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS,
               "%s: raid type: %d err type: 0x%x lun obj: 0x%x R/W Verify chkpt: 0x%llx percent: %d", 
               __FUNCTION__, rg_config_p->raid_type, 
               error_case_p->record.err_type, lun_object_id,
               lun_verify_status.checkpoint, lun_verify_status.percentage_completed);

    /* Step 9g: Validate the expected error count.
     */
    status = betty_lou_validate_sector_counts(0,
                                              0,
                                              0,
                                              FBE_SECTOR_TRACE_ERROR_LEVEL_INFO,
                                              0,
                                              FBE_SECTOR_TRACE_ERROR_FLAG_NONE,
                                              0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/* Step 10: Clear the lun verify counters (report) and the sector trace
     *         counters.
     */
    status = fbe_test_sep_util_lun_clear_verify_report(lun_object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = fbe_api_sector_trace_reset_counters();
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 11: Clear the `verify after write' debug flag.
     */
    status = fbe_api_raid_group_set_group_debug_flags(rg_object_id, 
                                                      0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return FBE_STATUS_OK;
}
/**************************************************
 * end betty_lou_test_verify_after_write_validate()
 **************************************************/

/*!**************************************************************
 * betty_lou_test_rg_config()
 ****************************************************************
 * @brief
 *  Run betty lou test on a set of configs.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  3/24/2011 - Created. Rob Foley
 *
 ****************************************************************/
void betty_lou_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void *context_p)
{
    fbe_status_t                    status;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       raid_group_count = fbe_test_get_rg_count(rg_config_p);
    betty_lou_error_case_t         *error_case_p;
    betty_lou_error_case_t         *verify_after_write_case_p;
    fbe_u32_t                       rg_index;
    fbe_u32_t                       record_index;

    /* Set the trace level */
    fbe_test_sep_util_set_default_trace_level(FBE_TRACE_LEVEL_INFO);

    /* Perform the write verify test on all raid groups. */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* From the raid type determine the error record array.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: %d out of: %d raid type: %d\n", 
                   __FUNCTION__, rg_index+1, raid_group_count, current_rg_config_p->raid_type);
        if (current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            error_case_p = &betty_lou_error_raid0_record_array[0];
            verify_after_write_case_p = NULL;
        }
        else
        {
            error_case_p = &betty_lou_error_redundant_record_array[0];
            verify_after_write_case_p = &betty_lou_verify_after_write_record_array[0];
        }

        /* For each error record inject the error and validate the
         * result from the sector trace service.
         */
        for (record_index = 0; record_index < BETTY_LOU_ERROR_RECORD_COUNT; record_index++)
        {
            /* Invoke the method to inject the error
             */
            status = betty_lou_test_inject_error_validate(current_rg_config_p, error_case_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Goto next error record.
             */
            error_case_p++;
        }

        /* Verify after write is only applicable to redundant raid types.
         */
        if (current_rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0)
        {
            /* For each record.
             */
            for (record_index = 0; record_index < BETTY_LOU_VERIFY_AFTER_WRITE_RECORD_COUNT; record_index++)
            {
                /* Invoke the method to test verify after write.
                */
                status = betty_lou_test_verify_after_write_validate(current_rg_config_p, verify_after_write_case_p);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* Goto next error record.
                 */
                verify_after_write_case_p++;
            }
        }
        
        /* Goto next raid group
         */
        current_rg_config_p++;
    }
    return;
}
/******************************************
 * end betty_lou_test_rg_config()
 ******************************************/

/*!**************************************************************
 * betty_lou_test()
 ****************************************************************
 * @brief
 *  RunThis is the entry point into the Betty Lou test scenario.
 *
 * @param None.               
 *
 * @return None.
 *
 *
 ****************************************************************/
void betty_lou_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &betty_lou_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &betty_lou_raid_group_config_qual[0];
    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, betty_lou_test_rg_config,
                                   BETTY_LOU_LUNS_PER_RAID_GROUP,
                                   BETTY_LOU_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end betty_lou_test()
 ******************************************/

/*!**************************************************************
 * betty_lou_setup()
 ****************************************************************
 * @brief
 *  This is the setup function for the betty lou test scenario.
 *  It configures the objects and attaches them into the desired
 *  configuration.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  06/18/2010 - Created. Omer Miranda
 *
 ****************************************************************/
void betty_lou_setup(void)
{    
    fbe_status_t status;
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* This test only works for 1 LUN and must be 3 chunks.
     */
    FBE_ASSERT_AT_COMPILE_TIME(BETTY_LOU_LUNS_PER_RAID_GROUP == 1);
    FBE_ASSERT_AT_COMPILE_TIME(BETTY_LOU_CHUNKS_PER_LUN >= 3);

    /* This test does error injection, turn off debug flags */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_test_rg_configuration_t *rg_config_p = NULL;

        if (test_level > 0)
        {
            rg_config_p = &betty_lou_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &betty_lou_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        elmo_load_config(rg_config_p, BETTY_LOU_LUNS_PER_RAID_GROUP, BETTY_LOU_CHUNKS_PER_LUN);
    }

    status = fbe_api_sector_trace_set_trace_level(FBE_SECTOR_TRACE_ERROR_LEVEL_DATA,FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_api_sector_trace_display_info(FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    
    return;
}
/******************************************
 * end betty_lou_setup()
 ******************************************/

/*!**************************************************************
 * betty_lou_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the sector tracing test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06/18/2010 - Created. Omer Miranda
 *
 ****************************************************************/

void betty_lou_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* restore to default */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end betty_lou_cleanup()
 ******************************************/
