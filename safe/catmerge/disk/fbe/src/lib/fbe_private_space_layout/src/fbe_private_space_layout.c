/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_private_space_layout.c
 *
 * @brief
 *  This file describes the Private Space Layout, and contains functions
 *  used to access it.
 *
 * @version
 *   2011-04-27 - Created. Matthew Ferson
 *
 ***************************************************************************/

/*
 * INCLUDE FILES
 */
#include "fbe_private_space_layout.h"
#include "fbe_imaging_structures.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_raid_group.h"
#ifndef UEFI_ENV
#include "fbe_provision_drive_private.h"
#include "spid_enum_types.h"
#include "EmcPAL_Misc.h"
#endif  /* ifndef UEFI_ENV */

#include "fbe_private_space_layout_rockies_hw.c"
#include "fbe_private_space_layout_rockies_sim.c"
#include "fbe_private_space_layout_kittyhawk_hw.c"
#include "fbe_private_space_layout_kittyhawk_sim.c"
#include "fbe_private_space_layout_fbe_sim.c"
#ifdef C4_INTEGRATED
#include "fbe_private_space_layout_haswell_hw.c"
#include "fbe_private_space_layout_haswell_sim.c"
#include "fbe_private_space_layout_virtual.c"
#endif

fbe_private_space_layout_t * fbe_private_space_layout_active_p = NULL;

#define CHECK_LIBRARY_INITIALIZED_RETURN_ERROR \
if(fbe_private_space_layout_active_p == NULL) { \
    return(FBE_STATUS_NOT_INITIALIZED); \
}

#define CHECK_LIBRARY_INITIALIZED_RETURN_INVALID_LBA \
if(fbe_private_space_layout_active_p == NULL) { \
    return(FBE_LBA_INVALID); \
}

#define CHECK_LIBRARY_INITIALIZED_RETURN_ZERO \
if(fbe_private_space_layout_active_p == NULL) { \
    return(0); \
}

/*!
 * The number of system raid group and lun which are not defined in private space layout.
 */
static fbe_u32_t num_internal_system_luns = 0;
static fbe_u32_t num_internal_system_raid_groups = 0;

/*! 
 *  @brief Locates the private space region located at
 *         the specified index
 *  
 *  @param index (IN)
 *  @param *region (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_region_by_index(
    fbe_u32_t index, fbe_private_space_layout_region_t *region)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    if(index < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS)
    {
        *region = fbe_private_space_layout_active_p->regions[index];
        status  = FBE_STATUS_OK;
    }

    return(status);
}

/*! 
 *  @brief Locates the private lun located at the specified index
 *  
 *  @param index (IN)
 *  @param *lun (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_lun_by_index(
    fbe_u32_t index, fbe_private_space_layout_lun_info_t *lun)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    if(index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS)
    {
        *lun    = fbe_private_space_layout_active_p->luns[index];
        status  = FBE_STATUS_OK;
    }

    return(status);
}

/*! 
 *  @brief Locates the private space region with the specified
 *         Region ID
 *  
 *  @param region_id (IN)
 *  @param *region (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_region_by_region_id(
    fbe_private_space_layout_region_id_t region_id, fbe_private_space_layout_region_t *region)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        if(fbe_private_space_layout_active_p->regions[i].region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        if(fbe_private_space_layout_active_p->regions[i].region_id == region_id)
        {
            *region = fbe_private_space_layout_active_p->regions[i];
            status  = FBE_STATUS_OK;
            return(status);
        }
    }

    return(status);
}

/*! 
 *  @brief Locates the private lun with the specified
 *         LUN Number
 *  
 *  @param lun_number (IN)
 *  @param *lun (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_lun_by_lun_number(
    fbe_lun_number_t lun_number, fbe_private_space_layout_lun_info_t *lun)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; i++ )
    {
        if(fbe_private_space_layout_active_p->luns[i].lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
        {
            break;
        }
        if(fbe_private_space_layout_active_p->luns[i].lun_number == lun_number)
        {
            *lun    = fbe_private_space_layout_active_p->luns[i];
            status  = FBE_STATUS_OK;
            return(status);
        }
    }

    return(status);
}

/*! 
 *  @brief Locates the private lun with the specified
 *         LUN Number, and returns its extended information
 *  
 *  @param lun_number (IN)
 *  @param *lun_ext (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_lun_extended_info_by_lun_number(
    fbe_lun_number_t lun_number, fbe_private_space_layout_extended_lun_info_t *lun_ext)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_lun_info_t lun;
    fbe_private_space_layout_region_t region;
    fbe_u32_t i;
    fbe_u32_t data_disks;
    fbe_lba_t blocks_per_fru;
    fbe_lba_t fru_address_offset;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    status = fbe_private_space_layout_get_lun_by_lun_number(lun_number, &lun);
    if(status != FBE_STATUS_OK)
    {
            return(status);
    }

    status = fbe_private_space_layout_get_region_by_raid_group_id(lun.raid_group_id, &region);
    if(status != FBE_STATUS_OK)
    {
            return(status);
    }

    if(region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID1)
    {
        data_disks = 1;
    }
    else if(region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        data_disks = region.number_of_frus;
    }
    else if(region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        data_disks = region.number_of_frus / 2;
    }
    else if(region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID3)
    {
        data_disks = region.number_of_frus - 1;
    }
    else if(region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID5)
    {
        data_disks = region.number_of_frus - 1;
    }
    else if(region.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        data_disks = region.number_of_frus - 1;
    }
    else {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    blocks_per_fru          = (lun.internal_capacity / data_disks);
    fru_address_offset      = (lun.raid_group_address_offset / data_disks) + region.starting_block_address;

    /* Fill in the supplied buffer */
    lun_ext->number_of_frus         = region.number_of_frus;
    lun_ext->size_in_blocks         = blocks_per_fru;
    lun_ext->starting_block_address = fru_address_offset;

    for(i = 0; i < region.number_of_frus; i++)
    {
        lun_ext->fru_id[i] = region.fru_id[i];
    }

    return(status);
}

