/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file doremi_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of lun zero operation.
 *
 * @version
 *  6/27/2012 - Created. Prahlad Purohit
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "sep_rebuild_utils.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * doremi_short_desc = "Lun zeroing checkpoint test.";
char * doremi_long_desc = "\
The doremi scenario tests the LUN zero checkpoint operations. \n\
                                 \n\
   1. Validate Lun Zero percentage with Normal RG. \n\
     - Create a random RG configuration so that every time a different RG type \n\
       is created across System and Non-system drives. \n\
     - Randomly select verify hook type and checkpoints. \n\
     - Set PVD zero checkpoints. \n\
     - Wait for PVD hooks to hit. \n\
     - Check LU zeroing status and validate that it matches expected value. \n\
                                                                            \n\
   2. Validate Lun Zero percentage with degraded RG. \n\
     - Remove a drive from each RG to make them degraded. \n\
     - Validate the zero percentage and verify that it matches expected value.\n\
                                                                            \n\
   3. Validate Lun Zero percentage with broken RG. \n\
     - Remove a second drive from each RG to make them broken. \n\
     - Validate the zero percentage and verify that it matches expected value. \n\
                                                                            \n\
   4. Validate Lun Zero completion persist. \n\
     - Remove all PVD zero hooks. \n\
     - Let zeroing complete for all LUNs.\n\
     - Reboot SEP.\n\
     - After power up verify that all LUNs have zero checkpoint Invalid.\n\
Description last updated: 07/05/2012.\n";


/*!*******************************************************************
 * @def DOREMI_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define DOREMI_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def DOREMI_MAX_WAIT_SEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define DOREMI_MAX_WAIT_SEC 120

/*!*******************************************************************
 * @def DOREMI_MAX_WAIT_MSEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define DOREMI_MAX_WAIT_MSEC DOREMI_MAX_WAIT_SEC * 1000

/*!*******************************************************************
 * @def DOREMI_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define DOREMI_CHUNKS_PER_LUN 9

/*!*******************************************************************
 * @def DOREMI_DEFAULT_OFFSET
 *********************************************************************
 * @brief The default offset on the disk where we create rg.
 *
 *********************************************************************/
#define DOREMI_DEFAULT_OFFSET 0x10000


/*!*******************************************************************
 * @def DOREMI_MAX_RG
 *********************************************************************
 * @brief Maximum number for RG for this test
 *
 *********************************************************************/
#define DOREMI_MAX_RG         16


#define DOREMI_MAX_REMOVE_PER_RG    2


/*!*******************************************************************
 * @def doremi_pvd_zero_checkpoint_hook_t
 *********************************************************************
 * @brief defines the type of pvd zero checkpoint hook.
 *
 *********************************************************************/
typedef enum doremi_pvd_zero_checkpoint_hook_e
{
    DOREMI_PVD_ZCP_HOOK_START,
    DOREMI_PVD_ZCP_HOOK_RANDOM,
    DOREMI_PVD_ZCP_HOOK_END,
    DOREMI_PVD_ZCP_HOOK_MAX,

} doremi_pvd_zero_checkpoint_hook_t;


/*!*******************************************************************
 * @var doremi_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 * The randomizes the permutation of this array.
 * 
 * @todo random parity is not enabled since copy has issues with R6.
 *
 *********************************************************************/
fbe_test_rg_configuration_t doremi_raid_group_config[] = 
{
    /* width,                        capacity    raid type,                  class,             block size   RAID-id.  bandwidth. */
    
    {8, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,  520,            0,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,   520,            1,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,   520,            2,          0},
    //{FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TYPE_PARITY,   FBE_CLASS_ID_PARITY,   520,            5,          0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,   520,            3, 0}, 
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID0,   FBE_CLASS_ID_STRIPER,  520,            4, 0}, 

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/* Checkpoint hook type and chunk value for each RG. These are initialized 
 * to random values in doremi_test().
 */
doremi_pvd_zero_checkpoint_hook_t pvd_checkpoint_hook_type[DOREMI_MAX_RG];
fbe_u32_t                         pvd_random_chunk[DOREMI_MAX_RG][FBE_RAID_MAX_DISK_ARRAY_WIDTH];


/*!**************************************************************
 * doremi_get_checkpoint_hook_type_string()
 ****************************************************************
 * @brief
 *  Gets human readable strings for hook types.
 *
 * @param hook_type - Hook type.
 *
 * @return char * - hook name string..
 *
 * @author
 *  6/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
static char *
doremi_get_checkpoint_hook_type_string(doremi_pvd_zero_checkpoint_hook_t hook_type)
{
    switch(hook_type)
    {
    case DOREMI_PVD_ZCP_HOOK_START:
        return "START";
        break;
        
    case DOREMI_PVD_ZCP_HOOK_RANDOM:
        return "RANDOM";
        break;
        
    case DOREMI_PVD_ZCP_HOOK_END:
        return "END";
        break;

    default:
        return "UNKNOWN";
        break;
    }
}


/*!**************************************************************
 * doremi_rg_fill_disk_sets()
 ****************************************************************
 * @brief
 *  Fill out the disk sets for a given raid group when we have
 *  a given number of max disks.
 *
 * @param rg_config_p - Raid group config array ptr.
 *
 * @return None.
 *
 * @author
 *  6/28/2012 - Created. Prahlad Purohit
 *
 * @note 
 *  wrote this function to use system disks for rg config.
 *
 ****************************************************************/
