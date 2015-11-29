/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-07
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * pp_enclosure_commands.c
 ***************************************************************************
 *
 * File Description:
 *
 *Debugging enclosure specific commands for PhysicalPackage driver.
 *
 * Author:
 *  Dipak Patel  
 *
 ***************************************************************************/
#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_base_enclosure_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_sas_viper_enclosure_debug.h"
#include "fbe_sas_magnum_enclosure_debug.h"
#include "fbe_sas_citadel_enclosure_debug.h"
#include "fbe_sas_bunker_enclosure_debug.h"
#include "fbe_sas_derringer_enclosure_debug.h"
#include "fbe_sas_voyagericm_enclosure_debug.h"
#include "fbe_sas_voyageree_enclosure_debug.h"
#include "fbe_sas_viking_iosxp_enclosure_debug.h"
#include "fbe_sas_viking_drvsxp_enclosure_debug.h"
#include "fbe_sas_cayenne_iosxp_enclosure_debug.h"
#include "fbe_sas_cayenne_drvsxp_enclosure_debug.h"
#include "fbe_sas_naga_iosxp_enclosure_debug.h"
#include "fbe_sas_naga_drvsxp_enclosure_debug.h"
#include "fbe_sas_fallback_enclosure_debug.h"
#include "fbe_sas_boxwood_enclosure_debug.h"
#include "fbe_sas_knot_enclosure_debug.h"
#include "fbe_sas_pinecone_enclosure_debug.h"
#include "fbe_sas_steeljaw_enclosure_debug.h"
#include "fbe_sas_ramhorn_enclosure_debug.h"
#include "fbe_sas_ancho_enclosure_debug.h"
#include "fbe_sas_calypso_enclosure_debug.h"
#include "fbe_sas_rhea_enclosure_debug.h"
#include "fbe_sas_miranda_enclosure_debug.h"
#include "fbe_sas_tabasco_enclosure_debug.h"
#include "EmcPAL_Memory.h"
#include "fbe/fbe_board_types.h"
#include "fbe/fbe_board.h"
#include "fbe_topology_debug.h"

/*!**************************************************************
 * @fn fbe_status_t fbe_enclosure_get_obj_handle_and_class(pp_ext_physical_package_name,
 *                                  fbe_dbgext_ptr topology_object_table_ptr,
 *                                  fbe_dbgext_ptr control_handle_ptr,
 *                                  fbe_class_id_t *class_id)
 ****************************************************************
 * @brief
 *  Fetch the object, control handle
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param control_handle_ptr - Ptr to  object.
 * @param class_id -encl class id
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  4-Aug-2009 - Created. Prasenjeet Ghaywat
 *
 ****************************************************************/
