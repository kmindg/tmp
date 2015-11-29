/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_enclosure_debug_ext.c
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
 *   10-Apr-2009 Dipak Patel - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_enclosure_debug.h"
#include "base_enclosure_private.h"

/*!**************************************************************
 * @fn fbe_base_enclosure_debug_prvt_data(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr base_enclosure_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display private data about the base enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_enclosure_p - Ptr to base enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  10-Apr-2009 - Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_base_enclosure_debug_prvt_data(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_u32_t       port_number = 0;
    fbe_u32_t       faultSymptom_p = 0;
    fbe_u32_t       number_of_slots = 0;
    fbe_u32_t       first_slot_index = 0;
    fbe_u32_t       enclosure_number = 0;

    fbe_u32_t       enclosure_faults_offset = 0;
    fbe_time_t       time_ride_through_start = 0;
    fbe_u32_t       number_of_expansion_ports = 0;
    fbe_u32_t       first_expansion_port_index = 0;
    fbe_u32_t       default_ride_through_period = 0;
    fbe_u32_t       expected_ride_through_period = 0;
    fbe_u32_t       number_of_drives_spinningup = 0;
    fbe_u32_t       max_number_of_drives_spinningup = 0;
    fbe_enclosure_component_id_t componentid = 0;
    FBE_GET_FIELD_OFFSET(module_name, 
                        fbe_base_enclosure_s, 
                        "enclosure_faults", 
                        &enclosure_faults_offset);

    trace_func(trace_context, "BASE Private Data:\n");

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        enclosure_number,
                        sizeof(fbe_u32_t),
                        &enclosure_number);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        port_number,
                        sizeof(fbe_u32_t),
                        &port_number);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        number_of_slots,
                        sizeof(fbe_u32_t),
                        &number_of_slots);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        first_slot_index,
                        sizeof(fbe_u32_t),
                        &first_slot_index);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        first_expansion_port_index,
                        sizeof(fbe_u32_t),
                        &first_expansion_port_index);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        number_of_expansion_ports,
                        sizeof(fbe_u32_t),
                        &number_of_expansion_ports);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        time_ride_through_start,
                        sizeof(fbe_time_t),
                        &time_ride_through_start);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        expected_ride_through_period,
                        sizeof(fbe_u32_t),
                        &expected_ride_through_period);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        default_ride_through_period,
                        sizeof(fbe_u32_t),
                        &default_ride_through_period);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        number_of_drives_spinningup,
                        sizeof(fbe_u32_t),
                        &number_of_drives_spinningup);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        max_number_of_drives_spinningup,
                        sizeof(fbe_u32_t),
                        &max_number_of_drives_spinningup);
    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        component_id,
                        sizeof(fbe_enclosure_component_id_t),
                        &componentid);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p + enclosure_faults_offset,
                        fbe_enclosure_fault_s,
                        faultSymptom,
                        sizeof(fbe_u32_t),
                        &faultSymptom_p);

    fbe_base_enclosure_print_enclosure_number(enclosure_number, trace_func, trace_context);
    fbe_base_enclosure_print_port_number(port_number, trace_func, trace_context); 
    fbe_base_enclosure_print_number_of_slots(number_of_slots, trace_func, trace_context);
    fbe_base_enclosure_print_first_slot_index(first_slot_index, trace_func, trace_context);
    fbe_base_enclosure_print_first_expansion_port_index(first_expansion_port_index, trace_func, trace_context);
    fbe_base_enclosure_print_number_of_expansion_ports(number_of_expansion_ports, trace_func, trace_context);
    fbe_base_enclosure_print_time_ride_through_start(time_ride_through_start, trace_func, trace_context);
    fbe_base_enclosure_print_expected_ride_through_period(expected_ride_through_period, trace_func, trace_context);
    fbe_base_enclosure_print_default_ride_through_period(default_ride_through_period, trace_func, trace_context);   
    fbe_base_enclosure_print_number_of_drives_spinningup(number_of_drives_spinningup, trace_func, trace_context); 
    fbe_base_enclosure_print_max_number_of_drives_spinningup(max_number_of_drives_spinningup, trace_func, trace_context);
    fbe_base_enclosure_print_component_id(componentid, trace_func, trace_context);
    fbe_base_enclosure_print_encl_fault_symptom(faultSymptom_p, trace_func, trace_context);
    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_base_enclosure_stat_debug_basic_info(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr base_enclosure_p,
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
 *  15/05/2009 - Created. UDK
 *
 ****************************************************************/

