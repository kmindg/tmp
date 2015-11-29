/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-09
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file rginfo.c
 ***************************************************************************
 *
 * File Description:
 *
 *   Debugging extensions for sep driver.
 *
 * Author:
 *   Jibing
 *
 * Revision History:
 *
 * Jibing 15/07/13  Initial version.
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_sas_pmc_port_debug.h"
#include "fbe_fc_port_debug.h"
#include "fbe_iscsi_port_debug.h"
#include "fbe_base_physical_drive_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_lun_debug.h"
#include "fbe_encl_mgmt_debug.h"
#include "fbe_sps_mgmt_debug.h"
#include "fbe_drive_mgmt_debug.h"
#include "fbe_ps_mgmt_debug.h"
#include "fbe_modules_mgmt_debug.h"
#include "fbe_board_mgmt_debug.h"
#include "fbe_cooling_mgmt_debug.h"
#include "fbe_topology_debug.h"
#include "pp_ext.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_sepls_debug.h"


#pragma data_seg ("EXT_HELP$4rginfo")
static char CSX_MAYBE_UNUSED usageMsg_rginfo[] =
"!rginfo\n"
" List brief info of all raid groups\n"
"  !rginfo -t\n"
"   max_out_time(ms) : the longest outstanding time among all the outstanding IO\n"
"   avg_out_time(ms) : the average outstanding time of all the outstanding IO\n"
"   max_sl_time(ms)  : the longest time spent on waiting stripe lock\n"
"   avg_sl_time(ms)  : the average time spend on waiting stripe lock\n";
#pragma data_seg ()

static fbe_bool_t fbe_rginfo_rg_is_degraded(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr rg_base_config_ptr);
static fbe_bool_t fbe_rginfo_rg_is_rebuilding(const fbe_u8_t * module_name, 
                                               fbe_dbgext_ptr rg_base_config_ptr);
static fbe_bool_t fbe_rginfo_rg_is_verify(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr rg_base_config_ptr);
static fbe_bool_t fbe_rginfo_rg_downstream_is_copy(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr control_handle_ptr);
static fbe_bool_t fbe_rginfo_rg_downstream_is_zeroing(const fbe_u8_t * module_name, 
                                                       fbe_dbgext_ptr control_handle_ptr);
static fbe_bool_t fbe_rginfo_rg_downstream_is_sniff_verify(const fbe_u8_t * module_name, 
                                                            fbe_dbgext_ptr control_handle_ptr);
static fbe_bool_t fbe_rginfo_rg_downstream_is_slf(const fbe_u8_t * module_name, 
                                                   fbe_dbgext_ptr control_handle_ptr);
static fbe_status_t fbe_rginfo_rg_downstream_drive_type(const fbe_u8_t * module_name, 
                                                         fbe_dbgext_ptr control_handle_ptr,
                                                         fbe_u32_t *drive_type_p);
static fbe_status_t fbe_rginfo_drive_type_debug_trace(fbe_drive_type_t drive_type,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t spaces_to_indent);

static fbe_status_t fbe_rginfo_get_outstanding_io(const fbe_u8_t * module_name,
                                                   fbe_u64_t block_transport_p,
                                                   fbe_atomic_t *outstanding_io_count_p,
                                                   fbe_u32_t *outstanding_io_max_p);

static fbe_status_t fbe_rginfo_get_io_credits(const fbe_u8_t * module_name,
                                               fbe_u64_t block_transport_p,
                                               fbe_u32_t *outstanding_io_credits_p,
                                               fbe_u32_t *io_credits_max_p);

static fbe_u32_t fbe_rginfo_get_rg_parity_disks(fbe_raid_group_type_t rg_type, fbe_u32_t width);


void fbe_rginfo_header_info_trace(const fbe_u8_t * module_name, 
                                   fbe_trace_func_t trace_func, 
                                   fbe_trace_context_t trace_context);

void fbe_rginfo_rg_info_trace_by_id(const fbe_u8_t * module_name,
                                     fbe_object_id_t object_id,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context);

fbe_status_t fbe_rginfo_rg_info_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr control_handle_ptr,
                                       fbe_bool_t b_all,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context);

#define FBE_RAID_INVALID_DISK_POSITION ((fbe_u32_t) -1)
#define FBE_PVD_LOCAL_STATE_SLF 0x0000000000000800

extern fbe_debug_enum_trace_info_t fbe_raid_debug_raid_group_type_trace_info[];
extern void fbe_get_rg_time_statics(fbe_u32_t rg_obj_id, 
                             fbe_u32_t *max_outstanding_time, fbe_u32_t *min_outstanding_time, fbe_u32_t *avg_outstanding_time,
                             fbe_u32_t *max_sl_wait_time, fbe_u32_t *min_sl_wait_time, fbe_u32_t *avg_sl_wait_time);

