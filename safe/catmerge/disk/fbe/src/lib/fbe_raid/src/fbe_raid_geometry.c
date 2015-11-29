/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_geometry.c
 ***************************************************************************
 *
 * @brief
 *  This file contains generic raid geometry functions.
 *
 * @version
 *   8/3/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_registry.h"

/*************************
 *   LOCAL GLOBALS
 *************************/
/*!*******************************************************************
 * @var fbe_raid_geometry_default_raid_library_debug_flags
 *********************************************************************
 * @brief These are the default library flags for all types of raid groups.
 *       
 *        To these flags we will add the below flags if it is either
 *        a user or a system rg.
 *
 *********************************************************************/
static fbe_raid_library_debug_flags_t fbe_raid_geometry_default_raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;

/*!*******************************************************************
 * @var fbe_raid_geometry_system_raid_library_debug_flags
 *********************************************************************
 * @brief These are the additional library debug flags that
 *        get set for a system raid group.
 *
 *********************************************************************/
static fbe_raid_library_debug_flags_t fbe_raid_geometry_system_raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;

/*!*******************************************************************
 * @var fbe_raid_geometry_user_raid_library_debug_flags
 *********************************************************************
 * @brief These are the additional library debug flags that
 *        get set for a user raid group.
 *
 *********************************************************************/
static fbe_raid_library_debug_flags_t fbe_raid_geometry_user_raid_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;

/*!*******************************************************************
 * @def FBE_RAID_LIBRARY_REG_KEY_USER_DEBUG_FLAG
 *********************************************************************
 * @brief Name of registry key where we keep the default debug flags
 *        for user raid groups.
 *
 *********************************************************************/
#define FBE_RAID_LIBRARY_REG_KEY_USER_DEBUG_FLAG "fbe_raid_library_user_debug_flag"

/*!*******************************************************************
 * @def FBE_RAID_LIBRARY_REG_KEY_SYSETEM_DEBUG_FLAG
 *********************************************************************
 * @brief Name of registry key where we keep the default debug flags
 *        for system raid groups.
 *
 *********************************************************************/
#define FBE_RAID_LIBRARY_REG_KEY_SYSTEM_DEBUG_FLAG "fbe_raid_library_system_debug_flag"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_raid_library_get_debug_flag_from_registry()
 ****************************************************************
 * @brief
 *  Read this library flag from the registry.
 *
 * @param flag_p - Flag to set in registry.
 * @param key_p - Name of key in registry to set.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 ****************************************************************/

static fbe_status_t fbe_raid_library_get_debug_flag_from_registry(fbe_raid_library_debug_flags_t *flag_p,
                                                                  fbe_u8_t* key_p)
{
    fbe_status_t status;
    fbe_u32_t flags;
    fbe_u32_t default_input_value = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;

    *flag_p = FBE_RAID_LIBRARY_DEBUG_FLAG_NONE;

    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               key_p,
                               &flags,
                               sizeof(fbe_u32_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &default_input_value,
                               sizeof(fbe_u32_t),
                               FALSE);

    if (status != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                 "raid library: successfully read %s from registry value: 0x%x\n", 
                                 key_p, flags); 
    *flag_p = (fbe_raid_library_debug_flags_t)flags;
    return status;
}
/******************************************
 * end fbe_raid_library_get_debug_flag_from_registry()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_write_debug_flag_to_registry()
 ****************************************************************
 * @brief
 *  Write this library flag to the registry.
 *
 * @param flag - Flag to set in registry.
 * @param key_p - Name of key in registry to set.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_write_debug_flag_to_registry(fbe_raid_library_debug_flags_t flag,
                                                           fbe_u8_t* key_p)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                                SEP_REG_PATH,
                                key_p,
                                FBE_REGISTRY_VALUE_DWORD,
                                &flag, 
                                sizeof(fbe_u32_t));

    if (status != FBE_STATUS_OK)
    {        
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                     "raid library: failed writing %s to registry debug flags: 0x%x\n", 
                                     key_p, flag);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    return status;
}
/******************************************
 * end fbe_raid_library_write_debug_flag_to_registry()
 ******************************************/
/*****************************************************************************
 *          fbe_raid_geometry_get_default_raid_library_debug_flags()
 *****************************************************************************
 *
 * @brief   Return the setting of the default raid library debug flags that
 *          will be used for all newly created raid groups.
 *
 * @return  debug_flags_p - Pointer to value of default raid library debug flags
 *
 *
 * @return   None (Always succeeds)
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void fbe_raid_geometry_get_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_raid_geometry_default_raid_library_debug_flags;
    return;
}
/* end of fbe_raid_geometry_get_default_raid_library_debug_flags() */

/*****************************************************************************
 *          fbe_raid_geometry_set_default_raid_library_debug_flags()
 *****************************************************************************
 *
 * @brief   Set the value to be used for the raid library debug flags for any
 *          newly created raid group.
 *
 * @param   new_default_raid_library_debug_flags - New raid library debug flags
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_geometry_set_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t new_default_raid_library_debug_flags)
{
    fbe_raid_geometry_default_raid_library_debug_flags = new_default_raid_library_debug_flags;
    return(FBE_STATUS_OK);
}
/* end of fbe_raid_geometry_set_default_raid_library_debug_flags() */
/*!**************************************************************
 * fbe_raid_geometry_get_default_object_library_flags()
 ****************************************************************
 * @brief
 *  Get the default raid library debug flags for a given object id.
 *  This allows us to differentiate between system and user objects.
 *
 * @param object_id
 * @param debug_flags_p
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_geometry_get_default_object_library_flags(fbe_object_id_t object_id,
                                                        fbe_raid_library_debug_flags_t *debug_flags_p)
{
    /* First get the default which applies to all types of objects (user and system). 
     */
    *debug_flags_p = fbe_raid_geometry_default_raid_library_debug_flags;

    /* Then add in the debug flag based upon the type of raid group.
     */
    if (object_id < FBE_RESERVED_OBJECT_IDS)
    {
        *debug_flags_p |= fbe_raid_geometry_system_raid_library_debug_flags;
    }
    else
    {
        *debug_flags_p |= fbe_raid_geometry_user_raid_library_debug_flags;
    }
    return;
}
/******************************************
 * end fbe_raid_geometry_get_default_object_library_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_set_default_system_library_flags()
 ****************************************************************
 * @brief
 *  Set the default raid library debug flags for system objects.
 *
 * @param debug_flags - New defaults for system objects.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_geometry_set_default_system_library_flags(fbe_raid_library_debug_flags_t debug_flags)
{
    fbe_raid_geometry_system_raid_library_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_geometry_set_default_system_library_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_set_default_user_library_flags()
 ****************************************************************
 * @brief
 *  Set the default raid library debug flags for user objects.
 *
 * @param debug_flags - New defaults for user objects.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_geometry_set_default_user_library_flags(fbe_raid_library_debug_flags_t debug_flags)
{
    fbe_raid_geometry_user_raid_library_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_geometry_set_default_user_library_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_persist_default_system_library_flags()
 ****************************************************************
 * @brief
 *  Set the default raid library debug flags for system objects.
 *
 *  Persist the new value in the registry.
 *
 * @param debug_flags - New defaults for system objects.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_geometry_persist_default_system_library_flags(fbe_raid_library_debug_flags_t debug_flags)
{
    fbe_raid_library_write_debug_flag_to_registry(debug_flags, FBE_RAID_LIBRARY_REG_KEY_SYSTEM_DEBUG_FLAG);
    fbe_raid_geometry_system_raid_library_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_geometry_persist_default_system_library_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_persist_default_user_library_flags()
 ****************************************************************
 * @brief
 *  Set the default raid library debug flags for user objects.
 *
 *  Persist the new value in the registry.
 *
 * @param debug_flags - New defaults for user objects.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_geometry_persist_default_user_library_flags(fbe_raid_library_debug_flags_t debug_flags)
{
    fbe_raid_library_write_debug_flag_to_registry(debug_flags, FBE_RAID_LIBRARY_REG_KEY_USER_DEBUG_FLAG);
    fbe_raid_geometry_user_raid_library_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_geometry_persist_default_user_library_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_get_default_system_library_flags()
 ****************************************************************
 * @brief
 *  Get the default raid library debug flags for system objects.
 *
 * @param debug_flags_p - Current defaults for system objects.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_geometry_get_default_system_library_flags(fbe_raid_library_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_raid_geometry_system_raid_library_debug_flags;
    return;
}
/******************************************
 * end fbe_raid_geometry_get_default_system_library_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_get_default_user_library_flags()
 ****************************************************************
 * @brief
 *  get the default raid library debug flags for user objects.
 *
 * @param debug_flags_p - Current defaults for user objects.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_geometry_get_default_user_library_flags(fbe_raid_library_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_raid_geometry_user_raid_library_debug_flags;
    return;
}
/******************************************
 * end fbe_raid_geometry_get_default_user_library_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_load_default_registry_values()
 ****************************************************************
 * @brief
 *  Load the default values for our default library flags from the registry.
 *
 * @param None              
 *
 * @return None 
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_geometry_load_default_registry_values(void)
{
    fbe_status_t status;
    fbe_raid_library_debug_flags_t debug_flags;

    status = fbe_raid_library_get_debug_flag_from_registry(&debug_flags, FBE_RAID_LIBRARY_REG_KEY_USER_DEBUG_FLAG);
    if (status == FBE_STATUS_OK) 
    { 
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                     "raid geo load def reg values found user library flags: 0x%x\n", debug_flags);
        fbe_raid_geometry_set_default_user_library_flags(debug_flags);
    }
    status = fbe_raid_library_get_debug_flag_from_registry(&debug_flags, FBE_RAID_LIBRARY_REG_KEY_SYSTEM_DEBUG_FLAG);
    if (status == FBE_STATUS_OK) 
    { 
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                     "raid geo load def reg values found system library flags: 0x%x\n", debug_flags);
        fbe_raid_geometry_set_default_system_library_flags(debug_flags);
    }
}
/******************************************
 * end fbe_raid_geometry_load_default_registry_values()
 ******************************************/
/***************************************************************
 * fbe_raid_get_stripe_range()
 ***************************************************************
 * @brief
 *   Determine the stripe range for a given unit's I/O.
 *   The extent structure is passed back.
 *
 * @param lba - host lba of I/O
 * @param blks_accessed - blocks of I/O
 * @param blks_per_element - units blks_per_elem
 * @param data_disks - data drives in unit
 * @param max_extents - total allowed extents (max of 2)
 * @param range - extent ranges.
 *
 * @return
 *  none
 *
 * @notes
 *
 * @author
 *  21-Mar-00 Rob Foley - Created.
 *
 ****************************************************************/

void fbe_raid_get_stripe_range(fbe_lba_t lba,
                               fbe_block_count_t blks_accessed,
                               fbe_element_size_t blks_per_element,
                               fbe_u16_t data_disks,
                               fbe_u16_t max_extents,
                               fbe_raid_extent_t range[])
{
    fbe_block_count_t blks_per_stripe;
    fbe_lba_t parity_start;
    fbe_block_count_t parity_count;

    blks_per_stripe = ((fbe_block_count_t)data_disks) * blks_per_element;

    if (max_extents > 1)
    {
        range[1].start_lba =
            range[1].size = 0;
    }

    /* The physical offset as already been added (from the edge) into the
     * requested lba before it is received by raid.
     */
    parity_start = blks_per_element * (lba / blks_per_stripe);
    parity_count = blks_per_element;

    /*
     * Calculate the region(s) to lock. Start at first stripe.
     */

    if (blks_accessed <= blks_per_element - (lba % blks_per_element))
    {
        /*
         * Case 1: Access begins and ends within
         * a single stripe element.
         *
         * +----+  +----+  +----+  +----+  +----+
         * |    |  |    |  |    |  |    |  |    |
         * |xxxx|  |    |  |    |  |    |  |pppp|
         * |    |  |    |  |    |  |    |  |    |
         * +----+  +----+  +----+  +----+  +----+
         */

        parity_start += lba % blks_per_element;
        parity_count = blks_accessed;
    }

    else if (blks_per_element > blks_per_stripe - (lba % blks_per_stripe))
    {
        /*
         * Case 2: Access begins within the last
         * stripe element and continues
         * into the next stripe.
         *
         * +----+  +----+  +----+  +----+  +----+
         * |    |  |    |  |    |  |    |  |    |
         * |    |  |    |  |    |  |    |  |    |
         * |    |  |    |  |    |  |xxxx|  |pppp|
         * +----+  +----+  +----+  +----+  +----+
         * +----+  +----+  +----+  +----+  +----+
         * |xxxx|  |    |  |    |  |    |  |pppp|
         * |????|  |    |  |    |  |    |  |????|
         * |    |  |    |  |    |  |    |  |    |
         * +----+  +----+  +----+  +----+  +----+
         *
         * Lock only the blocks which are accessed.
         */

        parity_start += lba % blks_per_element;
        parity_count -= lba % blks_per_element;
    }

    else if ((blks_accessed < blks_per_element) && (max_extents > 1))
    {
        /*
         * Case 3: Access ends in the stripe element
         * following that in which it begins,
         * but not enough blocks are accessed
         * to 'overlap'.
         *
         * +----+  +----+  +----+  +----+  +----+
         * |    |  |    |  |xxxx|  |    |  |pppp|
         * |    |  |    |  |    |  |    |  |    |
         * |    |  |xxxx|  |    |  |    |  |pppp|
         * +----+  +----+  +----+  +----+  +----+
         *
         * This is the special case where we will
         * specify two (2) distinct regions, if the
         * caller permits it.
         */

        range[1].start_lba = parity_start + (lba % blks_per_element);
        range[1].size = parity_count - (lba % blks_per_element);
        parity_count = (blks_accessed + lba) % blks_per_element;
    }

    else
    {
        /*
         * Case 4: Access in the first stripe is large
         * enough to 'overlap' the starting
         * point. (May be entire stripe!)
         *
         * +----+  +----+  +----+  +----+  +----+
         * |    |  |????|  |xxxx|  |    |  |pppp|
         * |    |  |xxxx|  |????|  |    |  |pppp|
         * |    |  |xxxx|  |    |  |    |  |pppp|
         * +----+  +----+  +----+  +----+  +----+
         *
         * Lock is applied across entire stripe.
         */
    }

    blks_accessed -= FBE_MIN(blks_accessed, blks_per_stripe -
                                                  (lba % blks_per_stripe));

    /*
     * Look for entire stripes which are accessed.
     */

    {
        /*
         * Case 5: Entire stripe is accessed.
         *
         * +----+  +----+  +----+  +----+  +----+
         * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |pppp|
         * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |pppp|
         * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |pppp|
         * +----+  +----+  +----+  +----+  +----+
         *
         * Lock is applied across entire stripe.
         */

        parity_count += (blks_accessed / blks_per_stripe) * blks_per_element;
        blks_accessed %= blks_per_stripe;
    }

    /*
     * Finally, check final stripe. If access extends beyond the first
     * element, the entire stripe should be locked; otherwise, only as
     * many sectors should be locked as will be accessed.
     */

    {
        /*
         * Access has crossed a stripe boundary to
         * end in the first element of a stripe.
         *
         * +----+  +----+  +----+  +----+  +----+
         * |    |  |    |  |    |  |    |  |    |
         * |    |  |    |  |    |  |????|  |????|
         * |    |  |    |  |    |  |xxxx|  |pppp|
         * +----+  +----+  +----+  +----+  +----+
         * +----+  +----+  +----+  +----+  +----+
         * |xxxx|  |    |  |    |  |    |  |pppp|
         * |    |  |    |  |    |  |    |  |    |
         * |    |  |    |  |    |  |    |  |    |
         * +----+  +----+  +----+  +----+  +----+
         */

        parity_count += FBE_MIN(blks_accessed, blks_per_element);
    }

    range[0].size = parity_count,
        range[0].start_lba = parity_start;

    return;
}
/*****************************************
 * end fbe_raid_get_stripe_range
 *****************************************/

