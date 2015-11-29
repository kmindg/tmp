/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_test_generate.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for testing raid generate code.
 *
 * @author
 *   9/24/2008:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_raid_library_private.h"
#include "fbe_raid_library_test_proto.h"
#include "fbe_raid_library_test.h"
#include "fbe_raid_library_test_state_object.h"
#include "fbe_parity_test_private.h"
#include "fbe_raid_library.h"
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

/*************************
 *   IMPORTED DEFINITIONS
 *************************/

/************************* 
 * GLOBAL DEFINITIONS 
 *************************/

static fbe_raid_test_degraded_info_t fbe_parity_test_degraded_db;

/* These are the default settings of the degraded_db. 
 * Nothing is degraded in this case. 
 */
static fbe_raid_test_degraded_info_t fbe_raid_test_non_degraded_db = 
{
    FBE_RAID_TEST_OPTION_FLAG_NONE, /* No additional test options */
    /* Both rebuild positions and checkpoints are set to invalid
     * since nothing is degraded.  
     */
    FBE_RAID_INVALID_DEAD_POS,            /* dead_pos[0] */ 
    FBE_RAID_INVALID_DEAD_POS,            /* dead_pos[1] */
    FBE_RAID_INVALID_DEGRADED_OFFSET,     /* degraded_chkpt[0] */
    FBE_RAID_INVALID_DEGRADED_OFFSET,     /* degraded_chkpt[1] */
    NULL, /* No rebuild logging. */
    NULL, /* No rebuild logging. */
};

/* These are the settings of the degraded_db.
 * For a single degraded data drive case.
 *  
 * This is how the first stripe layout looks on R5/R3
 *  a '#' indicates a degraded position  
 * | 0  | 1  | 2# |
 * | P  | D1 | D0 | 
 *  
 * This is how the first stripe layout looks on R6: 
 *  a '#' indicates a degraded position  
 * | 0  | 1  | 2# | 3  | 
 * | RP | DP | D1 | D0 |
 */
static fbe_raid_nr_extent_t fbe_parity_test_single_degraded_data_extents[] =
{
    {0,        0x10,                   FBE_FALSE}, /* 0..0xf     clean*/
    {0x10,     FBE_U32_MAX - 0x10,     FBE_TRUE},  /* 0x10..end dirty*/
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0},
};
static fbe_raid_test_degraded_info_t fbe_parity_test_single_degraded_data_db = 
{ 
    FBE_RAID_TEST_OPTION_FLAG_NONE,       /* No additional test options */
    0x02,                                 /* dead_pos[0] */ 
    FBE_RAID_INVALID_DEAD_POS,            /* dead_pos[1] */ 
    0x10,                                 /* degraded_chkpt[0] */
    FBE_RAID_INVALID_DEGRADED_OFFSET,     /* degraded_chkpt[1] */
    fbe_parity_test_single_degraded_data_extents,
    NULL, /* No rebuild logging. */
};

/* These are the settings of the degraded_db.
 * For a single degraded data drive case with rebuild
 * logging enabled.
 *  
 * This is how the first stripe layout looks on R5/R3
 *  a '#' indicates a degraded position  
 * | 0  | 1  | 2# |
 * | P  | D1 | D0 | 
 *  
 * This is how the first stripe layout looks on R6: 
 *  a '#' indicates a degraded position  
 * | 0  | 1  | 2# | 3  | 
 * | RP | DP | D1 | D0 |
 */
static fbe_raid_nr_extent_t fbe_parity_test_single_degraded_rl_data_extents[] =
{
    {0,        0x10,     FBE_FALSE}, /* 0..0xf     clean*/
    {0x10,     0x10,     FBE_TRUE},  /* 0x10..0x1f dirty*/
    {0x20,     0x60,     FBE_FALSE}, /* 0x20.. 0x7F clean*/
    {0x80,     0x80,     FBE_TRUE},  /* 0x80.. 0xFF dirty*/
    {0x100,    0xFFFFFFF,FBE_TRUE},  /* 0x100.. 0xFFFFFFF clean */
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0},
};
static fbe_raid_test_degraded_info_t fbe_parity_test_single_degraded_rl_data_db = 
{ 
    FBE_RAID_TEST_OPTION_FLAG_NONE, /* Enable rebuild logging */
    0x02,                            /* dead_pos[0] */ 
    FBE_RAID_INVALID_DEAD_POS,       /* dead_pos[1] */ 
    0x10,                            /* degraded_chkpt[0] */
    FBE_RAID_INVALID_DEGRADED_OFFSET,/* degraded_chkpt[1] */
    fbe_parity_test_single_degraded_rl_data_extents,
    NULL, /* No rebuild logging. */
};

/* These are the settings of the degraded_db.
 * For a double degraded data drive case.
 *  
 * This is how the first stripe layout looks on R6: 
 *  a '#' indicates a degraded position  
 * | 0  | 1  | 2# | 3# | 
 * | RP | DP | D1 | D0 |
 */
static fbe_raid_nr_extent_t fbe_parity_test_double_degraded_data_extents[] =
{
    {0,        0x10,                   FBE_FALSE}, /* 0..0xf     clean*/
    {0x10,     FBE_U32_MAX - 0x10,     FBE_TRUE},  /* 0x10..end dirty*/
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0},
};
static fbe_raid_test_degraded_info_t CSX_MAYBE_UNUSED fbe_parity_test_double_degraded_data_db = 
{ 
    FBE_RAID_TEST_OPTION_FLAG_NONE, /* No additional test options */
    0x02,                   /* dead_pos[0] */ 
    0x03,                   /* dead_pos[1] */ 
    0x0000000000000010,     /* degraded_chkpt[0] */
    0x0000000000000010,     /* degraded_chkpt[1] */
    fbe_parity_test_double_degraded_data_extents,
    fbe_parity_test_double_degraded_data_extents
};

/* These are the settings of the degraded_db, for a single degraded parity drive
 * case. 
 * This is how the first stripe layout looks on R5/R3
 *  a '#' indicates a degraded position  
 * | 0# | 1  | 2  |
 * | P  | D1 | D0 | 
 *  
 * This is how the first stripe layout looks on R6: 
 *  a '#' indicates a degraded position  
 * | 0# | 1  | 2  | 3  | 
 * | RP | DP | D1 | D0 |
 */
static fbe_raid_nr_extent_t CSX_MAYBE_UNUSED fbe_parity_test_single_degraded_parity_extents[] =
{
    {0,        0x10,                   FBE_FALSE}, /* 0..0xf     clean*/
    {0x10,     FBE_U32_MAX - 0x10,     FBE_TRUE},  /* 0x10..end dirty*/
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0},
};
static fbe_raid_test_degraded_info_t fbe_parity_test_single_degraded_parity_db = 
{
    FBE_RAID_TEST_OPTION_FLAG_NONE, /* No additional test options */
    0x00,                       /* dead_pos[0] */  
    FBE_RAID_INVALID_DEAD_POS,  /* dead_pos[1] */  
    0x0000000000000010,         /* degraded_chkpt[0] */ 
    FBE_RAID_INVALID_DEGRADED_OFFSET,     /* degraded_chkpt[1] */
    NULL,                                 /* No rebuild logging */
    NULL, /* No rebuild logging. */
};

/* These are the settings of the degraded_db, for a double degraded parity drive
 * case. 
 * This is how the first stripe layout looks on R6: 
 *  a '#' indicates a degraded position  
 * | 0# | 1# | 2  | 3  | 
 * | RP | DP | D1 | D0 |
 */
static fbe_raid_nr_extent_t fbe_parity_test_double_degraded_parity_extents[] =
{
    {0,        0x10,                   FBE_FALSE}, /* 0..0xf     clean*/
    {0x10,     FBE_U32_MAX - 0x10,     FBE_TRUE},  /* 0x10..end dirty*/
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0},
};
static fbe_raid_test_degraded_info_t CSX_MAYBE_UNUSED fbe_parity_test_double_degraded_parity_db = 
{ 
    FBE_RAID_TEST_OPTION_FLAG_NONE, /* No additional test options */
    0x00,                   /* dead_pos[0] */  
    0x01,                   /* dead_pos[1] */  
    0x0000000000000010,     /* degraded_chkpt[0] */  
    0x0000000000000010,     /* degraded_chkpt[1] */
    fbe_parity_test_double_degraded_parity_extents,
    fbe_parity_test_double_degraded_parity_extents,
};

/* These are the settings of the degraded_db, for a degraded parity drive and
 * degraded data drive case. 
 * This is how the first stripe layout looks on R6: 
 *  a '#' indicates a degraded position  
 * | 0# | 1  | 2# | 3  | 
 * | RP | DP | D1 | D0 |
 */
static fbe_raid_nr_extent_t fbe_parity_test_degraded_parity_data_extents[] =
{
    {0,        0x10,                   FBE_FALSE}, /* 0..0xf     clean*/
    {0x10,     FBE_U32_MAX - 0x10,     FBE_TRUE},  /* 0x10..end dirty*/
    {FBE_RAID_GROUP_TEST_INVALID_FIELD, 0, 0},
};
static fbe_raid_test_degraded_info_t CSX_MAYBE_UNUSED fbe_parity_test_degraded_parity_data_db = 
{ 
    FBE_RAID_TEST_OPTION_FLAG_NONE, /* No additional test options */
    0x00,                   /* dead_pos[0] */
    0x02,                   /* dead_pos[1] */
    0x0000000000000010,     /* degraded_chkpt[0] */ 
    0x0000000000000010,     /* degraded_chkpt[1] */
    fbe_parity_test_degraded_parity_data_extents,
    fbe_parity_test_degraded_parity_data_extents
};

/* Dummy rebuild log table (i.e. no rebuild logging)
 */

/* Globals used for displaying progress in our loops. 
 */
fbe_u64_t fbe_raid_test_iteration_count = 0;
fbe_u64_t fbe_raid_test_last_display_seconds = 0;
fbe_u64_t fbe_raid_test_current_seconds;

/* Global used to define the offsets we wish to test within each element.
 */
static fbe_raid_test_element_offset_t fbe_parity_test_lba_offsets[] = {

    /* Start of element */
    {1, 0},

    /* Offsets from start of element */
    {1, 1},
    {1, 2},
    {1, 5},

    /* End of element */
    {0, 0},

    /* Offsets from end of element */
    {0, 1},
    {0, 2},
    {0, 5},

    {FBE_RAID_GROUP_TEST_INVALID_FIELD, FBE_RAID_GROUP_TEST_INVALID_FIELD}
};

/* This is a list of the cache page sizes in blocks we wish to test.
 */
static fbe_u16_t fbe_parity_test_cache_page_sizes[] = {
    //4,  /* 2k */
    32, /* 16k */
    (fbe_u16_t)FBE_RAID_GROUP_TEST_INVALID_FIELD
};

