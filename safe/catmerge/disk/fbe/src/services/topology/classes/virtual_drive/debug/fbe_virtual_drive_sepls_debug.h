#ifndef FBE_VIRTUL_DRIVE_SEPLS_DEBUG_H
#define FBE_VIRTUL_DRIVE_SEPLS_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_virtual_drive_sepls_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the virtual drive debug library.
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

/*! @struct fbe_sepls_vd_config_t
 *  
 * @brief VD object information structure. This structure holds information related to VD object.
 * which we need to display for the sepls debug macro.
 */
typedef struct fbe_sepls_vd_config_s
{
    fbe_object_id_t object_id;
	fbe_lifecycle_state_t state;
    fbe_object_id_t server_id[2];
	fbe_vd_database_info_t vd_database_info;
    fbe_virtual_drive_configuration_mode_t config_mode;
    fbe_raid_group_rebuild_nonpaged_info_t rebuild_info;
}fbe_sepls_vd_config_t;


fbe_status_t fbe_sepls_display_vd_info_debug_trace(fbe_trace_func_t trace_func ,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_block_size_t exported_block_size,
                                                   fbe_sepls_vd_config_t *vd_data_ptr);

#endif /* FBE_VIRTUL_DRIVE_SEPLS_DEBUG_H */

/*************************
 * end file fbe_virtual_drive_sepls_debug.h
 *************************/