CSX_DBG_EXT_DEFINE_CMD(rginfo, "rginfo")
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status;
    fbe_dbgext_ptr control_handle_ptr = 0;    
    fbe_class_id_t filter_class = FBE_CLASS_ID_INVALID;
    ULONG topology_object_table_entry_size;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_bool_t b_all = FBE_FALSE;
    fbe_dbgext_ptr addr;
    fbe_u32_t max_entries;
    fbe_u32_t i;

    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_u32_t object_status_offset;
    fbe_u32_t control_handle_offset;
    fbe_u32_t class_id_offset;

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    fbe_debug_trace_func(NULL, "\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    fbe_debug_trace_func(NULL, "\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));
    fbe_debug_trace_func(NULL, "\t max_supported_objects %d\n\n", max_entries);

    if(topology_object_table_ptr == 0){
        return;
    }

    if (strlen(args) && strncmp(args, "-t", 2) == 0)
    {
        object_status = 0;
        /*
           fbe_debug_trace_func(NULL, "%-8s %-18s %-3s  %-3s  %-15s  %-15s  "
           "%-12s  %-12s  %-12s  %-12s  %-12s  %-12s   "
           "%-2s   %-2s   %-2s   %-2s   %-2s   %s\n",
           "RG", "TYPE", "DEG", "SLF", "outstanding/max", "credits/max",
           "max_out_time", "min_out_time", "avg_out_time", "max_sl_time", "min_sl_time", "avg_sl_time",
           "RB", "VF", "CP", "ZO", "SV", "Drive Type");
           */

        fbe_rginfo_header_info_trace(pp_ext_module_name, fbe_debug_trace_func, NULL);

        for (i = 0; i < max_entries; i++)
        {
            if (FBE_CHECK_CONTROL_C())
            {
                return;
            }

            FBE_GET_FIELD_OFFSET(pp_ext_module_name,
                                 topology_object_table_entry_t, "object_status",
                                 &object_status_offset);

            /* Get object status */
            addr = topology_object_tbl_ptr + object_status_offset;
            FBE_READ_MEMORY(addr, &object_status, sizeof(fbe_topology_object_status_t));

            if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
            {
                /* Fetch the control handle, which is the pointer to the object */
                FBE_GET_FIELD_OFFSET(pp_ext_module_name,
                                     topology_object_table_entry_t, "control_handle",
                                     &control_handle_offset);

                addr = topology_object_tbl_ptr + control_handle_offset;
                FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

                /* Fetch class id */
                FBE_GET_FIELD_OFFSET(pp_ext_module_name,
                                     fbe_base_object_s, "class_id",
                                     &class_id_offset);

                addr = control_handle_ptr + class_id_offset;
                FBE_READ_MEMORY(addr, &filter_class, sizeof(fbe_class_id_t));

                if (filter_class > FBE_CLASS_ID_RAID_FIRST && filter_class < FBE_CLASS_ID_RAID_LAST) {
                    fbe_rginfo_rg_info_trace(pp_ext_module_name, 
                                              control_handle_ptr, 
                                              b_all, 
                                              fbe_debug_trace_func, 
                                              NULL); 
                }
            }
            /* move to next entry */
            topology_object_tbl_ptr += topology_object_table_entry_size;        
        }
    }

    return;
}


void fbe_rginfo_header_info_trace(const fbe_u8_t * module_name, 
                                   fbe_trace_func_t trace_func, 
                                   fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "\t %-8s %-18s %-3s  %-3s  %-15s  %-15s  "
               "%-15s  %-15s  %-15s  %-15s   "
               "%-2s   %-2s   %-2s   %-2s   %-2s   %s\n",
               "RG", "TYPE", "DEG", "SLF", "outstanding/max", "credits/max",
               "max_out_time_ms", "avg_out_time_ms", "max_sl_time_ms", "avg_sl_time_ms",
               "RB", "VF", "CP", "ZO", "SV", "Drive Type");
    return;
}

