/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file    herry_monster_test.c
 *****************************************************************************
 *
 * @brief   These tests excercise `large' LU and physical drive binding with
 *          normal I/O.  The following test `large' LU and physical drives.
 *          These test generate I/Os that span the 2 TB block (both logical
 *          and physical) boundary.  That is the I/O will cross the
 *          0x00000000FFFFFFFF lba boundary.  To test the physical boundary
 *          it is simply the logical lba divided by the number of data disk
 *          with the bind offset subtracted out.
 *              o Binding of a LU greater than 2 TB
 *              o Binding a mirror raid group with 3 TB drives
 *              o The following I/O excercise both the logical and physical 
 *                2 TB boundary.
 *                  + Zero/Read/Compare - Both small and large I/O
 *                  + Write/Read/Compare - Both small and large I/O
 *                  + Corrupt CRC/Read/Compare - Both small and large I/O
 *                  + Corrupt Data/Read/Compare - Both small and large I/O
 *              o The following I/O excercise both the logical and physical
 *                2 TB boundary with degraded raid groups:
 *                  + Zero/Read/Compare - Both small and large I/O
 *                  + Write/Read/Compare - Both small and large I/O
 *                  + Corrupt CRC/Read/Compare - Both small and large I/O
 *
 * @version
 *   19/1/2011:  Created. Mahesh Agarkar
 *
 *****************************************************************************/

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
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_raid_error.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "sep_rebuild_utils.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * herry_monster_test_short_desc = "Perform IO on large size LUNs(~6TB) to test 64bit block count support";
char * herry_monster_test_long_desc = "\n\
    The Herry Monster scenario test binds R5, R6 and R10 type LUN of large capacity (~ 6TB)\n\
    using drive capacity of ~3TB to perform read, write and zeroing I/Os crossing 32bit boundary\n\
    for PVD object at disk level. To cross 32bit boundary, IO's disk PBA should be > 2TB.\n\
    We will be issuing reads, writes and zeroing IOs across and beyond 2TB boundaries of the drives as well as LUN's 2TB boundaries.\n\
    This includes:\n\
        - binding LUNs of size ~6TB with type R5, R6 and R10 \n\
        - running I/Os on these LUNs across and beyond 2TB(32bit boundary) limit\n\
          of PVD and at the end of LUN\n\n\
Dependencies:\n\
        - Drives of larger size(~8TB) to bind large size of lun.\n\n\
STEP 1:  Configure all raid groups and LUNs\n\
         - Raid Types R5, R6 and R10\n\
         - Bind large capacity(~6TB) of Raid groups and LUNs using ~8TB size of disk.\n\
         - All raid groups have 1 LUN each.\n\
STEP 2:  Perform I/O across 32bit boundary\n\
         - Perform zero/read/compare I/O across 32bit boundary.\n\
STEP 3:  Perform I/O beyond 32bit boundary\n\
         - Perform zero/read/compare I/O beyond 32bit boundary.\n\
STEP 4:  Perform I/O across 32bit boundary\n\
         - Perform zero I/O across 32bit boundary.\n\
STEP 5:  Perform I/O beyond 32bit boundary\n\
         - Perform zero I/O beyond 32bit boundary.\n\
STEP 6:  Perform I/O across 32bit boundary\n\
         - Perform read I/O across 32bit boundary.\n\
STEP 7:  Perform I/O beyond 32bit boundary\n\
         - Perform read I/O beyond 32bit boundary.\n\
STEP 8:  Perform I/O at the end of LUN\n\
         - Perform zero/read/compare I/O at the end of 6TB LUN.\n\
STEP 9:  Perform I/O at the end of LUN\n\
         - Perform zero IO at the end of 6TB LUN.\n\
STEP 10: Perform I/O at the end of LUN\n\
         - Perform write/read/compare IO at the end of 6TB LUN.\n\
STEP 11: Perform I/O to the degraded raid group\n\
         - Remove drives to bring raid group in degraded mode.\n\
         - Perform large size zero IO which cross 3TB boundary to verify that\n\
           degraded raid group handles large size zero IO correctly.\n\
         - [TODO] Perform random I/Os with read/write/compare and zero/compare.\n\
         - Reinsert drives to bring raid group into non degraded mode.\n\
STEP 12: Verify that there is no trace errors reported by sep, physical and neit package.\n\
STEP 13: Destroy raid groups and LUNs.\n\n\
\n\
Test Specific Switches:\n\
          - none.\n\
Outstanding Issues:[TODO]\n\
        - Need to perform read/write/zero random IOs while raid group is in degraded.\n\
        - Comments in the code needs to be align with size of raid group and LUN and should\n\
          be consistent across the test.\n\
        - Need to revisit IOs start LBA calculation and its usage while sending IO request using rdgen in this test.\n\
\n\
Description last updated: 9/30/2011\n";


/*!*******************************************************************
 * @def HERRY_MONSTER_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the qual test. 
 *
 *********************************************************************/
#define HERRY_MONSTER_TEST_LUNS_PER_RAID_GROUP                 1

/*!*******************************************************************
 * @def HERRY_MONSTER_TEST_CONFIGURATIONS_COUNT
 *********************************************************************
 * @brief test configuration count large zero test
 *
 *********************************************************************/
#define HERRY_MONSTER_TEST_CONFIGURATIONS_COUNT 1

