/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-09
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file rgio.c
 ***************************************************************************
 *
 * File Description:
 *
 *   Debugging extensions for sep driver.
 *
 * Author:
 *   Geng
 *
 * Revision History:
 *
 * Geng 15/07/13  Initial version.
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


#define MAX_IRPS    (10240)
#define INVALID_TIME 0

#define  ADD_SL_PACKT_TO_LIST(sl_status) \
{ \
                dbg_queue_item_t * packet_item_p; \
                fbe_u64_t packet_p; \
                FBE_READ_MEMORY(stripe_lock_operation_p + cmi_stripe_lock_offset + metadata_cmi_stripe_lock_packet_offset, &packet_p, ptr_size); \
                if (FBE_CHECK_CONTROL_C()) \
                { \
                    return;\
                } \
                packet_item_p = malloc(sizeof(dbg_queue_item_t)); \
                packet_item_p->private_attr.packet_attr.outstanding_time = 0; \
                packet_item_p->private_attr.packet_attr.max_read_pdo_time_time = 0; \
                packet_item_p->private_attr.packet_attr.max_write_pdo_time_time = 0; \
                packet_item_p->private_attr.packet_attr.siots_p = 0; \
                packet_item_p->private_attr.packet_attr.siots_outstanding_time = 0; \
                if (!packet_item_p) \
                { \
                    trace_func(trace_context,"%s failed to malloc dbg_queue_item_t\n", __FUNCTION__); \
                    return; \
                } \
                packet_item_p->queue_item_in_dump = packet_p; \
                packet_item_p->private_attr.packet_attr.state = sl_status; \
                fbe_add_item_to_packet_list(packet_list_head, packet_item_p); \
} \

struct sep_irp_info {
    fbe_dbgext_ptr      irp;
    fbe_dbgext_ptr      packet;
    fbe_cpu_id_t        cpu_id;
    fbe_u32_t           coarse_time;
    fbe_u32_t           coarse_wait_time;
};

struct sep_siots_info {
    fbe_dbgext_ptr      siots;
    fbe_dbgext_ptr      max_read_fruts;
    fbe_u32_t           max_read_time;
    fbe_dbgext_ptr      max_write_fruts;
    fbe_u32_t           max_write_time;
};

typedef enum packet_state_e
{
    WAITING_SL,
    AT_RAID_LIBARARY
} packet_state_t;

typedef struct packet_attribues_s
{
    packet_state_t state;
    fbe_u32_t max_sl_wait_time;
    fbe_u32_t outstanding_time;
    fbe_u64_t siots_p;
    fbe_u32_t siots_outstanding_time;
    fbe_u64_t max_read_fruts_p;
    fbe_u32_t max_read_pdo_time_time;
    fbe_u64_t max_write_fruts_p;
    fbe_u32_t max_write_pdo_time_time;
} packet_attribues_t;

typedef union dbg_queue_item_private_attr_u
{
    packet_attribues_t packet_attr;
} dbg_queue_item_private_attr_t;


typedef struct dbg_queue_element_s
{
   void * next;
   void * prev;
} dbg_queue_element_t;

typedef struct dbg_queue_item_s 
{
    dbg_queue_element_t queue_element;
    fbe_dbgext_ptr queue_item_in_dump;
    dbg_queue_item_private_attr_t private_attr;
} dbg_queue_item_t;


static void add_sep_irp(const struct sep_irp_info *info);
static void add_irp(const fbe_u8_t *module_name, fbe_cpu_id_t cpu_id, fbe_dbgext_ptr io_ptr);
static void build_irp_per_core(const fbe_u8_t *module_name, fbe_cpu_id_t cpu_id, fbe_dbgext_ptr head_ptr);
static void build_irp_list(void);
static struct sep_irp_info *search_sep_irp(fbe_dbgext_ptr packet);
static fbe_u32_t fruts_get_service_time(fbe_dbgext_ptr fruts_p);
static fbe_u32_t fruts_queue_get_max_service_time(fbe_dbgext_ptr head,
                                                  fbe_dbgext_ptr *fruts, fbe_u32_t prev_max_time);
void fbe_siots_get_max_service_time(struct sep_siots_info *info, fbe_dbgext_ptr siots);

void fbe_raid_group_max_siots_time(const fbe_u8_t * module_name,
                                   fbe_dbgext_ptr packet_p,
                                   fbe_dbgext_ptr *siots_queue_p,
                                   fbe_time_t *max_siots_time);


void fbe_get_rg_outstanding_io_by_id(const fbe_u8_t * module_name,
                                     fbe_dbgext_ptr topology_object_tbl_ptr,
                                     dbg_queue_element_t *packet_list_head,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context,
                                     fbe_u32_t spaces_to_indent);

void fbe_print_rg_outstanding_io_by_id(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr topology_object_tbl_ptr,
                                       dbg_queue_element_t *packet_list_head,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_u32_t spaces_to_indent);

static void 
fbe_get_rg_outstanding_io(const fbe_u8_t * pp_ext_module_name,
                           fbe_object_id_t filter_object,
                           fbe_trace_func_t trace_func,
                           fbe_trace_context_t trace_context,
                           fbe_u32_t spaces_to_indent);


void fbe_get_packets_from_terminator_queue(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr raid_group_p, 
                                           dbg_queue_element_t * packet_list_head, 
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_u32_t spaces_to_indent);

void fbe_get_packets_from_sl_queue(const fbe_u8_t * module_name, 
                                   fbe_dbgext_ptr raid_group_p, 
                                   dbg_queue_element_t * packet_list_head, 
                                   fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context,
                                   fbe_u32_t spaces_to_indent);

void fbe_get_max_sl_waiting_time_from_tracker(const fbe_u8_t * module_name,
                                              dbg_queue_item_t *packet_item_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent);

void fbe_get_max_siots(const fbe_u8_t * module_name,
                       dbg_queue_item_t *packet_item_p,
                       fbe_trace_func_t trace_func,
                       fbe_trace_context_t trace_context,
                       fbe_u32_t spaces_to_indent);

void fbe_get_max_fruts(const fbe_u8_t * module_name,
                       dbg_queue_item_t *packet_item_p,
                       fbe_trace_func_t trace_func,
                       fbe_trace_context_t trace_context,
                       fbe_u32_t spaces_to_indent);

void fbe_get_outstanding_time(const fbe_u8_t * module_name,
                              dbg_queue_item_t *packet_item_p,
                              fbe_trace_func_t trace_func,
                              fbe_trace_context_t trace_context,
                              fbe_u32_t spaces_to_indent);

void fbe_add_item_to_packet_list(dbg_queue_element_t * packet_list_head, dbg_queue_item_t *packet_item_p);

void fbe_add_dbg_queue_element_to_list(dbg_queue_element_t * list_head,
                                       dbg_queue_element_t * dbg_queue_element_p);

void fbe_sort_packet_list(dbg_queue_element_t * packet_list_head);

fbe_u32_t fbe_get_list_item_count(dbg_queue_element_t * packet_list_head);

fbe_u32_t fbe_get_pdo_service_time(const fbe_u8_t * module_name, fbe_dbgext_ptr fbe_packet_p);

