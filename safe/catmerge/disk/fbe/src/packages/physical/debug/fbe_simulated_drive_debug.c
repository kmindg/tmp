
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_simulated_drive_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the simulated drive.
 *
 * @author
 *  5/9/2012 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "pp_dbgext.h"
#include "fbe/fbe_sector.h"
#include "fbe_terminator_debug.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * fbe_sim_drive_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param terminator_drive_p - Ptr to object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_sim_drive_debug_trace(const fbe_u8_t * module_name,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_bool_t b_display_journal)
{
    fbe_status_t status;
    fbe_block_count_t total_journal_blocks = 0;
    fbe_block_count_t journal_blocks;
    fbe_u32_t ptr_size;
    fbe_u32_t drive_size;
    fbe_dbgext_ptr drive_table_ptr = 0;
    fbe_dbgext_ptr drive_p = 0;
    int i = 0;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    FBE_GET_TYPE_SIZE(module_name, terminator_drive_t, &drive_size);

    FBE_GET_EXPRESSION(module_name, drive_table, &drive_table_ptr);

    for (i = 0; i < 1024; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        FBE_READ_MEMORY(drive_table_ptr, &drive_p, ptr_size);
        if (drive_p != NULL)
        {
            fbe_trace_indent(trace_func, NULL, 2 /*spaces_to_indent*/);
            trace_func(NULL, "%3d) 0x%llx ", i, (unsigned long long)drive_p);
            FBE_GET_FIELD_DATA(module_name,
                           drive_p,
                           terminator_drive_t,
                           journal_blocks_allocated,
                           sizeof(fbe_block_count_t),
                           &journal_blocks);
            total_journal_blocks += journal_blocks;
            fbe_terminator_drive_debug_trace(module_name, drive_p, trace_func, NULL, NULL, 0, b_display_journal);
        }
        drive_table_ptr += ptr_size;
    }
    trace_func(NULL, "Total journal blocks: 0x%llx (%d) %d MB \n", 
               (unsigned long long)total_journal_blocks, (int)total_journal_blocks, (int)(total_journal_blocks * FBE_BE_BYTES_PER_BLOCK)/(1024*1024));
    return status;
}

#pragma data_seg ("EXT_HELP$4")
static char CSX_MAYBE_UNUSED usageMsg_sim_drive_server[] =
"!sim_drive_server\n"
"  display simulated drive objects\n"
"  -v display journal records\n.";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(sim_drive_server, "sim_drive_server")
{
    fbe_bool_t b_display_records = FBE_FALSE;
    fbe_char_t *str = NULL;
    /* Use blank module name so no matter executible name, it will just search for symbols.
     */
    char * module_name = "";

    str = strtok((char*)args, " \t");
    while (str != NULL)
    {
        if(strncmp(str, "-v", 3) == 0)
        {
            b_display_records = FBE_TRUE;
        }
        str = strtok(NULL, " \t");
    }
    fbe_sim_drive_debug_trace(module_name, fbe_debug_trace_func, NULL, b_display_records);
    return;
}


