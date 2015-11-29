/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file meriadoc_brandybuck_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of mapping lba to pba.
 *
 * @version
 *  6/6/2012 - Created. Rob Foley
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
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "pp_utils.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_common_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * meriadoc_brandybuck_short_desc = "raid group map of lba to pba test.";
char * meriadoc_brandybuck_long_desc ="\
The meriadoc_brandybuck scenario tests the raid groups mapping of lba to pba functionality \n";


enum meriadoc_brandybuck_enum {
    MERIADOC_BRANDYBUCK_RG_CONFIGS = 11,
    MERIADOC_BRANDYBUCK_MAX_TEST_CASES_PER_RG = 11,
    MERIADOC_BRANDYBUCK_LUNS_PER_RAID_GROUP = 2,
    MERIADOC_BRANDYBUCK_CHUNKS_PER_LUN = 999,
    MERIADOC_BRANDYBUCK_DRIVE_CAPACITY = FBE_RAID_DEFAULT_CHUNK_SIZE * MERIADOC_BRANDYBUCK_CHUNKS_PER_LUN * \
                                          MERIADOC_BRANDYBUCK_LUNS_PER_RAID_GROUP * 2,
};
/*!*******************************************************************
 * @var meriadoc_brandybuck_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations for qual (level 0).
 *
 *********************************************************************/