/*!*******************************************************************
 * @def HERRY_MONSTER_USER_LUN_BIND_OFFSET
 *********************************************************************
 * @brief In order to test physical 2 TB boundary we must know the
 *        bind offset.
 *
 * @todo  Need to export fbe_private_space_layout_get_start_of_user_space()
 *
 *********************************************************************/
#define HERRY_MONSTER_USER_LUN_BIND_OFFSET  0x10000

/*!*******************************************************************
 * @def HERRY_MONSTER_SMALL_IO_BLOCKS
 *********************************************************************
 * @brief This is the small I/O number of blocks to transfer
 *
 *********************************************************************/
#define HERRY_MONSTER_SMALL_IO_BLOCKS       0x800

/*!*******************************************************************
 * @def HERRY_MONSTER_LARGE_IO_BLOCKS
 *********************************************************************
 * @brief This is the large I/O number of blocks to transfer
 *
 *********************************************************************/
#define HERRY_MONSTER_LARGE_IO_BLOCKS       0x40000

/*!***************************************************************************
 * @def     HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB
 *****************************************************************************
 * @brief   This is the number of 512-bps blocks in a 2 TB LUN.
 *          o 2 tera-bytes                                  = 0x200'00000000
 *            divided by 512-bytes per data block (0x200)   / 0x000'00000200
 *                                                          ================
 *                                                            0x001'00000000
 *
 *****************************************************************************/
#define HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB  0x100000000ULL


/*!***************************************************************************
 * @def HERRY_MONSTER_LUN_CAPACITY
 *****************************************************************************
 * @brief Number of blocks per lun, we want to bind lun with a physical
 *        capacity consumed of at least 2 TB per data position.  Currently we
 *        assume the maximum number of data positions is (2).  Therefore the
 *        minumum LUN capacity is:
 *          o (2 * 0x0000000100000000) + HERRY_MONSTER_LARGE_IO_BLOCKS => ~4 TB
 *        However,  test doubled drive capacity to ~8TB.  Therefore doubling
 *        LUN capacity as well.  
 *
 *****************************************************************************/
#define HERRY_MONSTER_LUN_CAPACITY          (2 * ((2 * HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB) + HERRY_MONSTER_LARGE_IO_BLOCKS))

/*!***************************************************************************
 * @def HERRY_MONSTER_CHUNKS_PER_LUN
 *****************************************************************************
 * @brief Number of chunks per lun, we want to bind lun with a physical
 *        capacity consumed of at least 2 TB per data position.  Currently we
 *        assume the maximum number of data positions is (2).  Therefore the
 *        number of chunks is:
 *          o (HERRY_MONSTER_LUN_CAPACITY / FBE_RAID_DEFAULT_CHUNK_SIZE)
 *
 *****************************************************************************/
#define HERRY_MONSTER_CHUNKS_PER_LUN        (HERRY_MONSTER_LUN_CAPACITY / FBE_RAID_DEFAULT_CHUNK_SIZE) /*!< 0x400080 */

/*!*******************************************************************
 * @var herry_monster_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t herry_monster_test_contexts[HERRY_MONSTER_TEST_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @var herry_monster_pvd_debug_flags
 *********************************************************************
 * @brief This is used to enanle pvd debug flags in case we need extra traces from 
 * pvd for debugging
 *********************************************************************/
fbe_provision_drive_debug_flags_t herry_monster_pvd_debug_flags = (FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING | 
                                                                   FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING |
                                                                   FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING   );

/*!*******************************************************************
 * @var herry_monster_enable_pvd_tracing
 *********************************************************************
 * @brief flag for enabling the PVD debug tracing
 *********************************************************************/
fbe_bool_t herry_monster_enable_pvd_tracing = FBE_TRUE;

/*!*******************************************************************
 * @def HERRY_MONSTER_LUN_READY_STATE_TIMEOUT
 *********************************************************************
 * @brief waiting time for drive removal
 *********************************************************************/
#define HERRY_MONSTER_LUN_READY_STATE_TIMEOUT 20000

/*!*******************************************************************
 * @var     herry_monster_b_is_de62_corrupt_operations_fail_fixed
 *********************************************************************
 * @brief   Currently some instances of Corrupt CRC and Corrupt Data
 *          fail.  When they are fixed remove this variable and it's
 *          uses.
 *
 * @todo    Remove this when Corrupt CRC and Corrupt Data work for all
 *          raid types and lbas.
 *
 *********************************************************************/
static fbe_bool_t herry_monster_b_is_de62_corrupt_operations_fail_fixed = FBE_FALSE;

/*!*******************************************************************
 * @var     herry_monster_b_is_de62_degraded_verify_hangs_fixed
 *********************************************************************
 * @brief   Currently some instances of degraded verify fail.
 *          fail.  When they are fixed remove this variable and it's
 *          uses.
 *
 * @todo    Remove this when verify beyond the 2 TB range is fixed.
 *
 *********************************************************************/
static fbe_bool_t herry_monster_b_is_de62_degraded_verify_hangs_fixed = FBE_FALSE;

