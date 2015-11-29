#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "fbe_lifecycle.h"
#include "fbe_interface.h"
#include "fbe_topology_debug.h"


#pragma data_seg ("EXT_HELP$4ppobjrotary")
static char CSX_MAYBE_UNUSED usageMsg_ppobjrotary[] =
"!ppobjrotary [-s lifecycle_state] <object_id>\n"
"  Display an object's rotary for a lifecyle state.\n"
"    object_id = the object id (in hex).\n"
"    lifecycle_state = the rotary's state (in hex) defaults to the current state.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(ppobjrotary, "ppobjrotary")
{
    fbe_lifecycle_state_t lifecycle_state;
    fbe_object_id_t object_id;
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
	/* Since we are not going to display the rotary conditions for BVD object */
	if((strncmp(pp_ext_module_name, "sep", 3) == 0) &&
       (object_id == 0x0))
	{
		object_id = FBE_OBJECT_ID_INVALID;
	}

    trace_context.trace_env = FBE_LIFECYCLE_TRACE_EXT;
    trace_context.p_module_name = pp_ext_module_name;
    trace_context.trace_func = csx_dbg_ext_print;
	fbe_lifecycle_debug_process_rotary(&trace_context, object_id, lifecycle_state);
}

#pragma data_seg ("EXT_HELP$4pplifecyclestates")
static char CSX_MAYBE_UNUSED usageMsg_pplifecyclestates[] =
"!pplifecyclestates\n"
"  Display the lifecycle state constants.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(pplifecyclestates, "pplifecyclestates")
{
    fbe_lifecycle_state_type_t state;
    const char * p_state_name;

    csx_dbg_ext_print("\tNon-pending states:\n");
    for (state = 0; state < FBE_LIFECYCLE_STATE_NON_PENDING_MAX; state++) {
        p_state_name = "<error>";
        fbe_lifecycle_debug_get_state_name(state, &p_state_name);
        csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
    }
    csx_dbg_ext_print("\tPending states:\n");
    for ( ; state < FBE_LIFECYCLE_STATE_PENDING_MAX; state++) {
        p_state_name = "<error>";
        fbe_lifecycle_debug_get_state_name(state, &p_state_name);
        csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
    }
    csx_dbg_ext_print("\tInvalid states:\n");
    state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_lifecycle_debug_get_state_name(state, &p_state_name);
    csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
    state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_lifecycle_debug_get_state_name(state, &p_state_name);
    csx_dbg_ext_print("\t\t0x%02X %s\n", state, p_state_name);
}
