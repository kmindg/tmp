/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_utils.c
 ***************************************************************************
 *
 *  Description
 *      common utilities to be used in FBE API      
 *
 *  History:
 *      10/14/08    sharel - Created
 *      09/10/09    guov - change the wait functions to use fixed polling interval 
 *    
 ***************************************************************************/
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe/fbe_api_cmi_interface.h"

/**************************************
                Local variables
**************************************/
#define FBE_API_POLLING_INTERVAL 100 /*ms*/

/*******************************************
                Local functions
********************************************/


/*********************************************************************
 *  fbe_api_wait_for_object_lifecycle_state ()
 *********************************************************************
 *
 *  Description: this function poll the physical package on the object lifecycle state
 *               and compares with the given state.  If match, return success.
 *               If no match within given time, return fail.
 *
 *  Inputs: obj_id
 *          expected_lifecycle_state
 *          timeout_ms - timeout value in milo-second
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/15/08: guov Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_object_lifecycle_state (fbe_object_id_t obj_id, fbe_lifecycle_state_t expected_lifecycle_state, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_lifecycle_state_t   current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_object_lifecycle_state(obj_id, &current_state, package_id);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }

        if ((status == FBE_STATUS_NO_OBJECT) && (expected_lifecycle_state == FBE_LIFECYCLE_STATE_NOT_EXIST)) {
            return FBE_STATUS_OK;
        }
            
        if (current_state == expected_lifecycle_state){
            return FBE_STATUS_OK;
        }
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "%s: object 0x%x expected state %d != current state %d in %d ms!\n", 
                  __FUNCTION__, obj_id, expected_lifecycle_state, current_state, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            fbe_api_wait_for_object_lifecycle_state_warn_trace ()
 *********************************************************************
 *
 *  Description: this function poll the physical package on the object lifecycle state
 *               and compares with the given state.  If match, return success.
 *               If no match within given time, return fail but only make warning trace.
 *
 *  Inputs: obj_id
 *          expected_lifecycle_state
 *          timeout_ms - timeout value in milo-second
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    10/25/11: zphu Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_object_lifecycle_state_warn_trace (fbe_object_id_t obj_id, fbe_lifecycle_state_t expected_lifecycle_state, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_lifecycle_state_t   current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_object_lifecycle_state(obj_id, &current_state, package_id);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }

        if ((status == FBE_STATUS_NO_OBJECT) && (expected_lifecycle_state == FBE_LIFECYCLE_STATE_NOT_EXIST)) {
            return FBE_STATUS_OK;
        }
            
        if (current_state == expected_lifecycle_state){
            return FBE_STATUS_OK;
        }
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "api_wait_obj_state: obj 0x%x expect state %d != cur state %d in %d ms!\n", 
                  obj_id, expected_lifecycle_state, current_state, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            fbe_api_wait_for_object_to_transition_from_lifecycle_state ()
 *********************************************************************
 *
 *  Description: this function poll on the object lifecycle state
 *               and compares with the given state.  If does not match, return success.
 *               If still matches within given time, return fail.
 *
 *  Inputs: obj_id
 *          lifecycle_state
 *          timeout_ms - timeout value in milo-second
 *          package_id
 *  Outputs: None
 *
 *  Return Value: success or failure
 *    
 *********************************************************************/
