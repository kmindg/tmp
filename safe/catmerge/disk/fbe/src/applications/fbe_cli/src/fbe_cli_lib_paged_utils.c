/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_paged_utils.c
 ***************************************************************************
 *
 * @brief
 *    This file contains cli functions for paged operation
 *
 * @ingroup fbe_cli
 *
 * @revision
 *    05/03/2013 - Created. Jamin Kang
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_cli_private.h"
#include "fbe_cli_lib_paged_utils.h"
#include "fbe/fbe_api_utils.h"

/*!**************************************************************
 * fbe_cli_paged_modify_bits
 ****************************************************************
 *
 * @brief   do paged metadata set/clear bits command
 *
 * @param change
 * @param bits          - Metadata bits to modify.
 * @param is_set        - True: set bits; False: clear bits
 *
 * @return FBE_STATUS_OK for success, else error.
 *
 * @author
 *  05/03/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_status_t fbe_cli_paged_modify_bits(fbe_object_id_t obj,
                                              fbe_cli_paged_change_t *change,
                                              void *bits, fbe_bool_t is_set)
{
    fbe_api_base_config_metadata_paged_change_bits_t paged_change_bits;
    fbe_u32_t data_size = change->metadata_size;
    fbe_status_t status;

    memset(&paged_change_bits, 0, sizeof(paged_change_bits));
    paged_change_bits.metadata_offset = change->chunk_index * data_size;
    paged_change_bits.metadata_record_data_size = data_size;
    paged_change_bits.metadata_repeat_count = change->chunk_count;
    paged_change_bits.metadata_repeat_offset = 0;
    memcpy(&paged_change_bits.metadata_record_data[0], bits, data_size);

    if (is_set)
        status = fbe_api_base_config_metadata_paged_set_bits(obj, &paged_change_bits);
    else
        status = fbe_api_base_config_metadata_paged_clear_bits(obj, &paged_change_bits);

    return status;
}
/**************************************
 * end fbe_cli_paged_modify_bits
 **************************************/

/*!**************************************************************
 * fbe_cli_paged_clear_cache()
 ****************************************************************
 *
 * @brief   Clear the paged metadata cache for the associated
 *          object.
 *
 * @param   obj - The object id to clear the cache for.
 *
 * @return  FBE_STATUS_OK for success, else error.
 *
 * @author
 *  10/29/2013  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_cli_paged_clear_cache(fbe_object_id_t obj)
{
    fbe_status_t status;

    status = fbe_api_base_config_metadata_paged_clear_cache(obj);
    if (status != FBE_STATUS_OK)
    {
        /*! @note Clear cache can fail due to too much I/O. 
         */
        fbe_cli_printf ("%s: Failed with status: %d. This may occur under heavy I/O.\n",
                        __FUNCTION__, status);
    }

    return status;
}
/**************************************
 * end fbe_cli_paged_clear_cache()
 **************************************/

static fbe_status_t fbe_cli_paged_set_bits(fbe_object_id_t obj,
                                           fbe_cli_paged_change_t *change)
{
    return fbe_cli_paged_modify_bits(obj, change, &change->data[0], FBE_TRUE);
}

static fbe_status_t fbe_cli_paged_clear_bits(fbe_object_id_t obj,
                                             fbe_cli_paged_change_t *change)
{
    return fbe_cli_paged_modify_bits(obj, change, &change->data[0], FBE_FALSE);
}

static fbe_status_t fbe_cli_paged_write(fbe_object_id_t obj,
                                        fbe_cli_paged_change_t *change)
{
    fbe_status_t status;

    /* clear fields touched */
    status = fbe_cli_paged_modify_bits(obj, change, &change->bitmap[0], FBE_FALSE);
    if (status != FBE_STATUS_OK)
        return status;
    /* THen set the write data */
    status = fbe_cli_paged_modify_bits(obj, change, &change->data[0], FBE_TRUE);
    return status;
}

/*!**************************************************************
 * fbe_cli_paged_switch
 ****************************************************************
 *
 * @brief   do paged metadata operation
 *
 * @param change
 *
 * @return None
 *
 * @author
 *  05/03/2013 - Created. Jamin Kang
 *
 ****************************************************************/
fbe_status_t fbe_cli_paged_switch(fbe_object_id_t obj, fbe_cli_paged_change_t *change)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    switch (change->operation) {
    case FBE_CLI_PAGED_OP_SET_BITS:
        status = fbe_cli_paged_set_bits(obj, change);
        break;
    case FBE_CLI_PAGED_OP_CLEAR_BITS:
        status = fbe_cli_paged_clear_bits(obj, change);
        break;
    case FBE_CLI_PAGED_OP_WRITE:
        status = fbe_cli_paged_write(obj, change);
        break;
    case FBE_CLI_PAGED_OP_CLEAR_CACHE:
        status = fbe_cli_paged_clear_cache(obj);
        break;
    default:
        fbe_cli_error("%s: invalid operation: %u\n", __FUNCTION__, change->operation);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    return status;
}
/**************************************
 * end fbe_cli_paged_switch
 **************************************/
/*!**************************************************************
 * fbe_cli_paged_get_operation_name
 ****************************************************************
 *
 * @brief   do paged metadata operation
 *
 * @param cmd - Ptr to current rg command.
 *
 * @return None
 *
 * @author
 *  05/03/2013 - Created. Jamin Kang
 *
 ****************************************************************/
const char * fbe_cli_paged_get_operation_name(fbe_u32_t operation)
{
    static const char *op_name[] = {
        "invalid page operation",
        "paged set bits",
        "paged clear bits",
        "paged write",
    };

    if (operation < sizeof(op_name) / sizeof(op_name[0]))
        return op_name[operation];
    return op_name[FBE_CLI_PAGED_OP_INVALID];
}
/*******************************************************************
 * end fbe_cli_paged_get_operation_name
 *******************************************************************/

/*******************************************************************
 * end file fbe_cli_lib_paged_utils.c
 *******************************************************************/