fbe_status_t fbe_base_enclosure_stat_debug_basic_info(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr base_enclosure_p, 
                                           fbe_edal_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_u8_t    enclosureType_local = 0;
    fbe_u8_t    edalLocale_local = 0;
    fbe_u8_t    numberComponentTypes_local = 0;     // number of component in this block
    fbe_u8_t    maxNumberComponentTypes_local = 0;  // max number of component in this block
    fbe_u32_t   enclosureBlockCanary_local = 0;    

    fbe_u32_t   loopCount = 0;
    fbe_u32_t   blockSize_local = 0;                // size of current block
    fbe_u32_t   pNextBlock_offset = 0;    
    fbe_u32_t   availableDataSpace_local = 0;       // free space left inside the block
    fbe_u32_t   generation_count_local = 0;
    fbe_u32_t   enclosure_component_t_size = 0; 
    fbe_u32_t   overallStateChangeCount_local = 0;     
    fbe_u32_t   enclosure_component_block_size = 0;
    fbe_u32_t   enclosureComponentTypeBlock_offset = 0;

    ULONG64     pnextblock_p;  
    ULONG64     componentTypeDataPtr;
    ULONG64     enclosureComponentTypeBlock_p;

    fbe_u8_t    enclosure_component_local[FBE_MEMORY_CHUNK_SIZE];
    fbe_enclosure_component_t   *enclosure_component_data;

    /* Print bus and enclosure number */
    fbe_base_enclosure_debug_encl_port_data(module_name, base_enclosure_p ,trace_func,trace_context);

    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_component_s, &enclosure_component_t_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_component_block_s, &enclosure_component_block_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_enclosure_s, "enclosureComponentTypeBlock", &enclosureComponentTypeBlock_offset);    
    FBE_GET_FIELD_OFFSET(module_name, fbe_enclosure_component_block_s, "pNextBlock", &pNextBlock_offset);    //get the offset of properties sub structure       
    csx_dbg_ext_print("pnextBlock offset is : %d\n", pNextBlock_offset); 

    FBE_READ_MEMORY(base_enclosure_p + enclosureComponentTypeBlock_offset, &enclosureComponentTypeBlock_p, sizeof(ULONG64));  

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
                            enclosureBlockCanary,
                            sizeof(fbe_u32_t),
                            &enclosureBlockCanary_local);

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            edalLocale,
                            sizeof(fbe_u8_t),
                            &edalLocale_local);

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            enclosureType,
                            sizeof(fbe_u8_t),
                            &enclosureType_local);

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            maxNumberComponentTypes,
                            sizeof(fbe_u8_t),
                            &maxNumberComponentTypes_local);

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            blockSize,
                            sizeof(fbe_u32_t),
                            &blockSize_local);

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            availableDataSpace,
                            sizeof(fbe_u32_t),
                            &availableDataSpace_local);

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            overallStateChangeCount,
                            sizeof(fbe_u32_t),
                            &overallStateChangeCount_local);

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            numberComponentTypes,
                            sizeof(fbe_u8_t),
                            &numberComponentTypes_local);    

        FBE_GET_FIELD_DATA(module_name, 
                            enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            generation_count,
                            sizeof(fbe_u32_t),
                            &generation_count_local);
        trace_func( NULL, "Enclosure Data, BlockCanary            : 0x%x\n", enclosureBlockCanary_local); 
        trace_func( NULL, "Enclosure Data, componentId            : %d\n", edalLocale_local); 
        trace_func( NULL, "Enclosure Data, OverallStateChangeCount : %d\n", overallStateChangeCount_local); 
        trace_func( NULL, "Enclosure Data, Max no of ComponentTypes    : %d\n", maxNumberComponentTypes_local);  
        trace_func( NULL, "Enclosure Data, availableDataSpace : %d\n", availableDataSpace_local); 
        trace_func( NULL, "Enclosure Data, blockSize : %d\n", blockSize_local); 
        trace_func( NULL, "Enclosure Data, GenerationCount : %d\n", generation_count_local);

        componentTypeDataPtr = enclosureComponentTypeBlock_p + enclosure_component_block_size;

        if (FBE_MEMORY_CHUNK_SIZE < blockSize_local)
        {
            trace_func( NULL, "Block Size exceeded the Max Limit: %d\n", FBE_MEMORY_CHUNK_SIZE); 
            return FBE_STATUS_GENERIC_FAILURE;

        }

        FBE_READ_MEMORY(componentTypeDataPtr, &enclosure_component_local, (blockSize_local - enclosure_component_block_size));

        for (loopCount = 0; loopCount < numberComponentTypes_local; loopCount++)
        {
            enclosure_component_data =  (fbe_enclosure_component_t *)&enclosure_component_local[enclosure_component_t_size * loopCount];

            enclosure_access_printSpecificComponentData(enclosure_component_data, 
                                                        enclosureType_local,
                                                        FBE_ENCL_COMPONENT_INDEX_ALL,
                                                        TRUE,
                                                        trace_func);
        }// end of for loop*/
        enclosureComponentTypeBlock_p = pnextblock_p; 
        componentTypeDataPtr = NULL;
    }
             
    return FBE_STATUS_OK;    
}

