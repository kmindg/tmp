/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_private_space_layout.h
 ***************************************************************************
 *
 * @brief
 *  This file defines the data types used to describe the Private Space
 *  Layout.
 *
 * @version
 *   2011-04-27 - Created. Matthew Ferson
 *
 ***************************************************************************/

#ifndef	FBE_PRIVATE_SPACE_LAYOUT_H
#define	FBE_PRIVATE_SPACE_LAYOUT_H

/*
 * INCLUDE FILES
 */
#include "../disk/interface/fbe/fbe_types.h"
#include "spid_enum_types.h"
#include "ndu_toc_common.h"
#include "fbe_private_space_layout_generated.h"

#define TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY fbe_private_space_get_minimum_system_drive_size()

#define TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY 0x60000

/*! @def FBE_PRIVATE_SPACE_LAYOUT_MAX_FRUS_PER_REGION 
  * @brief The maximum number of FRUs in a single private space region
 */
#define FBE_PRIVATE_SPACE_LAYOUT_MAX_FRUS_PER_REGION 4

/*! @def FBE_PRIVATE_SPACE_LAYOUT_MAX_DEVICE_NAME
  * @brief The maximum length of a device name to be used when exporting a LUN
 */
#define FBE_PRIVATE_SPACE_LAYOUT_MAX_DEVICE_NAME 64

/*! @def FBE_PRIVATE_SPACE_LAYOUT_MAX_REGION_NAME
  * @brief The maximum length of a nice name for a private space region
 */
#define FBE_PRIVATE_SPACE_LAYOUT_MAX_REGION_NAME 64

/*! @def FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS
  * @brief Indicates that the specified region exists on all FRUs
 */
#define FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS 0xFBE

/*! @def FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_RAID_GROUPS
  * @brief The number of c4mirror raid group to be created 
 */
#define FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_RAID_GROUPS 0x2

/*! @def FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_LUNS
  * @brief The number of c4mirror lun to be created 
 */
#define FBE_PRIVATE_SPACE_LAYOUT_C4_MIRROR_LUNS 0x2

/*! @enum fbe_private_space_layout_region_type_t
 *  @brief All of the defined private space region types
 */
typedef enum fbe_private_space_layout_region_type_e {
    FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_INVALID = 0,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_EXTERNAL_USE,
    FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_LAST
} fbe_private_space_layout_region_type_t;

/*! @enum fbe_private_space_layout_cache_flags_t
 *  @brief All of the defined private space region types
 */
typedef enum fbe_private_space_layout_flags_e {
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE                    = 0x0,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_SEP_INTERNAL            = 0x1,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_SYSTEM_CRITICAL         = 0x2,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_READ              = 0x4,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_CACHE_WRITE             = 0x8,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED  = 0x10,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_ALLOW_ADMIN_ENUMERATION = 0x20,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN              = 0x40,
    FBE_PRIVATE_SPACE_LAYOUT_FLAG_ENCRYPTED_RG            = 0x80,
} fbe_private_space_layout_flags_t;

typedef enum fbe_private_space_layout_export_lun_object_id_e {
    FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_FIRST          = 0x80,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPA = 0x80,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPB = 0x81,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPA = 0x82,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPB = 0x83,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_UTILITY_BOOT_VOLUME_SPA = 0x84,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_UTILITY_BOOT_VOLUME_SPB = 0x85,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPA = 0x86,
    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPB = 0x87,
    FBE_RPIVATE_SPACE_LAYOUT_OBJECT_ID_BOOT_VOLUME_LAST            = 0x87,
} fbe_private_space_layout_export_lun_object_id_t;

/*! @struct fbe_private_space_layout_raid_group_info_t
 *  @brief Layout information specific to RAID groups
 */
typedef struct fbe_private_space_layout_raid_group_info_s {
    /*! The RAID type of the RAID group */
    fbe_u32_t   raid_type;
    /*! The RG ID of the RAID group */
    fbe_raid_group_number_t raid_group_id;
    /*! The Object ID of the RAID group */
    fbe_object_id_t         object_id;
    /*! The exported capacity of RAID group */
    fbe_lba_t               exported_capacity;
    /*! Use for encryption */
    fbe_u32_t               flags;
} fbe_private_space_layout_raid_group_info_t;

/*! @struct fbe_private_space_layout_lun_info_t
 *  @brief Describes a private LUN
 */