/* This is a list of the element sizes in blocks we wish to test.
 * As of Zeus we only support binding element sizes that are a 
 * multiple of 64, the optimal size for 520 and 512. 
 */
static fbe_u16_t fbe_parity_test_element_sizes[] = {
    64,
    128,
    (fbe_u16_t)FBE_RAID_GROUP_TEST_INVALID_FIELD
};

fbe_u32_t fbe_parity_count_nr_extents(fbe_u32_t pos)
{
    fbe_u32_t count = 0;
    fbe_raid_nr_extent_t *current_extent_p = NULL;

    if (fbe_parity_test_degraded_db.dead_pos[0] == pos)
    {
        current_extent_p = fbe_parity_test_degraded_db.nr_extent_p[0];
    }
    if (fbe_parity_test_degraded_db.dead_pos[1] == pos)
    {
        current_extent_p = fbe_parity_test_degraded_db.nr_extent_p[1];
    }

    if (current_extent_p != NULL)
    {
        while (current_extent_p->lba != FBE_RAID_GROUP_TEST_INVALID_FIELD)
        {
            count++;
            current_extent_p++;
        }
    }
    return count;
}

fbe_status_t fbe_parity_generate_test_find_end_nr_extent(fbe_u32_t pos,
                                                         fbe_lba_t lba,
                                                         fbe_block_count_t blocks,
                                                         fbe_block_count_t *blocks_remaining_p,
                                                         fbe_bool_t *b_dirty)
{
    fbe_raid_nr_extent_t *current_extent_p = NULL;

    if (fbe_parity_test_degraded_db.dead_pos[0] == pos)
    {
        current_extent_p = fbe_parity_test_degraded_db.nr_extent_p[0];
    }
    if (fbe_parity_test_degraded_db.dead_pos[1] == pos)
    {
        current_extent_p = fbe_parity_test_degraded_db.nr_extent_p[1];
    }

    /* We are not degraded if we did not find this position as one of our degraded
     * positions. 
     */
    if (current_extent_p == NULL)
    {
        *blocks_remaining_p = blocks;
        *b_dirty = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /* Find the extent that contains this lba.
     */
    while (current_extent_p->lba != FBE_RAID_INVALID_DEGRADED_OFFSET)
    {
        if ((lba >= current_extent_p->lba) && 
            (lba < (current_extent_p->lba + current_extent_p->blocks)))
        {
            /* Blocks remaining are either the same as the 
             * input blocks or the distance to the end of this region. 
             */
            *blocks_remaining_p = FBE_MIN(blocks,
                                                    (current_extent_p->blocks - (lba - current_extent_p->lba)));
            *b_dirty = current_extent_p->b_dirty;
            return FBE_STATUS_OK;
        }
        current_extent_p++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
fbe_status_t fbe_parity_generate_test_get_nr_extent(void *raid_group_p,
                                                    fbe_u32_t position,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t blocks,
                                                    fbe_raid_nr_extent_t *nr_extent_p,
                                                    fbe_u32_t nr_extents)
{
    fbe_lba_t current_lba = lba;
    fbe_block_count_t current_blocks = blocks;
    fbe_u32_t current_extent = 0;
    fbe_bool_t b_dirty;
    fbe_block_count_t blocks_remaining;

    fbe_zero_memory(nr_extent_p, (sizeof(fbe_raid_nr_extent_t) * nr_extents));

    /* Find this lba range in the nr extent.
     */
    while ((current_lba < (lba + blocks) && (current_extent < nr_extents)))
    {
        /* Get the end of this extent.  End is where a transition occurs.
         */
        fbe_parity_generate_test_find_end_nr_extent(position,
                                                    current_lba,
                                                    current_blocks,
                                                    &blocks_remaining,
                                                    &b_dirty);

        nr_extent_p->lba = current_lba;
        nr_extent_p->blocks = blocks_remaining;
        nr_extent_p->b_dirty = b_dirty;
        nr_extent_p++;
        current_extent++;
        current_blocks -= blocks_remaining;
        current_lba += blocks_remaining;
    }
    return FBE_STATUS_OK;
}


/****************************************************************
 * fbe_parity_test_setup_degraded_info()
 ****************************************************************
 * @brief
 *  Setup a set of degraded position fields in our
 *  global DB.
 *
 * @param none.
 *
 * @return
 *  None.             
 *
 * @author
 *  11/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_setup_degraded_info(fbe_raid_iots_t *iots_p)
{
    fbe_u32_t nr_extent_count = 0;
    fbe_raid_test_degraded_info_t *db_p = &fbe_parity_test_degraded_db;
    fbe_raid_position_bitmask_t bitmask = 0;

    /*! @note for now we assume we are still rebuild logging.  
     * Eventually we might want to pass in an indication if we are rebuild logging still.
     */
    if (db_p->dead_pos[0] != FBE_RAID_INVALID_DEAD_POS)
    {
        if (db_p->test_flags & FBE_RAID_TEST_OPTION_FLAG_RL_ENABLE)
        {
            bitmask |= (1 << db_p->dead_pos[0]);
        }
        nr_extent_count = fbe_parity_count_nr_extents(db_p->dead_pos[0]);
    }

    if (db_p->dead_pos[1] != FBE_RAID_INVALID_DEAD_POS)
    {
        if (db_p->test_flags & FBE_RAID_TEST_OPTION_FLAG_RL_ENABLE)
        {
            bitmask |= (1 << db_p->dead_pos[1]);
        }
        nr_extent_count = fbe_parity_count_nr_extents(db_p->dead_pos[1]);
    }

    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, bitmask);
    return;
}
/******************************************
 * end fbe_parity_test_setup_degraded_info()
 ******************************************/
/*!***************************************************************
 * fbe_parity_test_initialize_parity_object()
 ****************************************************************
 * @brief   Initialize the mirror object for use in a testing 
 *          environment that doesn't setup the entire fbe infrastrcture.
 *
 * @param   geometry_p - Pointer to raid geometry to populate
 * @param   block_edge_p - Pointer to block edge for this raid group
 * @param   width - The width of this raid group
 * @param   capacity - Exported capacity of this raid group
 *
 * @return   The status of the operation typically FBE_STATUS_OK.
 *
 ****************************************************************/
fbe_status_t fbe_parity_test_initialize_parity_object(fbe_raid_geometry_t *geometry_p,
                                                      fbe_block_edge_t *block_edge_p,
                                                      fbe_u32_t width,
                                                      fbe_lba_t capacity)
{
    fbe_raid_library_test_initialize_geometry(geometry_p,
                                              block_edge_p,
                                              FBE_RAID_GROUP_TYPE_RAID5,
                                              width, 
                                              FBE_RAID_SECTORS_PER_ELEMENT,
                                              FBE_RAID_ELEMENTS_PER_PARITY,
                                              capacity,
                                              (fbe_raid_common_state_t)fbe_parity_generate_start);
    return FBE_STATUS_OK;
}
/* end fbe_parity_test_initialize_parity_object() */

/****************************************************************
 * fbe_parity_test_setup_degraded_db()
 ****************************************************************
 * @brief
 *  Setup a set of degraded position fields in our
 *  global DB.
 *
 * @param new_db_p - ptr to new degraded db values.       
 *
 * @return
 *  None.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_setup_degraded_db(fbe_raid_test_degraded_info_t *new_db_p)
{
    /* Just setup our degraded db with the input values.
     */
    fbe_parity_test_degraded_db = *new_db_p;
    /* Setup the mock so that our mock is called to determine the nr ranges.
     */
    fbe_raid_group_set_nr_extent_mock(fbe_parity_generate_test_get_nr_extent);
    return;
}
/******************************************
 * end fbe_parity_test_setup_degraded_db()
 ******************************************/

/****************************************************************
 * fbe_parity_test_get_start_width()
 ****************************************************************
 * @brief
 *  Get the first supported width for this raid type.
 *
 * @param unit_type - The type to get the first supported width for. 
 *
 * @return
 *  The first supported width.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_parity_test_get_start_width(fbe_raid_group_type_t unit_type)
{
    fbe_u32_t width = 0;
    switch (unit_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID5:
            width = 3;
            break;
#if 0
        case LU_RAID6_DISK_ARRAY:
            width = 4;
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            width = 5;
            break;
#endif
        default:
            /* Unknown width.
             */
            break;
    };
    MUT_ASSERT_INT_NOT_EQUAL(width, 0);
    return width;
}
/******************************************
 * end fbe_parity_test_get_start_width()
 ******************************************/

/****************************************************************
 * fbe_parity_test_get_max_width()
 ****************************************************************
 * @brief
 *  Return the max supported width for this raid type.
 *
 * @param unit_type - The raid type to get max supported width for.             
 *
 * @return
 *  Max supported width.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_parity_test_get_max_width(fbe_raid_group_type_t unit_type) 
{
    fbe_u32_t width = 0;

    /* Just return the max supported width.
     */
    switch (unit_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID5:
        //case FBE_RAID_GROUP_TYPE_RAID6:
            width = FBE_RAID_MAX_DISK_ARRAY_WIDTH;
            break;
#if 0
        case FBE_RAID_GROUP_TYPE_RAID3:
            width = 9;
            break;
#endif
        default:
            /* Unknown width.
             */
            break;
    };
    MUT_ASSERT_INT_NOT_EQUAL(width, 0);
    return width;
}
/******************************************
 * end fbe_parity_test_get_max_width()
 ******************************************/

/****************************************************************
 * fbe_parity_test_get_next_width()
 ****************************************************************
 * @brief
 *  Return the next width from the given width of the given unit type.
 *  Note that it does not indicate that the next width is valid.
 *  Since we use this for loop control we need to return an invalid width in
 *  cases where we go beyond the max.
 *
 * @param unit_type - The raid type of this LUN. 
 * @param width - The width to get the next width for.
 *               So for example, if this is an R5 and the input
 *               width is 5, then we would return 6.
 *
 * @return
 *  Next supported width.  In cases where the next supported width is
 *  not valid we return FBE_RAID_GROUP_TEST_INVALID_FIELD.
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_parity_test_get_next_width(fbe_raid_group_type_t unit_type, fbe_u32_t width)
{
    /* We never expect a width that is invalid.
     * Striped mirrors must have an even multiple of disks. 
     */
    MUT_ASSERT_TRUE(width <= FBE_RAID_MAX_DISK_ARRAY_WIDTH);
#if 0
    MUT_ASSERT_TRUE(unit_type != LU_RAID10_DISK_ARRAY ||
                    (width % 2) == 0);