void doremi_rg_fill_disk_sets(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t rg_index;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t drive_index;
    fbe_u32_t current_slot = 0;
    fbe_u32_t current_encl = 1;    

    /* Fill in the raid group disk sets.
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (fbe_test_rg_config_is_enabled(&rg_config_p[rg_index]))
        {
            for ( drive_index = 0; drive_index < rg_config_p[rg_index].width; drive_index++)
            {
                rg_config_p[rg_index].rg_disk_set[drive_index].bus = 0;
                rg_config_p[rg_index].rg_disk_set[drive_index].enclosure = current_encl;
                rg_config_p[rg_index].rg_disk_set[drive_index].slot = current_slot;
                current_slot++;
                if ((current_slot == FBE_TEST_DRIVES_PER_ENCL) ||
                    ((current_encl == 0) && current_slot == fbe_private_space_layout_get_number_of_system_drives()))
                {
                    current_encl++;
                    current_slot = 0;
                }
            }
        }  // if enabled
    }
    return;
}
/**************************************
 * end doremi_rg_fill_disk_sets()
 **************************************/


/*!**************************************************************
 * doremi_randomize_rg_config_set()
 ****************************************************************
 * @brief
 *  Randomizes the permuatation or raid group configrations in given
 *  rg_config array.
 *
 * @param rg_config_p - Raid group config array ptr.
 *
 * @return None.
 *
 * @author
 *  6/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_randomize_rg_config_set(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t rg_index;
    fbe_u32_t rand_index;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t temp_rg_config;

    for ( rg_index = raid_group_count - 1; rg_index > 0; rg_index--)
    {
        rand_index = fbe_random() % rg_index;

        memcpy(&temp_rg_config, &rg_config_p[rg_index], sizeof(fbe_test_rg_configuration_t));
        memcpy(&rg_config_p[rg_index], &rg_config_p[rand_index], sizeof(fbe_test_rg_configuration_t));
        memcpy(&rg_config_p[rand_index], &temp_rg_config, sizeof(fbe_test_rg_configuration_t));
    }

    return;
}


void doremi_create_physical_config_for_disk_counts(fbe_u32_t num_520_drives,
                                                   fbe_u32_t num_4160_drives)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;
    fbe_api_terminator_device_handle_t  port_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_u32_t num_objects = 1; /* start with board */
    fbe_u32_t encl_number = 0;
    fbe_u32_t encl_index;
    fbe_u32_t num_520_enclosures;
    fbe_u32_t num_4160_enclosures;
    fbe_u32_t num_first_enclosure_drives;
    fbe_block_count_t drive_capacity;

    num_520_enclosures = (num_520_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_520_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);
    num_4160_enclosures = (num_4160_drives / FBE_TEST_DRIVES_PER_ENCL) + ((num_4160_drives % FBE_TEST_DRIVES_PER_ENCL) ? 1 : 0);

    /* Before loading the physical package we initialize the terminator.
     */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    num_first_enclosure_drives = fbe_private_space_layout_get_number_of_system_drives();

    drive_capacity =  TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY + TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY;    

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
     *  plus and one pdo for each of the first 10 drives.
     */
    num_objects = 3 + (num_first_enclosure_drives); 

    /* Next, add on all the enclosures and drives we will add.
     */
    num_objects += num_520_enclosures + ( num_520_drives);
    num_objects += num_4160_enclosures + ( num_4160_drives);

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
    status = fbe_api_wait_for_number_of_objects(num_objects, 60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===");

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


/*!**************************************************************
 * doremi_create_physical_config_for_rg()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param rg_config_p - We determine how many of each drive type
 *                      (520 or 512) we need from the rg configs.
 * @param num_raid_groups
 *
 * @return None.
 *
 * @author
 *  06/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/

void doremi_create_physical_config_for_rg(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_u32_t num_raid_groups)
{
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;
    fbe_u32_t num_520_extra_drives = 0;
    fbe_u32_t num_4160_extra_drives = 0;

    /* First get the count of drives.
     */
    fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, num_raid_groups,
                                          &num_520_drives,
                                          &num_4160_drives);

    /* consider extra disk set into account if count is set by consumer test of this functionality
     */
    fbe_test_sep_util_get_rg_num_extra_drives(rg_config_p, num_raid_groups,
                                          &num_520_extra_drives,
                                          &num_4160_extra_drives);
    num_520_drives+=num_520_extra_drives;
    num_4160_drives+=num_4160_extra_drives;
    
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 512 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);


    /* The drive capacity specified here is min sys + min non sys because
     * this test creates RGs across sys and non sys drives.
     */
    doremi_create_physical_config_for_disk_counts(num_520_drives,
                                                  num_4160_drives);

    /* Fill in our raid group disk sets with this physical config.
     */
    doremi_rg_fill_disk_sets(rg_config_p);
    
    return;
}
/******************************************
 * end doremi_create_physical_config_for_rg()
 ******************************************/