/***************************************************************
 * getLunStripeRange()
 ***************************************************************
 * @brief
 *  Returns the stripe range for a given I/O
 *
 * @param element_size - blocks per element
 * @param data_disks - number of disks with data
 * @param lba - host address
 * @param blocks - total blocks
 * @param extent - extent of stripe accessed.
 * @param flags - valid flags are
 *                  LU_RAID_EXPANSION and LU_RAID_USE_PHYSICAL
 *
 * @return
 *  none
 *
 * @notes
 *
 * @author
 *  21-Mar-00 Rob Foley - Created.
 *  16_Aug-00 RG - Added mirror support.
 *  10-Oct-00 kls - Touched to add expansion support.
 *
 ****************************************************************/

void getLunStripeRange(fbe_element_size_t element_size,
                       fbe_u32_t data_disks,
                       fbe_lba_t lba,
                       fbe_u32_t blocks,
                       fbe_raid_extent_t extent[],
                       fbe_u16_t flags)
{
    /* Pass in our data disks.
     */
    //array_width = dba_unit_get_num_data_disks(unit);

    fbe_raid_get_stripe_range(lba,
                   blocks,
                   element_size,
                   data_disks,
                   2,
                   extent);
    return;

}
/*****************************************
 * end getLunStripeRange
 *****************************************/

/*!**************************************************************************
 *      fbe_raid_geometry_calc_preread_blocks()
 ***************************************************************************
 *
 * @brief
 *  This function determines the number of blocks which
 *  must be pre-read to fill out an MR3-style write.
 *
 *
 *  lda                 - [I]   LUN address where access begins.
 *  geo                 - [I]   Pointer to siots geometry
 *  blks_to_write       - [I]   number of blocks to be written.
 *  blks_per_element    - [I]   number of sectors per stripe element.
 *  data_disks          - [I]   number of data disks in the array.
 *  parity_start        - [I]   offset to start of affected parity (does
 *                              not contain physical offset).
 *  parity_count        - [I]   number of sectors of affected parity.
 *
 *  read1_blkcnt        - [O]   counts of sectors pre-read and re-written
 *                              in first stripe, one count per data disk.
 *  read2_blkcnt        - [O]   counts of sectors pre-read and re-written
 *                              in final stripe, one count per data disk.
 *
 * @return
 *  fbe status.
 *
 * @author
 *  27-Apr-00: Rob Foley Created.
 *  15-Aug-00: BJP Modified r5_mr3_calc_preread blocks to use geometry
 *                 instead of lba.  Included asserts to make sure the new
 *                 stuff is the same as the old stuff.
 *
 **************************************************************************/

fbe_status_t fbe_raid_geometry_calc_preread_blocks(fbe_lba_t lda,
                                           fbe_raid_siots_geometry_t *geo,
                                           fbe_block_count_t blks_to_write,
                                           fbe_u16_t blks_per_element,
                                           fbe_u16_t data_disks,
                                           fbe_lba_t parity_start,
                                           fbe_block_count_t parity_count,
                                           fbe_block_count_t read1_blkcnt[],
                                           fbe_block_count_t read2_blkcnt[])
{
    fbe_block_count_t write_bound,
      read_offset,
      read_limit,
      blks_read;
    fbe_status_t status = FBE_STATUS_OK;
#ifdef RG_DEBUG
    fbe_block_count_t sum = (data_disks * parity_count);
#endif /*NDEBUG */

    fbe_u16_t data_pos;
    fbe_u16_t blks_per_stripe;
    fbe_block_count_t start_write_offset;
    fbe_block_count_t blks_before_write;

    blks_per_stripe = blks_per_element * data_disks;

    /*
     * Initially, no pre-read data is expected; the
     * data for each disk is expected to be supplied
     * entirely by the primary data source, i.e. the
     * write cache or the host (via the FED).
     */

    for (data_pos = data_disks; 0 < data_pos--;)
    {
        read1_blkcnt[data_pos] =
            read2_blkcnt[data_pos] = 0;
    }

    if(geo->blocks_remaining_in_data > FBE_U32_MAX)
    {
        /* we do not expect this to cross 32bit limit here
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid_geom_calc_prerd_blks geo->blocks_remaining_in_data (0x%llx) > FBE_U32_MAX\n",
                               (unsigned long long)geo->blocks_remaining_in_data);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Calculate the number of blocks in the element below the starting lba
     */
    start_write_offset = blks_per_element - geo->blocks_remaining_in_data;

    /*
     * Determine if a pre-read is required
     * to back-fill into the first stripe.
     */
    if(RAID_COND((geo->start_index != 0 || start_write_offset != 0) !=
                                            ((lda % blks_per_stripe) != 0) ) )
    {
        /*Split trace to two lines*/
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid_geom_calc_prerd_blks status: (geo->start_index 0x%x!=0||start_wt_offset 0x%x!=0)>\n",
                               (unsigned int)geo->start_index, (unsigned int)start_write_offset);
		fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid_geom_calc_prerd_blks !=((lda 0x%llx %% blks_per_stripe 0x%x )!= 0))<\n",
                               lda, blks_per_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (geo->start_index != 0 || start_write_offset != 0)
    {
        /* Determine how many sectors must be read from
         * each disk to acquire the missing information
         * necessary to rebuild parity.
         */
        read_offset = parity_start % blks_per_element;
        read_limit = FBE_MIN(parity_count, blks_per_element - read_offset);

        write_bound = start_write_offset;
        if(RAID_COND(write_bound != lda % blks_per_element) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid_geom_calc_prerd_blks stat: wt_bound 0x%x!=(lda 0x%x %% blks_per_elem 0x%x)\n",
                                   (unsigned int)write_bound, (unsigned int)lda, (unsigned int)blks_per_element);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*
         * Read only what won't be overwritten.
         */
        if(RAID_COND( (write_bound < read_offset) ||
                      (write_bound >= read_offset + read_limit) ) )
        {
            /*Split trace to two lines*/
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid_geom_calc_prerd_blks status: write_bound 0x%llx < read_offset 0x%llx) or>\n",
                                   (unsigned long long)write_bound,
                                   (unsigned long long)read_offset);
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid_geom_calc_prerd_blks write_bound 0x%llx >= read_offset 0x%llx + read_limit 0x%llx)<\n",
                                   (unsigned long long)write_bound,
                                   (unsigned long long)read_offset,
                                   (unsigned long long)read_limit);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        blks_read = write_bound - read_offset;
        data_pos = geo->start_index;
        if(RAID_COND(data_pos != (fbe_u32_t)((lda / blks_per_element) % data_disks)) )
        {
            /*Split trace to two lines*/
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                   "raid_geom_calc_prerd_blks status: data_pos 0x%x !=\n",
                   data_pos);
			fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                   "raid_geom_calc_prerd_blks (fbe_u32_t)(lda 0x%x %% blks_per_element 0x%x))%%data disk 0x%x\n",
                   (unsigned int)lda, (unsigned int)blks_per_element, (unsigned int)data_disks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        do
        {
            /* Move these sectors from the primary sector
             * count to the pre-read sector count.
             */

#ifdef RG_DEBUG
            sum -= blks_read,
#endif /*NDEBUG */

                if(blks_read > FBE_U32_MAX)
                {
                    /* we do not expect this to cross 32bit limit here
                     */
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                              "raid_geom_calc_prerd_blks blks_read(0x%llx) > FBE_U32_MAX\n",
                             (unsigned long long)blks_read);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                read1_blkcnt[data_pos] += blks_read;

            /*
             * Move on to the next stripe element.
             */
            blks_read = read_limit;
        }
        while (0 < data_pos--);
    }

    /* Determine if a pre-read is required
     * to back-fill into the final stripe.
     */
    lda += blks_to_write;

    /* Calculate the number of blocks in the stripe before the starting lba
     */
    blks_before_write = ((fbe_u64_t)(geo->start_index) * (fbe_u64_t)(blks_per_element)) +
                                           start_write_offset;

    write_bound = ((blks_before_write + blks_to_write) % blks_per_element);
    if(RAID_COND(write_bound != (lda % blks_per_element)) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                            "raid_geom_calc_prerd_blks stat: wt_bound 0x%x!=lda 0x%x%%blks_per_elem 0x%x\n",
                            (unsigned int)write_bound, (unsigned int)lda, (unsigned int)blks_per_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if((((blks_before_write + blks_to_write) / blks_per_element) % data_disks) > FBE_U32_MAX)
    {
        /* we do not expect this to cross 32bit limit here
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                  "%s data position > FBE_U32_MAX\n",
                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    data_pos = (fbe_u16_t)(((blks_before_write + blks_to_write) / blks_per_element) %
                                                              data_disks);
    if(RAID_COND( (data_pos != (lda / blks_per_element) % data_disks) ||
                  ((data_pos != 0 || (write_bound != 0)) != 
                                            ((lda % blks_per_stripe) != 0)) ) )
    {
        /*split trace to three lines*/
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid_geom_calc_prerd_blks status: (data_pos 0x%x != (lda 0x%x / blks_per_element 0x%x )\n",
                               (unsigned int)data_pos, (unsigned int)lda, (unsigned int)blks_per_element);
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid_geom_calc_prerd_blks %% data_disks 0x%x) OR"
                               "(data_pos 0x%x != 0 || (write_bound 0x%x != 0))\n",
                               (unsigned int)data_disks, (unsigned int)data_pos, (unsigned int)write_bound);
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid_geom_calc_prerd_blks != ((lda 0x%x %% blks_per_stripe 0x%x ) != 0)))\n",
                               (unsigned int)lda, (unsigned int)blks_per_stripe);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (data_pos != 0 || (write_bound != 0))
    {
        /* Determine how many sectors must be read from
         * each disk to acquire the missing information
         * necessary to rebuild parity.
         */

        read_limit = (parity_start + parity_count) % blks_per_element;

        if (0 == read_limit)
        {
            read_limit = blks_per_element;
        }

        read_offset = (read_limit > parity_count)
            ? (read_limit - parity_count) : 0;

        /* Read only what won't be overwritten.
         */
        if (0 == write_bound)
        {
            write_bound = read_offset;
        }

        if(RAID_COND( (write_bound < read_offset) ||
                      (write_bound > read_limit)) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "raid_geom_calc_prerd_blks status: write_bound 0x%llx < read_offset 0x%llx)>\n",
                                (unsigned long long)write_bound,
				(unsigned long long)read_offset);
	    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "raid_geom_calc_prerd_blks or write_bound 0x%llx > read_limit 0x%llx)<\n",
                                (unsigned long long)write_bound,
				(unsigned long long)read_limit);
        }

        blks_read = read_limit - write_bound;

        read_limit -= read_offset;


        do
        {
            /* Move these sectors from the primary sector
             * count to the pre-read sector count.
             */

#ifdef RG_DEBUG
            sum -= blks_read,
#endif /*NDEBUG */
            if(blks_read > FBE_U32_MAX)
            {
                /* we do not expect this to cross 32bit limit here
                 */
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "%s blks_read(0x%llx) > FBE_U32_MAX\n",
				       __FUNCTION__, (unsigned long long)blks_read);
                return FBE_STATUS_GENERIC_FAILURE;
            }
                read2_blkcnt[data_pos] +=blks_read;

            /* Move on to the next stripe element.
             */

            blks_read = read_limit;
        }
        while (++data_pos < data_disks);
    }