void fbe_init_dbg_queue_list(dbg_queue_element_t * list_head);
void fbe_destroy_dbg_queue_list(dbg_queue_element_t * list_head);
void fbe_get_rg_time_statics(fbe_u32_t rg_obj_id, 
                             fbe_u32_t *max_outstanding_time, fbe_u32_t *min_outstanding_time, fbe_u32_t *avg_outstanding_time,
                             fbe_u32_t *max_sl_wait_time, fbe_u32_t *min_sl_wait_time, fbe_u32_t *avg_sl_wait_time);


// exported by rginfo.c
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

// global variable
static int is_irp_list_init;
static int is_build_canceled;
static struct sep_irp_info irp_infos[MAX_IRPS];
static unsigned int irps_number;
static fbe_u32_t fbe_transport_coarse_time;
// -------------------

/* ***************************************************************************
 *
 * @brief
 *  Debug macro to display outstanding packets on one RG
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   15-Jul-2013:  Created. Geng
 *
 ***************************************************************************/
#pragma data_seg ("EXT_HELP$4rgio")
static char CSX_MAYBE_UNUSED usageMsg_rgio[] =
"!rgio\n"
"  To display outstanding packets on one RG\n"
"    !rgio -o rg_obj_id\n"
"    sl_waiting_time                        : the time spent on waiting the stripe lock\n"
"    siots_high_timecost/ms                 : the SIOTS pointer with the highest time cost in unit microseconds\n"
"    read_fruts_high_timecost/pdo_time(ms)  : the FRUTS pointer with the highest time cost among all the read fruts and the time consumed by PDO in unit microseconds\n"
"    write_fruts_high_timecost/pdo_time(ms) : the FRUTS pointer with the highest time cost among all the write fruts, and the time consumed by PDO in unit microseconds\n";
#pragma data_seg ()


CSX_DBG_EXT_DEFINE_CMD(rgio, "rgio")
{
    fbe_char_t *str = NULL;
    fbe_object_id_t filter_object = FBE_OBJECT_ID_INVALID;
    fbe_u32_t arg_number = 1; /* arg 0 is the cmd, arg 1 is the first arg, etc. */
    
    str = strtok((char*)args, " \t");
    while (str != NULL)
    {
        if (strncmp(str, "-o", 2) == 0)
        {
            str = strtok(NULL, " \t");
            filter_object = (fbe_object_id_t) GetArgument64(str, arg_number);
        }
        str = strtok(NULL, " \t");
    }

    fbe_get_rg_outstanding_io(pp_ext_module_name, 
                              filter_object, 
                              fbe_debug_trace_func,
                              NULL,
                              0);
    
    return;
}


void 
fbe_get_rg_outstanding_io(const fbe_u8_t * pp_ext_module_name,
                          fbe_object_id_t filter_object,
                          fbe_trace_func_t trace_func,
                          fbe_trace_context_t trace_context,
                          fbe_u32_t spaces_to_indent)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_status_t status = FBE_STATUS_OK;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context,"%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
#if 0
	fbe_trace_indent(trace_func, trace_context, 4 /*spaces_to_indent*/);
    trace_func(trace_context,"\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);
#endif

    /* Get the size of the topology entry.
     */
    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
#if 0
	fbe_trace_indent(trace_func, trace_context, 4 /*spaces_to_indent*/);
    trace_func(trace_context,"\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);
#endif

    if(topology_object_tbl_ptr == 0){
        return;
    }

    /* we don't have to loop through all objects if we know which object we wanted */
    if (filter_object != FBE_OBJECT_ID_INVALID)
    {
        dbg_queue_element_t packet_list_head;
        fbe_init_dbg_queue_list(&packet_list_head);
        
        if (filter_object >= max_entries)
        {
            trace_func(trace_context,"\t object id %d is greater than table size %d\n", filter_object, max_entries);
        }
        else
        {
            topology_object_tbl_ptr += filter_object * topology_object_table_entry_size;
            fbe_get_rg_outstanding_io_by_id(pp_ext_module_name, 
                                            topology_object_tbl_ptr,
                                            &packet_list_head,
                                            trace_func,
                                            trace_context,
                                            spaces_to_indent);

            fbe_print_rg_outstanding_io_by_id(pp_ext_module_name, 
                                              topology_object_tbl_ptr,
                                              &packet_list_head,
                                              trace_func,
                                              trace_context,
                                              spaces_to_indent);

        }

        // destry the list_head
        fbe_destroy_dbg_queue_list(&packet_list_head);
    }
    else
    {
        // todo
    }

    return;
}

void fbe_get_packets_from_terminator_queue(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr raid_group_p, 
                                           dbg_queue_element_t * packet_list_head, 
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_u32_t spaces_to_indent)
{
    fbe_u32_t terminator_queue_offset;
    fbe_u64_t terminator_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t packet_queue_element_offset = 0;
    fbe_u32_t ptr_size;


    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &packet_queue_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "terminator_queue_head", &terminator_queue_offset);

    terminator_queue_head_ptr = raid_group_p + terminator_queue_offset;

    FBE_READ_MEMORY(raid_group_p + terminator_queue_offset, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    while ((queue_element_p != terminator_queue_head_ptr) && (queue_element_p != NULL))
    {
        dbg_queue_item_t * packet_item_p;
        fbe_u64_t packet_p = queue_element_p - packet_queue_element_offset;

        if (FBE_CHECK_CONTROL_C())
        {
            return;
        }

        packet_item_p = malloc(sizeof(dbg_queue_item_t));

        if (!packet_item_p)
        {
            trace_func(trace_context,"%s failed to malloc dbg_queue_item_t\n", __FUNCTION__);
            return;
        }

        packet_item_p->private_attr.packet_attr.outstanding_time = 0;
        packet_item_p->private_attr.packet_attr.max_read_pdo_time_time = 0;
        packet_item_p->private_attr.packet_attr.max_write_pdo_time_time = 0;
        packet_item_p->private_attr.packet_attr.siots_p = 0;
        packet_item_p->private_attr.packet_attr.siots_outstanding_time = 0;

        packet_item_p->queue_item_in_dump = packet_p;
        packet_item_p->private_attr.packet_attr.state = AT_RAID_LIBARARY;

        fbe_add_item_to_packet_list(packet_list_head, packet_item_p);

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }

    return;
}

void fbe_get_rg_outstanding_packet_list_by_id(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr topology_object_tbl_ptr,
                                       dbg_queue_element_t * packet_list_head,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_u32_t spaces_to_indent)
{
    fbe_u32_t ptr_size;

    fbe_dbgext_ptr raid_group_p = 0;

    /* Fetch the control handle, which is the pointer to the object.  */

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_DATA(pp_ext_module_name,
                        topology_object_tbl_ptr,
                        topology_object_table_entry_s,
                        control_handle,
                        sizeof(fbe_dbgext_ptr),
                        &raid_group_p);

    // get the packets from terminator queue
    fbe_get_packets_from_terminator_queue(module_name, raid_group_p, packet_list_head, trace_func, NULL, 0);

    // get the packets from sl queue
    fbe_get_packets_from_sl_queue(module_name, raid_group_p, packet_list_head, trace_func, NULL, 0);

    return;
}

void fbe_get_packets_from_sl_queue(const fbe_u8_t * module_name, 
                                   fbe_dbgext_ptr raid_group_p, 
                                   dbg_queue_element_t * packet_list_head, 
                                   fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context,
                                   fbe_u32_t spaces_to_indent)
{
    // get the metadata element
    fbe_u32_t ptr_size;
    fbe_u32_t metadata_element_offset;
    fbe_u32_t stripe_lock_queue_head_offset = 0;
    fbe_u64_t stripe_lock_queue_head_ptr;
    fbe_u64_t stripe_lock_queue_head_p = 0;
    fbe_u64_t stripe_lock_queue_element_p;


    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &metadata_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "stripe_lock_queue_head", &stripe_lock_queue_head_offset);

    stripe_lock_queue_head_ptr = raid_group_p + metadata_element_offset + stripe_lock_queue_head_offset;

    FBE_READ_MEMORY(stripe_lock_queue_head_ptr, &stripe_lock_queue_head_p, ptr_size);

    stripe_lock_queue_element_p = stripe_lock_queue_head_p;

    while ((stripe_lock_queue_element_p != stripe_lock_queue_head_ptr) && (stripe_lock_queue_element_p != NULL))
    {
        fbe_u64_t stripe_lock_operation_p;
        fbe_u32_t sl_element_offset;
        fbe_u32_t cmi_stripe_lock_offset;
        fbe_u32_t wait_queue_offset;
        fbe_u64_t wait_queue_head_ptr;
        fbe_u64_t wait_queue_element_p;
        fbe_u64_t next_queue_element_p;
        fbe_u32_t sl_flags_offset;
        fbe_u32_t sl_flags_size;
        fbe_u32_t sl_flags;

        fbe_u32_t sl_status_offset;
        fbe_u32_t sl_status_size;
        fbe_u32_t sl_status;
        
        fbe_u32_t metadata_cmi_stripe_lock_packet_offset;


        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_stripe_lock_operation_t, "queue_element", &sl_element_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_stripe_lock_operation_t, "cmi_stripe_lock", &cmi_stripe_lock_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_stripe_lock_operation_t, "wait_queue", &wait_queue_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_stripe_lock_operation_t, "flags", &sl_flags_offset);
        FBE_GET_TYPE_SIZE(module_name, fbe_payload_stripe_lock_flags_t, &sl_flags_size);
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_stripe_lock_operation_t, "status", &sl_status_offset);
        FBE_GET_TYPE_SIZE(module_name, fbe_payload_stripe_lock_status_t, &sl_status_size);
        FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_cmi_stripe_lock_t, "packet", &metadata_cmi_stripe_lock_packet_offset);

        stripe_lock_operation_p = stripe_lock_queue_element_p - sl_element_offset;

        // check the sl flags and status 
        FBE_READ_MEMORY(stripe_lock_operation_p + sl_flags_offset, &sl_flags, sl_flags_size);
        FBE_READ_MEMORY(stripe_lock_operation_p + sl_status_offset, &sl_status, sl_status_size);
        sl_status = (sl_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING) ? WAITING_SL : AT_RAID_LIBARARY;
        //if (!(sl_flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && (sl_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING))
        if (!(sl_flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST))
        {
            ADD_SL_PACKT_TO_LIST(sl_status);
        }

        // loop over the wait_queue
        wait_queue_head_ptr = stripe_lock_operation_p + wait_queue_offset;
        FBE_READ_MEMORY(wait_queue_head_ptr, &wait_queue_element_p, ptr_size);
        while ((wait_queue_element_p != wait_queue_head_ptr) && (wait_queue_element_p != NULL))
        {
            stripe_lock_operation_p = wait_queue_element_p - sl_element_offset;
            // check the sl flags and status 
            FBE_READ_MEMORY(stripe_lock_operation_p + sl_flags_offset, &sl_flags, sl_flags_size);
            FBE_READ_MEMORY(stripe_lock_operation_p + sl_status_offset, &sl_status, sl_status_size);
            
            // if (!(sl_flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) && (sl_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_PENDING))
            if (!(sl_flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST))
            {
                ADD_SL_PACKT_TO_LIST(sl_status);
            }

            // get next queue element
            FBE_READ_MEMORY(wait_queue_element_p, &next_queue_element_p, ptr_size);
            wait_queue_element_p = next_queue_element_p;
        }

        // get next queue element
        FBE_READ_MEMORY(stripe_lock_queue_element_p, &next_queue_element_p, ptr_size);
        stripe_lock_queue_element_p = next_queue_element_p;
    }

    return;
}