fbe_status_t fbe_rginfo_rg_info_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr control_handle_ptr,
                                       fbe_bool_t b_all,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context)
{   
    fbe_object_id_t object_id;
    fbe_dbgext_ptr geo_ptr = 0;
    fbe_raid_group_type_t raid_type;
    fbe_dbgext_ptr base_config_ptr = 0;    
    fbe_u32_t width;
    fbe_bool_t rg_is_degraded = FBE_FALSE;
    fbe_bool_t rg_is_slf = FBE_FALSE;
    fbe_u32_t offset;

    fbe_u32_t transport_server_offset;
    fbe_atomic_t outstanding_io_count = 0;
    fbe_u32_t outstanding_io_max = 0;
    fbe_u32_t outstanding_io_credits = 0;
    fbe_u32_t io_credits_max = 0;

    fbe_u32_t rg_max_outstanding_time = 0;
    fbe_u32_t rg_min_outstanding_time = 0;
    fbe_u32_t rg_avg_outstanding_time = 0;
    fbe_u32_t rg_max_sl_wait_time = 0;
    fbe_u32_t rg_min_sl_wait_time = 0;
    fbe_u32_t rg_avg_sl_wait_time = 0;

    fbe_bool_t rg_is_rebuild = FBE_FALSE;
    fbe_bool_t rg_is_verify = FBE_FALSE;
    fbe_bool_t rg_downstream_is_copy = FBE_FALSE;
    fbe_bool_t rg_downstream_is_zeroing = FBE_FALSE;
    fbe_bool_t rg_downstream_is_sniff_verify = FBE_FALSE;

    fbe_u32_t drive_type_num[FBE_DRIVE_TYPE_LAST] = {0};
    fbe_drive_type_t drive_type;
    fbe_u32_t parity_disks = 0;
    fbe_class_id_t class_id;
    fbe_char_t type_string[64] = {'\0'};

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id);


    /* Get the base_config ptr */
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
    base_config_ptr = control_handle_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_base_config_t,
                         "block_transport_server",
                         &transport_server_offset);

    fbe_rginfo_get_outstanding_io(module_name, 
                                   base_config_ptr + transport_server_offset, 
                                   &outstanding_io_count, 
                                   &outstanding_io_max);

    if (!b_all && outstanding_io_count == 0)
    {        
        return FBE_STATUS_OK;
    }

    fbe_rginfo_get_io_credits(module_name, 
                               base_config_ptr + transport_server_offset, 
                               &outstanding_io_credits, 
                               &io_credits_max);

    /* Get the Raid type info */
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"geo",&offset);
    geo_ptr = control_handle_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_geometry_t,"raid_type",&offset);
    FBE_READ_MEMORY(geo_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));

    FBE_GET_FIELD_DATA(module_name,
                       base_config_ptr,
                       fbe_base_config_t,
                       width, sizeof(fbe_u32_t),
                       &width);

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       class_id,
                       sizeof(fbe_class_id_t),
                       &class_id);

    if (class_id == FBE_CLASS_ID_PARITY)
    {
        parity_disks = fbe_rginfo_get_rg_parity_disks(raid_type, width);
        _snprintf(type_string, 63, "%.12s(%d+%d)", fbe_raid_debug_raid_group_type_trace_info[raid_type].string_p, width - parity_disks, parity_disks);
    }
    else
    {
        _snprintf(type_string, 63, "%.12s(%d)", fbe_raid_debug_raid_group_type_trace_info[raid_type].string_p, width);
    }

    rg_is_degraded = fbe_rginfo_rg_is_degraded(module_name, base_config_ptr);
    rg_is_rebuild = fbe_rginfo_rg_is_rebuilding(module_name, base_config_ptr);
    rg_is_verify = fbe_rginfo_rg_is_verify(module_name, base_config_ptr);
    rg_downstream_is_copy = fbe_rginfo_rg_downstream_is_copy(module_name, control_handle_ptr);
    rg_downstream_is_zeroing = fbe_rginfo_rg_downstream_is_zeroing(module_name, control_handle_ptr);
    rg_downstream_is_sniff_verify = fbe_rginfo_rg_downstream_is_sniff_verify(module_name, control_handle_ptr);
    rg_is_slf = fbe_rginfo_rg_downstream_is_slf(module_name, control_handle_ptr);
    fbe_rginfo_rg_downstream_drive_type(module_name, control_handle_ptr, drive_type_num);

    fbe_get_rg_time_statics(object_id, 
                            &rg_max_outstanding_time, &rg_min_outstanding_time, &rg_avg_outstanding_time,
                            &rg_max_sl_wait_time, &rg_min_sl_wait_time, & rg_avg_sl_wait_time);

    trace_func(trace_context, 
               "\t 0x%-6x %-18s %-3d  %-3d  %7llu/%-7u  %7u/%-7u  "  \
               "%-15u  %-15u  %-15u  %-15u   " \
               "%-2d   %-2d   %-2d   %-2d   %-2d  ", 
               object_id, type_string, rg_is_degraded, rg_is_slf, outstanding_io_count, outstanding_io_max, outstanding_io_credits, io_credits_max, 
               rg_max_outstanding_time, rg_avg_outstanding_time, rg_max_sl_wait_time, rg_avg_sl_wait_time,
               rg_is_rebuild, rg_is_verify, rg_downstream_is_copy, rg_downstream_is_zeroing, rg_downstream_is_sniff_verify);

    for (drive_type = FBE_DRIVE_TYPE_INVALID; drive_type < FBE_DRIVE_TYPE_LAST; drive_type++)
    {
        if (drive_type_num[drive_type] != 0)
        {
            fbe_rginfo_drive_type_debug_trace(drive_type, trace_func, NULL, 4);
        }
    }

    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}

static fbe_dbgext_ptr fbe_rginfo_get_handle_by_id(const fbe_u8_t * module_name,
                                                   fbe_object_id_t object_id)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size = 0;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_u32_t max_entries;

    fbe_dbgext_ptr addr;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_u32_t control_handle_offset;

    FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if (object_id > max_entries)
        return NULL;

    FBE_GET_TYPE_SIZE(module_name, fbe_base_config_t*, &ptr_size);
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);

    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    topology_object_tbl_ptr += object_id * topology_object_table_entry_size;

    FBE_GET_FIELD_OFFSET(module_name,
                         topology_object_table_entry_t, "control_handle",
                         &control_handle_offset);

    addr = topology_object_tbl_ptr + control_handle_offset;
    FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

    return control_handle_ptr;
}

void fbe_rginfo_rg_info_trace_by_id(const fbe_u8_t * module_name,
                                     fbe_object_id_t object_id,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context)
{
    fbe_u32_t class_id_offset;
    fbe_class_id_t filter_class = FBE_CLASS_ID_INVALID;    
    fbe_dbgext_ptr control_handle_ptr = fbe_rginfo_get_handle_by_id(module_name, object_id);
    fbe_dbgext_ptr addr;

    if (control_handle_ptr)
    {
        FBE_GET_FIELD_OFFSET(module_name,
                             fbe_base_object_s, "class_id",
                             &class_id_offset);

        addr = control_handle_ptr + class_id_offset;
        FBE_READ_MEMORY(addr, &filter_class, sizeof(fbe_class_id_t));

        if (filter_class > FBE_CLASS_ID_RAID_FIRST && filter_class < FBE_CLASS_ID_RAID_LAST) 
        {
            fbe_rginfo_rg_info_trace(module_name,
                                      control_handle_ptr,
                                      FBE_FALSE,
                                      trace_func,
                                      trace_context);
        }
    }
    return;
}

