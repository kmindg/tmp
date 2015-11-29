#ifndef __FBE_CLI_PAGED_UTILS_H
#define __FBE_CLI_PAGED_UTILS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_paged_utils.h
 ***************************************************************************
 *
 * @brief
 *  This file contains paged metadata operation definitions.
 *
 * @version
 *  04/16/2013 - Created. Jamin Kang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <fbe/fbe_payload_metadata_operation.h>

enum {
    FBE_CLI_PAGED_OP_INVALID,
    FBE_CLI_PAGED_OP_SET_BITS,
    FBE_CLI_PAGED_OP_CLEAR_BITS,
    FBE_CLI_PAGED_OP_WRITE,
    FBE_CLI_PAGED_OP_CLEAR_CACHE,
};

typedef struct fbe_cli_paged_change_s {
    fbe_u32_t                           operation;
    fbe_u32_t                           metadata_size;
    fbe_chunk_index_t                   chunk_index;
    fbe_chunk_count_t                   chunk_count;
    /* For paged_write, if user dosen't provide all fields, we need to do
     * read-modify-write. Thus we use a bitmap to record which fields are touched.
     */
    fbe_u8_t                            bitmap[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];
    fbe_u8_t                            data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];
} fbe_cli_paged_change_t;

extern const char * fbe_cli_paged_get_operation_name(fbe_u32_t operation);
extern fbe_status_t fbe_cli_paged_switch(fbe_object_id_t obj, fbe_cli_paged_change_t *change);

#endif /* end __FBE_CLI_PAGED_UTILS__ */
