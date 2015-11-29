/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sps_mgmt_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the SPS Mgmt object.
 *
 * @author
 *  06-May-2010 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object.h"
#include "fbe_sps_mgmt_debug.h"
#include "fbe_sps_mgmt_private.h"
#include "fbe/fbe_sps_info.h"
#include "fbe_sps_debug.h"

/*!**************************************************************
 * @fn fbe_sps_mgmt_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr sps_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the virtual drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param sps_mgmt_p - Ptr to SPS Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  06-May-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr sps_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{

    /* Display the object id.
     */
    trace_func(trace_context, "  object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          sps_mgmt_p,
                                          trace_func,
                                          trace_context);
    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         sps_mgmt_p,
                                         trace_func,
                                         trace_context);
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      sps_mgmt_p,
                                      trace_func,
                                      trace_context);
    
    trace_func(trace_context, "\n");

   /* Display the Trace Level.
    */
    trace_func(trace_context, "  trace_level: ");
    fbe_base_object_debug_trace_level(module_name,
                                      sps_mgmt_p,
                                      trace_func,
                                      trace_context);
    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_debug_stat(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr sps_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the debug  iformation about the sps mgmt  object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param sps_mgmt_p - Ptr to SPS Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  09 -Aug-2010 - Created. Vishnu Sharma
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_debug_stat(const fbe_u8_t * module_name, 
                                     fbe_dbgext_ptr sps_mgmt_p,
                                     fbe_trace_func_t trace_func, 
                                     fbe_trace_context_t trace_context)
{
    fbe_sps_mgmt_t sps_mgmt_object;
    fbe_u32_t spsIndex = 0;
    fbe_u32_t offset;
    fbe_object_id_t object_id;
    fbe_u32_t total_sps_count;
    fbe_dbgext_ptr arraySpsInfo_ptr = 0;
    fbe_dbgext_ptr sps_info_ptr = 0;
    fbe_u32_t fbe_sps_info_size;
    SPS_STATE spsStatus;
    fbe_sps_fault_info_t spsFaultInfo;
    fbe_sps_cabling_status_t spsCablingStatus;
    fbe_dbgext_ptr spsManufInfo_ptr = 0;
    fbe_u8_t spsSerialNumber[FBE_SPS_SERIAL_NUM_REVSION_SIZE];
    fbe_u8_t spsPartNumber[FBE_SPS_PART_NUM_REVSION_SIZE];
    fbe_u8_t spsPartNumRevision[FBE_SPS_PART_NUM_REVSION_SIZE];
    fbe_u8_t spsVendor[FBE_SPS_VENDOR_NAME_SIZE];
    fbe_u8_t spsVendorModelNumber[FBE_SPS_VENDOR_PART_NUMBER_SIZE];
    fbe_u8_t spsFirmwareRevision[FBE_SPS_FW_REVISION_SIZE];
    fbe_u8_t spsModelString[FBE_SPS_MODEL_STRING_SIZE + 1] = {'\0'};
    fbe_system_time_t spsTestTime;
    fbe_sps_testing_state_t spsTestState;
    fbe_bool_t needToTest;
    fbe_s8_t timeString[8] = {'\0'};
    fbe_device_physical_location_t spsLocation;

    FBE_GET_FIELD_OFFSET(module_name, fbe_sps_mgmt_t, "object_id", &offset);
    FBE_READ_MEMORY(sps_mgmt_p + offset, &object_id, sizeof(fbe_object_id_t));
    trace_func(trace_context, "SPS INFO:");
    trace_func(trace_context, "\n");
    trace_func(trace_context, "Object ID:0x%x",object_id);
    FBE_GET_FIELD_OFFSET(module_name, fbe_sps_mgmt_t, "total_sps_count", &offset);
    FBE_READ_MEMORY(sps_mgmt_p + offset, &total_sps_count, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context,4);
    trace_func(trace_context, "total_sps_count:0x%x",total_sps_count);

    FBE_READ_MEMORY(sps_mgmt_p, &sps_mgmt_object, sizeof(fbe_sps_mgmt_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_sps_mgmt_t, "arraySpsInfo", &offset);
    arraySpsInfo_ptr = sps_mgmt_p + offset;
    FBE_GET_FIELD_OFFSET(module_name, fbe_array_sps_info_t, "sps_info", &offset);
    sps_info_ptr = arraySpsInfo_ptr + offset;
    FBE_GET_TYPE_SIZE(module_name, fbe_sps_info_t, &fbe_sps_info_size);

    spsLocation.bus = spsLocation.enclosure = 0;
    // display information about local sps as well as  peer sps.
    for(spsIndex= FBE_LOCAL_SPS; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
    {
        spsLocation.slot = spsIndex;
        trace_func(trace_context, "\n");
        trace_func(trace_context,"%s information:\n",fbe_sps_mgmt_getSpsString(&spsLocation));
        
        sps_info_ptr += (fbe_sps_info_size * spsIndex);

        FBE_GET_FIELD_OFFSET(module_name, fbe_sps_info_t, "spsStatus", &offset);
        FBE_READ_MEMORY(sps_info_ptr + offset, &spsStatus, sizeof(SPS_STATE));

        trace_func(trace_context, "\n");
        trace_func(trace_context, "SPS status: %s",fbe_sps_mgmt_getSpsStatusString(spsStatus));

        FBE_GET_FIELD_OFFSET(module_name, fbe_sps_info_t, "spsFaultInfo", &offset);
        FBE_READ_MEMORY(sps_info_ptr + offset, &spsFaultInfo, sizeof(fbe_sps_fault_info_t));
        fbe_trace_indent(trace_func, trace_context,4);
        trace_func(trace_context, "SPS Faults: %s", fbe_sps_mgmt_getSpsFaultString(&spsFaultInfo));
        
        FBE_GET_FIELD_OFFSET(module_name, fbe_sps_info_t, "spsCablingStatus", &offset);
        FBE_READ_MEMORY(sps_info_ptr + offset, &spsCablingStatus, sizeof(fbe_sps_cabling_status_t));
        fbe_trace_indent(trace_func, trace_context,4);
        trace_func(trace_context, "SPS Cabling: %s", fbe_sps_mgmt_getSpsCablingStatusString(spsCablingStatus));

        FBE_GET_FIELD_OFFSET(module_name, fbe_sps_info_t, "spsManufInfo", &offset);
        spsManufInfo_ptr = sps_info_ptr + offset;

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_sps_manuf_info_t, "spsSerialNumber", &offset);
        FBE_READ_MEMORY(spsManufInfo_ptr + offset, &spsSerialNumber, FBE_SPS_SERIAL_NUM_REVSION_SIZE);
        trace_func(trace_context, "\n");
        fbe_trace_indent(trace_func, trace_context,4);
        trace_func(trace_context,"SPS Manufacturing Info\n");
        fbe_trace_indent(trace_func, trace_context,8);
        trace_func(trace_context,"SerialNumber:%s",spsSerialNumber);

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_sps_manuf_info_t, "spsPartNumber", &offset);
        FBE_READ_MEMORY(spsManufInfo_ptr + offset, &spsPartNumber, FBE_SPS_PART_NUM_REVSION_SIZE);
        fbe_trace_indent(trace_func, trace_context,4);
        trace_func(trace_context,"PartNumber:%s",spsPartNumber);

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_sps_manuf_info_t, "spsPartNumRevision", &offset);
        FBE_READ_MEMORY(spsManufInfo_ptr + offset, &spsPartNumRevision, FBE_SPS_PART_NUM_REVSION_SIZE);
        fbe_trace_indent(trace_func, trace_context,4);
        trace_func(trace_context,"PartNumRevision:%s",spsPartNumRevision);

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_sps_manuf_info_t, "spsVendor", &offset);
        FBE_READ_MEMORY(spsManufInfo_ptr + offset, &spsVendor, FBE_SPS_VENDOR_NAME_SIZE);
        fbe_trace_indent(trace_func, trace_context,4);
        trace_func(trace_context,"Vendor:%s",spsVendor);

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_sps_manuf_info_t, "spsVendorModelNumber", &offset);
        FBE_READ_MEMORY(spsManufInfo_ptr + offset, &spsVendorModelNumber, FBE_SPS_VENDOR_PART_NUMBER_SIZE);
        trace_func(trace_context, "\n");
        fbe_trace_indent(trace_func, trace_context,8);
        trace_func(trace_context,"Vendor Model Num:%s",spsVendorModelNumber);

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_sps_manuf_info_t, "spsFirmwareRevision", &offset);
        FBE_READ_MEMORY(spsManufInfo_ptr + offset, &spsFirmwareRevision, FBE_SPS_FW_REVISION_SIZE);
        fbe_trace_indent(trace_func, trace_context,4);
        trace_func(trace_context,"FwRevision:%s",spsFirmwareRevision);

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_board_sps_manuf_info_t, "spsModelString", &offset);
        FBE_READ_MEMORY(spsManufInfo_ptr + offset, &spsModelString, FBE_SPS_MODEL_STRING_SIZE);
        fbe_trace_indent(trace_func, trace_context,4);
        
        if(strlen(spsModelString) > 0)
        {
            trace_func(NULL,"ModelString: %.8s",spsModelString);
        }
        else
        {
            trace_func(NULL,"ModelString: %s","NULL");
        }
        trace_func(trace_context, "\n");
    }
    //Print SPS testing details
    trace_func(trace_context, "\n");
    trace_func(trace_context,"SPS test details:");

    FBE_GET_FIELD_OFFSET(module_name, fbe_array_sps_info_t, "spsTestState", &offset);
    FBE_READ_MEMORY(arraySpsInfo_ptr + offset, &spsTestState, sizeof(fbe_sps_testing_state_t));
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context,4);
	trace_func(trace_context, "spsTestState: %s",
		       fbe_sps_mgmt_get_sps_test_state_string(spsTestState,trace_func,trace_context));
    
    FBE_GET_FIELD_OFFSET(module_name, fbe_array_sps_info_t, "needToTest", &offset);
    FBE_READ_MEMORY(arraySpsInfo_ptr + offset, &needToTest, sizeof(fbe_bool_t));
    fbe_trace_indent(trace_func, trace_context,4);
    trace_func(trace_context, "needToTest:0x%x",needToTest);
    
    FBE_GET_FIELD_OFFSET(module_name, fbe_array_sps_info_t, "spsTestTime", &offset);
    FBE_READ_MEMORY(arraySpsInfo_ptr + offset, &spsTestTime, sizeof(fbe_system_time_t));
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context,4);
	fbe_getTimeString(&spsTestTime,timeString);
    trace_func(trace_context,"\nWeekly SPS Test Time: %s %s\n",
               fbe_getWeekDayString(spsTestTime.weekday),
               timeString);

    return FBE_STATUS_OK; 
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_get_sps_test_state_string()
 ****************************************************************
 * @brief
 *  Function to convert the SPS test state enum into a string.
 *
 * @param spsTestState - SPS test state.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  19-Sep-2011 - Created. Omer Miranda
 *
 ****************************************************************/
