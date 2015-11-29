/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-10
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 *  All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file raid_library_ext.c
 ***************************************************************************
 *
 * @brief
 *   Raid debug extensions.
 *
 * @author
 *  @3/25/2010 - Created. Rob Foley
 *
 ***************************************************************************/

#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_raid_common.h"
#include "fbe_raid_library_debug.h"
#include "fbe_transport_debug.h"

/*!**************************************************************
 * fbe_raid_library_debug_print_siots_verbose()
 ****************************************************************
 * @brief
 *  Display the raid siots structure in full detail.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param siots_p - ptr to iots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/31/2012 - seperated from CSX_DBG_EXT_DEFINE_CMD( raid_siots , " raid_siots "). NCHIU
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_debug_print_siots_verbose(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr siots_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_u32_t spaces_to_indent)
{
    /* A siots argument was provided.
     */
    fbe_u64_t siots_common_p = 0; 
    fbe_u64_t siots_common_parent_p = 0;
    fbe_u64_t siots_common_parent = 0;
    fbe_u32_t Offset;
    fbe_u32_t flags; 
    fbe_u32_t ptr_size;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    /* Depending on the type, display either the iots (for normal siots),
     * or the entire iots, siots, nested siots chain (for nested siots).  
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "common", &Offset);
    siots_common_p = siots_p + Offset;
    FBE_GET_FIELD_DATA(module_name, siots_common_p, fbe_raid_common_t, flags, sizeof(flags), &flags);
    switch(flags & FBE_RAID_COMMON_FLAG_TYPE_ALL_STRUCT_TYPE_MASK)
    {
        case FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS:
            /* Nested siots,
             * Go all the way up to the IOTS, and display the entire chain.
             */
            FBE_GET_FIELD_OFFSET(module_name, fbe_raid_common_t, "parent_p", &Offset);
            siots_common_parent_p = siots_common_p + Offset;
            FBE_READ_MEMORY(siots_common_parent_p, &siots_common_parent, ptr_size);
            trace_func(trace_context, "SIOTS nest parent is 0x%llx\n", siots_common_parent_p );
            
            siots_common_p = siots_common_parent;
            siots_common_parent_p = siots_common_p + Offset;
            FBE_READ_MEMORY(siots_common_parent_p, &siots_common_parent, ptr_size);
            trace_func(trace_context, "IOTS is 0x%llx\n", siots_common_parent_p );
            fbe_raid_library_debug_print_iots(module_name, siots_common_parent,
                                              fbe_debug_trace_func, NULL, 2 /* spaces to indent */);
            break;

        case FBE_RAID_COMMON_FLAG_TYPE_SIOTS:
            /* Regular SIOTS, just display the parent IOTS.
             */
            FBE_GET_FIELD_OFFSET(module_name, fbe_raid_common_t, "parent_p", &Offset);
            siots_common_parent_p = siots_common_p + Offset;
            FBE_READ_MEMORY(siots_common_parent_p, &siots_common_parent, ptr_size);
            fbe_raid_library_debug_print_iots(module_name, siots_common_parent,
                                              fbe_debug_trace_func, NULL, 2 /* spaces to indent */);
            break;
        default:
            trace_func(trace_context, "Unknown common type 0x%x\nALL", (flags & FBE_RAID_COMMON_FLAG_TYPE_ALL_STRUCT_TYPE_MASK));
            break;
    }; /* end switch siots_p->common.flags & RAID_ALL_STRUCT_TYPE_MASK */

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_raid_library_debug_print_iots()
 ****************************************************************
 * @brief
 *  Display the raid iots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param iots_p - ptr to iots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_display_iots(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr iots_p,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset;
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr packet_p = 0;
    fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (ptr_size == 0)
    {
        trace_func(trace_context, "%s found pointer size is zero\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Display packet.
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_iots_t, "packet_p", &offset);
    FBE_READ_MEMORY(iots_p + offset, &packet_p, ptr_size);
    fbe_transport_print_fbe_packet(module_name, packet_p, trace_func, trace_context, spaces_to_indent);
    fbe_raid_library_debug_print_iots(module_name, iots_p,
                                      trace_func, trace_context, spaces_to_indent);
    return status;
}
/**************************************
 * end fbe_raid_library_debug_display_iots()
 **************************************/
CSX_DBG_EXT_DEFINE_CMD( raid_siots , " raid_siots ")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;

    /* For now we assume that only sep package has raid type of objects.
     * If it is not set to sep, then some things might not get resolved properly.
     */
    const fbe_u8_t * module_name = "sep";
    if (strlen(args) && strncmp(args,"-t",2) == 0)
    {
        trace_func(trace_context, "0x%llx 0x%x siots_p: 0x%llx\n",
		   (unsigned long long)strlen(args), strncmp(args,"-t",2),
		   (unsigned long long)GetArgument64(args, 2));
        /* The -t option means "terse".  In this case we simply display 
         * the provided siots.
         */
        fbe_raid_library_debug_print_siots(module_name, GetArgument64(args, 2), 
                                           fbe_debug_trace_func, NULL, 2 /* spaces to indent */);
    }
    else if (strlen(args))
    {
        fbe_u64_t siots_p;

        trace_func(trace_context, "Input SIOTS is 0x%llx\n", GetArgument64 (args, 1));
        siots_p = GetArgument64 (args, 1);
        fbe_raid_library_debug_print_siots_verbose(module_name, siots_p, trace_func, trace_context, 2);
    } /* end if strlen(args). */
    else
    {
        //trace_func(trace_context, "Showing all in-use iots in the system. Minors 0..0x%x (0..%d)\n",
        //        RG_MAX_MINOR,
        //        RG_MAX_MINOR);

        //raid_find_iots_64(0, RG_MAX_MINOR, RAID_DISPLAY_VERBOSE);
    }/* end else-if (! strlen (args)) */
    return;
}
#pragma data_seg ("EXT_HELP$4raid_siots")
static char CSX_MAYBE_UNUSED raid_siotsUsageMsg[] =
"!raid_siots\n"
"   Display information for all *IN USE* SIOTS structures\n"
"!raid_siots <flags> <siots pointer>\n"
"   -t - Only display the siots, not any parent structures.\n"
"   Display information for the specified SIOTS\n";
#pragma data_seg (".data")