void fbe_get_outstanding_time(const fbe_u8_t * module_name,
                              dbg_queue_item_t *packet_item_p,
                              fbe_trace_func_t trace_func,
                              fbe_trace_context_t trace_context,
                              fbe_u32_t spaces_to_indent)
{
    // check if the packet lies on the bvd_irp_list
    struct sep_irp_info *sep_irp_info_p;
    sep_irp_info_p = search_sep_irp(packet_item_p->queue_item_in_dump);

    if (sep_irp_info_p)
    {
        // update the outstanding time
        packet_item_p->private_attr.packet_attr.outstanding_time = fbe_transport_coarse_time - sep_irp_info_p->coarse_time;
        return;
    }
    else
    {
        // use current time minus the first uncompleted completion function timestamp
        // todo, ignore the wrapped case
        fbe_u32_t ptr_size;

        fbe_u32_t packet_tracker_current_index_offset;
        fbe_u32_t packet_tracker_current_index_size;
        fbe_u8_t packet_tracker_current_index;

        fbe_u32_t packet_tracker_ring_offset;
        fbe_dbgext_ptr packet_tracker_ring_p;
        fbe_u64_t fbe_packet_p;

        fbe_u32_t valid_tracker_cnt;
        fbe_u32_t loop_index;

        FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

        // get some offsets and size about tracker
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_current_index", &packet_tracker_current_index_offset);
        FBE_GET_TYPE_SIZE(module_name, fbe_u8_t, &packet_tracker_current_index_size);

        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_ring", &packet_tracker_ring_offset);

        fbe_packet_p = (fbe_u64_t)packet_item_p->queue_item_in_dump;

        FBE_READ_MEMORY(fbe_packet_p + packet_tracker_current_index_offset, &packet_tracker_current_index, packet_tracker_current_index_size);

        valid_tracker_cnt = packet_tracker_current_index;
        // loop over the tracker ring
        for (loop_index = 0; loop_index < valid_tracker_cnt; loop_index++)
        {

            fbe_u32_t tracker_entry_size;
            fbe_u32_t tracker_entry_coarse_time_offset;
            fbe_u32_t coarse_time;
            fbe_u32_t tracker_entry_action_offset;
            fbe_u8_t action;

            FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "coarse_time", &tracker_entry_coarse_time_offset);
            FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "action", &tracker_entry_action_offset);

            FBE_GET_TYPE_SIZE(module_name, fbe_packet_tracker_entry_t, &tracker_entry_size);

            packet_tracker_ring_p = fbe_packet_p + packet_tracker_ring_offset;
            FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_coarse_time_offset + (tracker_entry_size * loop_index)), &coarse_time, sizeof(fbe_u32_t));
            FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_action_offset + (tracker_entry_size * loop_index)), &action, sizeof(fbe_u8_t));

            if (action == 1) // todo, bad hardcode, change to macro
            {

                packet_item_p->private_attr.packet_attr.outstanding_time = fbe_transport_coarse_time - coarse_time;
                return;
            }
        }

        // this branch should not be met
        packet_item_p->private_attr.packet_attr.outstanding_time = 0;
    }
}


