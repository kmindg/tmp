/***************************************************************************
 *  Copyright (C)  EMC Corporation 2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file database_service.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands for the database service.
.*
 * @version
 *   01-Aug-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_devices.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_database.h"
#include "fbe_database_debug.h"
#include "fbe_private_space_layout.h"
#include "fbe_sepls_debug.h"

fbe_object_id_t fbe_debug_get_object_id(void)
{
	fbe_object_id_t object_id;
	fbe_char_t* str = NULL;

	str = strtok(NULL, " \t");
    if(str != NULL)
	{
		object_id = strtoul(str, 0, 0);
	}
    else
    {
		fbe_debug_trace_func(NULL, "Please provide Object ID\n");
		return FBE_OBJECT_ID_INVALID;
    }
    if(object_id >= FBE_MAX_SEP_OBJECTS)
    {
		csx_dbg_ext_print("Incorrect object Id\n");
        return FBE_OBJECT_ID_INVALID;
    }
	return object_id;
}

/* ***************************************************************************
 *
 * @brief
 *  Macro to display the database service tables.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   03-Aug-2011:  Created. Omer Miranda
 *
 ***************************************************************************/
#pragma data_seg ("EXT_HELP$4database_service")
static char CSX_MAYBE_UNUSED usageMsg_database_service[] =
"!database_service\n"
"   Display information of the various database config service tables\n"
"   Usage !database_service -table <o/e/u/g> [-obj <obj_id>]\n"
"   where -table  option displays information about one of the below tables\n"
"          o: object_table\n"
"          e: edge_table\n"
"          u: user_table\n"
"          g: global info table\n"
"          -obj <objectID> Displays information regarding specified object\n"
"   e.g. !database_service -table o -obj 0xa \n"
"   will display the object 0xa's object table information\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(database_service, "database_service")
{
    fbe_char_t* str = NULL;
    fbe_dbgext_ptr database_service_ptr = 0;
    fbe_u32_t display_format = FBE_DATABASE_DISPLAY_TABLE_NONE;
    fbe_object_id_t object_id = FBE_OBJECT_ID_INVALID;

   
    str = strtok((char*)args," \t");
    if(str != NULL)
    {
        if(strncmp(str, "-table", 6) == 0)
        {
            str = strtok(NULL," \t");
            if(str != NULL)
            {
				/* Object table */
                if(strncmp(str, "o", 1) == 0)
                {
                    str = strtok(NULL," \t");
                    if(str != NULL && strncmp(str, "-obj", 4) == 0)
                    {
						object_id = fbe_debug_get_object_id();
						/* Check if we have got a valid object id*/
						if(object_id == FBE_OBJECT_ID_INVALID)
						{
							return;
						}
                    }
					/* If the user does not want to specify an object,
					 * then set the object-id to invalid
					 */
                    else
                    {
                        object_id = FBE_OBJECT_ID_INVALID;
                    }
                    fbe_debug_trace_func(NULL, "DATABASE_CONFIG_TABLE_OBJECT\n" );
                    display_format = FBE_DATABASE_DISPLAY_OBJECT_TABLE;		
                }
				/* Edge table */
                else if(strncmp(str, "e", 1) == 0)
                {
                    str = strtok(NULL," \t");
                    if(str != NULL && strncmp(str, "-obj", 4) == 0)
                    {
						object_id = fbe_debug_get_object_id();
						if(object_id == FBE_OBJECT_ID_INVALID)
						{
							return;
						}
                    }
					/* If the user does not want to specify an object,
					 * then set the object-id to invalid
					 */
                    else
                    {
                        object_id = FBE_OBJECT_ID_INVALID;
                    }
                    fbe_debug_trace_func(NULL, "DATABASE_CONFIG_TABLE_EDGE\n" );
                    display_format = FBE_DATABASE_DISPLAY_EDGE_TABLE;		  
                }
				/* User table */
                else if(strncmp(str, "u", 1) == 0)
                {
                    str = strtok(NULL," \t");
                    if(str != NULL && strncmp(str, "-obj", 4) == 0)
                    {
						object_id = fbe_debug_get_object_id();
						if(object_id == FBE_OBJECT_ID_INVALID)
						{
							return;
						}
                    }
					/* If the user does not want to specify an object,
					 * then set the object-id to invalid
					 */
                    else
                    {
                        object_id = FBE_OBJECT_ID_INVALID;
                    }
                    fbe_debug_trace_func(NULL, "DATABASE_CONFIG_TABLE_USER\n" );
                    display_format = FBE_DATABASE_DISPLAY_USER_TABLE;
                }
				/* Global Info */
                else if(strncmp(str, "g", 1) == 0)
                {
                    str = strtok(NULL," \t");
                    if(str != NULL && strncmp(str, "-obj", 4) == 0)
                    {
						object_id = fbe_debug_get_object_id();
						if(object_id == FBE_OBJECT_ID_INVALID)
						{
							return;
						}
                    }
					/* If the user does not want to specify an object,
					 * then set the object-id to invalid
					 */
                    else
                    {
                        object_id = FBE_OBJECT_ID_INVALID;
                    }
                    fbe_debug_trace_func(NULL, "DATABASE_CONFIG_TABLE_GLOBAL_INFO\n" );
                    display_format = FBE_DATABASE_DISPLAY_GLOBAL_INFO_TABLE;
                }
                else
                {
                    fbe_debug_trace_func(NULL, "Please provide proper table type option \n");
                    return;  
                }
            }
            else
            {
                fbe_debug_trace_func(NULL, "Please provide table type \n");
                return;  
            }
        }
        else
        {
            fbe_debug_trace_func(NULL, "Please type -table string properly \n");
        }
    }
    else
    {
        fbe_debug_trace_func(NULL, "Please type proper command with option like -\n");
        fbe_debug_trace_func(NULL, "!database_service -table o For object table\n");
        return;
    }
    
    if(!strcmp(pp_ext_module_name, "sep"))
    {
        /* Get the database config service.ptr */
        FBE_GET_EXPRESSION(pp_ext_module_name, fbe_database_service, &database_service_ptr);
        if(database_service_ptr == 0)
        {
            fbe_debug_trace_func(NULL, "database_service_ptr is not available");
            return;
        }
        fbe_database_service_debug_trace(pp_ext_module_name, fbe_debug_trace_func, NULL, database_service_ptr, display_format, object_id);
        fbe_debug_trace_func(NULL, "\n");		
    }
    else
    {
        fbe_debug_trace_func(NULL, "Please set_module_name_sep \n");
    }
    return;
    
};

