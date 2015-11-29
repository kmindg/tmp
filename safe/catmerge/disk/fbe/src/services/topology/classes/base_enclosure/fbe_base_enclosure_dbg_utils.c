/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_enclosure_dbg_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains the utility functions for base enclosure.
 *  
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   02-May-2009 Dipak Patel - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_base_enclosure_debug.h"
#include "base_enclosure_private.h"


/*!**************************************************************
 * @fn fbe_base_enclosure_print_enclosure_number(
 *                         fbe_u32_t enclNo, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print enclNo variable of base
 *  enclosure.
 *
 * @param enclNo - enclosure_number.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_enclosure_number(fbe_u32_t enclNo, 
                                                         fbe_trace_func_t trace_func, 
                                                         fbe_trace_context_t trace_context)
{
   trace_func(trace_context, "EnclNumber: %d\n", enclNo);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_port_number(
 *                         fbe_u32_t portNo, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print enclNo variable of base
 *  enclosure.
 *
 * @param portNo - port_number.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_port_number(fbe_u32_t portNo, 
                                                         fbe_trace_func_t trace_func, 
                                                         fbe_trace_context_t trace_context)

{
    trace_func(trace_context, "PortNumber: %d\n", portNo);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_number_of_slots(
 *                         fbe_u32_t slots, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print enclNo variable of base
 *  enclosure.
 *
 * @param slots - number_of_slots.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_number_of_slots(fbe_u32_t slots, 
                                                         fbe_trace_func_t trace_func, 
                                                         fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "NumberOfSlots: %d\n", slots);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_first_slot_index(
 *                         fbe_u16_t firstSlotIndex, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print first_slot_index variable of base
 *  enclosure.
 *
 * @param firstSlotIndex - first_slot_index.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_first_slot_index(fbe_u16_t firstSlotIndex, 
                                                                 fbe_trace_func_t trace_func, 
                                                                 fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "FirstSlotIndex: %d\n", firstSlotIndex);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_first_expansion_port_index(
 *                         fbe_u16_t firstExpPortIndex, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print first_expansion_port_index variable of base
 *  enclosure.
 *
 * @param firstExpPortIndex - first_expansion_port_index.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_first_expansion_port_index(fbe_u16_t firstExpPortIndex, 
                                                                              fbe_trace_func_t trace_func, 
                                                                              fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "FirstExpPortIndex: %d\n", firstExpPortIndex);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_number_of_expansion_ports(
 *                         fbe_u32_t expPorts, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print number_of_expansion_ports variable of base
 *  enclosure.
 *
 * @param expPorts - number_of_expansion_ports.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_number_of_expansion_ports(fbe_u32_t expPorts, 
                                                                     fbe_trace_func_t trace_func, 
                                                                     fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "NumberOfExpansionPorts: %d\n", expPorts);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_time_ride_through_start(
 *                         fbe_time_t timeRideThroughStart, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print time_ride_through_start variable of base
 *  enclosure.
 *
 * @param timeRideThroughStart - time_ride_through_start.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_time_ride_through_start(fbe_time_t timeRideThroughStart, 
                                                                                      fbe_trace_func_t trace_func, 
                                                                                      fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "TimeRideThroughStart: %lld\n", timeRideThroughStart);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_expected_ride_through_period(
 *                         fbe_u32_t expectedRideThroughPeriod,  
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print expected_ride_through_period variable of base
 *  enclosure.
 *
 * @param expectedRideThroughPeriod - expected_ride_through_period.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_expected_ride_through_period(fbe_u32_t expectedRideThroughPeriod, 
                                                                          fbe_trace_func_t trace_func, 
                                                                          fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "ExpectedRideThroughPeriod: %d\n", expectedRideThroughPeriod);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_default_ride_through_period(
 *                         fbe_u32_t defaultRideThroughPeriod, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print default_ride_through_period variable of base
 *  enclosure.
 *
 * @param defaultRideThroughPeriod - default_ride_through_period.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/
void fbe_base_enclosure_print_default_ride_through_period(fbe_u32_t defaultRideThroughPeriod, 
                                                                          fbe_trace_func_t trace_func, 
                                                                          fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "DefaultRideThroughPeriod: %d\n", defaultRideThroughPeriod);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_number_of_drives_spinningup(
 *                         fbe_u32_t number_of_drives_spinningup, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print number_of_drives_spinningup variable of base
 *  enclosure.
 *
 * @param number_of_drives_spinningup - number_of_drives_spinningup.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
*   12/04/2009 PHE - Created.
 *
 ****************************************************************/
