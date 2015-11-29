#ifndef FBE_BOARD_MGMT_UTILS_H
#define FBE_BOARD_MGMT_UTILS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_board_mgmt_utils.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the Board Mgmt library.
 *
 * @author
 *  19-Jan-2015 PHE - Created.
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_pe_types.h"

fbe_char_t * fbe_board_mgmt_decode_dimm_state(fbe_dimm_state_t state);
fbe_char_t * fbe_board_mgmt_decode_dimm_subState(fbe_dimm_subState_t subState);

fbe_char_t * fbe_board_mgmt_decode_suitcase_state(fbe_suitcase_state_t state);
fbe_char_t * fbe_board_mgmt_decode_suitcase_subState(fbe_suitcase_subState_t subState);
fbe_char_t * fbe_board_mgmt_decode_ssd_state(fbe_ssd_state_t state);
fbe_char_t * fbe_board_mgmt_decode_ssd_subState(fbe_ssd_subState_t subState);

#endif  /* FBE_BOARD_MGMT_UTILS_H*/