/*! 
 *  @brief Locates the private space region associated with a
 *         private RAID group, given the RAID Group ID
 *  
 *  @param rg_id (IN)
 *  @param *region (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_region_by_raid_group_id(
    fbe_u32_t rg_id, fbe_private_space_layout_region_t *region)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        if(fbe_private_space_layout_active_p->regions[i].region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        if(fbe_private_space_layout_active_p->regions[i].raid_info.raid_group_id == rg_id)
        {
            *region = fbe_private_space_layout_active_p->regions[i];
            status  = FBE_STATUS_OK;
            return(status);
        }
    }

    return(status);
}

/*! 
 *  @brief Locates the private lun with the specified
 *         Object ID
 *  
 *  @param object_id (IN)
 *  @param *lun (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_lun_by_object_id(
    fbe_object_id_t object_id, fbe_private_space_layout_lun_info_t *lun)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; i++ )
    {
        if(fbe_private_space_layout_active_p->luns[i].lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
        {
            break;
        }
        if(fbe_private_space_layout_active_p->luns[i].object_id == object_id)
        {
            *lun    = fbe_private_space_layout_active_p->luns[i];
            status  = FBE_STATUS_OK;
            return(status);
        }
    }

    return(status);
}

/*! 
 *  @brief Locates the private space region associated with a
 *         private RAID group, given the Object ID of the RAID Group
 *  
 *  @param object_id (IN)
 *  @param *region (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_region_by_object_id(
    fbe_object_id_t object_id, fbe_private_space_layout_region_t *region)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        if(fbe_private_space_layout_active_p->regions[i].region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        if(fbe_private_space_layout_active_p->regions[i].raid_info.object_id == object_id)
        {
            *region = fbe_private_space_layout_active_p->regions[i];
            status  = FBE_STATUS_OK;
            return(status);
        }
    }

    return(status);
}

/*!
 *  @brief Returns the minimum required drive capacity, in blocks,
 *  for a system drive.
 *  
 *  @return The minimum required drive capacity, in blocks
 */