#ifdef RG_DEBUG
    if(RAID_COND(sum != blks_to_write))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                  "%s status: sum 0x%x != blks_to_write 0x%x\n",
                __FUNCTION__, sum, blks_to_write);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif
    return status;
}
/**************************************
 * end fbe_raid_geometry_calc_preread_blocks
 **************************************/

/****************************************************************
 *  fbe_raid_map_pba_to_lba_relative()
 ****************************************************************
 * @brief
 *  This function maps a physical disk address to a parity
 *  relative address.
 *
 * @param fru_lba - physical address on this fru 
 *                        (lba relative to fru)
 * @param dpos - data position
 * @param ppos - parity position
 * @param pstart_lba - physical address where parity starts 
 *                         (lba relative to fru)
 * @param width - width of array
 * @param extent - extent array
 * @param pstripe_offset - offset from parity stripe
 * @param parities - Number of parity drives.
 * @param logical_parity_start_p -  The logical address of parity..
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  3/21/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_map_pba_to_lba_relative(fbe_lba_t fru_lba,
                                                   fbe_u32_t dpos,
                                                   fbe_u32_t ppos,
                                                   fbe_lba_t pstart_lba,
                                                   fbe_u16_t width,
                                                   fbe_lba_t *logical_parity_start_p,
                                                   fbe_lba_t pstripe_offset,
                                                   fbe_u16_t parities)
{
    fbe_status_t status = FBE_STATUS_OK;
    /***************************************************
     * Map a fru's lba from an lba on one fru
     * to an lba relative to parity.
     *
     * In the below diagram we calculate:
     *
     *    return value = pstart_lba + (parity range offset)
     *
     *    where parity_range_offset = (fru's pstripe offset - pstripe_offset)
     *
     *   -------- -------- -------- <-- Parity stripe-- 
     *  |        |  /|\   |        |                 /|\     
     *  |        |   |    |        |                  |      
     *  |        |   |    |        |    pstripe_offset|
     *  |        |   |    |        |                  |      
     *  |        |   |    |        |                  |    
     *  |        |   |    |        |    pstart_lba   \|/
     *  |        | fru's  |ioioioio|<-- Parity range of this I/O
     *  |        | psripe |ioioioio|       /|\       
     *  |        | offset |ioioioio|        |      
     *  |        |   |    |ioioioio|  parity range offset        
     *  |        |   |    |ioioioio|        |      
     *  |        |  \|/   |ioioioio|       \|/       
     *  |        |ioioioio|ioioioio|<-- FRU I/O starts              
     *  |        |ioioioio|ioioioio|    fru_lba          
     *  |        |        |        |
     *  |________|________|________| 
     ****************************************************/
    if(RAID_COND((fru_lba - *logical_parity_start_p) < pstripe_offset) ||
           ((pstart_lba + (fru_lba - *logical_parity_start_p)- pstripe_offset) < pstart_lba))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "(fru_lba 0x%llx - *logical_parity_start_p )x%llx) < pstripe_offset 0x%llx\n Or\n",
                               (unsigned long long)fru_lba,
			       (unsigned long long)(*logical_parity_start_p),
			       (unsigned long long)pstripe_offset);
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "(pstart_lba %llx + (fru_lba 0x%llx - *logical_parity_start_p %llx )"
                               "- pstripe_offset %llx) < pstart_lba %llx)\n",
                               (unsigned long long)pstart_lba,
			       (unsigned long long)fru_lba,
			       (unsigned long long)(*logical_parity_start_p),
			       (unsigned long long)pstripe_offset,
			       (unsigned long long)pstart_lba);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *logical_parity_start_p =  (pstart_lba + ((fru_lba - *logical_parity_start_p) - pstripe_offset));
    return status;
}                               /* fbe_raid_map_pba_to_lba_relative */


/****************************************************************
 *  fbe_raid_map_pba_to_lba()
 ****************************************************************
 * @brief
 *  This function maps a physical disk address to a parity
 *  relative address.
 *  This differs from fbe_raid_map_pba_to_lba_relative in that
 *  this function takes a different parameter set.
 *
 * @param fru_lba - physical address on this fru 
 *                        (lba relative to fru)
 * @param dpos - data position
 * @param ppos - parity position
 * @param width - width of array
 * @param pstart_lba - physical address where parity starts 
 *                         (lba relative to fru)
 * @param extent - extent array
 * @param parities - Number of parity drives.
 * @param logical_parity_start_p - logical address of parity
 * @return
 *  fbe_status_t
 *
 * @author
 *  3/21/00 - Created. Rob Foley
 *
 ****************************************************************/

static __forceinline fbe_status_t fbe_raid_map_pba_to_lba(fbe_lba_t fru_lba,
                                          fbe_u32_t dpos,
                                          fbe_u32_t ppos,
                                          fbe_u16_t width,
                                          fbe_lba_t pstart_lba,
                                          fbe_lba_t *logical_parity_start_p,
                                          fbe_u16_t parities)
{
    fbe_status_t status = FBE_STATUS_OK;
    /***************************************************
     * Map a fru's lba from an lba on one fru
     * to an lba relative to parity.
     *
     * In the below diagram we calculate:
     *
     *    return value = pstart_lba + fru's pstripe offset
     *
     *    where fru's pstripe offset = fru_lba - fru's physical offset
     * 
     *   -------- -------- -------- <-- unit start 
     *  |        |  /|\   |        |   
     *  |        |   |    |        |  
     *  |        |   |    |        |
     *  |        |fru's   |        | 
     *  |        |physical|        |
     *  |        |offset  |        |
     *  |        |   |    |        | 
     *  |        |  \|/   |        |
     *   -------- -------- -------- <--  Parity Stripe
     *  |        |  /|\   |        |     pstart_lba
     *  |        |   |    |        |  
     *  |        |   |    |        |
     *  |        |   |    |        |
     *  |        |   |    |        |
     *  |        |   |    |        |
     *  |        | fru's  |ioioioio|
     *  |        | psripe |ioioioio|
     *  |        | offset |ioioioio| 
     *  |        |   |    |ioioioio|
     *  |        |   |    |ioioioio| 
     *  |        |  \|/   |ioioioio| 
     *  |        |ioioioio|ioioioio|<-- FRU I/O starts              
     *  |        |ioioioio|ioioioio|    fru_lba          
     *  |        |        |        |
     *  |________|________|________| 
     ****************************************************/
    if(RAID_COND(fru_lba < *logical_parity_start_p))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "(fru_lba 0x%llx < *logical_parity_start_p 0x%llx)\n",
                               (unsigned long long)fru_lba,
			       (unsigned long long)(*logical_parity_start_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *logical_parity_start_p = (pstart_lba + 
            (fru_lba - *logical_parity_start_p));
    return status;
}                               /* fbe_raid_map_pba_to_lba */


/****************************************************************
 *  fbe_raid_get_lba_parity_range_offset()
 ****************************************************************
 * @brief
 *  This function determines the offset of this lba from
 *  the parity stripe.
 *
 * @param fru_lba - physical address on this fru 
 *                        (lba relative to fru)
 * @param dpos - data position
 * @param ppos - parity position
 * @param pstart_lba - physical address where parity starts 
 *                         (lba relative to fru)
 * @param width - width of array
 * @param extent - extent array
 * @param pstripe_offset_p - offset from parity stripe
 * @param parities - Number of parity drives.
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  3/21/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_get_lba_parity_range_offset(fbe_lba_t fru_lba,
                                       fbe_u32_t dpos,
                                       fbe_u32_t ppos,
                                       fbe_lba_t pstart_lba,
                                       fbe_u16_t width,
                                       fbe_lba_t logical_parity_start,
                                       fbe_lba_t *pstripe_offset_p,
                                     fbe_u16_t parities)
{
    fbe_status_t status = FBE_STATUS_OK;
    /***************************************************
     * Determine offset of this fru's lba from
     * the current parity range, NOT the current parity stripe.
     *
     * In the below diagram we calculate:
     *
     *    return value = (fru's pstripe offset - pstripe_offset) 
     *
     *   -------- -------- -------- <-- Parity stripe-- 
     *  |        |  /|\   |        |                 /|\     
     *  |        |   |    |        |                  |      
     *  |        |   |    |        |    pstripe_offset|
     *  |        |   |    |        |                  |      
     *  |        |   |    |        |                  |    
     *  |        |   |    |        |    pstart_lba   \|/
     *  |        | fru's  |ioioioio|<-- Parity range of this I/O
     *  |        | psripe |ioioioio|       /|\       
     *  |        | offset |ioioioio|        |      
     *  |        |   |    |ioioioio|  parity range offset        
     *  |        |   |    |ioioioio|        |      
     *  |        |  \|/   |ioioioio|       \|/       
     *  |        |ioioioio|ioioioio|<-- FRU I/O starts              
     *  |        |ioioioio|ioioioio|    fru_lba          
     *  |        |        |        |
     *  |________|________|________| 
     ****************************************************/

    if(RAID_COND((fru_lba - logical_parity_start) < *pstripe_offset_p) ||
        ((pstart_lba + (fru_lba - logical_parity_start)- *pstripe_offset_p) < pstart_lba) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                     "(fru_lba 0x%llx - logical_parity_start )x%llx) < *pstripe_offset_p 0x%llx\n Or",
                     (unsigned long long)fru_lba,
		     (unsigned long long)logical_parity_start,
		     (unsigned long long)(*pstripe_offset_p));
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                     "(pstart_lba %llx + (fru_lba 0x%llx - logical_parity_start %llx )- *pstripe_offset_p %llx) < pstart_lba %llx)\n",
                     (unsigned long long)pstart_lba,
		     (unsigned long long)fru_lba,
		     (unsigned long long)logical_parity_start,
		     (unsigned long long)(*pstripe_offset_p),
		     (unsigned long long)pstart_lba);
        return FBE_STATUS_INVALID;
    }

    *pstripe_offset_p = ((fru_lba - logical_parity_start) - *pstripe_offset_p);
    return status;
}                               /* fbe_raid_get_lba_parity_range_offset() */

/****************************************************************
 *  fbe_raid_map_lba_to_pstripe_offset()
 ****************************************************************
 * @brief
 *  This function maps the parity relative lba into 
 *  an offset from the parity stripe.
 *

 * @param lba - physical address on this fru 
 *                    (lba relative to fru)
 * @param pstart_lba - physical address where parity starts 
 *                    (lba relative to fru)
 * @param pstripe_offset_p - offset from parity stripe
 *
 * @return
 *  The logical offset from parity.
 *
 * @author
 *  3/21/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_lba_t fbe_raid_map_lba_to_pstripe_offset(fbe_lba_t lba,
                                             fbe_lba_t pstart_lba,
                                             fbe_lba_t *pstripe_offset_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    /***************************************************
     * Determine offset of this fru's lba from
     * the current parity range.
     * The lba passed in is in the
     * same units as pstart_lba.
     *
     *   -------- -------- -------- <-- pstart_lba ------
     *  |        |        |        |                  /|\
     *  |        |        |        |                   |
     *  |        |        |        |                   |
     *  |        |        |        |                   |
     *  |        |        |        |                   |
     *  |        |        |ioioioio|                   |
     *  |        |        |ioioioio|                   |
     *  |        |        |ioioioio|determine this offset
     *  |        |        |ioioioio|                   |
     *  |        |        |ioioioio|                   |
     *  |        |        |ioioioio|                  \|/
     *  |        |ioioioio|ioioioio|<-- FRU I/O starts (lba)              
     *  |        |ioioioio|ioioioio|                             
     *  |        |        |        |
     *  |________|________|________| 
     ****************************************************/
    if(RAID_COND(lba < pstart_lba) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                     "lba 0x%llx < pstart_lba 0x%llx\n",
                     (unsigned long long)lba, (unsigned long long)pstart_lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pstripe_offset_p =  (lba - pstart_lba);
    return status;
}                               /* fbe_raid_map_lba_to_pstripe_offset() */