/*!**************************************************************
 * @fn fbe_base_enclosure_debug_encl_port_data(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr base_enclosure_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display port data of the enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_enclosure_p - Ptr to base enclosure object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  27-May-2009 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t
fbe_base_enclosure_debug_encl_port_data(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p,
                                              fbe_trace_func_t trace_func, 
                                              fbe_trace_context_t trace_context)
{
    fbe_u32_t                enclosure_number; 
    fbe_u32_t                port_number;

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        enclosure_number,
                        sizeof(fbe_u32_t),
                        &enclosure_number);

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        port_number,
                        sizeof(fbe_u32_t),
                        &port_number);
  
    trace_func(trace_context, " (Bus:%d Enclosure:%d) ",port_number,enclosure_number);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_status_t fbe_base_enclosure_debug_get_encl_pos(const fbe_u8_t * module_name, 
 *                                           fbe_dbgext_ptr base_enclosure_p,
 *                                           fbe_u32_t *encl_pos) 
 ****************************************************************
 * @brief
 *  Get the encl position from sas viper enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_enclosure_p - Ptr to base enclosure object.
 * @param encl_pos - Ptr to store the enclosure position
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  4-Aug-2009 - Created. Prasenjeet Ghaywat
 *
 ****************************************************************/
 fbe_status_t fbe_base_enclosure_debug_get_encl_pos(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p, fbe_u32_t *encl_pos)
{

    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        enclosure_number,
                        sizeof(fbe_u32_t),
                        encl_pos);
	
   return FBE_STATUS_OK; 	
}

/*!**************************************************************
 * @fn fbe_status_t fbe_base_enclosure_debug_get_comp_id(const fbe_u8_t * module_name, 
 *                                           fbe_dbgext_ptr base_enclosure_p,
 *                                           fbe_u32_t *comp_id) 
 ****************************************************************
 * @brief
 *  Get the component id  from sas enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_enclosure_p - Ptr to base enclosure object.
 * @param comp_id - Ptr to store the component id
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  22-July-2011 - Created: sawalera
 *
 ****************************************************************/
fbe_status_t fbe_base_enclosure_debug_get_comp_id(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p, fbe_u32_t *comp_id)
{

    FBE_GET_FIELD_DATA(module_name, 
                       base_enclosure_p,
                       fbe_base_enclosure_s,
                       component_id,
                       sizeof(fbe_enclosure_component_id_t),
                       comp_id);

    return FBE_STATUS_OK; 
}