#ifndef UEFI_ENV
fbe_block_count_t fbe_private_space_get_minimum_system_drive_size()
{
    fbe_lba_t lba = 0;
    fbe_lba_t overall_chunks = 0;
    fbe_lba_t bitmap_entries_per_block = 0;
    fbe_lba_t paged_bitmap_blocks = 0;
    fbe_lba_t paged_bitmap_capacity = 0;

    CHECK_LIBRARY_INITIALIZED_RETURN_INVALID_LBA

    lba = fbe_private_space_get_capacity_imported_from_pvd();

    /* This code is duplicated from fbe_provision_drive_class.c, 
       fbe_provision_drive_calculate_capacity() and  fbe_class_provision_drive_get_metadata_positions()
     */
    overall_chunks              = (lba + FBE_PROVISION_DRIVE_CHUNK_SIZE - 1) / FBE_PROVISION_DRIVE_CHUNK_SIZE;
    bitmap_entries_per_block    = FBE_METADATA_BLOCK_DATA_SIZE /  sizeof(fbe_provision_drive_paged_metadata_t);
    paged_bitmap_blocks         = (overall_chunks + (bitmap_entries_per_block - 1)) / bitmap_entries_per_block;
    if((paged_bitmap_blocks % FBE_PROVISION_DRIVE_CHUNK_SIZE) != 0)
    {
        paged_bitmap_blocks += FBE_PROVISION_DRIVE_CHUNK_SIZE - (paged_bitmap_blocks % FBE_PROVISION_DRIVE_CHUNK_SIZE);
    }

    paged_bitmap_capacity = paged_bitmap_blocks * FBE_PROVISION_DRIVE_NUMBER_OF_METADATA_COPIES;

    lba += paged_bitmap_capacity;

    return((fbe_block_count_t)lba);
}
#endif  /* #ifndef UEFI_ENV */

/*!
 *  @brief Returns the LBA on the specified FRU where space available for
 *  user data begins.
 *  
 *  @return The minimum required drive capacity, in blocks
 */
fbe_block_count_t fbe_private_space_get_capacity_imported_from_pvd()
{
    fbe_status_t status;
    fbe_lba_t lba = 0;
    fbe_lba_t temp_lba = 0;
    fbe_u32_t i;
    fbe_u32_t system_drives;

    CHECK_LIBRARY_INITIALIZED_RETURN_INVALID_LBA

    system_drives = fbe_private_space_layout_get_number_of_system_drives();

    for(i = 0; i < system_drives; i++) {
        status = fbe_private_space_layout_get_start_of_user_space(i, &temp_lba);
        if((status == FBE_STATUS_OK) && (temp_lba > lba)) {
            lba = temp_lba;
        }
    }

    return((fbe_block_count_t)lba);
}

/*!
 *  @brief Returns the LBA on the specified FRU where space available for
 *  user data begins.
 *  
 *  @param fru_number (IN)
 *  @param *lba (OUT)
 *  
 *  @return Always returns FBE_STATUS_OK
 */
fbe_status_t fbe_private_space_layout_get_start_of_user_space(
    fbe_u32_t fru_number, fbe_lba_t *lba)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t i; // index into regions
    fbe_u32_t j; // index into FRUs
    fbe_bool_t fru_in_use;
    fbe_private_space_layout_region_t *reg_ptr = NULL;
    *lba = 0;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        fru_in_use  = FBE_FALSE;
        reg_ptr     = &fbe_private_space_layout_active_p->regions[i];

        if(reg_ptr->region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        // Determine whether or not this region exists on the specified FRU 
        if(reg_ptr->number_of_frus == FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS)
        {
            fru_in_use = FBE_TRUE;
        }
        else {
            for(j = 0; j <reg_ptr->number_of_frus; j++)
            {
                if(reg_ptr->fru_id[j] == fru_number)
                {
                    fru_in_use = FBE_TRUE;
                }
            }
        }

        // If this region uses the specified FRU, find where it ends
        if(fru_in_use)
        {
            if((reg_ptr->starting_block_address + reg_ptr->size_in_blocks) > *lba)
            {
                *lba = (reg_ptr->starting_block_address + reg_ptr->size_in_blocks);
            }
        }
    }
    return(status);
}

