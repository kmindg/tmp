/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_database_export_lun.c
 ***************************************************************************
 *
 * @brief
 *  This file contains export lun related code.
 *  
 * @version
 *  08/29/2014 - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_database_common.h"
#include "fbe_database_private.h"
#include "fbe_database_drive_connection.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_config_tables.h"
#include "fbe/fbe_board.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_notification_lib.h"
#include "fbe_database_metadata_interface.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_database_homewrecker_db_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_database_registry.h"
#include "fbe_cmi.h"


typedef struct export_lun_zero_progress_s {
    fbe_semaphore_t                   sem;
    fbe_atomic_t                      outstanding_count;
}export_lun_zero_progress_t;

#define MIRROR_RG_NUM   4

/*****************************************
 * LOCAL VARIABLES
 *****************************************/
fbe_private_space_layout_region_t export_lun_regions[] = 
{
    {
        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPA,
        "UTILITY_BOOT_VOLUME_SPA",
        FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
        2, /* Number of FRUs */
        {
            1, 3, /* FRUs */
        },
        0x00000000, /* Starting LBA */
        0x00000000, /* Size */
        {
            FBE_RAID_GROUP_TYPE_RAID1,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_UTILITY_BOOT_VOLUME_SPA,
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPA,
            0x00000000, /* Exported Capacity */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE,  /* Set Encrypt/Un-Encrypt Flag for future usage */
        }
    },
    {
        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_UTILITY_BOOT_VOLUME_SPB,
        "UTILITY_BOOT_VOLUME_SPB",
        FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
        2, /* Number of FRUs */
        {
            0, 2, /* FRUs */
        },
        0x00000000, /* Starting LBA */
        0x00000000, /* Size */
        {
            FBE_RAID_GROUP_TYPE_RAID1,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_UTILITY_BOOT_VOLUME_SPB,
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPB,
            0x00000000, /* Exported Capacity */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE,  /* Set Encrypt/Un-Encrypt Flag for future usage */
        }
    },
    {
        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPA,
        "PRIMARY_BOOT_VOLUME_SPA",
        FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
        2, /* Number of FRUs */
        {
            0, 2, /* FRUs */
        },
        0x00000000, /* Starting LBA */
        0x00000000, /* Size */
        {
            FBE_RAID_GROUP_TYPE_RAID1,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_PRIMARY_BOOT_VOLUME_SPA,
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPA,
            0x00000000, /* Exported Capacity */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE,  /* Set Encrytion/Un-Encryption Flag for future usage */
        }
    },
    {
        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_PRIMARY_BOOT_VOLUME_SPB,
        "PRIMARY_BOOT_VOLUME_SPB",
        FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_RAID_GROUP,
        2, /* Number of FRUs */
        {
            1, 3, /* FRUs */
        },
        0x00000000, /* Starting LBA */
        0x00000000, /* Size */
        {
            FBE_RAID_GROUP_TYPE_RAID1,
            FBE_PRIVATE_SPACE_LAYOUT_RG_ID_PRIMARY_BOOT_VOLUME_SPB,
            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPB,
            0x00000000, /* Exported Capacity */
            FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE,  /* Set Encrypt/Un-Encrypt Flag for future usage */
        }
    },
    /* Terminator */
    {
        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID,
        "",
        FBE_PRIVATE_SPACE_LAYOUT_REGION_TYPE_UNUSED,
        0, /* Number of FRUs */
        {
            0, /* FRUs */
        },
        0x0, /* Starting LBA */
        0x0, /* Size in Blocks */
    },
};