fbe_test_rg_configuration_array_t meriadoc_brandybuck_raid_group_config_qual[2] = 
{
    /* width,   capacity  raid type,             class,                               block size  RAID-id. bandwidth.*/

    {
        {5, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,  0, 0},
        {5, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520,  1, 1},
        {9, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 8, FBE_RAID_GROUP_TYPE_RAID3, FBE_CLASS_ID_PARITY, 520,  2, 0},
        {9, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 8, FBE_RAID_GROUP_TYPE_RAID3, FBE_CLASS_ID_PARITY, 520,  3, 1},
        {6, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520,  4, 0},
        {6, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520,  5, 1},
        {2, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 1, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR, 520,  6, 0},
        {8, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER, 520,7, 0},
        {8, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID10, FBE_CLASS_ID_STRIPER, 520,8, 1},
        {4, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID0, FBE_CLASS_ID_STRIPER, 520, 9, 0},
        {4, MERIADOC_BRANDYBUCK_DRIVE_CAPACITY * 4, FBE_RAID_GROUP_TYPE_RAID0, FBE_CLASS_ID_STRIPER, 520, 10, 1},
        
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

static fbe_raid_group_map_info_t meriadoc_brandybuck_test_cases[][MERIADOC_BRANDYBUCK_MAX_TEST_CASES_PER_RG] = 
{
    /* Raid-5 small element size */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           4,       0,         0,     0},
        {0x400,  0x100,     0,              0,           4,       0,         0,     0},
        {0x800,  0x200,     0,              0,           4,       0,         0,     0},
        {0x1080, 0x400,     0,              0,           4,       1,         0,     0},
        {0x8000, 0x2000,    4,              0,           2,       3,         0,     0},
        {0x8400, 0x2100,    4,              0,           2,       3,         0,     0},
        {0x100000, 0x40000, 128,            1,           0,       1,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-5 large element size */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           4,       0,         0,     0},
        {0x400,  0,         0,              0,           3,       0,         0,     0},
        {0x1000, 0x400,     0,              0,           4,       0,         0,     0},
        {0x2000, 0x800,     1,              0,           0,       1,         0,     0},
        {0x8000, 0x2000,    4,              0,           3,       4,         0,     0},
        {0x100000, 0x40000, 128,            1,           2,       3,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-3 small element size. */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           8,       0,         0,     0},
        {0x80,   0,         0,              0,           7,       0,         0,     0},
        {0x400,  0x80,      0,              0,           8,       0,         0,     0},
        {0x2000, 0x400,     0,              0,           8,       0,         0,     0},
        {0x4000, 0x800,     1,              0,           8,       0,         0,     0},
        {0x8000, 0x1000,    2,              0,           8,       0,         0,     0},
        {0x200000, 0x40000, 128,            1,           8,       0,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-3 large element size. */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           8,       0,         0,     0},
        {0x400,  0,         0,              0,           7,       0,         0,     0},
        {0x1c00, 0,         0,              0,           1,       0,         0,     0},
        {0x2000, 0x400,     0,              0,           8,       0,         0,     0},
        {0x4000, 0x800,     1,              0,           8,       0,         0,     0},
        {0x10000, 0x2000,   4,              0,           8,       0,         0,     0},
        {0x200000, 0x40000, 128,            1,           8,       0,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-6 small element size*/
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           5,       0,         0,     0},
        {0x400,  0x100,     0,              0,           5,       0,         0,     0},
        {0x800,  0x200,     0,              0,           5,       0,         0,     0},
        {0x1080, 0x400,     0,              0,           0,       2,         0,     0},
        {0x8000, 0x2000,    4,              0,           3,       4,         0,     0},
        {0x8400, 0x2100,    4,              0,           3,       4,         0,     0},
        {0x100000, 0x40000, 128,            1,           1,       2,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-6 large element size */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           5,       0,         0,     0},
        {0x400,  0,         0,              0,           4,       0,         0,     0},
        {0x1000, 0x400,     0,              0,           5,       0,         0,     0},
        {0x2000, 0x800,     1,              0,           1,       2,         0,     0},
        {0x8000, 0x2000,    4,              0,           1,       2,         0,     0},
        {0x100000, 0x40000, 128,            1,           3,       4,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-1  */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           0,       1,         0,     0},
        {0x400,  0x400,     0,              0,           0,       1,         0,     0},
        {0x1000, 0x1000,    2,              0,           0,       1,         0,     0},
        {0x2000, 0x2000,    4,              0,           0,       1,         0,     0},
        {0x8000, 0x8000,    16,             0,           0,       1,         0,     0},
        {0x40000, 0x40000,  128,            1,           0,       1,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-10   */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           0,       1,         0,     0},
        {0x80,   0,         0,              0,           2,       3,         0,     0},
        {0x200,  0x80,      0,              0,           0,       1,         0,     0},
        {0x1000, 0x400,     0,              0,           0,       1,         0,     0},
        {0x2000, 0x800,     1,              0,           0,       1,         0,     0},
        {0x100000, 0x40000, 128,            1,           0,       1,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-10 large element size */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           0,       1,         0,     0},
        {0x400,  0,         0,              0,           2,       3,         0,     0},
        {0x800,  0,         0,              0,           4,       5,         0,     0},
        {0x1000, 0x400,     0,              0,           0,       1,         0,     0},
        {0x2000, 0x800,     1,              0,           0,       1,         0,     0},
        {0x8000, 0x2000,    4,              0,           0,       1,         0,     0},
        {0x100000, 0x40000, 128,            1,           0,       1,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-0   */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           0,       0,         0,     0},
        {0x80,   0,         0,              0,           1,       0,         0,     0},
        {0x200,  0x80,      0,              0,           0,       0,         0,     0},
        {0x1000, 0x400,     0,              0,           0,       0,         0,     0},
        {0x2000, 0x800,     1,              0,           0,       0,         0,     0},
        {0x100000, 0x40000, 128,            1,           0,       0,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },
    /* Raid-0 large element size */
    {
        /* lba, pba,    chunk_index,    metadata_lba data_pos parity_pos offset original_lba  */
        {0,      0,         0,              0,           0,       0,         0,     0},
        {0x400,  0,         0,              0,           1,       0,         0,     0},
        {0x800,  0,         0,              0,           2,       0,         0,     0},
        {0x1000, 0x400,     0,              0,           0,       0,         0,     0},
        {0x2000, 0x800,     1,              0,           0,       0,         0,     0},
        {0x8000, 0x2000,    4,              0,           0,       0,         0,     0},
        {0x100000, 0x40000, 128,            1,           0,       0,         0,     0},
        {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
    },

    {FBE_LBA_INVALID, FBE_LBA_INVALID, FBE_U64_MAX, /* Terminator. */},
};
/*!**************************************************************
 * meriadoc_brandybuck_test_case()
 ****************************************************************
 * @brief
 *  Test individual test cases for a given raid config.
 *
 * @param object_id
 * @param map_info_p           
 *
 * @return None.   
 *
 * @author
 *  6/6/2012 - Created. Rob Foley
 *
 ****************************************************************/
void meriadoc_brandybuck_lba_to_pba_test_case(fbe_object_id_t object_id,
                                   fbe_raid_group_map_info_t *map_info_p)
{
    fbe_status_t status;
    fbe_raid_group_map_info_t map_info;

    while ((map_info_p->lba != FBE_LBA_INVALID) &&
           (map_info_p->chunk_index != FBE_U64_MAX))
    {
        fbe_zero_memory(&map_info, sizeof(fbe_raid_group_map_info_t));
        map_info.lba = map_info_p->lba;
        status = fbe_api_raid_group_map_lba(object_id, &map_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        if (map_info.lba != map_info_p->lba)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "lba: 0x%llx != expected 0x%llx", (unsigned long long)map_info.lba, (unsigned long long)map_info_p->lba);
            MUT_FAIL_MSG("unexpected lba");
        }
        /* We intentionally do not validate any offset since this could change.
         * The pba from the table does not include the offset, so we add it when we do the compare. 
         */
        if (map_info.pba != map_info_p->pba + map_info.offset)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "pba: 0x%llx != expected 0x%llx", (unsigned long long)map_info.pba, (unsigned long long)map_info_p->pba);
            MUT_FAIL_MSG("unexpected pba");
        }
        if (map_info.chunk_index != map_info_p->chunk_index)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "chunk_index: 0x%llx != expected 0x%llx", (unsigned long long)map_info.chunk_index, (unsigned long long)map_info_p->chunk_index);
            MUT_FAIL_MSG("unexpected chunk_index");
        }
        if (map_info.metadata_lba != map_info_p->metadata_lba)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "metadata_lba: 0x%llx != expected 0x%llx", (unsigned long long)map_info.metadata_lba, (unsigned long long)map_info_p->metadata_lba);
            MUT_FAIL_MSG("unexpected metadata_lba");
        }
        if (map_info.data_pos != map_info_p->data_pos)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "data_pos: 0x%llx != expected 0x%llx", (unsigned long long)map_info.data_pos, (unsigned long long)map_info_p->data_pos);
            MUT_FAIL_MSG("unexpected data_pos");
        }
        if (map_info.parity_pos != map_info_p->parity_pos)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "parity_pos: 0x%llx != expected 0x%llx", (unsigned long long)map_info.parity_pos, (unsigned long long)map_info_p->parity_pos);
            MUT_FAIL_MSG("unexpected parity_pos");
        }
        // we need to set this for the pba to lba translation because raid10 physical 
        // offset on the striper will be 0.
        map_info_p->offset = map_info.offset;
        map_info_p++;
    }
}
/******************************************
 * end meriadoc_brandybuck_lba_to_pba_test_case()
 ******************************************/