/*!
 *  @brief Returns the LBA on the specified FRU where space available for
 *  user data begins.
 *  
 *  @param region_id (IN)
 *  @param fru_id (IN)
 *  
 *  @return FBE_TRUE if the region uses the specified FRU, FBE_FALSE if it does not.
 */
fbe_bool_t fbe_private_space_layout_does_region_use_fru(fbe_private_space_layout_region_id_t region_id, fbe_u32_t fru_id)
{
    fbe_bool_t includes_target  = FBE_FALSE;
    fbe_status_t status         = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t region;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ZERO

    status = fbe_private_space_layout_get_region_by_region_id(region_id, &region);
    if(status == FBE_STATUS_OK)
    {
        if(region.number_of_frus == FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS)
        {
            includes_target = FBE_TRUE;
        }
        else {
            for(i = 0; i < region.number_of_frus; i++)
            {
                if(region.fru_id[i] == fru_id)
                {
                    includes_target = FBE_TRUE;
                }
            }
        }
    }

    return(includes_target);
}

/*!
 *  @brief Returns the number of system drives.
 *  
 *  @return The number of system drives
 */
fbe_u32_t fbe_private_space_layout_get_number_of_system_drives()
{
    fbe_u32_t max_fru = 0;
    fbe_u32_t i;
    fbe_u32_t j;
    fbe_private_space_layout_region_t * region_p = NULL;

    CHECK_LIBRARY_INITIALIZED_RETURN_ZERO

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        region_p = &fbe_private_space_layout_active_p->regions[i];

        if(region_p->region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        else if(region_p->number_of_frus != FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS)
        {
            for(j = 0; j < region_p->number_of_frus; j++)
            {
                if(region_p->fru_id[j] > max_fru)
                {
                    max_fru = region_p->fru_id[j];
                }
            }
        }
    }

    return(max_fru + 1);
}

/*!
 *  @brief Returns the number of system raid groups.
 *  
 *  @return The number of system raid groups
 */
fbe_u32_t fbe_private_space_layout_get_number_of_system_raid_groups()
{
    fbe_u32_t count = 0;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ZERO

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        if(fbe_private_space_layout_active_p->regions[i].region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        else if(fbe_private_space_layout_active_p->regions[i].region_type == FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP)
        {
            count++;
        }
    }
    count += num_internal_system_raid_groups;
    return(count);
}

/*!
 *  @brief Returns the number of system LUNs.
 *  
 *  @return The number of system LUNs
 */
fbe_u32_t fbe_private_space_layout_get_number_of_system_luns()
{
    fbe_u32_t count = 0;
    fbe_u32_t i;

    CHECK_LIBRARY_INITIALIZED_RETURN_ZERO

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; i++ )
    {
        if(fbe_private_space_layout_active_p->luns[i].lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
        {
            break;
        }
        else
        {
            count++;
        }
    }
    count += num_internal_system_luns;
    return(count);
}

/*!
 *  @brief Determine if the specified object ID is a system LUN object
 *  
 *  @return FBE_STATUS_OK on success
 */
