#ifndef FBE_RAID_GROUP_SEPLS_DEBUG_H
#define FBE_RAID_GROUP_SEPLS_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_group_sepls_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the raid group debug library for
 *  the sepls debug macro.
 *
 * @author
 *  22/7/2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"
#include "fbe_database.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_debug.h"

/*! @struct fbe_sepls_rg_config_t
 *  
 * @brief RG object information structure. This structure holds information related to RG object.
 * which we need to display for the sepls debug macro.
 */
typedef struct fbe_sepls_rg_config_s
{
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t state;
    fbe_u32_t width;
    fbe_raid_group_type_t raid_type;
    fbe_lba_t rg_capacity;
    fbe_lba_t total_lun_capacity;
    fbe_object_id_t server_id;
	fbe_raid_group_database_info_t rg_database_info;
    fbe_raid_group_rebuild_nonpaged_info_t rebuild_info;
    fbe_raid_group_encryption_nonpaged_info_t encryption_info;
}fbe_sepls_rg_config_t;


fbe_status_t fbe_sepls_display_rg_info_debug_trace(fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_sepls_rg_config_t *rg_data_ptr);
fbe_status_t fbe_sepls_display_rg1_0_info_debug_trace(fbe_trace_func_t trace_func ,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_sepls_rg_config_t *rg_data_ptr);
fbe_status_t fbe_sepls_display_rg_type_debug_trace(fbe_trace_func_t trace_func ,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_sepls_rg_config_t *rg_data_ptr);
fbe_lba_t fbe_sepls_calculate_rebuild_capacity(fbe_sepls_rg_config_t *rg_data_ptr);
fbe_lba_t fbe_sepls_calculate_encryption_capacity(fbe_sepls_rg_config_t *rg_data_ptr);


#endif /* FBE_RAID_GROUP_SEPLS_DEBUG_H */

/*************************
 * end file fbe_raid_group_sepls_debug.h
 *************************/
