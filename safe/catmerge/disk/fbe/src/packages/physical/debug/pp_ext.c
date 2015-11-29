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
#include "fbe/fbe_types.h"
#include "pp_ext.h"
#include "fbe/fbe_dbgext.h"

/* pp_ext_module_name represents the universal module name set by the
 * user and used by most commands.
 */
char * pp_ext_module_name = "PhysicalPackage";

/* pp_ext_physical_package_name can only be set to PhysicalPackage or VmPhysicalPackage.
 * This is used by commands that are only supported under these modes.
 */
char * pp_ext_physical_package_name = "PhysicalPackage";

/* By default active queues will be displayed */
static fbe_bool_t active_queue_display_flag = FBE_TRUE;
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
static char CSX_MAYBE_UNUSED usageMsg_ppver[] =
"!ppver\n"
"  Display version of pp.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(ppver, "ppver")
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
#pragma data_seg ("EXT_HELP$4set_module_name_physical_package")
static char CSX_MAYBE_UNUSED usageMsg_ppsetkernelmode[] =
"!set_module_name_physical_package\n"
"  Set global module name to PhysicalPackage \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(set_module_name_physical_package, "set_module_name_physical_package")
{
	pp_ext_module_name = "PhysicalPackage";
	/* pp_ext_physical_package_name will only toggle between PhysicalPackage and VmPhysicalPackage.
	 */
	pp_ext_physical_package_name = pp_ext_module_name;
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
    csx_dbg_ext_print("pp_ext_physiscal_package_name = \"%s\"  \n", pp_ext_physical_package_name);
} // end of !set_module_name_physical_package extension

//
//  Description: sets global module name to "VmSimPhysicalPackage"
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4set_module_name_vmsim_physical_package")
static char CSX_MAYBE_UNUSED usageMsg_ppsetvmsimkernelmode[] =
"!set_module_name_vmsim_physical_package\n"
"  Set global module name to VmSimPhysicalPackage \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(set_module_name_vmsim_physical_package, "set_module_name_vmsim_physical_package")
{
	pp_ext_module_name = "VmSimPhysicalPackage";
        /* pp_ext_physical_package_name will only toggle between PhysicalPackage and VmPhysicalPackage.
         */
	pp_ext_physical_package_name = pp_ext_module_name;
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
    csx_dbg_ext_print("pp_ext_physiscal_package_name = \"%s\"  \n", pp_ext_physical_package_name);
} // end of !set_module_name_vmsim_physical_package extension


//
//  Description: sets global module name to "sep"
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4set_module_name_sep")
static char CSX_MAYBE_UNUSED usageMsg_set_module_name_sep[] =
"!set_module_name_sep\n"
"  Set global module name to sep \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(set_module_name_sep, "set_module_name_sep")
{
	pp_ext_module_name = "sep";
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
} // end of !set_module_name_sep extension

//  Description: sets global module name to "espkg"
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4set_module_name_espkg")
static char CSX_MAYBE_UNUSED usageMsg_set_module_name_espkg[] =
"!set_module_name_espkg\n"
"  Set global module name to espkg \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(set_module_name_espkg, "set_module_name_espkg")
{
    pp_ext_module_name = "espkg";
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
}// end of !set_module_name_espkg extension

//  Description: sets global module name to ""
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4clear_module_name")
static char CSX_MAYBE_UNUSED usageMsg_clear_module_name[] =
"!clear_module_name\n"
"  Set global module name to \"\" \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(clear_module_name, "clear_module_name")
{
	pp_ext_module_name = "";
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
} // end of !clear_module_name extension

//  Description: sets global module name to "" so that it will be useful for userspace also.
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4userspace_module_name")
static char CSX_MAYBE_UNUSED usageMsg_userspace_module_name[] =
"!userspace_module_name\n"
"  Set global module name to \"\" \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(userspace_module_name, "userspace_module_name")
{
    pp_ext_module_name = "";
    csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
} // end of !userspace_module_name extension

//
//  Description: Disables 'active_queue_display_flag'
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4disable_active_queue_display")
static char CSX_MAYBE_UNUSED usageMsg_disable_active_queue_display[] =
"!disable_active_queue_display\n"
"  Disable the display of active queues. \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(disable_active_queue_display, "disable_active_queue_display")
{
    set_active_queue_display_flag(FBE_FALSE);
    csx_dbg_ext_print("Active queue display is disabled.\n");
}// end of !disable_active_queue_display extension

//
//  Description: Enables 'active_queue_display_flag'
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4enable_active_queue_display")
static char CSX_MAYBE_UNUSED usageMsg_enable_active_queue_display[] =
"!enable_active_queue_display\n"
"  Enable the display of active queues. \n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(enable_active_queue_display, "enable_active_queue_display")
{
    set_active_queue_display_flag(FBE_TRUE);
    csx_dbg_ext_print("Active queue display is enabled.\n");
}// end of !enable_active_queue_display extension

void  set_active_queue_display_flag(fbe_bool_t queue_flag)
{
    active_queue_display_flag = queue_flag;
}

fbe_bool_t get_active_queue_display_flag()
{
    return active_queue_display_flag;
}
#define FBE_DEBUG_MAX_DRIVERS 9
fbe_char_t fbe_debug_driver_names[FBE_DEBUG_MAX_DRIVERS][50] =
{
    "NtMirror",
    "ppfd",
    "PhysicalPackage",
    "espkg",
    "sep",
    "Fct",
    "DRAMCache",
    "psm",
    "interSPlock"
};

/*!**************************************************************
 * show_fbe_driver_info()
 ****************************************************************
 * @brief
 *  Display the list of driver entry points for a list of drivers.
 *  We can use this along with the map file and a script to
 *  determine call stacks that we put into the ktrace.
 *
 ****************************************************************/

#pragma data_seg ("EXT_HELP$4show_fbe_driver_info")
static char CSX_MAYBE_UNUSED usageMsg_show_fbe_driver_info[] =
"!show_fbe_driver_info\n"
"  show all the fbe driver offsets.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(show_fbe_driver_info, "show_fbe_driver_info")
{
    ULONG64 driver_entry_p = 0;
    fbe_u32_t index;

    FBE_GET_EXPRESSION("EmcPAL", DriverEntry, &driver_entry_p);
    csx_dbg_ext_print("EmcPAL!DriverEntry = 0x%llx\n", driver_entry_p);

    for (index = 0; index < FBE_DEBUG_MAX_DRIVERS; index++)
    {
        FBE_GET_EXPRESSION(fbe_debug_driver_names[index], EmcpalDriverEntry, &driver_entry_p);
        csx_dbg_ext_print("%s!EmcpalDriverEntry = 0x%llx\n", fbe_debug_driver_names[index], driver_entry_p);
    } 
}
/******************************************
 * end show_fbe_driver_info
 ******************************************/