/*!***************************************************************
 * fbe_raid_geometry_init()
 ****************************************************************
 *
 * @brief This function initializes the raid geometry structure.
 *
 * @param   raid_geometry_p - Pointer to raid geometry structure
 * @param   object_id - The object identifier of the object that
 *                      this geometry is for (used for debug)
 * @param   class_id - The class identifier for the object
 * @param   block_edge_p - Pointer to block edge for object
 * @param generate_state - The state to use to kick off new siots.
 *
 * @return  The status of the operation typically FBE_STATUS_OK.
 *
 * @note    Assumes that caller has taken lock so that update is atomic.
 *
 * @author
 *  10/28/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_init(fbe_raid_geometry_t *raid_geometry_p,
                                    fbe_object_id_t object_id,
                                    fbe_class_id_t class_id,
                                    fbe_metadata_element_t *metadata_element_p,
                                    fbe_block_edge_t *block_edge_p,
                                    fbe_raid_common_state_t generate_state)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_library_debug_flags_t  debug_flags;

    /*! If the geometry pointer is NULL then something is wrong
     */
    if (raid_geometry_p == NULL)
    {
        /* Trace some information and return failure
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: raid geometry pointer is NULL \n");
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Set the object and class id since they are known at initialization
     * time.
     */
    fbe_raid_geometry_set_object_id(raid_geometry_p, object_id);
    fbe_raid_geometry_set_class_id(raid_geometry_p, class_id);
    fbe_raid_geometry_set_metadata_element(raid_geometry_p, metadata_element_p);
    
    /* Initialize the following to default values.
     */
    fbe_raid_geometry_init_flags(raid_geometry_p);
    fbe_raid_geometry_get_default_object_library_flags(object_id, &debug_flags);
    fbe_raid_geometry_init_debug_flags(raid_geometry_p, debug_flags);
    fbe_raid_geometry_init_raid_type(raid_geometry_p);
    fbe_raid_geometry_init_attributes(raid_geometry_p);
    fbe_raid_geometry_set_mirror_prefered_position(raid_geometry_p, FBE_MIRROR_PREFERED_POSITION_INVALID);

    /* Set the raid library generate state.
     */
    fbe_raid_geometry_set_generate_state(raid_geometry_p, generate_state);

    /*! If the generate state is NULL then something is wrong
     */
    if (generate_state == NULL)
    {
        /* Trace some information and clear the initialized flag
         */
        fbe_raid_library_object_trace(object_id, 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: %s: raid geometry: 0x%p generate state is NULL \n", 
                                      __FUNCTION__, raid_geometry_p);
        fbe_raid_geometry_clear_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_INITIALIZED);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*! Lets set these to 0, since one of our initial 
     *  conditions will set these. 
     */
    fbe_raid_geometry_set_element_size(raid_geometry_p, 0);
    fbe_raid_geometry_set_elements_per_parity(raid_geometry_p, 0);
    fbe_raid_geometry_set_optimal_size(raid_geometry_p, 0);
    fbe_raid_geometry_set_imported_size(raid_geometry_p, 0);

    /* Today the raid group only supports 520 exported size.
     */
    fbe_raid_geometry_set_exported_size(raid_geometry_p, FBE_BE_BYTES_PER_BLOCK);


    /* Set the width and capacity to sane values.
     */
    fbe_raid_geometry_init_width(raid_geometry_p);
    fbe_raid_geometry_init_configured_capacity(raid_geometry_p);
    
    /*! Initialize the metadata fields.  The values aren't known at
     *  initialization time.
     */
    fbe_raid_geometry_init_metadata_start_lba(raid_geometry_p);
    fbe_raid_geometry_init_metadata_capacity(raid_geometry_p);
    fbe_raid_geometry_init_metadata_copy_offset(raid_geometry_p);

    /*! Set the geometry block edge pointer.  This is typically known
     *  at initialization time.
     */
    fbe_raid_geometry_set_block_edge(raid_geometry_p, block_edge_p);

    /*! If the block edge is NULL then something is wrong
     */
    if (block_edge_p == NULL)
    {
        /* Trace some information and clear the initialized flag
         */
        fbe_raid_library_object_trace(object_id, 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: %s: raid geometry: 0x%p block edge pointer is NULL \n", 
                                      __FUNCTION__, raid_geometry_p);
        fbe_raid_geometry_clear_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_INITIALIZED);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    raid_geometry_p->raid_type_specific.journal_info.write_log_info_p = NULL;

    /* Always return the execution status
     */
    return(status);
}
/* end fbe_raid_geometry_init()*/

/*!**************************************************************
 * fbe_raid_geometry_refresh_block_sizes()
 ****************************************************************
 * @brief
 *  Refresh our block sizes by simply looking at the edges.
 *
 * @param raid_geometry_p - Current geo object.              
 *
 * @return fbe_status_t
 *
 * @author
 *  3/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_geometry_refresh_block_sizes(fbe_raid_geometry_t *raid_geometry_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index;
    fbe_block_edge_t *edge_p = NULL;
    fbe_raid_position_bitmask_t bitmask = 0;
    fbe_u32_t width;
    fbe_raid_position_bitmask_t orig_bitmask_4k;
    
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &orig_bitmask_4k);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    for ( index = 0; index < width; index++) {
        fbe_raid_geometry_get_block_edge(raid_geometry_p, &edge_p, index);
        if (edge_p->block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160) {
            bitmask |= (1 << index);
        }
    }
    fbe_raid_geometry_set_bitmask_4k(raid_geometry_p, bitmask);
    if (orig_bitmask_4k != bitmask) {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "refresh bl/s bitmask 4k changed from 0x%x to 0x%x\n",
                                      orig_bitmask_4k, bitmask);
        if (bitmask) {
            status = fbe_raid_geometry_set_block_sizes(raid_geometry_p,
                                                       FBE_BE_BYTES_PER_BLOCK, /* Exported block size */
                                                       FBE_4K_BYTES_PER_BLOCK, /* Physical block size */
                                                       FBE_520_BLOCKS_PER_4K   /* Optimal size */);
        } else {
            status = fbe_raid_geometry_set_block_sizes(raid_geometry_p,
                                                       FBE_BE_BYTES_PER_BLOCK, /* Exported block size */
                                                       FBE_BE_BYTES_PER_BLOCK, /* Physical block size */
                                                       1                       /* Optimal size */);
        }
    }
    return status;
}
/******************************************
 * end fbe_raid_geometry_refresh_block_sizes()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_should_refresh_block_sizes()
 ****************************************************************
 * @brief
 *  Determine if we need to refresh the block sizes.
 *
 * @param raid_geometry_p - Current geo object.          
 * @param b_refresh_p - TRUE refresh needed, FALSE not needed.    
 *
 * @return fbe_status_t
 *
 * @author
 *  6/4/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_geometry_should_refresh_block_sizes(fbe_raid_geometry_t *raid_geometry_p,
                                                          fbe_bool_t *b_refresh_p,
                                                          fbe_raid_position_bitmask_t *refresh_bitmap_p)
{
    fbe_u32_t index;
    fbe_block_edge_t *edge_p = NULL;
    fbe_raid_position_bitmask_t bitmask = 0;
    fbe_u32_t width;
    fbe_raid_position_bitmask_t orig_bitmask_4k;
    fbe_raid_position_bitmask_t update_needed_bitmask = 0;
    
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &orig_bitmask_4k);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    for ( index = 0; index < width; index++) {
        fbe_raid_geometry_get_block_edge(raid_geometry_p, &edge_p, index);
        if (edge_p->block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160) {
            bitmask |= (1 << index);
            if ((orig_bitmask_4k & (1 << index)) == 0) {
                /* Bit needs to get added (now it is 4k). */
                update_needed_bitmask |= (1 << index);
            }
        } else if ((orig_bitmask_4k & (1 << index)) != 0) {
            /* Bit needs to be removed (no longer 4k) */
            update_needed_bitmask |= (1 << index);
        }
    }
    if (orig_bitmask_4k != bitmask) {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "bl/s refresh needed old: 0x%x new: 0x%x update_needed: 0x%x\n",
                                      orig_bitmask_4k, bitmask, update_needed_bitmask);
        *b_refresh_p = FBE_TRUE;
    } else {
        *b_refresh_p = FBE_FALSE;
    }
    if (refresh_bitmap_p != NULL) {
        *refresh_bitmap_p = update_needed_bitmask;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_geometry_should_refresh_block_sizes()
 ******************************************/
/*!**************************************************************
 *          fbe_raid_geometry_validate_and_set_raid_type()
 ****************************************************************
 *
 * @brief   Validate the passed raid type is compatible with the
 *          base object.
 *
 * @param raid_geometry_p - Pointer to raid geometry informationm
 * @param raid_type - 
 *
 * @return status  - FBE_STATUS_OK - The geometry has been configured
 *                   other - The geometry hasn't been configured
 *
 * @author
 *  11/02/2009  Ron Proulx - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_raid_geometry_validate_and_set_raid_type(fbe_raid_geometry_t *raid_geometry_p,
                                                          fbe_raid_group_type_t raid_type)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_class_id_t  base_class_id = FBE_CLASS_ID_INVALID;

    /* First validate that the geometry has been initialized.
     */
    if (!fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_INITIALIZED))
    {
        /* The raid geometry wasn't properly initialized, fail the request.
         */
        status = FBE_STATUS_NOT_INITIALIZED;
    }
    else
    {
        /* Else based on the requested raid type, validate that the base object 
         * supports that raid type.
         */
        base_class_id = fbe_raid_geometry_get_class_id(raid_geometry_p);
        switch(raid_type)
        {
            case FBE_RAID_GROUP_TYPE_UNKNOWN:
                /* This raid type is invalid so we must fail the
                 * configuration request.
                 */
                status = FBE_STATUS_NOT_INITIALIZED;
                break;
        
            case FBE_RAID_GROUP_TYPE_RAID1:
            case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
                /* This must be a mirror object
                 */
                if (base_class_id != FBE_CLASS_ID_MIRROR)
                {
                    status = FBE_STATUS_GENERIC_FAILURE; 
                }
                break;

            case FBE_RAID_GROUP_TYPE_RAID10:
            case FBE_RAID_GROUP_TYPE_RAID0:
                /* This must be a striper class.
                 */
                if (base_class_id != FBE_CLASS_ID_STRIPER)
                {
                    status = FBE_STATUS_GENERIC_FAILURE; 
                }
                break;

            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
            case FBE_RAID_GROUP_TYPE_RAID6:
                /* This must be a parity class.
                 */
                if (base_class_id != FBE_CLASS_ID_PARITY)
                {   
                    status = FBE_STATUS_GENERIC_FAILURE; 
                }
                break;

            case FBE_RAID_GROUP_TYPE_SPARE:
                /* This must be a virtual drive class.
                 */
                if (base_class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
                {
                    status = FBE_STATUS_GENERIC_FAILURE; 
                }
                break;

            case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
                /* This must be a provisioned drive.
                 */
                if (base_class_id != FBE_CLASS_ID_PROVISION_DRIVE)
                {
                    status = FBE_STATUS_GENERIC_FAILURE; 
                }
                break;

            default:
                /* Something is wrong.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
        } /* end switch on raid type. */
    } /* end else check raid type against class */

    /* If the status is ok set the raid type in the geometry.
     */
    if (status == FBE_STATUS_OK)
    {
        fbe_raid_geometry_set_raid_type(raid_geometry_p, raid_type);
    }
    else
    {
        /* Else trace the failure.
         */

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: class id: 0x%x raid type: 0x%x status: 0x%x\n",
                               base_class_id, raid_type, status);
    }

    /* Return the status.
     */
    return(status);
}
/* end fbe_raid_geometry_validate_and_set_raid_type() */

/*!***************************************************************
 *          fbe_raid_geometry_set_block_sizes()
 *****************************************************************
 *
 * @brief   The raid geometry information has been configured.  Now
 *          update (set) the block size information.
 *
 * @param   raid_geometry_p - Pointer to raid geometry to configure
 * @param   exported_block_size - The block size that raid exports
 * @param   imported_block_size - The block size the is exported by one of the components
 * @param   optimal_block_size - The optimal block size
 *
 * @return  The status of the operation typically FBE_STATUS_OK.
 *
 * @note    Assumes that caller has taken lock so that update is atomic.
 *
 * @author
 *  11/17/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_set_block_sizes(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_block_size_t exported_block_size,
                                               fbe_block_size_t imported_block_size,
                                               fbe_block_size_t optimal_block_size)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_block_count_t   max_blocks_per_drive;

    /* We always set the block size parameters so that we can understand
     * which parameters are illegal.
     */
    fbe_raid_geometry_set_exported_size(raid_geometry_p, exported_block_size);
    fbe_raid_geometry_set_imported_size(raid_geometry_p, imported_block_size);
    fbe_raid_geometry_set_optimal_size(raid_geometry_p, optimal_block_size);

    /* Validate the block size values.
     */
    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);
    if (!(fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_CONFIGURED)) ||   
         (exported_block_size != FBE_BE_BYTES_PER_BLOCK)                                     ||
         (imported_block_size  < FBE_BYTES_PER_BLOCK)                                        ||
         (optimal_block_size   < 1)                                                          ||
         (max_blocks_per_drive < FBE_RAID_MAX_OPTIMAL_BLOCK_SIZE)                               )
    {
        /* Fail the request and don't set the block size valid.
         */
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "Error in - exported: 0x%x or imported: 0x%x or optimal: 0x%x or max per drive: 0x%llx\n",
                                      exported_block_size, imported_block_size,
				      optimal_block_size,
				     (unsigned long long)max_blocks_per_drive);
        fbe_raid_geometry_clear_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_VALID);
        fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_INVALID);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Else the block size parameters are valid but don't set
         * the block size valid if the negotiate failed flag is set.
         */
        fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_VALID);

    }

    /* Now return the status of the request.
     */
    return(status);
}
/* end fbe_raid_geometry_set_block_sizes() */

