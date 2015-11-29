/***************************************************************************
 *  fbe_terminator_miniport_sas_device_table.c
 ***************************************************************************
 *
 *  Description
 *      sas device table module, which is used to keep track of the cpd device id
 *
 *
 *  History:
 *      02/02/09    guov    Created
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_terminator_miniport_sas_device_table.h"
#include "fbe/fbe_winddk.h"
#include "fbe_terminator.h"
#include "fbe_terminator_common.h"

/**********************************/
/*        local variables         */
/**********************************/

/* the following are used to generate the cpd device id.
 * the cpd deivce id is 64 bit long
 * 0-15 bit are the index in the device table, 16-47 are the generation_id */
#define GENERATION_ID_OFFSET    0x10

fbe_status_t terminator_miniport_sas_device_table_init(fbe_u32_t record_count, terminator_miniport_sas_device_table_t *table)
{
    fbe_u32_t ind = 0;
    table->record_count = record_count;
    table->records = (terminator_miniport_sas_device_table_record_t *)fbe_terminator_allocate_memory(sizeof(terminator_miniport_sas_device_table_record_t)*record_count);
    if (table->records == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for miniport sas device table records at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (ind = 0; ind < record_count; ind ++) {
        table->records[ind].generation_id = 0;
        table->records[ind].record_state = RECORD_INVALID;
        table->records[ind].device_handle = NULL;
    }
    return FBE_STATUS_OK;
}

fbe_status_t terminator_miniport_sas_device_table_destroy(terminator_miniport_sas_device_table_t *table)
{
    table->record_count = 0;
    fbe_terminator_free_memory(table->records);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_miniport_sas_device_table_add_record(const terminator_miniport_sas_device_table_t *table,
                                                             const fbe_terminator_device_ptr_t device_handle,
                                                             fbe_u32_t *index)
{
    fbe_s32_t ind = 0;

    if (*index > MAX_DEVICES_PER_SAS_PORT) {
        /* Out of index range, return an error */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (*index == INVALID_TMSDT_INDEX){
        /* this is a new record, look for an entry that is marked invalid
         * start from the highest index.
         */
        for (ind = (MAX_DEVICES_PER_SAS_PORT - 1); ind >= 0; ind --) {
            if (table->records[ind].record_state == RECORD_INVALID)
            {
                *index = ind;
                table->records[ind].generation_id ++;
                table->records[ind].record_state = RECORD_VALID;
                table->records[ind].device_handle = device_handle;
                return FBE_STATUS_OK;
            }
        }
    }
    else{
        /* The record should be added to the user specified slot.
         * The slot should be reserved. If not reserved, log an error.
         */
        if (table->records[*index].record_state == RECORD_RESERVED) {
            table->records[*index].generation_id ++;
            table->records[*index].record_state = RECORD_VALID;
            table->records[*index].device_handle = device_handle;
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t terminator_miniport_sas_device_table_remove_record(const terminator_miniport_sas_device_table_t *table, const fbe_u32_t index)
{
    if (index >= MAX_DEVICES_PER_SAS_PORT) {
        /* Out of index range, return an error */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* This is a valid index, check the state now. If state is valid set it to invalid.
     * Otherwise, leave it alone. Don't want to change to INVALID if it is RESERVED.
     */
    if (table->records[index].record_state == RECORD_VALID) {
        table->records[index].record_state = RECORD_INVALID;
    }
    table->records[index].device_handle = NULL;
    return FBE_STATUS_OK;
}

fbe_status_t terminator_miniport_sas_device_table_reserve_record(const terminator_miniport_sas_device_table_t *table, const fbe_u32_t index)
{
    if (index >= MAX_DEVICES_PER_SAS_PORT) {
        /* Out of index range, return an error */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* This is a valid index, mark it RESERVED.
     */
    table->records[index].record_state = RECORD_RESERVED;
    return FBE_STATUS_OK;
}

fbe_status_t terminator_miniport_sas_device_table_get_device_id(const terminator_miniport_sas_device_table_t *table, const fbe_u32_t index, fbe_miniport_device_id_t *cpd_device_id)
{
    if (index >= MAX_DEVICES_PER_SAS_PORT) {
        /* Out of index range, return an error */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (table->records[index].record_state != RECORD_VALID) {
        /* The record is not valid, return an error*/
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *cpd_device_id = (index & INDEX_BIT_MASK) +
                     (((fbe_miniport_device_id_t)table->records[index].generation_id) << GENERATION_ID_OFFSET);

    return FBE_STATUS_OK;
}

fbe_status_t terminator_miniport_sas_device_table_get_device_handle(const terminator_miniport_sas_device_table_t *table,
                                                                    const fbe_u32_t index,
                                                                    fbe_terminator_device_ptr_t *device_handle)
{
    if (index >= MAX_DEVICES_PER_SAS_PORT) {
        /* Out of index range, return an error */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* This is a valid index, mark it RESERVED.
     */
    *device_handle = table->records[index].device_handle;
    return FBE_STATUS_OK;
}