fbe_status_t fbe_private_space_layout_initialize_library(SPID_HW_TYPE hw_type)
{
    switch(hw_type)
    {
        /* Moons Platforms */
        case SPID_ENTERPRISE_HW_TYPE:
        /* Rockies Platforms */
        case SPID_PROMETHEUS_M1_HW_TYPE:
        case SPID_DEFIANT_M1_HW_TYPE:
        case SPID_DEFIANT_M2_HW_TYPE:
        case SPID_DEFIANT_M3_HW_TYPE:
        case SPID_DEFIANT_M4_HW_TYPE:
        case SPID_DEFIANT_M5_HW_TYPE:
        case SPID_DEFIANT_K2_HW_TYPE:
        case SPID_DEFIANT_K3_HW_TYPE:
#ifdef C4_INTEGRATED
            num_internal_system_luns = FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_LUNS;
            num_internal_system_raid_groups = FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_RAID_GROUPS;
            fbe_private_space_layout_active_p = &fbe_private_space_layout_haswell_hw;
#else
            fbe_private_space_layout_active_p = &fbe_private_space_layout_rockies_hw;
#endif /* C4_INTEGRATED - C4ARCH - disk config */
            return FBE_STATUS_OK;
            break;
        /* Kittyhawk Platforms */
        case SPID_NOVA1_HW_TYPE:
        case SPID_NOVA3_HW_TYPE:
        case SPID_NOVA3_XM_HW_TYPE:
        case SPID_BEARCAT_HW_TYPE:
            num_internal_system_luns = FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_LUNS;
            num_internal_system_raid_groups = FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_RAID_GROUPS;
            fbe_private_space_layout_active_p = &fbe_private_space_layout_kittyhawk_hw;
            return FBE_STATUS_OK;
            break;
#ifdef C4_INTEGRATED
// ENCL_CLEANUP - do we need a new PSL for Moons
        case SPID_OBERON_1_HW_TYPE:
        case SPID_OBERON_2_HW_TYPE:
        case SPID_OBERON_3_HW_TYPE:
        case SPID_OBERON_4_HW_TYPE:
        case SPID_HYPERION_1_HW_TYPE:
        case SPID_HYPERION_2_HW_TYPE:
        case SPID_HYPERION_3_HW_TYPE:
        case SPID_TRITON_1_HW_TYPE:
            num_internal_system_luns = FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_LUNS;
            num_internal_system_raid_groups = FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_RAID_GROUPS;
            fbe_private_space_layout_active_p = &fbe_private_space_layout_haswell_hw;
            return FBE_STATUS_OK;
            break;
#endif            
        /* Rockies Simulators */
        case SPID_PROMETHEUS_S1_HW_TYPE:
        case SPID_DEFIANT_S1_HW_TYPE:
        case SPID_DEFIANT_S4_HW_TYPE:
#ifdef C4_INTEGRATED
            fbe_private_space_layout_active_p = &fbe_private_space_layout_kittyhawk_sim;
#else
            fbe_private_space_layout_active_p = &fbe_private_space_layout_rockies_sim;
#endif /* C4_INTEGRATED - C4ARCH - disk config */
            return FBE_STATUS_OK;
            break;
        /* Kittyhawk Simulators */
        case SPID_NOVA_S1_HW_TYPE:
            fbe_private_space_layout_active_p = &fbe_private_space_layout_kittyhawk_sim;
            return FBE_STATUS_OK;
            break;
#ifdef C4_INTEGRATED
        case SPID_OBERON_S1_HW_TYPE:
            fbe_private_space_layout_active_p = &fbe_private_space_layout_haswell_sim;
            return FBE_STATUS_OK;
            break;
        /* Virtual platform (Virtual VNX) */
        case SPID_MERIDIAN_ESX_HW_TYPE:
        case SPID_TUNGSTEN_HW_TYPE:
            fbe_private_space_layout_active_p = &fbe_private_space_layout_virtual;
            return FBE_STATUS_OK;
            break;
#endif
        /* Default - Error Case */
        default:
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    };
}

/*! 
 *  @brief find the smallest LUN number
 *  
 *  @param *lun_number (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_smallest_lun_number(fbe_u32_t *lun_number)
{
    fbe_u32_t 		i = 0;
	fbe_u32_t		smallest = FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; i++ )
    {
        if((fbe_private_space_layout_active_p->luns[i].lun_number != 0) &&
		   (fbe_private_space_layout_active_p->luns[i].lun_number < smallest))
        {
            smallest = fbe_private_space_layout_active_p->luns[i].lun_number;
        }
    }

	*lun_number = smallest;

    return(FBE_STATUS_OK);
}


/*! 
 *  @brief find the smallest RG number
 *  
 *  @param *rg_number (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_get_smallest_rg_number(fbe_u32_t *rg_number)
{
    fbe_u32_t 		i = 0;
	fbe_u32_t		smallest = FBE_PRIVATE_SPACE_LAYOUT_RG_ID_INVALID;

    CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS; i++ )
    {
        if(fbe_private_space_layout_active_p->regions[i].region_id == FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID)
        {
            break;
        }
        if((fbe_private_space_layout_active_p->regions[i].raid_info.raid_group_id != 0) && 
		   (fbe_private_space_layout_active_p->regions[i].raid_info.raid_group_id < smallest))
        {
            smallest = fbe_private_space_layout_active_p->regions[i].raid_info.raid_group_id;
        }
    }

	*rg_number = smallest;

    return(FBE_STATUS_OK);
}

/*! 
 *  @brief is this lun sitting on a tripple mirror ?

 *  @param lu_id (IN)
 *  @param *is_on_tripple_mirror (OUT)
 *  
 *  @return fbe_status_t
 */