fbe_status_t fbe_wait_for_object_to_transiton_from_lifecycle_state(fbe_object_id_t obj_id, fbe_lifecycle_state_t lifecycle_state, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t               current_time = 0;
    fbe_lifecycle_state_t   current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_object_lifecycle_state(obj_id, &current_state, package_id);

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
            return status;
        }

        if ((status == FBE_STATUS_NO_OBJECT) && (lifecycle_state != FBE_LIFECYCLE_STATE_NOT_EXIST)) { 
            return FBE_STATUS_OK;
        }
            
        if (current_state != lifecycle_state){
            return FBE_STATUS_OK;
        }
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "%s: object 0x%x didn't transtion from state %d in %d ms!\n", 
                  __FUNCTION__, obj_id,lifecycle_state,  timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            fbe_api_wait_for_number_of_objects ()
 *********************************************************************
 *
 *  Description: this function poll the physical package using enumerate objects
 *               and compares with the expected number of objects.  If match, return success.
 *               If no match within given time, return fail.
 *
 *  Inputs: expected_number
 *          timeout_ms - timeout value in milo-second
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/15/08: guov Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_number_of_objects (fbe_u32_t expected_number, fbe_u32_t timeout_ms, fbe_package_id_t package_id)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t           current_time = 0;
    fbe_u32_t           total_objects = 0;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_total_objects(&total_objects, package_id);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Enumerate objects failed with status %d\n", __FUNCTION__, status);
            return status;
        }
            
        if (total_objects == expected_number){
            return FBE_STATUS_OK;
        }
        current_time += FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "%s: expected number of objects %d != current number %d in %d ms!\n", 
                  __FUNCTION__, expected_number, total_objects, timeout_ms);

    return FBE_STATUS_TIMEOUT;
}
/*!**************************************************************
 * fbe_api_wait_for_number_of_class()
 ****************************************************************
 * @brief
 *  Wait for this number of objects of this class to exist.
 *
 * @param class_id - class of objects to wait for.
 * @param expected_number_of_objects -Number of objects to poll for
 * @param timeout_ms - Timeout in milliseconds.
 * @param package_id - package to check.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/16/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_api_wait_for_number_of_class(fbe_class_id_t class_id, 
                                              fbe_u32_t expected_number_of_objects,
                                              fbe_u32_t timeout_ms, 
                                              fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t current_time = 0;
    fbe_u32_t total_objects = 0;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_total_objects_of_class(class_id, package_id, &total_objects);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Enumerate objects failed with status %d\n", __FUNCTION__, status);
            return status;
        }
            
        if (total_objects == expected_number_of_objects){
            return status;
        }
        current_time += FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                  "%s: expected number of objects %d != current number %d in %d ms!\n", 
                  __FUNCTION__, expected_number_of_objects, total_objects, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end fbe_api_wait_for_number_of_class()
 ******************************************/