/*!**************************************************************
 * doremi_get_zero_perc_for_hook_type_and_chunk()
 ****************************************************************
 * @brief
 *  Gets the zeroing percentage at completion of specified zeroing.
 *
 * @param hook_type   - Hook type.
 * @param hook_chunk_p- Checkpoint chunks for random hook type.
 * @param width       - RG width.
 * @param data_drives - number of data disk in RG
 * @param lun_capacity - lun capacity 
 *
 * @return fbe_u32_t  - Zeroing percentage.
 *
 * @author
 *  06/29/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
fbe_u32_t doremi_get_zero_perc_for_hook_type_and_chunk(doremi_pvd_zero_checkpoint_hook_t hook_type,
                                                       fbe_u32_t                         *hook_chunk_p,
                                                       fbe_u32_t                         width,
                                                       fbe_u32_t                         data_drives,
                                                       fbe_lba_t                        lun_capacity)
{
    fbe_u32_t zero_chunks = 0;
    fbe_u32_t zero_percentage = 0;
    fbe_u32_t drive_index = 0;
    
    /* Determine the zeroing checkpoint based on the hook type & chunk.
     */
    switch(hook_type)
    {
    case DOREMI_PVD_ZCP_HOOK_START:
        zero_percentage = 0;
        break;

    case DOREMI_PVD_ZCP_HOOK_END:
        zero_percentage = 100;
        break;

    case DOREMI_PVD_ZCP_HOOK_RANDOM:
        for(drive_index = 0; drive_index < width; drive_index++, hook_chunk_p++)
        {

            zero_chunks += *hook_chunk_p;
        }

        /* Following calculation will calculate the expected zero percentage
         */
        zero_percentage = (fbe_u32_t)((zero_chunks * FBE_RAID_DEFAULT_CHUNK_SIZE * data_drives * 100)/(width * lun_capacity));
        break;

    default:
        MUT_ASSERT_TRUE_MSG(FALSE, "Invalid zero checkpoint hook type");
        break;
    }

    return zero_percentage;
}


/*!**************************************************************
 * doremi_get_zero_perc_for_hook_type_and_chunk_r10()
 ****************************************************************
 * @brief
 *  Gets the zeroing percentage for r1_0 at completion of specified zeroing.
 *
 * @param hook_type   - Hook type.
 * @param hook_chunk_p- Checkpoint chunks for random hook type.
 * @param width       - RG width.
 * @param data_drives - number of data disk in RG
 * @param lun_capacity - lun capacity 
 *
 * @return fbe_u32_t  - Zeroing percentage.
 *
 *
 ****************************************************************/
fbe_u32_t doremi_get_zero_perc_for_hook_type_and_chunk_r10(doremi_pvd_zero_checkpoint_hook_t hook_type,
                                                       fbe_u32_t                         *hook_chunk_p,
                                                       fbe_u32_t                         width,
                                                       fbe_u32_t                         data_drives,
                                                       fbe_lba_t                        lun_capacity)
{
    fbe_u32_t zero_chunks = 0;
    fbe_u32_t zero_percentage = 0;
    fbe_u32_t drive_index = 0;
    fbe_u32_t   rg_mirror_width = 2; /* R1_0 has to deal with mirror width */
    
    /* Determine the zeroing checkpoint based on the hook type & chunk.
     */
    switch(hook_type)
    {
    case DOREMI_PVD_ZCP_HOOK_START:
        zero_percentage = 0;
        break;

    case DOREMI_PVD_ZCP_HOOK_END:
        zero_percentage = 100;
        break;

    case DOREMI_PVD_ZCP_HOOK_RANDOM:
        for(drive_index = 0; drive_index < width; drive_index++, hook_chunk_p++)
        {

            zero_chunks += *hook_chunk_p;
        }

        /* Following calculation will calculate the expected zero percentage
         */
        zero_percentage = (fbe_u32_t)((zero_chunks * FBE_RAID_DEFAULT_CHUNK_SIZE * data_drives * 100)/(rg_mirror_width * lun_capacity));
        break;

    default:
        MUT_ASSERT_TRUE_MSG(FALSE, "Invalid zero checkpoint hook type");
        break;
    }

    return zero_percentage;
}


