#include "terminator_board.h"
#include "fbe_terminator_common.h"

terminator_board_t * board_new(void)
{
    terminator_board_t * new_board = NULL;

    new_board = (terminator_board_t *)fbe_terminator_allocate_memory(sizeof(terminator_board_t));
    if (new_board == NULL)
    {
        /* Can't allocate the memory we need */
        return new_board;
    }
    base_component_init(&new_board->base);
    base_component_set_component_type(&new_board->base, TERMINATOR_COMPONENT_TYPE_BOARD);

    return new_board;
}

fbe_terminator_board_info_t * allocate_board_info()
{
    fbe_terminator_board_info_t *new_info = NULL;
    new_info = (fbe_terminator_board_info_t *)fbe_terminator_allocate_memory(sizeof(fbe_terminator_board_info_t));
    if (new_info == NULL)
    {
        return new_info;
    }
    new_info->board_type = FBE_BOARD_TYPE_INVALID;
    new_info->platform_type = SPID_UNKNOWN_HW_TYPE;
    fbe_queue_init(&new_info->specl_sfi_request_queue_head);
    return new_info;
}

fbe_terminator_board_info_t * board_info_new(fbe_terminator_board_info_t *board_info)
{
    fbe_terminator_board_info_t *new_info = allocate_board_info();
    if (new_info == NULL)
    {
        return new_info;
    }
    memcpy(new_info, board_info, sizeof(fbe_terminator_board_info_t));
    return new_info;
}

/* Alex Alanov: this function is not used anywhere, commenting it out */
/*
fbe_status_t board_set_info(terminator_board_t * self, fbe_terminator_board_info_t * new_info)
{
    fbe_terminator_board_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info = base_component_get_attributes(&self->base);
    memcpy(info, new_info, sizeof(fbe_terminator_board_info_t));
    return FBE_STATUS_OK;
}
*/

fbe_status_t set_board_type(terminator_board_t * self, fbe_board_type_t board_type)
{
    fbe_terminator_board_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info = (fbe_terminator_board_info_t *) base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->board_type = board_type;
    return FBE_STATUS_OK;
}

fbe_board_type_t get_board_type(terminator_board_t * self)
{
    fbe_terminator_board_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info = (fbe_terminator_board_info_t *) base_component_get_attributes(&self->base);
    return info->board_type;
}

fbe_status_t set_platform_type(terminator_board_t * self, SPID_HW_TYPE platform_type)
{
    fbe_terminator_board_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info = (fbe_terminator_board_info_t *) base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->platform_type = platform_type;
    return FBE_STATUS_OK;
}

fbe_terminator_board_info_t *get_board_info(terminator_board_t * self)
{
    fbe_terminator_board_info_t *info = NULL;
    if (self == NULL)
    {
        return info;
    }
    info = (fbe_terminator_board_info_t *) base_component_get_attributes(&self->base);
    return info;
}

