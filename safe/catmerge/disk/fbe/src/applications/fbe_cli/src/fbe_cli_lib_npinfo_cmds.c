/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_npinfo_cmds.c
 ***************************************************************************
 *
 * @brief
 *    This file contains cli functions for the npinfo related features in
 *    FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *    04/16/2013 - Created. Jamin Kang
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

#include <stdio.h>

#define ARRAY_LEN(a)    (sizeof(a) / sizeof(a[0]))

#define MEM_OFF(type, mem)  (unsigned int)CSX_OFFSET_OF(type, mem)
#define MEM_SIZE(type, mem) ((unsigned int)sizeof(((type *)0)->mem))

#define MEMBER(type, cmd, mem, dump, set)       \
    { cmd, MEM_OFF(type, mem), MEM_SIZE(type, mem), dump, set }

#define MEMBER_BC(cmd, mem, dump, set)          \
    MEMBER(fbe_base_config_nonpaged_metadata_t, cmd, mem, dump, set)
#define MEMBER_BC_DEF(cmd, mem)                 \
    MEMBER_BC(cmd, mem, NULL, NULL)

#define MEMBER_LUN(cmd, mem, dump, set)         \
    MEMBER(fbe_lun_nonpaged_metadata_t, cmd, mem, dump, set)
#define MEMBER_LUN_DEF(cmd, mem)                \
    MEMBER_LUN(cmd, mem, NULL, NULL)

#define MEMBER_RG(cmd, mem, dump, set)          \
    MEMBER(fbe_raid_group_nonpaged_metadata_t, cmd, mem, dump, set)
#define MEMBER_RG_DEF(cmd, mem)                 \
    MEMBER_RG(cmd, mem, NULL, NULL)

#define MEMBER_PVD(cmd, mem, dump, set)         \
    MEMBER(fbe_provision_drive_nonpaged_metadata_t, cmd, mem, dump, set)
#define MEMBER_PVD_DEF(cmd, mem)                \
    MEMBER_PVD(cmd, mem, NULL, NULL)


#define MEM_DUMP_LEVEL  "    "

/* 64 bytes should be enough for all fields
 * max length of the fields is version_header (16 bytes)
 */
#define MAX_FIELD_LENGTH    (64)

/*
 * return value definition for string parse
 */
enum {
    P_OK        = 0,
    P_INVAL     = -1,   /* invalid character in string */
    P_RANGE     = -2,   /* out of range */
};

struct field_map {
    const char *name;
    unsigned int offset;
    unsigned int size;
    void (*dump) (struct field_map *map, void *data);

    /* function pointer for setting field.
     * Return:
     *     < 0:     error.
     *     >= 0     bytes set
     */
    int (*set)(struct field_map *map, const char *s, void *data);
};

struct field_map_array {
    struct field_map *map;
    int len;
};

/******************************************************************************
 *   Private functions declare
 *****************************************************************************/

/* dump callbacks */
static void dump_version_header(struct field_map *map, void *data);
static void dump_rm_ts(struct field_map *map, void *data);
static void dump_metadata_metadata(struct field_map *map, void *data);
static void dump_null(struct field_map *map, void *data);

/* parameter parser callback */
static int npinfo_set_integer(struct field_map *map, const char *s, void *data);
static int set_version_header(struct field_map *map, const char *s, void *data);
static int set_mdd_valid(struct field_map *map, const char *s, void *data);
static int set_mdd_verify(struct field_map *map, const char *s, void *data);
static int set_mdd_nr(struct field_map *map, const char *s, void *data);
static int set_mdd_rekey(struct field_map *map, const char *s, void *data);
static int set_mdd_reserved(struct field_map *map, const char *s, void *data);
static int set_rm_ts(struct field_map *map, const char *s, void *data);
/* set callbacks */

/* utils functions */
static fbe_status_t npinfo_get_nonpaged_metadata(fbe_object_id_t obj,
                                                 fbe_metadata_nonpaged_data_t *nd);
static fbe_status_t npinfo_write_nonpaged_metadata(fbe_object_id_t id,
                                                   fbe_topology_object_type_t type,
                                                   fbe_metadata_nonpaged_data_t *nd,
                                                   int offset, int len);

static int get_u8(const char *str, fbe_u8_t *value);
static int get_u16(const char *str, fbe_u16_t *value);
static int get_u32(const char *str, fbe_u32_t *value);
static int get_u64(const char *str, fbe_u64_t *value);
static char *get_token(char *s, char **saved);