/*!***************************************************************
 *          fbe_raid_geometry_set_configuration()
 *****************************************************************
 *
 * @brief   The raid geometry information has been initialized now
 *          it is configured.  After being configured it can be used
 *          for I/O.
 *
 * @param   raid_geometry_p - Pointer to raid geometry to configure
 * @param   width - The width of this raid group
 * @param   raid_type - The raid type for this geometry
 * @param   element_size - The element size for the raid group
 * @param   elements_per_parity_stripe - How many elements (per positon)
 *              in a parity stripe
 * @param   configured_capacity - Configured capacity of raid group
 * @param   max_blocks_per_drive_request - Maximum number of blocks for a
 *              each drive request
 *
 * @return  The status of the operation typically FBE_STATUS_OK.
 *
 * @note    Assumes that caller has taken lock so that update is atomic.
 *
 * @author
 *  11/06/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_set_configuration(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_u32_t width,
                                                 fbe_raid_group_type_t raid_type,
                                                 fbe_element_size_t element_size,
                                                 fbe_elements_per_parity_t elements_per_parity_stripe,
                                                 fbe_lba_t configured_capacity,
                                                 fbe_block_count_t max_blocks_per_drive_request)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Even if this routine fails we still flag the fact that the geometry
     * has been configured (for debug purposes).
     */
    fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_CONFIGURED);

    /* Set and validate the raid type.
     */
    status = fbe_raid_geometry_validate_and_set_raid_type(raid_geometry_p, raid_type);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: raid type: %d isn't valid for class: 0x%x status: 0x%x\n",
                                      raid_type, fbe_raid_geometry_get_class_id(raid_geometry_p), status);
        
        return status;
    }

    /* The maximum per-drive request size MUST at least the the size of an
     * element (i.e. the per-postions stripe size).
     */
    if (RAID_COND(max_blocks_per_drive_request < element_size))
    {
        /* Log an error and set status to failure
         */

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: max blocks per-drive: 0x%llx is less than element size: 0x%x\n",
                                      (unsigned long long)max_blocks_per_drive_request,
			              element_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the width.  For striped mirrors we multiply the width by 2
     * since the validation method expects the number of disks.
     */
    status = fbe_raid_geometry_validate_width(raid_type,
                                              ((raid_type == FBE_RAID_GROUP_TYPE_RAID10) ? (width * 2) : width) );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid: width: %d isn't valid for raid type: %d class: 0x%x status: 0x%x\n",
                                      width, raid_type, fbe_raid_geometry_get_class_id(raid_geometry_p), status);
        return status;
    }

    /* Configure the remaining geometry fields.
     */
    fbe_raid_geometry_set_width(raid_geometry_p, width);
    fbe_raid_geometry_set_configured_capacity(raid_geometry_p, configured_capacity);
    fbe_raid_geometry_set_element_size(raid_geometry_p, element_size);
    fbe_raid_geometry_set_elements_per_parity(raid_geometry_p, elements_per_parity_stripe);
    fbe_raid_geometry_set_max_blocks_per_drive(raid_geometry_p, max_blocks_per_drive_request);

    /* Return the status of the request.
     */
    return(status);
}
/* end fbe_raid_geometry_set_configuration() */

/*!***************************************************************************
 *          fbe_raid_geometry_init_journal_write_log()
 ***************************************************************************** 
 * 
 * @brief   This function inits the journal write log based on the fact what 
 * version of software is committed
 *
 * @param   raid_geometry_p - Pointer to raid group geometry
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ashok Tamilarasan  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_geometry_init_journal_write_log(fbe_raid_geometry_t *raid_geometry_p)
{
    fbe_element_size_t  element_size;
    fbe_u32_t write_log_slot_size;
    fbe_u32_t write_log_slot_count;
    fbe_parity_write_log_flags_t flags = FBE_PARITY_WRITE_LOG_FLAGS_NONE;

    /* Only applicable to parity raid groups.
     */
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p) == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);


    /* Journal is always aligned to 4K (4160) block size independant of the disk block size.
     */
    if (element_size == FBE_RAID_SECTORS_PER_ELEMENT) {
        write_log_slot_size = FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM_4K;
        write_log_slot_count = FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM_4K;
    } else {
        write_log_slot_size = FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH_4K;
        write_log_slot_count = FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_BANDWIDTH_4K;
    }
    fbe_raid_geometry_journal_set_flag(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_4K_SLOT);
    fbe_parity_write_log_get_flags(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p,
                                   &flags);
    fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                  FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                  "%s Write Log Size:%d, Count:%d flags: 0x%x\n",
                                  __FUNCTION__, write_log_slot_size, write_log_slot_count, flags);
        
    fbe_raid_geometry_set_journal_log_slot_size(raid_geometry_p, write_log_slot_size);
    fbe_raid_geometry_set_write_log_slot_count(raid_geometry_p, write_log_slot_count);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 *          fbe_raid_geometry_set_metadata_configuration()
 *****************************************************************
 *
 * @brief   The raid geometry information has been configured.  Now
 *          populate the neccessary metadata information.
 *
 * @param   raid_geometry_p - Pointer to raid geometry to configure
 * @param   metadata_start_lba - Starting lba for metadata
 * @param   metadata_capacity - Capacity of metadata for this object
 * @param   metadata_copy_offset - The difference between the start of
 *          of each copy of the metadata (only used for metadata I/O)
 * @param   journal_write_log_start_lba - start lba for the journal
 *
 * @return  The status of the operation typically FBE_STATUS_OK.
 *
 * @note    Assumes that caller has taken lock so that update is atomic.
 *
 * @author
 *  02/04/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_set_metadata_configuration(fbe_raid_geometry_t *raid_geometry_p,
                                                          fbe_lba_t metadata_start_lba,
                                                          fbe_lba_t metadata_capacity,
                                                          fbe_lba_t metadata_copy_offset,
                                                          fbe_lba_t write_log_start_lba)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Simply set the metadata field for the raid geometry.
     */
    fbe_raid_geometry_set_metadata_start_lba(raid_geometry_p, metadata_start_lba);
    fbe_raid_geometry_set_metadata_capacity(raid_geometry_p, metadata_capacity);
    fbe_raid_geometry_set_metadata_copy_offset(raid_geometry_p, metadata_copy_offset);

    /* Set the journal write log configuration.
     */
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p) == FBE_TRUE)
    {
        fbe_raid_geometry_set_journal_log_start_lba(raid_geometry_p, write_log_start_lba);
        fbe_raid_geometry_init_journal_write_log(raid_geometry_p);
    }

    /* Now flag the fact that the metadata has been configured.
     */ 
    fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_METADATA_CONFIGURED);

    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_raid_geometry_set_metadata_configuration() */

/*!**************************************************************
 *          fbe_raid_type_get_data_disks()
 ****************************************************************
 * @brief
 *  Get the number of data disks for this width based on the
 *  raid type.
 *
 * @param raid_type - type to get data_disks for.
 * @param width - number of total drives in raid group.
 * @param data_disks_p - ptr to return data disks
 *
 * @return status  - FBE_STATUS_OK - Success.
 *                   other - Failure.
 *
 * @author
 *  2/10/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_type_get_data_disks(fbe_raid_group_type_t raid_type,
                                          fbe_u32_t width,
                                          fbe_u16_t *data_disks_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (raid_type)
        {
            case FBE_RAID_GROUP_TYPE_UNKNOWN:
                /* The raid group hasn't been configured.
                 */
                status = FBE_STATUS_NOT_INITIALIZED;
                break;
    
            case FBE_RAID_GROUP_TYPE_RAID1:
            case FBE_RAID_GROUP_TYPE_SPARE:
            case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
                /* All supported mirror types have (1) data disk.
                 */
                *data_disks_p = 1;
                break;

            case FBE_RAID_GROUP_TYPE_RAID10:
                /* The striper's width is already set correctly in
                 * the geometry. 
                 */
                *data_disks_p = width;
                break;
    
            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
                /* For normal parity units the number of data disk is the
                 * width minus 1.
                 */
                *data_disks_p = width - 1;
                break;


            case FBE_RAID_GROUP_TYPE_RAID0:
                /* For RAID-0 the number of data disks is the width.
                 */
                *data_disks_p = width;
                break;

            case FBE_RAID_GROUP_TYPE_RAID6:
                /* For RAID-6 there are 2 parity positions.
                 */
                *data_disks_p = width - 2;
                break;

            default:
                /* Unsupported raid type.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
    };
    
    /* Return the status of the request.
     */
    return(status);
}
/******************************************
 * end fbe_raid_type_get_data_disks()
 ******************************************/

/*!**************************************************************
 *          fbe_raid_geometry_get_data_disks()
 ****************************************************************
 * @brief
 *  Get the number of data disks for this raid group based on the
 *  raid type.
 *
 * @param raid_geometry_p - Pointer to raid geometry informationm
 * @param data_disks_p - ptr to return data disks
 *
 * @return status  - FBE_STATUS_OK - The geometry has been configured
 *                   other - The geometry hasn't been configured
 *
 * @author
 *  10/29/2009  Ron Proulx - Created.
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_get_data_disks(fbe_raid_geometry_t *raid_geometry_p,
                                              fbe_u16_t *data_disks_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t   width;

    /* This routine shouldn't be invoked unless the raid geometry has been
     * configured. Default the number of data disks to 0.
     */
    *data_disks_p = 0;
    if (fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_CONFIGURED))
    {
        /* Get the raid group width and switch on the raid type.
         */
        fbe_raid_geometry_get_width(raid_geometry_p, &width);

        status = fbe_raid_type_get_data_disks(raid_geometry_p->raid_type,
                                              width,
                                              data_disks_p);
    }
    else
    {
        /* Else flag the fact the the geometry hasn't been configured.
         */
        status = FBE_STATUS_NOT_INITIALIZED; 
    }
    
    /* Return the status of the request.
     */
    return(status);
}
/******************************************
 * end fbe_raid_geometry_get_data_disks()
 ******************************************/

/*!**************************************************************
 *          fbe_raid_geometry_get_parity_disks()
 ****************************************************************
 * @brief
 *  Get the number of parity disks for this raid group based on the
 *  raid type.
 *
 * @param raid_geometry_p - Pointer to raid geometry informationm
 * @param parity_disks_p - ptr to return number of parity disks
 *
 * @return status  - FBE_STATUS_OK - The geometry has been configured
 *                   other - The geometry hasn't been configured
 *
 * @author
 *  10/29/2009  Ron Proulx - Created.
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_get_parity_disks(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_u16_t *parity_disks_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t   width;

    /* This routine shouldn't be invoked unless the raid geometry has been
     * configured. Default the number of parity disks to 0.
     */
    *parity_disks_p = 0;
    if (fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_CONFIGURED))
    {
        /* Get the raid group width and switch on the raid type.
         */
        fbe_raid_geometry_get_width(raid_geometry_p, &width);
        switch(raid_geometry_p->raid_type)
        {
            case FBE_RAID_GROUP_TYPE_UNKNOWN:
                /* The raid group hasn't been configured.
                 */
                status = FBE_STATUS_NOT_INITIALIZED;
                break;
    
            case FBE_RAID_GROUP_TYPE_RAID1:
            case FBE_RAID_GROUP_TYPE_SPARE:
            case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
                /* The number of parity disks is simply the width minus 1.
                 */
                *parity_disks_p = width - 1;
                break;

            case FBE_RAID_GROUP_TYPE_RAID10:
                /* Since each mirror must have a width of 2 the redundancy
                 * (parity) is the width divided by 2.
                 */                       
                *parity_disks_p = width / 2;
                break;
    
            case FBE_RAID_GROUP_TYPE_RAID3:
            case FBE_RAID_GROUP_TYPE_RAID5:
                /* Normal parity units have 1 parity disk.
                 */
                *parity_disks_p = 1;
                break;


            case FBE_RAID_GROUP_TYPE_RAID0:
                /* There are no parity disks for a RAID-0.
                 */
                *parity_disks_p = 0;
                break;

            case FBE_RAID_GROUP_TYPE_RAID6:
                /* For RAID-6 there are 2 parity positions.
                 */
                *parity_disks_p = 2;
                break;

            default:
                /* Unsupported raid type.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
        }
    }
    else
    {
        /* Else flag the fact the the geometry hasn't been configured.
         */
        status = FBE_STATUS_NOT_INITIALIZED; 
    }
    
    /* Return the status of the request.
     */
    return(status);
}
/******************************************
 * end fbe_raid_geometry_get_parity_disks()
 ******************************************/

/*!**************************************************************
 *          fbe_raid_geometry_is_ready_for_io()
 ****************************************************************
 *
 * @brief   Check if the raid geometry is valid or not.  
 *
 * @param raid_geometry_p - Pointer to raid geometry informationm
 *
 * @return FBE_TRUE - The raid geometry has been properly configured
 *         FBE_FALSE - The raid geometry hasn't been configured properly
 *
 * @author
 *  11/02/2009  Ron Proulx - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_raid_geometry_is_ready_for_io(fbe_raid_geometry_t *raid_geometry_p)
{
    fbe_bool_t  ready_for_io = FBE_TRUE;

    /* Currently the only check is that the raid geometry block size is
     * valid.
     */
    if (!fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_BLOCK_SIZE_VALID))
    {
        /* Not ready to accept I/O.
         */
        ready_for_io = FBE_FALSE;
    }

    /* Return whether we are able to accept I/O or not.
     */
    return(ready_for_io);
}
/* end fbe_raid_geometry_is_ready_for_io() */