fbe_status_t pop_specl_sfi_mask_data(terminator_board_t * self, PSPECL_SFI_MASK_DATA sfi_mask_data_ptr)
{
    fbe_terminator_board_info_t *info = NULL;
    terminator_specl_sfi_mask_data_t *terminator_pop_mask_data = NULL;

    if (self == NULL || sfi_mask_data_ptr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info = (fbe_terminator_board_info_t *) base_component_get_attributes(&self->base);

    if(fbe_queue_is_empty(&info->specl_sfi_request_queue_head))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_pop_mask_data = (terminator_specl_sfi_mask_data_t *)fbe_queue_pop(&info->specl_sfi_request_queue_head);
    fbe_copy_memory(sfi_mask_data_ptr, &terminator_pop_mask_data->mask_data, sizeof(SPECL_SFI_MASK_DATA));
    fbe_terminator_free_memory(terminator_pop_mask_data);

    return FBE_STATUS_OK;
}

fbe_status_t push_specl_sfi_mask_data(terminator_board_t * self, PSPECL_SFI_MASK_DATA sfi_mask_data_ptr)
{
    fbe_terminator_board_info_t *info = NULL;
    terminator_specl_sfi_mask_data_t *new_data = NULL;

    if (self == NULL || sfi_mask_data_ptr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info = (fbe_terminator_board_info_t *) base_component_get_attributes(&self->base);

    /* Allocate a new queue element for the mask data */
    new_data = fbe_terminator_allocate_memory(sizeof(terminator_specl_sfi_mask_data_t));
    if (new_data == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Copy and insert new element to the queue */
    fbe_copy_memory(&new_data->mask_data, sfi_mask_data_ptr, sizeof(SPECL_SFI_MASK_DATA));
    fbe_queue_push(&info->specl_sfi_request_queue_head, &new_data->queue_element);

    return FBE_STATUS_OK;
}

fbe_status_t get_specl_sfi_mask_data_queue_count(terminator_board_t * self, fbe_u32_t *cnt)
{
    fbe_terminator_board_info_t *info = NULL;

    if (self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info = (fbe_terminator_board_info_t *) base_component_get_attributes(&self->base);

    *cnt = fbe_queue_length(&info->specl_sfi_request_queue_head);

    return FBE_STATUS_OK;
}

fbe_status_t board_destroy(terminator_board_t *self)
{
    fbe_terminator_board_info_t      *attributes;
    fbe_queue_element_t              *queue_element;
    terminator_specl_sfi_mask_data_t *specl_sfi_mask_data;

    if (self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((attributes = (fbe_terminator_board_info_t *)base_component_get_attributes(&self->base)) != NULL)
    {
        queue_element = fbe_queue_pop(&attributes->specl_sfi_request_queue_head);

        while (queue_element != NULL)
        {
            specl_sfi_mask_data = (terminator_specl_sfi_mask_data_t *)queue_element;
            fbe_terminator_free_memory(specl_sfi_mask_data);

            queue_element = fbe_queue_pop(&attributes->specl_sfi_request_queue_head);
        }

        fbe_queue_destroy(&attributes->specl_sfi_request_queue_head);
    }

    /* We shouldn't delete the memory of self->base in base_component_destroy(). However, it is
     *  safe now since base is the only data member in struct terminator_board_t.
     *
     * Instead, we should delete the whole memory here since we allocate the whole memory in
     *  board_new().
     */
    base_component_destroy(&self->base, FBE_FALSE);
    fbe_terminator_free_memory(self);

    return FBE_STATUS_OK;
}

fbe_u32_t board_list_backend_port_number(terminator_board_t * self, fbe_terminator_device_ptr_t port_list[])
{
    fbe_u32_t count = 0;
    terminator_port_t * port = NULL;
    fbe_port_type_t port_type;

    if (self == NULL)
    {
        return 0;
    }
    port = (terminator_port_t *)base_component_get_first_child(&self->base);
    while (port != NULL)
    {
        port_type = port_get_type(port);
        switch(port_type) {
            case FBE_PORT_TYPE_SAS_PMC:
            case FBE_PORT_TYPE_FC_PMC:
            case FBE_PORT_TYPE_ISCSI:
            case FBE_PORT_TYPE_FCOE:
                port_list[count] = port;
                count++;
                break;
            default:
                break;
        }
        port = (terminator_port_t *)base_component_get_next_child(&self->base, &port->base);
    }
    return count;
}

terminator_port_t * board_get_port_by_number(terminator_board_t * self, fbe_u32_t port_number)
{
    terminator_port_t * matching_port = NULL;

    if (self == NULL)
    {
        return 0;
    }

    matching_port = (terminator_port_t *)base_component_get_first_child(&self->base);
    while (matching_port != NULL)
    {
        if (sas_port_get_backend_number(matching_port) == port_number)
        {
            return (matching_port);
        }
        matching_port = (terminator_port_t *)base_component_get_next_child(&self->base, &matching_port->base);
    }
    return NULL;
}