/******************************************************************************
 *   Private variables definition
 *****************************************************************************/

static struct field_map base_config_map[] = {
    MEMBER_BC("bc_version_header", version_header, dump_version_header, set_version_header),
    MEMBER_BC_DEF("bc_generation_number", generation_number),
    MEMBER_BC_DEF("bc_object_id", object_id),
    MEMBER_BC_DEF("bc_np_md_state", nonpaged_metadata_state),
    MEMBER_BC_DEF("bc_operation_bitmask", operation_bitmask),
};

static struct field_map lun_map[] = {
    MEMBER_LUN_DEF("z_chkpt", zero_checkpoint),
    MEMBER_LUN_DEF("has_been_written", has_been_written),
    MEMBER_LUN_DEF("flags", flags),
};

static struct field_map raid_group_map[] = {
    MEMBER_RG_DEF("rl_bitmask", rebuild_info.rebuild_logging_bitmask),
    MEMBER_RG_DEF("chkpt_0", rebuild_info.rebuild_checkpoint_info[0].checkpoint),
    MEMBER_RG_DEF("rb_pos_0", rebuild_info.rebuild_checkpoint_info[0].position),
    MEMBER_RG_DEF("chkpt_1", rebuild_info.rebuild_checkpoint_info[1].checkpoint),
    MEMBER_RG_DEF("rb_pos_1", rebuild_info.rebuild_checkpoint_info[1].position),
    MEMBER_RG_DEF("ro_vr_chkpt", ro_verify_checkpoint),
    MEMBER_RG_DEF("rw_vr_chkpt", rw_verify_checkpoint),
    MEMBER_RG_DEF("err_vr_chkpt", error_verify_checkpoint),
    MEMBER_RG_DEF("jvr_chkpt", journal_verify_checkpoint),
    MEMBER_RG_DEF("iwvr_chkpt", incomplete_write_verify_checkpoint),
    MEMBER_RG_DEF("sys_vr_chkpt", system_verify_checkpoint),
    MEMBER_RG_DEF("rg_np_flags", raid_group_np_md_flags),
    MEMBER_RG_DEF("rg_np_extended_flags", raid_group_np_md_extended_flags),
    MEMBER_RG_DEF("rekey_chkpt", encryption.rekey_checkpoint),
    MEMBER_RG_DEF("encrypt_flags",encryption.flags),
    MEMBER_RG_DEF("glitch_disks_bm", glitching_disks_bitmask),
    MEMBER_RG_DEF("drive_tier", drive_tier),

    MEMBER_RG("mdd_valid", paged_metadata_metadata, dump_metadata_metadata, set_mdd_valid),
    MEMBER_RG("mdd_verify", paged_metadata_metadata, dump_null, set_mdd_verify),
    MEMBER_RG("mdd_nr", paged_metadata_metadata, dump_null, set_mdd_nr),
    MEMBER_RG("mdd_rekey", paged_metadata_metadata, dump_null, set_mdd_rekey),
    MEMBER_RG("mdd_reserved", paged_metadata_metadata, dump_null, set_mdd_reserved),
};

static struct field_map pvd_map[] = {
    MEMBER_PVD_DEF("sv_chkpt", sniff_verify_checkpoint),
    MEMBER_PVD_DEF("z_chkpt", zero_checkpoint),
    MEMBER_PVD_DEF("zod", zero_on_demand),
    MEMBER_PVD_DEF("eol", end_of_life_state),
    MEMBER_PVD_DEF("port", nonpaged_drive_info.port_number),
    MEMBER_PVD_DEF("encl", nonpaged_drive_info.enclosure_number),
    MEMBER_PVD_DEF("slot", nonpaged_drive_info.slot_number),
    MEMBER_PVD_DEF("drive_type", nonpaged_drive_info.drive_type),
    MEMBER_PVD_DEF("media_error_lba", media_error_lba),
    MEMBER_PVD("remove_ts", remove_timestamp, dump_rm_ts, set_rm_ts),
    MEMBER_PVD_DEF("verify_inv_chkpt", verify_invalidate_checkpoint),
    MEMBER_PVD_DEF("drive_fault_state", drive_fault_state),
    MEMBER_PVD_DEF("flags",flags),
    MEMBER_PVD_DEF("v_area_bmp", validate_area_bitmap),
};