static fbe_status_t fbe_rginfo_drive_type_debug_trace(fbe_drive_type_t drive_type,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    trace_func(trace_context, " ");

    switch(drive_type)
    {
    case FBE_DRIVE_TYPE_INVALID:
        trace_func(trace_context, "%s", "INVALID");
        break;
    case FBE_DRIVE_TYPE_FIBRE:
        trace_func(trace_context, "%s", "FIBRE");
        break;
    case FBE_DRIVE_TYPE_SAS:
        trace_func(trace_context, "%s", "SAS");
        break;
    case FBE_DRIVE_TYPE_SATA:
        trace_func(trace_context, "%s", "SATA");
        break;
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
        trace_func(trace_context, "%s", "SAS_FLASH_HE");
        break;
    case FBE_DRIVE_TYPE_SATA_FLASH_HE:
        trace_func(trace_context, "%s", "SATA_FLASH_HE");
        break;
    case FBE_DRIVE_TYPE_SAS_NL:
        trace_func(trace_context, "%s", "SAS_NL");
        break;
    case FBE_DRIVE_TYPE_SATA_PADDLECARD:
        trace_func(trace_context, "%s", "SATA_PADDLECARD");
        break;
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
        trace_func(trace_context, "%s", "SAS_FLASH_ME");
        break;            
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
        trace_func(trace_context, "%s", "SAS_FLASH_LE");
        break;            
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        trace_func(trace_context, "%s", "SAS_FLASH_RI");
        break;            
    default:
        trace_func(trace_context, "%s", "UNKNOWN");
    }

    return status;
}

static fbe_u32_t fbe_rginfo_get_rg_parity_disks(fbe_raid_group_type_t rg_type, fbe_u32_t width)
{
    fbe_u32_t parity_disks = 0;

    switch (rg_type)
    {
    case FBE_RAID_GROUP_TYPE_RAID1:
    case FBE_RAID_GROUP_TYPE_SPARE:
    case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
    case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
    case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
        parity_disks = width - 1;
        break;

    case FBE_RAID_GROUP_TYPE_RAID10:
        parity_disks = width / 2;
        break;

    case FBE_RAID_GROUP_TYPE_RAID3:
    case FBE_RAID_GROUP_TYPE_RAID5:
        parity_disks = 1;
        break;

    case FBE_RAID_GROUP_TYPE_RAID0:
        parity_disks = 0;
        break;

    case FBE_RAID_GROUP_TYPE_RAID6:
        parity_disks = 2;
        break;

    default:
        break;
    }
    return parity_disks; 
}

static fbe_status_t fbe_rginfo_get_rg_rebuild_info(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr rg_base_config_ptr,
                                                    void *rb_info_p)
{
    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;
    fbe_dbgext_ptr rg_rebuild_info_ptr;
    fbe_dbgext_ptr rebuild_checkpoint_ptr;
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;
    fbe_u32_t loop_counter = 0;
    fbe_u32_t checkpoint_offset;
    fbe_u32_t position_offset;
    fbe_u32_t checkpoint_info_size;
    fbe_lba_t checkpoint;
    fbe_u32_t position;
    fbe_raid_group_rebuild_nonpaged_info_t *rebuild_info_p = (fbe_raid_group_rebuild_nonpaged_info_t*)rb_info_p;

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
    fbe_metadata_ptr = rg_base_config_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
    nonpaged_record_ptr = fbe_metadata_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
    FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_nonpaged_metadata_t, "rebuild_info", &offset);
    rg_rebuild_info_ptr = data_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_nonpaged_info_t, "rebuild_logging_bitmask", &offset);
    FBE_READ_MEMORY(rg_rebuild_info_ptr + offset, &rebuild_logging_bitmask, sizeof(fbe_raid_position_bitmask_t));

    rebuild_info_p->rebuild_logging_bitmask = rebuild_logging_bitmask;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_nonpaged_info_t, "rebuild_checkpoint_info", &offset);
    rebuild_checkpoint_ptr = rg_rebuild_info_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_checkpoint_info_t, "checkpoint", &checkpoint_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_checkpoint_info_t, "position", &position_offset);
    FBE_GET_TYPE_SIZE(module_name, fbe_raid_group_rebuild_checkpoint_info_t, &checkpoint_info_size);

    for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; ++loop_counter)
    {
        FBE_READ_MEMORY(rebuild_checkpoint_ptr + checkpoint_offset, &checkpoint, sizeof(fbe_lba_t));
        rebuild_info_p->rebuild_checkpoint_info[loop_counter].checkpoint = checkpoint;
        FBE_READ_MEMORY(rebuild_checkpoint_ptr + position_offset, &position, sizeof(fbe_u32_t));
        rebuild_info_p->rebuild_checkpoint_info[loop_counter].position = position;

        rebuild_checkpoint_ptr += checkpoint_info_size;
    }

    return FBE_STATUS_OK;
}


