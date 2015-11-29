/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_field_utils.c
 ***************************************************************************
 *
 * @brief
 *    This file contains cli functions for structure field operation
 *
 * @ingroup fbe_cli
 *
 * @revision
 *    10/31/2013 - Created. Jamin Kang
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_private.h"
#include "fbe_cli_field_utils.h"
#include "fbe/fbe_api_utils.h"



/*************************************************************************
 *                        @fn fbe_cli_field_map_is_len_valid
 *************************************************************************
 *
 * @brief
 *    Check if some data has vaild length
 *
 * @param :
 *    map:     The map for checking
 *    data:    Unused
 *    len:     The length to check
 * @return:
 *    FBE_TRUE:    The length is valid
 *    FBE_FALSE:   The length is invalid
 *
 * @author
 *    10/29/2013 - Created. Jamin Kang
 *************************************************************************/
fbe_bool_t fbe_cli_field_map_is_len_valid(struct fbe_cli_field_map *map, fbe_u8_t *data, fbe_u32_t len)
{
    if (len < map->offset + map->size) {
        fbe_cli_error("%s: Unexcepted len: %u, field offset: %u, field size: %u\n",
                      map->name, len, map->offset, map->size);
        return FBE_FALSE;
    }

    return FBE_TRUE;
}


static struct fbe_cli_field_map *find_map_in_array(const char *str, struct fbe_cli_field_map_array *array)
{
    int i;
    struct fbe_cli_field_map *map = array->map;

    for (i = 0; i < array->len; i++, map++) {
        if (strcmp(str, map->name) == 0) {
            return map;
        }
    }
    return NULL;
}

/*************************************************************************
 *                        @fn fbe_cli_field_map_find
 *************************************************************************
 *
 * @brief
 *    Find a field_map for specific keyword
 *
 * @param :
 *    str:     The keyword for searching
 *    fields:  Pointer to an array of field_map_array.
 *             This array should be terminated with FIELD_MAP_ARRAY_TERMINATION
 *
 * @return:
 *    NULL:   Can't find the map
 *    else:   Pointer to the map found
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
struct fbe_cli_field_map * fbe_cli_field_map_find(const char *str,
                                                  struct fbe_cli_field_map_array *fields)
{
    const char *s = str;
    struct fbe_cli_field_map *map = NULL;

    if (*s != '-') {
        return NULL;
    }
    s++;
    /* We allow '-' and '--' switches */
    if (*s == '-') {
        s++;
    }

    for ( ; fields->map && fields->len; fields++) {
        map = find_map_in_array(s, fields);
        if (map) {
            break;
        }
    }
    return map;
}


static void fbe_cli_field_map_dump_show_name(const char *name)
{
    char buf[64];

    fbe_sprintf(buf, sizeof(buf), "%s:", name);
    fbe_cli_printf("%-24s", buf);
}

