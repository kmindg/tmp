/******************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *****************************************************************************/

/*!****************************************************************************
 * @file    pp_drive_mgmt_config_ext.c
 *
 * @brief   This module implements the dmo macro.
 *
 * @author
 *          @7/26/2012 Michael Li -- Created.
 *
 *****************************************************************************/
#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_base_enclosure_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_drive_mgmt_private.h" 
#include "fbe_topology_debug.h"
#include "fbe_drive_mgmt_debug.h" 


#define P_DMO_MACRO_CMD_MAX_LEN          32     

/*Macro to verify the valid board class */
#define IS_VALID_DRIVE_MGMT_CLASS(class_id) (class_id == FBE_CLASS_ID_DRIVE_MGMT)

#pragma data_seg ("EXT_HELP$4dmo")
static char usageMsg_dmo[] =
"!dmo\n"
"  Show drive management object information.\n\n"
"  Usage:dmo \n\
          -help                                    - For this help information.\n\
          [-b bus] [-e encl] [-s slot] -drive_info - Display all or specified drive info.\n\
          -dieh_display                            - Display DIEH configuration. \n\
          -get_crc_actions                         - Get the logical crc error actions. Optional b-e-s can be provided.\n\n";

#pragma data_seg ()

#define MODULE_NAME_FOR_DMO               "espkg"                                        

 /*!****************************************************************************
 * @fn  dmo
 *
 * @brief
 *      Entry of dmo macro.
 *
 * @param 
 *
 * @return
 *
 * @author
 *          @7/26/2012 Michael Li -- Created.
 *
 *****************************************************************************/