static fbe_bool_t fbe_rginfo_rg_is_rebuilding(const fbe_u8_t * module_name, 
                                               fbe_dbgext_ptr rg_base_config_ptr)
{
    fbe_u32_t  i;
    fbe_bool_t is_rebuilding = FBE_FALSE;    
    fbe_raid_group_rebuild_nonpaged_info_t rebuild_info;
    fbe_dbgext_ptr block_edge_p;
    fbe_object_id_t server_id;
    fbe_u32_t block_edge_ptr_offset;
    fbe_dbgext_ptr base_edge_ptr = 0;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t block_edge_size = 0;

    fbe_rginfo_get_rg_rebuild_info(module_name, rg_base_config_ptr, &rebuild_info);

    FBE_GET_TYPE_SIZE(module_name, fbe_base_config_t*, &ptr_size);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
    FBE_READ_MEMORY(rg_base_config_ptr + block_edge_ptr_offset, &block_edge_p, ptr_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);

    for (i = 0; i < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; i++)
    {
        if (rebuild_info.rebuild_checkpoint_info[i].position != FBE_RAID_INVALID_DISK_POSITION)
        {
            base_edge_ptr = block_edge_p + (block_edge_size * rebuild_info.rebuild_checkpoint_info[i].position);
            FBE_GET_FIELD_DATA(module_name, base_edge_ptr, fbe_base_edge_s, server_id, sizeof(fbe_object_id_t), &server_id);
            if (server_id != FBE_OBJECT_ID_INVALID && server_id != 0)
            {
                is_rebuilding = FBE_TRUE;
                break;
            }
        }
    }

    return is_rebuilding;
}

static fbe_bool_t fbe_rginfo_rg_is_verify(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr rg_base_config_ptr)
{
    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;

    fbe_lba_t error_verify_checkpoint;
    fbe_lba_t ro_verify_checkpoint;
    fbe_lba_t rw_verify_checkpoint;
    fbe_lba_t system_verify_checkpoint;
    fbe_lba_t journal_verify_checkpoint;
    fbe_lba_t incomplete_write_verify_checkpoint;

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
    fbe_metadata_ptr = rg_base_config_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
    nonpaged_record_ptr = fbe_metadata_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
    FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

    FBE_GET_FIELD_DATA(module_name,
                       data_ptr,
                       fbe_raid_group_nonpaged_metadata_t,
                       error_verify_checkpoint,
                       sizeof(fbe_lba_t),
                       &error_verify_checkpoint);

    FBE_GET_FIELD_DATA(module_name,
                       data_ptr,
                       fbe_raid_group_nonpaged_metadata_t,
                       ro_verify_checkpoint,
                       sizeof(fbe_lba_t),
                       &ro_verify_checkpoint);

    FBE_GET_FIELD_DATA(module_name,
                       data_ptr,
                       fbe_raid_group_nonpaged_metadata_t,
                       rw_verify_checkpoint,
                       sizeof(fbe_lba_t),
                       &rw_verify_checkpoint);

    FBE_GET_FIELD_DATA(module_name,
                       data_ptr,
                       fbe_raid_group_nonpaged_metadata_t,
                       system_verify_checkpoint,
                       sizeof(fbe_lba_t),
                       &system_verify_checkpoint);

    FBE_GET_FIELD_DATA(module_name,
                       data_ptr,
                       fbe_raid_group_nonpaged_metadata_t,
                       journal_verify_checkpoint,
                       sizeof(fbe_lba_t),
                       &journal_verify_checkpoint);

    FBE_GET_FIELD_DATA(module_name,
                       data_ptr,
                       fbe_raid_group_nonpaged_metadata_t,
                       incomplete_write_verify_checkpoint,
                       sizeof(fbe_lba_t),
                       &incomplete_write_verify_checkpoint);


    if (error_verify_checkpoint == FBE_LBA_INVALID &&
        ro_verify_checkpoint == FBE_LBA_INVALID &&
        rw_verify_checkpoint == FBE_LBA_INVALID &&
        system_verify_checkpoint == FBE_LBA_INVALID &&
        journal_verify_checkpoint == FBE_LBA_INVALID &&
        incomplete_write_verify_checkpoint == FBE_LBA_INVALID) 
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}