/*!**************************************************************
 * @fn fbe_status_t fbe_base_enclosure_debug_get_port_index(const fbe_u8_t * module_name, 
 *                                           fbe_dbgext_ptr sas_viper_enclosure_p,
 *                                           fbe_u32_t *port_index) 
 ****************************************************************
 * @brief
 *  Get the encl position from sas viper enclosure object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_enclosure_p - Ptr to base enclosure object.
 * @param port_index -Ptr to store the port index
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  4-Aug-2009 - Created. Prasenjeet Ghaywat
 *
 ****************************************************************/
 fbe_status_t fbe_base_enclosure_debug_get_port_index(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_enclosure_p, fbe_u32_t *port_index)
{
    FBE_GET_FIELD_DATA(module_name, 
                        base_enclosure_p,
                        fbe_base_enclosure_s,
                        port_number,
                        sizeof(fbe_u32_t),
                        port_index);
	
   return FBE_STATUS_OK;
}
/*!***************************************************************
 * @fn fbe_base_pe_enclosure_stat_debug_basic_info(const fbe_u8_t * module_name, 
 *                                                 fbe_dbgext_ptr base_board_p, 
 *                                                 fbe_edal_trace_func_t trace_func, 
 *                                                 fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information for Processor Enclosure.
 *
 * @param module_name   Module name
 * @param base_board_p  Board object pointer
 * @param trace_func    Trace function 
 * @param trace_context Trace context
 * 
 * @return fbe_status_t
 *
 * @author
 *  22-June-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t 
fbe_base_pe_enclosure_stat_debug_basic_info(const fbe_u8_t * module_name, 
                                            fbe_dbgext_ptr base_board_p, 
                                            fbe_edal_trace_func_t trace_func, 
                                            fbe_trace_context_t trace_context)
{
    fbe_u32_t    enclosureBlockCanary_local = 0;
    fbe_u8_t    maxNumberComponentTypes_local = 0;
    fbe_u8_t    enclosureType_local = 0;
    fbe_u8_t    numberComponentTypes_local = 0; 
    fbe_u8_t    enclosure_component_local[FBE_MEMORY_CHUNK_SIZE];
    fbe_u32_t   enclosure_component_t_size = 0;
    fbe_u32_t   enclosure_component_block_size = 0;
    fbe_u32_t   edal_block_handle_offset = 0;
    fbe_u32_t   pNextBlock_offset = 0;  
    fbe_u32_t   blockSize_local = 0;
    fbe_u32_t   availableDataSpace_local = 0;
    fbe_u32_t   overallStateChangeCount_local = 0;
    fbe_u32_t   generation_count_local = 0;
    fbe_u32_t   loopCount = 0;
    fbe_enclosure_component_t   *enclosure_component_data;
    ULONG64     pnextblock_p;  
    ULONG64     pe_enclosureComponentTypeBlock_p;
    ULONG64     componentTypeDataPtr;

    trace_func( NULL, "Processor Encl Data\n");

    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_component_s, &enclosure_component_t_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_enclosure_component_block_s, &enclosure_component_block_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_t, "edal_block_handle",  &edal_block_handle_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_enclosure_component_block_s, "pNextBlock", &pNextBlock_offset); 
    csx_dbg_ext_print("pnextBlock offset is : %d\n", pNextBlock_offset); 

    FBE_READ_MEMORY(base_board_p + edal_block_handle_offset, &pe_enclosureComponentTypeBlock_p, sizeof(ULONG64));  
    
    while (pe_enclosureComponentTypeBlock_p !=  NULL)
    {
        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p ,
                            fbe_enclosure_component_block_t,
                            pNextBlock,
                            sizeof(ULONG64),
                            &pnextblock_p);

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            enclosureBlockCanary,
                            sizeof(fbe_u32_t),
                            &enclosureBlockCanary_local);

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            enclosureType,
                            sizeof(fbe_u8_t),
                            &enclosureType_local);

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            maxNumberComponentTypes,
                            sizeof(fbe_u8_t),
                            &maxNumberComponentTypes_local);

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            blockSize,
                            sizeof(fbe_u32_t),
                            &blockSize_local);

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            availableDataSpace,
                            sizeof(fbe_u32_t),
                            &availableDataSpace_local);

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            overallStateChangeCount,
                            sizeof(fbe_u32_t),
                            &overallStateChangeCount_local);

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            numberComponentTypes,
                            sizeof(fbe_u8_t),
                            &numberComponentTypes_local);    

        FBE_GET_FIELD_DATA(module_name, 
                            pe_enclosureComponentTypeBlock_p,
                            fbe_enclosure_component_block_t,
                            generation_count,
                            sizeof(fbe_u32_t),
                            &generation_count_local);
        trace_func( NULL, "Enclosure Data, BlockCanary            : 0x%x\n", enclosureBlockCanary_local); 
        trace_func( NULL, "Enclosure Data, OverallStateChangeCount : %d\n", overallStateChangeCount_local); 
        trace_func( NULL, "Enclosure Data, Max no of ComponentTypes    : %d\n", maxNumberComponentTypes_local);  
        trace_func( NULL, "Enclosure Data, availableDataSpace : %d\n", availableDataSpace_local); 
        trace_func( NULL, "Enclosure Data, blockSize : %d\n", blockSize_local); 
        trace_func( NULL, "Enclosure Data, GenerationCount : %d\n", generation_count_local);

        componentTypeDataPtr = pe_enclosureComponentTypeBlock_p + enclosure_component_block_size;

        if (FBE_MEMORY_CHUNK_SIZE < blockSize_local)
        {
            trace_func( NULL, "Block Size exceeded the Max Limit: %d\n", FBE_MEMORY_CHUNK_SIZE); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        FBE_READ_MEMORY(componentTypeDataPtr, &enclosure_component_local, (blockSize_local - enclosure_component_block_size));

        for (loopCount = 0; loopCount < numberComponentTypes_local; loopCount++)
        {
            enclosure_component_data =  (fbe_enclosure_component_t *)&enclosure_component_local[enclosure_component_t_size * loopCount];
            
            enclosure_access_printSpecificComponentData(enclosure_component_data, 
                                                        enclosureType_local,
                                                        FBE_ENCL_COMPONENT_INDEX_ALL,
                                                        TRUE,
                                                        trace_func);
        }// end of for loop*/
        pe_enclosureComponentTypeBlock_p = pnextblock_p; 
        componentTypeDataPtr = NULL;
    }
   return FBE_STATUS_OK;
}
