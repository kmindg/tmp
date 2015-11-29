/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_debug_ext.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains the enclosure object's debugger
 *  extension related functions.
 *
 *
 * NOTE: 
 *
 * HISTORY
 *   09-Dec-2008 bphilbin - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_base_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_eses_enclosure_config.h"
#include "edal_eses_enclosure_data.h"
#include "base_object_private.h"
#include "../fbe_eses_enclosure_lookup.c"

// local functions
static fbe_status_t fbe_eses_enclosure_debug_parse_drive_phy_fw_data(const fbe_u8_t * module_name,
                                           ULONG64 enclosureComponentTypeBlock_p, 
                                           fbe_u8_t *enclosure_type, 
                                           fbe_u8_t max_drive_count, 
                                           fbe_u8_t max_phy_count,
                                           slot_flags_t *driveInfo,
                                           char *fw_rev,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context);

/*!**************************************************************
 * @fn fbe_eses_enclosure_debug_basic_info(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr eses_enclosure,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the eses enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param eses_enclosure_p - Ptr to eses enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  12/9/2008 - Created. bphilbin
 *
 ****************************************************************/

fbe_status_t fbe_eses_enclosure_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr eses_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_u32_t unique_id = 0;
    fbe_u32_t sas_encl_offset = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sas_enclosure_type_t sas_enclosure_type;

    FBE_GET_FIELD_OFFSET(module_name, fbe_sas_enclosure_t, "sas_enclosure", &sas_encl_offset);
    
    csx_dbg_ext_print("  Class id: ");
    fbe_base_object_debug_trace_class_id(module_name, eses_enclosure_p, trace_func, trace_context);
    
    csx_dbg_ext_print("  Lifecycle State: ");
    fbe_base_object_debug_trace_state(module_name, eses_enclosure_p, trace_func, trace_context);

    csx_dbg_ext_print("\n");

    /* Display eses enclosure specific data. */
    FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           sas_enclosure_type,
                           sizeof(fbe_sas_enclosure_type_t),
                           &sas_enclosure_type);

    csx_dbg_ext_print("  SAS Enclosure Type: %d", sas_enclosure_type);
    FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           unique_id,
                           sizeof(fbe_u32_t),
                           &unique_id);

    csx_dbg_ext_print("  Enclosure Unique ID: 0x%x\n", unique_id);

    /* Print out the sas enclosure specific information. */
    status = fbe_sas_enclosure_debug_extended_info(module_name, 
                                          (eses_enclosure_p + sas_encl_offset), 
                                           trace_func, 
                                           trace_context);
    return status;
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_debug_basic_info(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr eses_enclosure,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the eses enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param eses_enclosure_p - Ptr to eses enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK/FBE_STATUS_INVALID  
 *
 * HISTORY:
 *  09/01/2009 - Created. UDK
 *
 ****************************************************************/

fbe_status_t fbe_eses_enclosure_element_group_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr eses_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_u32_t i = 0;
    fbe_u32_t count = 0 ;
    fbe_u32_t sas_encl_offset = 0;
    fbe_u32_t element_group_offset = 0;
    fbe_u32_t enclosure_properties_offset = 0;  
   
    ULONG64 element_group;

    fbe_eses_elem_group_t  eses_element_group_local;   
    fbe_eses_enclosure_properties_t eses_enclosure_properties_local;   

    ULONG eses_element_group_size;
    ULONG eses_enclosure_properties_size;

    FBE_GET_TYPE_SIZE(module_name, fbe_eses_elem_group_s, &eses_element_group_size );
    FBE_GET_TYPE_SIZE(module_name, fbe_eses_enclosure_properties_s, &eses_enclosure_properties_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_sas_enclosure_t, "sas_enclosure", &sas_encl_offset);
    
    FBE_GET_FIELD_OFFSET(module_name, fbe_eses_enclosure_s, "elem_group", &element_group_offset);//get the offset of element_group sub structure 

    FBE_GET_FIELD_OFFSET(module_name, fbe_eses_enclosure_s, "properties", &enclosure_properties_offset);    //get the offset of properties sub structure

    FBE_READ_MEMORY(eses_enclosure_p + element_group_offset, &element_group, sizeof(ULONG64));

    csx_dbg_ext_print("element_group address: %llX\n", (unsigned long long)element_group);
    FBE_READ_MEMORY(eses_enclosure_p + enclosure_properties_offset, &eses_enclosure_properties_local, eses_enclosure_properties_size);  
    csx_dbg_ext_print("\n\nNo. of  element groups : %d\n", eses_enclosure_properties_local.number_of_actual_elem_groups);

    count = eses_enclosure_properties_local.number_of_actual_elem_groups;

    if(count == 0)   
    {
        csx_dbg_ext_print("\n Returning !\n");        
        return FBE_STATUS_INVALID;
    }    

    // print enclosure position
    fbe_sas_enclosure_debug_encl_port_data(module_name, eses_enclosure_p + sas_encl_offset,
        trace_func,trace_context);

    csx_dbg_ext_print("\nGroup_Id  First_index  byte_offset  num_elem    subencl_id     Elem_Type\n");
    for(i = 0; i < count; i ++)
    {
        FBE_READ_MEMORY(element_group, &eses_element_group_local , eses_element_group_size);
            
        csx_dbg_ext_print("\n");     

        csx_dbg_ext_print("\t%d",i); 
        csx_dbg_ext_print("\t\t%d",eses_element_group_local.first_elem_index);  
        csx_dbg_ext_print("\t\t%d",eses_element_group_local.byte_offset);
        csx_dbg_ext_print("\t\t%d",eses_element_group_local.num_possible_elems);


        csx_dbg_ext_print("\t\t%d",eses_element_group_local.subencl_id);    
        csx_dbg_ext_print("\t%s",elem_type_to_text(eses_element_group_local.elem_type));    
        element_group += eses_element_group_size;  
    }

    return FBE_STATUS_OK; 
}
/*!**************************************************************
 * @fn fbe_eses_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr eses_enclosure_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display private data about the eses enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param eses_enclosure_p - Ptr to eses enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  10-Apr-2009 - Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_eses_enclosure_debug_prvt_data(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr eses_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
  char               encl_serial_number[MAX_SIZE_EMC_SERIAL_NUMBER];
  fbe_u8_t           primary_port_entire_connector_index_0 = 0;
  fbe_u8_t           primary_port_entire_connector_index_1 = 0;
  fbe_bool_t         enclosureConfigBeingUpdated = FALSE;
  fbe_bool_t         inform_fw_activating = FALSE;
  fbe_u8_t           outstanding_write = 0;
  fbe_u8_t           emc_encl_ctrl_outstanding_write = 0;
  fbe_u8_t           mode_select_needed = 0;
  fbe_u8_t           test_mode_status = 0;
  fbe_u8_t           test_mode_rqstd = 0;

  fbe_u32_t          sas_enclosure_offset = 0;
    fbe_sas_enclosure_type_t     sasEnclosureType; 
  
  trace_func(trace_context, "ESES Private Data:\n");

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           encl_serial_number,
                           (sizeof(char)* MAX_SIZE_EMC_SERIAL_NUMBER),
                           encl_serial_number);

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           primary_port_entire_connector_index[0],
                           sizeof(fbe_u8_t),
                           &primary_port_entire_connector_index_0);

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           primary_port_entire_connector_index[1],
                           sizeof(fbe_u8_t),
                           &primary_port_entire_connector_index_1);

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           enclosureConfigBeingUpdated,
                           sizeof(fbe_bool_t),
                           &enclosureConfigBeingUpdated);
  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           inform_fw_activating,
                           sizeof(fbe_bool_t),
                           &inform_fw_activating);
  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           outstanding_write,
                           sizeof(fbe_u8_t),
                           &outstanding_write);

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           emc_encl_ctrl_outstanding_write,
                           sizeof(fbe_u8_t),
                           &emc_encl_ctrl_outstanding_write);

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           mode_select_needed,
                           sizeof(fbe_u8_t),
                           &mode_select_needed);

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           test_mode_status,
                           sizeof(test_mode_rqstd),
                           &test_mode_status);

  FBE_GET_FIELD_DATA(module_name, 
                           eses_enclosure_p,
                           fbe_eses_enclosure_s,
                           test_mode_rqstd,
                           sizeof(test_mode_rqstd),
                           &test_mode_rqstd);
    FBE_GET_FIELD_DATA(module_name, 
        eses_enclosure_p,
        fbe_eses_enclosure_s,
        sas_enclosure_type,
        sizeof(fbe_sas_enclosure_type_t),
        &sasEnclosureType);

  fbe_eses_enclosure_print_encl_serial_number(encl_serial_number, trace_func, trace_context);
  fbe_eses_enclosure_print_primary_port_entire_connector_index(primary_port_entire_connector_index_0, trace_func, trace_context);
  fbe_eses_enclosure_print_primary_port_entire_connector_index(primary_port_entire_connector_index_1, trace_func, trace_context);
  fbe_eses_enclosure_print_enclosureConfigBeingUpdated(enclosureConfigBeingUpdated, trace_func, trace_context);
  fbe_eses_enclosure_print_outstanding_write(outstanding_write, trace_func, trace_context);
  fbe_eses_enclosure_print_emc_encl_ctrl_outstanding_write(emc_encl_ctrl_outstanding_write, trace_func, trace_context);
  fbe_eses_enclosure_print_mode_select_needed(mode_select_needed, trace_func, trace_context);
  fbe_eses_enclosure_print_test_mode_status(test_mode_status, trace_func, trace_context);
  fbe_eses_enclosure_print_test_mode_rqstd(test_mode_rqstd, trace_func, trace_context);
  fbe_eses_enclosure_print_fw_activating_status(inform_fw_activating, trace_func, trace_context);
  fbe_sas_enclosure_print_sasEnclosureType(sasEnclosureType, trace_func, trace_context);
  
  FBE_GET_FIELD_OFFSET(module_name, fbe_sas_enclosure_t, "sas_enclosure", &sas_enclosure_offset);
  fbe_sas_enclosure_debug_prvt_data(module_name, eses_enclosure_p + sas_enclosure_offset ,trace_func,trace_context);
  return FBE_STATUS_OK;

}

/*!**************************************************************
 * @fn fbe_eses_enclosure_debug_encl_stat(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr eses_enclosure,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display status information about the eses enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param eses_enclosure_p - Ptr to eses enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK/FBE_STATUS_INVALID  
 *
 * HISTORY:
 *  19/05/2009 - Nayana Chaudhari   Created.
 *
 ****************************************************************/