static fbe_status_t fbe_rginfo_rg_enumerate_downstream_objects(const fbe_u8_t * module_name,
                                                                fbe_object_id_t filter_object,
                                                                fbe_class_id_t filter_class,
                                                                fbe_object_id_t * server_ids,
                                                                fbe_u32_t * count_p)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_u32_t max_entries;
    fbe_u32_t ptr_size = 0;

    fbe_dbgext_ptr addr;
    fbe_dbgext_ptr control_handle_ptr = 0;    
    fbe_class_id_t class_id;
    fbe_object_id_t object_id;

    fbe_u32_t control_handle_offset;

    FBE_GET_TYPE_SIZE(module_name, fbe_base_config_t*, &ptr_size);
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);

    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    topology_object_tbl_ptr += filter_object * topology_object_table_entry_size;

    FBE_GET_EXPRESSION(module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    FBE_GET_FIELD_OFFSET(module_name,
                         topology_object_table_entry_t, "control_handle",
                         &control_handle_offset);

    addr = topology_object_tbl_ptr + control_handle_offset;
    FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

    if (filter_object != FBE_OBJECT_ID_INVALID && filter_object != 0)
    {
        if (filter_object >= max_entries)
        {
            return FBE_STATUS_NO_OBJECT;
        }
        else
        {

            FBE_GET_FIELD_DATA(module_name, 
                               control_handle_ptr,
                               fbe_base_object_s,
                               class_id,
                               sizeof(fbe_class_id_t),
                               &class_id);

            FBE_GET_FIELD_DATA(module_name, 
                               control_handle_ptr,
                               fbe_base_object_s,
                               object_id,
                               sizeof(fbe_object_id_t),
                               &object_id);

            if (class_id == filter_class)
            {
                server_ids[*count_p] = object_id;
                (*count_p)++;
                return FBE_STATUS_OK;
            }
            else
            {
                fbe_dbgext_ptr base_config_ptr = 0;
                fbe_u32_t edge_index = 0;
                fbe_u64_t block_edge_p = 0;
                fbe_u32_t block_edge_size = 0;
                fbe_u32_t block_edge_ptr_offset;
                fbe_u32_t offset = 0;
                fbe_dbgext_ptr base_edge_ptr = 0;
                fbe_object_id_t     server_id;
                fbe_u32_t width;
                fbe_package_id_t server_package_id;
                fbe_package_id_t client_package_id;

                FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
                base_config_ptr = control_handle_ptr + offset;

                FBE_GET_FIELD_DATA(module_name,
                                   base_config_ptr,
                                   fbe_base_config_t,
                                   width, sizeof(fbe_u32_t),
                                   &width);

                FBE_GET_TYPE_SIZE(module_name, fbe_base_config_t*, &ptr_size);
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
                FBE_READ_MEMORY(base_config_ptr + block_edge_ptr_offset, &block_edge_p, ptr_size);
                FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);

                for (edge_index = 0; edge_index < width; edge_index++)
                {
                    if (FBE_CHECK_CONTROL_C())
                    {
                        return FBE_STATUS_CANCELED;
                    }

                    base_edge_ptr = block_edge_p + (block_edge_size * edge_index);
                    FBE_GET_FIELD_DATA(module_name, base_edge_ptr, fbe_block_edge_s, server_package_id, sizeof(fbe_package_id_t), &server_package_id);
                    FBE_GET_FIELD_DATA(module_name, base_edge_ptr, fbe_block_edge_s, client_package_id, sizeof(fbe_package_id_t), &client_package_id);

                    if (server_package_id != client_package_id)
                        continue;

                    FBE_GET_FIELD_OFFSET(module_name, fbe_block_edge_t, "base_edge", &offset);
                    base_edge_ptr = block_edge_p + (block_edge_size * edge_index) + offset;
                    FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &offset);

                    FBE_READ_MEMORY(base_edge_ptr + offset, &server_id, sizeof(fbe_object_id_t));

                    if (server_id != FBE_OBJECT_ID_INVALID && server_id < max_entries)
                        fbe_rginfo_rg_enumerate_downstream_objects(module_name, server_id, filter_class, server_ids, count_p);
                }
            }
            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_NO_OBJECT;
}

static fbe_bool_t fbe_rginfo_vd_is_copy(const fbe_u8_t * module_name, fbe_object_id_t vd_object_id)
{
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_dbgext_ptr rg_base_config_ptr = 0;
    fbe_u32_t offset;
    fbe_class_id_t class_id;

    control_handle_ptr = fbe_rginfo_get_handle_by_id(module_name, vd_object_id);
    if (!control_handle_ptr)
        return FBE_FALSE;


    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       class_id,
                       sizeof(fbe_class_id_t),
                       &class_id);

    if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {    
        FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
        rg_base_config_ptr = control_handle_ptr + offset;

        return fbe_rginfo_rg_is_rebuilding(module_name, rg_base_config_ptr);
    }
    else
    {
        fbe_debug_trace_func(NULL, "%s %d not a virtual drive\n", __FUNCTION__, vd_object_id);        
    }
    return FBE_FALSE;
}

static fbe_bool_t fbe_rginfo_rg_downstream_is_copy(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr control_handle_ptr)
{
    fbe_u32_t i = 0;
    fbe_u32_t count = 0;
    fbe_object_id_t rg_all_downstream_vd_server_ids[256] = { FBE_OBJECT_ID_INVALID };
    fbe_object_id_t object_id;
    fbe_bool_t vd_is_copy = FBE_FALSE;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id);


    fbe_rginfo_rg_enumerate_downstream_objects(module_name, object_id, FBE_CLASS_ID_VIRTUAL_DRIVE, rg_all_downstream_vd_server_ids, &count);

    for (i = 0; i < count; i++)
    {

        vd_is_copy = fbe_rginfo_vd_is_copy(module_name, rg_all_downstream_vd_server_ids[i]);
        if (vd_is_copy)
            break;
    }

    return vd_is_copy;
}

static fbe_bool_t fbe_rginfo_pvd_is_zeroing(const fbe_u8_t * module_name, fbe_object_id_t pvd_object_id)
{

    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_dbgext_ptr pvd_base_config_ptr = 0;    
    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;    
    fbe_lba_t zero_checkpoint;
    fbe_class_id_t class_id;

    control_handle_ptr = fbe_rginfo_get_handle_by_id(module_name, pvd_object_id);
    if (!control_handle_ptr)
        return FBE_FALSE;

    /* Get the base_config ptr */
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
    pvd_base_config_ptr = control_handle_ptr + offset;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       class_id,
                       sizeof(fbe_class_id_t),
                       &class_id);

    if (class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {
        FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
        fbe_metadata_ptr = pvd_base_config_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
        nonpaged_record_ptr = fbe_metadata_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
        FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

        FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_nonpaged_metadata_t, "zero_checkpoint", &offset);
        FBE_READ_MEMORY(data_ptr + offset, &zero_checkpoint, sizeof(fbe_lba_t));

        if (zero_checkpoint != FBE_LBA_INVALID)
            return FBE_TRUE;
    }
    else
    {
        fbe_debug_trace_func(NULL, "%s %d not a provision drive\n", __FUNCTION__, pvd_object_id);
    }
    return FBE_FALSE;
}

