/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_c4_mirror.c
 ***************************************************************************
 *
 * @brief
 *    This file contains cli functions for c4mirror command
 *    FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *    06/29/2015 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_npinfo.h"
#include "fbe_cli_private.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_cli_c4_mirror.h"

typedef struct c4_mirror_rebuild_status_s {
    fbe_lba_t capacity;
    fbe_u32_t rl_bitmask;
    fbe_u32_t rebuild_bitmask;
    fbe_lba_t rebuild_checkpoint;
} c4_mirror_rebuild_status_t;


static fbe_status_t
c4_mirror_get_rebuild_status_by_object_id(fbe_object_id_t object_id,
                                         c4_mirror_rebuild_status_t *rebuild_status)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t raid_group_info;
    fbe_u32_t i;
    fbe_lba_t rb_checkpoint = FBE_LBA_INVALID;
    fbe_u32_t rl_bitmask = 0;

    status = fbe_api_raid_group_get_info(object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to get raid information for object 0x%x\n", object_id);
        return status;
    }

    memset(rebuild_status, 0, sizeof(*rebuild_status));
    rebuild_status->capacity = raid_group_info.capacity;
    for (i = 0; i < raid_group_info.width; i += 1) {
        if (raid_group_info.b_rb_logging[i]) {
            rl_bitmask |= (1 << i);
        }
    }
    rebuild_status->rl_bitmask = rl_bitmask;
    rebuild_status->rebuild_bitmask = 0;
    if (rl_bitmask) {
        rebuild_status->rebuild_checkpoint = 0;
        return FBE_STATUS_OK;
    }

    for (i = 0; i < raid_group_info.width; i += 1) {
        fbe_lba_t rebuild_checkpoint = raid_group_info.rebuild_checkpoint[i];

        if (rebuild_checkpoint == FBE_LBA_INVALID) {
            continue;
        }
        rebuild_status->rebuild_bitmask |= (1 << i);
        /* Rebuild metadata now. Havn't rebuild user data yet */
        if (rebuild_checkpoint >= raid_group_info.paged_metadata_start_lba) {
            rebuild_checkpoint = 0;
        }
        if (rebuild_checkpoint < rb_checkpoint) {
            rb_checkpoint = rebuild_checkpoint;
        }
    }
    rebuild_status->rebuild_checkpoint = rb_checkpoint;
    return FBE_STATUS_OK;
}


static void c4_mirror_display_rebuild_status(void)
{
    fbe_object_id_t c4_mirror_raid_object_id[2];
    fbe_cmi_service_get_info_t cmi_info;
    fbe_status_t status;
    c4_mirror_rebuild_status_t rebuild_status[2];
    fbe_u32_t i;
    fbe_bool_t rebuild_in_progress = FBE_FALSE;
    fbe_bool_t rebuild_required = FBE_FALSE;

    status = fbe_api_cmi_service_get_info(&cmi_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to get SP ID\n");
        return;
    }
    if (cmi_info.sp_id == FBE_CMI_SP_ID_A) {
        c4_mirror_raid_object_id[0] = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPA;
        c4_mirror_raid_object_id[1] = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPA;
    } else if (cmi_info.sp_id == FBE_CMI_SP_ID_B) {
        c4_mirror_raid_object_id[0] = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_UTILITY_BOOT_VOLUME_SPB;
        c4_mirror_raid_object_id[1] = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RG_PRIMARY_BOOT_VOLUME_SPB;
    } else {
        fbe_cli_error("Invalid SP ID: %u\n", cmi_info.sp_id);
        return;
    }
    for (i = 0; i < 2; i += 1) {
        status = c4_mirror_get_rebuild_status_by_object_id(c4_mirror_raid_object_id[i], &rebuild_status[i]);
        if (status != FBE_STATUS_OK) {
            return;
        }
    }
    if (rebuild_status[0].rl_bitmask != 0 || rebuild_status[1].rl_bitmask != 0) {
        rebuild_required = FBE_TRUE;
    }
    for (i = 0; i < 2; i += 1) {
        if (rebuild_status[i].rl_bitmask != 0) {
            rebuild_required = FBE_TRUE;
            continue;
        }
        if (rebuild_status[i].rebuild_checkpoint != FBE_LBA_INVALID) {
            rebuild_in_progress = FBE_TRUE;
        }
    }

    fbe_cli_printf ("  Mirror: \n\tRebuildInProgress:\t%s \n\tRebuildRequired:\t%s\n",
                    rebuild_in_progress? "TRUE" : "FALSE",
                    rebuild_required? "TRUE" : "FALSE");

    if (rebuild_in_progress) {
        fbe_lba_t total_capacity = 0;
        fbe_lba_t rebuilt_lba = 0;
        fbe_u32_t percent_rebuild;
        for (i = 0; i < 2; i += 1) {
            total_capacity += rebuild_status[i].capacity;
            if (rebuild_status[i].rebuild_checkpoint == FBE_LBA_INVALID) {
                rebuilt_lba += rebuild_status[i].capacity;
            } else {
                rebuilt_lba += rebuild_status[i].rebuild_checkpoint;
            }
        }
        percent_rebuild = (fbe_u32_t)(rebuilt_lba * 100 / total_capacity);
        fbe_cli_printf ("\tPercentageRebuilt:\t%u\n", percent_rebuild);
        /* TODO: If we need print Src disk and Dest disk, we can use 'fbe_cli_get_rg_details' to
         * get downstream PVDs. Still don't know how to print checkpoint as we may be 2 checkpoint
         */
    }
}