void fbe_get_max_sl_waiting_time_from_tracker(const fbe_u8_t * module_name,
                                              dbg_queue_item_t *packet_item_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent)
{

   fbe_u32_t ptr_size;

   fbe_u32_t packet_tracker_index_wrapped_offset;
   fbe_u32_t packet_tracker_index_wrapped_size;
   fbe_u8_t packet_tracker_index_wrapped;

   fbe_u32_t packet_tracker_current_index_offset;
   fbe_u32_t packet_tracker_current_index_size;
   fbe_u8_t packet_tracker_current_index;

   fbe_u32_t packet_tracker_ring_offset;
   fbe_dbgext_ptr packet_tracker_ring_p;
   fbe_u64_t fbe_packet_p;

   fbe_u32_t valid_tracker_cnt;
   fbe_u32_t loop_index;

   fbe_u32_t max_sl_wait_time = 0;

   fbe_dbgext_ptr metadata_paged_dispatch_acquire_stripe_lock_completion_p;
   fbe_dbgext_ptr fbe_raid_group_stripe_lock_completion_p;
   fbe_dbgext_ptr fbe_raid_group_verify_get_stripe_lock_completion_p;


   FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

   // get the 3 possible aquciring stripe lock function ptr
   FBE_GET_EXPRESSION(module_name, metadata_paged_dispatch_acquire_stripe_lock_completion, &metadata_paged_dispatch_acquire_stripe_lock_completion_p);
   FBE_GET_EXPRESSION(module_name, fbe_raid_group_stripe_lock_completion, &fbe_raid_group_stripe_lock_completion_p);
   FBE_GET_EXPRESSION(module_name, fbe_raid_group_verify_get_stripe_lock_completion, &fbe_raid_group_verify_get_stripe_lock_completion_p);

   // get some offsets and size about tracker
   FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_index_wrapped", &packet_tracker_index_wrapped_offset);
   FBE_GET_TYPE_SIZE(module_name, fbe_u8_t, &packet_tracker_index_wrapped_size);
                     
   FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_current_index", &packet_tracker_current_index_offset);
   FBE_GET_TYPE_SIZE(module_name, fbe_u8_t, &packet_tracker_current_index_size);

   FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_ring", &packet_tracker_ring_offset);

   fbe_packet_p = (fbe_u64_t)packet_item_p->queue_item_in_dump;

   FBE_READ_MEMORY(fbe_packet_p + packet_tracker_index_wrapped_offset, &packet_tracker_index_wrapped, packet_tracker_index_wrapped_size);
   FBE_READ_MEMORY(fbe_packet_p + packet_tracker_current_index_offset, &packet_tracker_current_index, packet_tracker_current_index_size);

   valid_tracker_cnt = packet_tracker_index_wrapped == 1 ? FBE_PACKET_TRACKER_DEPTH : packet_tracker_current_index;

   // loop over the tracker ring
   for (loop_index = 0; loop_index < valid_tracker_cnt; loop_index++)
   {
       fbe_u32_t tracker_entry_size;
       fbe_u32_t tracker_entry_callback_fn_offset;
       fbe_dbgext_ptr callback_fn;
       fbe_u32_t tracker_entry_coarse_time_offset;
       fbe_u32_t coarse_time;
       fbe_u32_t tracker_entry_action_offset;
       fbe_u8_t action;

       FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "callback_fn", &tracker_entry_callback_fn_offset);
       FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "coarse_time", &tracker_entry_coarse_time_offset);
       FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "action", &tracker_entry_action_offset);

       FBE_GET_TYPE_SIZE(module_name, fbe_packet_tracker_entry_t, &tracker_entry_size);

       packet_tracker_ring_p = fbe_packet_p + packet_tracker_ring_offset;
       FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_callback_fn_offset + (tracker_entry_size * loop_index)), &callback_fn, ptr_size);
       FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_coarse_time_offset + (tracker_entry_size * loop_index)), &coarse_time, sizeof(fbe_u32_t));
       FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_action_offset + (tracker_entry_size * loop_index)), &action, sizeof(fbe_u8_t));

       if (callback_fn == metadata_paged_dispatch_acquire_stripe_lock_completion_p 
           || callback_fn == fbe_raid_group_stripe_lock_completion_p
           || callback_fn == fbe_raid_group_verify_get_stripe_lock_completion_p)
       {

           // check whether it has been completed
           if (action == 7) // todo, bad hardcode, change to macro
           {
               if (max_sl_wait_time < coarse_time)
               {
                   max_sl_wait_time = coarse_time;
               }
           }
           else
           {
               if (max_sl_wait_time < (fbe_transport_coarse_time - coarse_time))
               {
                   max_sl_wait_time = fbe_transport_coarse_time - coarse_time;
               }
           }
       }
   }

   // update max_sl_wait_time
   packet_item_p->private_attr.packet_attr.max_sl_wait_time = max_sl_wait_time;
}

static void add_sep_irp(const struct sep_irp_info *info)
{
    if (irps_number >= MAX_IRPS) {
        fbe_debug_trace_func(NULL, "IRP number >= %u, skip\n", MAX_IRPS);
        return;
    }
    irp_infos[irps_number] = *info;
    irps_number += 1;
}

static void add_irp(const fbe_u8_t *module_name, fbe_cpu_id_t cpu_id, fbe_dbgext_ptr io_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dbgext_ptr packet_ptr = 0;
    fbe_u32_t coarse_time_offset;
    fbe_u32_t packet_offset;
    fbe_u32_t irp_coarse_time;
    fbe_u32_t irp_coarse_wait_time;
    fbe_u32_t current_coarse_time;
    fbe_time_t current_time;
    fbe_u32_t ptr_size;
    struct sep_irp_info info;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) {
        fbe_debug_trace_func(NULL, "unable to get ptr size status: %d\n", status);
        return;
    }

    fbe_debug_get_idle_thread_time(module_name, &current_time);
    current_coarse_time = (fbe_u32_t)current_time;
    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "coarse_time", &coarse_time_offset);
    FBE_READ_MEMORY(io_ptr + coarse_time_offset , &irp_coarse_time, sizeof(fbe_u32_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "coarse_wait_time", &coarse_time_offset);
    FBE_READ_MEMORY(io_ptr + coarse_time_offset , &irp_coarse_wait_time, sizeof(fbe_u32_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "packet", &packet_offset);
    FBE_READ_MEMORY(io_ptr + packet_offset, &packet_ptr, ptr_size);
    info.irp = io_ptr;
    info.packet = packet_ptr;
    info.cpu_id = cpu_id;
    info.coarse_time = irp_coarse_time;
    info.coarse_wait_time = irp_coarse_wait_time;
    add_sep_irp(&info);
}

static void build_irp_per_core(const fbe_u8_t *module_name, fbe_cpu_id_t cpu_id, fbe_dbgext_ptr head_ptr)
{
    fbe_status_t status;
    fbe_u32_t ptr_size;
    fbe_u32_t next_offset, prev_offset;
    fbe_dbgext_ptr next_io = 0;
    fbe_dbgext_ptr prev_io = 0;
    fbe_dbgext_ptr next_io_ptr = 0;
    fbe_u32_t retry_count = 0;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return;
    }
        /* Get the HEAD's next and prev entries*/
    FBE_GET_FIELD_OFFSET(module_name, fbe_queue_head_t, "next", &next_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_queue_head_t, "prev", &prev_offset);

    FBE_READ_MEMORY(head_ptr + next_offset, &next_io, ptr_size);
    FBE_READ_MEMORY(head_ptr + prev_offset, &prev_io, ptr_size);

    if (next_io == head_ptr && prev_io == head_ptr) {
        return;
    }

    next_io_ptr = next_io;
    while (next_io_ptr != head_ptr) {
        if (FBE_CHECK_CONTROL_C()) {
            is_build_canceled = 1;
            return;
        }
        /* If retry count is greater than 10 I/Os, then just break out. */
        if(retry_count > 10)
            break;

        if (next_io_ptr == NULL) {
            retry_count++;
            fbe_debug_trace_func(NULL, "\tNext IO is NULL; Retry count: %u\n", retry_count);
            continue;
        }
        add_irp(module_name, cpu_id, next_io_ptr);
        FBE_READ_MEMORY(next_io + next_offset , &next_io_ptr, ptr_size);
        next_io = next_io_ptr;
    }
}