/*********************************************************************
 *            fbe_api_wait_for_block_edge_path_state()
 *********************************************************************
 *
 *  Description: This function poll the Package for the particular
 *  block edge path state.
 *
 *  Inputs: object_id  - Object id where you send usurper command.
 *          edge_idex  - index of the edge to wait for.
 *          package_id - package id to wait for.
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure status.
 *
 *  History:
 *    10/22/08: Dhaval Patel Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_block_edge_path_state(fbe_object_id_t object_id, 
                                                    fbe_u32_t edge_index,
                                                    fbe_u32_t expected_path_state, 
                                                    fbe_u32_t package_id,
                                                    fbe_u32_t timeout_ms)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_api_get_block_edge_info_t       edge_info;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_block_edge_info(object_id, edge_index, &edge_info, package_id);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s: Discovery edge info returned with status %d\n", __FUNCTION__, status); 
            return status;
        }
            
        if (edge_info.path_state == expected_path_state){
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
         fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: block edge (index %d) path state not %d in %d ms (path state is %d)\n", 
                  __FUNCTION__, edge_index, expected_path_state, timeout_ms, edge_info.path_state);

    return FBE_STATUS_TIMEOUT;
}


/*********************************************************************
 *            fbe_api_wait_for_block_edge_path_attributes()
 *********************************************************************
 *
 *  Description: This function poll the Package for the particular
 *  block edge path attributes.
 *
 *  Inputs: object_id  - Object id where you send usurper command.
 *          edge_idex  - index of the edge to wait for.
 *          package_id - package id to wait for.
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure status.
 *
 *  History:
 *    10/22/08: Dhaval Patel Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_block_edge_path_attribute(fbe_object_id_t object_id, 
                                                        fbe_u32_t edge_index,
                                                        fbe_u32_t expected_path_attribute, 
                                                        fbe_u32_t package_id,
                                                        fbe_u32_t timeout_ms)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_api_get_block_edge_info_t       edge_info;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_block_edge_info(object_id, edge_index, &edge_info, package_id);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s: Discovery edge info returned with status %d\n", __FUNCTION__, status); 
            return status;
        }
            
        if (edge_info.path_attr & expected_path_attribute){
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
         fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: block edge (index %d) path attribute %d is not set within the expected %d ms!\n", 
                  __FUNCTION__, edge_index, expected_path_attribute, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            fbe_api_wait_for_block_edge_path_attribute_cleared()
 *********************************************************************
 *
 *  Description: This function poll the Package for the particular
 *  block edge path attribute to be cleared
 *
 *  Inputs: object_id  - Object id where you send usurper command.
 *          edge_idex  - index of the edge to wait for.
 *          package_id - package id to wait for.
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure status.
 *
 *  History:
 *    10/27/15: Deanna Heng 
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_block_edge_path_attribute_cleared(fbe_object_id_t object_id, 
                                                                fbe_u32_t edge_index,
                                                                fbe_u32_t cleared_path_attribute, 
                                                                fbe_u32_t package_id,
                                                                fbe_u32_t timeout_ms)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_api_get_block_edge_info_t       edge_info;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_block_edge_info(object_id, edge_index, &edge_info, package_id);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s: Discovery edge info returned with status %d\n", __FUNCTION__, status); 
            return status;
        }
            
        if ((edge_info.path_attr & cleared_path_attribute) == 0){
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
         fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: block edge (index %d) path attribute %d is not cleared within the expected %d ms!\n", 
                  __FUNCTION__, edge_index, cleared_path_attribute, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}


/*********************************************************************
 *            fbe_api_wait_for_discovery_protocol_edge_attr ()
 *********************************************************************
 *
 *  Description: This function poll the physical package for the 
 *                  discovery edge for NOT_RPESENT attribute.
 *
 *  Inputs: expected_number
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/28/08: Arunkumar Selvapillai Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_discovery_edge_attr(fbe_object_id_t edge_handle_p, 
                                                    fbe_u32_t expected_attr, fbe_u32_t timeout_ms)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_api_get_discovery_edge_info_t   edge_info;
    
    while (current_time < timeout_ms){
        status = fbe_api_get_discovery_edge_info(edge_handle_p, &edge_info);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s: Discovery edge info returned with status %d\n", __FUNCTION__, status); 
            return status;
        }
            
        if (edge_info.path_attr & expected_attr){
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
         fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: Discovery edge attribute %d is not set within the expected %d ms!\n", 
                  __FUNCTION__, expected_attr, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            fbe_api_wait_for_protocol_edge_attr ()
 *********************************************************************
 *
 *  Description: This function poll the physical package for the 
 *                  Protocol edge attribute for CLOSED attribute.
 *
 *  Inputs: expected_number
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    08/28/08: Arunkumar Selvapillai Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_protocol_edge_attr(fbe_object_id_t edge_handle_p, fbe_u32_t expected_attr, fbe_u32_t timeout_ms)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                       current_time = 0;
    fbe_api_get_ssp_edge_info_t     ssp_transport_edge_info;

    while (current_time < timeout_ms){
        status = fbe_api_get_ssp_edge_info(edge_handle_p, &ssp_transport_edge_info);
        
        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Protocol edge info returned with status %d\n", __FUNCTION__, status); 
            return status;
        }
            
        if (ssp_transport_edge_info.path_attr == expected_attr){
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }
    
    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: Protocol edge attribute %d is not set within the expected %d ms!\n", 
                  __FUNCTION__, expected_attr, timeout_ms); 

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *            fbe_api_wait_for_fault_attr ()
 *********************************************************************
 *
 *  Description: This function poll the physical package for the 
 *                  fault attribute for a specified component.
 *                  (Components: Power Supply, Fan, OverTemp, etc)
 *
 *  Inputs: expected_number
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    09/22/08: Arunkumar Selvapillai Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_fault_attr(fbe_object_id_t object_handle, 
                                         fbe_base_enclosure_attributes_t expected_attr,
                                         fbe_bool_t expected_value,
                                         fbe_enclosure_component_types_t component_type,
                                         fbe_u8_t component_index,
                                         fbe_u32_t timeout_ms)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_edal_status_t       edalStatus;
    fbe_u32_t               current_time = 0;
    fbe_bool_t              is_fault = FALSE;
    
    fbe_base_object_mgmt_get_enclosure_info_t   encl_info;

    while (current_time < timeout_ms)
    {
        /* Get the enclosure data blob */
        status = fbe_api_enclosure_get_info(object_handle, &encl_info);
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Cannot retrieve the Enclosure blob data\n", __FUNCTION__); 
            return status;
        }
        fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)&encl_info);
        switch (component_type)
        {
            case FBE_ENCL_POWER_SUPPLY:
            case FBE_ENCL_COOLING_COMPONENT:
            case FBE_ENCL_TEMP_SENSOR:
                edalStatus = fbe_edal_getBool(&encl_info, expected_attr, component_type, component_index, &is_fault);
                break;
            case FBE_ENCL_DRIVE_SLOT:
                edalStatus = fbe_edal_getDriveBool(&encl_info, expected_attr, component_index, &is_fault);
                break;
            break;
            default:
            {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                                "%s: Invalid Component requested: %d!\n", __FUNCTION__, component_type); 
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
 
        if (edalStatus != FBE_EDAL_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s: Component %d don't have the valid Fault status in %d ms!\n", 
                           __FUNCTION__, expected_attr, timeout_ms); 

            return FBE_STATUS_GENERIC_FAILURE;
        }
            
        if (is_fault == expected_value)
        {
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                    "%s: Attribute %d is not set within the expected %d ms!\n", 
                    __FUNCTION__, expected_attr, timeout_ms); 

    return FBE_STATUS_GENERIC_FAILURE;
}


