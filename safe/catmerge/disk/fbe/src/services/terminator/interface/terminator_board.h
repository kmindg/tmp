/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#ifndef TERMINATOR_BOARD_H
#define TERMINATOR_BOARD_H

#include "fbe/fbe_winddk.h"
#include "terminator_base.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_board_types.h"
#include "terminator_port.h"
#include "specl_sfi_types.h"

typedef struct terminator_board_s{
    base_component_t base;
}terminator_board_t;

typedef struct terminator_specl_sfi_mask_data_s {
    fbe_queue_element_t queue_element;
    SPECL_SFI_MASK_DATA mask_data;
}terminator_specl_sfi_mask_data_t;

terminator_board_t * board_new(void);

/* allocates empty fbe_terminator_board_info_t */
fbe_terminator_board_info_t * allocate_board_info(void);

fbe_terminator_board_info_t * board_info_new(fbe_terminator_board_info_t *board_info);

fbe_status_t board_set_info(terminator_board_t * self, fbe_terminator_board_info_t * info);

fbe_status_t set_board_type(terminator_board_t * self, fbe_board_type_t board_type);
fbe_board_type_t get_board_type(terminator_board_t * self);
fbe_status_t set_platform_type(terminator_board_t * self, SPID_HW_TYPE platform_type);
fbe_terminator_board_info_t *get_board_info(terminator_board_t * self);
fbe_status_t pop_specl_sfi_mask_data(terminator_board_t * self, PSPECL_SFI_MASK_DATA sfi_mask_data_ptr);
fbe_status_t push_specl_sfi_mask_data(terminator_board_t * self, PSPECL_SFI_MASK_DATA sfi_mask_data_ptr);
fbe_status_t get_specl_sfi_mask_data_queue_count(terminator_board_t * self, fbe_u32_t *cnt);
fbe_status_t board_destroy(terminator_board_t * self);
fbe_u32_t board_list_backend_port_number(terminator_board_t * self, fbe_terminator_device_ptr_t port_list[]);
terminator_port_t * board_get_port_by_number(terminator_board_t * self, fbe_u32_t port_number);

#endif /* TERMINATOR_BOARD_H */