/*!**************************************************************
 * build_irp_list
 ****************************************************************
 * @brief
 *  build iprs <---> packets mapping for all irps pending in SEP
 *
 * @param None
 *
 * @return None
 *
 * @author
 *  2013.07.16 - Created. Jamin Kang
 *
 ****************************************************************/
static void build_irp_list(void)
{
    const fbe_u8_t *module_name = "sep";
    fbe_status_t status;
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr io_in_progress_ptr = 0;
    fbe_dbgext_ptr io_queue_entry_ptr = 0;
    fbe_dbgext_ptr cpu_count_ptr = 0;
    fbe_cpu_id_t cpu_count;
    fbe_cpu_id_t cpu_id = 0;
    fbe_u32_t fbe_multicore_queue_entry_size;

    if (is_irp_list_init)
        return;

    is_build_canceled = 0;
    irps_number = 0;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ;
    }

    FBE_GET_EXPRESSION(module_name, fbe_sep_shim_io_in_progress_queue, &io_in_progress_ptr);
    FBE_READ_MEMORY(io_in_progress_ptr, &io_queue_entry_ptr, ptr_size);
    if(io_in_progress_ptr == 0) {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "fbe_sep_shim_io_in_progress_queue is not available \n");
        return;
    }
    FBE_READ_MEMORY(io_in_progress_ptr, &io_queue_entry_ptr, ptr_size);
    if (io_queue_entry_ptr == 0) {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "IO queue is not available \n");
        return;
    }

    /* Get the CPU count */
    FBE_GET_EXPRESSION(module_name, fbe_sep_shim_cpu_count, &cpu_count_ptr);
    FBE_READ_MEMORY(cpu_count_ptr, &cpu_count, sizeof(fbe_cpu_id_t));
    cpu_count = cpu_count & 0x3f;

    FBE_GET_TYPE_SIZE(module_name, fbe_multicore_queue_entry_t, &fbe_multicore_queue_entry_size);
    for(cpu_id = 0; cpu_id < cpu_count; cpu_id++) {
        fbe_dbgext_ptr io_queue_entry_per_core_ptr;
        if (FBE_CHECK_CONTROL_C()) {
            is_build_canceled = 1;
            break;
        }
        io_queue_entry_per_core_ptr = io_queue_entry_ptr + (fbe_multicore_queue_entry_size * cpu_id);
        build_irp_per_core(module_name, cpu_id, io_queue_entry_per_core_ptr);
    }

    if (!is_build_canceled)
        is_irp_list_init = 1;
}

/*!**************************************************************
 * search_irp
 ****************************************************************
 * @brief
 *  find a sep irp via packet pointer
 *
 * @param packet: packet pointer
 *
 * @return sep_irp_info pointer for the irp, or NULL if not found.
 *
 * @author
 *  2013.07.16 - Created. Jamin Kang
 *
 ****************************************************************/
struct sep_irp_info *search_sep_irp(fbe_dbgext_ptr packet)
{
    unsigned int i;

    build_irp_list();
    for (i = 0; i < irps_number; i++) {
        if (irp_infos[i].packet == packet)
            return &irp_infos[i];
    }
    return NULL;
}

fbe_u32_t fbe_get_list_item_count(dbg_queue_element_t * packet_list_head)
{
    fbe_u32_t cnt = 0;
    dbg_queue_element_t *dbg_queue_element_p = packet_list_head->next;

    while (dbg_queue_element_p != packet_list_head && dbg_queue_element_p != NULL)
    {
        cnt++;
        dbg_queue_element_p = dbg_queue_element_p->next;
    }

    return cnt;
}

void fbe_add_item_to_packet_list(dbg_queue_element_t * packet_list_head, dbg_queue_item_t *packet_item_p)
{
    dbg_queue_element_t * dbg_queue_element_p;
    dbg_queue_item_t * current_packet_item_p;

    dbg_queue_element_p = (dbg_queue_element_t *)packet_list_head->next;

    while (dbg_queue_element_p != packet_list_head && dbg_queue_element_p != NULL)
    {
        current_packet_item_p = (dbg_queue_item_t*)dbg_queue_element_p;
        if (current_packet_item_p->queue_item_in_dump == packet_item_p->queue_item_in_dump)
        {
            // free the memory 
            free(packet_item_p);
            return;
        }

        dbg_queue_element_p = (dbg_queue_element_t*)dbg_queue_element_p->next;
    }

    // not a dup one, add to the list
    fbe_add_dbg_queue_element_to_list(packet_list_head, &packet_item_p->queue_element);
}
        

void fbe_add_dbg_queue_element_to_list(dbg_queue_element_t * list_head,
                                       dbg_queue_element_t * dbg_queue_element_p)
{
    dbg_queue_element_t * temp_p;
    temp_p = list_head->prev;
    list_head->prev = dbg_queue_element_p;
    dbg_queue_element_p->prev = temp_p;
    temp_p->next = dbg_queue_element_p;
    dbg_queue_element_p->next = list_head;

    return;
}

void fbe_sort_packet_list(dbg_queue_element_t * packet_list_head)
{
    // bubble up, sort down
    dbg_queue_item_t *cmp_packet_item_p;
    dbg_queue_item_t *current_packet_item_p;
    dbg_queue_element_t *cmp_dbg_queue_element_p;
    dbg_queue_element_t *current_dbg_queue_element_p = packet_list_head->next;
    dbg_queue_element_t *next_dbg_queue_element_p = current_dbg_queue_element_p->next;

    while (current_dbg_queue_element_p != packet_list_head)
    {
        current_packet_item_p = (dbg_queue_item_t*)current_dbg_queue_element_p;
        // start fromt the first
        cmp_dbg_queue_element_p = packet_list_head->next;
        while (cmp_dbg_queue_element_p != current_dbg_queue_element_p)
        {
            cmp_packet_item_p = (dbg_queue_item_t*)cmp_dbg_queue_element_p;
            if (current_packet_item_p->private_attr.packet_attr.outstanding_time > cmp_packet_item_p->private_attr.packet_attr.outstanding_time)
            {

                // remove current_dbg_queue_element_p
                ((dbg_queue_element_t*)current_dbg_queue_element_p->prev)->next = current_dbg_queue_element_p->next;
                ((dbg_queue_element_t*)current_dbg_queue_element_p->next)->prev = current_dbg_queue_element_p->prev;

                // insert current_dbg_queue_element_p to the head of cmp_dbg_queue_element_p
                ((dbg_queue_element_t*)cmp_dbg_queue_element_p->prev)->next = current_dbg_queue_element_p;
                current_dbg_queue_element_p->prev = cmp_dbg_queue_element_p->prev;

                current_dbg_queue_element_p->next = cmp_dbg_queue_element_p;
                cmp_dbg_queue_element_p->prev = current_dbg_queue_element_p;

                break;
            }

            cmp_dbg_queue_element_p = cmp_dbg_queue_element_p->next;
        }

        current_dbg_queue_element_p = next_dbg_queue_element_p;
        next_dbg_queue_element_p = next_dbg_queue_element_p->next;
    }
}