#endif
    if (width == FBE_RAID_MAX_DISK_ARRAY_WIDTH)
    {
        /* When we reach the end of a loop we expect this
         * function to get called with max. 
         */
        return FBE_RAID_GROUP_TEST_INVALID_FIELD;
    }
    /* Just iterate to the next supported width.
     */
    switch (unit_type)
    {
#if 0
        case LU_DISK_MIRROR:
            /* We only support a width of 2.
             * Return invalid since we do not have a next width.
             */
            MUT_ASSERT_INT_EQUAL(width, 2);
            width = FBE_RAID_GROUP_TEST_INVALID_FIELD;
            break;
        case LU_RAID10_DISK_ARRAY:
        case FBE_RAID_GROUP_TYPE_RAID6:
            /* R10/R6 only supports even widths.
             */
            width +=2;
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            /* Only 5 and 9 are supported.
             * We return 10 if width is 9 so that 
             * the caller will know we are beyond max. 
             */
            width = (width == 5) ? 9 : FBE_RAID_GROUP_TEST_INVALID_FIELD;
            break;
#endif
        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID5:
            width++;
            break;
        default:
            /* Unknown width.
             */
            width = 0;
            break;
    };
    MUT_ASSERT_INT_NOT_EQUAL(width, 0);

    return width;
}
/******************************************
 * end fbe_parity_test_get_next_width()
 ******************************************/

/****************************************************************
 * fbe_parity_test_get_offset()
 ****************************************************************
 * @brief
 *  This is an accessor method for the fbe_raid_test_element_offset_t,
 *  which allows us to represent offsets within an element.
 * 
 *  This method allows us to return an offset for a given
 *  element size.
 *
 * @param offset_p - This structure contains an offset and an 
 *                   indicator b_positive that tells us if this is an offset
 *                   from the beginning or the ending of the element.
 * 
 * @param sectors_per_element - The number of 520 byte blocks in the element
 *                             size.
 * 
 * @return
 *  The offset within an element to use for this given test case.
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_element_size_t fbe_parity_test_get_offset(fbe_raid_test_element_offset_t *offset_p,
                               fbe_element_size_t sectors_per_element)
{
    fbe_element_size_t offset;

    /* Our offset should not be larger than our element.
     */
    MUT_ASSERT_TRUE(offset_p->offset < sectors_per_element);

    if (offset_p->b_positive)
    {
        /* Positive offset is just the offset itself.
         */
        offset = offset_p->offset;
    }
    else
    {
        /* Otherwise we provide the offset from the end of the element.
         */
        offset = (sectors_per_element - offset_p->offset) - 1;
    }

    return offset;
}
/******************************************
 * end fbe_parity_test_get_offset()
 ******************************************/

/*!**************************************************************
 * fbe_parity_test_get_elements_per_parity()
 ****************************************************************
 * @brief
 *   Calculate the elements per parity for a given raid type.
 *
 * @param raid_type - Type of the raid group
 * @param sectors_per_element -
 * @param default_sectors_per_element
 * 
 * @return fbe_u32_t
 *
 * @author
 *  11/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_parity_test_get_elements_per_parity(fbe_raid_group_type_t raid_type,
                                                  fbe_element_size_t sectors_per_element,
                                                  fbe_element_size_t default_sectors_per_element)
{
    fbe_elements_per_parity_t elements_per_parity_stripe;

    /* Calculate elements_per_parity_stripe such that
     * sectors_per_element * elements_per_parity_stripe >= 128.
     */
    if (sectors_per_element < default_sectors_per_element)
    {
        elements_per_parity_stripe = 
            (default_sectors_per_element / sectors_per_element) +
            ((default_sectors_per_element % sectors_per_element) ? 1 : 0);
    }
    else
    {
        elements_per_parity_stripe = 1;
    }
#if 0
    if (raid_type == FBE_RAID_GROUP_TYPE_RAID3)
    {
       elements_per_parity_stripe = 
           capacity / ((fru_count - 1) * sectors_per_element );
    }
#endif
    return elements_per_parity_stripe;
}
/******************************************
 * end fbe_parity_test_get_elements_per_parity()
 ******************************************/
/****************************************************************
 * fbe_parity_test_setup_geometry_optimal_size()
 ****************************************************************
 * @brief
 *  Setup the raid group optimal size for test.
 *
 * @param ptimal_size - The number of blocks per optimal block.
 *
 * @return
 *  fbe_status_t           
 *
 * @author
 *  11/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_test_setup_geometry_optimal_size(fbe_raid_geometry_t *raid_geometry_p,
                                                         fbe_raid_group_type_t unit_type, 
                                                         fbe_u32_t optimal_size)
{
   /* Setup the optimal size in the raid group.
     */
    raid_geometry_p->optimal_block_size = optimal_size;
    raid_geometry_p->exported_block_size = 520;

    /* We currently support 64 (512) and 1 (520).
     */
    if (optimal_size == 64)
    {
        raid_geometry_p->imported_block_size = 512;
    }
    else
    {
        MUT_ASSERT_INT_EQUAL(optimal_size, 1);

        raid_geometry_p->imported_block_size = 520;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_test_setup_geometry_optimal_size()
 ******************************************/

/*!***************************************************************
 * fbe_parity_test_state_destroy()
 ****************************************************************
 * @brief
 *  This function destroys the test state object.
 *
 * @param state_p - State object to setup.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @version
 *  07/30/09 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_parity_test_state_destroy(fbe_raid_group_test_state_t * const state_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    if (status == FBE_STATUS_OK)
    {
        fbe_raid_group_test_state_destroy(state_p);
    }

    return status;
}
/* end fbe_parity_test_state_destroy() */

/****************************************************************
 * fbe_parity_test_generate_validate_siots_blocks()
 ****************************************************************
 * @brief
 *  Make sure the xfer_count in the siots is valid.
 *
 * @param siots_p - siots to validate.               
 *
 * @return
 *  None.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_parity_test_generate_validate_siots_blocks(fbe_raid_siots_t *siots_p)
{
    MUT_ASSERT_TRUE(siots_p->algorithm == R5_468 ||
                    siots_p->algorithm == R5_MR3 ||
                    siots_p->algorithm == R5_RCW ||
                    siots_p->algorithm == R5_RD ||
                    siots_p->algorithm == R5_SM_RD ||
                    siots_p->algorithm == RG_ZERO ||
                    siots_p->algorithm == R5_DEG_RD);
    return;
}
/******************************************
 * end fbe_parity_test_generate_validate_siots_blocks()
 ******************************************/

/****************************************************************
 * fbe_parity_test_generate_rl_validate_clean_zero()
 ****************************************************************
 * @brief
 *  For zero operations it is allowed to have the request span a
 *  CLEAN->DIRTY transition. 
 *
 *  start_pba       - Start pba of request to validate
 *  blocks          - Blocks for request to validate
 *  rl_start_pba    - Rebuild log start pba
 *  rl_blocks       - Rebuild blocks
 *  siots_p         - Pointer to siots for the zero request
 *  siots_dead2     - Second siots dead position               
 *
 * @return
 *  None.             
 *
 * @notes
 *
 * @author
 *  06/08/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
void fbe_parity_test_generate_rl_validate_clean_zero(fbe_lba_t start_pba,
                                               fbe_block_count_t start_blocks,
                                               fbe_lba_t rl_start_pba,
                                               fbe_block_count_t rl_blocks,
                                               fbe_raid_siots_t *siots_p)
{
    fbe_lba_t   end_pba, rl_end_pba;

    end_pba = start_pba + start_blocks - 1;
    rl_end_pba = rl_start_pba + rl_blocks - 1;

    /* Zero requests are allowed to span the rebuild logs if there is any 
     * CLEAN portion.  But the rebuild logs are only used if the entire request
     * is above the rebuild checkpoint.  It is also possible that the request 
     * is entirely out of the CLEAN region.  First check if it spans the CLEAN 
     * region at all.
     */
    if ( (start_pba >= fbe_parity_test_degraded_db.degraded_chkpt[0]) &&
        !((end_pba   < rl_start_pba) ||
          (start_pba > rl_end_pba)      )                           )
    {
        /* For zero requests we expect the FBE_RAID_SIOTS_FLAGREBUILD_LOG_CLEAN flag to be
         * set even for partially CLEAN requests. 
         */
        //MUT_ASSERT_TRUE(fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN));

        /* For an originating request of zero that generates a CLEAN
         * request (i.e. the paroled drive has returned and this area doesn't
         * contain any shed data), the dead postion should be invalid.
         */
        MUT_ASSERT_TRUE((siots_p->dead_pos   == FBE_RAID_INVALID_DEAD_POS) &&
                        (siots_p->dead_pos_2 == FBE_RAID_INVALID_DEAD_POS)    );
    }
    
    return;
}
/************************************************
 * end fbe_parity_test_generate_rl_validate_clean_zero()
 ************************************************/