static fbe_bool_t fbe_rginfo_rg_downstream_is_zeroing(const fbe_u8_t * module_name, 
                                                       fbe_dbgext_ptr control_handle_ptr)
{
    fbe_u32_t i = 0;    
    fbe_u32_t count = 0;
    fbe_object_id_t rg_all_downstream_pvd_server_ids[512] = { FBE_OBJECT_ID_INVALID };
    fbe_object_id_t object_id;
    fbe_bool_t pvd_is_zeroing = FBE_FALSE;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id);

    fbe_rginfo_rg_enumerate_downstream_objects(module_name, object_id, FBE_CLASS_ID_PROVISION_DRIVE, rg_all_downstream_pvd_server_ids, &count);

    for (i = 0; i < count; i++)
    {
        pvd_is_zeroing = fbe_rginfo_pvd_is_zeroing(module_name, rg_all_downstream_pvd_server_ids[i]);
        if (pvd_is_zeroing)
            break;
    }

    return pvd_is_zeroing;
}

static fbe_bool_t fbe_rginfo_pvd_is_sniff_verify(const fbe_u8_t * module_name, fbe_object_id_t pvd_object_id)
{

    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_dbgext_ptr pvd_base_config_ptr = 0;    
    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;    
    fbe_lba_t sniff_checkpoint;
    fbe_class_id_t class_id;

    control_handle_ptr = fbe_rginfo_get_handle_by_id(module_name, pvd_object_id);
    if (!control_handle_ptr)
        return FBE_FALSE;


    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       class_id,
                       sizeof(fbe_class_id_t),
                       &class_id);

    if (class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {

        /* Get the base_config ptr */
        FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
        pvd_base_config_ptr = control_handle_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
        fbe_metadata_ptr = pvd_base_config_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
        nonpaged_record_ptr = fbe_metadata_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
        FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

        FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_nonpaged_metadata_t, "sniff_verify_checkpoint", &offset);
        FBE_READ_MEMORY(data_ptr + offset, &sniff_checkpoint, sizeof(fbe_lba_t));

        if (sniff_checkpoint != FBE_LBA_INVALID)
            return FBE_TRUE;
    }
    else
    {
        fbe_debug_trace_func(NULL, "%s %d not a provision drive\n", __FUNCTION__, pvd_object_id);
    }

    return FBE_FALSE;

}

static fbe_bool_t fbe_rginfo_rg_downstream_is_sniff_verify(const fbe_u8_t * module_name, 
                                                            fbe_dbgext_ptr control_handle_ptr)
{
    fbe_u32_t i = 0;    
    fbe_u32_t count = 0;
    fbe_object_id_t rg_all_downstream_pvd_server_ids[512] = { FBE_OBJECT_ID_INVALID };
    fbe_object_id_t object_id;
    fbe_bool_t pvd_is_sniff_verify = FBE_FALSE;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id);

    fbe_rginfo_rg_enumerate_downstream_objects(module_name, object_id, FBE_CLASS_ID_PROVISION_DRIVE, rg_all_downstream_pvd_server_ids, &count);

    for (i = 0; i < count; i++)
    {
        pvd_is_sniff_verify = fbe_rginfo_pvd_is_sniff_verify(module_name, rg_all_downstream_pvd_server_ids[i]);
        if (pvd_is_sniff_verify)
            break;
    }

    return pvd_is_sniff_verify;
}

static fbe_bool_t fbe_rginfo_pvd_is_slf(const fbe_u8_t * module_name, fbe_object_id_t pvd_object_id)
{
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_class_id_t class_id;
    fbe_u32_t offset;
    fbe_u64_t pvd_local_state = 0;

    control_handle_ptr = fbe_rginfo_get_handle_by_id(module_name, pvd_object_id);
    if (!control_handle_ptr)
        return FBE_FALSE;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       class_id,
                       sizeof(fbe_class_id_t),
                       &class_id);

    if (class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {    
        FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_t, "local_state", &offset);
        FBE_READ_MEMORY(control_handle_ptr + offset, &pvd_local_state, sizeof(fbe_u64_t));

        if (pvd_local_state & FBE_PVD_LOCAL_STATE_SLF /* 0x0000000000000800 */)
            return FBE_TRUE;
    }
    else
    {
        fbe_debug_trace_func(NULL, "%s %d not a provision drive\n", __FUNCTION__, pvd_object_id);
    }

    return FBE_FALSE;
}

static fbe_bool_t fbe_rginfo_rg_downstream_is_slf(const fbe_u8_t * module_name, 
                                                   fbe_dbgext_ptr control_handle_ptr)
{
    fbe_u32_t i = 0;    
    fbe_u32_t count = 0;
    fbe_object_id_t rg_all_downstream_pvd_server_ids[512] = { FBE_OBJECT_ID_INVALID };
    fbe_object_id_t object_id;
    fbe_bool_t pvd_is_slf = FBE_FALSE;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id);

    fbe_rginfo_rg_enumerate_downstream_objects(module_name, object_id, FBE_CLASS_ID_PROVISION_DRIVE, rg_all_downstream_pvd_server_ids, &count);

    for (i = 0; i < count; i++)
    {
        pvd_is_slf = fbe_rginfo_pvd_is_slf(module_name, rg_all_downstream_pvd_server_ids[i]);
        if (pvd_is_slf)
            break;
    }

    return pvd_is_slf;
}