/*********************************************************************
 *            fbe_api_get_encl_attr ()
 *********************************************************************
 *
 *  Description: This function gets the attribute values from the 
 *                  physical package for the enclosure component.
 *
 *  Inputs: expected_number
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/17/08: Arunkumar Selvapillai Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_get_encl_attr(fbe_object_id_t object_handle, 
                                         fbe_base_enclosure_attributes_t expected_attr,
                                         fbe_u32_t component,
                                         fbe_u32_t index, 
                                         fbe_u32_t *expected_value)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_edal_status_t       edalStatus;
    fbe_u8_t                edal_value_u8;
    fbe_base_object_mgmt_get_enclosure_info_t   *encl_info_p = NULL;

    encl_info_p = (fbe_base_object_mgmt_get_enclosure_info_t *) fbe_api_allocate_memory (sizeof (fbe_base_object_mgmt_get_enclosure_info_t));

    if (encl_info_p == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: encl_info_p buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    /* Get the enclosure data blob */
    status = fbe_api_enclosure_get_info(object_handle, encl_info_p);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, 
                        "%s: Cannot retrieve the Enclosure blob data\n", __FUNCTION__); 
        fbe_api_free_memory(encl_info_p);
        return status;
    }
    fbe_edal_updateBlockPointers((fbe_edal_block_handle_t)encl_info_p);
    switch (component)
    {
        case FBE_ENCL_ENCLOSURE:
        {
            switch (expected_attr)
            {
                case FBE_ENCL_GENERATION_CODE:
                    edalStatus = fbe_edal_getU32(encl_info_p, expected_attr, component, 0, expected_value);
                    break;

                case FBE_ENCL_ADDITIONAL_STATUS_PAGE_UNSUPPORTED:
                case FBE_ENCL_EMC_SPECIFIC_STATUS_PAGE_UNSUPPORTED:
                    edalStatus = fbe_edal_getBool(encl_info_p, expected_attr, component, 0, expected_value);
                    break;

                default:
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                                    "%s: Invalid Attribute requested: %d!\n", __FUNCTION__, expected_attr); 
                    fbe_api_free_memory(encl_info_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
        }
        break;
        case FBE_ENCL_DISPLAY:
        {
            switch (expected_attr)
            {
                case FBE_DISPLAY_CHARACTER_STATUS:
                    edalStatus = fbe_edal_getU8(encl_info_p, expected_attr, component, index, &edal_value_u8);
                    *expected_value = edal_value_u8;
                    break;

                default:
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                                    "%s: Invalid Attribute requested: %d!\n", __FUNCTION__, expected_attr); 
                    fbe_api_free_memory(encl_info_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
        }
        break;
        default:
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                            "%s: Invalid Component requested: %d!\n", __FUNCTION__, component); 
            fbe_api_free_memory(encl_info_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (edalStatus != FBE_EDAL_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                        "%s: Component %d don't have the valid value!\n", 
                        __FUNCTION__, expected_attr); 

        fbe_api_free_memory(encl_info_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_free_memory(encl_info_p);
    return status;
}

/*********************************************************************
 *            fbe_api_wait_for_encl_attr ()
 *********************************************************************
 *
 *  Description: This function poll the physical package for the 
 *                  attributes of the enclosure component.
 *
 *  Inputs: expected_number
 *          timeout_ms - timeout value in milo-second
 *
 *  Outputs: None
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/17/08: Arunkumar Selvapillai Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_encl_attr(fbe_object_id_t object_handle, 
                                         fbe_base_enclosure_attributes_t expected_attr,
                                         fbe_u32_t expected_value,
                                         fbe_u32_t component,
                                         fbe_u32_t index, 
                                         fbe_u32_t timeout_ms)
{
    fbe_u32_t       is_set = 0;
    fbe_u32_t       current_time = 0;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE; 
    
    while (current_time < timeout_ms)
    {
        /* Get the enclosure attribute value from the enclosure blob */
        status = fbe_api_get_encl_attr(object_handle, expected_attr, component, index, &is_set);
        
        if ( status != FBE_STATUS_OK && status != FBE_STATUS_BUSY) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                            "%s: Invalid status %d\n", 
                            __FUNCTION__, status); 
            return status;
        }

        if (is_set == expected_value) {
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                    "%s: Attribute %d is not set within the expected %d ms!\n", 
                    __FUNCTION__, expected_attr, timeout_ms); 

    return FBE_STATUS_GENERIC_FAILURE;
}