fbe_private_space_layout_lun_info_t export_lun_luns[] =
{
    {
        FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_UTILITY_BOOT_VOLUME_SPA,
        "UTILITY_BOOT_VOLUME_SPA",
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_UTILITY_BOOT_VOLUME_SPA, /* Object ID */
        FBE_IMAGING_IMAGE_TYPE_INVALID,
        FBE_PRIVATE_SPACE_LAYOUT_RG_ID_UTILITY_BOOT_VOLUME_SPA, /* RG ID */
        0x00000000, /* Address Offset within RG */
        0x00000000, /* Internal Capacity */
        0x00000000, /* External Capacity */
        FBE_TRUE, /* Export as Device */
        "mirrorb", /* Device Name */
        FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN, /* Flags */
        FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
    },
    {
        FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_UTILITY_BOOT_VOLUME_SPB,
        "UTILITY_BOOT_VOLUME_SPB",
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_UTILITY_BOOT_VOLUME_SPB, /* Object ID */
        FBE_IMAGING_IMAGE_TYPE_INVALID,
        FBE_PRIVATE_SPACE_LAYOUT_RG_ID_UTILITY_BOOT_VOLUME_SPB, /* RG ID */
        0x00000000, /* Address Offset within RG */
        0x00000000, /* Internal Capacity */
        0x00000000, /* External Capacity */
        FBE_TRUE, /* Export as Device */
        "mirrorb", /* Device Name */
        FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN, /* Flags */
        FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
    },
    {
        FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_PRIMARY_BOOT_VOLUME_SPA,
        "PRIMARY_BOOT_VOLUME_SPA",
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPA, /* Object ID */
        FBE_IMAGING_IMAGE_TYPE_INVALID,
        FBE_PRIVATE_SPACE_LAYOUT_RG_ID_PRIMARY_BOOT_VOLUME_SPA, /* RG ID */
        0x00000000, /* Address Offset within RG */
        0x00000000, /* Internal Capacity */
        0x00000000, /* External Capacity */
        FBE_TRUE, /* Export as Device */
        "mirrora", /* Device Name */
        FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN, /* Flags */
        FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
    },
    {
        FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_PRIMARY_BOOT_VOLUME_SPB,
        "PRIMARY_BOOT_VOLUME_SPB",
        FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPB, /* Object ID */
        FBE_IMAGING_IMAGE_TYPE_INVALID,
        FBE_PRIVATE_SPACE_LAYOUT_RG_ID_PRIMARY_BOOT_VOLUME_SPB, /* RG ID */
        0x00000000, /* Address Offset within RG */
        0x00000000, /* Internal Capacity */
        0x00000000, /* External Capacity */
        FBE_TRUE, /* Export as Device */
        "mirrora", /* Device Name */
        FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED | FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN, /* Flags */
        FLARE_COMMIT_LEVEL_ROCKIES, /* Commit Level*/
    },
    /* Terminator*/
    {
        FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID,
        "",
        0x0,                /* Object ID */
        FBE_IMAGING_IMAGE_TYPE_INVALID,
        0x0,                /* RG ID */
        0x0,                /* Address Offset within RG */
        0x0,                /* Internal Capacity */
        0x0,                /* External Capacity */
        FBE_FALSE,          /* Export as Device */
        "",                 /* Device Name */
        FBE_PRIVATE_SPACE_LAYOUT_FLAG_NONE, /* Flags */
    },
};

extern fbe_status_t database_system_objects_create_c4_mirror_rg_from_psl(fbe_database_service_t *fbe_database_service,
                                                               fbe_object_id_t object_id, 
                                                               fbe_private_space_layout_region_t *region);
extern fbe_status_t database_system_objects_create_c4_mirror_lun_from_psl(fbe_database_service_t *fbe_database_service,
                                                                fbe_object_id_t object_id, 
                                                                fbe_private_space_layout_lun_info_t *lun_info,
                                                                fbe_bool_t invalidate);

static fbe_bool_t database_system_objects_is_create_export_device_needed(fbe_u8_t *device_name);
static fbe_status_t database_system_objects_zero_export_lun_luns(fbe_bool_t invalidate, 
                                                                 fbe_private_space_layout_export_lun_rg_id_t *rg_id_array,
                                                                 fbe_u32_t  array_size);
static fbe_status_t database_system_objects_zero_export_lun_luns_complete(fbe_packet_t * packet, 
                                                                          fbe_packet_completion_context_t context);
static fbe_status_t database_system_objects_load_mirror_rginfo_and_nonpaged_metadata(void);
static fbe_status_t database_mirror_need_auto_reinit(homewrecker_system_disk_table_t* sdt,
                                                     fbe_private_space_layout_region_t *region,
                                                     fbe_bool_t *auto_reinit);



/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!****************************************************************************
 * fbe_database_layout_get_export_lun_region_by_raid_group_id()
 ******************************************************************************
 * @brief
 *  This function gets export lun region by rg id.
 * 
 * @param  rg_id             - RG id
 * @param  region            - Pointer to region
 *
 * @return status            - status of the operation.
 *
 * @author
 *  08/29/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_database_layout_get_export_lun_region_by_raid_group_id(
                                                                        fbe_u32_t rg_id, fbe_private_space_layout_region_t *region)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t * region_p;

    region_p = &export_lun_regions[0];
    while (region_p->region_id != FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
        if(region_p->raid_info.raid_group_id == rg_id)
        {
            *region = *region_p;
            status  = FBE_STATUS_OK;
            return(status);
        }
        region_p++;
    }

    return(status);
}