/*!**************************************************************
 * doremi_get_lba_for_hook_type_and_chunk()
 ****************************************************************
 * @brief
 *  Gets the zero checkpoint lba for specified hook type and chunk.
 *
 * @param hook_type   - Hook type.
 * @param hook_chunk  - Checkpoint chunk for random hook type.
 * @param lu_capacity - Lun capacity.
 * @param is_sys_drive- TRUE indicates system drive.
 *
 * @return fbe_lba_t  - LBA checkpoint.
 *
 * @author
 *  06/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
fbe_lba_t doremi_get_lba_for_hook_type_and_chunk(doremi_pvd_zero_checkpoint_hook_t hook_type,
                                                 fbe_u32_t                         hook_chunk,
                                                 fbe_lba_t                         lu_capacity,
                                                 fbe_bool_t                        is_sys_drive)
{
    fbe_lba_t   checkpoint_lba = FBE_LBA_INVALID;
    fbe_lba_t   rg_start_lba = FBE_LBA_INVALID;

    if(is_sys_drive) 
    {
        rg_start_lba = fbe_private_space_get_capacity_imported_from_pvd();
    } 
    else
    {
        rg_start_lba =  DOREMI_DEFAULT_OFFSET;
    }

    /* Determine the zeroing checkpoint based on the hook type & chunk.
     */
    switch(hook_type)
    {
    case DOREMI_PVD_ZCP_HOOK_START:
        checkpoint_lba = rg_start_lba;
        break;

    case DOREMI_PVD_ZCP_HOOK_END:
        checkpoint_lba = rg_start_lba + lu_capacity;
        break;

    case DOREMI_PVD_ZCP_HOOK_RANDOM:
        checkpoint_lba = rg_start_lba + (hook_chunk * FBE_RAID_DEFAULT_CHUNK_SIZE);
        break;

    default:
        MUT_ASSERT_TRUE_MSG(FALSE, "Invalid zero checkpoint hook type");
        break;
    }

    return checkpoint_lba;
}


