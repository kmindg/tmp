/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-07
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * pp_ext.c
 ***************************************************************************
 *
 * File Description:
 *
 *   Debugging extensions for PhysicalPackage driver.
 *
 * Author:
 *   Peter Puhov
 *
 * Revision History:
 *
 * Peter Puhov 11/05/00  Initial version.
 *
 ***************************************************************************/
#include "dbgext.h"

char * pp_ext_module_name = "";

//
//  Description: Extension to display the version of pp.
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4ppver")
static char usageMsg_ppver[] =
"!ppver\n"
"  Display version of pp.\n";
#pragma data_seg ()

DECLARE_API(ppver)
{
     csx_dbg_ext_print("\tThe very first version of Physical Package debug macros\n");
} // end of !ppver extension

//
//  Description: sets global module name to "PhysicalPackage"
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4ppsetkernelmode")
static char usageMsg_ppsetkernelmode[] =
"!ppsetkernelmode\n"
"  Set global module name to PhysicalPackage \n";
#pragma data_seg ()

DECLARE_API(ppsetkernelmode)
{
	pp_ext_module_name = "PhysicalPackage";
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
} // end of !ppsetkernelmode extension

//
//  Description: sets global module name to ""
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4ppsetusermode")
static char usageMsg_ppsetusermode[] =
"!ppsetusermode\n"
"  Set global module name to \"\" \n";
#pragma data_seg ()

DECLARE_API(ppsetusermode)
{
	pp_ext_module_name = "";
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
} // end of !ppsetusermode extension