/****************************************************************
 * fbe_parity_test_generate_rl_validate_clean()
 ****************************************************************
 * @brief
 *  Any degraded operation shouldn't span the degraded checkpoint
 *  or rebuild logging checkpoint.
 *
 *  start_pba       - Start pba of request to validate
 *  blocks          - Blocks for request to validate
 *  rl_start_pba    - Rebuild log start pba
 *  rl_blocks       - Rebuild blocks
 *  siots_dead1     - First siots dead position
 *  siots_dead2     - Second siots dead position               
 *
 * @return
 *  None.             
 *
 * @notes
 *
 * @author
 *  06/08/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
void fbe_parity_test_generate_rl_validate_clean(fbe_lba_t start_pba,
                                                fbe_block_count_t start_blocks,
                                                fbe_lba_t rl_start_pba,
                                                fbe_block_count_t rl_blocks,
                                                fbe_u32_t siots_dead1,
                                                fbe_u32_t siots_dead2)
{
    fbe_lba_t   end_pba, rl_end_pba;

    end_pba = start_pba + start_blocks - 1;
    rl_end_pba = rl_start_pba + rl_blocks - 1;

    /* It is ok to be entirely out of the clean region.
     * But if the request is in the clean region it must be entirely
     * in the region.
     */
    if (!(   (end_pba   < rl_start_pba)
          || (start_pba > rl_end_pba)   )  )
    {
        /* Request is in the clean region.  Validate that it is
         * entirely in th clean region.
         */
        MUT_ASSERT_TRUE(start_pba >= rl_start_pba);
        MUT_ASSERT_TRUE(end_pba <= rl_end_pba);
    }

    /* Validate that there isn't a dead position in the siots.
     */
    MUT_ASSERT_TRUE((siots_dead1 == FBE_RAID_INVALID_DEAD_POS) &&
                    (siots_dead2 == FBE_RAID_INVALID_DEAD_POS)    );
    return;

}
/******************************************
 * end fbe_parity_test_generate_rl_validate_clean()
 ******************************************/

 /****************************************************************
 * fbe_parity_test_generate_rl_validate_dirty_zero()
 ****************************************************************
 * @brief
 *  For zero operations it is allowed to have the request span a
 *  DIRTY->CLEAN transition.  In addition since the drive is now
 *  out of probation we must zero any CLEAN portions of the request.
 *  This is done by clearing the dead position in the siots.
 *
 *  start_pba       - Start pba of request to validate
 *  blocks          - Blocks for request to validate
 *  rl_start_pba    - Rebuild log start pba
 *  rl_blocks       - Rebuild blocks
 *  siots_p         - Pointer to siots for the zero request
 *  siots_dead2     - Second siots dead position               
 *
 * @return
 *  None.             
 *
 * @notes
 *
 * @author
 *  07/27/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
void fbe_parity_test_generate_rl_validate_dirty_zero(fbe_lba_t start_pba,
                                               fbe_block_count_t start_blocks,
                                               fbe_lba_t rl_start_pba,
                                               fbe_block_count_t rl_blocks,
                                               fbe_raid_siots_t *siots_p)
{
    fbe_lba_t   end_pba, rl_end_pba;

    end_pba = start_pba + start_blocks - 1;
    rl_end_pba = rl_start_pba + rl_blocks - 1;

    /* Zero requests are allowed to span the rebuild logs if there is any 
     * CLEAN portion.  But the rebuild logs are only used if the entire request
     * is above the rebuild checkpoint.  It is also possible that the request 
     * is entirely out of the DIRTY region.  First check if it spans the DIRTY 
     * region at all.
     */
    if ( (start_pba >= fbe_parity_test_degraded_db.degraded_chkpt[0]) &&
        !((end_pba   < rl_start_pba) ||
          (start_pba > rl_end_pba)      )                           )
    {
        /* The request at least spans the DIRTY->CLEAN region.
         * Now check if it is entirely in the DIRTY region.
         */
        if ((start_pba >= rl_start_pba) &&
            (end_pba   <= rl_end_pba)      )
        {
            /* Request is entirely in the DIRTY region.  Validate that the
             * FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN flag is cleared.
             */
            //MUT_ASSERT_FALSE(fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN));

            /* For operations that will pre-read data from the now paroled drive
             * the dead position will be cleared (so that we cna execute the pre-read).
             * Therefore switch on the algorithm to determine if the dead position
             * should be valid or not.
             */
            switch(siots_p->algorithm)
            {
                case R5_MR3:
                case R5_468:
                    /* Special case for algorithms that will allow the pre-read
                     * of the paroled drive.
                     */
                    MUT_ASSERT_TRUE((siots_p->dead_pos   == FBE_RAID_INVALID_DEAD_POS) &&
                                    (siots_p->dead_pos_2 == FBE_RAID_INVALID_DEAD_POS)    );
                    break;

                default:
                    /* For all other algoritms the dead position should be valid.
                     */
                    MUT_ASSERT_TRUE((siots_p->dead_pos   != FBE_RAID_INVALID_DEAD_POS) ||
                                    (siots_p->dead_pos_2 != FBE_RAID_INVALID_DEAD_POS)    );
                    break;
            }
        }
        else
        {
            /* The zero algorithm flags the fact that there is a CLEAN portion of
             * the request (so that the paroled drive is properly zeroed).  Therefore
             * that algorithm is handled differently.
             */
            if (siots_p->algorithm == RG_ZERO)
            {
                /* We need to zero the paroled drive for this region. The dead
                 * position should be cleared (so that the paroled drive is zeroed)
                 * and the FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN flag should be set.
                 */
                MUT_ASSERT_TRUE((siots_p->dead_pos   == FBE_RAID_INVALID_DEAD_POS) &&
                                (siots_p->dead_pos_2 == FBE_RAID_INVALID_DEAD_POS)    );
                //MUT_ASSERT_TRUE(fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN));
            }
            else
            {
                /* Else the request should be treated as a degraded write.  Therfore
                 * the dead position should be valid and the FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN should
                 * be cleared.
                 */
                MUT_ASSERT_TRUE((siots_p->dead_pos   != FBE_RAID_INVALID_DEAD_POS) ||
                                (siots_p->dead_pos_2 != FBE_RAID_INVALID_DEAD_POS)    );
               // MUT_ASSERT_FALSE(fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN));
            }
        } /* end if partialy DIRTY */
    } /* end if request spans rebuild logs */
    
    return;
}
/************************************************
 * end fbe_parity_test_generate_rl_validate_dirty_zero()
 ************************************************/

/****************************************************************
 * fbe_parity_test_generate_rl_validate_dirty()
 ****************************************************************
 * @brief
 *  Any degraded operation shouldn't span the degraded checkpoint
 *  or rebuild logging checkpoint.
 *
 * start_pba       - Start pba of request to validate 
 *  blocks          - Blocks for request to validate
 *  rl_start_pba    - Rebuild log start pba
 *  rl_blocks       - Rebuild blocks
 *  siots_dead1     - First siots dead position
 *  siots_dead2     - Second siots dead position  
 *  dead_pos        - Dead position that has rebuild logs                
 *
 * @return
 *  None.             
 *
 * @notes
 *
 * @author
 *  06/08/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
void fbe_parity_test_generate_rl_validate_dirty(fbe_lba_t start_pba,
                                          fbe_block_count_t start_blocks,
                                          fbe_lba_t rl_start_pba,
                                          fbe_block_count_t rl_blocks,
                                          fbe_u32_t siots_dead1,
                                          fbe_u32_t siots_dead2,
                                          fbe_u32_t dead_pos)

{
    fbe_lba_t   end_pba, rl_end_pba;

    end_pba = start_pba + start_blocks - 1;
    rl_end_pba = rl_start_pba + rl_blocks - 1;

    /* It is expected that the request be entirely in the dirty region.
     */
    if (!(   (end_pba   < rl_start_pba)
          || (start_pba > rl_end_pba)   )  )
    {
        /* Request is in the dirty region.  Validate that it is
         * entirely in the dirty region.
         */
        MUT_ASSERT_TRUE(start_pba >= rl_start_pba);
        MUT_ASSERT_TRUE(end_pba <= rl_end_pba);

        /* Validate that the dead position matches one of those
         * in the siots.
         */
        MUT_ASSERT_TRUE((siots_dead1 == dead_pos) ||
                        (siots_dead2 == dead_pos)    );

    }
    else
    {
        /* The request isn't in the dirty region, this
         * is wrong.
         */
        MUT_ASSERT_TRUE((start_pba >= rl_start_pba) && (end_pba <= rl_end_pba));
    }

    return;

}
/******************************************
 * end fbe_parity_test_generate_rl_validate_dirty()
 ******************************************/

/****************************************************************
 * fbe_parity_test_generate_r5_read_fru_extents()
 ****************************************************************
 * @brief
 *  To validate that a siots doesn't span a degraded region we need
 *  to covert from a siots request into an array of fruts requests.
 *  Unfortunately this routine is tightly couple to the RAID 
 *  algorithms and thus relies on the contents of the siots.
 *
 * @param siots_p - siots to check 
 * @param fru_extents - Pointer to fru extents to populate     
 *
 * @return
 *  None
 *  
 * @notes
 *  There may be a simplier way (instead of having a per algorithm)
 *  of determine the fru access range.
 *
 * @author
 *  06/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
void fbe_parity_test_generate_r5_read_extents(fbe_raid_siots_t *siots_p,
                                              fbe_raid_extent_t *fru_extents)
{
    fbe_u8_t *position = siots_p->geo.position;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u32_t data_pos;
    fbe_u32_t array_pos;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;

    /* Optimization for single fru request.
     */
    if (siots_p->data_disks == 1)
    {
        fbe_status_t status = FBE_STATUS_OK;
        /* If we are reading from a single drive, it is
         * a data drive.  Get the relative data position.
         */
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                    siots_p->parity_pos,
                                    fbe_raid_siots_get_width(siots_p),
                                    parity_drives,
                                    &data_pos);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* Our start pos should be our only position.
         */
        MUT_ASSERT_TRUE(siots_p->start_pos == position[data_pos]);

        /* Populate appropiate extent.
         */
        array_pos = position[data_pos];
        MUT_ASSERT_TRUE(fru_extents[array_pos].size == 0);
        fru_extents[array_pos].position = position[data_pos];
        fru_extents[array_pos].size = siots_p->xfer_count;
        fru_extents[array_pos].start_lba = logical_parity_start + siots_p->geo.start_offset_rel_parity_stripe;
    }
    else
    {
        fbe_lba_t phys_blkoff;    /* block offset where access starts */
        fbe_block_count_t phys_blkcnt;          /* number of blocks accessed */
        fbe_block_count_t parity_range_offset;  /* Offset from parity stripe to start of effected parity range. */
        fbe_status_t status = FBE_STATUS_OK;
        fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1],
                                   r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* block counts for pre-read */

        /* Calculate the read areas on each fru for this multi drive I/O.
         * r1_blkcnt and r2_blkcnt contain the areas we will NOT read.
         */
        status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                   &siots_p->geo,
                                   siots_p->xfer_count,
                                   fbe_raid_siots_get_blocks_per_element(siots_p),
                                   fbe_raid_siots_get_width(siots_p) - parity_drives,
                                   siots_p->parity_start,
                                   siots_p->parity_count,
                                   r1_blkcnt,
                                   r2_blkcnt);
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status,  
                                 "parity: bad status from fbe_raid_geometry_calc_preread_blocks");

        /***************************************************
         * Determine the offset from the parity stripe to
         * the parity range.
         *
         * The calc_preread_blocks function determines
         * the pre-reads relative to the parity range
         * (ie. siots->parity_start, siots->parity count)
         *
         * The geometry information is all
         * relative to the parity stripe.
         *
         * Thus to get the start lba on a particular drive.
         * We need to calculate the offset from the 
         * parity STRIPE to the parity RANGE.
         *
         * Fru's start_lba = geo.extent[fru].start_lba +
         *                   parity range offset +
         *                   r1_blkcnt[fru]
         *
         *   -------- -------- -------- <-- Parity stripe-- 
         *  |        |        |        |                 /|\     
         *  |        |        |        |                  |      
         *  |        |        |        |    pstripe_offset|
         *  |        |        |        |                  |      
         *  |        |        |        |                  |    
         *  |        |        |        |    pstart_lba   \|/
         *  |        |        |ioioioio|<-- Parity range start of this I/O
         *  |        |        |ioioioio|       /|\       
         *  |        |        |ioioioio|        |      
         *  |        |        |ioioioio|  parity range offset        
         *  |        |        |ioioioio|        |      
         *  |        |        |ioioioio|       \|/       
         *  |        |ioioioio|ioioioio|<-- FRU I/O starts              
         *  |        |ioioioio|ioioioio|    fru_lba          
         *  |        |        |        |
         *  |________|________|________| 
         ****************************************************/
        if ((siots_p->data_disks == 1) ||
            (siots_p->geo.start_index == (fbe_raid_siots_get_width(siots_p)- (parity_drives + 1))))
        {
            /* Parity range starts at first data range.
             */
            parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;
        }
        else
        {
            /* Parity range starts at previous element boundary from
             * the first data element.
             */
            parity_range_offset = (siots_p->geo.start_offset_rel_parity_stripe / fbe_raid_siots_get_blocks_per_element(siots_p)) *
                fbe_raid_siots_get_blocks_per_element(siots_p);
        }

        /* Loop through all data positions.
         * Fill in each fruts which has a nonzero blkcnt.
         */
        for (data_pos = 0;
             data_pos < fbe_raid_siots_get_width(siots_p) - parity_drives;
             data_pos++)
        {
            phys_blkcnt = siots_p->parity_count - r1_blkcnt[data_pos]- r2_blkcnt[data_pos];

            MUT_ASSERT_TRUE((r1_blkcnt[data_pos] + r2_blkcnt[data_pos]) <= siots_p->parity_count);
            MUT_ASSERT_TRUE(position[data_pos] != siots_p->parity_pos);

            if (phys_blkcnt > 0)
            {
                array_pos = position[data_pos];
                phys_blkoff = logical_parity_start + parity_range_offset + r1_blkcnt[data_pos];
                /* Populate the associated fru extent using the position.
                 */
                MUT_ASSERT_TRUE(fru_extents[array_pos].size == 0);
                fru_extents[array_pos].position = position[data_pos];
                fru_extents[array_pos].size = phys_blkcnt;
                fru_extents[array_pos].start_lba = phys_blkoff; 
            }
        }
    }

    /* The fru_extents array has been updated.  There is nothing
     * else to return.
     */
    return;

}
/*************************************************
 * fbe_parity_test_generate_r5_read_extents()
 *************************************************/