/*!****************************************************************************
 * fbe_database_layout_get_export_lun_region_by_raid_object_id()
 ******************************************************************************
 * @brief
 *  This function gets export lun region by rg object id.
 *
 * @param  rg_id             - RG object id
 * @param  region            - Pointer to region
 *
 * @return status            - status of the operation.
 *
 * @author
 *  2015-03-23 - Created. Kang, Jamin
 *
 ******************************************************************************/
fbe_status_t fbe_database_layout_get_export_lun_region_by_raid_object_id(fbe_object_id_t obj,
                                                                         fbe_private_space_layout_region_t *region)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t * region_p;

    region_p = &export_lun_regions[0];
    while (region_p->region_id != FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
        if(region_p->raid_info.object_id == obj) {
            *region = *region_p;
            status  = FBE_STATUS_OK;
            return(status);
        }
        region_p++;
    }

    return(status);
}

/*!****************************************************************************
 * fbe_database_get_export_lun_by_lun_number()
 ******************************************************************************
 * @brief
 *  This function gets export lun region by rg id.
 * 
 * @param  rg_id             - RG id
 * @param  region            - Pointer to region
 *
 * @return status            - status of the operation.
 *
 * @author
 *  08/29/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_database_get_export_lun_by_lun_number(
                                                       fbe_u32_t lun_number, fbe_private_space_layout_lun_info_t * lun_info)
{
    fbe_private_space_layout_lun_info_t * lun_info_p;

    lun_info_p = &export_lun_luns[0];
    while (lun_info_p->lun_number != FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {
        if (lun_info_p->lun_number == lun_number) {
            *lun_info = *lun_info_p;
            return FBE_STATUS_OK;
        }
        lun_info_p++;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!****************************************************************************
 * fbe_private_space_layout_get_rg_info_by_rg_id()
 ******************************************************************************
 * @brief
 *  This function gets rg_info from export lun regions by rg id.
 * 
 * @param  rg_id             - RG id
 * @param  info              - Pointer to RG info
 *
 * @return status            - status of the operation.
 *
 * @author
 *  08/29/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_private_space_layout_get_rg_info_by_rg_id(
                                                           fbe_raid_group_number_t rg_id, fbe_private_space_layout_raid_group_info_t *info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t * region_p;

    region_p = &export_lun_regions[0];
    while (region_p->region_id != FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
        if(region_p->raid_info.raid_group_id == rg_id)
        {
            *info = region_p->raid_info;
            status  = FBE_STATUS_OK;
            return(status);
        }
        region_p++;
    }

    return(status);
}

/*!****************************************************************************
 * database_system_objects_update_export_lun_regions()
 ******************************************************************************
 * @brief
 *  This function updates local regions from system layout.
 * 
 * @param  None
 *
 * @return status            - status of the operation.
 *
 * @author
 *  08/29/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t database_system_objects_update_export_lun_regions(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_region_t * region_p;
    fbe_private_space_layout_region_t region;

    region_p = &export_lun_regions[0];
    while (region_p->region_id != FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
        status = fbe_private_space_layout_get_region_by_region_id(region_p->region_id, &region);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Search failed RG %d, stat:0x%x\n",
                           region_p->region_id,
                           status);
            return FBE_STATUS_NO_DEVICE;
        }

        region_p->starting_block_address = region.starting_block_address;
#if defined(SIMMODE_ENV)
        region_p->size_in_blocks = 0x2000;
#else
        region_p->size_in_blocks = region.size_in_blocks;
#endif
        region_p->raid_info.exported_capacity = region_p->size_in_blocks - 0x800;

        region_p++;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_system_objects_update_export_lun_luns()
 ******************************************************************************
 * @brief
 *  This function updates local luns from system layout.
 * 
 * @param  void
 *
 * @return status            - status of the operation.
 *
 * @author
 *  08/29/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t database_system_objects_update_export_lun_luns(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_lun_info_t * lun_info_p;
    fbe_private_space_layout_raid_group_info_t info;

    lun_info_p = &export_lun_luns[0];
    while (lun_info_p->lun_number != FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {
        status = fbe_private_space_layout_get_rg_info_by_rg_id(lun_info_p->raid_group_id, &info);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Search failed LUN %d, stat:0x%x\n",
                           lun_info_p->lun_number,
                           status);
            return status;
        }

        if (database_system_objects_is_create_export_device_needed(lun_info_p->lun_name))
        {
            lun_info_p->flags |= FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN;
        }
        else
        {
            lun_info_p->flags &= ~FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXPORT_LUN;
        }

        /* NDB=1, CZ=1 */
        /* lun_info_p->flags |= FBE_PRIVATE_SPACE_LAYOUT_FLAG_EXTERNALLY_INITIALIZED;*/

        lun_info_p->internal_capacity = info.exported_capacity;
        lun_info_p->external_capacity = lun_info_p->internal_capacity - 0x1000;
        lun_info_p++;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_system_objects_is_create_export_device_needed()
 ******************************************************************************
 * @brief
 *  This function decides whether the RG/LUN should be created on this SP.
 * 
 * @param  device_name             - name
 *
 * @return fbe_bool_t
 *
 * @author
 *  08/29/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_bool_t database_system_objects_is_create_export_device_needed(fbe_u8_t *device_name)
{
    fbe_cmi_sp_id_t my_sp_id;

    fbe_cmi_get_sp_id(&my_sp_id);

    if (my_sp_id == FBE_CMI_SP_ID_A && csx_p_strstr(device_name, "SPA")) {
        return FBE_TRUE;
    } else if (my_sp_id == FBE_CMI_SP_ID_B && csx_p_strstr(device_name, "SPB")) {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!****************************************************************************
 * database_system_objects_create_export_lun_objects()
 ******************************************************************************
 * @brief
 *  This function creates RGs and LUNs for export luns.
 * 
 * @param  fbe_database_service    - Pointer to database
 * @param  invalidate              - whether to invalidate
 *
 * @return status                  - status of the operation.
 *
 * @author
 *  08/29/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t database_system_objects_create_export_lun_objects(fbe_database_service_t *fbe_database_service, 
                                                               homewrecker_system_disk_table_t* system_disk_table)
{
    fbe_status_t status;
    fbe_private_space_layout_region_t * region_p;
    fbe_private_space_layout_lun_info_t * lun_info_p;
    fbe_bool_t invalidate;
    fbe_bool_t auto_reinit;
    fbe_private_space_layout_export_lun_rg_id_t reinit_rg_id[MIRROR_RG_NUM] = {0};
    fbe_u32_t   reinit_rg_number = 0;
    fbe_u32_t   index = 0;

    /* generate the private psl region info for export lun */
    status = database_system_objects_update_export_lun_regions();
    if (status == FBE_STATUS_NO_DEVICE)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "No export lun objects found, skip!\n");

        return FBE_STATUS_OK; 
    }
    else if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Fail to update export lun regions\n",
                       __FUNCTION__);
        return status;
    }
    
    /* generate the private psl lun info for export lun */
    status = database_system_objects_update_export_lun_luns();
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Fail to update export lun info\n",
                       __FUNCTION__);
        return status;
    }

    status = fbe_database_registry_get_c4_mirror_reinit_flag(&invalidate);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Fail to get reinit flag\n",
                       __FUNCTION__);

        /* No invalidate flag found, hence assuming it as FALSE(not ICA) */
        invalidate = FBE_FALSE;
    }

    /* If it is not ICA case, load the mirror rg's rginfo and nonpaged metadata */
    if (!invalidate)
    {
        status = database_system_objects_load_mirror_rginfo_and_nonpaged_metadata();
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Fail to read mirror rginfo and nonpaged metadata\n",
                           __FUNCTION__);
            return status;
        }
    }


    /* create the RG objects */
    region_p = &export_lun_regions[0];
    while (region_p->region_id != FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) {
        auto_reinit = FBE_FALSE;

        /* create c4mirror RG */
        if (database_system_objects_is_create_export_device_needed(region_p->region_name))
        {
            /* Check if this RG need reinit automatically. 
             * Only check if the mirror RG needs auto reinit in bootflash mode. 
             * In normal boot path, the argument system_disk_table will be NULL.
             * */
            if (!invalidate && system_disk_table != NULL)
            {
                /* check if this mirror RG is required to be reinit automatically */
                status = database_mirror_need_auto_reinit(system_disk_table, region_p, &auto_reinit);
                if (status != FBE_STATUS_OK)
                {
                    database_trace(FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Fail to check if mirror need auto reinitialization\n",
                                   __FUNCTION__);
                    return status;
                }

                /* record the RG_ID reinit, later we need to recreate and zero the LUN */
                if (auto_reinit)
                {
                    reinit_rg_id[reinit_rg_number] = region_p->raid_info.raid_group_id;
                    reinit_rg_number++;
                }
            }

            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: invalidate= %d, auto_reinit = %d \n",
                           __FUNCTION__, invalidate, auto_reinit);
            /* if ICA, zero and persist the nonpaged of c4mirror rg in bootflash */
            if (invalidate || auto_reinit) {
                status = fbe_database_metadata_nonpaged_system_zero_and_persist(region_p->raid_info.object_id, 1);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s: Fail to zero and persist np metadata\n",
                                  __FUNCTION__);
                    return status;  
                }
                status = fbe_c4_mirror_update_default(region_p->raid_info.object_id);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Fail to persist c4mirror pdo info\n",
                                  __FUNCTION__);
                    return status;

                }
            }
        
            status = database_system_objects_create_c4_mirror_rg_from_psl(fbe_database_service,
                                                                region_p->raid_info.object_id, 
                                                                region_p);
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Create export lun RG %d, stat:0x%x\n",
                           region_p->region_id,
                           status);
        }

        region_p++;
    }

    /* create the LUN objects */
    lun_info_p = &export_lun_luns[0];
    while (lun_info_p->lun_number != FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {
        
        auto_reinit = FBE_FALSE;
        /* create c4mirror LUN */ 
        if (database_system_objects_is_create_export_device_needed(lun_info_p->lun_name))
        {
            /* If the mirror RG is required to be reinit automatically, then recreate the LUN */
            if (reinit_rg_number != 0)
            {
                for (index = 0; index < MIRROR_RG_NUM; index++)
                {
                    if (lun_info_p->raid_group_id == reinit_rg_id[index])
                    {
                        auto_reinit = FBE_TRUE;
                    }
                }
            }

            /* if ICA, zero and persist the nonpaged of c4mirror lun in bootflash */
            if (invalidate || auto_reinit) {
                status = fbe_database_metadata_nonpaged_system_zero_and_persist(lun_info_p->object_id, 1);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s: Fail to zero and persist np metadata\n",
                                  __FUNCTION__);
                    return status;  
                }
            }

            status = database_system_objects_create_c4_mirror_lun_from_psl(fbe_database_service,
                                                                 lun_info_p->object_id, 
                                                                 lun_info_p,
                                                                 invalidate);
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Create export lun LUN %d, stat:0x%x\n",
                           lun_info_p->lun_number,
                           status);

            /* Let's set the SP Fault LED blink rate to 1/4 Hz and color to
             * blue to indicate c4mirror has exported the boot partition. 
             */
            if (lun_info_p->object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPA ||
                lun_info_p->object_id == FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPB)
            {
                fbe_database_set_sp_fault_led(LED_BLINK_0_25HZ, BOOT_CONDITION);
            }
        }

        lun_info_p++;
    }

    /* zero the LUNs when ICA */
    if (invalidate || reinit_rg_number != 0 /*&& fbe_database_mini_mode()*/)
    {
        status = database_system_objects_zero_export_lun_luns(invalidate, reinit_rg_id, MIRROR_RG_NUM);

        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    
        fbe_database_registry_set_c4_mirror_reinit_flag(FBE_FALSE);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_system_objects_zero_export_lun_luns()
 *****************************************************************
 * @brief
 *   This function zero the export luns.
 *
 * @param : invalidate - the ICA flags was set
 *          rg_id_array - record the recreated mirror RG id
 *          array_size - the rg_id_array size
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *
 ****************************************************************/
static fbe_status_t database_system_objects_zero_export_lun_luns(fbe_bool_t invalidate, 
                                                                 fbe_private_space_layout_export_lun_rg_id_t *rg_id_array,
                                                                 fbe_u32_t  array_size)
{
    fbe_status_t status;
    fbe_u32_t lun_index = 0;
    fbe_packet_t * packet_array[5] = { NULL };
    fbe_private_space_layout_lun_info_t * lun_info_p;
    fbe_lun_hard_zero_t lun_hard_zero_req;
    export_lun_zero_progress_t lun_zero_progress;
    fbe_u32_t   index = 0;
    fbe_bool_t  need_zero = FBE_FALSE;

    fbe_semaphore_init(&lun_zero_progress.sem, 0, FBE_SEMAPHORE_MAX);
    lun_zero_progress.outstanding_count = 0;

    lun_info_p = &export_lun_luns[0];
    while (lun_info_p->lun_number != FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) {

        /* if the lun is not used in this SP, skip */
        if (!database_system_objects_is_create_export_device_needed(lun_info_p->lun_name))
        {
            lun_info_p++;
            continue;
        }

        /* Check if the Mirror RG was recreated. If yes, then zero the LUN on this RG*/
        need_zero = FBE_FALSE;
        for (index = 0; index < array_size; index++)
        {
            if (lun_info_p->raid_group_id == rg_id_array[index])
            {
                need_zero = FBE_TRUE;
            }
        }
        if (!invalidate && !need_zero)
        {
            lun_info_p++;
            continue;
        }
        
        /* wait for export lun ready before zeroing */
        status = fbe_database_wait_for_object_state(lun_info_p->object_id, FBE_LIFECYCLE_STATE_READY, DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
        if (status == FBE_STATUS_TIMEOUT) {
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Export lun LUN %d not ready in %d secs!\n",
                           lun_info_p->lun_number,
                           DATABASE_WAIT_FOR_OBJECT_STATE_TIME);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }

        /* allocate and initialize packets */
        packet_array[lun_index] = fbe_transport_allocate_packet();
        if (packet_array[lun_index] == NULL) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: unable to allocate packet\n",
                            __FUNCTION__);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }

        fbe_transport_initialize_sep_packet(packet_array[lun_index]);
        /* set lun zero regions */
        lun_hard_zero_req.lba = 0;
        lun_hard_zero_req.blocks = lun_info_p->external_capacity;
        lun_hard_zero_req.threads = 3;
        lun_hard_zero_req.clear_paged_metadata = FBE_TRUE;
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "Zero export lun LUN %d, lba:0x%llx, count:0x%llx\n",
                       lun_info_p->lun_number,
                       lun_hard_zero_req.lba,
                       lun_hard_zero_req.blocks);

        fbe_atomic_increment(&lun_zero_progress.outstanding_count);
        status = fbe_database_send_packet_to_object_async(packet_array[lun_index],
                                                          FBE_LUN_CONTROL_CODE_INITIATE_HARD_ZERO,
                                                          &lun_hard_zero_req,
                                                          sizeof(fbe_lun_hard_zero_t),
                                                          lun_info_p->object_id,
                                                          NULL,  /* no sg list*/
                                                          0,  /* no sg list*/
                                                          FBE_PACKET_FLAG_NO_ATTRIB,
                                                          database_system_objects_zero_export_lun_luns_complete,
                                                          &lun_zero_progress,
                                                          FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING) 
        {
            fbe_atomic_decrement(&lun_zero_progress.outstanding_count);
 
            database_trace (FBE_TRACE_LEVEL_CRITICAL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: failed to send zero packet, status 0x%x\n",
                            __FUNCTION__, status);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }

        lun_index++;
        lun_info_p++;
    }

    while (lun_zero_progress.outstanding_count != 0)
    {
        /* wait for zero finished */
        fbe_semaphore_wait(&lun_zero_progress.sem, NULL);

        fbe_atomic_decrement(&lun_zero_progress.outstanding_count);
    }

    /* destroy and free packets */
    lun_index = 0;
    while (packet_array[lun_index] != NULL)
    {
        fbe_transport_release_packet(packet_array[lun_index]);
        lun_index++;
    }

    fbe_semaphore_destroy(&lun_zero_progress.sem);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_system_objects_zero_export_lun_luns_complete()
 *****************************************************************
 * @brief
 *   This function is the completion for zeroing export lun.
 *
 * @param packet  - packet
 * @param context - context
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *
 ****************************************************************/