fbe_bool_t
doremi_is_system_drive(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    if((0 == bus) && (0 == encl) && 
       (slot < fbe_private_space_layout_get_number_of_system_drives()))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*!**************************************************************
 * doremi_enable_zeroing_and_wait_for_pvd_hooks()
 ****************************************************************
 * @brief
 *  Waits for the Zero checkpoint hooks on all PVDs of specified RG.
 *
 * @param rg_config_p - Ptr to RG configuration.
 * @param hook_type   - Hook type.
 * @param hook_chunk_p- Checkpoint chunks for random hook type.
 *
 * @return None.
 *
 * @author
 *  06/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_enable_zeroing_and_wait_for_pvd_hooks(fbe_test_rg_configuration_t       *rg_config_p,
                                                  doremi_pvd_zero_checkpoint_hook_t hook_type,
                                                  fbe_u32_t                         *hook_chunk_p)
{
    fbe_status_t        status;
    fbe_object_id_t     pvd_id[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t           disk_position;
    fbe_lba_t           checkpoint_lba;
    fbe_bool_t          is_system_drive = FALSE;

    /* We fill the RG config with drives in sequentially increasing slot & encl.
     * order. Thus if rg_disk_set 0 is not a system disk no disk here is a 
     * system disk.
     */
    is_system_drive = doremi_is_system_drive(rg_config_p->rg_disk_set[0].bus, 
                                             rg_config_p->rg_disk_set[0].enclosure,
                                             rg_config_p->rg_disk_set[0].slot);

    /* Enable zeroing for all PVDs in this RG.
     */
    for(disk_position = 0; disk_position < rg_config_p->width; disk_position++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[disk_position].bus,
                                                                 rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                 rg_config_p->rg_disk_set[disk_position].slot,
                                                                 &pvd_id[disk_position]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_id[disk_position]);

        status = fbe_test_sep_util_provision_drive_enable_zeroing(pvd_id[disk_position]);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Enable zeroing for all PVDs in this RG.
     */
    for(disk_position = 0; disk_position < rg_config_p->width; disk_position++, hook_chunk_p++)
    {
    
        checkpoint_lba = doremi_get_lba_for_hook_type_and_chunk(hook_type, 
                                                                *hook_chunk_p,
                                                                rg_config_p->logical_unit_configuration_list[0].imported_capacity,
                                                                is_system_drive);

        status = fbe_test_wait_for_debug_hook(pvd_id[disk_position], 
                                             SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO, 
                                             FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
                                             SCHEDULER_CHECK_VALS_LT, SCHEDULER_DEBUG_ACTION_PAUSE,
                                             checkpoint_lba, NULL);
        
        MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    }

    return;
}


/*!**************************************************************
 * doremi_disable_zeroing_for_all_drives()
 ****************************************************************
 * @brief
 *  Disables zeroing on all drives for specified RG config.
 *
 * @param rg_config_p - Ptr to RG configuration.
 *
 * @return None.
 *
 * @author
 *  06/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_disable_zeroing_for_all_drives(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t        status;
    fbe_object_id_t     pvd_id;
    fbe_u32_t           disk_position;

    /* Get object id for each PVD and set the zeroing checkpoint.
     */
    for(disk_position = 0; disk_position < rg_config_p->width; disk_position++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[disk_position].bus,
                                                                 rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                 rg_config_p->rg_disk_set[disk_position].slot,
                                                                 &pvd_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_id);

        status = fbe_api_wait_for_object_lifecycle_state(pvd_id, FBE_LIFECYCLE_STATE_READY,
                                                         DOREMI_MAX_WAIT_MSEC, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);        

        status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}


/*!**************************************************************
 * doremi_setup_pvd_hooks()
 ****************************************************************
 * @brief
 *  Setup specified type of zeroing checkpoint hooks for all PVDs on specified 
 *  Raid Group.
 *
 * @param rg_config_p - Ptr to RG configuration.
 * @param hook_type   - Hook type.
 * @param hook_chunk_p- Checkpoint chunks for random hook type.
 *
 * @return None.
 *
 * @author
 *  06/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_setup_pvd_hooks(fbe_test_rg_configuration_t       *rg_config_p,
                            doremi_pvd_zero_checkpoint_hook_t hook_type,
                            fbe_u32_t                         *hook_chunk_p)
{
    fbe_status_t        status;
    fbe_object_id_t     pvd_id;
    fbe_u32_t           disk_position;
    fbe_lba_t           checkpoint_lba;
    fbe_bool_t          is_system_drive = FALSE;


    doremi_disable_zeroing_for_all_drives(rg_config_p);

    /* We fill RG drives in sequentially increasing slot and encl order.
     * Thus if rg_disk_set 0 is not a system disk no disk here is a system disk.
     */
    is_system_drive = doremi_is_system_drive(rg_config_p->rg_disk_set[0].bus,
                                             rg_config_p->rg_disk_set[0].enclosure,
                                             rg_config_p->rg_disk_set[0].slot);

    /* Get object id for each PVD and set the zeroing checkpoint.
     */
    for(disk_position = 0; disk_position < rg_config_p->width; disk_position++, hook_chunk_p++)
    {
    
        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[disk_position].bus,
                                                                 rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                 rg_config_p->rg_disk_set[disk_position].slot,
                                                                 &pvd_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_id);

        checkpoint_lba = doremi_get_lba_for_hook_type_and_chunk(hook_type, 
                                                                *hook_chunk_p,
                                                                rg_config_p->logical_unit_configuration_list[0].imported_capacity,
                                                                is_system_drive);

        status = fbe_api_scheduler_add_debug_hook(pvd_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY, 
                                                  checkpoint_lba,
                                                  NULL, SCHEDULER_CHECK_VALS_LT, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

#if 0
        /* If RG is bound at system disk offset set zero checkpoint at
         * end of system area offset because zeroing system area will take too LONG.
         */
        if(is_system_drive)
        {
            status = fbe_api_provision_drive_set_zero_checkpoint(pvd_id, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        } 
#endif

    }

    return;
}


/*!**************************************************************
 * doremi_delete_pvd_hooks()
 ****************************************************************
 * @brief
 *  Setup specified type of zeroing checkpoint hooks for all PVDs on specified 
 *  Raid Group.
 *
 * @param rg_config_p - Ptr to RG configuration.
 * @param hook_type   - Hook type.
 * @param hook_chunk_p- Checkpoint chunks for random hook type.
 *
 * @return None.
 *
 * @author
 *  06/28/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_delete_pvd_hooks(fbe_test_rg_configuration_t       *rg_config_p,
                            doremi_pvd_zero_checkpoint_hook_t hook_type,
                            fbe_u32_t                         *hook_chunk_p)
{
    fbe_status_t        status;
    fbe_object_id_t     pvd_id;
    fbe_u32_t           disk_position;
    fbe_lba_t           checkpoint_lba;
    fbe_bool_t          is_system_drive = FALSE;

    /* We fill RG drives in sequentially increasing slot and encl order.
     * Thus if rg_disk_set 0 is not a system disk no disk here is a system disk.
     */
    is_system_drive = doremi_is_system_drive(rg_config_p->rg_disk_set[0].bus,
                                             rg_config_p->rg_disk_set[0].enclosure,
                                             rg_config_p->rg_disk_set[0].slot);

    /* Get object id for each PVD and set the zeroing checkpoint.
     */
    for(disk_position = 0; disk_position < rg_config_p->width; disk_position++, hook_chunk_p++)
    {
        status = fbe_api_provision_drive_get_obj_id_by_location( rg_config_p->rg_disk_set[disk_position].bus,
                                                                 rg_config_p->rg_disk_set[disk_position].enclosure,
                                                                 rg_config_p->rg_disk_set[disk_position].slot,
                                                                 &pvd_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_id);

        checkpoint_lba = doremi_get_lba_for_hook_type_and_chunk(hook_type, 
                                                                *hook_chunk_p,
                                                                rg_config_p->logical_unit_configuration_list[0].imported_capacity,
                                                                is_system_drive);

        status = fbe_api_scheduler_del_debug_hook(pvd_id, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                                  FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY, 
                                                  checkpoint_lba,
                                                  NULL, SCHEDULER_CHECK_VALS_LT, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return;
}



/*!**************************************************************
 * doremi_setup_checkpoint_info_and_pvd_hooks()
 ****************************************************************
 * @brief
 *  Setup randomized checkpoint hook type. randomized hook value and setup PVD
 *  hooks based on these type and values.
 *
 * @param rg_config_p - Raid group configuration array.               
 *
 * @return None.
 *
 * @author
 *  07/02/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_setup_checkpoint_info_and_pvd_hooks(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);
    const fbe_char_t *raid_type_string_p = NULL;    
    fbe_u32_t rg_index;

    mut_printf(MUT_LOG_TEST_STATUS, "Doremi test will use following LUN Zero checkpoint hooks.");    

    /* For each RG in our config randomly determine the hook type and 
     * checkpoint for random type. Also set the PVD hooks.
     */    
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t disk_index = 0;   

        pvd_checkpoint_hook_type[rg_index] = fbe_random() % DOREMI_PVD_ZCP_HOOK_MAX;

        fbe_test_sep_util_get_raid_type_string(rg_config_p[rg_index].raid_type, &raid_type_string_p);

        mut_printf(MUT_LOG_TEST_STATUS, "RG: %d (%s) checkpoint_type: %s", 
                   rg_index, raid_type_string_p,
                   doremi_get_checkpoint_hook_type_string(pvd_checkpoint_hook_type[rg_index]));

        if(DOREMI_PVD_ZCP_HOOK_RANDOM == pvd_checkpoint_hook_type[rg_index])
        {
            for(disk_index = 0; disk_index < rg_config_p[rg_index].width; disk_index++)
            {
                pvd_random_chunk[rg_index][disk_index] = fbe_random() % (DOREMI_CHUNKS_PER_LUN + 1);

                mut_printf(MUT_LOG_TEST_STATUS, "\tdisk: %d, checkpoint: %d, chunks %d", 
                           disk_index, (pvd_random_chunk[rg_index][disk_index] * 100) / DOREMI_CHUNKS_PER_LUN,pvd_random_chunk[rg_index][disk_index] );
            }
        }

        doremi_setup_pvd_hooks(&rg_config_p[rg_index],
                               pvd_checkpoint_hook_type[rg_index],
                               pvd_random_chunk[rg_index]);
    }
}