static void c4_mirror_mark_lun_clean(void)
{
    fbe_cmi_service_get_info_t cmi_info;
    fbe_status_t status;
    fbe_object_id_t lun_object_id;

    status = fbe_api_cmi_service_get_info(&cmi_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to get SP ID\n");
        return;
    }
    
    if (cmi_info.sp_id == FBE_CMI_SP_ID_A) {
        lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPA;
    } else if (cmi_info.sp_id == FBE_CMI_SP_ID_B) {
        lun_object_id = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_PRIMARY_BOOT_VOLUME_SPB;
    } else {
        fbe_cli_error("Invalid SP ID: %u\n", cmi_info.sp_id);
        return;
    }

    status = fbe_api_lun_prepare_for_power_shutdown(lun_object_id);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Failed to mark lun clean\n");
        return;
    }
}

static void fbe_cli_c4_mirror_usage(void)
{
    fbe_cli_printf(MIRROR_CMD_USAGE);
}

/*************************************************************************
 *                            @fn fbe_cli_c4_mirror ()
 *************************************************************************
 *
 * @brief
 *    Function to implement c4mirror commands
 *
 * @param :
 *    argc     (INPUT) NUMBER OF ARGUMENTS
 *    argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *    None
 *
 * @author
 *    06/29/2015 - Created. Jamin Kang
 *************************************************************************/
void fbe_cli_c4_mirror(int argc, char** argv)
{
    fbe_object_id_t board_object_id;
    fbe_board_mgmt_platform_info_t platform_info = {0};
    fbe_object_id_t rg_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_u8_t    sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1] = {0};
    fbe_u16_t   edge_index = FBE_INVALID_SLOT_NUM;
    fbe_status_t    status;
    fbe_u32_t   index;

    if (argc <= 0 ||
        !strcmp(argv[0], "-help") ||
        !strcmp(argv[0], "-h")) {
        fbe_cli_c4_mirror_usage();
        return;
    }

    /* Get the board object id*/
    status = fbe_api_get_board_object_id(&board_object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving board object ID, Status %d", status);
        return;
    }
    status = fbe_api_board_get_platform_info(board_object_id, &platform_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving base board info, Status %d", status);
        return;
    }

    if (platform_info.hw_platform_info.platformType == SPID_MERIDIAN_ESX_HW_TYPE ||
        platform_info.hw_platform_info.platformType == SPID_TUNGSTEN_HW_TYPE)
    {
        /* Virtual VNX */
        fbe_cli_error("\n Error. Not supported in VIRTUAL environment...\n");
        return;
    }
    else if (platform_info.hw_platform_info.platformType == SPID_NOVA_S1_HW_TYPE ||
             platform_info.hw_platform_info.platformType == SPID_TRIN_S1_HW_TYPE)
    {
        /* BC Simulator */
        fbe_cli_error("\n Error. Not supported in SIM64 environment...\n");
        return;
    }

    if (strcmp(argv[0], "-rebuild_status") == 0) {
        c4_mirror_display_rebuild_status();
    } else if (strcmp(argv[0], "-update_pvd") == 0) {
        argc--;
        argv++;

        while (argc > 0) {
            if (strcmp(*argv, "-rg_obj_id") == 0) {
                argc--;
                argv++;

                rg_obj_id = atoi(*argv);

            } else if (strcmp(*argv, "-sn") == 0) {
                argc--;
                argv++;

                strncpy((char *)sn, *argv, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
                fbe_cli_printf("update c4mirror pvd map, sn= %s\n", sn);

            } else if (strcmp(*argv, "-edge_index") == 0) {
                argc--;
                argv++;

                edge_index = atoi(*argv);

            } else {
                fbe_cli_c4_mirror_usage();
                return;
            }
            argc--;
            argv++;
        }
        if (rg_obj_id == FBE_OBJECT_ID_INVALID || 
            edge_index == FBE_INVALID_SLOT_NUM)
        {
            fbe_cli_printf("rg_obj_id: %d, edge_index = %d\n", rg_obj_id, edge_index);
            fbe_cli_c4_mirror_usage();
            return;
        }
        status = fbe_api_database_update_mirror_pvd_map(rg_obj_id, edge_index, sn, sizeof(sn));
        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf("Succeed to update c4mirror pvd map, status = %d\n", status);
        } else {
            fbe_cli_printf("Fail to update c4mirror pvd map, status = %d\n", status);
        }

        return;
    } else if (strcmp(argv[0], "-get_pvd_map") == 0) {
        argc--;
        argv++;

        while (argc > 0) {
            if (strcmp(*argv, "-rg_obj_id") == 0) {
                argc--;
                argv++;

                rg_obj_id = atoi(*argv);

            } else {
                fbe_cli_c4_mirror_usage();
                return;
            }
            argc--;
            argv++;
        }

        if (rg_obj_id == FBE_OBJECT_ID_INVALID)
        {
            fbe_cli_printf("rg_obj_id: %d\n", rg_obj_id);
            fbe_cli_c4_mirror_usage();
            return;
        }

        fbe_cli_printf("RG:%x pvd_mapping is:\n", rg_obj_id);
        for (index = 0; index < 2; index++)
        {
            fbe_zero_memory(sn, sizeof(sn));
            status = fbe_api_database_get_mirror_pvd_map(rg_obj_id, index, sn, sizeof(sn));
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Failed to get c4mirror pvd map\n");
                return;
            }
            fbe_cli_printf("edge_index: %d, SN:%s", index, sn);
        }
        fbe_cli_printf("\n");
    } else if (strcmp(argv[0], "-mark_clean") == 0) {
        c4_mirror_mark_lun_clean();
    } else {
        fbe_cli_c4_mirror_usage();
        return;
    }
}

