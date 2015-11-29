/***************************************************************************
 *  Copyright (C)  EMC Corporation 2012
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_dest_commands.c 
 ***************************************************************************
 *
 * @brief
 *  Debugging extensions for NewNeitPackage containing dest command.
 *
 * @revision
 *   07/17/2012:  Armando Vega - created
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_class.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_dest.h"
#include "stddef.h"

#define DEST_MAX_STR_LEN_DEBUG 64

typedef struct fbe_queue_element_debug_s {
	FBE_ALIGN(8)fbe_dbgext_ptr next; 
	FBE_ALIGN(8)fbe_dbgext_ptr prev; 
}fbe_queue_element_debug_t;

fbe_u8_t *module_name;

/*!**************************************************************
 * fbe_queue_front_debug()
 ****************************************************************
 * @brief
 *  Returns the first element on the queue.
 *
 * @param head - Pointer to head of queue.               
 *
 * @return - First element on the queue.
 *
 * @author
 *  07/23/2012 - Created. Armando Vega
 *
 ****************************************************************/
fbe_dbgext_ptr fbe_queue_front_debug(fbe_dbgext_ptr head)
{
    fbe_u32_t offset;
    fbe_u64_t field_p = 0;
    fbe_dbgext_ptr next_ptr;

	if(head == 0){
		return 0;
    } else {
		/* Return the pointer to the next element */       
        FBE_GET_FIELD_OFFSET(module_name, fbe_queue_element_t, "next", &offset);         
        field_p = head + offset;
        FBE_READ_MEMORY(field_p, &next_ptr, sizeof(fbe_dbgext_ptr));
        return next_ptr;
	}
}
/******************************************
 * end fbe_queue_front_debug()
 ******************************************/

/*!**************************************************************
 * fbe_queue_next_debug()
 ****************************************************************
 * @brief
 *  Returns the next element on the queue.
 *
 * @param head - Pointer to head of queue.   
 * @param element - Current element iteration.  
 *
 * @return - Next element on the queue or 0 if end has been reached.
 *
 * @author
 *  07/23/2012 - Created. Armando Vega
 *
 ****************************************************************/
fbe_dbgext_ptr fbe_queue_next_debug(fbe_dbgext_ptr head, fbe_dbgext_ptr element)
{
    fbe_u32_t offset;
    fbe_u64_t field_p = 0;
    fbe_dbgext_ptr next_ptr;
	
    if(head == 0){        
		return 0;
	}
    
    offset = offsetof(fbe_queue_element_debug_t, next);

    field_p = element + offset;
    
    FBE_READ_MEMORY(field_p, &next_ptr, sizeof(fbe_dbgext_ptr));	    

	if(next_ptr == head){        
		return 0;
	}
	if(next_ptr == element){        
		return 0;
	}
	return (next_ptr);
}
/******************************************
 * end queue_next_debug()
 ******************************************/

/*!**************************************************************
 * fbe_display_dest_info_debug()
 ****************************************************************
 * @brief
 *  Returns the next element on the queue.
 *
 * @param head - Pointer to head of queue.   
 * @param element - Current element iteration.  
 *
 * @return - Next element on the queue or 0 if end has been reached.
 *
 * @author
 *  07/23/2012 - Created. Armando Vega
 *
 ****************************************************************/