typedef struct fbe_private_space_layout_lun_info_s {
    /*! The LUN number for the private LUN */
    fbe_u32_t                   lun_number;
    /*! The nicename for the private LUN */    
    fbe_char_t                  lun_name[64];
    /*! The Object ID of the LUN */
    fbe_object_id_t             object_id;
    /*! The type of image ICA may write into this LUN*/  
    fbe_u32_t                   ica_image_type;
    /*! The ID of the RAID group that contains this LUN */
    fbe_raid_group_number_t     raid_group_id;
    /*! The address offset within the RAID group where this LUN begins */
    fbe_lba_t                   raid_group_address_offset;
    /*! The capacity reserved for the LUN's user data and metadata */
    fbe_lba_t                   internal_capacity;
    /*! The capacity exported to the LUN's consumer(s) */
    fbe_lba_t                   external_capacity;
    /*! Whether or not to export a device for this LUN */
    fbe_bool_t                  export_device_b;
    /*! The name of the device to export. If empty, use the system default naming scheme. */
    fbe_char_t                  exported_device_name[FBE_PRIVATE_SPACE_LAYOUT_MAX_DEVICE_NAME]; 
    /*! Whether or not this LUN should be cached by spcache */
    fbe_u32_t                   flags;
    /*! Minimum commit level where this object should exist */
    fbe_u64_t                   commit_level;
} fbe_private_space_layout_lun_info_t;

/*! @struct fbe_private_space_layout_extended_lun_info_t
 *  @brief Describes the disk extents used by a private LUN
 */
typedef struct fbe_private_space_layout_extended_lun_info_s {
    /*! The number of FRUs used by this region */
    fbe_u32_t   number_of_frus;
    /*! The IDs of the FRUs used by this region */
    fbe_u32_t   fru_id[FBE_PRIVATE_SPACE_LAYOUT_MAX_FRUS_PER_REGION];
    /*! The LBA on disk where this region begins */
    fbe_lba_t   starting_block_address; 
    /*! This size on disk of this region, in blocks */
    fbe_lba_t   size_in_blocks; 
} fbe_private_space_layout_extended_lun_info_t;

/*! @struct fbe_private_space_layout_region_t
 *  @brief Describes a private space region
 */
typedef struct fbe_private_space_layout_region_s {
    /*! The ID of the region */
    fbe_private_space_layout_region_id_t region_id;
    /*! The nicename of the region */
    fbe_char_t  region_name[FBE_PRIVATE_SPACE_LAYOUT_MAX_REGION_NAME];
    /*! The type of the region */
    fbe_private_space_layout_region_type_t region_type;
    /*! The number of FRUs used by this region */
    fbe_u32_t   number_of_frus;
    /*! The IDs of the FRUs used by this region */
    fbe_u32_t   fru_id[FBE_PRIVATE_SPACE_LAYOUT_MAX_FRUS_PER_REGION];
    /*! The LBA on disk where this region begins */
    fbe_lba_t   starting_block_address; 
    /*! This size on disk of this region, in blocks */
    fbe_lba_t   size_in_blocks; 
        /*! RAID-group specific information */
    fbe_private_space_layout_raid_group_info_t raid_info; 
} fbe_private_space_layout_region_t;

/*! @struct fbe_private_space_layout_t
 *  @brief Describes the entire system's private space layout
 */
typedef struct fbe_private_space_layout_s {
    /*! A table of Regions */
    fbe_private_space_layout_region_t   regions[FBE_PRIVATE_SPACE_LAYOUT_MAX_REGIONS];
    /*! A table of LUNs */
    fbe_private_space_layout_lun_info_t luns[FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS]; 
} fbe_private_space_layout_t;



/*!
 *  @brief Determine if the specified object ID is the BVD interface
 *  
 *  @return True if the specified object ID is the BVD interface; false if it is not
 */
static __inline fbe_bool_t fbe_private_space_layout_object_id_is_bvd_interface(fbe_object_id_t object_id)
{
    if(object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_BVD_INTERFACE)
    {
        return(FBE_TRUE);
    }
    else
    {
        return(FBE_FALSE);
    }
}

/*!
 *  @brief Determine if the specified object ID is a system PVD object
 *  
 *  @return True if the specified object ID is a system PVD object; false if it is not
 */
static __inline fbe_bool_t fbe_private_space_layout_object_id_is_system_pvd(fbe_object_id_t object_id)
{
    if((object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST)
       && (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_LAST))
    {
        return(FBE_TRUE);
    }
    else
    {
        return(FBE_FALSE);
    }
}

/*!
 *  @brief Determine if the specified object ID is a system spare PVD object
 *  
 *  @return True if the specified object ID is a system spare PVD object; false if it is not
 */
static __inline fbe_bool_t fbe_private_space_layout_object_id_is_system_spare_pvd(fbe_object_id_t object_id)
{
    if((object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_FIRST)
       && (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_SPARE_LAST))
    {
        return(FBE_TRUE);
    }
    else
    {
        return(FBE_FALSE);
    }
}