/*!***************************************************************************
 * @var     herry_monster_raid_group_config
 *****************************************************************************
 * @brief   This is the array of configurations we will loop over for the
 *          `extended' version of the herry monster test.  This version includes
 *          testing of the 2 TB `physical boundry also (depending on raid type).
 *          We configure RG capacities to create ~8 TB drives. 
 *          To test the physical 2 TB bloundary, the LUN must be greater than:
 *              o (2 TB - bind offset) * (number of data disks)
 *          The following raid groups are created:
 *              o RAID-5 - 2 + 1: 
 *****************************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t herry_monster_raid_group_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t herry_monster_raid_group_config[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
        {
            /*width,    capacity        raid type,                  class,                  block size  RAID-id.        bandwidth.*/
            {1,         0x420000000,    FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,           0,                 0},
            {2,         0x420000000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           1,                 0},
            {3,         0x840000000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,           2,                 0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/*!***************************************************************************
 * @var     herry_monster_raid_group_extended_config
 *****************************************************************************
 * @brief   This is the array of configurations we will loop over for the
 *          `extended' version of the herry monster test.  This version includes
 *          testing of the 2 TB `physical boundry also (depending on raid type).
 *          We configure RG capacities to create ~8 TB drives 
 *          To test the physical 2 TB bloundary, the LUN must be greater than:
 *              o (2 TB - bind offset) * (number of data disks)
 *          The following raid groups are created:
 *              o RAID-5 - 2 + 1: 
 *****************************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t herry_monster_raid_group_extended_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t herry_monster_raid_group_extended_config[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
        {
            /*width,    capacity        raid type,                  class,                  block size  RAID-id.        bandwidth.*/
            {1,         0x420000000,    FBE_RAID_GROUP_TYPE_RAID0,  FBE_CLASS_ID_STRIPER,   520,           0,                 0},
            {2,         0x420000000,    FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    520,           1,                 0},
            {3,         0x840000000,    FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,           3,                 0},
            {4,         0x840000000,    FBE_RAID_GROUP_TYPE_RAID6,  FBE_CLASS_ID_PARITY,    520,           4,                 0},
            {4,         0x420000000,    FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER,   520,           5,                 0},
            {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
        },
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*************************
 *   FUNCTIONS
 *************************/

void herry_monster_test_rebuild_large_rgs(fbe_test_rg_configuration_t * const rg_config_p);

/*!****************************************************************************
 * herry_monster_setup()
 ******************************************************************************
 * @brief
 *  Setup for herry monster test, binding large size LUN exceeding 32bit boundary
 *
 * @param None.
 *
 * @return None.   
 *
 * @author
 *  4/10/2010 - Created.Mahesh Agarkar
 ******************************************************************************/
void herry_monster_setup(void)
{

    fbe_u32_t extended_testing_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_array_t *array_p = NULL;
    fbe_u32_t rg_index, raid_type_count;

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {

        if (extended_testing_level > 0)
        {
            /* Extended.
             */
            array_p = &herry_monster_raid_group_extended_config[0];

        }
        else
        {
            /* Qual.
             */
            array_p = &herry_monster_raid_group_config[0];
        }
        raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);

        for(rg_index = 0; rg_index < raid_type_count; rg_index++)
        {
            fbe_test_sep_util_init_rg_configuration_array(array_p[rg_index]);

            /* TODO: Add this to test 4K and random block sizes.*/
#if 0            
            fbe_test_rg_setup_random_block_sizes(array_p[rg_index]);
            fbe_test_sep_util_randomize_rg_configuration_array(array_p[rg_index]);
#endif
        }

        fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
        sep_config_load_sep_and_neit();

    }
    return;
}
/******************************************************************************
 * end herry_monster_setup()
 ******************************************************************************/

/*!****************************************************************************
 * herry_monster_enable_debug_flags()
 ******************************************************************************
 * @brief
 *  setting debug flags
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  4/10/2010 - Created.Mahesh Agarkar
 ******************************************************************************/
void herry_monster_enable_debug_flags( fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t index;
    fbe_object_id_t pvd_object_id;
    fbe_status_t status;

    if (herry_monster_enable_pvd_tracing == FBE_TRUE)
    {
        for (index = 0; index < rg_config_p->width; index++)
        {
            status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[index].bus,
                                                        rg_config_p->rg_disk_set[index].enclosure,
                                                        rg_config_p->rg_disk_set[index].slot,
                                                        &pvd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Set the PVD debug flags to the default
             */
            fbe_test_sep_util_provision_drive_set_debug_flag(pvd_object_id, herry_monster_pvd_debug_flags);
        }
    }
}

/*!***************************************************************************
 *          herry_monster_large_zero_test_one_io()
 *****************************************************************************
 * 
 *  @brief  This method sends (1) very large Zero only request to each large
 *          lun.  It purposefully only sends the Zero request to (1) LUN at
 *          a time since more than (1) might overwhelm the system.
 *
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   start_lba - Start lun lba
 * @param   test_blocks - Number of blocks for request
 *
 * @return None.
 *
 * @author
 *  10/27/2011  Ron Proulx  - Created.
 *
 *****************************************************************************/