static fbe_status_t database_system_objects_zero_export_lun_luns_complete(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    export_lun_zero_progress_t *lun_zero_progress_p;

    lun_zero_progress_p = (export_lun_zero_progress_t *)context;

    fbe_semaphore_release(&lun_zero_progress_p->sem, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_system_objects_load_mirror_rginfo_and_nonpaged_metadata()
 ******************************************************************************
 * @brief
 *  This function will load the mirror rginfo and the nonpaged metadata of 
 *  mirror RGs and LUNs.
 * 
 * @param  
 *
 * @return fbe_status_t
 *
 * @author
 *  08/07/2015 - Created. Hongpo Gao
 *
 ******************************************************************************/
static fbe_status_t database_system_objects_load_mirror_rginfo_and_nonpaged_metadata(void)
{
    fbe_status_t status;
    fbe_private_space_layout_region_t * region_p;
    fbe_private_space_layout_lun_info_t * lun_info_p;
    fbe_u32_t   drive_mask = 0;

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: entry\n", __FUNCTION__);

   status = fbe_database_raw_mirror_get_valid_drives(&drive_mask);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "DB failed to get valid RAW Mirror drives. Sts:0x%x. Will load from all drives.\n", status);

        drive_mask = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    } 
    
    region_p = &export_lun_regions[0];
    while (region_p->region_id != FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_INVALID) 
    {
        if (database_system_objects_is_create_export_device_needed(region_p->region_name))
        {
            status = fbe_c4_mirror_load_rginfo_during_boot_path(region_p->raid_info.object_id);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "db_load_mirror_rginfo_and_nonpaged_during_boot: Fail to load RG %d mirror rginfo, status = %d\n",
                              region_p->raid_info.object_id, status);
                return status;  
            }

            status = fbe_database_metadata_nonpaged_system_read_persist_with_drivemask(drive_mask, region_p->raid_info.object_id, 1);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "db_load_mirror_rginfo_and_nonpaged_during_boot: Fail to load export lun RG %d np metadata\n",
                              region_p->raid_info.object_id);
                return status;  
            }
        }
        region_p++;
    }

    /* Load the nonpaged metadata for mirror LUNs */
    lun_info_p = &export_lun_luns[0];
    while (lun_info_p->lun_number != FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID) 
    {
        if (database_system_objects_is_create_export_device_needed(lun_info_p->lun_name))
        {
            /* read nonpaged from persist */
            status = fbe_database_metadata_nonpaged_system_read_persist_with_drivemask(drive_mask, lun_info_p->object_id, 1);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: Fail to load export lun LUN %d np metadata\n",
                              __FUNCTION__,
                              lun_info_p->object_id);
                return status;  
            }
        }
        lun_info_p++;
            
    }

    return FBE_STATUS_OK;
}