/*!
 *  @brief Determine if the specified object ID is a system VD object
 *  
 *  @return True if the specified object ID is a system VD object; false if it is not
 */
static __inline fbe_bool_t fbe_private_space_layout_object_id_is_system_vd(fbe_object_id_t object_id)
{
    if((object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_FIRST)
       && (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_LAST))
    {
        return(FBE_TRUE);
    }
    else
    {
        return(FBE_FALSE);
    }
}


/*!
 *  @brief Determine if the specified object ID is a system RG object
 *  
 *  @return True if the specified object ID is a system RG object; false if it is not
 */
static __inline fbe_bool_t fbe_private_space_layout_object_id_is_system_raid_group(fbe_object_id_t object_id)
{
    if((object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_FIRST)
       && (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_LAST))
    {
        return(FBE_TRUE);
    }
    else if((object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPA)
       && (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPB))
    {
        return(FBE_TRUE);
    }
    else
    {
        return(FBE_FALSE);
    }
}

/*!
 *  @brief Determine if the specified object ID is a system LUN object
 *  
 *  @return True if the specified object ID is a system LUN object; false if it is not
 */
static __inline fbe_bool_t fbe_private_space_layout_object_id_is_system_lun(fbe_object_id_t object_id)
{
    if((object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST)
       && (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST))
    {
        return(FBE_TRUE);
    }
    else if((object_id >= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_UTILITY_BOOT_VOLUME_SPA)
       && (object_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPB))
    {
        return(FBE_TRUE);
    }
    else
    {
        return(FBE_FALSE);
    }
}

/* Function declarations; implemented in fbe_private_space_layout.c */
fbe_status_t fbe_private_space_layout_get_region_by_index(
    fbe_u32_t, fbe_private_space_layout_region_t *);

fbe_status_t fbe_private_space_layout_get_lun_by_index(
    fbe_u32_t, fbe_private_space_layout_lun_info_t *);

fbe_status_t fbe_private_space_layout_get_region_by_object_id(
    fbe_object_id_t, fbe_private_space_layout_region_t *);

fbe_status_t fbe_private_space_layout_get_lun_by_object_id(
    fbe_object_id_t, fbe_private_space_layout_lun_info_t *);

fbe_status_t fbe_private_space_layout_get_region_by_region_id(
    fbe_private_space_layout_region_id_t, fbe_private_space_layout_region_t *);

fbe_status_t fbe_private_space_layout_get_lun_by_lun_number(
    fbe_u32_t, fbe_private_space_layout_lun_info_t *);

fbe_status_t fbe_private_space_layout_get_lun_extended_info_by_lun_number(
    fbe_lun_number_t, fbe_private_space_layout_extended_lun_info_t *);

fbe_status_t fbe_private_space_layout_get_region_by_raid_group_id(
    fbe_u32_t, fbe_private_space_layout_region_t *);

fbe_status_t fbe_private_space_layout_get_start_of_user_space(
    fbe_u32_t, fbe_lba_t *);

fbe_bool_t fbe_private_space_layout_does_region_use_fru(
    fbe_private_space_layout_region_id_t region_id, fbe_u32_t fru_id);

fbe_status_t fbe_private_space_layout_is_lun_enumerable_by_admin(fbe_object_id_t, fbe_bool_t *);

fbe_block_count_t fbe_private_space_get_minimum_system_drive_size(void);
fbe_block_count_t fbe_private_space_get_capacity_imported_from_pvd(void);
fbe_u32_t fbe_private_space_layout_get_number_of_system_drives(void);
fbe_u32_t fbe_private_space_layout_get_number_of_system_raid_groups(void);
fbe_u32_t fbe_private_space_layout_get_number_of_system_luns(void);
fbe_status_t fbe_private_space_layout_initialize_library(SPID_HW_TYPE hw_type);
fbe_status_t fbe_private_space_layout_initialize_fbe_sim(void);
fbe_status_t fbe_private_space_layout_get_smallest_lun_number(fbe_u32_t *lun_number);
fbe_status_t fbe_private_space_layout_get_smallest_rg_number(fbe_u32_t *rg_number);
fbe_status_t fbe_private_space_layout_is_lun_on_tripple_mirror(fbe_object_id_t lu_id, fbe_bool_t *is_on_tripple_mirror);
fbe_bool_t fbe_private_space_layout_object_id_is_system_object(fbe_object_id_t object_id);
fbe_status_t fbe_private_space_layout_get_number_of_system_objects(fbe_u32_t *system_object_count, fbe_u64_t current_commit_level);

#endif /* FBE_PRIVATE_SPACE_LAYOUT_H */