fbe_u32_t fbe_get_pdo_service_time(const fbe_u8_t * module_name, fbe_dbgext_ptr fbe_packet_p)
{
    fbe_u32_t fbe_packet_current_level_offset;
    fbe_u32_t fbe_packet_physical_drive_service_time_ms_offset;
    fbe_u32_t current_level;
    fbe_u32_t physical_drive_service_time_ms;

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "current_level", &fbe_packet_current_level_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "physical_drive_service_time_ms", &fbe_packet_physical_drive_service_time_ms_offset);

    FBE_READ_MEMORY(fbe_packet_p + fbe_packet_current_level_offset, &current_level, sizeof(fbe_u32_t));
    
    // if the packet has been finished, get it from the physical_drive_service_time_ms directly
    if (current_level == 0xFFFFFFFF) // todo, remove hardcode
    {
        FBE_READ_MEMORY(fbe_packet_p + fbe_packet_physical_drive_service_time_ms_offset, &physical_drive_service_time_ms, sizeof(fbe_u32_t));
        return physical_drive_service_time_ms;
    }
    else
    {
        // otherwise using current time minus the timestamp in tracker ring
        // todo, ignore the wrapped case
        fbe_u32_t ptr_size;

        fbe_u32_t packet_tracker_current_index_offset;
        fbe_u32_t packet_tracker_current_index_size;
        fbe_u8_t packet_tracker_current_index;

        fbe_u32_t packet_tracker_ring_offset;
        fbe_dbgext_ptr packet_tracker_ring_p;

        fbe_u32_t valid_tracker_cnt;
        fbe_u32_t loop_index;

        FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

        // get some offsets and size about tracker
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_current_index", &packet_tracker_current_index_offset);
        FBE_GET_TYPE_SIZE(module_name, fbe_u8_t, &packet_tracker_current_index_size);

        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_ring", &packet_tracker_ring_offset);

        FBE_READ_MEMORY(fbe_packet_p + packet_tracker_current_index_offset, &packet_tracker_current_index, packet_tracker_current_index_size);

        valid_tracker_cnt = packet_tracker_current_index;
        // loop over the tracker ring
        for (loop_index = 0; loop_index < valid_tracker_cnt; loop_index++)
        {
            fbe_u32_t tracker_entry_callback_fn_offset;
            fbe_dbgext_ptr callback_fn;
            fbe_u32_t tracker_entry_size;
            fbe_u32_t tracker_entry_coarse_time_offset;
            fbe_u32_t coarse_time;
            fbe_u32_t tracker_entry_action_offset;
            fbe_u8_t action;

            fbe_dbgext_ptr physical_package_block_transport_server_bouncer_completion_p;

            FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "callback_fn", &tracker_entry_callback_fn_offset);
            FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "coarse_time", &tracker_entry_coarse_time_offset);
            FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "action", &tracker_entry_action_offset);

            FBE_GET_EXPRESSION("PhysicalPackage", block_transport_server_bouncer_completion, &physical_package_block_transport_server_bouncer_completion_p);

            FBE_GET_TYPE_SIZE(module_name, fbe_packet_tracker_entry_t, &tracker_entry_size);

            packet_tracker_ring_p = fbe_packet_p + packet_tracker_ring_offset;
            FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_callback_fn_offset + (tracker_entry_size * loop_index)), &callback_fn, ptr_size);
            FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_coarse_time_offset + (tracker_entry_size * loop_index)), &coarse_time, sizeof(fbe_u32_t));
            FBE_READ_MEMORY((packet_tracker_ring_p + tracker_entry_action_offset + (tracker_entry_size * loop_index)), &action, sizeof(fbe_u8_t));

            if (action == 1 && callback_fn == physical_package_block_transport_server_bouncer_completion_p) // todo, remove the hardcode
            {
                return (fbe_transport_coarse_time - coarse_time);
            }
        }

        // the packet has not been sent to physical package
        return 0;
    }
}

void fbe_get_rg_time_statics(fbe_u32_t rg_obj_id, 
                             fbe_u32_t *max_outstanding_time, fbe_u32_t *min_outstanding_time, fbe_u32_t *avg_outstanding_time,
                             fbe_u32_t *max_sl_wait_time, fbe_u32_t *min_sl_wait_time, fbe_u32_t *avg_sl_wait_time)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_u32_t ptr_size;
    
    dbg_queue_element_t packet_list_head;
    fbe_init_dbg_queue_list(&packet_list_head);

    FBE_GET_TYPE_SIZE(pp_ext_module_name, fbe_u32_t*, &ptr_size);
    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    

    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);

    topology_object_tbl_ptr += rg_obj_id * topology_object_table_entry_size;

    fbe_get_rg_outstanding_io_by_id(pp_ext_module_name, 
                                    topology_object_tbl_ptr,
                                    &packet_list_head,
                                    fbe_debug_trace_func,
                                    NULL,
                                    0);

    // get statics time
    {
        fbe_u32_t fbe_packet_cnt = 0;
        fbe_u32_t max_out_time = 0;
        fbe_u32_t min_out_time = 0xFFFFFFFF;
        fbe_u32_t total_out_time = 0;
        fbe_u32_t max_sl_time = 0;
        fbe_u32_t min_sl_time = 0xFFFFFFFF;
        fbe_u32_t total_sl_wait_time = 0;
        // loop over packet_list_head, get the time statics

        dbg_queue_element_t *dbg_queue_element_p;
        dbg_queue_item_t *packet_item_p;

        dbg_queue_element_p = packet_list_head.next;
        while (dbg_queue_element_p != &packet_list_head && dbg_queue_element_p != NULL)
        {
            fbe_packet_cnt++;

            packet_item_p = (dbg_queue_item_t*)dbg_queue_element_p;
            if (max_out_time < packet_item_p->private_attr.packet_attr.outstanding_time)
            {
                max_out_time = packet_item_p->private_attr.packet_attr.outstanding_time;
            }

            if (min_out_time > packet_item_p->private_attr.packet_attr.outstanding_time)
            {
                min_out_time = packet_item_p->private_attr.packet_attr.outstanding_time;
            }

            total_out_time += packet_item_p->private_attr.packet_attr.outstanding_time;

            if (max_sl_time < packet_item_p->private_attr.packet_attr.max_sl_wait_time)
            {
                max_sl_time = packet_item_p->private_attr.packet_attr.max_sl_wait_time;
            }

            if (min_sl_time > packet_item_p->private_attr.packet_attr.max_sl_wait_time)
            {
                min_sl_time = packet_item_p->private_attr.packet_attr.max_sl_wait_time;
            }

            total_sl_wait_time += packet_item_p->private_attr.packet_attr.max_sl_wait_time;
            
            // next 
            dbg_queue_element_p = dbg_queue_element_p->next;
        }

        *max_outstanding_time = max_out_time;
        *min_outstanding_time = min_out_time == 0xFFFFFFFF ? 0 : min_out_time;
        *max_sl_wait_time = max_sl_time;
        *min_sl_wait_time = min_sl_time == 0xFFFFFFFF ? 0 : min_sl_time;
        if (fbe_packet_cnt)
        {
            *avg_outstanding_time = (total_out_time / fbe_packet_cnt);
            *avg_sl_wait_time = (total_sl_wait_time / fbe_packet_cnt);
        }
        else
        {
            *avg_outstanding_time = 0;
            *avg_sl_wait_time = 0;
        }

    }

    // destroy the list
    fbe_destroy_dbg_queue_list(&packet_list_head);
}