static struct field_map_array base_config_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &pvd_map[0], ARRAY_LEN(pvd_map) },
    { &raid_group_map[0], ARRAY_LEN(raid_group_map) },
    { &lun_map[0], ARRAY_LEN(lun_map) },
    { NULL, 0 },
};
static struct field_map_array raid_group_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &raid_group_map[0], ARRAY_LEN(raid_group_map) },
    { NULL, 0 },
};
static struct field_map_array lun_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &lun_map[0], ARRAY_LEN(lun_map) },
    { NULL, 0 },
};
static struct field_map_array pvd_table[] = {
    { &base_config_map[0], ARRAY_LEN(base_config_map) },
    { &pvd_map[0], ARRAY_LEN(pvd_map) },
    { NULL, 0 },
};

/******************************************************************************
 *   Private functions definition
 *****************************************************************************/
static fbe_bool_t map_is_len_valid(struct field_map *map, fbe_u8_t *data, unsigned int len)
{
    if (len < map->offset + map->size) {
        fbe_cli_error("%s: Unexcepted len: %u, field offset: %u, field size: %u\n",
                      map->name, len, map->offset, map->size);
        return FBE_FALSE;
    }

    return FBE_TRUE;
}

static struct field_map *find_map_in_array(const char *str, struct field_map_array *array)
{
    int i;
    struct field_map *map = array->map;

    for (i = 0; i < array->len; i++, map++) {
        if (strcmp(str, map->name) == 0)
            return map;
    }
    return NULL;
}

static struct field_map * find_field_map(const char *str, struct field_map_array *fields)
{
    const char *s = str;
    struct field_map *map = NULL;

    if (*s != '-')
        return NULL;
    s++;
    /* We allow '-' and '--' switches */
    if (*s == '-')
        s++;

    for ( ; fields->len; fields++) {
        map = find_map_in_array(s, fields);
        if (map)
            break;
    }
    return map;
}

/********************************************************************************
 *     Private functions for dumping nonpaged metadata.
 ********************************************************************************/

static void dump_show_member(const char *name)
{
    char buf[64];

    fbe_sprintf(buf, sizeof(buf), "%s:", name);
    fbe_cli_printf("%s%-16s", MEM_DUMP_LEVEL, buf);
}

static void dump_show_name(const char *name)
{
    char buf[64];

    fbe_sprintf(buf, sizeof(buf), "%s:", name);
    fbe_cli_printf("%-24s", buf);
}

static void dump_version_header(struct field_map *map, void *data)
{
    fbe_sep_version_header_t *s = (fbe_sep_version_header_t *)data;
    int i;

    fbe_cli_printf("%s:\n", map->name);
    dump_show_member("size");
    fbe_cli_printf("0x%08x\n", s->size);
    dump_show_member("reserve");
    for (i = 0; i < ARRAY_LEN(s->padded_reserve_space) - 1; i++)
        fbe_cli_printf("0x%08x ", s->padded_reserve_space[i]);
    fbe_cli_printf("0x%08x\n", s->padded_reserve_space[i]);
}

static void dump_rm_ts(struct field_map *map, void *data)
{
    fbe_system_time_t *rm_ts = (fbe_system_time_t *)data;

    dump_show_name(map->name);
    fbe_cli_printf("%02u-%02u-%02u(W %02u) %02u:%02u:%02u:%03u\n",
                   rm_ts->year, rm_ts->month, rm_ts->day, rm_ts->weekday,
                   rm_ts->hour, rm_ts->minute, rm_ts->second, rm_ts->milliseconds);
}

static void dump_metadata(int slot, fbe_raid_group_paged_metadata_t *mdd)
{
    fbe_cli_printf("  Slot %d\n", slot);
    fbe_cli_printf("    valid: %x, verify: %02x, nr: %04x, rekey: %x, reserved: %02x\n",
                   mdd->valid_bit, mdd->verify_bits,
                   mdd->needs_rebuild_bits, mdd->rekey, mdd->reserved_bits);
}

static void dump_metadata_slots(fbe_raid_group_paged_metadata_t *mdd,
                                unsigned int slots)
{
    int i;

    fbe_cli_printf("paged_metadata_metadata:\n");
    for (i = 0; i < slots; i++, mdd++)
        dump_metadata(i, mdd);
    fbe_cli_printf("\n");
}