CSX_DBG_EXT_DEFINE_CMD( raid_iots , " raid_iots ")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t iots_p = 0;
    /* For now we assume that only sep package has raid type of objects.
     * If it is not set to sep, then some things might not get resolved properly.
     */
    const fbe_u8_t * module_name = "sep";

    if (strlen(args) == 0)
    {
        trace_func(trace_context, "!raid_iots <iots_ptr>  ptr argument must be provided.\n");
    }
    else
    {
        iots_p = GetArgument64(args, 1);
        fbe_raid_library_debug_display_iots(module_name, iots_p,
                                            fbe_debug_trace_func, NULL, 2);
    }
    return;
}
#pragma data_seg ("EXT_HELP$4raid_iots")
static char CSX_MAYBE_UNUSED raid_iotsUsageMsg[] =
"!raid_iots <iots pointer>\n"
"   Display information for the specified IOTS\n";
#pragma data_seg (".data")

CSX_DBG_EXT_DEFINE_CMD( raid_fruts , " raid_fruts ")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t fruts_common_p = 0;
    fbe_u64_t fruts_p = 0;
    fbe_u64_t siots_p = 0;
    fbe_u32_t spaces_to_indent = 2;
    fbe_u32_t ptr_size;
    fbe_u32_t offset;

    /* For now we assume that only sep package has raid type of objects.
     * If it is not set to sep, then some things might not get resolved properly.
     */
    const fbe_u8_t * module_name = "sep";

    fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (ptr_size == 0)
    {
        trace_func(trace_context, "%s found pointer size is zero\n", __FUNCTION__);
        return ;
    }

    if (strlen(args) == 0)
    {
        trace_func(trace_context, "!raid_fruts <fruts_ptr>  ptr argument must be provided.\n");
    }
    else
    {
        fruts_p = GetArgument64(args, 1);
        /* get parent.
        */
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_siots_t, "common", &offset);
        fruts_common_p = fruts_p + offset;
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_common_t, "parent_p", &offset);
        FBE_READ_MEMORY(fruts_common_p + offset, &siots_p, ptr_size);
        if (siots_p == NULL)
        {
            trace_func(trace_context, "NULL parent for fruts 0x%llx.\n", fruts_p);
            return;
        }
        fbe_raid_library_debug_print_siots_verbose(module_name, siots_p, trace_func, trace_context, spaces_to_indent);
    }
    return;
}