/****************************************************************
 * fbe_parity_test_generate_degraded_fru_extent()
 ****************************************************************
 * @brief
 *  Any degraded operation shouldn't span the degraded checkpoint
 *  or rebuild logging checkpoint.  This routine uses the RAID
 *  algorithms to determine the pbas accessed for the specified 
 *  dead_pos.
 *
 * @param siots_p - siots to check 
 * @param dead_pos - the dead position to check for
 * @param dead_extent_p - Pointer to extent to populate fru pba          
 *
 * @return
 *  hits_dead,[O] - FBE_TRUE, this I/O hits the dead position
 *                FBE_FALSE, This I/O doesn't hit the dead positon            
 *
 * @notes
 *
 * @author
 *  06/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_parity_test_generate_degraded_fru_extent(fbe_raid_siots_t *siots_p,
                                            fbe_u32_t dead_pos,
                                            fbe_raid_extent_t *dead_extent_p)
{
    fbe_bool_t    dead_valid;
    /* These are the per position extents.
     */
    fbe_raid_extent_t fru_extents[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /* Initialize the extents and validate the dead_pos.
     */
    memset((void *)&fru_extents[0], 0x00, (sizeof(fbe_raid_extent_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH));
    MUT_ASSERT_TRUE((fbe_s32_t)dead_pos < FBE_RAID_MAX_DISK_ARRAY_WIDTH); 

    /* Simply use the siots algorithm to determine which extent
     * generation routine to use.
     */
    switch(siots_p->algorithm)
    {
        case R5_RD:
        case R5_DEG_RD:
        case R5_SM_RD:
            /* Invoke the R5 read generation routine.
             */
            fbe_parity_test_generate_r5_read_extents(siots_p, &fru_extents[0]);
            break;

        case R5_MR3:
        case R5_468:
        case RG_ZERO:
            /* Current R5 zero uses the read extent generation.
             */
            fbe_parity_test_generate_r5_read_extents(siots_p, &fru_extents[0]);
            break;

        default:
            /*! @note Currently only the algorithms above are supported.
             */
            MUT_ASSERT_TRUE(siots_p->algorithm == R5_RD);
            break;
    }

    /* Now populate the passed extent.
     */
    dead_extent_p->position = fru_extents[dead_pos].position;
    dead_extent_p->size = fru_extents[dead_pos].size;
    dead_extent_p->start_lba = fru_extents[dead_pos].start_lba;

    /* If the size is greater than 0, then the dead_pos extent is
     * valid.
     */
    dead_valid = (fru_extents[dead_pos].size > 0) ? FBE_TRUE : FBE_FALSE;

    return(dead_valid);
}
/*********************************************
 * fbe_parity_test_generate_degraded_fru_extent()
 *********************************************/

/****************************************************************
 * fbe_parity_test_generate_validate_degraded_blocks()
 ****************************************************************
 * @brief
 *  Any degraded operation shouldn't span the degraded checkpoint
 *  or rebuild logging checkpoint.
 *
 * @param siots_p - siots to validate.               
 *
 * @return
 *  None.             
 *
 * @notes
 *
 * @author
 *  06/08/2009  Ron Proulx  - Created.
 *
 ****************************************************************/
void fbe_parity_test_generate_validate_degraded_blocks(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t dead_pos;
    fbe_raid_extent_t  dead_extent;

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    /*! @note we could Validate other degraded checkpoint overlap.
     */

    /* Notice that we don't trust the dead positon from the siots since
     * if there is a bug it will incorrectly clear the dead position.
     */
    dead_pos = (fbe_parity_test_degraded_db.dead_pos[0] != FBE_RAID_INVALID_DEAD_POS) ? fbe_parity_test_degraded_db.dead_pos[0] :
               fbe_parity_test_degraded_db.dead_pos[1];
    MUT_ASSERT_TRUE(dead_pos != FBE_RAID_INVALID_DEAD_POS);

    /* First check if the I/O hits the dead position and get the
     * fru extent.
     */
    if (fbe_parity_test_generate_degraded_fru_extent(siots_p, dead_pos, &dead_extent))
    {
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_bool_t b_rebuild_logging = FBE_FALSE;
        fbe_bool_t b_rebuild_logs_available = FBE_FALSE;
        fbe_raid_nr_extent_t nr_extent[FBE_RAID_TEST_MAX_NR_EXTENT];

        status = fbe_raid_iots_is_rebuild_logging(iots_p, dead_pos, &b_rebuild_logging);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_raid_iots_rebuild_logs_available(iots_p, dead_pos, &b_rebuild_logs_available);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        if (b_rebuild_logging)
        {
            /* If we are still logging then everything is degraded.
             */
            switch (opcode)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                    if ((siots_p->algorithm == R5_RD) || (siots_p->algorithm == R5_SM_RD))
                    {
                        /* We could check that the I/O does not hit the dead drive.
                         */
                        MUT_ASSERT_INT_EQUAL(siots_p->dead_pos, FBE_RAID_INVALID_DEAD_POS);
                        MUT_ASSERT_INT_EQUAL(siots_p->dead_pos_2, FBE_RAID_INVALID_DEAD_POS);
                    }
                    else
                    {
                        MUT_ASSERT_INT_EQUAL(siots_p->algorithm, R5_DEG_RD);
                    }
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
                    {
                        /* The I/O hits the dead drive, since this is a degraded state
                         * machine. 
                         */
                        //MUT_ASSERT_INT_EQUAL(siots_p->algorithm, R5_DEG_WR);
                        MUT_ASSERT_INT_NOT_EQUAL(siots_p->dead_pos, FBE_RAID_INVALID_DEAD_POS);
                        MUT_ASSERT_INT_EQUAL(siots_p->dead_pos_2, FBE_RAID_INVALID_DEAD_POS);
                    }
                    break;
                default:
                    break;
            };
        }
        else if (b_rebuild_logs_available)
        {
            /* If rebuild logs are available we will validate them against
             * the siots.
             */
            fbe_bool_t b_is_nr;
            fbe_block_count_t blocks_remaining;

            status = fbe_raid_iots_get_nr_extent(iots_p, dead_pos, dead_extent.start_lba, dead_extent.size,
                                                  &nr_extent[0], FBE_RAID_TEST_MAX_NR_EXTENT);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Now check how far the dirty region extends
             */
            MUT_ASSERT_TRUE(dead_extent.position == dead_pos);
            MUT_ASSERT_TRUE(dead_extent.size > 0);

            fbe_raid_extent_is_nr(dead_extent.start_lba, dead_extent.size, &nr_extent[0], FBE_RAID_TEST_MAX_NR_EXTENT, 
                                  &b_is_nr, &blocks_remaining);
            /* We expect the request to be either ALL dirty or ALL clean.
             */
            if (b_is_nr == FBE_FALSE)
            {
                /* Some algorithms allow the request to span the RL
                 * CLEAN->DIRTY region.  Thus switch on operation.
                 */
                switch (opcode)
                {
                    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                        /* Zero request can span the CLEAN->DIRTY region.
                         */
                        fbe_parity_test_generate_rl_validate_clean_zero(dead_extent.start_lba,
                                                                        dead_extent.size,     
                                                                        dead_extent.start_lba,   
                                                                        blocks_remaining,
                                                                        siots_p);
                        break;

                    default:
                        /* For all other request types validate it is ALL CLEAN.
                         */
                        fbe_parity_test_generate_rl_validate_clean(dead_extent.start_lba,
                                                                   dead_extent.size,
                                                                   dead_extent.start_lba,
                                                                   blocks_remaining,
                                                                   siots_p->dead_pos,
                                                                   siots_p->dead_pos_2);
                        break;
                }
            }
            else
            {
                /* Some algorithms allow the request to span the RL
                 * DIRTY->CLEAN region.  Thus switch on operation.
                 */
                switch (opcode)
                {
                    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                        /* Zero request can span the DIRTY->CLEAN region.
                         */
                        fbe_parity_test_generate_rl_validate_dirty_zero(dead_extent.start_lba,
                                                                        dead_extent.size,  
                                                                        dead_extent.start_lba,
                                                                        blocks_remaining,  
                                                                        siots_p);
                        break;

                    default:
                        /* For all other request types validate it is ALL DIRTY.
                         */
                        fbe_parity_test_generate_rl_validate_dirty(dead_extent.start_lba,
                                                                   dead_extent.size, 
                                                                   dead_extent.start_lba,
                                                                   blocks_remaining,
                                                                   siots_p->dead_pos,
                                                                   siots_p->dead_pos_2,
                                                                   dead_pos);
                        break;
                }
            }    /* else if DIRTY logs. */
        }    /* end if rebuild logs are available */
        else
        {
            /* Not degraded.
             */
            switch (opcode)
            {
                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                    if ((siots_p->algorithm == R5_RD) || (siots_p->algorithm == R5_SM_RD))
                    {
                        /* We could check that the I/O does not hit the dead drive.
                         */
                        MUT_ASSERT_INT_EQUAL(siots_p->dead_pos, FBE_RAID_INVALID_DEAD_POS);
                        MUT_ASSERT_INT_EQUAL(siots_p->dead_pos_2, FBE_RAID_INVALID_DEAD_POS);
                    }
                    break;

                case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
                    if ((siots_p->algorithm == R5_468) || (siots_p->algorithm == R5_MR3))
                    {
                        /* We check that the I/O does not hit the dead drive, since
                         * for R5/R3, these are non-degraded state machines. 
                         */
                        MUT_ASSERT_INT_EQUAL(siots_p->dead_pos, FBE_RAID_INVALID_DEAD_POS);
                        MUT_ASSERT_INT_EQUAL(siots_p->dead_pos_2, FBE_RAID_INVALID_DEAD_POS);
                    }
                    break;
                default:
                    break;
            };

        }
    }    /* end if I/O hits the dead position */

    return;
}
/***************************************************
 * end fbe_parity_test_generate_validate_degraded_blocks()
 ***************************************************/