static void dump_metadata_metadata(struct field_map *map, void *data)
{
    fbe_raid_group_paged_metadata_t *md = (fbe_raid_group_paged_metadata_t *)data;
    dump_metadata_slots(md, map->size / sizeof(*md));
}

static void dump_null(struct field_map *map, void *data)
{
}

static void def_dump(struct field_map *map, void *data)
{
    if (map->size > sizeof(fbe_u64_t)) {
        fbe_cli_printf("\n");
        fbe_cli_error("%s(%u)unexcepted internal error: size: %u\n",
                      __FUNCTION__, __LINE__, map->size);
        return;
    }

    dump_show_name(map->name);
    switch (map->size) {
    case sizeof(fbe_u8_t):
        fbe_cli_printf("0x%02x", *(fbe_u8_t *)data);
        break;
    case sizeof(fbe_u16_t):
        fbe_cli_printf("0x%04x", *(fbe_u16_t *)data);
        break;
    case sizeof(fbe_u32_t):
        fbe_cli_printf("0x%08x", *(fbe_u32_t *)data);
        break;
    case sizeof(fbe_u64_t):
        fbe_cli_printf("0x%016llx", (unsigned long long)*(fbe_u64_t *)data);
        break;
    default:
        fbe_cli_error("%s(%u): unexcepted size error: size: %u\n",
                      __FUNCTION__, __LINE__, map->size);
        break;
    }
    fbe_cli_printf("\n");
}

static void dump_member(fbe_metadata_nonpaged_data_t *nd, struct field_map *map, int sz)
{
    fbe_u8_t *data = &nd->data[0];
    int i;

    for (i = 0; i < sz; i++, map++) {
        if (!map_is_len_valid(map, data, sizeof(nd->data)))
            continue;
        if (map->dump)
            map->dump(map, &data[map->offset]);
        else
            def_dump(map, &data[map->offset]);
    }
}

static void dump_type(fbe_object_id_t obj, fbe_topology_object_type_t type)
{
    const char *s;

    switch (type) {
    case FBE_TOPOLOGY_OBJECT_TYPE_LUN:
        s = "lun";
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP:
        s = "raid group";
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE:
        s = "virtual dirve";
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE:
        s = "provision drive";
        break;
    default:
        s = "<unknown>";
        break;
    }
    fbe_cli_printf("\nObject: 0x%x, Type: %s\n\n", (fbe_u32_t)obj, s);
}

static void dump_base(fbe_metadata_nonpaged_data_t *nd, fbe_topology_object_type_t type)
{
    dump_member(nd, &base_config_map[0], ARRAY_LEN(base_config_map));
}

static void dump_lun(fbe_metadata_nonpaged_data_t *nd, fbe_topology_object_type_t type)
{
    dump_base(nd, type);
    dump_member(nd, &lun_map[0], ARRAY_LEN(lun_map));
}

static void dump_raid_group(fbe_metadata_nonpaged_data_t *nd, fbe_topology_object_type_t type)
{
    dump_base(nd, type);
    dump_member(nd, &raid_group_map[0], ARRAY_LEN(raid_group_map));
}

static void dump_pvd(fbe_metadata_nonpaged_data_t *nd, fbe_topology_object_type_t type)
{
    dump_base(nd, type);
    dump_member(nd, &pvd_map[0], ARRAY_LEN(pvd_map));
}

static void npinfo_do_dump(fbe_object_id_t obj, fbe_topology_object_type_t type,
                           fbe_metadata_nonpaged_data_t *nd)
{
    dump_type(obj, type);

    switch (type) {
    case FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP:
    case FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE:
        dump_raid_group(nd, type);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_LUN:
        dump_lun(nd, type);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE:
        dump_pvd(nd, type);
        break;
    default:
        dump_base(nd, type);
        break;
    }
}

static void npinfo_dump(fbe_object_id_t obj, fbe_topology_object_type_t type)
{
    fbe_metadata_nonpaged_data_t nd;

    if (npinfo_get_nonpaged_metadata(obj, &nd) != FBE_STATUS_OK)
        return;

    npinfo_do_dump(obj, type, &nd);
}

/**********************************************************************************
 *    Private functions for setting nonpage metadata
 **********************************************************************************/