#pragma data_seg ("EXT_HELP$4raid_fruts")
static char CSX_MAYBE_UNUSED raid_frutsUsageMsg[] =
"!raid_fruts <iots pointer>\n"
"   Display information for the specified fruts\n";
#pragma data_seg (".data")

CSX_DBG_EXT_DEFINE_CMD( raid_print_sg_list , " raid_print_sg_list ")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t sg_list_p = 0;
    fbe_lba_t lba_base = 0;
    /* For now we assume that only sep package has raid type of objects.
     * If it is not set to sep, then some things might not get resolved properly.
     */
    const fbe_u8_t * module_name = "sep";

    if (strlen(args) == 0)
    {
        trace_func(trace_context, "!raid_print_sg_list <fbe_sg_element_t*> sg element ptr must be provided.\n");
    }
    else
    {
        sg_list_p = GetArgument64(args, 1);
        lba_base = GetArgument64(args, 2);
        fbe_raid_library_debug_print_sg_list(module_name, sg_list_p,
                                             fbe_debug_trace_func, NULL, 2, /* spaces to indent */
                                             0, /* Do not display any sector words */
                                             lba_base);
    }
    return;
}
#pragma data_seg ("EXT_HELP$4raid_print_sg_list")
static char CSX_MAYBE_UNUSED raid_print_sg_listUsageMsg[] =
"!raid_print_sg_list <fbe_sg_element_t*><base lba>\n"
"   Display sg list\n";
#pragma data_seg (".data")

CSX_DBG_EXT_DEFINE_CMD( raid_print_sg_list_sectors , " raid_print_sg_list_sectors ")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t sg_list_p = 0;
    fbe_u32_t num_sector_words_to_trace; 
    fbe_lba_t lba_base = 0;
    /* By default trace the metadata and 4 overhead words.
     */
    #define RAID_PRINT_SG_LIST_SECTORS_MIN_WORDS 6
    /* For now we assume that only sep package has raid type of objects.
     * If it is not set to sep, then some things might not get resolved properly.
     */
    const fbe_u8_t * module_name = "sep";

    if (strlen(args) == 0)
    {
        trace_func(trace_context, "!raid_print_sg_list_sectors <fbe_sg_element_t*> sg element ptr must be provided.\n");
    }
    else
    {
        sg_list_p = GetArgument64(args, 1);
        num_sector_words_to_trace = (fbe_u32_t)GetArgument64(args, 2);
        lba_base = GetArgument64(args, 3);

        if (num_sector_words_to_trace < RAID_PRINT_SG_LIST_SECTORS_MIN_WORDS)
        {
            num_sector_words_to_trace = RAID_PRINT_SG_LIST_SECTORS_MIN_WORDS;
        }
        fbe_raid_library_debug_print_sg_list(module_name, sg_list_p,
                                             fbe_debug_trace_func, NULL, 2, /* spaces to indent */
                                             num_sector_words_to_trace, lba_base);
    }
    return;
}
#pragma data_seg ("EXT_HELP$4raid_print_sg_list_sectors")
static char CSX_MAYBE_UNUSED raid_print_sg_list_sectorsUsageMsg[] =
"!raid_print_sg_list_sectors <fbe_sg_element_t*>\n"
"   Display sectors in an sg list\n"
"!raid_print_sg_list_sectors <fbe_sg_element_t*><number of sector words to trace><base lba>\n"
"   Display sectors in an sg list.  Display this number of words from every sector.\n";
#pragma data_seg (".data")
/*************************
 * end file raid_library_ext.c
 *************************/