/*!***************************************************************
 * fbe_parity_test_state_setup
 ****************************************************************
 * @brief
 *  This function sets up for a read state machine test.
 *
 * @param test_state_p - The test state object to use in the test.
 * @param task_p - The test task to use.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author:
 *  07/30/09 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_parity_test_state_setup(fbe_raid_group_test_state_t * const test_state_p,
                                         fbe_raid_group_test_state_machine_task_t *const task_p,
                                         fbe_payload_block_operation_opcode_t opcode)
{
    fbe_raid_siots_t *siots_p = NULL;

    /* Init the state object.
     */
    fbe_raid_test_state_setup(test_state_p, task_p, opcode);

    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);
    MUT_ASSERT_NOT_NULL_MSG( siots_p, "siots is NULL");

    /* Generate the read.
     */
#if 0
    fbe_raid_state_status_t raid_state_status = FBE_RAID_STATE_STATUS_INVALID;
    raid_state_status = fbe_parity_generate_start(siots_p);   
    MUT_ASSERT_INT_EQUAL_MSG(raid_state_status, FBE_RAID_STATE_STATUS_EXECUTING, 
                             "parity: bad status from generate");
    MUT_ASSERT_TRUE_MSG((siots_p->common.state == (fbe_raid_common_state_t)fbe_parity_read_state0), 
                        "parity: bad status from generate");
    MUT_ASSERT_TRUE_MSG((siots_p->algorithm == RAID0_RD), "parity: bad status from generate");
#endif
    return FBE_STATUS_OK;
}
/* end fbe_parity_test_state_setup() */


/****************************************************************
 * fbe_parity_test_generate_validate()
 ****************************************************************
 * @brief
 *  Generate an iorb for both the host and cached cases.
 *
 * @param test_state_p
 *
 * @return
 *  None.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_validate(fbe_raid_group_test_state_t * const test_state_p)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_block_count_t iots_blocks;

    iots_p = fbe_raid_group_test_state_get_iots(test_state_p);
    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);

    fbe_raid_iots_get_blocks(iots_p, &iots_blocks);

    /* Make sure our transfer count is OK. 
     */
    fbe_parity_test_generate_validate_siots_blocks(siots_p);

    /* If we are degraded make sure we don't span any checkpoints and validate
     * the degraded position is set properly.
     */
    if ( fbe_parity_test_degraded_db.dead_pos[0] != FBE_RAID_INVALID_DEAD_POS )
    {
        /* Validate that the siots doesn't span any checkpoints.
         */
        fbe_parity_test_generate_validate_degraded_blocks(siots_p);

        /* If the siots is still degraded validate the dead position.
         */
        if (siots_p->dead_pos != FBE_RAID_INVALID_DEAD_POS)
        {
            MUT_ASSERT_INT_EQUAL(fbe_parity_test_degraded_db.dead_pos[0], siots_p->dead_pos);
        }
    }
    if ( fbe_parity_test_degraded_db.dead_pos[1] != FBE_RAID_INVALID_DEAD_POS )
    {
        /* Validate that the siots doesn't span any checkpoints.
         */
        fbe_parity_test_generate_validate_degraded_blocks(siots_p);

        /* If the siots is still degraded validate the dead position.
         */
        if (siots_p->dead_pos_2 != FBE_RAID_INVALID_DEAD_POS)
        {
            MUT_ASSERT_INT_EQUAL(fbe_parity_test_degraded_db.dead_pos[1], siots_p->dead_pos_2);
        }
    }
    return;
}
/**************************************
 * end fbe_parity_test_generate_validate() 
 **************************************/

/*!**************************************************************
 * fbe_parity_test_generate_siots()
 ****************************************************************
 * @brief
 *  Generate all the siots for a given iots.
 *
 * @param param_p - Ptr to parameters for this I/O.
 * @param test_state_p - Ptr to our tests tate object.
 *
 * @return None. 
 *
 * @author
 *  11/4/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_siots(fbe_raid_test_generate_parameters_t * param_p, 
                                    fbe_raid_group_test_state_t *const test_state_p)
{
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_block_count_t iots_blocks;
    fbe_block_count_t total_blocks_transferred = 0;

    iots_p = fbe_raid_group_test_state_get_iots(test_state_p);
    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);

    fbe_raid_iots_get_blocks(iots_p, &iots_blocks);
    MUT_ASSERT_TRUE(iots_blocks == iots_p->blocks_remaining);
    MUT_ASSERT_TRUE(iots_p->blocks_remaining == param_p->blocks);

    fbe_parity_test_setup_degraded_info(iots_p);
    /* Generate the read.
     */
    while (iots_p->blocks_remaining != 0)
    {
        fbe_raid_state_status_t raid_state_status;
        raid_state_status = fbe_parity_generate_start(siots_p);   

        MUT_ASSERT_INT_EQUAL_MSG(raid_state_status, FBE_RAID_STATE_STATUS_EXECUTING, 
                                 "parity: bad status from generate");

        /* Make sure we generated properly.
         */
        fbe_parity_test_generate_validate(test_state_p);

        /* Keep track of total blocks we transferred and at the end make sure 
         * it matches the iorb. 
         */
        total_blocks_transferred += siots_p->xfer_count;
        MUT_ASSERT_TRUE(iots_p->blocks_remaining < param_p->blocks);
        MUT_ASSERT_TRUE(total_blocks_transferred <= param_p->blocks);

        /* Allow the siots to free resources.
         */
        fbe_raid_siots_destroy_resources(siots_p);
        fbe_raid_siots_destroy(siots_p);

        if (iots_p->blocks_remaining > 0)
        {
            /* If we still have more to do, re-initialize the siots for the 
             * next piece of this iots. 
             */
            fbe_raid_siots_embedded_init(siots_p, iots_p);
        }
    }
    /* Make sure all the siots generated.
     */
    MUT_ASSERT_UINT64_EQUAL(iots_blocks, total_blocks_transferred);
}
/******************************************
 * end fbe_parity_test_generate_siots()
 ******************************************/
/****************************************************************
 * fbe_parity_test_generate_iots()
 ****************************************************************
 * @brief
 *  Generate an iorb for both the host and cached cases.
 *
 * @param lba - logical block address. 
 * @param blocks - block count.
 * @param opcode - Opcode for this iorb (e.g. FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
 *
 * @return
 *  None.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_iots(fbe_raid_test_generate_parameters_t *param_p)
{
    fbe_raid_group_test_state_t * const test_state_p = fbe_raid_group_test_get_state_object();
    fbe_raid_group_test_state_machine_task_t  task;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_test_state_get_raid_geometry(test_state_p);
    fbe_u32_t max_bytes_per_sg_entry = FBE_U32_MAX;
    fbe_status_t status;
    fbe_block_size_t imported_size;
    fbe_block_size_t exported_size;

    /* Init the raid group.
     */
    fbe_parity_test_initialize_parity_object(raid_geometry_p,
                                             fbe_raid_group_test_state_get_block_edge(test_state_p), 
                                             param_p->width, 
                                             0xFFFFFFFF /* capacity */);

    /* Setup our raid group database.
     */
    status = fbe_parity_test_setup_geometry_optimal_size(raid_geometry_p, param_p->unit_type, param_p->optimal_size);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_raid_geometry_get_exported_size(raid_geometry_p, &exported_size);
    fbe_raid_geometry_get_imported_size(raid_geometry_p, &imported_size);

    MUT_ASSERT_TRUE(param_p->blocks > 0);

    fbe_raid_group_test_state_machine_task_init(&task,
                                                param_p->lba,
                                                param_p->blocks,
                                                520, /* exported size*/ 
                                                param_p->optimal_size,
                                                imported_size, 1, /* imported optimal */
                                                max_bytes_per_sg_entry,
                                                param_p->element_size,
                                                param_p->elements_per_parity,
                                                param_p->width,
                                                -1);
    /* Init the state object.
     * This also inits the iots. 
     */
    fbe_parity_test_state_setup(test_state_p, &task, param_p->opcode);

    fbe_raid_test_current_seconds = (fbe_u64_t)(fbe_get_time() / FBE_TIME_MILLISECONDS_PER_SECOND);
    if ((fbe_raid_test_current_seconds - fbe_raid_test_last_display_seconds) > FBE_RAID_TEST_DISPLAY_SECONDS)
    {
        fbe_raid_test_last_display_seconds = fbe_raid_test_current_seconds;
        mut_printf(MUT_LOG_LOW,
                   "%s: %016llu) page size: %d width: %d element_size: %d opt_size: %d lba: 0x%llx blocks: 0x%llx",
                   __FUNCTION__,
		   (unsigned long long)fbe_raid_test_iteration_count, 
                   param_p->cache_page_size, param_p->width,
		   param_p->element_size, param_p->optimal_size,
                   (unsigned long long)param_p->lba,
		   (unsigned long long)param_p->blocks);
    }

    /* Now generate all the siots for a given iots.
     */
    fbe_parity_test_generate_siots(param_p, test_state_p);

    /* Free all resources here.
     */
    fbe_parity_test_state_destroy(test_state_p);
    /* Test generate for the Write cached case.
     * Only test this case if it is a legal size the cache can 
     * issue. 
     */
    return;
}
/******************************************
 * end fbe_parity_test_generate_iots()
 ******************************************/