static int set_version_header(struct field_map *map, const char *s, void *data)
{
    int ret;

    ret = get_u32(s, data);
    if (ret < 0)
        return -1;

    return sizeof(fbe_u32_t);
}

static int set_mdd_valid(struct field_map *map, const char *s, void *data)
{
    int ret;
    fbe_u8_t valid;
    fbe_raid_group_paged_metadata_t *pmd = data;
    int i;

    ret = get_u8(s, &valid);
    if (ret < 0)
        return -1;
    if (valid != 0x0 && valid != 0x1) {
        fbe_cli_error("Argument for '%s' field should be 0/1\n", map->name);
        return -1;
    }

    for (i = 0; i < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; i++, pmd++)
        pmd->valid_bit = valid;

    return sizeof(*pmd) * FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;
}

static int set_mdd_verify(struct field_map *map, const char *s, void *data)
{
    int ret;
    fbe_u8_t verify;
    fbe_raid_group_paged_metadata_t *pmd = data;
    int i;

    ret = get_u8(s, &verify);
    if (ret < 0)
        return -1;
    if (verify > 0x7f) {
        fbe_cli_error("Argument for '%s' field should < 0x7f\n", map->name);
        return -1;
    }

    for (i = 0; i < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; i++, pmd++)
        pmd->verify_bits = verify;

    return sizeof(*pmd) * FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;
}

static int set_mdd_nr(struct field_map *map, const char *s, void *data)
{
    int ret;
    fbe_u16_t nr;
    fbe_raid_group_paged_metadata_t *pmd = data;
    int i;

    ret = get_u16(s, &nr);
    if (ret < 0)
        return -1;

    for (i = 0; i < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; i++, pmd++)
        pmd->needs_rebuild_bits = nr;

    return sizeof(*pmd) * FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;
}

static int set_mdd_rekey(struct field_map *map, const char *s, void *data)
{
    int ret;
    fbe_u8_t rekey;
    fbe_raid_group_paged_metadata_t *pmd = data;
    int i;

    ret = get_u8(s, &rekey);
    if (ret < 0)
        return -1;

    for (i = 0; i < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; i++, pmd++)
        pmd->rekey = rekey;

    return sizeof(*pmd) * FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;
}
static int set_mdd_reserved(struct field_map *map, const char *s, void *data)
{
    int ret;
    fbe_u8_t reserved;
    fbe_raid_group_paged_metadata_t *pmd = data;
    int i;

    ret = get_u8(s, &reserved);
    if (ret < 0)
        return -1;

    for (i = 0; i < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; i++, pmd++)
        pmd->reserved_bits = reserved;

    return sizeof(*pmd) * FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS;
}

static int set_rm_ts_time(char *cur, fbe_u16_t *st)
{
    fbe_u16_t cur_time;

    if (strcmp(cur, "") == 0) {
        cur_time = 0;
    } else {
        int ret;

        ret = get_u16(cur, &cur_time);
        if (ret < 0) {
            return -1;
        }
    }
    *st = cur_time;
    return 0;
}

static int set_rm_ts(struct field_map *map, const char *s, void *data)
{
    char tmp_str[128];
    /* WARNING: we assume all elements in fbe_system_time_t are fbe_u16_t */
    fbe_u16_t *st = data;
    int len = sizeof(fbe_system_time_t) / sizeof(fbe_u16_t);
    int i;
    char *cur, *saved = NULL;

    if (strlen(s) > sizeof(tmp_str) - 1) {
        fbe_cli_error("%s: argument '%s' is too long\n", map->name, s);
        return -1;
    }

    strcpy(tmp_str, s);
    memset(st, 0, sizeof(fbe_system_time_t));

    cur = get_token(tmp_str, &saved);
    if (cur == NULL) {
        return -1;
    }

    for (i = 0; i < len && cur; i++, st++) {
        if (set_rm_ts_time(cur, st) < 0) {
            fbe_cli_error("%s: invalid time '%s'\n", map->name, cur);
            return -1;
        }
        cur = get_token(NULL, &saved);
    }

    return sizeof(fbe_system_time_t);
}