/*!**************************************************************
 * meriadoc_brandybuck_pba_to_lba_test_case()
 ****************************************************************
 * @brief
 *  Test individual test cases for a given raid config.
 *
 * @param object_id
 * @param map_info_p           
 *
 * @return None.   
 *
 * @author
 *  11/1/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void meriadoc_brandybuck_pba_to_lba_test_case(fbe_object_id_t object_id,
                                   fbe_raid_group_map_info_t *map_info_p)
{
    fbe_status_t status;
    fbe_raid_group_map_info_t map_info;
    //fbe_api_raid_group_get_info_t rg_info;

    while ((map_info_p->lba != FBE_LBA_INVALID) &&
           (map_info_p->chunk_index != FBE_U64_MAX))
    {
        /*fbe_zero_memory(&map_info, sizeof(fbe_raid_group_map_info_t));

        status = fbe_api_raid_group_get_info(object_id, &rg_info,
                                             FBE_PACKET_FLAG_NO_ATTRIB);*/
        map_info.pba = map_info_p->pba + map_info_p->offset;//+ rg_info.physical_offset;
        map_info.position = map_info_p->data_pos;
        status = fbe_api_raid_group_map_pba(object_id, &map_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        //mut_printf(MUT_LOG_TEST_STATUS, "pba: 0x%llx rgoffset: 0x%llx ", map_info.pba, rg_info.physical_offset);
        if (map_info.pba != map_info_p->pba + map_info_p->offset)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "pba: 0x%llx != expected 0x%llx", map_info.pba, map_info_p->pba);
            MUT_FAIL_MSG("unexpected pba");
        }
        /* We intentionally do not validate any offset since this could change.
         * The pba from the table does not include the offset, so we add it when we do the compare. 
         */
        if (map_info.lba != map_info_p->lba)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "lba: 0x%llx != expected 0x%llx", map_info.lba, map_info_p->lba);
            MUT_FAIL_MSG("unexpected lba");
        }
        if (map_info.chunk_index != map_info_p->chunk_index)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "chunk_index: 0x%llx != expected 0x%llx", map_info.chunk_index, map_info_p->chunk_index);
            MUT_FAIL_MSG("unexpected chunk_index");
        }
        if (map_info.metadata_lba != map_info_p->metadata_lba)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "metadata_lba: 0x%llx != expected 0x%llx", map_info.metadata_lba, map_info_p->metadata_lba);
            MUT_FAIL_MSG("unexpected metadata_lba");
        }
        if (map_info.data_pos != map_info_p->data_pos)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "data_pos: 0x%llx != expected 0x%llx", (csx_u64_t)map_info.data_pos, (csx_u64_t)map_info_p->data_pos);
            MUT_FAIL_MSG("unexpected data_pos");
        }
        if (map_info.parity_pos != map_info_p->parity_pos)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "parity_pos: 0x%llx != expected 0x%llx", (csx_u64_t)map_info.parity_pos, (csx_u64_t)map_info_p->parity_pos);
            MUT_FAIL_MSG("unexpected parity_pos");
        }
        /*if (map_info.offset != rg_info.physical_offset)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "offset: 0x%llx != expected 0x%llx", map_info.offset, rg_info.physical_offset);
            MUT_FAIL_MSG("unexpected offset");
        }*/
        map_info_p++;
    }
}
/******************************************
 * end meriadoc_brandybuck_pba_to_lba_test_case()
 ******************************************/