static void herry_monster_large_zero_test_one_io(fbe_test_rg_configuration_t * const rg_config_p,
                                                 fbe_lba_t start_lba,
                                                 fbe_block_count_t test_blocks)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_api_rdgen_context_t *context_p = &herry_monster_test_contexts[0];
    fbe_block_count_t   verify_blocks = 0;

    /* Step 1: Send the `logical' Zero request  
     */
    status = fbe_api_rdgen_send_one_io(context_p, FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN, /* Issue I/O to every LUN. */
                                       FBE_PACKAGE_ID_SEP_0,
                                       FBE_RDGEN_OPERATION_ZERO_ONLY,
                                       FBE_RDGEN_PATTERN_ZEROS,
                                       start_lba, /* lba */
                                       test_blocks /* blocks */,
                                       FBE_RDGEN_OPTIONS_STRIPE_ALIGN,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    fbe_test_sep_io_validate_status(status, context_p, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 2: Now initiate a verify for fixed `smaller' range
     */
    verify_blocks = HERRY_MONSTER_LARGE_IO_BLOCKS;
    status = fbe_test_sep_io_run_verify_on_all_luns(rg_config_p,
                                                    FBE_FALSE, /* Do not verify the entire lun */
                                                    start_lba,
                                                    verify_blocks);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/*******************************************
 * end herry_monster_large_zero_test_one_io()
 ******************************************/

/*!***************************************************************************
 *          herry_monster_test_one_io()
 *****************************************************************************
 * 
 *  @brief  This method generates (2) I/Os for each LUN bound:
 *              o Generate and send a `logical' I/O to all bound raid groups
 *              o For each raid group determine the `physical' range requested
 *                then for each lun in the raid groups send a `logical' I/O
 *                that will span the `physical' boundary.
 *
 * @param   rg_config_p - Pointer to array of raid groups under test
 * @param   start_lba - Start lun lba
 * @param   test_blocks - Number of blocks for request
 * @param   rdgen_op - The rdgen operation to send
 *
 * @return None.
 *
 * @author
 *  10/21/2011  Ron Proulx  - Created.
 *
 *****************************************************************************/
void herry_monster_test_one_io(fbe_test_rg_configuration_t * const rg_config_p,
                               fbe_lba_t start_lba,
                               fbe_block_count_t test_blocks,
                               fbe_rdgen_operation_t rdgen_op)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_api_rdgen_context_t *context_p = &herry_monster_test_contexts[0];
    fbe_rdgen_pattern_t rdgen_pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    fbe_u32_t       raid_group_count = 0;
    fbe_u32_t       raid_group_index;
    fbe_object_id_t lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t       physical_lba = start_lba;
    fbe_lba_t       logical_for_physical_lba = FBE_LBA_INVALID;

    /* Determine the rdgen pattern based on opcode.
     */
    switch(rdgen_op)
    {
        case FBE_RDGEN_OPERATION_ZERO_ONLY:
        case FBE_RDGEN_OPERATION_ZERO_READ_CHECK:
            rdgen_pattern = FBE_RDGEN_PATTERN_ZEROS;
            break;

        default:
            /* Otherwise we send the default pattern
             */
            break;
    }

    /* Step 1: Send the `logical' request  
     */
    status = fbe_api_rdgen_send_one_io(context_p, FBE_OBJECT_ID_INVALID,
                                       FBE_CLASS_ID_LUN, /* Issue I/O to every LUN. */
                                       FBE_PACKAGE_ID_SEP_0,
                                       rdgen_op,
                                       rdgen_pattern,
                                       start_lba, /* lba */
                                       test_blocks /* blocks */,
                                       FBE_RDGEN_OPTIONS_STRIPE_ALIGN,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    fbe_test_sep_io_validate_status(status, context_p, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Step 2: Now initiate a verify for the specified range of the lun in each 
     *         raid group.  If the operation was `Corrupt CRC' skip the verify.
     */
    if (rdgen_op != FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK)
    {
        /* If not corrupt crc run verify.
         */
        status = fbe_test_sep_io_run_verify_on_all_luns(rg_config_p,
                                                        FBE_FALSE, /* Do not verify the entire lun */
                                                        start_lba,
                                                        test_blocks);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Send the request that spands the `physical' boundary.
     */
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    for (raid_group_index = 0; raid_group_index < raid_group_count; raid_group_index++)
    {
        fbe_lun_number_t lun_number = current_rg_config_p->logical_unit_configuration_list[0].lun_number;
        fbe_lba_t        lun_capacity = current_rg_config_p->logical_unit_configuration_list[0].capacity;
        fbe_u32_t        data_drives = 0;

        /* Get the (1) lun id associated with this raid group.
         */
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Get the number of data drives associated with this raid group.
         */
        data_drives = fbe_test_sep_get_data_drives_for_raid_group_config(current_rg_config_p);

        /* Calculated the adjusted start lba
         */
        logical_for_physical_lba = (physical_lba * data_drives);
        if (logical_for_physical_lba < (HERRY_MONSTER_USER_LUN_BIND_OFFSET * data_drives))
        {
            logical_for_physical_lba = (HERRY_MONSTER_USER_LUN_BIND_OFFSET * data_drives);
        }
        logical_for_physical_lba = logical_for_physical_lba - (HERRY_MONSTER_USER_LUN_BIND_OFFSET * data_drives);

        /* Must span boundary.
         */
        if (data_drives > 1)
        {
            logical_for_physical_lba += (test_blocks - (test_blocks / data_drives));
        }

        /* Cannot go beyond capacity.
         */
        if ((logical_for_physical_lba + test_blocks) > lun_capacity)
        {
            logical_for_physical_lba = lun_capacity - test_blocks;
        }

        /* Step 3: Send the I/O to the lun object.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "** herry monster: physical_lba: 0x%llx logical_for_physical_lba: 0x%llx data_drives: %d **",
                   (unsigned long long)physical_lba,
		   (unsigned long long)logical_for_physical_lba,
		   data_drives);
        status = fbe_api_rdgen_send_one_io(context_p, 
                                           lun_object_id, /* Issue I/O to specific LUN */
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           rdgen_op,
                                           rdgen_pattern,
                                           logical_for_physical_lba, /* lba */
                                           test_blocks /* blocks */,
                                           FBE_RDGEN_OPTIONS_STRIPE_ALIGN,
                                           0, 0, /* no expiration or abort time */
                                           FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        fbe_test_sep_io_validate_status(status, context_p, FBE_FALSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Step 4: Send a verify request to the logical range tested.
         *         If the operation was `Corrupt CRC' skip the verify.
         */
        if (rdgen_op != FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK)
        {
            /* If not corrupt crc run verify.
            */
            status = fbe_test_sep_io_run_verify_on_lun(lun_object_id,
                                                       FBE_FALSE, /* Do not verify the entire lun */
                                                       logical_for_physical_lba,
                                                       test_blocks);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto the next raid group
         */
        current_rg_config_p++;

    } /* for each raid group */

    return;
}
/******************************************
 * end herry_monster_test_one_io()
 ******************************************/

/*!**************************************************************
 * herry_monster_run_io_test_for_all_operations()
 ****************************************************************
 * @brief
 *  This will run single I/O test for given lba range,
 *  basically this is to run I/Os across the 32bit boundaries
 *
 * @param   rg_config_p - pointer to raid group array
 * @param   start_lba - Starting LUN lba to send I/O to
 * @param   test_blocks - Number of blocks for I/O
 * @param   b_is_degraded - FBE_TRUE - Degraded raid group.
 *
 * @return None.
 *
 * @author
 *  10/24/2011  Ron Proulx  - Created
 *
 ****************************************************************/
void herry_monster_run_io_test_for_all_operations(fbe_test_rg_configuration_t *const rg_config_p,
                                                  const fbe_lba_t start_lba,
                                                  const fbe_block_count_t test_blocks,
                                                  const fbe_bool_t b_is_degraded)   
{
    fbe_char_t  *degraded_string = NULL;

    /* Generate degraded string.
     */
    if (b_is_degraded == FBE_TRUE)
    {
        degraded_string = "degraded";
    }
    else
    {
        degraded_string = "non-degraded";
    }

    /* Operation 1:  Test Zero/Read/Compare
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "*** herry monster: Zero/Read/Compare lba: 0x%llx blks: 0x%llx (%s)***",
               (unsigned long long)start_lba, (unsigned long long)test_blocks,
	       degraded_string);
    herry_monster_test_one_io(rg_config_p, start_lba, test_blocks,
                              FBE_RDGEN_OPERATION_ZERO_READ_CHECK);

    /* Operation 2:  Test Write/Read/Compare
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "**** herry monster: Write/Read/Compare lba: 0x%llx blks: 0x%llx (%s)***",
               (unsigned long long)start_lba, (unsigned long long)test_blocks,
	       degraded_string);
    herry_monster_test_one_io(rg_config_p, start_lba, test_blocks,
                              FBE_RDGEN_OPERATION_WRITE_READ_CHECK);

    /* Operation 3:  Test Corrupt CRC/Read/Compare
     */
    /*! @todo Currently this isn't working on RAID-10 raid groups.
     */
    if (herry_monster_b_is_de62_corrupt_operations_fail_fixed == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS,
                   "*** herry monster: Corrupt CRC/Read/Compare lba: 0x%llx blks: 0x%llx (%s)***",
                   (unsigned long long)start_lba,
		   (unsigned long long)test_blocks, degraded_string);
        herry_monster_test_one_io(rg_config_p, start_lba, test_blocks,
                                  FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK);
    }

    /* Operation 4:  Test Corrupt Data/Read/Compare
     *               Can only run this test for non-degraded raid groups.
     */
    if (b_is_degraded == FBE_FALSE)
    {
        /*! @todo Currently Corrupt Data does not work for request at or beyond
         *        the 2 TB boundary.
         */
        if (((start_lba + test_blocks -1) < HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB)                 ||
            (herry_monster_b_is_de62_corrupt_operations_fail_fixed == FBE_TRUE)    )
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                   "*** herry monster: Corrupt Data/Read/Compare lba: 0x%llx blks: 0x%llx (%s)***",
                       (unsigned long long)start_lba,
		       (unsigned long long)test_blocks, degraded_string);
            herry_monster_test_one_io(rg_config_p, start_lba, test_blocks,
                                      FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK);
        }
    }

}

/*!**************************************************************
 * herry_monster_run_io_test()
 ****************************************************************
 * @brief
 *  This will run single I/O test for given lba range,
 *  basically this is to run I/Os across the 32bit boundaries
 *
 * @param   rg_config_p - pointer to raid group array
 * @param   test_blocks - Number of blocks for I/O
 * @param   b_is_degraded - FBE_TRUE - Degraded raid group.
 *
 * @return None.
 *
 * @todo    Need to run random I/Os here, read-write-check, Zero-check
 *
 * @author
 *  3/29/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/
void herry_monster_run_io_test(fbe_test_rg_configuration_t *const rg_config_p,
                               const fbe_block_count_t test_blocks,
                               const fbe_bool_t b_is_degraded)   
{
    fbe_lba_t   start_lba = FBE_LBA_INVALID;

    /* Test 1:  Test I/O across `logical' 2 TB boundary.
     */
    start_lba = 0xFFFFFC00;         /*!< HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB - 0x400*/
    mut_printf(MUT_LOG_TEST_STATUS, 
               "*** %s: lba: 0x%llx blks: 0x%llx 32-bit boundary ***",
               __FUNCTION__, (unsigned long long)start_lba,
	       (unsigned long long)test_blocks);
    herry_monster_run_io_test_for_all_operations(rg_config_p,
                                                 start_lba,
                                                 test_blocks,
                                                 b_is_degraded);

    /* Test 2: Test I/O beyond 32-bit boundary
     */
    start_lba = 0x100000000;        /*!< HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "*** %s: lba: 0x%llx blks: 0x%llx beyond 32-bit boundary ***",
               __FUNCTION__, (unsigned long long)start_lba,
	       (unsigned long long)test_blocks);
    herry_monster_run_io_test_for_all_operations(rg_config_p,
                                                 start_lba,
                                                 test_blocks,
                                                 b_is_degraded);

    /* Test 3: Test I/O at end of LUN (4 TB boundary)
     */
    start_lba = 0x1FFFFFC00;        /*!< (2 * HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB) - 0x400 */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "*** %s: lba: 0x%llx blks: 0x%llx at 4 TB boundary ***",
               __FUNCTION__, start_lba, test_blocks);
    herry_monster_run_io_test_for_all_operations(rg_config_p,
                                                 start_lba,
                                                 test_blocks,
                                                 b_is_degraded);

    /*! @todo Need to run random I/Os here, read-write-check, Zero-check
     */

    return;
}
/************************************
 * end of herry_monster_run_io_test()
 ************************************/

/*!**************************************************************
 * herry_monster_run_non_degraded_io_test()
 ****************************************************************
 * @brief
 *  This will run single I/O test for given lba range,
 *  basically this is to run I/Os across the 32bit boundaries
 *
 * @param rg_config_p
 *
 * @return None.
 *
 * @author
 *  3/29/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/
void herry_monster_run_non_degraded_io_test(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_lba_t           start_lba = 0;
    fbe_block_count_t   test_blocks = 0;

    /* First run small block count tests.
     */
    test_blocks = HERRY_MONSTER_SMALL_IO_BLOCKS;
    mut_printf(MUT_LOG_TEST_STATUS, 
               "*** %s: Run small request size: 0x%llx (%llu) ***",
               __FUNCTION__, (unsigned long long)test_blocks,
	      (unsigned long long)test_blocks);
    herry_monster_run_io_test(rg_config_p,
                              test_blocks,
                              FBE_FALSE /* Non-degraded */); 

    /* Now run large zero only test
     */
    start_lba = 0x0;
    test_blocks = HERRY_MONSTER_NUMBER_OF_BLOCKS_IN_2TB;  
    mut_printf(MUT_LOG_TEST_STATUS, 
               "*** %s: Run large zero request size: 0x%llx (%llu) ***",
               __FUNCTION__, (unsigned long long)test_blocks,
	       (unsigned long long)test_blocks);
    herry_monster_large_zero_test_one_io(rg_config_p,
                                         start_lba,
                                         test_blocks);

    return;
}
/*************************************************
 * end of herry_monster_run_non_degraded_io_test()
 *************************************************/

/*!**************************************************************
 * herry_monster_run_degraded_io_test()
 ****************************************************************
 * @brief
 *  This will run single I/O test for given lba range,
 *  basically this is to run I/Os across the 32bit boundaries
 *
 * @param rg_config_p
 *
 * @return None.
 *
 * @author
 *  3/29/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/
void herry_monster_run_degraded_io_test(fbe_test_rg_configuration_t * const rg_config_p)
{
    fbe_status_t        status;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_block_count_t   test_blocks = 0;
    fbe_u32_t           rg_count, rg_index;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* pull out one drive from each RAID group
     */
    big_bird_remove_all_drives(current_rg_config_p, rg_count, 1,
                               FBE_TRUE, /* We are pulling and reinserting same drive */
                               0, /* msec wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);

    /* First run small block count tests.
     */
    test_blocks = HERRY_MONSTER_SMALL_IO_BLOCKS;
    mut_printf(MUT_LOG_TEST_STATUS, 
               "*** %s: Run small request size: 0x%llx (%llu) ***",
               __FUNCTION__, (unsigned long long)test_blocks,
	       (unsigned long long)test_blocks);
    herry_monster_run_io_test(rg_config_p,
                              test_blocks,
                              FBE_TRUE /* degraded */); 

    /* Re-insert the drives
     */
    big_bird_insert_all_drives(rg_config_p, rg_count, 0,
                               FBE_TRUE    /* We are pulling and reinserting same drive */);

    for (rg_index = 0; rg_index < rg_count; rg_index++)
    {
        fbe_object_id_t lun_object_id;
        fbe_u32_t lu_index;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        for (lu_index = 0; lu_index < current_rg_config_p->number_of_luns_per_rg; lu_index++)
        {
            /* find the object id of the lun - unless the lun id was defaulted .... we have n oway of knowing the lun id
             or the object id in that case */
            if ( rg_config_p->logical_unit_configuration_list[lu_index].lun_number != FBE_LUN_ID_INVALID )
            {
                status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lu_index].lun_number, 
                                                               &lun_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
                
                /* verify the lun get to ready state in reasonable time. */
                status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY, 
                                                                 HERRY_MONSTER_LUN_READY_STATE_TIMEOUT, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                mut_printf(MUT_LOG_TEST_STATUS, "%s LUN Object ID 0x%x in READY STATE, RAID TYPE: 0x%x",
                                  __FUNCTION__, lun_object_id,  current_rg_config_p->logical_unit_configuration_list[lu_index].raid_type);
            }
        }
        current_rg_config_p++;
    }

    return;
}
/******************************************
 * end herry_monster_run_degraded_io_test()
 ******************************************/

/*!**************************************************************************
  * herry_monster_rebuild_wait_for_hook
  ****************************************************************************
  * @brief
  *
  * @param rg_config_p
  * 
  * @return None
  *
  * @author
  * 02/02/2012 Created Ashwin Tamilarasan
  *
  ****************************************************************************/
void herry_monster_rebuild_wait_for_hook(fbe_object_id_t rg_object_id, fbe_lba_t  target_checkpoint)
{
    fbe_u32_t            count;
    fbe_scheduler_debug_hook_t  hook;
    fbe_status_t           status;

    for (count = 0; count < 240; count++)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

        hook.object_id = rg_object_id;
        hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
        hook.check_type = SCHEDULER_CHECK_VALS_LT;
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD;
        hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
        hook.val1 = target_checkpoint;
        hook.val2 = NULL;
        hook.counter = 0;

        status = fbe_api_scheduler_get_debug_hook(&hook);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
		   (unsigned long long)hook.counter);
        if(hook.counter)
        {
            return;
        }
        //  Wait before trying again
        fbe_api_sleep(SEP_REBUILD_UTILS_RETRY_TIME);
    }
    
        MUT_FAIL_MSG("The hook did not hit within the time limit!\n");
    
    return;
}

