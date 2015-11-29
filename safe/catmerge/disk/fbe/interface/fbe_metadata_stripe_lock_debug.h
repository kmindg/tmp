#ifndef FBE_METADATA_STRIPE_LOCK_DEBUG_H
#define FBE_METADATA_STRIPE_LOCK_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_metadata_stripe_lock_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the stripe lock debug library.
 *
 * @author
 *  01/31/2011 - Created. Hari Singh
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"
#include "fbe_metadata.h"
//#include "fbe_metadata_stripe_lock.h"

fbe_status_t fbe_metadata_stripe_lock_slot_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr object_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent);

#endif /* FBE_METADATA_STRIPE_LOCK_DEBUG_H */

/*************************
 * end file fbe_metadata_stripe_lock_debug.h
 *************************/
