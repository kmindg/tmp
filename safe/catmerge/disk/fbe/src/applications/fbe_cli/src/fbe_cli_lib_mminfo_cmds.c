/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_mminfo_cmds.c
 ***************************************************************************
 *
 * @brief
 *    This file contains cli functions for the mminfo related features in
 *    FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *    10/29/2013 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_lib_mminfo.h"
#include "fbe_cli_private.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe_cli_field_utils.h"

/*****************************************************************************
 * Macros
 *****************************************************************************/

#define MEMBER_BC(cmd, mem, dump, set)          \
    MEMBER(fbe_base_config_metadata_memory_t, cmd, mem, dump, set)
#define MEMBER_BC_DEF(cmd, mem)                 \
    MEMBER_BC(cmd, mem, NULL, NULL)

#define MEMBER_LUN(cmd, mem, dump, set)         \
    MEMBER(fbe_lun_metadata_memory_t, cmd, mem, dump, set)
#define MEMBER_LUN_DEF(cmd, mem)                \
    MEMBER_LUN(cmd, mem, NULL, NULL)

#define MEMBER_RG(cmd, mem, dump, set)          \
    MEMBER(fbe_raid_group_metadata_memory_t, cmd, mem, dump, set)
#define MEMBER_RG_DEF(cmd, mem)                 \
    MEMBER_RG(cmd, mem, NULL, NULL)

#define MEMBER_PVD(cmd, mem, dump, set)         \
    MEMBER(fbe_provision_drive_metadata_memory_t, cmd, mem, dump, set)
#define MEMBER_PVD_DEF(cmd, mem)                \
    MEMBER_PVD(cmd, mem, NULL, NULL)


/******************************************************************************
 *   Private variables definition
 *****************************************************************************/

static struct fbe_cli_field_map base_config_map[] = {
    MEMBER_BC_DEF("bc_version_header", version_header.size),
    MEMBER_BC_DEF("bc_vh_res_0", version_header.padded_reserve_space[0]),
    MEMBER_BC_DEF("bc_vh_res_1", version_header.padded_reserve_space[1]),
    MEMBER_BC_DEF("bc_vh_res_2", version_header.padded_reserve_space[2]),
    MEMBER_BC_DEF("bc_lifecycle_state", lifecycle_state),
    MEMBER_BC_DEF("bc_power_save_state", power_save_state),
    MEMBER_BC_DEF("bc_last_io_time", last_io_time),
    MEMBER_BC_DEF("bc_flags", flags),
};

static struct fbe_cli_field_map lun_map[] = {
    MEMBER_LUN_DEF("lun_flags", flags),
};

static struct fbe_cli_field_map raid_group_map[] = {
    MEMBER_RG_DEF("rg_flags", flags),
    MEMBER_RG_DEF("failed_drives_bitmap", failed_drives_bitmap),
    MEMBER_RG_DEF("failed_ios_pos_bitmap", failed_ios_pos_bitmap),
    MEMBER_RG_DEF("num_slf_drives", num_slf_drives),
    MEMBER_RG_DEF("disabled_pos_bitmap", disabled_pos_bitmap),
    MEMBER_RG_DEF("blocks_rebuilt_0", blocks_rebuilt[0]),
    MEMBER_RG_DEF("blocks_rebuilt_1", blocks_rebuilt[1]),
};

static struct fbe_cli_field_map pvd_map[] = {
    MEMBER_PVD_DEF("pvd_flags", flags),
    MEMBER_PVD_DEF("port_number", port_number),
    MEMBER_PVD_DEF("enclosure_number", enclosure_number),
    MEMBER_PVD_DEF("slot_number", slot_number),
};

/**
 * WARNING: Allow to set everything if we don't know its object type
 */
static struct fbe_cli_field_map_array base_config_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &pvd_map[0], ARRAY_LEN(pvd_map) },
    { &raid_group_map[0], ARRAY_LEN(raid_group_map) },
    { &lun_map[0], ARRAY_LEN(lun_map) },
    FBE_CLI_FIELD_MAP_ARRAY_TERMINATION,
};
static struct fbe_cli_field_map_array raid_group_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &raid_group_map[0], ARRAY_LEN(raid_group_map) },
    FBE_CLI_FIELD_MAP_ARRAY_TERMINATION,
};
static struct fbe_cli_field_map_array lun_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &lun_map[0], ARRAY_LEN(lun_map) },
    FBE_CLI_FIELD_MAP_ARRAY_TERMINATION,
};
static struct fbe_cli_field_map_array pvd_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &pvd_map[0], ARRAY_LEN(pvd_map) },
    FBE_CLI_FIELD_MAP_ARRAY_TERMINATION,
};