/*!***************************************************************
 *          fbe_raid_geometry_not_ready_for_io()
 *****************************************************************
 *
 * @brief   This is generic routine to handle cases where the raid
 *          geometry hasn't been configured to the point where it can
 *          accespt I/O.  This can mean an optimal block size of 0,
 *          a null edge pointer etc.
 * 
 * @param   raid_geometry_p - Pointer to raid information for failed request
 * @param   packet_p - The packet that is arriving.
 *
 * @return fbe_status_t - Typically FBE_STATUS_GENERIC_FAILURE.
 *
 * @author
 *  11/17/2009  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_not_ready_for_io(fbe_raid_geometry_t *raid_geometry_p, 
                                                fbe_packet_t * const packet_p)
{
    /* First get a pointer to the block command.
     */
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_attribute_flags_t  attribute_flags;

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        
    /* Now report the failure.
    */
    fbe_raid_geometry_get_attribute_flags(raid_geometry_p, &attribute_flags);
    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                           "raid: RAID Geometry: 0x%p hasn't been properly configured. Flags: 0x%x \n", 
                           raid_geometry_p, attribute_flags);
        
    /* Transport status on the packet is OK since it was received OK. 
     * The payload status is set to bad since this command is not formed 
     * correctly. 
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_GENERIC_FAILURE;
}
/* end fbe_raid_geometry_not_ready_for_io() */


fbe_status_t fbe_raid_geometry_validate_width(fbe_raid_group_type_t raid_type, fbe_u32_t width)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    switch (raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID5:
            if ((width >= FBE_RAID_LIBRARY_RAID5_MIN_WIDTH) &&
                (width <= FBE_RAID_LIBRARY_RAID5_MAX_WIDTH))
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_RAID_GROUP_TYPE_RAID6:
            /* Raid 6 must be within a specific range and be a multiple of 2.
             */
            if ((width >= FBE_RAID_LIBRARY_RAID6_MIN_WIDTH) &&
                (width <= FBE_RAID_LIBRARY_RAID6_MAX_WIDTH) &&
                ((width % 2) == 0))
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_RAID_GROUP_TYPE_RAID3:
            if ((width == FBE_RAID_LIBRARY_RAID3_MIN_WIDTH) ||
                (width == FBE_RAID_LIBRARY_RAID3_MAX_WIDTH) ||
                (width == FBE_RAID_LIBRARY_RAID3_VAULT_WIDTH))
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_RAID_GROUP_TYPE_RAID0:
            if ((width >= FBE_RAID_LIBRARY_RAID0_MIN_WIDTH) &&
                (width <= FBE_RAID_LIBRARY_RAID0_MAX_WIDTH))
            {
                status = FBE_STATUS_OK;
            }
            /* Individual disk has just one width.
             */
            if (width == FBE_RAID_LIBRARY_INDIVIDUAL_DISK_WIDTH)
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_RAID_GROUP_TYPE_RAID10:
            /* Raid 10 must be within a specific range and be a multiple of 2.
             */
            if ((width >= FBE_RAID_LIBRARY_RAID10_MIN_WIDTH) &&
                (width <= FBE_RAID_LIBRARY_RAID10_MAX_WIDTH) &&
                ((width % 2) == 0))
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_RAID_GROUP_TYPE_RAID1:
        case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
            /* Raid 1 must be within a specific range and be a multiple of 2.
             */
            if ((width >= FBE_RAID_LIBRARY_RAID1_MIN_WIDTH) &&
                (width <= FBE_RAID_LIBRARY_RAID1_MAX_WIDTH))
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            /* Raid mirror under striper must be of width 2.
            */
            if (width == 2)
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_RAID_GROUP_TYPE_SPARE:
            /* Virtual drives/spares can only have (1) width
             */
            if (width == FBE_VIRTUAL_DRIVE_WIDTH)
            {
                status = FBE_STATUS_OK;
            }
            break;

        default:
            /* Unsupported raid type
             */
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                   "raid: raid_type: %d isn't supported. width: %d\n",
                                   raid_type, width);
            break;
    };
    return status;
}

fbe_status_t fbe_raid_geometry_determine_element_size(fbe_raid_group_type_t raid_type, 
                                                      fbe_bool_t b_bandwidth,
                                                      fbe_element_size_t *element_size_p,
                                                      fbe_elements_per_parity_t *elements_per_parity_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_element_size_t element_size = 0;
    fbe_elements_per_parity_t elements_per_parity = 0;

    if (b_bandwidth)
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT;
    }
    switch (raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID5:
        case FBE_RAID_GROUP_TYPE_RAID6:
        case FBE_RAID_GROUP_TYPE_RAID3:
            if (b_bandwidth)
            {
                elements_per_parity = FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH;
            }
            else
            {
                elements_per_parity = FBE_RAID_ELEMENTS_PER_PARITY;
            }
            break;
        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID10:
        case FBE_RAID_GROUP_TYPE_RAID1:
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
        case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
            /* Elements per parity not used for these.
             */
            elements_per_parity = 0;
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    *element_size_p = element_size;
    *elements_per_parity_p = elements_per_parity;
    return status;
}
/*!**************************************************************
 * fbe_raid_geometry_calculate_lock_range()
 ****************************************************************
 * @brief
 *  Determine the stripe number and stripe count in blocks.
 *
 * @param raid_geometry_p - Geometry to calculate for.
 * @param lba - Host lba.
 * @param blocks - host block count.
 * @param stripe_number_p - output stripe number to lock.
 * @param stripe_count_p - output count of stripes to lock.
 * 
 * @return fbe_status_t
 *
 * @author
 *  2/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_calculate_lock_range(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t blocks,
                                                    fbe_u64_t *stripe_number_p,
                                                    fbe_u64_t *stripe_count_p)
{
    fbe_u16_t data_disks;
    fbe_element_size_t element_size;
    fbe_raid_extent_t parity_range[1];

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /*! Parity units would lock on a parity lba, 
     * and other unit types would lock on lbas. 
     * Raid 0 only locks on a stripe basis since if it needs to do recovery verify 
     * it will have the while stripe lock and will not need to extend the range to cover 
     * the entire stripe. 
     */
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p) ||
        fbe_raid_geometry_is_raid0(raid_geometry_p))
    {
        fbe_raid_get_stripe_range(lba,
                                  blocks,
                                  element_size,
                                  data_disks,
                                  1, /* Only one lock extent is allowed. */
                                  parity_range);
        *stripe_number_p = parity_range[0].start_lba;
        *stripe_count_p = parity_range[0].size;
    }
    else
    {
        /* Otherwise we lock on simply the lba and blocks.
         */
        *stripe_number_p = lba;
        *stripe_count_p = blocks;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_geometry_calculate_lock_range()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_calculate_zero_lock_range()
 ****************************************************************
 * @brief
 *  Calculate a lock for a zero request, which must align to the chunk stripe.
 *  We need to align to the stripe due to pass around optimizations
 *  that the PVD does.
 *
 *  The PVD zeros on a chunk basis, and to prevent background zero from
 *  conflicting with any in chunk r/w requests we need to lock the zero
 *  on a chunk stripe.
 *
 * @param raid_geometry_p - Geometry to calculate for.
 * @param lba - Host lba.
 * @param blocks - host block count.
 * @param chunk_size - Number of bytes in a chunk.
 * @param stripe_number_p - output stripe number to lock.
 * @param stripe_count_p - output count of stripes to lock.
 * 
 * @return fbe_status_t
 *
 * @author
 *  2/13/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_calculate_zero_lock_range(fbe_raid_geometry_t *raid_geometry_p,
                                                         fbe_lba_t lba,
                                                         fbe_block_count_t blocks,
                                                         fbe_chunk_size_t chunk_size,
                                                         fbe_u64_t *stripe_number_p,
                                                         fbe_u64_t *stripe_count_p)
{
    fbe_u16_t data_disks;
    fbe_element_size_t element_size;
    fbe_u64_t stripe_number;
    fbe_u64_t end_stripe_number;
    fbe_u64_t stripe_count;
    fbe_block_count_t stripe_size;
    fbe_block_count_t chunk_stripe_size; 

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    stripe_size = ((fbe_block_count_t)element_size) * data_disks;
    chunk_stripe_size = ((fbe_block_count_t)chunk_size) * data_disks;

    /* First align the lba and blocks to the chunk stripe.
     */
    if (lba % chunk_stripe_size)
    {
        lba -= (lba % chunk_stripe_size);
        blocks += (lba % chunk_stripe_size);
    }
    if (blocks % chunk_stripe_size)
    {
        blocks += chunk_stripe_size - (blocks % chunk_stripe_size);
    }

    /*! This limits the amount of possible locks the lock service needs. 
     * Ideally parity units would lock on a parity lba, 
     * and other unit types would lock on lbas, but this is not supported 
     * with the current lock service. 
     * Raid 0 only locks on a stripe basis since if it needs to do recovery verify 
     * it will have the while stripe lock and will not need to extend the range to cover 
     * the entire stripe. 
     */
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p) ||
        fbe_raid_geometry_is_raid0(raid_geometry_p))
    {
        stripe_number = lba / stripe_size;
        end_stripe_number = (lba + blocks - 1) / stripe_size;
    }
    else
    {
        /* Otherwise we lock on an element basis.
         */
        stripe_number = lba / element_size;
        end_stripe_number = ((lba + blocks - 1) / element_size);
    }
    stripe_count = (end_stripe_number - stripe_number + 1);
    *stripe_number_p = stripe_number;
    *stripe_count_p = stripe_count;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_geometry_calculate_zero_lock_range()
 ******************************************/

/*!**************************************************************
 * fbe_raid_geometry_calculate_lock_range_physical()
 ****************************************************************
 * @brief
 *  
 *
 * @param raid_geometry_p - Geometry to calculate for.
 * @param lba - Offset from start of raid group on one disk.
 * @param blocks - Size of operation on one disk.
 * @param stripe_number_p - output stripe number to lock.
 * @param stripe_count_p - output count of stripes to lock.
 * 
 * @return fbe_status_t
 *
 * @author
 *  2/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_calculate_lock_range_physical(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_lba_t lba,
                                                             fbe_block_count_t blocks,
                                                             fbe_u64_t *stripe_number_p,
                                                             fbe_u64_t *stripe_count_p)
{
    fbe_u16_t data_disks;
    fbe_element_size_t element_size;
    fbe_u64_t stripe_number;
    fbe_u64_t end_stripe_number;
    fbe_u64_t stripe_count;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /* Parity units lock on a parity lba, 
     * and other unit types would lock on lbas. 
     * Raid 0 only locks on a stripe basis since if it needs to do recovery verify 
     * it will have the while stripe lock and will not need to extend the range to cover 
     * the entire stripe. 
     */
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p) ||
        fbe_raid_geometry_is_raid0(raid_geometry_p))
    {
        stripe_number = lba / element_size;
        end_stripe_number = (lba + blocks - 1) / element_size;
    }
    else
    {
        /* Otherwise we lock on an element basis.
         * In this case we need to multiply by data disks to make sure we 
         * lock the entire stripe range. 
         */
        stripe_number = (lba / element_size) * data_disks;
        end_stripe_number = (lba + blocks - 1) / element_size;
        end_stripe_number *= data_disks;
    }
    stripe_count = (end_stripe_number - stripe_number + 1);

    /* The output stripe # and count are in terms of blocks.
     */
    *stripe_number_p = stripe_number * element_size;
    *stripe_count_p = stripe_count * element_size;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_geometry_calculate_lock_range_physical()
 ******************************************/