fbe_status_t fbe_eses_enclosure_debug_encl_stat(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr eses_enclosure_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_u32_t index = 0;
    fbe_u8_t max_phy_count = 0;
    fbe_u8_t max_drive_count = 0;
    slot_flags_t *driveInfo = NULL;

    fbe_u8_t  enclosureType_local = 0;
    fbe_u32_t sas_enclosure_offset = 0;
    char  fw_rev[MAX_FW_REV_STR_LEN+1] = {'\0'};

    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t base_enclosure_offset = 0;
    ULONG64 enclosureComponentTypeBlock_p; 
    ULONG  fbe_enclosure_fault_t_size = 0;
    fbe_u32_t enclosure_faults_offset = 0;
    fbe_lifecycle_state_t lifecycle_state;

    fbe_u32_t enclosure_properties_offset = 0;
    ULONG fbe_enclosure_component_block_size = 0;
    fbe_u32_t enclosureComponentTypeBlock_offset = 0;
    fbe_enclosure_fault_t enclosure_faults_local;
    fbe_enclosure_component_id_t component_id = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_component_block_s, &fbe_enclosure_component_block_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_fault_s, &fbe_enclosure_fault_t_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_eses_enclosure_s, "properties", &enclosure_properties_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_eses_enclosure_s, "sas_enclosure", &sas_enclosure_offset); 
    FBE_GET_FIELD_OFFSET(module_name, fbe_sas_enclosure_s, "base_enclosure", &base_enclosure_offset); 
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_enclosure_s, "enclosureComponentTypeBlock", &enclosureComponentTypeBlock_offset);    
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_enclosure_s, "enclosure_faults", &enclosure_faults_offset);

    // get max_num_drives and max_num_phys from fbe_eses_enclosure_properties 
    FBE_GET_FIELD_DATA(module_name,
                       eses_enclosure_p + enclosure_properties_offset,
                       fbe_eses_enclosure_properties_t,
                       number_of_slots,
                       sizeof(fbe_u8_t),
                       &max_drive_count);

     FBE_GET_FIELD_DATA(module_name,
                       eses_enclosure_p + enclosure_properties_offset,
                       fbe_eses_enclosure_properties_t,
                       number_of_expander_phys,
                       sizeof(fbe_u8_t),
                       &max_phy_count);
    
    // get life cycle state. We use this to decide if enclosure is failed.
    FBE_GET_FIELD_DATA(module_name,
                       eses_enclosure_p,
                       fbe_base_object_s,
                       lifecycle.state,
                       sizeof(fbe_lifecycle_state_t),
                       &lifecycle_state);

    // read pointer to enclosure blob
    FBE_READ_MEMORY(eses_enclosure_p  + sas_enclosure_offset + base_enclosure_offset + enclosureComponentTypeBlock_offset , 
                &enclosureComponentTypeBlock_p, sizeof(ULONG64 ));

    driveInfo = (slot_flags_t *) malloc(sizeof(slot_flags_t) * max_drive_count);

    if(driveInfo == NULL)
    {
        trace_func(trace_context, "driveInfo is NULL pointer.\n"); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        // get drive, phy and fw info from enclsoure blob
        status = fbe_eses_enclosure_debug_parse_drive_phy_fw_data(module_name,
                        enclosureComponentTypeBlock_p, 
                        &enclosureType_local, 
                        max_drive_count,
                        max_phy_count,
                        driveInfo,
                        fw_rev,
                        trace_func,
                        trace_context);

        if(status != FBE_STATUS_OK)
        {
            trace_func(trace_context, "fbe_eses_enclosure_debug_parse_drive_phy_fw_data failed:0x%x\n",
                status);

            free (driveInfo); /* Free the drive Info */
            return status;
        }
    }

    // print enclosure type
    trace_func(NULL, "%s",fbe_edal_print_Enclosuretype(enclosureType_local));

    // print enclosure position
    fbe_sas_enclosure_debug_encl_port_data(module_name, eses_enclosure_p + sas_enclosure_offset,
        trace_func,trace_context);

    if(enclosureType_local == FBE_ENCL_SAS_VOYAGER_EE)
    {
        FBE_GET_FIELD_DATA(module_name, 
                        eses_enclosure_p + sas_enclosure_offset + base_enclosure_offset,
                        fbe_base_enclosure_s,
                        component_id,
                        sizeof(fbe_enclosure_component_id_t),
                        &component_id);
        trace_func(trace_context, " (Component Id: %d)\n",component_id);
    }
    else
    {
        trace_func(trace_context, "\n");
    }

    // print lifecycle state
    trace_func(trace_context, " Lifecycle State:");
    fbe_base_object_debug_trace_state(module_name, eses_enclosure_p, trace_func, trace_context);
    trace_func(trace_context,"  BundleRev:%s",fw_rev);
    trace_func(trace_context,"\n"); 

    // print drive info
    trace_func(trace_context,"\nSlot :");
    for (index = 0; index < max_drive_count; index++)
    {
        trace_func(trace_context,"%3d ",index);
    }
    trace_func(trace_context,"\nstate:");
    for (index = 0; index < max_drive_count; index++)
    {
        trace_func(trace_context,"%s ",fbe_eses_debug_decode_slot_state(&driveInfo[index]));
    }
    trace_func(trace_context,"\n");

    // printf fault info when enclosure if failed.
    if(lifecycle_state == FBE_LIFECYCLE_STATE_FAIL )
    {
        // get fault info
        FBE_READ_MEMORY(eses_enclosure_p + sas_enclosure_offset + base_enclosure_offset + enclosure_faults_offset, 
                    &enclosure_faults_local, fbe_enclosure_fault_t_size);

        // print fault info
        trace_func(trace_context, "\nFaultyCompType:%s   FaultyCompIndex:0x%x   ", 
        enclosure_access_printComponentType(enclosure_faults_local.faultyComponentType), 
        enclosure_faults_local.faultyComponentIndex);
        fbe_base_enclosure_print_encl_fault_symptom(enclosure_faults_local.faultSymptom, trace_func, trace_context);
        trace_func(trace_context, "\n");
    }
    if(driveInfo)
    {
        free(driveInfo);
    }
    return FBE_STATUS_OK;    
}