/*********************************************************************************************
 * DUMP utils
 *********************************************************************************************/

static void dump_type(fbe_object_id_t obj, fbe_topology_object_type_t type)
{

    const char *s;

    s = fbe_cli_field_map_get_class_name(obj);
    fbe_cli_printf("\nObject: 0x%x, Type: %s\n\n", (fbe_u32_t)obj, s);
}

static void dump_member(fbe_base_config_control_metadata_memory_read_t *mminfo,
                        struct fbe_cli_field_map *map, unsigned int map_num)
{
    fbe_cli_field_map_dump(&mminfo->memory_data[0], mminfo->memory_size,
                           map, map_num);
}

static void dump_base(fbe_base_config_control_metadata_memory_read_t *mminfo)
{
    if (mminfo->memory_size < sizeof(fbe_base_config_metadata_memory_t)) {
        fbe_cli_printf("%s: Invalid len: %u.\n", __FUNCTION__, mminfo->memory_size);
        return;
    }
    dump_member(mminfo, &base_config_map[0], ARRAY_LEN(base_config_map));
}

static void dump_raid_group(fbe_base_config_control_metadata_memory_read_t *mminfo)
{
    if (mminfo->memory_size < sizeof(fbe_raid_group_metadata_memory_t)) {
        fbe_cli_printf("%s: Invalid len: %u.\n", __FUNCTION__, mminfo->memory_size);
        return;
    }
    dump_base(mminfo);
    dump_member(mminfo, &raid_group_map[0], ARRAY_LEN(raid_group_map));
}

static void dump_lun(fbe_base_config_control_metadata_memory_read_t *mminfo)
{
    if (mminfo->memory_size < sizeof(fbe_lun_metadata_memory_t)) {
        fbe_cli_printf("%s: Invalid len: %u.\n", __FUNCTION__, mminfo->memory_size);
        return;
    }
    dump_base(mminfo);
    dump_member(mminfo, &lun_map[0], ARRAY_LEN(lun_map));
}

static void dump_pvd(fbe_base_config_control_metadata_memory_read_t *mminfo)
{
    if (mminfo->memory_size < sizeof(fbe_provision_drive_metadata_memory_t)) {
        fbe_cli_printf("%s: Invalid len: %u.\n", __FUNCTION__, mminfo->memory_size);
        return;
    }
    dump_base(mminfo);
    dump_member(mminfo, &pvd_map[0], ARRAY_LEN(pvd_map));
}

static void mminfo_do_dump(fbe_object_id_t obj, fbe_topology_object_type_t type,
                           fbe_bool_t is_peer,
                           fbe_base_config_control_metadata_memory_read_t *mminfo)
{
    if (mminfo->is_peer) {
        fbe_cli_printf("\nPEER: ");
    }

    dump_type(obj, type);

    switch (type) {
    case FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP:
    case FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE:
        dump_raid_group(mminfo);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_LUN:
        dump_lun(mminfo);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE:
        dump_pvd(mminfo);
        break;
    default:
        dump_base(mminfo);
        break;
    }
}

/*************************************************************************
 *                            @fn mminfo_dump()
 *************************************************************************
 *
 * @brief
 *    Show metadata memory
 *
 * @param :
 *    obj:      object to show
 *    type:     object type when we dump
 *    is_peer:  if is_peer == TRUE, we show peer memory, else show local
 *
 * @return:
 *    None
 *
 * @author
 *    10/29/2013 - Created. Jamin Kang
 *************************************************************************/
static void mminfo_dump(fbe_object_id_t obj,
                        fbe_topology_object_type_t type,
                        fbe_bool_t is_peer)
{
    fbe_base_config_control_metadata_memory_read_t mminfo;
    fbe_status_t status;

    mminfo.memory_size = sizeof(mminfo.memory_data);
    mminfo.is_peer = is_peer;
    status = fbe_api_base_config_metadata_memory_read(obj, &mminfo);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("%s: Read metadata memory faield: 0x%x\n",
                       __FUNCTION__, status);
        return;
    }
    mminfo_do_dump(obj, type, FBE_FALSE, &mminfo);
}


static struct fbe_cli_field_map_array *get_table(fbe_topology_object_type_t type)
{
    struct fbe_cli_field_map_array *table = &base_config_table[0];