static fbe_status_t database_mirror_need_auto_reinit(homewrecker_system_disk_table_t* sdt,
                                                     fbe_private_space_layout_region_t *region,
                                                     fbe_bool_t *auto_reinit) 
{
    fbe_raid_group_nonpaged_metadata_t *rg_nonpaged = NULL;
    fbe_database_get_nonpaged_metadata_data_ptr_t get_nonpaged_metadata;
    fbe_raid_group_rebuild_nonpaged_info_t *rg_rebuild_info = NULL;
    fbe_database_physical_drive_info_t  physical_drive_entry;
    fbe_u32_t   invalid_disks = 0;
    fbe_u32_t   invalid_disk_index = FBE_INVALID_SLOT_NUM;
    fbe_u32_t   index = 0;
    fbe_u32_t   fru_inx = 0;
    fbe_u32_t   new_pvd_bitmask = 0;
    fbe_u16_t   rebuild_pos_bitmask = 0;
    fbe_u16_t   combined_failed_bitmask = 0;
    fbe_u16_t   invalid_disks_bitmask = 0;
    fbe_u16_t   failed_drives = 0;
    fbe_bool_t  nonpaged_not_initialized = FBE_FALSE;
    fbe_status_t    status;

    if (sdt == NULL || region == NULL  || auto_reinit == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: illegal arguments \n",
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (index = 0; index < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; index++)
    {
        /*invalid disk count.*/
        if (sdt[index].is_invalid_system_drive == FBE_TRUE)
        {
            invalid_disks++;
            invalid_disk_index = index;  // only record the one invalid disk index

            /* Get the invalid_disks_bitmask for this mirror RG */
            for (fru_inx = 0; fru_inx < region->number_of_frus; fru_inx++)
            {
                if (region->fru_id[fru_inx] == index)
                {
                    invalid_disks_bitmask |= 1 << fru_inx;
                }
            }
        }

    } 

    /* If invalid disks number >= 2, don't reinit the mirror RG automatically */
    if (invalid_disks > 1)
    {
        *auto_reinit = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /* check if mirror RG is broken, if no broken, auto_reinit = FALSE */
    fbe_zero_memory(&get_nonpaged_metadata, sizeof(fbe_database_get_nonpaged_metadata_data_ptr_t));
    get_nonpaged_metadata.object_id = region->raid_info.object_id;
    status = fbe_database_metadata_nonpaged_get_data_ptr(&get_nonpaged_metadata);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: fail to get nonpaged data ptr of obj: %d, status = %d \n",
                       __FUNCTION__, get_nonpaged_metadata.object_id, status);

        return status;
    }
    
    rg_nonpaged = (fbe_raid_group_nonpaged_metadata_t *)(get_nonpaged_metadata.data_ptr);
    rg_rebuild_info = &(rg_nonpaged->rebuild_info);
    if (rg_nonpaged->base_config_nonpaged_metadata.object_id != region->raid_info.object_id || 
        rg_nonpaged->base_config_nonpaged_metadata.generation_number != region->raid_info.object_id)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: mirror RG 0x%x non-paged metadata is not initalized \n",
                       __FUNCTION__, region->raid_info.object_id);
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "Nonpaged metadata object_id: 0x%x, generation_number: 0x%llx \n",
                       rg_nonpaged->base_config_nonpaged_metadata.object_id, 
                       rg_nonpaged->base_config_nonpaged_metadata.generation_number);
        nonpaged_not_initialized = FBE_TRUE;
    }

    status = fbe_c4_mirror_check_new_pvd(region->raid_info.object_id, &new_pvd_bitmask);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: fail to check new pvd, status = %d\n",
                       __FUNCTION__, status);
        return status;
    }

    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        if (rg_rebuild_info->rebuild_checkpoint_info[index].checkpoint != FBE_LBA_INVALID || nonpaged_not_initialized)
        {
            rebuild_pos_bitmask |= 1 << rg_rebuild_info->rebuild_checkpoint_info[index].position;
        }
    }
    
    /* check if RG is broken by rebuild_info and new_pvd_bitmask */
    combined_failed_bitmask = rebuild_pos_bitmask | new_pvd_bitmask | invalid_disks_bitmask;
    failed_drives = fbe_raid_get_bitcount(combined_failed_bitmask);

    /* debug, remove it later */
    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: combined failed_bitmask:%x rebuild_pos_bitmask: %x \n",
                   __FUNCTION__, combined_failed_bitmask, rebuild_pos_bitmask);
    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: new_pvd_bitmask: %x, invalid_disks_bitmask:%x, failed_drives = %d\n",
                    __FUNCTION__, new_pvd_bitmask, invalid_disks_bitmask, failed_drives);
        
    if (failed_drives < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)
    {
        *auto_reinit = FBE_FALSE;
        return FBE_STATUS_OK;
    }
    /* The RG is broken. The invalid disks number should be one. We need to judge if it can be 
     * reinit automatically according to the invalid disk type */
    if (failed_drives >= FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)
    {
        /* No invlaid disks, only the mirror RG broken. We need to reinit it automatically */
        if (invalid_disks == 0)
        {
            database_trace(FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed_drives:%d, invalid_disks:%d, auto_reinit the RG\n",
                           __FUNCTION__, failed_drives, invalid_disks);
            *auto_reinit = FBE_TRUE;
            return FBE_STATUS_OK;
        }
        /* invalid_disks == 1, we need to do following check:
         * 1. if the drive in this invalid system slot is allowed to be consumed, the allowed drive type includes:
         *      1> FBE_HW_NEW_DISK
         *      2> FBE_HW_OTHER_ARR_SYS_DISK
         *      3> FBE_HW_OTHER_ARR_USR_DISK
         * 2. if the original system drive in this invalid system slot is found in user slot. If it is, we can't
         *    reinit the mirror RG automatically. User should try to return the original system drive.
         *
         * */

        if (sdt[invalid_disk_index].disk_type == FBE_HW_NEW_DISK || 
            sdt[invalid_disk_index].disk_type == FBE_HW_OTHER_ARR_SYS_DISK || 
            sdt[invalid_disk_index].disk_type == FBE_HW_OTHER_ARR_USR_DISK)  
        {
            /* If the original system drive was found in user slot, we don't allow auto reinit */
            if (sdt[invalid_disk_index].actual_bus != FBE_INVALID_PORT_NUM && 
                sdt[invalid_disk_index].actual_encl != FBE_INVALID_ENCL_NUM && 
                sdt[invalid_disk_index].actual_slot != FBE_INVALID_SLOT_NUM && 
                sdt[invalid_disk_index].actual_bus >= 0 &&
                sdt[invalid_disk_index].actual_encl >= 0 && 
                sdt[invalid_disk_index].actual_slot >= FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER)
            {
                database_trace(FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: find system drive in user slot. drive actual_slot is %d\n",
                               __FUNCTION__, sdt[invalid_disk_index].actual_slot);
                *auto_reinit = FBE_FALSE;
                return FBE_STATUS_OK;
            }

            /* Other case, we allow reinit the mirror RG automatically */
            database_trace(FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: sdt[%d] drive_type is %x, auto reinit the mirror RG\n",
                           __FUNCTION__, invalid_disk_index, sdt[invalid_disk_index].disk_type);
            *auto_reinit = FBE_TRUE;
            /* Reconnect the PVD with PDO */
            fbe_zero_memory(&physical_drive_entry, sizeof(fbe_database_physical_drive_info_t));
            physical_drive_entry.bus = 0;
            physical_drive_entry.enclosure = 0;
            physical_drive_entry.slot = invalid_disk_index;
            status = fbe_database_connect_to_pdo_by_location(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_FIRST + invalid_disk_index, 
                                                         physical_drive_entry.bus,
                                                         physical_drive_entry.enclosure,
                                                         physical_drive_entry.slot,
                                                         &physical_drive_entry);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: fail to connect PVD (0_0_%d), status = %d \n",
                               __FUNCTION__, physical_drive_entry.slot, status);
            
                return status;
            }
            return FBE_STATUS_OK;
        }
    }

    *auto_reinit = FBE_FALSE;
    return FBE_STATUS_OK;
}