void fbe_base_enclosure_print_number_of_drives_spinningup(fbe_u32_t number_of_drives_spinningup, 
                                                                          fbe_trace_func_t trace_func, 
                                                                          fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "NumberOfDrivesSpinningup: %d\n", number_of_drives_spinningup);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_max_number_of_drives_spinningup(
 *                         fbe_u32_t max_number_of_drives_spinningup, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print max_number_of_drives_spinningup variable of base
 *  enclosure.
 *
 * @param defaultRideThroughPeriod - max_number_of_drives_spinningup.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   12/04/2009 PHE - Created. 
 *
 ****************************************************************/
void fbe_base_enclosure_print_max_number_of_drives_spinningup(fbe_u32_t max_number_of_drives_spinningup, 
                                                                          fbe_trace_func_t trace_func, 
                                                                          fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "MaxNumberOfDrivesSpinningup: %d\n", max_number_of_drives_spinningup);
}
/*!**************************************************************
 * @fn fbe_base_enclosure_print_component_id(
 *                         fbe_u32_t componentid, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print component_id variable of base
 *  enclosure.
 *
 * @param componentid - component_id.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 *
 ****************************************************************/
void fbe_base_enclosure_print_component_id(fbe_enclosure_component_id_t componentid, 
                                                                          fbe_trace_func_t trace_func, 
                                                                          fbe_trace_context_t trace_context)
{
    trace_func(trace_context, "Component_ID: %d\n", componentid);
}
/*!**************************************************************
 * @fn fbe_base_enclosure_print_encl_fault_symptom(
 *                         fbe_u32_t flt_symptom, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function prints enclosure fault symptom.
 *  enclosure.
 *
 * @param flt_symptom - fault symptom to be printed.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   05/04/2009:  Created. Nayana Chaudhari
 *
 ****************************************************************/