void fbe_get_rg_outstanding_io_by_id(const fbe_u8_t * module_name,
                                     fbe_dbgext_ptr topology_object_tbl_ptr,
                                     dbg_queue_element_t *packet_list_head,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context,
                                     fbe_u32_t spaces_to_indent)
{
    dbg_queue_element_t * dbg_queue_element_p;
    dbg_queue_item_t * packet_item_p;

    fbe_dbgext_ptr fbe_transport_coarse_time_p;

    // get the global coarse time
    FBE_GET_EXPRESSION(pp_ext_module_name, fbe_transport_coarse_time, &fbe_transport_coarse_time_p);
    FBE_READ_MEMORY(fbe_transport_coarse_time_p, &fbe_transport_coarse_time, sizeof(fbe_u32_t));
    

    // get all the outstanding packet list
    fbe_get_rg_outstanding_packet_list_by_id(pp_ext_module_name, 
                                             topology_object_tbl_ptr,
                                             packet_list_head,
                                             trace_func,
                                             trace_context,
                                             spaces_to_indent);

    // loop over all the packets in the list
    dbg_queue_element_p = packet_list_head->next;
    while (dbg_queue_element_p != packet_list_head && dbg_queue_element_p != NULL)
    {
        packet_item_p = (dbg_queue_item_t *)dbg_queue_element_p;
        // get outstanding time
        fbe_get_outstanding_time(module_name, packet_item_p, trace_func, trace_context, spaces_to_indent);

        // get the max SL waiting time
        fbe_get_max_sl_waiting_time_from_tracker(module_name, packet_item_p, trace_func, trace_context, spaces_to_indent);

        // get the max SIOTS
        fbe_get_max_siots(module_name, packet_item_p, trace_func, trace_context, spaces_to_indent);

        // get the max PDO service time
        fbe_get_max_fruts(module_name, packet_item_p, trace_func, trace_context, spaces_to_indent);

        dbg_queue_element_p = dbg_queue_element_p->next;
    }

    // sort these packets against outstanding_time
    fbe_sort_packet_list(packet_list_head);
    
    return;
}

void fbe_get_max_siots(const fbe_u8_t * module_name,
                       dbg_queue_item_t *packet_item_p,
                       fbe_trace_func_t trace_func,
                       fbe_trace_context_t trace_context,
                       fbe_u32_t spaces_to_indent)
{

    fbe_dbgext_ptr siots_queue_p;
    fbe_time_t max_siots_time;

    fbe_raid_group_max_siots_time(module_name, packet_item_p->queue_item_in_dump, &siots_queue_p, &max_siots_time);

    // update packet_item_p

    packet_item_p->private_attr.packet_attr.siots_p = siots_queue_p;
    packet_item_p->private_attr.packet_attr.siots_outstanding_time = (fbe_u32_t)max_siots_time;
}

void fbe_init_dbg_queue_list(dbg_queue_element_t * list_head)
{
    list_head->next = list_head->prev = list_head;
}

void fbe_destroy_dbg_queue_list(dbg_queue_element_t * list_head)
{
    dbg_queue_element_t *dbg_queue_element_p;
    dbg_queue_element_t *dbg_queue_element_next_p;

    if(list_head == NULL)
    {
        return;
    }

    dbg_queue_element_p = (dbg_queue_element_t*)list_head->next;

    if(dbg_queue_element_p != NULL)
    {
        dbg_queue_element_next_p = (dbg_queue_element_t*)dbg_queue_element_p->next;
    }

    while (dbg_queue_element_p != list_head && dbg_queue_element_p != NULL)
    {
        // free
        free(dbg_queue_element_p);

        dbg_queue_element_p = dbg_queue_element_next_p;

        if(dbg_queue_element_p != NULL)
        {
            dbg_queue_element_next_p = (dbg_queue_element_t*)dbg_queue_element_p->next;
        }
    }

    list_head->next = list_head->prev = list_head;
}

/*!**************************************************************
 * fbe_raid_group_max_siots_time()
 ****************************************************************
 * @brief
 *  Calculate the raid siots max outstanding time.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param packet_p - fbe packet
 * @param siots_queue_p - ptr to siots which consume the max time.
 * @param max_siots_time - max consumed time in siots.
 *
 * @author
 *  7/16/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void fbe_raid_group_max_siots_time(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr packet_p,
                                              fbe_dbgext_ptr *siots_queue_p,
                                              fbe_time_t *max_siots_time)
{
    fbe_u64_t siots_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t common_flags;
    
    fbe_time_t m_seconds = 0;
    fbe_u64_t time_ptr = 0;
    fbe_time_t siots_time_stamp;
    fbe_time_t current_time;
    fbe_time_t temp_max_time = 0;
    fbe_u64_t temp_siots_p;

    fbe_u32_t payload_offset;
    fbe_u32_t iots_offset;
    fbe_u32_t siots_queue_offset;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_iots_t, "siots_queue", &siots_queue_offset);

    siots_queue_head_ptr = packet_p + payload_offset + iots_offset + siots_queue_offset;
    
    FBE_READ_MEMORY(siots_queue_head_ptr, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    if ((queue_element_p == siots_queue_head_ptr) || (queue_element_p == NULL))
    {
        *siots_queue_p = 0;
        *max_siots_time = INVALID_TIME;
        return;
    }

    /* Initialize the temp_siots_p as the first queue element */
    temp_siots_p = queue_element_p;
    
    /* Now that we have the head of the list, iterate over it, analyzing each siots.
     * We finish when we reach the end of the list (head) or null. 
     */
    while ((queue_element_p != siots_queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u64_t siots_p = queue_element_p;
        
        FBE_GET_FIELD_DATA(module_name, 
                           siots_p,
                           fbe_raid_common_t,
                           flags,
                           sizeof(fbe_u32_t),
                           &common_flags);

        /* Check the raid common flags to verify if the siots is completed 
          We caculate the incomplete siots consumed time */
        if((common_flags & FBE_RAID_COMMON_FLAG_TYPE_SIOTS) != 0)
        {
            FBE_GET_FIELD_DATA(module_name, 
                               siots_p, 
                               fbe_raid_siots_t, 
                               time_stamp, 
                               sizeof(fbe_time_t), 
                               &siots_time_stamp);
            
            FBE_GET_EXPRESSION(module_name, idle_thread_time, &time_ptr);
            FBE_READ_MEMORY(time_ptr, &current_time, sizeof(fbe_time_t));
            
            if (current_time < siots_time_stamp)
            {
                m_seconds = 0;
            }
            else
            {
                m_seconds = current_time - siots_time_stamp;
            }

        }
        /* TODO: refine */
        else
            m_seconds = 0;
        
        if(m_seconds > temp_max_time)
        {
            temp_max_time = m_seconds;
            temp_siots_p = siots_p;
        }

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }

    *max_siots_time = temp_max_time;
    *siots_queue_p = temp_siots_p;

    return;
}