char *fbe_sps_mgmt_get_sps_test_state_string (fbe_sps_testing_state_t spsTestState,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    char *spsTestStateStringTemp;

    switch (spsTestState)
    {
        case FBE_SPS_NO_TESTING_IN_PROGRESS:
            spsTestStateStringTemp = "NO_TESTING_IN_PROGRESS";
            break;
        case FBE_SPS_LOCAL_SPS_TESTING:
            spsTestStateStringTemp = "LOCAL_SPS_TESTING";
            break;
        case FBE_SPS_PEER_SPS_TESTING:
            spsTestStateStringTemp = "PEER_SPS_TESTING";
            break;
        case FBE_SPS_WAIT_FOR_PEER_PERMISSION:
            spsTestStateStringTemp = "WAIT_FOR_PEER_PERMISSION";
            break;
        case FBE_SPS_WAIT_FOR_PEER_TEST:
            spsTestStateStringTemp = "WAIT_FOR_PEER_TEST";
            break;
        case FBE_SPS_WAIT_FOR_STABLE_ARRAY:
            spsTestStateStringTemp = "WAIT_FOR_STABLE_ARRAY";
            break;
        default:
            trace_func(trace_context, "Undefined:0x%x",spsTestState);
            break;
    }
    return spsTestStateStringTemp;
}
/********************************************
 * end fbe_sps_mgmt_get_sps_test_state_string
 ********************************************/

