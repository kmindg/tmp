/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-11
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_terminator_commands.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands for terminator registry.
.*
 * @version
 *   13- May- 2011:  Created. Hari Singh Chauhan
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "../src/services/topology/fbe_topology_private.h"
#include "fbe_terminator_debug.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_base_object_debug.h"


/* ***************************************************************************
 *
 * @brief
 *  Extension to display terminator registry Information.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   13- May- 2010:  Created. Hari Singh Chauhan
 *
 ***************************************************************************/
#pragma data_seg ("EXT_HELP$4tdr_encl_port_info")
static char CSX_MAYBE_UNUSED usageMsg_tdr_encl_port_info[] =
"!tdr_encl_port_info\n"
"   Display information for the Enclosure and Port data of terminator registry\n"
"   [-p <port_number>]\n"
"   [-e <enclosure_number>]\n"
"   [-d/s <drive_number/slot_number>]\n"
"   [-r <depth_of_the_display>]\n"
"   -r :\n"
"    1  Display just the port information\n"
"    2  Display the port and enclosure information\n"
"    3  Display the port, enclosure and drive information\n"
"   Examples:\n"
"      !tdr_encl_port_info -p 1 -e 0 -d 12\n"
"      !tdr_encl_port_info -p 2 -r 1\n"
"      !tdr_encl_port_info -e 0 -d 10\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(tdr_encl_port_info, "tdr_encl_port_info")
{
    fbe_char_t *str = NULL;
    fbe_u32_t terminator_registry_size = 0;
    fbe_s32_t port_number = -1;
    fbe_s32_t enclosure_number = -1;
    fbe_s32_t drive_number = -1;
    fbe_s32_t recursive_number = -1;
    fbe_dbgext_ptr registry_ptr = 0;

    /* Get the terminator_registry pointer information. */
    FBE_GET_EXPRESSION_WITHOUT_MODULE_NAME(fbe_terminator_registry, &registry_ptr);
    fbe_trace_indent(fbe_debug_trace_func, NULL, TERMINATOR_REGISTRY_INDENT);
    fbe_debug_trace_func(NULL, "terminator_registry_ptr: 0x%llx\n",
			 (unsigned long long)registry_ptr);

   /* Get the size of terminator_registry_t structure. */
    FBE_GET_TYPE_SIZE_WITHOUT_MODULE_NAME(terminator_registry_t, &terminator_registry_size);
    fbe_trace_indent(fbe_debug_trace_func, NULL, TERMINATOR_REGISTRY_INDENT);
    fbe_debug_trace_func(NULL, "terminator_registry_size: %u\n\n", terminator_registry_size);

    if(registry_ptr == 0){
        fbe_debug_trace_func(NULL, "terminator_registry_ptr is not available");
        fbe_debug_trace_func(NULL, "\n");
        return;
    }
    /* Process the command line arguments
     */
    str = strtok((char*)args, " \t");
    while(str != NULL)
    {
        if(strncmp(str, "-p", 2) == 0)
        {
            str = strtok(NULL, " \t");
            if(str != NULL)
                port_number = strtoul(str, 0, 0);
            else
            {
                fbe_debug_trace_func(NULL, "Please provide port number\n");
                return;
            }
        }
        else if(str != NULL && strncmp(str, "-e", 2) == 0)
        {
            str = strtok(NULL, " \t");
            if(str != NULL)
                enclosure_number = strtoul(str, 0, 0);
            else
            {
                fbe_debug_trace_func(NULL, "Please provide enclosure number\n");
                return;
            }
        }
        else if(str != NULL && (strncmp(str, "-d", 2) == 0 || strncmp(str, "-s", 2) == 0))
        {
            str = strtok(NULL, " \t");
            if(str != NULL)
                drive_number = strtoul(str, 0, 0);
            else
            {
                fbe_debug_trace_func(NULL, "Please provide drive number\n");
                return;
            }
        }
        else if(str != NULL && strncmp(str, "-r", 2) == 0)
        {
            str = strtok(NULL, " \t");
            if(str != NULL)
            {
                recursive_number = strtoul(str, 0, 0);
                if(!(recursive_number >= 1 && recursive_number <= 3))
                {
                    fbe_debug_trace_func(NULL, "recursive_number can be 1 / 2 /3 only");
                    fbe_debug_trace_func(NULL, "\n");
                    return;
                }
            }
            else
            {
                fbe_debug_trace_func(NULL, "Please provide valid depth number\n");
                return;
            }
        }
        else
        {
            fbe_debug_trace_func(NULL, "Please provide a valid option. See help for more details\n");
            return;
        }

        str = strtok(NULL, " \t");
    }

    fbe_terminator_registry_debug_trace(fbe_debug_trace_func, NULL, registry_ptr, port_number, enclosure_number, drive_number, recursive_number);
    fbe_debug_trace_func(NULL, "\n");
}


//end of fbe_terminator_commands.c