/*!**************************************************************
 * @fn fbe_eses_enclosure_debug_parse_drive_phy_fw_data(const fbe_u8_t * module_name,
                                           ULONG64 enclosureComponentTypeBlock_p, 
                                           fbe_u8_t *enclosure_type, 
                                           fbe_u8_t max_drive_count,
                                           fbe_u8_t max_phy_count,
                                           slot_flags_t *driveInfo,
                                           char *fw_rev,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display status information about the eses enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param enclosureComponentTypeBlock_p - pointer to enclosure block
 * @param enclosure_type - enclosure type to be returned
 * @param max_drive_count - max drive count
 * @param max_phy_count - max phy count
 * @param driveInfo - drive and phy info
 * @param fw_rev - fw rev 
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK/FBE_STATUS_INVALID  
 *
 * HISTORY:
 *  06/09/2009 - Nayana Chaudhari   Created.
 *
 ****************************************************************/
static fbe_status_t fbe_eses_enclosure_debug_parse_drive_phy_fw_data(const fbe_u8_t * module_name,
                                           ULONG64 enclosureComponentTypeBlock_p, 
                                           fbe_u8_t *enclosure_type, 
                                           fbe_u8_t max_drive_count, 
                                           fbe_u8_t max_phy_count,
                                           slot_flags_t *driveInfo,
                                           char *fw_rev,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    ULONG64 pnextblock_p  = 0;
    fbe_u8_t    enclosureType_local;
    fbe_u32_t blockSize_local;
    fbe_u8_t    numberComponentTypes_local;
    ULONG64     componentTypeDataPtr;
    ULONG fbe_enclosure_component_block_size;
    fbe_u8_t    enclosure_component_local[FBE_MEMORY_CHUNK_SIZE];
    fbe_u32_t loopCount;
    fbe_enclosure_component_t   *fbe_enclosure_component_local;
    ULONG  fbe_enclosure_component_t_size; 
    fbe_u32_t                    index;
    fbe_edal_component_data_handle_t generalDataPtr;
    slot_flags_t *driveStat=NULL;
    fbe_u8_t side_id = 0;
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;

    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_component_block_s, &fbe_enclosure_component_block_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_component_s, &fbe_enclosure_component_t_size);

    driveStat = (slot_flags_t *) malloc(sizeof(slot_flags_t) * max_drive_count);

    if(driveStat == NULL)
    {
        trace_func(trace_context, "driveStat is NULL pointer.\n"); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            while ( enclosureComponentTypeBlock_p !=  NULL)
            {
                FBE_GET_FIELD_DATA(module_name, 
                                    enclosureComponentTypeBlock_p ,
                                    fbe_enclosure_component_block_t,
                                    pNextBlock,
                                    sizeof(ULONG64),
                                    &pnextblock_p); 
                FBE_GET_FIELD_DATA(module_name, 
                                    enclosureComponentTypeBlock_p,
                                    fbe_enclosure_component_block_t,
                                    enclosureType,
                                    sizeof(fbe_u8_t),
                                    &enclosureType_local);
                FBE_GET_FIELD_DATA(module_name, 
                                    enclosureComponentTypeBlock_p,
                                    fbe_enclosure_component_block_t,
                                    blockSize,
                                    sizeof(fbe_u32_t),
                                    &blockSize_local);
                FBE_GET_FIELD_DATA(module_name, 
                                    enclosureComponentTypeBlock_p,
                                    fbe_enclosure_component_block_t,
                                    numberComponentTypes,
                                    sizeof(fbe_u8_t),
                                    &numberComponentTypes_local);

                if (FBE_MEMORY_CHUNK_SIZE < blockSize_local)
                {
                    trace_func( trace_context, "Block Size exceeded the Max Limit: %d\n", FBE_MEMORY_CHUNK_SIZE); 

                    free(driveStat);

                    return FBE_STATUS_GENERIC_FAILURE;
                }
                
                componentTypeDataPtr = (enclosureComponentTypeBlock_p + fbe_enclosure_component_block_size);
                FBE_READ_MEMORY(componentTypeDataPtr, &enclosure_component_local, 
                    (blockSize_local - fbe_enclosure_component_block_size));
                
                fbe_edal_get_fw_target_component_type_attr(FBE_FW_TARGET_LCC_MAIN,&fw_comp_type,&fw_comp_attr);

                for (loopCount = 0; loopCount < numberComponentTypes_local; loopCount++)
                {
                    fbe_enclosure_component_local =  
                        (fbe_enclosure_component_t *)&enclosure_component_local[fbe_enclosure_component_t_size * loopCount];

                    // get side_id and index 
                if(fbe_enclosure_component_local->componentType == FBE_ENCL_ENCLOSURE)
                     {
                       enclosure_access_getSpecifiedComponentData(fbe_enclosure_component_local,
                                                                        0, &generalDataPtr);
                       fbe_eses_encl_get_side_id(generalDataPtr,enclosureType_local,&side_id);
                     }
                    if(fbe_enclosure_component_local->componentType == FBE_ENCL_DRIVE_SLOT)
                    { 
                        for (index = 0; index < max_drive_count; index++)
                        {
                            enclosure_access_getSpecifiedComponentData(fbe_enclosure_component_local,
                                                                        index, &generalDataPtr);

                            fbe_eses_debug_getSpecificDriveData(generalDataPtr, index,
                                enclosureType_local, driveStat);
                        }
                    }

                    // get the lcc fw data
                    if(fbe_enclosure_component_local->componentType == fw_comp_type)
                    {
                        for (index = 0; index < fbe_enclosure_component_local->maxNumberOfComponents; index++)
                        {
                            enclosure_access_getSpecifiedComponentData(fbe_enclosure_component_local,
                                                                        index, &generalDataPtr);
                            if((fbe_enclosure_fw_target_t)index == side_id)
                           {
                               fbe_eses_getFwRevs(generalDataPtr, enclosureType_local, 
                                                  fw_comp_type, fw_comp_attr, fw_rev);
                           }
                        }
                    }
                    // go to next component type
                    componentTypeDataPtr += fbe_enclosure_component_t_size;
                }   // end of for loop

                enclosureComponentTypeBlock_p = pnextblock_p;
                componentTypeDataPtr = NULL;
            } // end of while

            *enclosure_type = enclosureType_local;

            for (index = 0; index < max_drive_count; index++)
            {
                driveInfo[index].driveInserted = driveStat[index].driveInserted;
                driveInfo[index].driveFaulted = driveStat[index].driveFaulted;
                driveInfo[index].drivePoweredOff = driveStat[index].drivePoweredOff;
                driveInfo[index].driveBypassed  = driveStat[index].driveBypassed;
                driveInfo[index].driveLoggedIn= driveStat[index].driveLoggedIn;                                             
            }

            free(driveStat);

            return FBE_STATUS_OK;
        }
    }


/* Dummy stubs required to satisfy linker when linking fbe enclosure data
 * access library with ppdbg.dll.
 *
 * @todo The following stubs need to be cleaned up when we address the issue of
 * edal library used for both production and dbgdll.
 */
fbe_trace_level_t fbe_trace_default_trace_level = FBE_TRACE_LEVEL_INFO;

#if 0
fbe_trace_level_t 
fbe_trace_get_default_trace_level(void)
{
    return FBE_TRACE_LEVEL_INFO;
}
#endif

void 
fbe_trace_report(fbe_u32_t component_type,
                 fbe_u32_t component_id,
                 fbe_trace_level_t trace_level,
                 fbe_u32_t message_id,
                 const fbe_u8_t * fmt, 
                 va_list argList)

{

}

