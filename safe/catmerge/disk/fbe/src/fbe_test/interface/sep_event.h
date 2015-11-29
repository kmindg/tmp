#ifndef SEP_EVENT_H
#define SEP_EVENT_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file    sep_event.h
 ***************************************************************************
 *
 * @brief   This file contains the function prototypes for the SEP test event.
 *          for both simulation and hardware tests.  New tests should be added
 *          here accordingly.
 *
 * @version
 *          Initial version
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe/fbe_types.h"

/******************************
 *  FUNCTION PROTOTYPES
 ******************************/
fbe_status_t fbe_test_sep_event_validate_event_generated(fbe_u32_t event_message_id,
                                                         fbe_bool_t *b_event_found_p,
                                                         fbe_bool_t b_reset_event_log_if_found,
                                                         fbe_u32_t  wait_tmo_ms);

#endif /* SEP_EVENT_H */

