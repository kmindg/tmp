 /***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sep_standard_logical_config.c
 ***************************************************************************
 *
 * @brief
 *  This file contains standard logical configs for sep tests.  Physical config
 *  must be created before calling the logical config.
 *
 * @version
 *   5/20/2011 - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_utils.h"
#include "pp_utils.h"
#include "sep_tests.h"



/*-----------------------------------------------------------------------------
    DEFINITIONS:
*/

/*!*******************************************************************
 * @def BUBBLE_BASS_TEST_RAID_GROUP_COUNT
 *********************************************************************
 * @brief Max number of RGs to create
 *
 *********************************************************************/
#define STANDARD_LOGICAL_CONFIG_1_RG          2

/*!*******************************************************************
 * @def STANDARD_LOGICAL_CONFIG_1_RG_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define STANDARD_LOGICAL_CONFIG_1_RG_CHUNKS_PER_LUN 3


/* use a static for now.  can be configured on the fly base on the hardware configuration*/
static fbe_test_rg_configuration_t sep_standard_logical_config_one_r5[2] = 
{
    /* width, capacity, raid type,                 class,               block size, RAID-id.    bandwidth.*/
    {3,       0xE000,   FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 4160,        5,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

static fbe_test_rg_configuration_t sep_standard_logical_config_one_of_each[6] = 
{
    /* width,   capacity    raid type,                  class,              block size, RAID-id.    bandwidth.*/
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER,   4160,            10,         0},
    {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,    4160,            5,          0},
    {4,       0xE000,      FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,    4160,            6,          0},
    {5,       0xE000,      FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,    4160,            3,          0},
    {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,    4160,            1,          0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};


void sep_standard_logical_config_get_rg_configuration(fbe_test_sep_rg_config_type_t rg_type, fbe_test_rg_configuration_t **rg_config_p)
{
    switch(rg_type) {
    case FBE_TEST_SEP_RG_CONFIG_TYPE_ONE_R5:
        *rg_config_p = sep_standard_logical_config_one_r5;
        break;
    case FBE_TEST_SEP_RG_CONFIG_TYPE_ONE_OF_EACH:
        *rg_config_p = sep_standard_logical_config_one_of_each;
        break;
    default:
        MUT_FAIL_MSG("rg config type not supported!");
    }
    return;
}


/*!**************************************************************
 * sep_standard_logical_config_load_physical_config()
 ****************************************************************
 * @brief
 *  Configure the test's physical configuration based on the
 *  raid groups we intend to create.
 *
 * @param config_p - Current config.
 *
 * @return None.
 *
 * @author
 *  8/23/2010 - Created. guov
 *
 ****************************************************************/
void sep_standard_logical_config_load_physical_config(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_u32_t                   raid_group_count;
    const fbe_char_t *raid_type_string_p = NULL;
    fbe_u32_t num_520_drives = 0;
    fbe_u32_t num_4160_drives = 0;

    fbe_test_sep_util_get_raid_type_string(rg_config_p->raid_type, &raid_type_string_p);
    raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    /* First get the count of drives.
     */
    fbe_test_sep_util_get_rg_num_enclosures_and_drives(rg_config_p, raid_group_count,
                                                       &num_520_drives,
                                                       &num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: %s %d 520 drives and %d 4160 drives", 
               __FUNCTION__, raid_type_string_p, num_520_drives, num_4160_drives);

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Creating %d 520 drives and %d 4160 drives", 
               __FUNCTION__, num_520_drives, num_4160_drives);

    /* Add a configuration to each raid group config.
     */
    fbe_test_sep_util_rg_fill_disk_sets(rg_config_p, raid_group_count, 
                                        num_520_drives, num_4160_drives);
    fbe_test_pp_util_create_physical_config_for_disk_counts(num_520_drives,
                                                            num_4160_drives,
                                                            TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY+rg_config_p->capacity);
    return;
}

void sep_standard_logical_config_create_rg_with_lun(fbe_test_rg_configuration_t *raid_group_config_p, fbe_u32_t luns_per_rg)
{
    fbe_u32_t       raid_group_count;

    fbe_test_sep_util_init_rg_configuration_array(raid_group_config_p);

    sep_standard_logical_config_load_physical_config(raid_group_config_p);

    sep_config_load_sep_and_neit();
    
    raid_group_count = fbe_test_get_rg_array_length(raid_group_config_p);

    /* Setup the lun capacity with the given number of chunks for each lun.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(raid_group_config_p, 
                                                      raid_group_count,
                                                      STANDARD_LOGICAL_CONFIG_1_RG_CHUNKS_PER_LUN, 
                                                      luns_per_rg);
    
    /* Kick of the creation of all the raid group with Logical unit configuration.
     */
    fbe_test_sep_util_create_raid_group_configuration(raid_group_config_p, raid_group_count);

    return;

} /* End sep_standard_logical_config_create_rg_with_lun() */




/*************************
 * end file sep_standard_logical_config.c
 *************************/


