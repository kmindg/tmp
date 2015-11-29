#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "fbe_lifecycle.h"
#include "fbe_interface.h"

static fbe_dbgext_ptr lifecycle_get_object_ptr(fbe_object_id_t object_id)
{
    fbe_dbgext_ptr p_object_table = 0;
    fbe_dbgext_ptr p_table_entry = 0;
    fbe_dbgext_ptr p_object = 0;
    unsigned long entry_size = 0;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return NULL; 
    }

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if (object_id >= max_entries) {
        return 0;
    }

    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &p_object_table);
    FBE_READ_MEMORY(p_object_table, &topology_object_tbl_ptr, ptr_size);
    if (topology_object_tbl_ptr == 0) {
        return 0;
    }

    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &entry_size);
    /* csx_dbg_ext_print("\tDEBUG: topology object table entry size %d\n", entry_size); */
    if (entry_size == 0) {
        return 0;
    }

    p_table_entry = topology_object_tbl_ptr + (entry_size * object_id);
    /* csx_dbg_ext_print("\tDEBUG: topology object table entry at 0x%I64x\n", p_table_entry); */

    FBE_GET_FIELD_DATA(pp_ext_module_name,
                       p_table_entry,
                       topology_object_table_entry_s,
                       control_handle,
                       sizeof(fbe_dbgext_ptr),
                       &p_object);
    return p_object;
}

#pragma data_seg ("EXT_HELP$4ppobjrotary")
static char usageMsg_ppobjrotary[] =
"!ppobjrotary [-s lifecycle_state] <object_id>\n"
"  Display an object's rotary for a lifecyle state.\n"
"    object_id = the object id (in hex).\n"
"    lifecycle_state = the rotary's state (in hex) defaults to the current state.\n";
#pragma data_seg ()

DECLARE_API(ppobjrotary)
{
    fbe_lifecycle_state_t lifecycle_state;
    fbe_object_id_t object_id;
    fbe_dbgext_ptr p_object;
    fbe_lifecycle_trace_context_t trace_context;
    fbe_u32_t arg_ii;

    if (strlen(args) <= 0) {
        csx_dbg_ext_print("\tusage: !ppobjrotary [-s lifecycle_state] <object_id>\n");
        return;
    }
    /* csx_dbg_ext_print("\tDEBUG: entered ppobjrotary, args: \"%s\"\n", args); DEBUG */

    arg_ii = 1;
    lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    if ((args[0] == '-') && (args[1] == 's')) {
        lifecycle_state = (fbe_lifecycle_state_t)GetArgument64(args, 2);
        if (lifecycle_state >= FBE_LIFECYCLE_STATE_NON_PENDING_MAX) {
            csx_dbg_ext_print("\tERROR! invalid lifecycle state for rotary, state: 0x%X\n", lifecycle_state);
            return;
        }
        arg_ii = 3;
        /* csx_dbg_ext_print("\tDEBUG: lifecycle_state: 0x%X\n", lifecycle_state); DEBUG */
    }

    object_id = FBE_OBJECT_ID_INVALID;
    object_id = (fbe_object_id_t)GetArgument64(args, arg_ii);
    p_object = lifecycle_get_object_ptr(object_id);
    if (p_object == 0) {
        csx_dbg_ext_print("\tERROR! invalid object id 0x%X\n", object_id);
        return;
    }

    trace_context.trace_env = FBE_LIFECYCLE_TRACE_EXT;
    trace_context.p_module_name = pp_ext_module_name;
    trace_context.trace_func = csx_dbg_ext_print;
    fbe_lifecycle_debug_trace_rotary(&trace_context, object_id, p_object, lifecycle_state);
}

#pragma data_seg ("EXT_HELP$4pplifecyclestates")
static char usageMsg_pplifecyclestates[] =
"!ppobjrotary\n"
"  Display the lifecycle state constants.\n";
#pragma data_seg ()

DECLARE_API(pplifecyclestates)
{
    fbe_lifecycle_state_type_t state;
    const char * p_state_name;

    csx_dbg_ext_print("\tNon-pending states:\n");
    for (state = 0; state < FBE_LIFECYCLE_STATE_NON_PENDING_MAX; state++) {
        p_state_name = "<error>";
        fbe_get_lifecycle_state_name(state, &p_state_name);
        csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
    }
    csx_dbg_ext_print("\tPending states:\n");
    for ( ; state < FBE_LIFECYCLE_STATE_PENDING_MAX; state++) {
        p_state_name = "<error>";
        fbe_get_lifecycle_state_name(state, &p_state_name);
        csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
    }
    csx_dbg_ext_print("\tInvalid states:\n");
    state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_get_lifecycle_state_name(state, &p_state_name);
    csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
    state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_get_lifecycle_state_name(state, &p_state_name);
    csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
}
