/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_board_ext.c
 ***************************************************************************
 *
 * @brief
 *  This file contain debug function used to board info.
 * 
 * @version
 *  11-Aug-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_class.h"
#include "base_board_private.h"
#include "fbe_base_object_debug.h"
#include "generic_utils_lib.h"
#include "fbe_topology_debug.h"
#include "fbe/fbe_dbgext.h"

/* forward declaration */
static void fbe_armada_base_board_debug_prvt_data(fbe_dbgext_ptr control_handle_ptr);


#pragma data_seg ("EXT_HELP$4ppboardprvdata")
static char CSX_MAYBE_UNUSED usageMsg_ppboardprvdata[] =
"!ppboardprvdata \n"
"  Display the base board private data\n";
#pragma data_seg ()
CSX_DBG_EXT_DEFINE_CMD(ppboardprvdata, "ppboardprvdata")
{
    int index;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_topology_object_status_t object_status;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_class_id_t class_id;
    const fbe_u8_t * module_name;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    module_name = fbe_get_module_name();
    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return; 
    }
    
    /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    fbe_debug_trace_func(NULL, "\t topology_object_table_ptr %llx\n",
			 (unsigned long long)topology_object_table_ptr);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    fbe_debug_trace_func(NULL, "\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    if(topology_object_table_ptr == 0)
    {
        return;
    }

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    object_status = 0;
    for(index = 0; index < max_entries ; index++)
    {
        /* Fetch the object's topology status. */
        FBE_GET_FIELD_DATA(module_name,
                           topology_object_tbl_ptr,
                           topology_object_table_entry_s,
                           object_status,
                           sizeof(fbe_topology_object_status_t),
                           &object_status);

        if(object_status != FBE_TOPOLOGY_OBJECT_STATUS_NOT_EXIST)
        {
            /* Fetch the control handle, which is the pointer to the object. */
            FBE_GET_FIELD_DATA(module_name,
                               topology_object_tbl_ptr,
                               topology_object_table_entry_s,
                               control_handle,
                               sizeof(fbe_dbgext_ptr),
                               &control_handle_ptr);

            /* If the object's topology status is ready, then get the
             * topology status.
             */
            // csx_dbg_ext_print("\n object status is ready \n");
            FBE_GET_FIELD_DATA(module_name,
                               control_handle_ptr,
                               fbe_base_object_s,
                               class_id,
                               sizeof(fbe_class_id_t),
                               &class_id);

            if(class_id == FBE_CLASS_ID_ARMADA_BOARD)
            {
                fbe_armada_base_board_debug_prvt_data(control_handle_ptr );
	    	return;
            }
        }
        topology_object_tbl_ptr += topology_object_table_entry_size;
    }
    return;
}

/*!*************************************************************************
 * fbe_armada_base_board_debug_prvt_data(fbe_dbgext_ptr control_handle_ptr)
 ***************************************************************************
 * @brief
 *  This function is used display board info
 *
 * @param  control_handle_ptr - Pointer to base board.
 *
 * @author 
 *  11-Aug-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static void 
fbe_armada_base_board_debug_prvt_data(fbe_dbgext_ptr control_handle_ptr)
{
    fbe_u32_t base_board_offset;
    fbe_base_board_t   base_board_value;
    const fbe_u8_t * module_name;
    fbe_dbgext_ptr base_board_ptr;
       
    module_name = fbe_get_module_name();
    FBE_GET_FIELD_OFFSET(module_name, fbe_armada_board_s, "base_board", &base_board_offset); 

    base_board_ptr = control_handle_ptr + base_board_offset;

    FBE_GET_FIELD_DATA(module_name, base_board_ptr, fbe_base_board_s, number_of_io_ports, sizeof(fbe_u32_t), &base_board_value.number_of_io_ports);
    FBE_GET_FIELD_DATA(module_name, base_board_ptr, fbe_base_board_s, edal_block_handle, sizeof(fbe_edal_block_handle_t), &base_board_value.edal_block_handle);
    FBE_GET_FIELD_DATA(module_name, base_board_ptr, fbe_base_board_s, spid, sizeof(SP_ID), &base_board_value.spid);
    FBE_GET_FIELD_DATA(module_name, base_board_ptr, fbe_base_board_s, localIoModuleCount, sizeof(fbe_u32_t), &base_board_value.localIoModuleCount);
    FBE_GET_FIELD_DATA(module_name, base_board_ptr, fbe_base_board_s, platformInfo, sizeof(SPID_PLATFORM_INFO), &base_board_value.platformInfo);

    fbe_debug_trace_func(NULL, "Number of IO ports: %d\n", base_board_value.number_of_io_ports);
    fbe_debug_trace_func(NULL, "Edal Block Handle: 0x%x\n", base_board_value.edal_block_handle);
    fbe_debug_trace_func(NULL, "SPID: %s\n", decodeSpId(base_board_value.spid));
    fbe_debug_trace_func(NULL, "Local IO Module Count: %d\n",base_board_value.localIoModuleCount);
    /* Print the Platform Info */
    fbe_debug_trace_func(NULL, "Platform Info:\n");
    fbe_debug_trace_func(NULL, "\tFamily Type: %s\n", decodeFamilyType(base_board_value.platformInfo.familyType));
    fbe_debug_trace_func(NULL, "\tPlatform Type: %s\n", decodeSpidHwType( base_board_value.platformInfo.platformType));
    fbe_debug_trace_func(NULL, "\tHardwareName: %s\n", base_board_value.platformInfo.hardwareName);
    fbe_debug_trace_func(NULL, "\tUnique Type: %s\n", decodeFamilyFruID(base_board_value.platformInfo.uniqueType));
    fbe_debug_trace_func(NULL, "\tCPUModule: %s\n", decodeCPUModule(base_board_value.platformInfo.cpuModule));
    fbe_debug_trace_func(NULL, "\tEnclosure Type: %s\n", decodeEnclosureType(base_board_value.platformInfo.enclosureType));
    fbe_debug_trace_func(NULL, "\tMidplane Type: %s\n", decodeMidplaneType(base_board_value.platformInfo.midplaneType));
	
    return;
}
/**********************************************
*   end of fbe_armada_base_board_debug_prvt_data
**********************************************/