/****************************************************************
 * fbe_parity_test_generate_all_lba_and_block_combinations()
 ****************************************************************
 * @brief
 *  Generate all combinations of lba and block counts for this LUN.
 *

 * @param opcode - The opcode to test.        
 *
 * @return
 *  None.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_all_lba_and_block_combinations(fbe_raid_test_generate_parameters_t *param_p)
{
    fbe_lba_t lba;
    fbe_u32_t blocks;

    for (lba = 0; lba < ( ((fbe_lba_t)(param_p->width)) * FBE_RAID_SECTORS_PER_PARITY ); lba ++)
    {
        /* Determine the distance to the end of the next parity stripe. 
         * This is the block count we want to test. 
         */
        fbe_u32_t next_pstripe_end_blocks = (fbe_u32_t) ((param_p->width * FBE_RAID_SECTORS_PER_PARITY * 2) - lba);

        for (blocks = 1; blocks < next_pstripe_end_blocks; blocks ++)
        {
            /* Display progress every N seconds.
             */
            fbe_raid_test_current_seconds = (fbe_u64_t)(fbe_get_time() / FBE_TIME_MILLISECONDS_PER_SECOND);
            if ((fbe_raid_test_current_seconds - fbe_raid_test_last_display_seconds) > FBE_RAID_TEST_DISPLAY_SECONDS)
            {
                fbe_raid_test_last_display_seconds = fbe_raid_test_current_seconds;
            }
            param_p->lba = lba;
            param_p->blocks = blocks;
            fbe_parity_test_generate_iots(param_p);

            fbe_raid_test_iteration_count++;
        }    /* end for all block range */
    }    /* end for all lba range */
    return;
}
/******************************************
 * end fbe_parity_test_generate_all_lba_and_block_combinations()
 ******************************************/

/****************************************************************
 * fbe_parity_test_generate_select_cases()
 ****************************************************************
 * @brief
 *  Generate specific cases for LUN 0.  These specific cases will
 *  iterate over a range of LBAs between the elements of
 *  begin_element_start and begin_element_end.
 * 
 *  We also iterate over a range of block counts where our end
 *  element is within elements from ending_element_start to
 *  ending_element_end.
 *

 * @param begin_element_start - The element that contains the start
 *                              of the region where our lbas will start.
 * 
 * @param begin_element_end - The element that contains the end of the region
 *                           where our lbas will begin.
 * 
 * @param ending_element_start - The element contains the start of
 *                              region where our I/Os will end.
 * 
 * @param ending_element_end - The element contains the end of
 *                            the region where our I/Os will end.
 * 
 * @param opcode - Opcode for this case.
 * 
 * @return
 *  None.             
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_parity_test_generate_select_cases( fbe_raid_test_generate_parameters_t *param_p,
                                            fbe_lba_t begin_element_start, 
                                            fbe_lba_t begin_element_end,
                                            fbe_lba_t ending_element_start, 
                                            fbe_lba_t ending_element_end)
{
    fbe_u64_t current_element;
    fbe_element_size_t sectors_per_element = param_p->element_size;

    /* Loop over all the elements we wish to test start lbas in. 
     * For each of these elements we will generate a set of start LBAs. 
     */
    for (current_element = begin_element_start; 
          current_element < begin_element_end; 
          current_element++)
    {
        /* Next, loop over the offsets within this element we wish to test.
         * For each offset an lba will be generated. 
         */
        fbe_u32_t start_lba_offset_index;

        for (start_lba_offset_index = 0; 
              (fbe_parity_test_lba_offsets[start_lba_offset_index].offset != FBE_RAID_GROUP_TEST_INVALID_FIELD  &&
               fbe_parity_test_lba_offsets[start_lba_offset_index].offset < sectors_per_element);
              start_lba_offset_index++ )
        {
            fbe_u64_t current_end_element;
            fbe_lba_t current_lba = sectors_per_element * current_element;

            /* Calculate the lba using the offset.
             */
            current_lba += 
                fbe_parity_test_get_offset(&fbe_parity_test_lba_offsets[start_lba_offset_index],
                                     sectors_per_element);

            /* We next generate the block counts using the ending_element_start 
             * and ending_element_end as the loop boundaries. 
             *  
             * Note that we do a MAX on ending_element_start and 
             * current_element, which ensures that we always begin with an 
             * element that is greater than or equal to current_element. 
             */
            for (current_end_element = FBE_MAX(ending_element_start, current_element);
                  current_end_element < ending_element_end; 
                  current_end_element ++)
            {
                /* Next, loop over the offsets within the end element we wish to
                 * test. For each offset an end lba will be generated. 
                 */
                fbe_u32_t end_lba_offset_index;

                for (end_lba_offset_index = 0; 
                      (fbe_parity_test_lba_offsets[end_lba_offset_index].offset != FBE_RAID_GROUP_TEST_INVALID_FIELD &&
                       fbe_parity_test_lba_offsets[end_lba_offset_index].offset < sectors_per_element);
                      end_lba_offset_index++ )
                {
                    fbe_block_count_t current_block_count;
                    fbe_lba_t current_end_lba = sectors_per_element * current_end_element;

                    current_end_lba += 
                        fbe_parity_test_get_offset(&fbe_parity_test_lba_offsets[end_lba_offset_index],
                                             sectors_per_element);

                    /* Only test this case if our end lba is greater or equal to
                     * our start. 
                     */
                    if (current_end_lba >= current_lba)
                    {
                        /* Make sure our current_block_count is a fbe_u32_t.
                         */
                        MUT_ASSERT_TRUE(current_end_lba - current_lba + 1 < ((fbe_lba_t) -1));

                        /* Calculate the blocks given our start/end lbas.
                         */
                        current_block_count = current_end_lba - current_lba + 1;

                        /* Kick off the generate case with the lba and block 
                         * count. 
                         */
                        param_p->lba = current_lba;
                        param_p->blocks = current_block_count;
                        fbe_parity_test_generate_iots(param_p);
                    }

                    fbe_raid_test_iteration_count++;

                } /* end for all block offsets in end element */

            } /* end for all end elements */

        } /* end for all lba offsets in start element */

    }    /* end for all start elements */
    return;
}
/******************************************
 * end fbe_parity_test_generate_select_cases()
 ******************************************/
/*****************************************************************
 * fbe_parity_test_generate_case()
 *****************************************************************
 * @brief
 *   Test a single generate case for the given unit type.
 * 
 *  unit_type - The raid type to test.
 *  opcode - the operation, read, write, etc to generate.
 *  width - the number of drives in the lun.
 *  cache_page_size - The number of blocks per cache page.
 *  element_size - The number of blocks per element.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_case( fbe_raid_test_generate_parameters_t *param_p)
{
    fbe_raid_test_current_seconds = (fbe_u64_t)(fbe_get_time() / FBE_TIME_MILLISECONDS_PER_SECOND);

    if ((fbe_raid_test_current_seconds - fbe_raid_test_last_display_seconds) > FBE_RAID_TEST_DISPLAY_SECONDS)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_parity_test_generate: %016llu) page size: %d width: %d element_size: %d opt_size: %d",
                   (unsigned long long)fbe_raid_test_iteration_count,
		   param_p->cache_page_size, param_p->width,
		   param_p->element_size, param_p->optimal_size);
        fbe_raid_test_last_display_seconds = fbe_raid_test_current_seconds;
    }
    else
    {
        mut_printf(MUT_LOG_LOW, 
                   "fbe_parity_test_generate: %016llu) page size: %d width: %d element_size: %d opt_size: %d",
                   (unsigned long long)fbe_raid_test_iteration_count,
		   param_p->cache_page_size, param_p->width,
		   param_p->element_size, param_p->optimal_size);
    }

    /* Now decide which test to execute, either the all combinations 
     * test or the select combinations test. 
     */
    if (getenv("raid_test_all_combinations") == NULL)
    {
        /* Did not find the option test select cases only (default).
         */  
        fbe_elements_per_parity_t elements_per_parity_stripe; 
        fbe_u32_t parity_disks = 1;
        fbe_element_size_t elements_per_stripe;

        /*! @note we could add raid 6
         */

        elements_per_stripe = param_p->width - parity_disks;
        elements_per_parity_stripe = elements_per_stripe * param_p->elements_per_parity; 
#if 0
        /* RAID 3 LUNs have huge parity stripes, so we don't 
         * test an entire parity stripe, we cut it down to 
         * something reasonable. 
         */
        if (dba_unit_get_type(adm_handle, 0) == FBE_RAID_GROUP_TYPE_RAID3)
        {
            elements_per_parity_stripe = 8;
        }
#endif

        /* Only test other cases if the element size is not greater
         * than the standard one. 
         */
        if (param_p->element_size <= FBE_RAID_SECTORS_PER_ELEMENT)
        {
            fbe_u32_t rand_index;
            fbe_u32_t max_width = 5;

            /* We will first test the first two stripes. 
             */
            fbe_parity_test_generate_select_cases(param_p,
                                                  0,    /* Start element */ 
                                                  /* end of start I/O range */
                                                  elements_per_stripe * 2,
                                                  0,    /* end element */ 
                                                  /* end of end I/O range */
                                                  elements_per_stripe * 2);
            /* When our width is more than 5, we only test one of the below, since 
             * the number of combinations to test all the below is excessive. 
             */
            rand_index = rand() % 4;

            if ((param_p->width < max_width) || (rand_index == 0))
            {
                /* Then we will test I/Os that start in the first two 
                 * stipes and end in the first two stripes of the next 
                 * parity stripe. 
                 */ 
                fbe_parity_test_generate_select_cases(param_p,
                                                      0,    /* Start element */ 
                                                      /* end of start I/O range */
                                                      elements_per_stripe * 2,
                                                      /* end element */ 
                                                      elements_per_parity_stripe,
                                                      /* end of end I/O range */
                                                      elements_per_parity_stripe +
                                                      elements_per_stripe);
            }

            if ((param_p->width < max_width) || (rand_index == 1))
            {
                /* Then we will test I/Os that start in the first stripe
                 * and end in the first stripe of the third parity 
                 * stripe. 
                 */
                fbe_parity_test_generate_select_cases(param_p,
                                                      0,    /* Start element */ 
                                                      /* end of start I/O range */
                                                      elements_per_stripe,
                                                      /* end element */ 
                                                      elements_per_parity_stripe * 2,
                                                      /* end of end I/O range */
                                                      (elements_per_parity_stripe * 2) +
                                                      elements_per_stripe);
            }
            if ((param_p->width < max_width) || (rand_index == 2))
            {
                /* Then we will test I/Os that start in the first stripe
                 * and end in the first stripe of the third parity 
                 * stripe. 
                 */
                fbe_parity_test_generate_select_cases(param_p,
                                                      0,    /* Start element */ 
                                                      /* end of start I/O range */
                                                      elements_per_stripe,
                                                      /* end element */ 
                                                      elements_per_parity_stripe * 3,
                                                      /* end of end I/O range */
                                                      (elements_per_parity_stripe * 3) +
                                                      2);
            }
            if ((param_p->width < max_width) || (rand_index == 3))
            {
                /*
                 * Then we will test I/Os that start in the first two 
                 * stipes and end in the first two stripes of the next 
                 * parity stripe. 
                 */ 
                fbe_parity_test_generate_select_cases(param_p,
                                                      0,    /* Start element */ 
                                                      /* end of start I/O range */
                                                      elements_per_stripe,
                                                      /* end element */ 
                                                      elements_per_parity_stripe * 4,
                                                      /* end of end I/O range */
                                                      (elements_per_parity_stripe * 4) +
                                                      2);
            }
        }    /* end if element size less than max */
        else
        {
            /* Test a smaller number of cases for larger element
             * sizes. 
             */
            fbe_parity_test_generate_select_cases(param_p,
                                                  0,    /* Start element */ 
                                                  /* end of start I/O range */
                                                  elements_per_stripe,
                                                  0,    /* end element */ 
                                                  /* end of end I/O range */
                                                  elements_per_stripe);

        }
    }    /* end if only testing select combinations */
    else
    {
        /* Found the all combinations option, test all combinations 
         * of lba and block count. 
         * This obviously takes a long time, so this is not the 
         * default. 
         */
        fbe_parity_test_generate_all_lba_and_block_combinations(param_p);
    }    /* end if testing all combinations */
    return;
}
/******************************************
 * end fbe_parity_test_generate_case()
 ******************************************/