static int npinfo_set_integer(struct field_map *map, const char *s, void *data)
{
    int ret;
    int len = -1;

    switch (map->size) {
    case sizeof(fbe_u8_t):
        ret = get_u8(s, data);
        len = sizeof(fbe_u8_t);
        break;
    case sizeof(fbe_u16_t):
        ret = get_u16(s, data);
        len = sizeof(fbe_u16_t);
        break;
    case sizeof(fbe_u32_t):
        ret = get_u32(s, data);
        len = sizeof(fbe_u32_t);
        break;
    case sizeof(fbe_u64_t):
        ret = get_u64(s, data);
        len = sizeof(fbe_u64_t);
        break;
    default:
        ret = -1;
        len = -1;
    }

    if (ret < 0)
        return ret;

    return len;
}

static struct field_map_array *get_table(fbe_topology_object_type_t type)
{
    struct field_map_array *table = &base_config_table[0];

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

static void npinfo_set(fbe_object_id_t obj, fbe_topology_object_type_t type, int argc, char *argv[])
{
    struct field_map_array *table = get_table(type);
    fbe_metadata_nonpaged_data_t nd;
    int write_end = 0;
    int write_start = sizeof(nd);
    fbe_status_t status;

    if (npinfo_get_nonpaged_metadata(obj, &nd) != FBE_STATUS_OK)
        return;

    while (argc) {
        char *name = argv[0];
        struct field_map *map;
        int ret;
        fbe_u8_t *data;
        unsigned int len;

        map = find_field_map(name, table);
        if (map == NULL ) {
            fbe_cli_error("Invalid field %s for object 0x%x\n",
                          name, (fbe_u32_t)obj);
            return;
        }

        argc--;
        argv++;
        if (argc == 0) {
            fbe_cli_error("Field %s needs an argument\n", name);
            return;
        }

        data = &nd.data[map->offset];
        if (map->set)
            ret = map->set(map, argv[0], data);
        else
            ret = npinfo_set_integer(map, argv[0], data);

        if (ret < 0) {
            fbe_cli_error("Argument '%s' for field '%s' is invalid\n",
                          argv[0], name);
            return;
        }
        argc--;
        argv++;

        len = ret;
        if (map->offset < write_start)
            write_start = map->offset;
        if (map->offset + len > write_end)
            write_end = map->offset + len;
    }
    /* sanity check */
    if (write_start >= write_end || write_start < 0 || write_end > sizeof(nd)) {
        fbe_cli_error("Range error: %u - %u\n", write_start, write_end);
        return;
    }
    status = npinfo_write_nonpaged_metadata(obj, type,
                                            &nd, write_start, write_end - write_start);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Write nonpaged metada failed, error: %u\n", (fbe_u32_t)status);
    }
}

/******************************************************************************
 *     Utils functions
 ******************************************************************************/

/*************************************************************************
 *                            @fn get_token ()
 *************************************************************************
 *
 * @brief
 *    Function to split string by ':'
 *    We have to reimplement strtok_r as MSVC dosen't support C99
 *    *WARNING*: different from strtok_r: when encounter empty string(""), we
 *    return "" for the first time, and NULL for next iteration.
 *
 * @param :
 *    str     (INPUT) string to split
 *    saved   (OUTPUT) the pointer for next iteration.
 *
 * @return:
 *    NULL:    string end
 *    others:  string contains more data
 *
 * @author
 *    04/24/2013 - Created. Jamin Kang
 *************************************************************************/
static char *get_token(char *s, char **saved)
{
    char *start;
    char *end;

    /* If both s and *saved are NULL, we encounter end of string */
    if (s == NULL && *saved == NULL)
        return NULL;

    if (s == NULL)
        start = *saved;
    else
        start = s;

    end = start;
    while (*end != '\0' && *end != ':')
        end++;

    if (*end != '\0') {
        *end = '\0';
        end++;
    }

    if (end == start) {
        *saved = NULL;
    } else {
        *saved = end;
    }
    return start;
}

/*************************************************************************
 *                            @fn char_to_int ()
 *************************************************************************
 *
 * @brief
 *    Convert a character to a integer.
 *    For dec, character can be [0-9]
 *    For hex, character can be [0-9A-Fa-f]
 *
 * @param :
 *    c       (INPUT) character to convert
 *    base    (INPUT) base of the character (10 or 16)
 *    v       (OUTPUT) the integer returned
 *
 * @return:
 *    0:    success
 *    -1:   character is invalid.
 *
 * @author
 *    04/18/2013 - Created. Jamin Kang
 *************************************************************************/