static fbe_drive_type_t fbe_rginfo_get_pvd_drive_type(const fbe_u8_t * module_name, fbe_object_id_t pvd_object_id)
{
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_dbgext_ptr pvd_base_config_ptr = 0;    

    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;    
    fbe_dbgext_ptr nonpaged_drive_info_ptr;
    fbe_class_id_t class_id;
    fbe_drive_type_t drive_type = FBE_DRIVE_TYPE_INVALID;

    control_handle_ptr = fbe_rginfo_get_handle_by_id(module_name, pvd_object_id);
    if (!control_handle_ptr)
        return FBE_FALSE;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       class_id,
                       sizeof(fbe_class_id_t),
                       &class_id);

    if (class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {

        /* Get the base_config ptr */
        FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
        pvd_base_config_ptr = control_handle_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
        fbe_metadata_ptr = pvd_base_config_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
        nonpaged_record_ptr = fbe_metadata_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
        FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

        FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_nonpaged_metadata_t, "nonpaged_drive_info", &offset);
        nonpaged_drive_info_ptr = data_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_nonpaged_metadata_drive_info_t, "drive_type", &offset);
        FBE_READ_MEMORY(nonpaged_drive_info_ptr + offset, &drive_type, sizeof(fbe_drive_type_t));

    }
    else
    {
        fbe_debug_trace_func(NULL, "%s %d not a provision drive\n", __FUNCTION__, pvd_object_id);
    }

    return drive_type;
}

static fbe_status_t fbe_rginfo_rg_downstream_drive_type(const fbe_u8_t * module_name, 
                                                         fbe_dbgext_ptr control_handle_ptr,
                                                         fbe_u32_t *drive_type_p)
{
    fbe_u32_t i = 0;    
    fbe_u32_t count = 0;
    fbe_object_id_t rg_all_downstream_pvd_server_ids[512] = { FBE_OBJECT_ID_INVALID };
    fbe_object_id_t object_id;
    fbe_drive_type_t drive_type;

    FBE_GET_FIELD_DATA(module_name, 
                       control_handle_ptr,
                       fbe_base_object_s,
                       object_id,
                       sizeof(fbe_object_id_t),
                       &object_id);

    fbe_rginfo_rg_enumerate_downstream_objects(module_name, object_id, FBE_CLASS_ID_PROVISION_DRIVE, rg_all_downstream_pvd_server_ids, &count);

    for (i = 0; i < count; i++)
    {
        drive_type =  fbe_rginfo_get_pvd_drive_type(module_name, rg_all_downstream_pvd_server_ids[i]);
        if (drive_type > FBE_DRIVE_TYPE_INVALID && drive_type < FBE_DRIVE_TYPE_LAST)
            drive_type_p[drive_type]++;
    }

    return FBE_STATUS_OK;
}


static fbe_bool_t fbe_rginfo_rg_is_degraded(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr rg_base_config_ptr)
{
    fbe_raid_group_rebuild_nonpaged_info_t rebuild_info;

    fbe_rginfo_get_rg_rebuild_info(module_name, rg_base_config_ptr, &rebuild_info);

    if (rebuild_info.rebuild_logging_bitmask != 0)
        return FBE_TRUE;

    if (fbe_rginfo_rg_is_rebuilding(module_name, rg_base_config_ptr))
        return FBE_TRUE;

    return FBE_FALSE;
}

static fbe_status_t fbe_rginfo_get_outstanding_io(const fbe_u8_t * module_name,
                                                   fbe_u64_t block_transport_p,
                                                   fbe_atomic_t *outstanding_io_count_p,
                                                   fbe_u32_t *outstanding_io_max_p)
{
    fbe_u64_t outstanding_io_count;
    fbe_u32_t outstanding_io_max;

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_s,
                       outstanding_io_count,
                       sizeof(fbe_atomic_t),
                       &outstanding_io_count);

    if (outstanding_io_count_p)
        *outstanding_io_count_p = outstanding_io_count;

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_s,
                       outstanding_io_max,
                       sizeof(fbe_u32_t),
                       &outstanding_io_max);

    if (outstanding_io_max_p)
        *outstanding_io_max_p = outstanding_io_max;

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_rginfo_get_io_credits(const fbe_u8_t * module_name,
                                               fbe_u64_t block_transport_p,
                                               fbe_u32_t *outstanding_io_credits_p,
                                               fbe_u32_t *io_credits_max_p)
{
    fbe_u32_t outstanding_io_credits;
    fbe_u32_t io_credits_max;

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_s,
                       outstanding_io_credits,
                       sizeof(fbe_u32_t),
                       &outstanding_io_credits);

    if (outstanding_io_credits_p)
        *outstanding_io_credits_p = outstanding_io_credits;

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_s,
                       io_credits_max,
                       sizeof(fbe_u32_t),
                       &io_credits_max);

    if (io_credits_max_p)
        *io_credits_max_p = io_credits_max;

    return FBE_STATUS_OK;
}