/*********************************************************************
 *  fbe_api_wait_for_virtual_drive_configuration_mode()
 *********************************************************************
 *
 *  Description: This function is used to poll and wait until it times
 *  out to get the expected configuration mode.
 *
 *  Inputs: object_id                    - virtual drive object-id.
 *          expected_configuration_mode  - expected configuration mode.
 *          current_configuration_mode_p - current config mode.
 *          timeout_ms                   - timeout value in milo-second.
 *
 *  Outputs: None
 *
 *  Return Value: success or failure status.
 *
 *  History:
 *    05/06/10: Dhaval Patel Created.
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id, 
                                                               fbe_virtual_drive_configuration_mode_t   expected_configuration_mode,
                                                               fbe_virtual_drive_configuration_mode_t * current_configuration_mode_p,
                                                               fbe_u32_t timeout_ms)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                                   current_time = 0;
    fbe_api_virtual_drive_get_configuration_t   get_configuration;
    fbe_class_id_t                              class_id;
    
    while (current_time < timeout_ms)
    {
        /* verify the class-id of the object to be virtual. */
        status = fbe_api_get_object_class_id(vd_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
        if ((status != FBE_STATUS_OK) || (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE))
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the configuration mode. */
        status = fbe_api_virtual_drive_get_configuration(vd_object_id, &get_configuration);        
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s: virtual drive get config failed with status:%d\n", __FUNCTION__, status); 
            return status;
        }
            
        /*  if we find the configuration mode gets updated as asked by user then return. */
        if (get_configuration.configuration_mode  == expected_configuration_mode)
        {
            *current_configuration_mode_p = get_configuration.configuration_mode;
            return FBE_STATUS_OK;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep(FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: vd obj: 0x%x mode: %d not expected: %d in %d ms\n", 
                  __FUNCTION__, vd_object_id, get_configuration.configuration_mode,
                  expected_configuration_mode, timeout_ms);
    *current_configuration_mode_p = get_configuration.configuration_mode;

    return FBE_STATUS_TIMEOUT;
}
/*********************************************************************
 *  fbe_api_wait_for_lun_peer_state_ready()
 *********************************************************************
 *
 *  Description: This function is used to poll and wait until it times
 *  out or lun reports peer lifecycle state ready.
 *
 *  Inputs: object_id                    - lun object-id.
 *          timeout_ms                   - timeout value in milo-second.
 *
 *  Outputs: None
 *
 *  Return Value: success or failure status.
 *
 *  History:
 *    04/25/11: Naizhong Chiu Created.
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_lun_peer_state_ready(fbe_object_id_t lun_object_id, 
                                                   fbe_u32_t timeout_ms)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                                   current_time = 0;
    fbe_api_lun_get_lun_info_t                  get_lu_info;
    fbe_class_id_t                              class_id;
	fbe_cmi_service_get_info_t           		get_cmi_info;
    
    while (current_time < timeout_ms)
    {
        /* verify the class-id of the object to be virtual. */
        status = fbe_api_get_object_class_id(lun_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
        if ((status != FBE_STATUS_OK) || (class_id != FBE_CLASS_ID_LUN))
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the configuration mode. */
        status = fbe_api_lun_get_lun_info(lun_object_id, &get_lu_info);        
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s: lun get config failed with status:%d\n", __FUNCTION__, status); 
            return status;
        }

		status = fbe_api_cmi_service_get_info(&get_cmi_info);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Failed to get CMI Info.\n",
                      __FUNCTION__);
            return status;
        }
            
        if (get_lu_info.peer_lifecycle_state  == FBE_LIFECYCLE_STATE_READY || get_cmi_info.peer_alive == FBE_FALSE){
            return FBE_STATUS_OK;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        /* wait */
        fbe_api_sleep(FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                   "%s: lun object(%d) peer lifecycle state is not ready within the expected %d ms!\n", 
                  __FUNCTION__, lun_object_id, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
 *  fbe_api_wait_for_rg_peer_state_ready()
 *********************************************************************
 *
 *  Description: This function is used to poll and wait until it times
 *  out or RG reports peer lifecycle state ready.
 *
 *  Inputs: object_id                    - RG object-id.
 *          timeout_ms                   - timeout value in milo-second.
 *
 *  Outputs: None
 *
 *  Return Value: success or failure status.
 *
 *  History:
 *    09/14/12: Created.
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_rg_peer_state_ready(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t timeout_ms)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                                   current_time = 0;
    fbe_api_raid_group_get_info_t               get_rg_info;
    fbe_class_id_t                              class_id;
	fbe_cmi_service_get_info_t           		get_cmi_info;
    
    while (current_time < timeout_ms)
    {
        /* verify the class-id of the object to be virtual. */
        status = fbe_api_get_object_class_id(rg_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, Failed to get object class id\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (class_id <= FBE_CLASS_ID_RAID_FIRST || class_id >= FBE_CLASS_ID_RAID_LAST) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s, The object id is not RG\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the configuration mode. */
        status = fbe_api_raid_group_get_info(rg_object_id, &get_rg_info, FBE_PACKET_FLAG_NO_ATTRIB);        
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s: RG get config failed with status:%d\n", __FUNCTION__, status); 
            return status;
        }

        status = fbe_api_cmi_service_get_info(&get_cmi_info);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Failed to get CMI Info.\n",
                      __FUNCTION__);
            return status;
        }
            
        if (get_rg_info.peer_state  == FBE_LIFECYCLE_STATE_READY || get_cmi_info.peer_alive == FBE_FALSE){
            return FBE_STATUS_OK;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        /* wait */
        fbe_api_sleep(FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_INFO,
                   "%s: RG object(%d) peer lifecycle state is not ready within the expected %d ms!\n", 
                  __FUNCTION__, rg_object_id, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!********************************************************************
 *  @fn fbe_api_wait_for_encl_firmware_download_status ()
 *********************************************************************
 *  @brief
 *    This function poll the physical package for the 
 *                  expected firmware download status.
 *
 *  @param object_id
 *  @param expected_download_status
 *  @param expected_extended_download_status
 *  @param timeout_ms - timeout value in mili-second
 *
 *  @return fbe_status_t
 *
 *  @version
 *    29-June-2010: PHE - Created
 *    
 *********************************************************************/
fbe_status_t fbe_api_wait_for_encl_firmware_download_status(fbe_object_id_t object_id,
                                  fbe_enclosure_fw_target_t target, 
                                  fbe_enclosure_firmware_status_t expected_download_status,
                                  fbe_enclosure_firmware_ext_status_t expected_extended_download_status,
                                  fbe_u32_t timeout_ms)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u32_t                           current_time = 0;
    fbe_enclosure_mgmt_download_op_t    firmware_status = {0};

    while (current_time < timeout_ms){
        firmware_status.target = target;
        firmware_status.op_code = FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS;
        status = fbe_api_enclosure_firmware_download_status(object_id, &firmware_status);
 
        if (status != FBE_STATUS_OK){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s: Firmware download status returned with status 0x%X.\n", 
                          __FUNCTION__, status); 
            return status;
        }
            
        if ((firmware_status.status == expected_download_status)&&
            (firmware_status.extended_status == expected_extended_download_status)) {
            return status;
        }
        
        current_time = current_time + FBE_API_POLLING_INTERVAL;
        fbe_api_sleep (FBE_API_POLLING_INTERVAL);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: firmware download status 0x%x and/or ext status 0x%x are not set within the expected %d ms!\n", 
                  __FUNCTION__, 
                  expected_download_status,
                  expected_extended_download_status,
                  timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}


/*!****************************************************************************
 * @fn fbe_api_wait_till_object_is_destroyed(fbe_object_id_t obj_id,fbe_package_id_t package_id)
 ******************************************************************************
 *****************************************************************
 * @brief
 *   This function waits up to a minute for an object to be destroyed
 *
 * @param object_id         - object id to wait for 
 * @param package_id        - package ID.
 * 
 * @return fbe_status_t
 * 
 *
 ****************************************************************/
fbe_status_t fbe_api_wait_till_object_is_destroyed(fbe_object_id_t obj_id,fbe_package_id_t package_id)
{
    fbe_u32_t                                  wait_count=0;
    fbe_topology_mgmt_check_object_existent_t  check_object_existent;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t status_info;

    /* Populate the Object ID and set default exists to TRUE */
    check_object_existent.object_id = obj_id;
    check_object_existent.exists_b = FBE_TRUE;

    /* Wait up to one minute for the object to completely destroy */
    do {
        /* Ask topology to check for object existent */
        status = fbe_api_common_send_control_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_CHECK_OBJECT_EXISTENT,
                                     &check_object_existent,
                                     sizeof(fbe_topology_mgmt_check_object_existent_t),
                                     FBE_SERVICE_ID_TOPOLOGY,
                                     FBE_PACKET_FLAG_NO_ATTRIB,
                                     &status_info,
                                     package_id);

        if(status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s: send_control_packet_to_service returned with status 0x%X.\n", 
                          __FUNCTION__, status); 
            return status;
        }


        if(status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                          "%s: send_control_packet_to_service returned with control_operation_status 0x%X.\n", 
                          __FUNCTION__, status_info.control_operation_status); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
        

        if (check_object_existent.exists_b != FBE_TRUE)
        {
            return FBE_STATUS_OK;
        }
        else
        {
            fbe_api_sleep(100); 
            
            /* Increment wait count */
            wait_count++;       
        }

    } while ((check_object_existent.exists_b == FBE_TRUE) && (wait_count < 600));

    
    fbe_api_trace(FBE_TRACE_LEVEL_WARNING,
                   "%s: object 0x%x still exist after 1 minute !\n", 
                  __FUNCTION__, 
                  obj_id);              

    return FBE_STATUS_GENERIC_FAILURE;
    
}