/* @todo Following is temporary function until we fix the unaligned lu export
 * size.
 */
void doremi_check_lun_zeroing_percentage(fbe_u32_t exp_zero_perc, 
                                         fbe_u32_t act_zero_perc)
{

    if((exp_zero_perc != act_zero_perc) && 
       ((exp_zero_perc - 1) != act_zero_perc) &&
       ((exp_zero_perc - 2) != act_zero_perc))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Exp: %d, Act: %d", exp_zero_perc, act_zero_perc);
        MUT_ASSERT_TRUE_MSG(FALSE, "Zero percent mismatch !!!");
        
    }
}


/*!**************************************************************
 * doremi_wait_for_lun_zero_update()
 ****************************************************************
 * @brief
 *  Wait for a lun zero checkpoint update to occur.
 *
 * @param lun_object_id - Lun object Id.
 *
 * @return None.
 *
 * @author
 *  07/05/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_wait_for_lun_zero_update(fbe_object_id_t lun_object_id)
{
    fbe_status_t                status;
    fbe_api_lun_get_zero_status_t   lun_zero_status;

    // Add a hook for zero checkpoint update.
    status = fbe_api_scheduler_add_debug_hook(lun_object_id, 
                                              SCHEDULER_MONITOR_STATE_LUN_ZERO,
                                              FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED, 
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Get zero status to see if it in progress or completed.
    status = fbe_api_lun_get_zero_status(lun_object_id, &lun_zero_status);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // If Lun zeroing is still in progress wait for update hook.
    if(lun_zero_status.zero_checkpoint != FBE_LBA_INVALID)
    {
        status = fbe_test_wait_for_debug_hook(lun_object_id,
                                             SCHEDULER_MONITOR_STATE_LUN_ZERO,
                                             FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED,
                                             SCHEDULER_CHECK_STATES,
                                             SCHEDULER_DEBUG_ACTION_CONTINUE,
                                             NULL, NULL);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    
    status = fbe_api_scheduler_del_debug_hook(lun_object_id,
                                              SCHEDULER_MONITOR_STATE_LUN_ZERO,
                                              FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED,
                                              NULL,
                                              NULL, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_CONTINUE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}


/*!**************************************************************
 * doremi_run_tests()
 ****************************************************************
 * @brief
 *  We create a config and run the rebuild tests.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  06/27/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_u32_t                     raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t                     rg_index;
    fbe_status_t                  status;
    fbe_object_id_t               lun_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_lun_get_zero_status_t lun_zero_status;
    fbe_u32_t                     exp_zero_perc;
    fbe_api_terminator_device_handle_t drive_handle[DOREMI_MAX_RG][DOREMI_MAX_REMOVE_PER_RG];
    fbe_api_lun_get_lun_info_t lun_info;


    /*1. Validate zeroing checkpoint percentage with RGs Normal. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Doremi: 1. Normal RG checkpoint test. ===");

    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        doremi_enable_zeroing_and_wait_for_pvd_hooks(&rg_config_p[rg_index],
                                                     pvd_checkpoint_hook_type[rg_index],
                                                     pvd_random_chunk[rg_index]);

        status = fbe_api_database_lookup_lun_by_number(rg_config_p[rg_index].logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        doremi_wait_for_lun_zero_update(lun_object_id);
        status = fbe_api_lun_get_zero_status(lun_object_id, &lun_zero_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(rg_config_p[rg_index].raid_type != FBE_RAID_GROUP_TYPE_RAID10)
        {
            exp_zero_perc = doremi_get_zero_perc_for_hook_type_and_chunk(pvd_checkpoint_hook_type[rg_index], 
                                                                     pvd_random_chunk[rg_index],
                                                                     rg_config_p[rg_index].width,
                                                                     fbe_test_sep_get_data_drives_for_raid_group_config(&rg_config_p[rg_index]),
                                                                     lun_info.capacity);
        }
        else
        {
            exp_zero_perc = doremi_get_zero_perc_for_hook_type_and_chunk_r10(pvd_checkpoint_hook_type[rg_index], 
                                                                     pvd_random_chunk[rg_index],
                                                                     rg_config_p[rg_index].width,
                                                                     fbe_test_sep_get_data_drives_for_raid_group_config(&rg_config_p[rg_index]),
                                                                     lun_info.capacity);
        }

        doremi_check_lun_zeroing_percentage(exp_zero_perc, lun_zero_status.zero_percentage);

        mut_printf(MUT_LOG_TEST_STATUS, "1.%d Checkpoint %d pct ok ", rg_index, exp_zero_perc);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== Finished Doremi: 1. Normal RG checkpoint test. ===");


    /*2. Remove a drive in each RG to make them degraded. Validate zero pct. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Doremi: 2. Degraded RG checkpoint test. ===");

    for(rg_index = 1; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t           number_physical_objects;

        status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
        sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[rg_index], SEP_REBUILD_UTILS_POSITION_0,
                                                  number_physical_objects - 1, &drive_handle[rg_index][SEP_REBUILD_UTILS_POSITION_0]);
    }

    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(rg_config_p[rg_index].logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if (rg_config_p[rg_index].raid_type != FBE_RAID_GROUP_TYPE_RAID0)
        {
            doremi_wait_for_lun_zero_update(lun_object_id);
        }
        status = fbe_api_lun_get_zero_status(lun_object_id, &lun_zero_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);     

        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if(rg_config_p[rg_index].raid_type != FBE_RAID_GROUP_TYPE_RAID10)
        {
            exp_zero_perc = doremi_get_zero_perc_for_hook_type_and_chunk(pvd_checkpoint_hook_type[rg_index], 
                                                                     pvd_random_chunk[rg_index],
                                                                     rg_config_p[rg_index].width,
                                                                     fbe_test_sep_get_data_drives_for_raid_group_config(&rg_config_p[rg_index]),
                                                                     lun_info.capacity);
        }
        else
        {
            exp_zero_perc = doremi_get_zero_perc_for_hook_type_and_chunk_r10(pvd_checkpoint_hook_type[rg_index], 
                                                                     pvd_random_chunk[rg_index],
                                                                     rg_config_p[rg_index].width,
                                                                     fbe_test_sep_get_data_drives_for_raid_group_config(&rg_config_p[rg_index]),
                                                                     lun_info.capacity);
        }
        doremi_check_lun_zeroing_percentage(exp_zero_perc, lun_zero_status.zero_percentage);

        mut_printf(MUT_LOG_TEST_STATUS, "2.%d Checkpoint %d pct ok ", rg_index, exp_zero_perc);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "=== Finished Doremi: 2. Degraded RG checkpoint test. ===");


    /*3. Remove second drive in each RG to make them broken. Validate zero pct. */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Doremi: 3. Broken RG checkpoint test. ===");

    for(rg_index = 1; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t           number_physical_objects;

        status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
        sep_rebuild_utils_remove_drive_and_verify(&rg_config_p[rg_index], SEP_REBUILD_UTILS_POSITION_1,
                                                  number_physical_objects - 1, &drive_handle[rg_index][SEP_REBUILD_UTILS_POSITION_1]);
    }

    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(rg_config_p[rg_index].logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        // Here LU is in failed state so no point waiting for CP update.
        status = fbe_api_lun_get_zero_status(lun_object_id, &lun_zero_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);        

        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);        

        if(rg_config_p[rg_index].raid_type != FBE_RAID_GROUP_TYPE_RAID10)
        {
            exp_zero_perc = doremi_get_zero_perc_for_hook_type_and_chunk(pvd_checkpoint_hook_type[rg_index], 
                                                                     pvd_random_chunk[rg_index],
                                                                     rg_config_p[rg_index].width,
                                                                     fbe_test_sep_get_data_drives_for_raid_group_config(&rg_config_p[rg_index]),
                                                                     lun_info.capacity);
        }
        else
        {
            exp_zero_perc = doremi_get_zero_perc_for_hook_type_and_chunk_r10(pvd_checkpoint_hook_type[rg_index], 
                                                                     pvd_random_chunk[rg_index],
                                                                     rg_config_p[rg_index].width,
                                                                     fbe_test_sep_get_data_drives_for_raid_group_config(&rg_config_p[rg_index]),
                                                                     lun_info.capacity);
        }
        doremi_check_lun_zeroing_percentage(exp_zero_perc, lun_zero_status.zero_percentage);

        mut_printf(MUT_LOG_TEST_STATUS, "3.%d Checkpoint %d pct ok ", rg_index, exp_zero_perc);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "=== Finished Doremi: 3. Broken RG checkpoint test. ===");


    /* Re-insert drives we removed before.
     */
    for(rg_index = 1; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t           number_physical_objects;
    
        status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
        sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[rg_index], SEP_REBUILD_UTILS_POSITION_0, 
                                                    (number_physical_objects + 1 /* Pdo*/), 
                                                    &drive_handle[rg_index][SEP_REBUILD_UTILS_POSITION_0]);

        status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
        sep_rebuild_utils_reinsert_drive_and_verify(&rg_config_p[rg_index], SEP_REBUILD_UTILS_POSITION_1, 
                                                    (number_physical_objects + 1 /* Pdo*/), 
                                                    &drive_handle[rg_index][SEP_REBUILD_UTILS_POSITION_1]);
    }


    /*4. Remove the hooks and let zeroing complete for all Luns. Reboot the SP.
     * verify that all LUNs have invalid zero checkpoint (persisted).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Starting Doremi: 4. Zero checkpoint persist test. ===");
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        doremi_delete_pvd_hooks(&rg_config_p[rg_index], 
                                pvd_checkpoint_hook_type[rg_index],
                                pvd_random_chunk[rg_index]);
    }

    /* Wait for zeroing to complete for all LUNs so that checkpoint is 
     * persisted.
     */
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t check_iterations = 500;
        
        while(check_iterations--)
        {
            status = fbe_api_database_lookup_lun_by_number(rg_config_p[rg_index].logical_unit_configuration_list[0].lun_number, 
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_lun_get_zero_status(lun_object_id, &lun_zero_status);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if(FBE_LBA_INVALID == lun_zero_status.zero_checkpoint)
            {
                mut_printf(MUT_LOG_TEST_STATUS, "lun zeroing finnished for lun %d ",lun_object_id );
                break;
            }

            fbe_api_sleep(200);
        }

        MUT_ASSERT_INT_NOT_EQUAL(0, check_iterations);

        /* Disable zeroing on all drives of this RG so that there is no zeroing
         * even after we reboot the SP.
         */
        doremi_disable_zeroing_for_all_drives(&rg_config_p[rg_index]);
    }

    // Reboot SEP.
    mut_printf(MUT_LOG_TEST_STATUS, "%s Rebooting SEP", __FUNCTION__);    
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();

    // Verify that all LUs have Invalid zero checkpoint that was persisted earlier.
    for(rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_test_sep_util_wait_for_all_lun_objects_ready(&rg_config_p[rg_index],
                                                                  fbe_api_transport_get_target_server());
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
        status = fbe_api_database_lookup_lun_by_number(rg_config_p[rg_index].logical_unit_configuration_list[0].lun_number, 
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_lun_get_zero_status(lun_object_id, &lun_zero_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        doremi_check_lun_zeroing_percentage(100, lun_zero_status.zero_percentage);

        mut_printf(MUT_LOG_TEST_STATUS, "4.%d Checkpoint %d pct ok ", rg_index, 100);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "=== Finished Doremi: 4. Zero checkpoint persist test. ===");    

    return;
}
/**************************************
 * end doremi_run_tests()
 **************************************/

/*!**************************************************************
 * doremi_test()
 ****************************************************************
 * @brief
 *  Run Lun zeroing checkpoint test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06/27/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = &doremi_raid_group_config[0];
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);

    MUT_ASSERT_TRUE(raid_group_count <= DOREMI_MAX_RG);

    fbe_test_sep_util_fill_lun_configurations_rounded(rg_config_p, 
                                                      raid_group_count,
                                                      DOREMI_CHUNKS_PER_LUN, 
                                                      DOREMI_LUNS_PER_RAID_GROUP);

    /* setup randomized zero checkpoint type values and pvd hooks based on it. 
     */
    doremi_setup_checkpoint_info_and_pvd_hooks(rg_config_p);

    fbe_test_sep_util_create_raid_group_configuration(rg_config_p, raid_group_count);    

    doremi_run_tests(rg_config_p, NULL);

    return;
}
/******************************************
 * end doremi_test()
 ******************************************/
/*!**************************************************************
 * doremi_setup()
 ****************************************************************
 * @brief
 *  Setup the doremi test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  06/27/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
void doremi_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &doremi_raid_group_config[0];

        /* Randomize the permitation of rg config to allow system disks have
         * a random rg config.
         */
        doremi_randomize_rg_config_set(rg_config_p);

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        
        /* The below code is disabled because copy cannot handle unconsumed areas on a LUN yet.
         */
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, DOREMI_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        doremi_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }
    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end doremi_setup()
 **************************************/
/*!**************************************************************
 * doremi_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the doremi test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  06/27/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/

void doremi_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end doremi_cleanup()
 ******************************************/
/*************************
 * end file doremi_test.c
 *************************/