static fbe_u32_t fruts_get_service_time(fbe_dbgext_ptr fruts_p)
{
    fbe_u32_t packet_offset;
    fbe_u64_t packet_p = 0;
    const char *module_name = "sep";

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_fruts_t, "io_packet", &packet_offset);
    packet_p = fruts_p + packet_offset;
    return fbe_get_pdo_service_time(module_name, packet_p);
}

static fbe_u32_t fruts_queue_get_max_service_time(fbe_dbgext_ptr head,
                                                  fbe_dbgext_ptr *fruts, fbe_u32_t prev_max_time)
{
    fbe_u64_t fruts_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u64_t prev_queue_element_p = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t length = 0;
    fbe_u32_t cur_max_time = prev_max_time;
    const char *module_name = "sep";

    *fruts = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);
    fruts_queue_head_ptr = head;
    FBE_READ_MEMORY(fruts_queue_head_ptr, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    /* Now that we have the head of the list, iterate over it, displaying each fruts.
     * We finish when we reach the end of the list (head) or null. 
     */
    while ((queue_element_p != fruts_queue_head_ptr) && 
           (queue_element_p != NULL) &&
           (queue_element_p != prev_queue_element_p) &&
           (length < FBE_RAID_MAX_DISK_ARRAY_WIDTH))
    {
        fbe_u64_t fruts_p = queue_element_p;
        fbe_u32_t service_time;

        if (FBE_CHECK_CONTROL_C())
            break;

        service_time = fruts_get_service_time(fruts_p);
        if (service_time > cur_max_time) {
            cur_max_time = service_time;
            *fruts = fruts_p;
        }

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        prev_queue_element_p = queue_element_p;
        queue_element_p = next_queue_element_p;
        length++;
    }
    return cur_max_time;
}

/*!**************************************************************
 * fbe_siots_get_max_service_time
 ****************************************************************
 * @brief
 *  get siots max read/write time
 *
 * @param info
 *    store fruts pointer and read/write time
 * @param siots
 *    pointer to siots
 *
 * @return None
 *
 * @author
 *  2013.07.16 - Created. Jamin Kang
 *
 ****************************************************************/
void fbe_siots_get_max_service_time(struct sep_siots_info *info, fbe_dbgext_ptr siots)
{
    fbe_u32_t offset;
    fbe_dbgext_ptr read_head, read2_head, write_head;
    fbe_u32_t max_read_time, max_write_time;
    const char *module_name = "sep";
    fbe_dbgext_ptr read_fruts_ptr;
    fbe_dbgext_ptr write_fruts_ptr;
    fbe_dbgext_ptr read2_fruts_ptr;

    if (siots == 0)
    {
        memset(info, 0, sizeof(struct sep_siots_info));
        return;
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "read_fruts_head", &offset);
    read_head = siots + offset;
    max_read_time = fruts_queue_get_max_service_time(read_head, &read_fruts_ptr, 0);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "read2_fruts_head", &offset);
    read2_head = siots + offset;
    max_read_time = fruts_queue_get_max_service_time(read2_head, &read2_fruts_ptr, max_read_time);
    if (read2_fruts_ptr != 0)
        read_fruts_ptr = read2_fruts_ptr;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "write_fruts_head", &offset);
    write_head = siots + offset;
    max_write_time = fruts_queue_get_max_service_time(write_head, &write_fruts_ptr, 0);

    info->siots = siots;
    info->max_read_fruts = read_fruts_ptr;
    info->max_read_time = max_read_time;
    info->max_write_fruts = write_fruts_ptr;
    info->max_write_time = max_write_time;
}

void fbe_get_max_fruts(const fbe_u8_t * module_name,
                       dbg_queue_item_t *packet_item_p,
                       fbe_trace_func_t trace_func,
                       fbe_trace_context_t trace_context,
                       fbe_u32_t spaces_to_indent)
{
    struct sep_siots_info siots_info;
    fbe_siots_get_max_service_time(&siots_info, packet_item_p->private_attr.packet_attr.siots_p);

    packet_item_p->private_attr.packet_attr.max_read_pdo_time_time = siots_info.max_read_time;
    packet_item_p->private_attr.packet_attr.max_read_fruts_p = siots_info.max_read_fruts;
    packet_item_p->private_attr.packet_attr.max_write_pdo_time_time = siots_info.max_write_time;
    packet_item_p->private_attr.packet_attr.max_write_fruts_p = siots_info.max_write_fruts;
}

void fbe_print_rg_outstanding_io_by_id(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr topology_object_tbl_ptr,
                                       dbg_queue_element_t *packet_list_head,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_u32_t spaces_to_indent)
{
    fbe_u32_t packets_cnt;
    dbg_queue_element_t * dbg_queue_element_p;
    dbg_queue_item_t * packet_item_p;
    fbe_dbgext_ptr raid_group_p;

    FBE_GET_FIELD_DATA(module_name,
                       topology_object_tbl_ptr,
                       topology_object_table_entry_s,
                       control_handle,
                       sizeof(fbe_dbgext_ptr),
                       &raid_group_p);


    // print the rg general info
    (void)fbe_rginfo_header_info_trace(module_name, trace_func, trace_context);
    (void)fbe_rginfo_rg_info_trace(module_name, raid_group_p, FALSE, trace_func, trace_context);

    // get list item cnt
    packets_cnt = fbe_get_list_item_count(packet_list_head);

    // show all the packets:
    trace_func(trace_context,"\t packets count %d\n", packets_cnt);                                                

    // show the list
    trace_func(trace_context,"\t packet                outstanding_time_ms current_state  sl_waiting_time_ms siots_high_timecost/ms      read_fruts_high_timecost/pdo_time_ms   write_fruts_high_timecost/pdo_time_ms\n");

    dbg_queue_element_p = (dbg_queue_element_t *)packet_list_head->next;
    while (dbg_queue_element_p != packet_list_head && dbg_queue_element_p != NULL)
    {
        packet_item_p = (dbg_queue_item_t*)dbg_queue_element_p;

        trace_func(trace_context, "\t 0x%-20llx", packet_item_p->queue_item_in_dump);
        trace_func(trace_context, "%-20d", packet_item_p->private_attr.packet_attr.outstanding_time);
        trace_func(trace_context, "%-15s", packet_item_p->private_attr.packet_attr.state == AT_RAID_LIBARARY ? "AT_RAID_LIB" : "WAITING_SL");
        trace_func(trace_context, "%-19d", packet_item_p->private_attr.packet_attr.max_sl_wait_time);
        trace_func(trace_context, "0x%016llx/%-9d", packet_item_p->private_attr.packet_attr.siots_p, packet_item_p->private_attr.packet_attr.siots_outstanding_time);
        trace_func(trace_context, "0x%016llx/%-20d", packet_item_p->private_attr.packet_attr.max_read_fruts_p, packet_item_p->private_attr.packet_attr.max_read_pdo_time_time);
        trace_func(trace_context, "0x%016llx/%-20d", packet_item_p->private_attr.packet_attr.max_write_fruts_p, packet_item_p->private_attr.packet_attr.max_write_pdo_time_time);
        trace_func(trace_context, "\n");
        dbg_queue_element_p = (dbg_queue_element_t *)dbg_queue_element_p->next;
    }

}