void fbe_base_enclosure_print_encl_fault_symptom(fbe_u32_t flt_symptom, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
    char * fltSymptom;

    switch (flt_symptom)
    {
        case FBE_ENCL_FLTSYMPT_NONE:
            fltSymptom = (char *)("No Fault");
            break;
        case FBE_ENCL_FLTSYMPT_DUPLICATE_UID:
            fltSymptom = (char *)("Duplicate UID");
            break;
        case FBE_ENCL_FLTSYMPT_ALLOCATED_MEMORY_INSUFFICIENT:
            fltSymptom = (char *)("Allocated Memory Insufficient");
            break;
        case FBE_ENCL_FLTSYMPT_PROCESS_PAGE_FAILED:
            fltSymptom = (char *)("Process Page Failed");
            break;
        case FBE_ENCL_FLTSYMPT_CMD_FAILED:
            fltSymptom = (char *)("Command Failed");
            break;
        case FBE_ENCL_FLTSYMPT_ENCL_FUNC_UNSUPPORTED:
            fltSymptom = (char *)("Unsupported Function");
            break;
        case FBE_ENCL_FLTSYMPT_PARAMETER_INVALID:
            fltSymptom = (char *)("Invalid Parameter");
            break;
        case FBE_ENCL_FLTSYMPT_HARDWARE_ERROR:
            fltSymptom = (char *)("Hardware Error");
            break;
        case FBE_ENCL_FLTSYMPT_CDB_REQUEST_FAILED:
            fltSymptom = (char *)("CDB Request Failed");
            break;
        case FBE_ENCL_FLTSYMPT_LIFECYCLE_FAILED:
            fltSymptom = (char *)("Lifecycle Failed");
            break;
        case FBE_ENCL_FLTSYMPT_EDAL_FAILED:
            fltSymptom = (char *)("EDAL Failed");
            break;
        case FBE_ENCL_FLTSYMPT_CONFIG_INVALID:
            fltSymptom = (char *)("Invalid Configuration");
            break;
        case FBE_ENCL_FLTSYMPT_MAPPING_FAILED:
            fltSymptom = (char *)("Mapping Failed");
            break;
        case FBE_ENCL_FLTSYMPT_COMP_UNSUPPORTED:
            fltSymptom = (char *)("Unsupported Component");
            break;
        case FBE_ENCL_FLTSYMPT_ESES_PAGE_INVALID:
            fltSymptom = (char *)("Invalid ESES Page");
            break;
        case FBE_ENCL_FLTSYMPT_CTRL_CODE_UNSUPPORTED:
            fltSymptom = (char *)("Unsupported Control Code");
            break;
        case FBE_ENCL_FLTSYMPT_DATA_ILLEGAL:
            fltSymptom = (char *)("Illegal Data");
            break;
        case FBE_ENCL_FLTSYMPT_BUSY:
            fltSymptom = (char *)("Busy");
            break;
        case FBE_ENCL_FLTSYMPT_DATA_ACCESS_FAILED:
            fltSymptom = (char *)("Data Access Failed");
            break;
        case FBE_ENCL_FLTSYMPT_UNSUPPORTED_PAGE_HANDLED:
            fltSymptom = (char *)("Unsupported Page Handled");
            break;
        case FBE_ENCL_FLTSYMPT_MODE_SELECT_NEEDED:
            fltSymptom = (char *)("Mode Select Needed");
            break;
        case FBE_ENCL_FLTSYMPT_MEM_ALLOC_FAILED:
            fltSymptom = (char *)("Memory Allocation Failed");
            break;
        case FBE_ENCL_FLTSYMPT_PACKET_FAILED:
            fltSymptom = (char *)("Packet Failed");
            break;
        case FBE_ENCL_FLTSYMPT_EDAL_NOT_NEEDED:
            fltSymptom = (char *)("EDAL Not Needed");
            break;
        case FBE_ENCL_FLTSYMPT_TARGET_NOT_FOUND:
            fltSymptom = (char *)("Target Not Found");
            break;
        case FBE_ENCL_FLTSYMPT_ELEM_GROUP_INVALID:
            fltSymptom = (char *)("Invalid Element Group");
            break;
        case FBE_ENCL_FLTSYMPT_PATH_ATTR_UNAVAIL:
            fltSymptom = (char *)("Path Attribute Unavailable");
            break;
        case FBE_ENCL_FLTSYMPT_CHECKSUM_ERROR:
            fltSymptom = (char *)("Checksum Error");
            break;
        case FBE_ENCL_FLTSYMPT_ILLEGAL_REQUEST:
            fltSymptom = (char *)("Illegal Request");
            break;
        case FBE_ENCL_FLTSYMPT_BUILD_CDB_FAILED:
            fltSymptom = (char *)("Build CDB Failed");
            break;
        case FBE_ENCL_FLTSYMPT_BUILD_PAGE_FAILED:
            fltSymptom = (char *)("Build Page Failed");
            break;
        case FBE_ENCL_FLTSYMPT_NULL_PTR:
            fltSymptom = (char *)("Null Pointer Error");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID_CANARY:
            fltSymptom = (char *)("Invalid Canary Error");
            break;
        case FBE_ENCL_FLTSYMPT_EDAL_ERROR:
            fltSymptom = (char *)("EDAL Error");
            break;
        case FBE_ENCL_FLTSYMPT_COMPONENT_NOT_FOUND:
            fltSymptom = (char *)("Component Not Found");
            break;
        case FBE_ENCL_FLTSYMPT_ATTRIBUTE_NOT_FOUND:
            fltSymptom = (char *)("Attribute Not Found");
            break;
        case FBE_ENCL_FLTSYMPT_COMP_TYPE_INDEX_INVALID:
            fltSymptom = (char *)("Invalid Component Type Index");
            break;
        case FBE_ENCL_FLTSYMPT_COMP_TYPE_UNSUPPORTED:
            fltSymptom = (char *)("Unsupported Component Type");
            break;
        case FBE_ENCL_FLTSYMPT_UNSUPPORTED_ENCLOSURE:
            fltSymptom = (char *)("Unsupported Enclosure");
            break;
        case FBE_ENCL_FLTSYMPT_INSUFFICIENT_RESOURCE:
            fltSymptom = (char *)("Insufficient Resource");
            break;
        case FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED:
            fltSymptom = (char *) ("Component Index Mapping Failed");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID_ENCL_PLATFORM_TYPE:
            fltSymptom = (char *) ("Invalid Enclosure Platform Type");
            break;
        case FBE_ENCL_FLTSYMPT_MINIPORT_FAULT_ENCL:
            fltSymptom = (char *) ("Miniport Fault Enclosure");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID_ESES_VERSION_DESCRIPTOR:
            fltSymptom = (char *) ("Invalid ESES version descriptor");
            break;
        case FBE_ENCL_FLTSYMPT_EXCEEDS_MAX_LIMITS:
            fltSymptom = (char *) ("Exceeds platform limits");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID:
        default:
            fltSymptom = (char *)( "Unknown ");
        break;
    }
    trace_func(trace_context, "FaultSymptom: %s\n", fltSymptom);
}

// End of file fbe_base_enclosure_utils.c