/*!**************************************************************
 * herry_monster_test_rebuild_large_rgs()
 ****************************************************************
 * @brief
 *    This function tests the rebuild of large raid groups

     1) Remove a drive
     2) Do a metadata io so that all the metadata chunks are marked for rebuild
     3) Reinsert the drive
     2) Wait for the rebuild to hit the 2 nd chunk of metadata rebuild
     4) Write a pattern to the 1st chunk metadata lba, this io should go as non degraded io
     5) Let the rebuild complete for the metadata portion
     6) Read back the pattern and make sure it is the same
    
 *  When verify of large rgs is ready, issue a verify to the lun 
 *
 * @param rg_config_p
 *
 * @return None.
 *
 * @author
 *  02/01/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
void herry_monster_test_rebuild_large_rgs(fbe_test_rg_configuration_t * const rg_config_p)
{
    
    fbe_test_rg_configuration_t                       *current_rg_config_p = rg_config_p;
    fbe_u32_t                                          rg_count;
    fbe_api_base_config_metadata_paged_change_bits_t   paged_change_bits;
    fbe_object_id_t                                    rg_object_id;
    fbe_api_raid_group_get_info_t                      get_info;
    fbe_lba_t                                          rg_disk_capacity;
    fbe_lba_t                                          target_checkpoint;
    fbe_u32_t                                          position_removed;
    fbe_u32_t                                          index;
    fbe_status_t                                       status;

    rg_count = fbe_test_get_rg_array_length(rg_config_p);


    for(index = 0; index < rg_count; index ++)
    {
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);

        if(current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
        {
            current_rg_config_p++;
            continue;
        }

        if(current_rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            fbe_test_set_object_id_for_r10(&rg_object_id);
        }

        /* pull out one drive from each RAID group
         */
        big_bird_remove_all_drives(current_rg_config_p, rg_count, 1,
                                   FBE_TRUE, /* We are pulling and reinserting same drive */
                                   0, /* msec wait between removals */
                                   FBE_DRIVE_REMOVAL_MODE_SEQUENTIAL);
        position_removed = current_rg_config_p->drives_removed_array[0];

        fbe_api_raid_group_get_info(rg_object_id, &get_info, FBE_PACKET_FLAG_NO_ATTRIB);

        /* Wait for the rebuild logging to be set */
        sep_rebuild_utils_verify_rb_logging(current_rg_config_p, position_removed);

        /* Do a single write metadata IO so that all the metadata chunks are marked for rebuild */
        paged_change_bits.metadata_offset = 0;
        paged_change_bits.metadata_record_data_size = 4;
        paged_change_bits.metadata_repeat_count = 1;
        paged_change_bits.metadata_repeat_offset = 0;
    
        * (fbe_u32_t *)paged_change_bits.metadata_record_data = 0x101;
        status = fbe_api_base_config_metadata_paged_set_bits(rg_object_id, &paged_change_bits);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Make sure that metadata of metadata bits is set for NR */
    
        rg_disk_capacity = get_info.capacity / get_info.num_data_disk;
    
        mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

        target_checkpoint = rg_disk_capacity + 0x800;
        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD,
                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                target_checkpoint,
                                NULL,
                                SCHEDULER_CHECK_VALS_LT,
                                SCHEDULER_DEBUG_ACTION_PAUSE);
    
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /* Re-insert the drives */
        big_bird_insert_all_drives(current_rg_config_p, rg_count, 0,
                                   FBE_TRUE    /* We are pulling and reinserting same drive */);

        sep_rebuild_utils_verify_not_rb_logging(current_rg_config_p, position_removed); 

        /* wait for the hook to be hit*/
        herry_monster_rebuild_wait_for_hook(rg_object_id, target_checkpoint);

        /* Write something to the first chunk in the metadata region  It should
           be a non degraded IO since the first metadata chunk has been rebuilt 
        */
        paged_change_bits.metadata_offset = 0;
        paged_change_bits.metadata_record_data_size = 4;
        paged_change_bits.metadata_repeat_count = 1;
        paged_change_bits.metadata_repeat_offset = 0;
    
        * (fbe_u32_t *)paged_change_bits.metadata_record_data = 0x03;
        status = fbe_api_base_config_metadata_paged_set_bits(rg_object_id, &paged_change_bits);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Delete the hook */
        mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD,
                                FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                target_checkpoint,
                                NULL,
                                SCHEDULER_CHECK_VALS_LT,
                                SCHEDULER_DEBUG_ACTION_PAUSE);

        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Wait for the entire metadata rebuild to be done 
           Start of user region signifies the end of metadata rebuild
        */
        sep_rebuild_utils_wait_for_rb_to_start(current_rg_config_p, position_removed);

        mut_printf(MUT_LOG_TEST_STATUS, "Issuing a verify to the first metadata chunk...");

        /* Issue a verify to that chunk alone and make sure we do not get any errors */
        /*status = fbe_api_rdgen_send_one_io(context_p,
                                           rg_object_id,
                                           FBE_CLASS_ID_INVALID,
                                           FBE_PACKAGE_ID_SEP_0,
                                           FBE_RDGEN_OPERATION_VERIFY,
                                           FBE_RDGEN_PATTERN_LBA_PASS,
                                           rg_disk_capacity,
                                           0x800,
                                           FBE_RDGEN_OPTIONS_INVALID,
                                           0, 0 ); //no expiration or abort time );
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        mut_printf(MUT_LOG_TEST_STATUS, "== %s Validate correctable coherency errors ==\n", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(context_p->status, FBE_STATUS_OK);

        // Check the packet status, block status and block qualifier
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.verify_errors.c_coh_count, 0);*/

        current_rg_config_p++;

    }

    return;
}
/******************************************
 * end herry_monster_test_rebuild_large_rgs()
 ******************************************/