fbe_status_t fbe_enclosure_get_obj_handle_and_class(const fbe_u8_t * module_name,
                                          fbe_dbgext_ptr topology_object_table_ptr,
                                          fbe_dbgext_ptr *control_handle_ptr, 
                                          fbe_class_id_t *class_id)
{
    fbe_topology_object_status_t object_status;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size = 0;

    *control_handle_ptr = NULL;
    *class_id = FBE_CLASS_ID_INVALID;

    status = fbe_debug_get_ptr_size(pp_ext_physical_package_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status; 
    }
    /* Fetch the object's topology status. */
    FBE_GET_FIELD_DATA(module_name,
                    topology_object_table_ptr,
                    topology_object_table_entry_s,
                    object_status,
                    sizeof(fbe_topology_object_status_t),
                    &object_status);

    if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
    {
        /* Fetch the control handle, which is the pointer to the object. */
        FBE_GET_FIELD_DATA(module_name,
                            topology_object_table_ptr,
                            topology_object_table_entry_s,
                            control_handle,
                            ptr_size,
                            control_handle_ptr);
        FBE_GET_FIELD_DATA(module_name, 
                        *control_handle_ptr,
                        fbe_base_object_s,
                        class_id,
                        sizeof(fbe_class_id_t),
                        class_id);
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn  fbe_status_t fbe_enclosure_print_debug_prvt_data(const fbe_u8_t * module_name,
 *                                             fbe_dbgext_ptr control_handle_ptr, 
 *                                             fbe_class_id_t class_id,int obj_id)
 ****************************************************************
 * @brief
 *  Filter the encl by class id ans forward the data for printing
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param control_handle_ptr - Ptr to object.
 * @param class_id -encl class id
 * @param int -obj_id
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  4-Aug-2009 - Created. Prasenjeet Ghaywat
 *
 ****************************************************************/
 fbe_status_t fbe_enclosure_print_debug_prvt_data(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr control_handle_ptr, 
                                        fbe_class_id_t class_id,int obj_id)
{
    switch (class_id)
    {
        case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
             csx_dbg_ext_print("SAS VIPER ENCLOSURE: %d\n",obj_id);                     
             fbe_sas_viper_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
             csx_dbg_ext_print("SAS MAGNUM ENCLOSURE: %d\n",obj_id);
             fbe_sas_magnum_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE:
             csx_dbg_ext_print("SAS DERRINGER ENCLOSURE: %d\n",obj_id);
             fbe_sas_derringer_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
             csx_dbg_ext_print("SAS CITADEL ENCLOSURE: %d\n",obj_id);
             fbe_sas_citadel_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
             csx_dbg_ext_print("SAS BUNKER ENCLOSURE: %d\n",obj_id);
             fbe_sas_bunker_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE:
             csx_dbg_ext_print("SAS VOYAGER ICM ENCLOSURE: %d\n",obj_id);
             fbe_sas_voyager_icm_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_VOYAGER_EE_LCC:
             csx_dbg_ext_print("SAS VOYAGER EE ENCLOSURE: %d\n",obj_id);
             fbe_sas_voyager_ee_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE:
             csx_dbg_ext_print("SAS VIKING IOSXP ENCLOSURE: %d\n",obj_id);
             fbe_sas_viking_iosxp_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC:
             csx_dbg_ext_print("SAS VIKING DRVSXP ENCLOSURE: %d\n",obj_id);
             fbe_sas_viking_drvsxp_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE:
             csx_dbg_ext_print("SAS CAYENNE IOSXP ENCLOSURE: %d\n",obj_id);
             fbe_sas_cayenne_iosxp_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC:
             csx_dbg_ext_print("SAS CAYENNE DRVSXP ENCLOSURE: %d\n",obj_id);
             fbe_sas_cayenne_drvsxp_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE:
             csx_dbg_ext_print("SAS NAGA IOSXP ENCLOSURE: %d\n",obj_id);
             fbe_sas_naga_iosxp_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC:
             csx_dbg_ext_print("SAS NAGA DRVSXP ENCLOSURE: %d\n",obj_id);
             fbe_sas_naga_drvsxp_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
             csx_dbg_ext_print("SAS FALLBACK ENCLOSURE: %d\n",obj_id);
             fbe_sas_fallback_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
             csx_dbg_ext_print("SAS BOXWOOD ENCLOSURE: %d\n",obj_id);
             fbe_sas_boxwood_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
             csx_dbg_ext_print("SAS KNOT ENCLOSURE: %d\n",obj_id);
             fbe_sas_knot_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;		
        case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
             csx_dbg_ext_print("SAS ANCHO ENCLOSURE: %d\n",obj_id);                     
             fbe_sas_ancho_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE:
             csx_dbg_ext_print("SAS PINECONE ENCLOSURE: %d\n",obj_id);
             fbe_sas_pinecone_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE:
             csx_dbg_ext_print("SAS STEELJAW ENCLOSURE: %d\n",obj_id);
             fbe_sas_steeljaw_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE:
             csx_dbg_ext_print("SAS RAMHORN ENCLOSURE: %d\n",obj_id);
             fbe_sas_ramhorn_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
            csx_dbg_ext_print("SAS CALYPSO ENCLOSURE: %d\n",obj_id);
            fbe_sas_calypso_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
            break;
        case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
            csx_dbg_ext_print("SAS RHEA ENCLOSURE: %d\n",obj_id);
            fbe_sas_rhea_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
            break;
        case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
            csx_dbg_ext_print("SAS MIRANDA ENCLOSURE: %d\n",obj_id);
            fbe_sas_miranda_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
            break;
        case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE:
             csx_dbg_ext_print("SAS TABASCO ENCLOSURE: %d\n",obj_id);
             fbe_sas_tabasco_enclosure_debug_prvt_data(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
        break;
        default:
        break;
    } 
    return FBE_STATUS_OK;
}

//
//  Description: Extension to display enclosure private data.
//      
//
//  Arguments:
//  obj - object id if user wants to have data for specific object
//
//  Return Value:
//  NONE
//

#pragma data_seg ("EXT_HELP$4ppenclprvdata")
static char CSX_MAYBE_UNUSED usageMsg_ppenclprvdata[] =
"!ppenclprvdata\n"
"  By default display private Data for all enclosure.\n"
"  -obj <object id> displays just for this object.\n"
"  -b <port index> -e <encl. position> displays just for this port and encl.\n"
"  -b <port index> displays just for this port.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(ppenclprvdata, "ppenclprvdata")
{

    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    ULONG topology_object_table_entry_size;
    int i = 0,len=0;
    fbe_class_id_t class_id;
    fbe_u64_t obj_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t specific_object_id = FALSE;
    fbe_u32_t           port_index, port_index_tmp;
    fbe_u32_t           encl_pos,encl_pos_tmp;  
    fbe_bool_t  encl_filter = FALSE,port_filter = FALSE;
    PCHAR   str = NULL;
    CHAR    buffer[20] = {'\0'};
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_debug_get_ptr_size(pp_ext_physical_package_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    len = (int)strlen(args);
    if (len>18)
    {
        len = 18;  // we need to leave a space to terminator the string.
    }
    memcpy( buffer, args, len + 1 );
    

	FBE_GET_EXPRESSION(pp_ext_physical_package_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if (len)
    {
       if(strncmp(buffer, "-obj", 4) == 0)
       {
            csx_dbg_ext_print("filter by object id: %d \n", (int)GetArgument64(args, 2));
            obj_id = GetArgument64(args, 2);

            if(obj_id >= max_entries)
            {
                csx_dbg_ext_print("Incorrect object Id\n");
                return;
            }
            specific_object_id = TRUE;
       }
       else
       {
         str = strtok( buffer, " \t" );
         while (str != NULL)
         {
           if(strncmp(str, "-b", 2) == 0)
           {
              str = strtok (NULL, " \t");
              port_index= (fbe_u32_t)strtoul(str, 0, 0);
              port_filter =  TRUE;
           }
           else if(strncmp(str, "-e", 2) == 0)
           {
              str = strtok (NULL, " \t");
              encl_pos = (fbe_u32_t)strtoul(str, 0, 0);
              encl_filter = TRUE;
           }
          str = strtok (NULL, " \t");
         }
       }
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(pp_ext_physical_package_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(pp_ext_physical_package_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    if(topology_object_tbl_ptr == 0){
        return;
    }

    if(specific_object_id == TRUE)
    {
        for(i=0; i<=obj_id; i++)
        {
            topology_object_tbl_ptr += topology_object_table_entry_size;
        }
        fbe_enclosure_get_obj_handle_and_class(pp_ext_physical_package_name,
                topology_object_tbl_ptr,&control_handle_ptr,&class_id);
        if (control_handle_ptr!=NULL)
        {
            fbe_enclosure_print_debug_prvt_data(pp_ext_physical_package_name,control_handle_ptr, class_id,i);
        }
    }
    else if(port_filter)
    {
        for(i = 0; i < max_entries && port_filter; i++)
        {
            fbe_enclosure_get_obj_handle_and_class(pp_ext_physical_package_name,
            topology_object_tbl_ptr,&control_handle_ptr,&class_id);

            if(class_id <= FBE_CLASS_ID_SAS_ENCLOSURE_LAST && class_id >= FBE_CLASS_ID_ENCLOSURE_FIRST)         
            {
                fbe_base_enclosure_debug_get_port_index(pp_ext_physical_package_name,control_handle_ptr, &port_index_tmp);
                if(port_index == port_index_tmp)
                {
                    if(!encl_filter)
                    {
                        fbe_enclosure_print_debug_prvt_data(pp_ext_physical_package_name,control_handle_ptr, class_id,i);
                    }
                    else
                    {
                        fbe_base_enclosure_debug_get_encl_pos(pp_ext_physical_package_name,control_handle_ptr, &encl_pos_tmp);
                        if(encl_pos == encl_pos_tmp)                            
                        {
                            fbe_enclosure_print_debug_prvt_data(pp_ext_physical_package_name,control_handle_ptr, class_id,i);
                            encl_filter = FALSE;
                            port_filter = FALSE;
                            break;
                        }
                    }
                }
            }
            topology_object_tbl_ptr += topology_object_table_entry_size; 
        }
    }
    else
    {
        for(i = 0; i < max_entries ; i++)
        {
            fbe_enclosure_get_obj_handle_and_class(pp_ext_physical_package_name,
                                topology_object_tbl_ptr,&control_handle_ptr,&class_id);

            if (control_handle_ptr!=NULL)
            {
                fbe_enclosure_print_debug_prvt_data(pp_ext_physical_package_name,control_handle_ptr, class_id,i);
            }
            topology_object_tbl_ptr += topology_object_table_entry_size; 
        }
    }
    return ;
}


#pragma data_seg ("EXT_HELP$4ppencledal")
static char usageMsg_ppencledal[] =
"!ppencledal \n"
"  Displays enclosure data.\n"
"  By default displays all enclosure data.\n"
"  -b <port index>   Displays edal for all enclosure on this port.\n"
"  -b <port index> -e <encl. position>  Displays edal for this port and encl.\n"
"  -b <port index> -e <encl. position> -cid <comp id> Displays edal for this port, encl and comp id(in case of voyager).\n";

#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(ppencledal, "ppencledal")
{
    int i = 0;
    fbe_class_id_t class_id;
    fbe_dbgext_ptr control_handle_ptr = 0;
    ULONG topology_object_table_entry_size = 0;
    fbe_topology_object_status_t  object_status;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t port_index, port_index_tmp;
    fbe_u32_t encl_index, encl_index_tmp;
    fbe_u32_t comp_id, comp_id_tmp ;
    fbe_bool_t  encl_filter = FALSE, port_filter = FALSE, comp_filter = FALSE, print_all = TRUE;
    fbe_u32_t len = 0;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t max_entries = 0;
    fbe_u8_t*   str = NULL;
    fbe_u8_t    buffer[20] = {'\0'};
    ULONG64 max_toplogy_entries_p = 0;
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_debug_get_ptr_size(pp_ext_physical_package_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    len = (fbe_u32_t)strlen(args);
    if (len>18)
    {
        len = 18;  // we need to leave a space to terminator the string.
    }
    memcpy( buffer, args, len + 1 );
    
    if (len)
    {
        str = strtok( buffer, " \t" );
        while (str != NULL)
        {
            if(strncmp(str, "-b", 2) == 0)
            {
                str = strtok (NULL, " \t");
                port_index= (fbe_u32_t)strtoul(str, 0, 0);
                port_filter =  TRUE;
                print_all = FALSE;
            }
            else if(strncmp(str, "-e", 2) == 0)
            {
                str = strtok (NULL, " \t");
                encl_index = (fbe_u32_t)strtoul(str, 0, 0);
                encl_filter = TRUE;
                print_all = FALSE;
            }
            else if(strncmp(str, "-cid", 4) == 0)
            {
                str = strtok (NULL, " \t");
                comp_id = (fbe_u32_t)strtoul(str, 0, 0);
                comp_filter = TRUE;
                print_all = FALSE;
            }
            str = strtok (NULL, " \t");
        }
    }

    if(encl_filter && (!port_filter ))
    {
        csx_dbg_ext_print("\n** Please give bus number. **\n");
        csx_dbg_ext_print("%s\n",usageMsg_ppencledal);
        return;
    }
    else if(comp_filter && (!port_filter || !encl_filter))
    {
        csx_dbg_ext_print("\n** Please give both bus number and enclosure number. **\n");
        csx_dbg_ext_print("%s\n",usageMsg_ppencledal);
        return;
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(pp_ext_physical_package_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(pp_ext_physical_package_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

	FBE_GET_EXPRESSION(pp_ext_physical_package_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if(topology_object_tbl_ptr == 0)
    {
        return;
    }

    object_status = 0;

    if(port_filter)
    {
        for(i = 0; i < FBE_MAX_OBJECTS; i++)
        {
            fbe_enclosure_get_obj_handle_and_class(pp_ext_physical_package_name,
                                      topology_object_tbl_ptr,&control_handle_ptr,&class_id);
                
            if(IS_VALID_LEAF_ENCL_CLASS(class_id)) 
            {
                fbe_base_enclosure_debug_get_port_index(pp_ext_physical_package_name,control_handle_ptr, &port_index_tmp);
                if(port_index == port_index_tmp)
                {
                    if(!encl_filter)
                    {
                        fbe_base_enclosure_stat_debug_basic_info(pp_ext_physical_package_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                        csx_dbg_ext_print("\n");
                    }
                    else
                    {
                        fbe_base_enclosure_debug_get_encl_pos(pp_ext_physical_package_name,control_handle_ptr, &encl_index_tmp);
                        if(encl_index == encl_index_tmp)                            
                        {
                            if(!comp_filter)
                            {
                                fbe_base_enclosure_stat_debug_basic_info(pp_ext_physical_package_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                                csx_dbg_ext_print("\n");
                            }
                            else
                            {
                                fbe_base_enclosure_debug_get_comp_id(pp_ext_physical_package_name,control_handle_ptr, &comp_id_tmp);
            
                                if(comp_id == comp_id_tmp)
                                {
                                    fbe_base_enclosure_stat_debug_basic_info(pp_ext_physical_package_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                                    csx_dbg_ext_print("\n");
                                 
                                    comp_filter = FALSE;
                                    encl_filter = FALSE;
                                    port_filter = FALSE;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            topology_object_tbl_ptr += topology_object_table_entry_size; 
        }
    }
    else  /* print_all */
    {
        for(i = 0; i < FBE_MAX_OBJECTS; i++)
        {
            fbe_enclosure_get_obj_handle_and_class(pp_ext_physical_package_name,
                                      topology_object_tbl_ptr,&control_handle_ptr,&class_id);

            if(IS_VALID_LEAF_ENCL_CLASS(class_id)) 
            {
                fbe_base_enclosure_stat_debug_basic_info(pp_ext_physical_package_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                csx_dbg_ext_print("\n");
            }
            
            if(IS_VALID_BOARD_CLASS(class_id))
            {
                fbe_base_pe_enclosure_stat_debug_basic_info(pp_ext_physical_package_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                csx_dbg_ext_print("\n");   
            }
            topology_object_tbl_ptr += topology_object_table_entry_size; 
        }
    }

    return;
}

//
//  Description: Extension to display enclosure status data.
//      
//
//  Arguments:
//  NONE
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4ppenclstat")
static char CSX_MAYBE_UNUSED usageMsg_ppenclstat[] =
"!ppenclstat\n"
"  Displays enclosure status info.\n";
#pragma data_seg ()


CSX_DBG_EXT_DEFINE_CMD(ppenclstat, "ppenclstat")
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status;
    fbe_dbgext_ptr control_handle_ptr = 0;
    ULONG topology_object_table_entry_size;
    int i = 0;
    fbe_class_id_t class_id;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_debug_get_ptr_size(pp_ext_physical_package_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(pp_ext_physical_package_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(pp_ext_physical_package_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    fbe_debug_trace_func(NULL, "\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

	FBE_GET_EXPRESSION(pp_ext_physical_package_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if(topology_object_tbl_ptr == 0){
        return;
    }

    for(i = 0; i < max_entries ; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return;
        }
        
        object_status = 0;
        class_id = 0;

        /* Fetch the object's topology status. */
        FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                        topology_object_tbl_ptr,
                        topology_object_table_entry_s,
                        object_status,
                        sizeof(fbe_topology_object_status_t),
                        &object_status);

        /* Fetch the control handle, which is the pointer to the object. */
        FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                        topology_object_tbl_ptr,
                        topology_object_table_entry_s,
                        control_handle,
                        sizeof(fbe_dbgext_ptr),
                        &control_handle_ptr);

        /* If the object's topology status is ready, then get the
              * topology status. 
              */
        if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            // csx_dbg_ext_print("\n object status is ready \n");
            FBE_GET_FIELD_DATA(pp_ext_physical_package_name,
                            control_handle_ptr,
                            fbe_base_object_s,
                            class_id,
                            sizeof(fbe_class_id_t),
                            &class_id);

            if(IS_VALID_LEAF_ENCL_CLASS(class_id))
            {
                fbe_eses_enclosure_debug_encl_stat(pp_ext_physical_package_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                fbe_debug_trace_func(NULL, "\n"); 
            }
        }

        topology_object_tbl_ptr += topology_object_table_entry_size;
    }
    return;
}