void fbe_display_dest_info_debug(fbe_dbgext_ptr element)
{
    fbe_u32_t offset = 0;
    fbe_u64_t start_p = 0;
    fbe_dest_error_record_t error_record_data;
    fbe_u32_t record_offset;
    fbe_u64_t record_field_p = 0;
    fbe_char_t error_type_str[DEST_MAX_STR_LEN_DEBUG]; 

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_element_t, "dest_error_record", &offset);        
  
    start_p = element + offset;
        
    csx_dbg_ext_print("--------------------------------------------------\n");     

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "object_id", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.object_id, sizeof(fbe_object_id_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "bus", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.bus, sizeof(fbe_u32_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "enclosure", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.enclosure, sizeof(fbe_u32_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "slot", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.slot, sizeof(fbe_u32_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "lba_start", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.lba_start, sizeof(fbe_lba_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "lba_end", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.lba_end, sizeof(fbe_lba_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "opcode", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.opcode, DEST_MAX_STR_LEN_DEBUG); 

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "dest_error_type", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.dest_error_type, sizeof(fbe_dest_error_type_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_dest_error_record_t, "dest_error_type", &record_offset);        
    record_field_p = start_p + record_offset;
    FBE_READ_MEMORY(record_field_p, &error_record_data.dest_error_type, sizeof(fbe_dest_error_type_t));

    switch (error_record_data.dest_error_type)
    {
	    case FBE_DEST_ERROR_TYPE_INVALID:
            strncpy(error_type_str, "invalid", DEST_MAX_STR_LEN_DEBUG);
            break;
	    case FBE_DEST_ERROR_TYPE_NONE:
		    strncpy(error_type_str, "none", DEST_MAX_STR_LEN_DEBUG);
		    break;
	    case FBE_DEST_ERROR_TYPE_PORT:               
		    strncpy(error_type_str, "port", DEST_MAX_STR_LEN_DEBUG);            
		    break;
	    case FBE_DEST_ERROR_TYPE_SCSI:
            strncpy(error_type_str, "scsi", DEST_MAX_STR_LEN_DEBUG);                       
		    break;
	    case FBE_DEST_ERROR_TYPE_GLITCH:
            strncpy(error_type_str, "glitch", DEST_MAX_STR_LEN_DEBUG);
		    break;
	    case FBE_DEST_ERROR_TYPE_FAIL:
		    strncpy(error_type_str, "fail", DEST_MAX_STR_LEN_DEBUG);
		    break;
	    default:
		    strncpy(error_type_str, "unknown type (%d)", (int)error_record_data.dest_error_type);
		    break;
    }

    csx_dbg_ext_print("Object id:                 0x%x\n", error_record_data.object_id);
    csx_dbg_ext_print("Drive:                     %u_%u_%u\n", 
        error_record_data.bus, error_record_data.enclosure, error_record_data.slot);
    csx_dbg_ext_print("LBA:                       [0x%llX .. 0x%llX]\n",
        (unsigned long long)error_record_data.lba_start,(unsigned long long)error_record_data.lba_end);
    csx_dbg_ext_print("Opcode:                    %s\n", error_record_data.opcode);       
    csx_dbg_ext_print("Error Type:                %s\n", error_type_str);  
    csx_dbg_ext_print("--------------------------------------------------\n"); 
}
/******************************************
 * end fbe_display_dest_info_debug()
 ******************************************/

#pragma data_seg ("EXT_HELP$4dest")
static char CSX_MAYBE_UNUSED usageMsg_neitdest[] =
"!dest\n"
"  Display drive error injection information.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(dest, "dest")
{
    fbe_dbgext_ptr dest_error_queue_ptr = 0;    
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr dest_error_element;     
    fbe_dbgext_ptr dest_record_handle = 0; 

    /* We either are in simulation or on hardware.
     * Depending on where we are, use a different module name. 
     * We validate the module name by checking the ptr size. 
     */
    fbe_debug_get_ptr_size("NewNeitPackage", &ptr_size);
            
    module_name = (ptr_size == 0) ? "new_neit" : "NewNeitPackage";    

    /* Get the error queue ptr. */
    FBE_GET_EXPRESSION(module_name, dest_error_queue, &dest_error_queue_ptr);
    
    if(dest_error_queue_ptr == 0)
    {
        csx_dbg_ext_print("dest_error_queue pointer is not available\n");
		return;
    }
        
    dest_error_element = fbe_queue_front_debug(dest_error_queue_ptr);    
    
    if(dest_error_element != 0)
    {
        fbe_display_dest_info_debug(dest_error_element);        
    }

    dest_record_handle = dest_error_element;

    while (dest_error_element != 0)
    {
        dest_error_element = fbe_queue_next_debug(dest_error_queue_ptr, dest_record_handle);
        
        if(dest_error_element != 0)
        {
            fbe_display_dest_info_debug(dest_error_element);
        }

    	dest_record_handle = dest_error_element;
    }

    return;
}
