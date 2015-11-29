/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*
 * redirect [file]
 *  redirect csx_dbg_ext_print output to file or restore to debugger output window
 *
 *  ported from fdb_redirect.c
 */

#include "pp_dbgext.h"

static FILE * ppdbg_redirect_file_handle = NULL;

static int ppdbg_redirect_open_file(const char *file_name)
{
    ppdbg_redirect_file_handle = NULL;
    ppdbg_redirect_file_handle = fopen(file_name, "w");
    return(ppdbg_redirect_file_handle != NULL);
}

static void ppdbg_redirect_close_file(void)
{
    fclose(ppdbg_redirect_file_handle);
    ppdbg_redirect_file_handle = NULL;
}

static void ppdbg_redirect_write_file(csx_cstring_t data, ...)
{
    int written;
    written = fputs(data, ppdbg_redirect_file_handle);
}

void CSX_DBGEXT_DEFCC ppdbg_redirect_dbg_print(csx_dbg_ext_print_interceptor_context_t context, csx_cstring_t intercepted_print)
{
    if (ppdbg_redirect_file_handle) {
        ppdbg_redirect_write_file(intercepted_print);
    }
}

CSX_DBG_EXT_DEFINE_CMD(redirect, "redirect")
{   
    if (ppdbg_redirect_file_handle)
    {
        
        csx_dbg_ext_print_interceptor_clear();
        ppdbg_redirect_close_file();
        csx_dbg_ext_print("Output is no longer redirected\n");
    }
    if (*args) 
    {
        if (ppdbg_redirect_open_file(args))
        {
            csx_dbg_ext_print("Output now redirected to file %s.\n", args);
            csx_dbg_ext_print_interceptor_set(NULL, ppdbg_redirect_dbg_print);
        }
        else
        {
            csx_dbg_ext_print("Unable to open destination file %s. Output redirection aborted.\n", args);
        }
    }
}

#pragma data_seg ("EXT_HELP$4redirect")
static char CSX_MAYBE_UNUSED usageMsg_redirect[] =
"!redirect [file.txt]\n"
"  Redirect debugger output to file.txt,\n"
"  or close and revert to screen display.\n";
#pragma data_seg (".data")
