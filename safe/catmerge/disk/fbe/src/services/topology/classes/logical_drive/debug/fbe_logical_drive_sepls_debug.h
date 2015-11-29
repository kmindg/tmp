#ifndef FBE_LOGICAL_DRIVE_SEPLS_DEBUG_H
#define FBE_LOGICAL_DRIVE_SEPLS_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_logical_drive_sepls_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the logical drive debug library.
 *
 * @author
 *   22/07/2011:  Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

/*! @struct fbe_sepls_ldo_config_t
 *  
 * @brief Logical drive object information structure. This structure holds information related to LDO object.
 * which we need to display for the sepls debug macro.
 */
typedef struct fbe_sepls_ldo_config_s
{
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t state;
    fbe_lba_t capacity;
    fbe_object_id_t server_id;
    fbe_u32_t  capacity_of_ldo;
}fbe_sepls_ldo_config_t;

fbe_status_t fbe_sepls_display_ldo_info_debug_trace(fbe_trace_func_t trace_func ,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_sepls_ldo_config_t *ldo_data_ptr);


#endif /* FBE_LOGICAL_DRIVE_SEPLS_DEBUG_H */

/*************************
 * end file fbe_logical_drive_sepls_debug.h
 *************************/