/***************************************************************************
 *  Copyright (C)  EMC Corporation 2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 *  All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file rdgen_ext.c
 ***************************************************************************
 *
 * @brief
 *   Rdgen debug extensions.
 *
 * @author
 *  3/25/2010 - Created. Rob Foley
 *
 ***************************************************************************/

#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "fbe_rdgen_debug.h"


CSX_DBG_EXT_DEFINE_CMD( rdgen_display , " rdgen_display ")
{
    fbe_bool_t b_display_packets = FBE_FALSE;
    fbe_bool_t b_display_queues = FBE_TRUE;
    fbe_u32_t ptr_size = 0;
    fbe_u8_t const * sim_module_name = "NewNeitPackage";
    fbe_u8_t const * hw_module_name = "new_neit";
    static fbe_u8_t const *module_name = NULL;

    if (strlen(args) && strncmp(args, "-v", 3) == 0)
    {
        b_display_packets = FBE_TRUE;
        /* Increment past the io flag.
         */
        args += FBE_MIN(4, strlen(args));
    }
    if (strlen(args) && strncmp(args, "-t", 3) == 0)
    {
        b_display_queues = FBE_FALSE;
        /* Increment past the io flag.
         */
        args += FBE_MIN(4, strlen(args));
    }

    /* We either are in simulation or on hardware.
     * Depending on where we are, use a different module name. 
     * We set the module name once and then will use it if we get called again. 
     */
    if (module_name == NULL)
    {
        /* Check if we are sim.
         */
        FBE_GET_TYPE_SIZE(sim_module_name, fbe_u32_t*, &ptr_size);

        if (ptr_size == 0)
        {
            /* Not sim, use hardware name.
             */
            module_name = hw_module_name;
        }
        else
        {
            /* Sim, use sim module name.
             */
            module_name = sim_module_name;
        }
    }
    fbe_rdgen_debug_display(module_name, fbe_debug_trace_func, NULL, 2    /* spaces to indent */, b_display_packets, b_display_queues);
    return;
}
#pragma data_seg ("EXT_HELP$4rdgen_display")
static char CSX_MAYBE_UNUSED rdgen_displayUsageMsg[] =
"!rdgen_display\n"
"   Display information for all rdgen structures\n"
"   -t (terse) do not display any queues\n"
"   -v verbose display will display the contents of all packets that are outstanding\n";
#pragma data_seg (".data")
/*************************
 * end file rdgen_ext
 *************************/