CSX_DBG_EXT_DEFINE_CMD(dmo, "dmo")
{
    fbe_u32_t               ptr_size = 0;
    fbe_status_t            status   = FBE_STATUS_OK;
    fbe_char_t*             str      = NULL;
    fbe_dbgext_ptr          topology_object_table_ptr   = 0;
    fbe_dbgext_ptr          topology_object_tbl_ptr     = 0;
    ULONG                   topology_object_table_entry_size;
    fbe_u32_t               port_index = 0;
    fbe_u32_t               encl_index = 0;
    fbe_u32_t               slot_index = 0;
    fbe_bool_t              port_filter = FBE_FALSE, encl_filter = FBE_FALSE, slot_filter = FBE_FALSE; 
    fbe_bool_t              drive_info_filter       = FBE_FALSE; 
    fbe_bool_t              dieh_get_status_filter  = FBE_FALSE;
    fbe_bool_t              dieh_display_filter     = FBE_FALSE;
    fbe_bool_t              get_crc_actions_filter  = FBE_FALSE;
    fbe_dbgext_ptr          control_handle_ptr      = 0;
    fbe_dbgext_ptr          drive_info_ptr          = 0;
    ULONG                   drive_info_struct_size;
    fbe_u32_t               max_num_drives          = 0;
    fbe_u32_t               i = 0;
    fbe_class_id_t          class_id;
	ULONG64                 max_toplogy_entries_p   = 0;
	fbe_u32_t               max_entries;
    fbe_topology_object_status_t  object_status;
    fbe_object_id_t         object_id;
    fbe_drive_info_t        a_drive_info;
    
    str = strtok((char*)args, " \t");
    
    if(str == NULL)
    {
        //display drive info for all drive if no argument specified
        drive_info_filter   = FBE_TRUE;
        port_filter         = FBE_FALSE;
        encl_filter         = FBE_FALSE;
        slot_filter         = FBE_FALSE;
    }
    else
    {  
        while (str != NULL)
        {
            if((strncmp(str, "-help", 5) == 0 || (strncmp(str, "-h", 2) == 0)))
            {
                csx_dbg_ext_print("%s\n",usageMsg_dmo);
                return;
            }
            if(strncmp(str, "-b", 2) == 0 || strncmp(str, "-p", 2) == 0)
            {
                str = strtok (NULL, " \t");
                port_index= (fbe_u32_t)strtoul(str, 0, 0);
                port_filter = FBE_TRUE;
                //automatically assume "drive_info" if port is specified
                drive_info_filter = FBE_TRUE;
            }
            else if(strncmp(str, "-e", 2) == 0)
            {
                str = strtok (NULL, " \t");
                encl_index = (fbe_u32_t)strtoul(str, 0, 0);
                encl_filter = FBE_TRUE;
                //automatically assume "drive_info" if encl is specified
                drive_info_filter = FBE_TRUE;
            }
            else if(strncmp(str, "-s", 2) == 0)
            {
                str = strtok (NULL, " \t");
                slot_index = (fbe_u32_t)strtoul(str, 0, 0);
                slot_filter = FBE_TRUE;
                //automatically assume "drive_info" if slot is specified
                drive_info_filter = FBE_TRUE;
            }
            else if(strncmp(str, "-drive_info", strlen("-drive_info")) == 0)
            {
                drive_info_filter = FBE_TRUE;
            }
            else if(strncmp(str, "-dieh_get_stats", strlen("-dieh_get_stats")) == 0)
            {
                dieh_get_status_filter = FBE_TRUE;
            }
            else if(strncmp(str, "-dieh_display", strlen("-dieh_display")) == 0)
            {
                dieh_display_filter = FBE_TRUE;
            }
            else if(strncmp(str, "-get_crc_actions", strlen("-get_crc_actions")) == 0)
            {
                get_crc_actions_filter = FBE_TRUE;
            }
            else
            {
                csx_dbg_ext_print("%s Invalid input. Please try again.\n", __FUNCTION__);
                return;
            }
            //advance to the next field
            str = strtok (NULL, " \t");
        } //while (str != NULL)
    }

    status = fbe_debug_get_ptr_size(MODULE_NAME_FOR_DMO, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return;
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(MODULE_NAME_FOR_DMO, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(MODULE_NAME_FOR_DMO, topology_object_table_entry_s, &topology_object_table_entry_size);
    fbe_debug_trace_func(NULL, "\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    FBE_GET_EXPRESSION(MODULE_NAME_FOR_DMO, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if(topology_object_tbl_ptr == 0){
        return;
    }

    csx_dbg_ext_print("%s max_entries: %d\n", __FUNCTION__, max_entries);
    
    object_status = 0;
    for(i = 0; i < max_entries ; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return;
        }

        /* Fetch the object's topology status. */
        FBE_GET_FIELD_DATA( MODULE_NAME_FOR_DMO,
                            topology_object_tbl_ptr,
                            topology_object_table_entry_s,
                            object_status,
                            sizeof(fbe_topology_object_status_t),
                            &object_status);

        
        if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            /* Fetch the control handle, which is the pointer to the object. */
            FBE_GET_FIELD_DATA( MODULE_NAME_FOR_DMO,
                                topology_object_tbl_ptr,
                                topology_object_table_entry_s,
                                control_handle,
                                sizeof(fbe_dbgext_ptr),
                                &control_handle_ptr);

            
            FBE_GET_FIELD_DATA( MODULE_NAME_FOR_DMO, 
                                control_handle_ptr,
                                fbe_base_object_s,
                                class_id,
                                sizeof(fbe_class_id_t),
                                &class_id);
                                
            
            //Do we need to check object_status ???
            if(IS_VALID_DRIVE_MGMT_CLASS(class_id))
            {
                //Drive management object
                //Fetch drive_info array pointer from fbe_drive_mgmt_t
                FBE_GET_FIELD_DATA( MODULE_NAME_FOR_DMO,
                                    control_handle_ptr,
                                    fbe_drive_mgmt_t,                   
                                    drive_info_array,
                                    sizeof(fbe_dbgext_ptr),
                                    &drive_info_ptr);
           
               
                FBE_GET_FIELD_DATA( MODULE_NAME_FOR_DMO,
                                    control_handle_ptr,
                                    fbe_drive_mgmt_t,
                                    max_supported_drives,                   
                                    sizeof(fbe_u32_t),
                                    &max_num_drives);
                
                //Get the size of fbe_drive_info_t structure                    
                FBE_GET_TYPE_SIZE(MODULE_NAME_FOR_DMO, fbe_drive_info_t, &drive_info_struct_size);
               
                //Check max_num_drives to make sure it is a reasonable value. Here we use FBE_MAX_OBJECTS as the
                //upper limit. (Is there any way to get more accurate number of actual drives in the array?)
                if(max_num_drives > FBE_MAX_OBJECTS)
                {
                    csx_dbg_ext_print("%s invalid max number of drives: %d\n", __FUNCTION__, max_num_drives);
                    return;
                }
                
                for (i = 0; i < max_num_drives; i++)
                {                
                    
                    FBE_GET_FIELD_DATA( MODULE_NAME_FOR_DMO,
                                        drive_info_ptr,
                                        fbe_drive_info_t,
                                        object_id, 
                                        sizeof(fbe_object_id_t),
                                        &object_id);                    
                                        
                    FBE_READ_MEMORY(drive_info_ptr, &a_drive_info, drive_info_struct_size);

                    if(object_id != FBE_OBJECT_ID_INVALID)
                    {

                        if(drive_info_filter)
                        {
                            if(!((port_filter && a_drive_info.location.bus != port_index) ||
                                 (encl_filter && a_drive_info.location.enclosure != encl_index) ||
                                 (slot_filter && a_drive_info.location.slot != slot_index))) 
                            { 
                                //drive info found, print it out
                                status = fbe_drive_mgmt_drive_info_debug_trace(MODULE_NAME_FOR_DMO,
                                                                              (fbe_dbgext_ptr)&a_drive_info,
                                                                               fbe_debug_trace_func,
                                                                               NULL);
                                if (status != FBE_STATUS_OK) 
                                {
                                    csx_dbg_ext_print("%s failed getting drive info. status: %d, i:%d, object id:%d\n", __FUNCTION__, status, i, a_drive_info.object_id);
                                }
                            }
                        }
                        else
                        if(dieh_display_filter)
                        {
                            //display dieh information
                            csx_dbg_ext_print("%s >>>> dieh_display <<<<<\n", __FUNCTION__);

                #if 0
                        //need to implement this !!!
                            status = fbe_dieh_display_debug_trace(MODULE_NAME_FOR_DMO,
                                                                  fbe_debug_trace_func,
                                                                  NULL);
                            if (status != FBE_STATUS_OK) 
                            {
                                csx_dbg_ext_print("%s failed at dieh display. status: %d\n", __FUNCTION__, status);
                            }
                #endif 
                        }
                        else if(get_crc_actions_filter)
                        {
                            //display drive crc error actions information
                            csx_dbg_ext_print("%s >>>> get_crc_actions <<<<<\n", __FUNCTION__);
                        }
                        else
                        {
                            //invalid command
                            csx_dbg_ext_print("%s invalid comand\n", __FUNCTION__);
							return;
                        }
                    }   // if(a_drive_info.object_id != FBE_OBJECT_ID_INVALID)
                    drive_info_ptr += drive_info_struct_size;
                }       //for (i = 0; i < max_num_drives; i++)
                
                fbe_debug_trace_func(NULL, "\n");
            }   //if(IS_VALID_DRIVE_MGMT_CLASS(class_id)
            topology_object_tbl_ptr += topology_object_table_entry_size;      
        }   // if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
    } // for(i = 0; i < max_entries ; i++)
    return;
}