/*****************************************************************
 * fbe_parity_test_generate()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate(fbe_raid_group_type_t unit_type, fbe_u32_t opcode)
{
    fbe_u32_t width;
    fbe_u32_t cache_page_size_index;
    fbe_u32_t element_size_index;

    if (getenv("raid_test_all_combinations") != NULL)
    {
        mut_printf(MUT_LOG_HIGH, "%s testing all combinations is enabled",
                   __FUNCTION__);
    }

    fbe_raid_test_last_display_seconds = (fbe_u64_t)(fbe_get_time() / FBE_TIME_MILLISECONDS_PER_SECOND);

    for (cache_page_size_index = 0; 
          fbe_parity_test_cache_page_sizes[cache_page_size_index] != (fbe_u16_t)FBE_RAID_GROUP_TEST_INVALID_FIELD; 
          cache_page_size_index++)
    {
        /*! @note not used yet. Setup the page size.
         */
        fbe_u32_t cache_page_size = fbe_parity_test_cache_page_sizes[cache_page_size_index];

        /* We use a special type of test iterator to iterate over the width.
         */
        for (width = fbe_parity_test_get_start_width(unit_type); 
              width <= fbe_parity_test_get_max_width(unit_type); 
              width = fbe_parity_test_get_next_width(unit_type, width))
        {
            /* Next, iterate over all the element sizes we wish to test.
             */
            for (element_size_index = 0; 
                  fbe_parity_test_element_sizes[element_size_index] != (fbe_u16_t)FBE_RAID_GROUP_TEST_INVALID_FIELD; 
                  element_size_index++)
            {
                fbe_element_size_t element_size = fbe_parity_test_element_sizes[element_size_index];
                fbe_raid_test_generate_parameters_t params;

                /* We test for both the optimal size of 1 (520) and the optimal size of
                 * 64 (512).
                 */
                params.unit_type = unit_type;
                params.opcode = opcode; 
                params.width = width;
                params.cache_page_size = cache_page_size;
                params.element_size = element_size;
                params.elements_per_parity = fbe_parity_test_get_elements_per_parity(unit_type,
                                                                                     element_size,
                                                                                     FBE_RAID_SECTORS_PER_PARITY);
                params.optimal_size = 1; /* for 520 */
                fbe_parity_test_generate_case(&params);

                params.optimal_size = 64; /* for 512 */
                fbe_parity_test_generate_case(&params);
            }    /* end for all element sizes */
        }    /* end for all widths */
    }    /* end for all cache page sizes */
    return;
}
/******************************************
 * end fbe_parity_test_generate()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_r5_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_r5_write(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_raid_test_non_degraded_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_r5_write()
 ******************************************/
/*****************************************************************
 * fbe_parity_test_generate_r5_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_r5_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_raid_test_non_degraded_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_r5_read()
 ******************************************/

#if 0
/*****************************************************************
 * fbe_parity_test_generate_r3_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r3 write generate cases.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_r3_write(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_raid_test_non_degraded_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID3, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_r3_write()
 ******************************************/
/*****************************************************************
 * fbe_parity_test_generate_r3_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r3 read generate cases.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_r3_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_raid_test_non_degraded_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID3, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_r3_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_r6_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 write generate cases.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_r6_write(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_raid_test_non_degraded_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_r6_write()
 ******************************************/
/*****************************************************************
 * fbe_parity_test_generate_r6_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 read generate cases.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_r6_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_raid_test_non_degraded_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_r6_read()
 ******************************************/
#endif
/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r5_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r5_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_data_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_data_r5_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r5_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r5_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_data_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_data_r5_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r5_rl_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 read
 *   generate cases we wish to test and uses the rebuild logs to
 *   generate the requests.
 *
 * @return
 *  None.
 *
 * @author
 *  06/08/2009  Ron Proulx  - Created.
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r5_rl_read(void)
{
    /* Init to degraded with rebuild logging.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_rl_data_db);

    /* Enable rebuild log tracing.
     * Comment enabling of tracing for time being (DIMS 238189).
     *
     * FlareTracingEnable(FLARE_RAID_RL_TRACING);
     */

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);

    /* Now disable rebuild log tracing.
     * Comment disabling of tracing as it has not been enabled (DIMS 238189).
     *
     * FlareTracingDisable(FLARE_RAID_RL_TRACING);
     */

    return;
}
/*************************************************
 * end fbe_parity_test_generate_degraded_data_r5_rl_read()
 *************************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r5_rl_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 write
 *   generate cases we wish to test and uses the rebuild logs to
 *   generate the requests.
 *
 * @return
 *  None.
 *
 * @author
 *  06/08/2009  Ron Proulx  - Created.
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r5_rl_write(void)
{
    /* Init to degraded with rebuild logging.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_rl_data_db);

    /* Enable rebuild log tracing.
     * Comment enabling of tracing for time being (DIMS 238189).
     *
     * FlareTracingEnable(FLARE_RAID_RL_TRACING);
     */

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, 
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);

    /* Now disable rebuild log tracing.
     * Comment disabling of tracing as it has not been enabled (DIMS 238189).
     *
     * FlareTracingDisable(FLARE_RAID_RL_TRACING);
     */

    return;
}
/*************************************************
 * end fbe_parity_test_generate_degraded_data_r5_rl_write()
 *************************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r5_zero()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 zero
 *   generate cases we wish to test and uses the rebuild logs to
 *   generate the requests.
 *
 * @return
 *  None.
 *
 * @author
 *  07/21/2009  Ron Proulx  - Created.
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r5_rl_zero(void)
{
    /* Init to degraded with rebuild logging.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_rl_data_db);

    /* Enable rebuild log tracing.
     * Comment enabling of tracing for time being (DIMS 238189).
     *
     * FlareTracingEnable((FLARE_RAID_RL_TRACING | FLARE_RAID_TRAFFIC_TRACING));
     */

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO);

    /* Now disable rebuild log tracing 
     * Comment disabling of tracing as it has not been enabled (DIMS 238189).
     *
     * FlareTracingDisable((FLARE_RAID_RL_TRACING | FLARE_RAID_TRAFFIC_TRACING));
     */

    return;
}
/*************************************************
 * end fbe_parity_test_generate_degraded_data_r5_rl_zero()
 *************************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_r5_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_r5_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_parity_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_r5_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_r5_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r5 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_r5_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_parity_db);

    /* Simply kick off test generate with the raid 5 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_r5_read()
 ******************************************/
#if 0
/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r3_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r3 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r3_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_data_db);

    /* Simply kick off test generate with the raid 3 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_data_r3_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r3_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r3 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r3_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_data_db);

    /* Simply kick off test generate with the raid 3 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_data_r3_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_r3_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r3 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_r3_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_parity_db);

    /* Simply kick off test generate with the raid 3 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_r3_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_r3_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r3 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_r3_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_parity_db);

    /* Simply kick off test generate with the raid 3 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID5, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_r3_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r6_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r6_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_data_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_data_r6_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_data_r6_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_data_r6_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_data_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_data_r6_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_two_data_r6_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_two_data_r6_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_double_degraded_data_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_two_data_r6_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_two_data_r6_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_two_data_r6_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_double_degraded_data_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_two_data_r6_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_r6_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_r6_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_parity_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_r6_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_r6_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_r6_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_single_degraded_parity_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_r6_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_two_parity_r6_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_two_parity_r6_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_double_degraded_parity_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_two_parity_r6_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_two_parity_r6_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_two_parity_r6_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_double_degraded_parity_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_two_parity_r6_read()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_data_r6_write()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 write generate cases we 
 *   wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_data_r6_write(void)
{
    /* Init to singly degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_double_degraded_parity_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_data_r6_write()
 ******************************************/

/*****************************************************************
 * fbe_parity_test_generate_degraded_parity_data_r6_read()
 *****************************************************************
 * @brief
 *   This function iterates over the r6 read
 *   generate cases we wish to test.
 *
 * @return
 *  None.
 *
 * @author
 *  9/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_parity_test_generate_degraded_parity_data_r6_read(void)
{
    /* Init to not degraded.
     */
    fbe_parity_test_setup_degraded_db(&fbe_parity_test_double_degraded_parity_db);

    /* Simply kick off test generate with the raid 6 type. 
     */
    fbe_parity_test_generate(FBE_RAID_GROUP_TYPE_RAID6, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    return;
}
/******************************************
 * end fbe_parity_test_generate_degraded_parity_data_r6_read()
 ******************************************/
#endif
/*****************************************************************
 * fbe_parity_test_generate_add_tests()
 *****************************************************************
 * @brief
 *   This function acts as a test suite for unit test cases used
 *   to validate the implementation of functions in r5_generate.c.
 *
 * @param suite_p - suite to add tests to.
 *
 * @return
 *  None.
 *
 * @author
 *  9/29/2008 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_parity_test_generate_add_tests(mut_testsuite_t *suite_p)
{
    /* Normal mode cases.
     */
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_r5_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_r5_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
#if 0
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_r3_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_r3_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_r6_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_r6_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
#endif
    /* Degraded cases.
     */
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r5_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r5_rl_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_r5_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r5_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r5_rl_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_r5_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
#if 0
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r5_rl_zero, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r3_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_r3_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r3_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_r3_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r6_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_data_r6_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_r6_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_r6_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_two_data_r6_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_two_data_r6_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_two_parity_r6_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_two_parity_r6_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_data_r6_write, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, fbe_parity_test_generate_degraded_parity_data_r6_read, fbe_parity_unit_tests_setup, fbe_parity_unit_tests_teardown);
#endif
    return;
}
/******************************************
 * end fbe_parity_test_generate_add_tests()
 ******************************************/
/*************************
 * end file fbe_parity_test_generate.c
 *************************/