fbe_status_t fbe_private_space_layout_is_lun_on_tripple_mirror(fbe_object_id_t lu_id, fbe_bool_t *is_on_tripple_mirror)
{
    fbe_status_t						status;
	fbe_private_space_layout_lun_info_t	lu_info;

	CHECK_LIBRARY_INITIALIZED_RETURN_ERROR

    /*is it even a system lun*/
	if (!fbe_private_space_layout_object_id_is_system_lun(lu_id)){
		*is_on_tripple_mirror = FBE_FALSE;
		return FBE_STATUS_OK;
	}

    status = fbe_private_space_layout_get_lun_by_object_id(lu_id, &lu_info);
	if (status != FBE_STATUS_OK) {
		return status;
	}

	*is_on_tripple_mirror = (lu_info.raid_group_id == FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR) ? FBE_TRUE : FBE_FALSE;

	return FBE_STATUS_OK;
}


fbe_status_t fbe_private_space_layout_is_lun_enumerable_by_admin(fbe_object_id_t lun_id, fbe_bool_t *is_enumerable)
{
    fbe_status_t                        status;
    fbe_private_space_layout_lun_info_t lun_info;

    status = fbe_private_space_layout_get_lun_by_object_id(lun_id, &lun_info);

    if (status != FBE_STATUS_OK) {
        return status;
    }

    *is_enumerable = (lun_info.flags & FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION) ? FBE_TRUE : FBE_FALSE;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_private_space_layout_get_number_of_system_objects(
    fbe_u32_t *system_object_count, 
    fbe_u64_t current_commit_level)
{
    fbe_u32_t i;
    fbe_private_space_layout_lun_info_t lun_info;

    *system_object_count = 0;

    for(i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST; i++)
    {
        if(fbe_private_space_layout_object_id_is_bvd_interface(i))
        {
            *system_object_count += 1;
        }
        else if(fbe_private_space_layout_object_id_is_system_pvd(i))
        {
            *system_object_count += 1;
        }
        else if(fbe_private_space_layout_object_id_is_system_raid_group(i))
        {
            *system_object_count += 1;
        }
        else if(fbe_private_space_layout_object_id_is_system_lun(i))
        {
            fbe_private_space_layout_get_lun_by_object_id(i, &lun_info);
            if(lun_info.commit_level <= current_commit_level)
            {
                *system_object_count += 1;
            }
            
        }
    }
    *system_object_count += num_internal_system_luns;
    *system_object_count += num_internal_system_raid_groups;
    return FBE_STATUS_OK;
}

/*!
 *  @brief Determine if the specified object ID is a system object
 *  
 *  @return True if the specified object ID is a system object; false if it is not
 */
fbe_bool_t fbe_private_space_layout_object_id_is_system_object(fbe_object_id_t object_id)
{
        if(fbe_private_space_layout_object_id_is_bvd_interface(object_id))
        {
            return FBE_TRUE;
        }
        else if(fbe_private_space_layout_object_id_is_system_pvd(object_id))
        {
            return FBE_TRUE;
        }
        else if(fbe_private_space_layout_object_id_is_system_raid_group(object_id))
        {
            return FBE_TRUE;
        }
        else if(fbe_private_space_layout_object_id_is_system_lun(object_id))
        {
            return FBE_TRUE;
        }
        return FBE_FALSE;
}
fbe_status_t fbe_private_space_layout_initialize_fbe_sim(void)
{
    fbe_private_space_layout_active_p = &fbe_private_space_layout_fbe_sim;
    return FBE_STATUS_OK;
}