/*!**************************************************************
 * fbe_raid_geometry_calculate_chunk_range()
 ****************************************************************
 * @brief
 *  Calculate the chunk range for a given I/O.
 *
 * @param raid_geometry_p - The current raid geometry.
 * @param lba - the host lba.
 * @param blocks - the host blocks
 * @param chunk_size - the number of blocks in a chunk
 * @param chunk_index_p - output chunk index for start of range.
 * @param chunk_count_p - number of chunks in range.
 * 
 * @return fbe_status_t
 *
 * @author
 *  2/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_geometry_calculate_chunk_range(fbe_raid_geometry_t *raid_geometry_p,
                                                     fbe_lba_t lba,
                                                     fbe_block_count_t blocks,
                                                     fbe_chunk_size_t chunk_size,
                                                     fbe_chunk_index_t *chunk_index_p,
                                                     fbe_chunk_count_t *chunk_count_p)
{
    fbe_u16_t data_disks;
    fbe_element_size_t element_size;
    fbe_chunk_index_t chunk_number;
    fbe_chunk_index_t end_chunk_number;
    fbe_u32_t chunk_count;
    fbe_raid_extent_t extent[FBE_RAID_MAX_PARITY_EXTENTS];
    fbe_block_count_t stripe_blocks;
    fbe_lba_t stripe_lba;

    fbe_raid_geometry_get_element_size(raid_geometry_p, 
                                                                    &element_size);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /* Determine the extent of this I/O on this raid group. 
     * This maps the host lba to a physical lba on the raid group. 
     */
    fbe_raid_get_stripe_range(lba, blocks, element_size, data_disks, 
                              FBE_RAID_MAX_PARITY_EXTENTS, extent);
    stripe_lba = extent[0].start_lba;

    if (extent[1].size != 0)
    {
        /* Discontinuous range. 
         * Just merge the two ranges together for the purposes of 
         * determining the chunks. 
         */
        fbe_lba_t end_pt = (extent[1].start_lba +
                            extent[1].size - 1);
        stripe_blocks = (fbe_block_count_t)( end_pt - extent[0].start_lba + 1);
    }
    else
    {
        stripe_blocks = extent[0].size;
    }

    /* Determine our start and end chunk and the number of chunks.
     */
    chunk_number = stripe_lba / chunk_size;
    end_chunk_number = (stripe_lba + stripe_blocks - 1) / chunk_size; 
    chunk_count = (fbe_chunk_count_t)(end_chunk_number - chunk_number) + 1;

    *chunk_index_p = chunk_number;
    *chunk_count_p = chunk_count;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_geometry_calculate_chunk_range()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_geometry_does_request_exceed_extent()
 *****************************************************************************
 *
 * @brief   Simply validate that we don't exceed the range of an extent.
 *
 * @param   raid_geometry_p - Pointer to raid geometry to check against
 * @param   start_lba - Start of request to check
 * @param   blocks - Number of blocks in this request
 * @param   b_allow_full_journal_access - FBE_TRUE - Allow this request to access
 *                                          the journal area even if mds is invalid.
 *                                      FBE_FALSE - Should not be accessing journal
 *                                          area if mds is invalid.
 *
 * @return  bool - FBE_TRUE - Request exceeds extent
 *                 FBE_FALSE - Request is ok
 *
 * @author
 *  03/09/2009  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_geometry_does_request_exceed_extent(fbe_raid_geometry_t *raid_geometry_p,
                                                        fbe_lba_t start_lba,
                                                        fbe_block_count_t blocks,
                                                        fbe_bool_t b_allow_full_journal_access)
{
    fbe_bool_t  b_exceeded = FBE_FALSE;
    fbe_lba_t   end_lba;
    fbe_lba_t   metadata_start_lba;
    fbe_lba_t   metadata_end_lba;
    fbe_lba_t   metadata_capacity;
    fbe_u16_t   data_disks = 0;
    fbe_lba_t   journal_start_lba;
    fbe_object_id_t raid_geometry_object_id = FBE_OBJECT_ID_INVALID;

    if (fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_EXTENT_POOL)) {
        return FBE_FALSE;
    }
    /* Get the last block that is being accessed.
     */
    end_lba = (start_lba + blocks) - 1;

    /* Simply use the geometry to get the metadata start lba
     */
     fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
    fbe_raid_geometry_get_metadata_capacity(raid_geometry_p,
                                            &metadata_capacity);
     fbe_raid_type_get_data_disks(raid_geometry_p->raid_type, 
                                  raid_geometry_p->width,
                                  &data_disks);
    raid_geometry_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    /* Journal start lba is physical.
     */
     fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_start_lba);
     
    /* Since the metadata_start_lba is logical convert it to physical if it is
     * valid.
     */
    if(metadata_start_lba != FBE_LBA_INVALID) 
    {
        metadata_start_lba = (metadata_start_lba / data_disks);
        metadata_end_lba   =  metadata_start_lba + (metadata_capacity / data_disks) - 1;
    }

     /* If the metadata start is invalid or the request is less than it, the 
      * request is in the configured extent.
      */
     if (metadata_start_lba == FBE_LBA_INVALID)
     {

        fbe_lba_t   configured_capacity;

         fbe_raid_geometry_get_configured_capacity(raid_geometry_p,
                                                  &configured_capacity);
         /* Assumption that extent start offset is 0.
          */
         if (end_lba >= (configured_capacity / data_disks))
         {
             b_exceeded = FBE_TRUE;
             fbe_raid_library_object_trace(raid_geometry_object_id, 
                                           FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                           "raid: Check request end_lba: 0x%llx exceeds user space end: 0x%llx\n",
                                           (unsigned long long)end_lba,
					   (unsigned long long)(configured_capacity / data_disks));
         }
     }
     else if (end_lba < metadata_start_lba)
     {
         /* Majority of I/Os will be in user space.
          */
     }
     else if ((journal_start_lba == FBE_LBA_INVALID) ||
              (end_lba < journal_start_lba)             )
     {
         /* There is no journal area.  Validate that we don't span
          * the user and metadata areas.
          */
         if (start_lba < metadata_start_lba)
         {
             /* The request started in user space, it cannot span into
              * the meatadata space.
              */
             if (end_lba >= metadata_start_lba)
             {
                 b_exceeded = FBE_TRUE;
                 fbe_raid_library_object_trace(raid_geometry_object_id, 
                                               FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                               "raid: Check request end_lba: 0x%llx spans into metadata start: 0x%llx\n",
                                               (unsigned long long)end_lba,
					       (unsigned long long)metadata_start_lba);
             }
         }
         else
         {
         /* Check against metadata extent.
          */
             if (end_lba > metadata_end_lba)
         {
             b_exceeded = FBE_TRUE;
             fbe_raid_library_object_trace(raid_geometry_object_id, 
                                           FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                           "raid: Check request end_lba: 0x%llx exceeds metadata end: 0x%llx\n",
                                           (unsigned long long)end_lba,
					   (unsigned long long)metadata_end_lba);
             }
         }
     }
     else
     {
         fbe_lba_t  journal_end_lba;

         /* Else this request is using the journal extent.  Validate that
          * it doesn't span the  metadata->journal area unless the 
          * `b_allow_full_journal_access' flag is set. 
          */
         journal_end_lba = journal_start_lba + (FBE_RAID_GROUP_WRITE_LOG_SIZE) - 1;

         /* If `full' journal access is allowed.
          */
         if (b_allow_full_journal_access == FBE_TRUE)
         {
             /* Validate that we don't go beyond the journal space.
              */
             if (end_lba > journal_end_lba)
             {
                 b_exceeded = FBE_TRUE;
                 fbe_raid_library_object_trace(raid_geometry_object_id, 
                                               FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                               "raid: Check request end_lba: 0x%llx exceeds journal end: 0x%llx\n",
                                               (unsigned long long)end_lba,
					       (unsigned long long)journal_end_lba);
             }
         }
         else
         {
             /* Validate that we don't span the metadata->journal area.
              */
             if (start_lba < journal_start_lba)
             {
                b_exceeded = FBE_TRUE;
                fbe_raid_library_object_trace(raid_geometry_object_id, 
                                              FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                              "raid: Check request start_lba: 0x%llx spans journal start: 0x%llx\n",
                                              (unsigned long long)start_lba,
					      (unsigned long long)journal_start_lba);
             }
             else if (end_lba > journal_end_lba)
             {
                 /* Else validate that we don't exceed the journal area.
                  */
                 b_exceeded = FBE_TRUE;
                 fbe_raid_library_object_trace(raid_geometry_object_id, 
                                               FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                               "raid: Check request end_lba: 0x%llx exceeds journal end: 0x%llx\n",
                                               (unsigned long long)end_lba,
					       (unsigned long long)journal_end_lba);
             }
         }
     } /* end else this request access the journal area */

     /* A value of true means the i/O exceeds the extent
      */
     return(b_exceeded);
}
/****************************************************
 * end fbe_raid_geometry_does_request_exceed_extent()
 ****************************************************/


/*!***************************************************************************
 *          fbe_raid_geometry_is_journal_io()
 *****************************************************************************
 *
 * @brief   Determines if specified LBA is journal lba.
 *
 * @param   raid_geometry_p - Pointer to raid geometry to check against
 * @param   io_start_lba    - LBA to be checked for journal range
 *
 * @return  bool - FBE_TRUE - io_start_lba is in journal area.
 *                 FBE_FALSE - io_start_lba not in journal area.
 *
 * @author
 *  06/12/2012  Prahlad Purohit - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_geometry_is_journal_io(fbe_raid_geometry_t *raid_geometry_p,
                                           fbe_lba_t io_start_lba)
{
    fbe_lba_t journal_lba = FBE_LBA_INVALID;
    fbe_u16_t data_disks;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_lba);

    journal_lba = journal_lba * data_disks;

    if(FBE_LBA_INVALID == journal_lba)
    {
        return FALSE;
    }
    else if(io_start_lba >= journal_lba)
    {
        /* Yes the I/O is within the journal area.
         */
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/****************************************************
 * end fbe_raid_geometry_is_journal_io()
 ****************************************************/

/*!**************************************************************
 * fbe_raid_geometry_get_raid_group_offset()
 ****************************************************************
 * @brief
 *  Determine if there is an additional offset we need to apply
 *  at the raid group level.
 * 
 * @param raid_geometry_p - current object.
 * @param rg_offset_p - The output ptr to the offset we need to apply.
 *
 * @return fbe_status_t 
 *
 * @author
 *  8/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_geometry_get_raid_group_offset(fbe_raid_geometry_t *raid_geometry_p,
                                                     fbe_lba_t *rg_offset_p)
{
    /* We need to use a 0 raid group offset since all lba stamps will be relative to the raid group, 
     * not relative to the physical lba.  This gives us the option to put our drives at 
     * different physical offsets in the raid group without issue. 
     *  
     * One exception is a raw mirror instance without an object.  In this case we might have a raw mirror instance 
     * that has an offset onto the raid group that needs to get applied.  This ensures that 
     * the lba stamps that are generated/checked by the library agree with what is generated/checked by the object. 
     */
    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        fbe_raid_geometry_get_raw_mirror_rg_offset(raid_geometry_p, rg_offset_p);
    }
    else
    {
        *rg_offset_p = 0;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_geometry_get_raid_group_offset()
 ******************************************/

/****************************************************************
 * fbe_raid_geometry_is_stripe_aligned()
 ****************************************************************
 * @brief
 *  Determine if we are stripe aligned.
 *  
 * @param siots_p - Current sub request.
 *
 * @return FBE_TRUE if aligned FBE_FALSE otherwise.
 * 
 * @date
 *  2/19/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_raid_geometry_is_stripe_aligned(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_lba_t lba, fbe_block_count_t blocks)
{
    fbe_bool_t b_aligned = FBE_TRUE;
    fbe_u32_t blocks_per_element;
    fbe_u16_t data_disks;
    fbe_u32_t blocks_per_stripe;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &blocks_per_element);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    blocks_per_stripe = blocks_per_element * data_disks;

    /* We are not aligned if we are not stripe aligned.
     * The first block and last blocks must be stripe aligned. 
     */
    if ((lba % blocks_per_stripe) != 0)
    {
        b_aligned = FBE_FALSE;
    }
    if (((lba + blocks) % blocks_per_stripe) != 0)
    {
        b_aligned = FBE_FALSE;
    }
    return b_aligned;
}
/**************************************
 * end fbe_raid_geometry_is_stripe_aligned()
 **************************************/

/****************************************************************
 * fbe_raid_geometry_is_single_position()
 ****************************************************************
 * @brief
 *  Determine if we are writing to a single disk.
 *  
 * @param siots_p - Current sub request.
 *
 * @return FBE_TRUE if within a single disk, FBE_FALSE otherwise.
 * 
 * @date
 *  03/22/2013 - Created. Dave Agans
 *
 ****************************************************************/
fbe_bool_t fbe_raid_geometry_is_single_position(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_lba_t lba,
                                                fbe_block_count_t blocks)
{
    fbe_bool_t b_single_disk = FBE_TRUE;
    fbe_u32_t blocks_per_element;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &blocks_per_element);

    /* We are not single disk if the start lbe is not in the same element as the end,
     * or the count > one element (which catches the full stripe wrap around case).
     */
    if (((lba / blocks_per_element) != ((lba + blocks - 1) / blocks_per_element)) ||
        (blocks > blocks_per_element))
    {
        b_single_disk = FBE_FALSE;
    }
    return b_single_disk;
}
/**************************************
 * end fbe_raid_geometry_is_single_position()
 **************************************/

static fbe_u32_t fbe_raid_group_zeroing_multiplier = 0;
static fbe_u32_t fbe_raid_group_degraded_multiplier = 0;
static fbe_u32_t fbe_raid_group_parity_degraded_read_multiplier = FBE_RAID_DEGRADED_PARITY_READ_MULTIPLIER;
static fbe_u32_t fbe_raid_group_parity_degraded_write_multiplier = FBE_RAID_DEGRADED_PARITY_WRITE_MULTIPLIER;
static fbe_u32_t fbe_raid_group_mirror_degraded_read_multiplier = FBE_RAID_DEGRADED_MIRROR_READ_MULTIPLIER;
static fbe_u32_t fbe_raid_group_mirror_degraded_write_multiplier = FBE_RAID_DEGRADED_MIRROR_WRITE_MULTIPLIER;
static fbe_u32_t fbe_raid_group_credits_cost_ceiling_divisor = FBE_RAID_DEGRADED_CREDIT_CEILING_DIVISOR;

/***************************************************************
 *  Accessors to set the parameters that are used to calculate the 
 *  io weights when degraded
 * 
 *  9/30/2013 - Created. Deanna Heng
 ****************************************************************/
void fbe_raid_geometry_set_parity_degraded_read_multiplier(fbe_u32_t multiplier)
{
    fbe_raid_group_parity_degraded_read_multiplier = multiplier;
}

void fbe_raid_geometry_set_parity_degraded_write_multiplier(fbe_u32_t multiplier)
{
    fbe_raid_group_parity_degraded_write_multiplier = multiplier;
}

void fbe_raid_geometry_set_mirror_degraded_read_multiplier(fbe_u32_t multiplier)
{
    fbe_raid_group_mirror_degraded_read_multiplier = multiplier;
}

void fbe_raid_geometry_set_mirror_degraded_write_multiplier(fbe_u32_t multiplier)
{
    fbe_raid_group_mirror_degraded_write_multiplier = multiplier;
}

void fbe_raid_geometry_set_credits_ceiling_divisor(fbe_u32_t divisor)
{
    fbe_raid_group_credits_cost_ceiling_divisor = divisor;
}

