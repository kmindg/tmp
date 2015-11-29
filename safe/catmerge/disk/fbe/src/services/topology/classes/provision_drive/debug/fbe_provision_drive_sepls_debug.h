#ifndef FBE_PROVISION_DRIVE_SEPLS_DEBUG_H
#define FBE_PROVISION_DRIVE_SEPLS_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_provision_drive_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the provision drive debug library.
 *
 * @author
 *  22/07/2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

/*! @struct fbe_sepls_pvd_config_t
 *  
 * @brief PVD object information structure. This structure holds information related to PVD object.
 * which we need to display for the sepls debug macro.
 */
typedef struct fbe_sepls_pvd_config_s
{
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t state;
    fbe_lba_t capacity;
    fbe_object_id_t server_id;
    fbe_lba_t sniff_checkpoint;
    fbe_traffic_priority_t  verify_priority;
    fbe_lba_t zero_checkpoint;
    fbe_traffic_priority_t  zero_priority;
    fbe_u32_t  capacity_of_pvd;
	fbe_pvd_database_info_t pvd_database_info;
}fbe_sepls_pvd_config_t;

fbe_status_t fbe_sepls_check_pvd_display_info_debug_trace(fbe_trace_func_t trace_func ,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_sepls_pvd_config_t *pvd_data_ptr,
                                                          fbe_bool_t display_default,
                                                          fbe_u32_t display_format);
fbe_status_t fbe_sepls_display_pvd_info_debug_trace(fbe_trace_func_t trace_func ,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_sepls_pvd_config_t *pvd_data_ptr);
fbe_status_t fbe_sepls_get_pvd_checkpoints_info(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr pvd_base_config_ptr,
                                                fbe_sepls_pvd_config_t *pvd_data_ptr);

#endif /* FBE_PROVISION_DRIVE_SEPLS_DEBUG_H */

/*************************
 * end file fbe_provision_drive_sepls_debug.h
 *************************/