    switch (type) {
    case FBE_TOPOLOGY_OBJECT_TYPE_LUN:
        table = &lun_table[0];
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE:
    case FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP:
        table = &raid_group_table[0];
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE:
        table = &pvd_table[0];
        break;
    default:
        table = &base_config_table[0];
        break;
    }
    return table;
}

/*************************************************************************
 *                            @fn mminfo_update()
 *************************************************************************
 *
 * @brief
 *    Update metadata memory data
 *
 * @param :
 *    obj:      object to update
 *    type:     object type when we dump
 *    argc:     command line argument number
 *    argv:     arguments array
 *
 * @return:
 *    None
 *
 * @author
 *    10/29/2013 - Created. Jamin Kang
 *************************************************************************/
static void mminfo_update(fbe_object_id_t obj, fbe_topology_object_type_t type, int argc, char *argv[])
{
    struct fbe_cli_field_map_array *table = get_table(type);
    fbe_base_config_control_metadata_memory_update_t update_data;
    fbe_status_t status;
    fbe_cli_field_parse_contex_t contex;
    int ret;

    fbe_zero_memory(&update_data, sizeof(update_data));
    fbe_zero_memory(&contex, sizeof(contex));
    contex.data = &update_data.memory_data[0];
    contex.mask = &update_data.memory_mask[0];
    contex.len = sizeof(update_data.memory_data);

    ret = fbe_cli_field_map_parse(argc, argv, table, &contex);
    if (ret < 0) {
        return;
    }

    update_data.memory_offset = contex.update_offset;
    update_data.memory_size = contex.update_size;

    status = fbe_api_base_config_metadata_memory_update(obj, &update_data);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Update metada failed, error: %u\n", (fbe_u32_t)status);
    } else {
        mminfo_dump(obj, type, FBE_FALSE);
    }
}

static fbe_status_t get_object_type(fbe_object_id_t id, fbe_topology_object_type_t *type)
{
    fbe_status_t status;

    status = fbe_api_get_object_type(id, type, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Can't get object type for 0x%x, return: %u\n", id, (fbe_u32_t)status);
        return status;
    }

    return FBE_STATUS_OK;
}

static void fbe_cli_mminfo_print_switches(void)
{
    fbe_cli_field_map_print_switches("base_config", base_config_map, ARRAY_LEN(base_config_map));
    fbe_cli_field_map_print_switches("lun", lun_map, ARRAY_LEN(lun_map));
    fbe_cli_field_map_print_switches("raid_group/vd", raid_group_map, ARRAY_LEN(raid_group_map));
    fbe_cli_field_map_print_switches("pvd", pvd_map, ARRAY_LEN(pvd_map));
    fbe_cli_printf("\n");
}

static void fbe_cli_mminfo_usage(void)
{
    fbe_cli_printf(MMINFO_CMD_USAGE);
    fbe_cli_mminfo_print_switches();
}

/************************************************************************
 * Public functions
 ************************************************************************/

/*************************************************************************
 *                            @fn fbe_cli_mminfo ()
 *************************************************************************
 *
 * @brief
 *    Function to implement mminfo commands
 *
 * @param :
 *    argc     (INPUT) NUMBER OF ARGUMENTS
 *    argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *    None
 *
 * @author
 *    10/29/2013 - Created. Jamin Kang
 *************************************************************************/
void fbe_cli_mminfo(int argc, char** argv)
{
    fbe_status_t status;
    fbe_topology_object_type_t type;
    int ret;
    fbe_object_id_t obj;
    fbe_bool_t is_peer = FBE_FALSE;

    if (argc <= 0 ||
        !strcmp(argv[0], "-help") ||
        !strcmp(argv[0], "-h")) {
        fbe_cli_mminfo_usage();
        return;
    }

    if (!strcmp(argv[0], "-p")) {
        is_peer = FBE_TRUE;
        argc -= 1;
        argv += 1;
    }

    ret = fbe_cli_field_get_object_id(argc, argv, &obj);
    if (ret < 0) {
        fbe_cli_mminfo_usage();
        return;
    }
    argc -= ret;
    argv += ret;

    status = get_object_type(obj, &type);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("%s: WARNING: unable to determine object type\n", __FUNCTION__);
    }


    if (is_peer && argc != 0) {
        fbe_cli_error("%s: don't allow to update peer memory. You can update it from another SP\n",
                      __FUNCTION__);
        return;
    }

    if (argc == 0) {
        mminfo_dump(obj, type, is_peer);
    } else {
        mminfo_update(obj, type, argc, argv);
    }
}
/******************************************************
 * end fbe_cli_lib_mminfo()
 ******************************************************/