/*!**************************************************************************
  * herry_monster_run_tests
  ****************************************************************************
  * @brief: bind lun and run I/O for herry monster test
  *
  * @param rg_config_p
  * 
  * @return None
  *
  * @author
  * 4/14/2011 Created Mahesh Agarkar
  *
  ****************************************************************************/
void herry_monster_run_tests(fbe_test_rg_configuration_t * const rg_config_p, void * config_p)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Keeping it disbaled as this will be needed to get more traces from PVD
     * in case there is any issue
     */
    //herry_monster_enable_debug_flags(rg_config_p); 
    

    /* setting the PVD debug flags for timebeing */
    /*fbe_test_sep_util_provision_drive_set_class_debug_flag(HERRY_MONSTER_DEBUG_FLAGS_PVD);
     */
    /* Run single I/Os across the boundries first 
      */
    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Starting Targeted I/Os *****", __FUNCTION__);

    /* Always run non-degraded.
     */
    herry_monster_run_non_degraded_io_test(rg_config_p);

    /* Only run degraded if the test level is greater than 0.
     */
    if (test_level > 0)
    {
        /*! @todo Need to find out why degraded verify hangs to large lbas.
         */
        if (herry_monster_b_is_de62_degraded_verify_hangs_fixed == FBE_TRUE)
        {
            /* Run the large lab range on degraded raid groups.
             */
            herry_monster_run_degraded_io_test(rg_config_p);
        }
        else
        {
            /*! @todo Currently background verify to large lbas on a degraded
             *        raid group hangs.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "**** herry_monster_run_degraded_io_test Currently disabled due to DE62 *****");
        }
    }

    herry_monster_test_rebuild_large_rgs(rg_config_p);

    /* Make sure we did not get any trace errors.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();

    mut_printf(MUT_LOG_TEST_STATUS, "**** %s Completed successfully ****", __FUNCTION__);

    return;
}

/*!****************************************************************************
 * herry_monster_test()
 ******************************************************************************
 * @brief
 *  Run herry monster test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  4/10/2010 - Created. Mahesh Agarkar
 ******************************************************************************/
void herry_monster_test(void)
{
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    if (test_level > 0)
    {
        rg_config_p = &herry_monster_raid_group_extended_config[0][0];
    }
    else
    {
        rg_config_p = &herry_monster_raid_group_config[0][0];
    }

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, herry_monster_run_tests,
                                   HERRY_MONSTER_TEST_LUNS_PER_RAID_GROUP,
                                   HERRY_MONSTER_CHUNKS_PER_LUN);  

    return;
}
/******************************************************************************
 * end herry_monster_test()
 ******************************************************************************/


/*!**************************************************************
 * herry_monster_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the herry_monster_test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  15/4/2011 - Created. Rob Foley
 *
 ****************************************************************/
void herry_monster_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}


