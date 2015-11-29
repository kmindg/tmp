#ifndef __FBE_CLI_FIELD_UTILS_H__
#define __FBE_CLI_FIELD_UTILS_H__

/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#include "fbe_cli_private.h"

/*!*************************************************************************
 * @file fbe_cli_field_utils.h
 ***************************************************************************
 *
 * @brief
 *  This file contains structure field operation declarition.
 *
 * @version
 *  10/31/2013 - Created. Jamin Kang
 *
 ***************************************************************************/

#define ARRAY_LEN(a)    (sizeof(a) / sizeof(a[0]))

#define MEMBER_OFF(type, mem)  (unsigned int)CSX_OFFSET_OF(type, mem)
#define MEMBER_SIZE(type, mem) ((unsigned int)sizeof(((type *)0)->mem))
#define MEMBER(type, cmd, mem, dump, set)       \
    { cmd, MEMBER_OFF(type, mem), MEMBER_SIZE(type, mem), dump, set }

#define MEMBER_DUMP_LEVEL  "    "

/* 64 bytes should be enough for all fields
 * max length of the fields is version_header (16 bytes)
 */
#define MAX_FIELD_LENGTH    (64)

struct fbe_cli_field_map {
    const char *name;
    fbe_u32_t offset;    /* Field offset in the structure */
    fbe_u32_t size;      /* Field size */

    /**
     * Function pointer for printing field
     * @map: the field_map structure
     * @data: the data to print
     */
    void (*dump) (struct fbe_cli_field_map *map, void *data);

    /**
     * function pointer for setting field.
     * @map: The field_map structure
     * @key: The name of parameter.
     *       It is now always same as map->name. But may change to support complex type.
     * @data: The data to store parsed result.
     * @mask: Mask of the data. '1' means this bit in @data takes effect.
     *        If we need update whole data, we can set mask to all '1's.
     *        @mask can be NULL, which means we update all data.
     *
     * Return:
     *     < 0:     error.
     *     >= 0     bytes set
     */
    int (*set)(struct fbe_cli_field_map *map, const char *key, void *data, void *mask);
};

struct fbe_cli_field_map_array {
    struct fbe_cli_field_map *map;
    fbe_u32_t len;
};


typedef struct fbe_cli_field_parse_contex {
    void    *data;                  /* Buffer to store parse result */
    void    *mask;                  /* Mask of buffer */
    fbe_u32_t len;                  /* Length of data/mask */
    fbe_u32_t update_offset;        /* Specific the start of update range */
    fbe_u32_t update_size;          /* Specific the size of update range */
} fbe_cli_field_parse_contex_t;


#define FBE_CLI_FIELD_MAP_ARRAY_TERMINATION      { NULL, 0 }

/* Check if the data can fit in this field */
fbe_bool_t fbe_cli_field_map_is_len_valid(struct fbe_cli_field_map *map, fbe_u8_t *data, fbe_u32_t len);
/* Find map according to @str */
struct fbe_cli_field_map * fbe_cli_field_map_find(const char *str, struct fbe_cli_field_map_array *fields);
/* Print all map */
void fbe_cli_field_map_dump(void *dump_data, fbe_u32_t len,
                            struct fbe_cli_field_map *map, fbe_u32_t map_num);
/* Return class name for a specific object */
const char * fbe_cli_field_map_get_class_name(fbe_object_id_t obj);
/* Print switches */
void fbe_cli_field_map_print_switches(const char *name,
                                      struct fbe_cli_field_map *map, fbe_u32_t map_num);
int fbe_cli_field_get_object_id(int argc, char *argv[], fbe_object_id_t *id);
int fbe_cli_field_map_parse(int argc, char *argv[],
                            struct fbe_cli_field_map_array *table,
                            fbe_cli_field_parse_contex_t *contex);
/******************************************************************
 * end file fbe_cli_field_utils.h
 ******************************************************************/

#endif /* end __FBE_CLI_FIELD_UTILS_H__ */