static int char_to_int(char c, int base, int *v)
{
    int ret = 0;

    *v = 0;
    if (c >= '0' && c <='9') {
        *v = c - '0';
    } else if (base == 16) {
        if (c >= 'a' && c <= 'f')
            *v = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            *v = c - 'A' + 10;
        else
            ret = -1;
    } else {
        ret = -1;
    }

    return ret;
}

/*************************************************************************
 *                            @fn get_u64 ()
 *************************************************************************
 *
 * @brief
 *    Function to convert string to u64 integer.
 *    We have to reimplement strtoull as MSVC dosen't support C99
 *    Only support unsigned integer, can't deal with '+' '-'.
 *    *WARNING*: leading 0 doesn't means octal radix.
 *
 * @param :
 *    str     (INPUT) string to convert
 *    value   (OUTPUT) the integer
 *
 * @return:
 *    0:    success
 *    -1:   str contains invalid character
 *    -2:   integer overflow. value will be set to FBE_U64_MAX.
 *
 * @author
 *    04/18/2013 - Created. Jamin Kang
 *************************************************************************/
static int get_u64(const char *str, fbe_u64_t *value)
{
    const char *s = str;
    int base = 10;
    fbe_u64_t n;
    int conv_result = 0;
    fbe_u64_t max_i, max_left;

    if(str == NULL)
        return P_INVAL;

    /* Remove leading space */
    while (*s == ' ')
        s++;

    if (s[0] == '0' &&
        (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        s += 2;
    }

    max_i = FBE_U64_MAX / base;
    max_left = FBE_U64_MAX % base;

    for (n = 0; *s; s++) {
        int ret;
        int v;

        ret = char_to_int(*s, base, &v);
        if (ret < 0) {
            conv_result = P_INVAL;
            break;
        }
        if (n > max_i || (n == max_i && v > max_left)) {
            conv_result = P_RANGE;
            break;
        }
        n = n * base + v;
    }
    if (conv_result == P_RANGE)
        *value = FBE_U64_MAX;
    else
        *value = n;

    return conv_result;
}

static int get_u8(const char *str, fbe_u8_t *value)
{
    int ret;
    fbe_u64_t v;

    ret = get_u64(str, &v);
    if (ret < 0)
        return ret;
    if (v > 0xff) {
        *value = 0xff;
        return P_RANGE;
    }

    *value = (fbe_u8_t)v;
    return 0;
}

static int get_u16(const char *str, fbe_u16_t *value)
{
    int ret;
    fbe_u64_t v;

    ret = get_u64(str, &v);
    if (ret < 0)
        return ret;
    if (v > FBE_U16_MAX) {
        *value = FBE_U16_MAX;
        return P_RANGE;
    }

    *value = (fbe_u16_t)v;
    return 0;
}

static int get_u32(const char *str, fbe_u32_t *value)
{
    int ret;
    fbe_u64_t v;

    ret = get_u64(str, &v);
    if (ret < 0)
        return ret;
    if (v > FBE_U32_MAX) {
        *value = FBE_U32_MAX;
        return -2;
    }
    *value = (fbe_u32_t)v;

    return 0;
}

static int get_object_id(int argc, char *argv[], fbe_object_id_t *id)
{
    const char *obj_cmd = "-object_id";
    fbe_u32_t v;

    if (argc < 2)
        return -1;
    if (strcmp(argv[0], obj_cmd) != 0)
        return -1;
    if (get_u32(argv[1], &v) < 0) {
        fbe_cli_error("Invalid object id: %s\n", argv[1]);
        return -1;
    }
    *id = (fbe_object_id_t)v;

    return 2;
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

/*************************************************************************
 *                     @fn npinfo_write_nonpaged_metadata ()
 *************************************************************************
 * @brief
 *     call api to write nonpaged metadata
 *
 * @param:
 *     obj      (INPUT) the object to write
 *     nd       (INPUT) pointer to nonpaged metadata
 *
 * @return:
 *    FBE_STATUS_OK for success. Others for error
 *
 * @author
 *     04/19/2013 - created. Jamin Kang
 *
 ************************************************************************/
static fbe_status_t npinfo_write_nonpaged_metadata(fbe_object_id_t id,
                                                   fbe_topology_object_type_t type,
                                                   fbe_metadata_nonpaged_data_t *nd,
                                                   int offset, int len)
{
    fbe_api_base_config_metadata_nonpaged_change_bits_t change;
    fbe_status_t status;

    fbe_cli_printf("Write 0x%x: offset %u, len %u\n",
                   (fbe_u32_t)id, offset, len);
    npinfo_do_dump(id, type, nd);

    if (len > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE) {
        fbe_cli_printf("%s: due to api limited, only %u bytes can be set\n",
                       __FUNCTION__, FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    change.metadata_offset = (fbe_u64_t)offset;
    memcpy(&change.metadata_record_data[0], &nd->data[offset], len);
    change.metadata_record_data_size = (fbe_u32_t)len;
    change.metadata_repeat_count = 0;
    change.metadata_repeat_offset = 0;

    status = fbe_api_base_config_metadata_nonpaged_write_persist(id, &change);

    return status;
}

/*************************************************************************
 *                     @fn npinfo_get_nonpaged_metadata ()
 *************************************************************************
 * @brief
 *     call api to read nonpaged metadata
 *
 * @param:
 *    obj       (INPUT) the object id of metadata
 *    nd        (OUTPUT) for saving nonpaged metadata
 *
 * @return:
 *    FBE_STATUS_OK for success. Others for error
 *
 * @author
 *     04/19/2013 - created. Jamin Kang
 ************************************************************************/
static fbe_status_t npinfo_get_nonpaged_metadata(fbe_object_id_t obj,
                                                 fbe_metadata_nonpaged_data_t *nd)
{
    fbe_status_t status;

    status = fbe_api_base_config_metadata_get_data(obj, nd);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Get nonpaged metadata failed: %u\n", (fbe_u32_t)status);
    }

    return status;
}

static void print_switches(const char *name, struct field_map *map, unsigned int len)
{
    unsigned int i;

    fbe_cli_printf("\nSwitches for %s:", name);
    for (i = 0; i < len; i++) {
        if (i % 4 == 0)
            fbe_cli_printf("\n");
        fbe_cli_printf("    -%s", map[i].name);
    }
    fbe_cli_printf("\n");
}

static void fbe_cli_npinfo_print_switches(void)
{
    print_switches("base_config", base_config_map, ARRAY_LEN(base_config_map));
    print_switches("lun", lun_map, ARRAY_LEN(lun_map));
    print_switches("raid_group/vd", raid_group_map, ARRAY_LEN(raid_group_map));
    print_switches("pvd", pvd_map, ARRAY_LEN(pvd_map));
    fbe_cli_printf("\nFor pvd -remove_ts, use format:\n");
    fbe_cli_printf("    -remove_ts year:month:wd:day:hour:minute:second:ms\n");
    fbe_cli_printf("\n");
}

/*************************************************************************
 *                     @fn fbe_cli_npinfo_usage ()
 *************************************************************************
 *
 * @brief
 *    Function to show npinfo usage
 *
 * @param :
 *    None
 *
 * @return:
 *    None
 *
 * @author
 *    04/17/2013 - Created. Jamin Kang
 *************************************************************************/
static void fbe_cli_npinfo_usage(void)
{
    fbe_cli_printf(NPINFO_CMD_USAGE);
    fbe_cli_npinfo_print_switches();
}

/*************************************************************************
 *                            @fn fbe_cli_npinfo ()
 *************************************************************************
 *
 * @brief
 *    Function to implement npinfo commands
 *
 * @param :
 *    argc     (INPUT) NUMBER OF ARGUMENTS
 *    argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *    None
 *
 * @author
 *    04/16/2013 - Created. Jamin Kang
 *************************************************************************/
void fbe_cli_npinfo(int argc, char** argv)
{
    fbe_status_t status;
    fbe_topology_object_type_t type;
    int ret;
    fbe_object_id_t obj;

    if (argc <= 0 ||
        !strcmp(argv[0], "-help") ||
        !strcmp(argv[0], "-h")) {
        fbe_cli_npinfo_usage();
        return;
    }

    ret = get_object_id(argc, argv, &obj);
    if (ret < 0) {
        fbe_cli_npinfo_usage();
        return;
    }

    status = get_object_type(obj, &type);
    if (status != FBE_STATUS_OK) {
        return;
    }

    argc -= ret;
    argv += ret;
    if (argc == 0) {
        npinfo_dump(obj, type);
    } else {
        npinfo_set(obj, type, argc, argv);
    }
}
/******************************************************
 * end fbe_cli_lib_npinfo()
 ******************************************************/
