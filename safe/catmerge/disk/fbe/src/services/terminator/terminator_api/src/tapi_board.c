#include "fbe_terminator.h"
#include "terminator_board.h"

static speclSFIEntryType specl_sfi_entry_func = NULL;

fbe_status_t fbe_terminator_api_get_specl_sfi_entry_func(speclSFIEntryType *function)
{
    if (function == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *function = specl_sfi_entry_func;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_api_set_specl_sfi_entry_func(speclSFIEntryType function)
{
    specl_sfi_entry_func = function;

    return FBE_STATUS_OK;
}

/* This function is used by board object to get the board type during discovery */
fbe_status_t fbe_terminator_api_get_board_type(fbe_board_type_t *board_type)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    if (board_type == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_board_type(board_type);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: board_type: 0x%X\n", __FUNCTION__, *board_type);

    return status;
}

fbe_status_t fbe_terminator_api_get_board_info(fbe_terminator_board_info_t *board_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    if (board_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_board_info(board_info);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: Get Board Info successfull.\n", __FUNCTION__);

    return status;
}

/* Process all queued SPECL SFI mask data if SFI entry function is set*/
fbe_status_t fbe_terminator_api_process_specl_sfi_mask_data_queue(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t cnt, i;
    SPECL_SFI_MASK_DATA specl_sfi_mask_data;

    /* if specl_sfi_entry_func is not set, cannot process */
    if(specl_sfi_entry_func == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get SPECL SFI mask data count */
    if (terminator_get_specl_sfi_mask_data_queue_count(&cnt) != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(i = 0; i < cnt; ++i)
    {
        /* pop one */
        if (terminator_pop_specl_sfi_mask_data(&specl_sfi_mask_data) != FBE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* process use SPECL SFI entry function */
        if(!specl_sfi_entry_func(&specl_sfi_mask_data))
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*
 * Send SPECL SFI mask data to Terminator to set component status. If SPECL SFI entry is set, call it directly;
 * otherwise store the data in the board_info queue.
 */
fbe_status_t fbe_terminator_api_send_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA sfi_mask_data)
{
    SPECL_STATUS specl_status = SPECL_SFI_ERROR, status = FBE_STATUS_GENERIC_FAILURE;

    /* if SPECL SFI entry is set, call it */
    if(specl_sfi_entry_func != NULL)
    {
        specl_status = specl_sfi_entry_func(sfi_mask_data);
        if(specl_status)
        {
            status = FBE_STATUS_OK;
        }
    }
    /* else add the mask data to board_info queue */
    else
    {
        status = terminator_push_specl_sfi_mask_data(sfi_mask_data);
    }

    return status;
}