/*!**************************************************************
 * fbe_raid_geometry_calc_parity_disk_ios()
 ****************************************************************
 * @brief
 *  Determine the number of disk I/Os needed for this logical I/O.
 *
 * @param raid_geometry_p
 * @param block_op_p
 * @param b_is_zeroing
 * @param b_is_degraded
 * 
 * @return fbe_u32_t
 *
 * @author
 *  4/15/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_geometry_calc_parity_disk_ios(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_payload_block_operation_t *block_op_p,
                                                 fbe_bool_t b_is_zeroing,
                                                 fbe_bool_t b_is_degraded,
                                                 fbe_packet_priority_t priority,
                                                 fbe_u32_t max_credits)
{
    fbe_u32_t io_weight;
    fbe_block_count_t block_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t width;
    fbe_u16_t data_disks;
    fbe_u32_t multiplier = 1;
    fbe_u32_t drives_touched;
    fbe_element_size_t element_size;
    fbe_lba_t lba;
    fbe_elements_per_parity_t elements_per_parity;
    fbe_u16_t parity_disks;
    fbe_u32_t stripe_size;

    fbe_payload_block_get_lba(block_op_p, &lba);
    fbe_payload_block_get_block_count(block_op_p, &block_count);
    fbe_payload_block_get_opcode(block_op_p, &opcode);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &elements_per_parity);
    stripe_size = (data_disks * element_size);

    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            if (b_is_zeroing) {
                multiplier += fbe_raid_group_zeroing_multiplier;
            }
            if (b_is_degraded && 
                priority == FBE_PACKET_PRIORITY_NORMAL && 
                !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
            {
                multiplier += fbe_raid_group_parity_degraded_read_multiplier;
            }

            if (block_count <= element_size) {        /* small read optimization, no divide */
                drives_touched = 1;
                io_weight = drives_touched * multiplier; 
            }
            else {
                if ((block_count / element_size) < data_disks){

                    drives_touched = (fbe_u32_t)(  ((lba + block_count - 1) / element_size)
                                                 - (lba / element_size)
                                                 + 1);
                }
                else {
                    drives_touched = data_disks;
                }
                io_weight = drives_touched * multiplier; 

                if ((multiplier > 1) && 
                    b_is_degraded && 
                    priority == FBE_PACKET_PRIORITY_NORMAL && 
                    !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
                {
                    /* Make sure we only penalize at most max_credits/fbe_raid_group_credits_cost_ceiling_divisor 
                     * per IO
                     */
                    if (io_weight > max_credits / fbe_raid_group_credits_cost_ceiling_divisor)
                    {
                        io_weight = max_credits / fbe_raid_group_credits_cost_ceiling_divisor;
                    }
                }
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
            if (fbe_raid_geometry_is_vault(raid_geometry_p)){
                io_weight = 0;
                break;
            }
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
            fbe_raid_geometry_get_parity_disks(raid_geometry_p, &parity_disks);

            if (block_count < stripe_size){
                /* Small write.
                 */
                drives_touched = (fbe_u32_t)(  ((lba + block_count - 1) / element_size)
                                             - (lba / element_size)
                                             + 1
                                             + parity_disks);
            }
            else if ( ((lba % stripe_size) == 0) &&
                      ((block_count % stripe_size) == 0))
            {
                /* Stripe aligned.
                 */
                drives_touched = width;
            }
            else {
                /* Stripe size or greater that is unaligned.
                 */
                drives_touched = width;
            }
            if (b_is_degraded) {

                /* When journaling we will break the I/O across data stripes generating more I/Os.
                 * First check if write_logging is enabled for this RG
                 */
                if (fbe_raid_geometry_is_write_logging_enabled(raid_geometry_p))
                {
                    fbe_block_count_t front_block_count = 0;
                    fbe_block_count_t mid_block_count = 0;
                    fbe_block_count_t end_block_count = 0;

                    multiplier += fbe_raid_group_degraded_multiplier;
                    if (priority == FBE_PACKET_PRIORITY_NORMAL &&
                        !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
                    {
                        multiplier += fbe_raid_group_parity_degraded_write_multiplier;
                    }
                    /* Split the io up into 3 possible sections
                     */
                    if ((lba % stripe_size) || (block_count < stripe_size))
                    {
                        front_block_count = FBE_MIN(block_count, (stripe_size - (lba % stripe_size)));
                    }
                    if (((lba + block_count) % stripe_size) &&
                        (lba / stripe_size) != ((lba + block_count) / stripe_size))
                    {
                        end_block_count = (lba + block_count) % stripe_size;
                    }
                    mid_block_count = block_count - (front_block_count + end_block_count);

                    /* Now calculate the drives touched for each section
                     */
                    drives_touched = 0;
                    if (front_block_count)
                    {
                        drives_touched += (fbe_u32_t)(  ((lba + front_block_count - 1) / element_size)
                                                      - (lba / element_size)
                                                      + 1
                                                      + parity_disks);
                    }
                    if (end_block_count)
                    {
                        drives_touched += (fbe_u32_t)(  ((end_block_count - 1) / element_size)
                                                      + 1
                                                      + parity_disks);
                    }

                    if (mid_block_count)
                    {
                        drives_touched += width;
                    }
                }
            }
            else if (b_is_zeroing) {
                multiplier += fbe_raid_group_zeroing_multiplier;
            }
            io_weight = drives_touched * multiplier;
            if ((multiplier > 1) && 
                b_is_degraded && 
                priority == FBE_PACKET_PRIORITY_NORMAL && 
                !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
            {
                /* Make sure we only penalize at most max_credits/fbe_raid_group_credits_cost_ceiling_divisor 
                 * per IO
                 */
                if (io_weight > max_credits / fbe_raid_group_credits_cost_ceiling_divisor)
                {
                    io_weight = max_credits / fbe_raid_group_credits_cost_ceiling_divisor;
                }
            }

            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
            /* We will generate one set of I/Os for every parity stripe. 
             * The credits we need is based on this total number of I/Os. 
             */
            io_weight = (fbe_u32_t)((block_count / (element_size * elements_per_parity)) * width);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
            /* 1 read and 1 write. 
             */
            io_weight = 2;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            /* We can't allow these to prevent background ops from coming in. 
             * This gets the NP lock and BG ops have the np lock when then come into 
             * the block transport server.  Thus if we allow these initiate requests 
             * to hold up BG ops we will have a deadlock. 
             */
            io_weight = 0;
            break;
        default:
            io_weight = 0;
            break;
    }
    return io_weight;
}
/******************************************
 * end fbe_raid_geometry_calc_parity_disk_ios()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_calc_mirror_disk_ios()
 ****************************************************************
 * @brief
 *  Determine the number of disk I/Os needed for this logical I/O.
 *   
 * @param raid_geometry_p
 * @param block_op_p
 * @param b_is_zeroing
 * 
 * @return fbe_u32_t
 *
 * @author
 *  4/15/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_geometry_calc_mirror_disk_ios(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_payload_block_operation_t *block_op_p,
                                                 fbe_bool_t b_is_zeroing,
                                                 fbe_bool_t b_is_degraded,
                                                 fbe_packet_priority_t priority,
                                                 fbe_u32_t max_credits)
{
    fbe_u32_t io_weight;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t width;

    fbe_payload_block_get_opcode(block_op_p, &opcode);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            /* We touch one side of the mirror.
             */
            io_weight = 1;
            if ((fbe_raid_group_mirror_degraded_read_multiplier != 0) &&
                b_is_degraded && 
                priority == FBE_PACKET_PRIORITY_NORMAL &&
                !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
            {
                io_weight = io_weight * fbe_raid_group_mirror_degraded_read_multiplier;
                /* Make sure we only penalize at most max_credits/fbe_raid_group_credits_cost_ceiling_divisor 
                 * per IO
                 */
                if (io_weight > max_credits / fbe_raid_group_credits_cost_ceiling_divisor)
                {
                    io_weight = max_credits / fbe_raid_group_credits_cost_ceiling_divisor;
                }
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
            /* We write all disks of the mirror.
             */
            io_weight = width;
            if ((fbe_raid_group_mirror_degraded_write_multiplier != 0) &&
                b_is_degraded && 
                priority == FBE_PACKET_PRIORITY_NORMAL &&
                !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
            {
                io_weight = io_weight * fbe_raid_group_mirror_degraded_write_multiplier;
                /* Make sure we only penalize at most max_credits/fbe_raid_group_credits_cost_ceiling_divisor 
                 * per IO
                 */
                if (io_weight > max_credits / fbe_raid_group_credits_cost_ceiling_divisor)
                {
                    io_weight = max_credits / fbe_raid_group_credits_cost_ceiling_divisor;
                } 
            }
            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
            io_weight = width;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
            /* 1 read and 1 write. 
             */
            io_weight = 2;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            /* We can't allow these to prevent background ops from coming in. 
             * This gets the NP lock and BG ops have the np lock when then come into 
             * the block transport server.  Thus if we allow these initiate requests 
             * to hold up BG ops we will have a deadlock. 
             */
            io_weight = 0;
            break;
        default:
            io_weight = 0;
            break;
    }
    return io_weight;
}
/******************************************
 * end fbe_raid_geometry_calc_mirror_disk_ios()
 ******************************************/
/*!**************************************************************
 * fbe_raid_geometry_calc_striper_disk_ios()
 ****************************************************************
 * @brief
 *  Determine the number of disk I/Os needed for this logical I/O.
 *   
 * @param raid_geometry_p
 * @param block_op_p
 * @param b_is_zeroing
 * 
 * @return fbe_u32_t
 *
 * @author
 *  4/15/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_geometry_calc_striper_disk_ios(fbe_raid_geometry_t *raid_geometry_p,
                                                  fbe_payload_block_operation_t *block_op_p,
                                                  fbe_bool_t b_is_zeroing)
{
    fbe_u32_t io_weight;
    fbe_block_count_t block_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t width;
    fbe_u32_t drives_touched;
    fbe_element_size_t element_size;
    fbe_u32_t stripe_size;
    fbe_payload_block_get_block_count(block_op_p, &block_count);
    fbe_payload_block_get_opcode(block_op_p, &opcode);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    stripe_size = (width * element_size);

    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            if (block_count <= element_size) {
                drives_touched = 1;
            }
            else {
                if ((block_count / element_size) < width){
                    drives_touched = (fbe_u32_t)(block_count / element_size);
                    if (block_count % element_size){
                        drives_touched++;
                    }
                }
                else {
                    drives_touched = width;
                }
            }
            io_weight = drives_touched;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:

            if (block_count < stripe_size){
                /* Small write.
                 */
                drives_touched = (fbe_u32_t)(block_count / element_size);
                if (block_count % element_size) {
                    drives_touched++;
                }
            }
            else {
                /* Stripe size or greater that is unaligned.
                 */
                drives_touched = width;
            }
            /* The striper considers the total weight at the mirror level. 
             * Writes need to count twice as much on striped mirrors since both 
             * drives are touched. 
             */
            if (fbe_raid_geometry_is_raid10(raid_geometry_p))
            {
                io_weight = drives_touched * 2;
            }
            else
            {
                io_weight = drives_touched;
            }
            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
            io_weight = width;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            /* We can't allow these to prevent background ops from coming in. 
             * This gets the NP lock and BG ops have the np lock when then come into 
             * the block transport server.  Thus if we allow these initiate requests 
             * to hold up BG ops we will have a deadlock. 
             */
            io_weight = 0;
            break;
        default:
            io_weight = 0;
            break;
    }
    return io_weight;
}
/******************************************
 * end fbe_raid_geometry_calc_striper_disk_ios()
 ******************************************/

void fbe_raid_geometry_align_io(fbe_raid_geometry_t *raid_geometry_p,
                                fbe_lba_t * const lba_p,
                                fbe_block_count_t * const blocks_p)
{
    fbe_lba_t lba = *lba_p;
    fbe_lba_t end_lba = *lba_p + *blocks_p - 1;
    fbe_block_count_t blocks = end_lba - lba + 1;

    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {

        fbe_block_transport_align_io(FBE_520_BLOCKS_PER_4K, &lba, &blocks);

        if ((lba % FBE_520_BLOCKS_PER_4K) ||
            ((lba + blocks) % FBE_520_BLOCKS_PER_4K)) {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "raid: start: 0x%llx blocks: 0x%llx lba: %llx blocks: %llx\n",
                                   *lba_p, *blocks_p, lba, blocks);
        }
        *lba_p = lba;
        *blocks_p = blocks;
    }
}
fbe_bool_t fbe_raid_geometry_io_needs_alignment(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_lba_t lba,
                                                fbe_block_count_t blocks)
{
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        if ((lba % FBE_520_BLOCKS_PER_4K) ||
            ((lba + blocks) % FBE_520_BLOCKS_PER_4K)) {
            return FBE_TRUE;
        } else {
            return FBE_FALSE;
        }
    }
    return FBE_FALSE;
}
void fbe_raid_geometry_align_lock_request(fbe_raid_geometry_t *raid_geometry_p,
                                          fbe_lba_t * const aligned_start_lba_p,
                                          fbe_lba_t * const aligned_end_lba_p)
{
    fbe_lba_t lba = *aligned_start_lba_p;
    fbe_lba_t end_lba = *aligned_end_lba_p;
    fbe_block_count_t blocks = end_lba - lba + 1;

    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        fbe_block_transport_align_io(FBE_520_BLOCKS_PER_4K, &lba, &blocks);
        *aligned_end_lba_p = lba + blocks - 1;
        *aligned_start_lba_p = lba;
    }
}
/******************************
 * end file fbe_raid_geometry.c
 ******************************/