#pragma data_seg ("EXT_HELP$4database_dump")
static char CSX_MAYBE_UNUSED usageMsg_database_dump[] =
"!database_dump\n"
"   dump information of the various database tables\n"
"   Usage !database_dump\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(database_dump, "database_dump")
{
    if(!fbe_sep_offsets.is_fbe_sep_offsets_set)
    {
        fbe_sep_offsets_set("sep");
    }
    
	fbe_database_service_debug_dump_all("sep",fbe_debug_trace_func,NULL);

    fbe_sep_offsets_clear();
    return;
}

#pragma data_seg ("EXT_HELP$4lun_user_data")
static char CSX_MAYBE_UNUSED usageMsg_lun_user_data[] = 
"!lun_user_data\n"
"   Dump the user information of the LUN\n"
"   Usage !lun_user_data [-lun lun_number]\n";
#pragma data_seg ()


CSX_DBG_EXT_DEFINE_CMD(lun_user_data, "lun_user_data")
{
    fbe_char_t *str = NULL;
    fbe_assigned_wwid_t wwn;
    fbe_u32_t type;
    fbe_user_defined_lun_name_t name;
    fbe_bool_t is_exist;
    fbe_u32_t   wwn_index;
    fbe_u32_t   wwn_size;
    fbe_u32_t cur_object_id = 0;
    fbe_object_id_t lun_obj_id = 0;
    fbe_lun_number_t lun_number = 0;
    fbe_u32_t max_object_count = 0;
    fbe_bool_t is_lun_or_not = FBE_FALSE;
    fbe_bool_t status;

    max_object_count = fbe_database_get_total_object_count();

    str = strtok((char *)args, " \t");
    if (str != NULL) {
        if (strncmp(str, "-lun", 4) == 0 &&
            (str = strtok(NULL, " \t")) != NULL)
        {
            lun_number = strtoul(str, 0, 0);
            fbe_database_get_lun_config_debug(FBE_OBJECT_ID_INVALID, &lun_number, 
                                              (fbe_assigned_wwid_t *) &wwn,
                                              &type,
                                              &name,
                                              &status,
                                              &is_exist);
            if (!is_exist)
            {
                fbe_debug_trace_func(NULL, "lun %lu does not exist:\t%lu\n", lun_number);
                return;
            }

            fbe_debug_trace_func(NULL, "lun :\t%lu\n", lun_number);
            fbe_debug_trace_func(NULL, "lun type is:\t%s (%d)\n", fbe_database_get_lun_type_string(type), type);
            fbe_debug_trace_func(NULL, "user defined name:\t%s\n", name.name);
            fbe_debug_trace_func(NULL, "World wide name:\t");
            wwn_size = sizeof(wwn);
            for (wwn_index = 0;  wwn_index < wwn_size; wwn_index++) 
            {
                fbe_debug_trace_func(NULL, "%02x", (fbe_u32_t)wwn.bytes[wwn_index]);
                if (wwn_index != wwn_size - 1)
                {
                    fbe_debug_trace_func(NULL, ":");
                }
            }

            fbe_debug_trace_func(NULL, "\n\n");

            return;
        }
        else
        {
            fbe_debug_trace_func(NULL, usageMsg_lun_user_data);
            return;
        }
    }

    for (cur_object_id = 0; cur_object_id < max_object_count; cur_object_id++)
    {
        is_lun_or_not = fbe_database_is_object_id_user_lun(cur_object_id);

        if(is_lun_or_not == FBE_TRUE)
        {
            lun_obj_id = (fbe_object_id_t)cur_object_id;
            fbe_database_get_lun_config_debug(lun_obj_id, &lun_number, 
                                              (fbe_assigned_wwid_t *) &wwn,
                                              &type,
                                              &name,
                                              &status,
                                              &is_exist);
            if (!is_exist)
            {
                continue;
            }

            fbe_debug_trace_func(NULL, "lun :\t%lu\n", lun_number);
            fbe_debug_trace_func(NULL, "lun type is:\t%s (%d)\n", fbe_database_get_lun_type_string(type), type);
            fbe_debug_trace_func(NULL, "user defined name:\t%s\n", name.name);
            fbe_debug_trace_func(NULL, "World wide name:\t");
            wwn_size = sizeof(wwn);
            for (wwn_index = 0;  wwn_index < wwn_size; wwn_index++) 
            {
                fbe_debug_trace_func(NULL, "%02x", (fbe_u32_t)wwn.bytes[wwn_index]);
                if (wwn_index != wwn_size - 1)
                {
                    fbe_debug_trace_func(NULL, ":");
                }
            }
            fbe_debug_trace_func(NULL, "\n\n");

            if (FBE_CHECK_CONTROL_C())
            {
                break;
            }
        }
    }
}



#pragma data_seg ("EXT_HELP$4lun_max_number")
static char CSX_MAYBE_UNUSED usageMsg_lun_max_number[] = 
"!lun_max_number\n"
"   Dump the max number of lun count\n"
"   Usage !lun_max_number\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(lun_max_number, "lun_max_number")
{
    fbe_debug_trace_func(NULL, "Max number of luns is:\t%lu\n", fbe_database_get_max_number_of_lun());
    fbe_debug_trace_func(NULL, "\n");
}

//end of database_service.c