/*!**************************************************************
 * meriadoc_brandybuck_test_rg_config()
 ****************************************************************
 * @brief
 *  Run a raid6 I/O test.
 *
 * @param rg_config_p - config to run test against.               
 *
 * @return None.   
 *
 * @author
 *  6/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

void meriadoc_brandybuck_test_rg_config(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id;
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    const fbe_char_t *raid_type_string_p = NULL;

    for ( index = 0; index < raid_group_count; index++)
    {
        current_rg_config_p = &rg_config_p[index];
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_test_sep_util_get_raid_type_string(current_rg_config_p->raid_type, &raid_type_string_p);
        mut_printf(MUT_LOG_TEST_STATUS, "test rg %d (rg_id: 0x%x) raid_type: %s bandwidth: %d",
                   current_rg_config_p->raid_group_id, rg_object_id, raid_type_string_p, current_rg_config_p->b_bandwidth);
        meriadoc_brandybuck_lba_to_pba_test_case(rg_object_id, &meriadoc_brandybuck_test_cases[index][0]);
        //if (current_rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10) {
            meriadoc_brandybuck_pba_to_lba_test_case(rg_object_id, &meriadoc_brandybuck_test_cases[index][0]);
        //}
    }
    /* Make sure no errors occurred.
     */
    fbe_test_sep_util_sep_neit_physical_verify_no_trace_error();
    return;
}
/******************************************
 * end meriadoc_brandybuck_test_rg_config()
 ******************************************/
/*!**************************************************************
 * meriadoc_brandybuck_test()
 ****************************************************************
 * @brief
 *  Run a raid group map lba test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

void meriadoc_brandybuck_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &meriadoc_brandybuck_raid_group_config_qual[0][0];

    fbe_test_run_test_on_rg_config(rg_config_p, NULL, meriadoc_brandybuck_test_rg_config,
                                   MERIADOC_BRANDYBUCK_LUNS_PER_RAID_GROUP,
                                   MERIADOC_BRANDYBUCK_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end meriadoc_brandybuck_test()
 ******************************************/

/*!**************************************************************
 * meriadoc_brandybuck_setup()
 ****************************************************************
 * @brief
 *  Setup for a map test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  6/6/2012 - Created. Rob Foley
 *
 ****************************************************************/
void meriadoc_brandybuck_setup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_test_rg_configuration_array_t *array_p = NULL;

        rg_config_p = &meriadoc_brandybuck_raid_group_config_qual[0][0];
        array_p = &meriadoc_brandybuck_raid_group_config_qual[0];

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_rg_config_array_with_extra_drives_load_physical_config(array_p);
        sep_config_load_sep_and_neit();
    }

    /* Initialize any required fields 
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end meriadoc_brandybuck_setup()
 **************************************/


/*!**************************************************************
 * meriadoc_brandybuck_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the meriadoc_brandybuck test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/6/2012 - Created. Rob Foley
 *
 ****************************************************************/

void meriadoc_brandybuck_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end meriadoc_brandybuck_cleanup()
 ******************************************/
/*************************
 * end file meriadoc_brandybuck_test.c
 *************************/