/*************************************************************************
 *                        @fn fbe_cli_field_map_def_dump
 *************************************************************************
 *
 * @brief
 *    Default dump method for field_map.
 *    If map->dump == NULL, we will call this function to print the data
 *
 * @param :
 *    map:      the map
 *    data:     data to print
 *
 * @return:
 *    None
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
static void fbe_cli_field_map_def_dump(struct fbe_cli_field_map *map, void *data)
{
    if (map->size > sizeof(fbe_u64_t)) {
        fbe_cli_printf("\n");
        fbe_cli_error("%s(%u)unexcepted internal error: size: %u\n",
                      __FUNCTION__, __LINE__, map->size);
        return;
    }

    fbe_cli_field_map_dump_show_name(map->name);
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

/*************************************************************************
 *                        @fn fbe_cli_field_map_dump
 *************************************************************************
 *
 * @brief
 *    Print all map data to console
 *
 * @param :
 *    dump_data:    the data to print
 *    len:          length of the data
 *    map:          pointer to the map array
 *    map_num:      Number of maps
 *
 * @return:
 *    None
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
void fbe_cli_field_map_dump(void *dump_data, fbe_u32_t len,
                            struct fbe_cli_field_map *map, fbe_u32_t map_num)
{
    fbe_u8_t *data = dump_data;
    int i;

    for (i = 0; i < map_num; i++, map++) {
        if (!fbe_cli_field_map_is_len_valid(map, data, len)) {
            continue;
        }
        if (map->dump) {
            map->dump(map, &data[map->offset]);
        } else {
            fbe_cli_field_map_def_dump(map, &data[map->offset]);
        }
    }
}

/*************************************************************************
 *                        @fn fbe_cli_field_map_def_set
 *************************************************************************
 *
 * @brief
 *    Default set method for field_map.
 *    If map->set == NULL, we will call this function to print the data
 *
 * @param :
 *    map:      the map
 *    data:     data to print
 *
 * @return:
 *    >= 0:     length of data modified
 *    < 0:      failed
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
static int fbe_cli_field_map_def_set(struct fbe_cli_field_map *map, const char *s,
                                     void *data, void *mask)
{
    fbe_status_t status;
    int len = -1;

    switch (map->size) {
    case sizeof(fbe_u8_t):
        status = fbe_cli_strtoui8(s, data);
        len = sizeof(fbe_u8_t);
        if (mask != NULL) {
            *(fbe_u8_t *)mask = 0xff;
        }
        break;
    case sizeof(fbe_u16_t):
        status = fbe_cli_strtoui16(s, data);
        len = sizeof(fbe_u16_t);
        if (mask != NULL) {
            *(fbe_u16_t *)mask = 0xffff;
        }
        break;
    case sizeof(fbe_u32_t):
        status = fbe_cli_strtoui32(s, data);
        len = sizeof(fbe_u32_t);
        if (mask != NULL) {
            *(fbe_u32_t *)mask = 0xffffffff;
        }
        break;
    case sizeof(fbe_u64_t):
        status = fbe_cli_strtoui64(s, data);
        len = sizeof(fbe_u64_t);
        if (mask != NULL) {
            *(fbe_u64_t *)mask = 0xffffffffffffffff;
        }
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        len = -1;
        break;
    }

    if (status != FBE_STATUS_OK) {
        return -1;
    }
    return len;
}


/*************************************************************************
 *                        @fn fbe_cli_field_map_parse
 *************************************************************************
 *
 * @brief
 *    Parse command line and store value for a specific map table
 *
 * @param :
 *    argc:     number of arguments
 *    argv:     arguments array
 *    table:    the tabel for parsing
 *    contex:   contex for storing parsing result
 *
 * @return:
 *    >= 0:     length of arguments parsed
 *    < 0:      failed
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
int fbe_cli_field_map_parse(int argc, char *argv[],
                            struct fbe_cli_field_map_array *table,
                            fbe_cli_field_parse_contex_t *contex)
{
    fbe_u8_t *memory_data = contex->data;
    fbe_u8_t *memory_mask = contex->mask;
    fbe_u32_t memory_len = contex->len;
    fbe_u32_t update_start = memory_len;
    fbe_u32_t update_end = 0;
    int orig_argc = argc;

    if (argc <= 0) {
        fbe_cli_error("%s: invalid argument number: %d\n",
                      __FUNCTION__, argc);
        return -1;
    }

    while (argc > 0) {
        char *name = argv[0];
        struct fbe_cli_field_map *map;
        fbe_u8_t *data;
        fbe_u8_t *mask;
        int ret;
        int len;

        map = fbe_cli_field_map_find(name, table);
        if (map == NULL ) {
            fbe_cli_error("Invalid field %s\n", name);
            return -1;
        }
        if (!fbe_cli_field_map_is_len_valid(map, memory_data, memory_len)) {
            return -1;
        }

        argc--;
        argv++;

        if (argc == 0) {
            fbe_cli_error("Field %s needs an argument\n", name);
            return -1;
        }

        data = &memory_data[map->offset];
        mask = &memory_mask[map->offset];

        if (map->set) {
            ret = map->set(map, argv[0], data, mask);
        } else {
            ret = fbe_cli_field_map_def_set(map, argv[0], data, mask);
        }
        if (ret < 0) {
            fbe_cli_error("Argument '%s' for field '%s' is invalid\n",
                          argv[0], name);
            return -1;
        }
        len = ret;
        if (map->offset < update_start) {
            update_start = map->offset;
        }
        if (map->offset + len > update_end) {
            update_end = map->offset + len;
        }

        argc--;
        argv++;
    }

    /* sanity check */
    if (update_start >= update_end ||
        update_end > memory_len) {
        fbe_cli_error("%s: Range error: %u - %u\n",
                      __FUNCTION__, update_start, update_end);
        return -1;
    }
    contex->update_offset = update_start;
    contex->update_size = update_end - update_start;

    return orig_argc - argc;
}

/*************************************************************************
 *                        @fn fbe_cli_field_map_get_class_name
 *************************************************************************
 *
 * @brief
 *    Return class name for a specific object
 *
 * @param :
 *    obj:          the object
 *
 * @return:
 *    class name of the object.
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
const char * fbe_cli_field_map_get_class_name(fbe_object_id_t obj)
{
    fbe_status_t status;
    fbe_const_class_info_t *class_info;

    status = fbe_cli_get_class_info(obj, FBE_PACKAGE_ID_SEP_0, &class_info);
    if (status != FBE_STATUS_OK) {
        return "<unknown>";
    }
    return class_info->class_name;
}


/*************************************************************************
 *                        @fn fbe_cli_field_map_print_switches
 *************************************************************************
 *
 * @brief
 *    Print switches information for specified map
 *
 * @param :
 *    name:     provide switches name
 *    map:      The map array to print
 *    map_num:  Length of the array
 *
 * @return:
 *    None
 *
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
void fbe_cli_field_map_print_switches(const char *name,
                                      struct fbe_cli_field_map *map, fbe_u32_t map_num)
{
    fbe_u32_t i;

    fbe_cli_printf("Switches for %s:\n", name);
    for (i = 0; i < map_num; i++) {
        fbe_cli_printf("    -%s\n", map[i].name);
    }
    fbe_cli_printf("\n");
}


/*************************************************************************
 *                        @fn fbe_cli_field_get_object_id
 *************************************************************************
 *
 * @brief
 *    Parser and store object_id from command line.
 *    Expect input: " -object_id 0x118"
 *
 * @param :
 *    argc:     argument number
 *    argv:     array of arguments
 *    id:       For storing object id
 *
 * @return:
 *    >= 0:     argument number consumed
 *     < 0:     failed.
 * @author
 *    10/31/2013 - Created. Jamin Kang
 *************************************************************************/
int fbe_cli_field_get_object_id(int argc, char *argv[], fbe_object_id_t *id)
{
    const char *obj_cmd = "-object_id";
    fbe_u32_t v;

    if (argc < 2) {
        return -1;
    }
    if (strcmp(argv[0], obj_cmd) != 0) {
        return -1;
    }
    if (fbe_cli_strtoui32(argv[1], &v) != FBE_STATUS_OK) {
        fbe_cli_error("Invalid object id: %s\n", argv[1]);
        return -1;
    }
    *id = (fbe_object_id_t)v;
    return 2;
}

/*******************************************************************
 * end file fbe_cli_field_utils.c
 *******************************************************************/
