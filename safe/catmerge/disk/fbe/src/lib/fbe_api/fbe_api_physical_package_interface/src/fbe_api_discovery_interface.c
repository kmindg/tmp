/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_discovery_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Discovery related APIs.
 *      
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_discovery_interface
 * 
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_system_interface
 *
 * @version
 *      10/10/08    sharel - Created
 *    
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_port.h"
#include "fbe_discovery_transport.h"
#include "fbe_ssp_transport.h"
#include "fbe_stp_transport.h"
#include "fbe_smp_transport.h"
#include "fbe_block_transport.h"


/**************************************
 *              Local variables
**************************************/
/*!********************************************************************* 
 * @struct death_reason_to_str_t 
 *  
 * @brief 
 *  This contains the data info for Death Reason.
 *
 * @ingroup fbe_api_discovery_interface
 * @ingroup death_reason_to_str
 **********************************************************************/
typedef struct death_reason_to_str_s{
    fbe_object_death_reason_t   death_reason; /*!< Death Reason */
    fbe_u8_t *                  death_reason_str; /*!< Death Reason String */
}death_reason_to_str_t;


death_reason_to_str_t   death_opcode_to_str_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED),
    FBE_API_ENUM_TO_STR(FBE_BASE_DISCOVERED_DEATH_REASON_HARDWARE_NOT_READY),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_THRESHOLD_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_DATA_THRESHOLD_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ACTIVATE_TIMER_EXPIRED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_MODE_SENSE),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_READ_CAPACITY),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DRIVE_NOT_SPINNING),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DC_FLUSH_FAILED),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_FAILED),
    FBE_API_ENUM_TO_STR(FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_FAILED),
    FBE_API_ENUM_TO_STR(FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_INVALID_INSCRIPTION),
    FBE_API_ENUM_TO_STR(FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_PIO_MODE_UNSUPPORTED),
    FBE_API_ENUM_TO_STR(FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_UDMA_MODE_UNSUPPORTED),
    FBE_API_ENUM_TO_STR(FBE_BASE_ENCLOSURE_DEATH_REASON_FAULTY_COMPONENT),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_PRICE_CLASS_MISMATCH),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SIMULATED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_MULTI_BIT),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_SINGLE_BIT),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_OTHER),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION),
    FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_ID),
	FBE_API_ENUM_TO_STR(FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SED_DRIVE_NOT_SUPPORTED),
    {FBE_DEATH_REASON_INVALID, "No text mapping, please update death_opcode_to_str_table"}
};
/*******************************************
 *              Local functions
 *******************************************/

/*!***************************************************************
 * @fn fbe_api_enumerate_objects(
 *     fbe_object_id_t * obj_array, 
 *     fbe_u32_t total_objects, 
 *     fbe_u32_t *actual_objects, 
 *     fbe_package_id_t package_id) 
 *****************************************************************
 * @brief
 *  This function enumerates all objects in the topology. 
 *
 * @param obj_array - The array to return results in.
 * @param total_objects - how many object we allocated for.
 * @param package_id - which package to send to
 * @param actual_objects - how many objects were actually copied to memory
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enumerate_objects(fbe_object_id_t * obj_array, 
                                                    fbe_u32_t total_objects, 
                                                    fbe_u32_t *actual_objects, 
                                                    fbe_package_id_t package_id)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_topology_mgmt_enumerate_objects_t   enumerate_objects;/*structure is small enough to be on the stack*/
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_sg_element_t *                      temp_sg_list = NULL;

    if (obj_array == NULL || total_objects == 0) {
        *actual_objects = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);/*one for the entry and one for the NULL termination*/
    temp_sg_list = sg_list;

    /*set up the sg list to point to the list of objects the user wants to get*/
    temp_sg_list->address = (fbe_u8_t *)obj_array;
    temp_sg_list->count = total_objects * sizeof(fbe_object_id_t);
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    enumerate_objects.number_of_objects = total_objects;

    /*upon completion, the user memory will be filled with the data, this is taken care of by the FBE API common code*/
    status = fbe_api_common_send_control_packet_to_service_with_sg_list (FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_OBJECTS,
                                                                         &enumerate_objects,
                                                                         sizeof (fbe_topology_mgmt_enumerate_objects_t),
                                                                         FBE_SERVICE_ID_TOPOLOGY,
                                                                         FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                         sg_list,
                                                                         2,/*2 sg entries*/
                                                                         &status_info,
                                                                         package_id);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        fbe_api_free_memory (sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (actual_objects != NULL) {
        *actual_objects = enumerate_objects.number_of_objects_copied;
    }

    fbe_api_free_memory (sg_list);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_get_object_port_number(
 *     fbe_object_id_t object_id, 
 *     fbe_port_number_t * port_number)
 *****************************************************************
 * @brief
 *   This function returns the port number of the object.
 *
 * @param object_id     - object to query.
 * @param port_number   - what port object is on.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_object_port_number (fbe_object_id_t object_id, fbe_port_number_t * port_number)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_port_mgmt_get_port_number_t     base_port_mgmt_get_port_number;
    fbe_api_control_operation_status_info_t status_info;
    
    status = fbe_api_common_send_control_packet (FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER,
                                                 &base_port_mgmt_get_port_number,
                                                 sizeof (fbe_base_port_mgmt_get_port_number_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status == FBE_STATUS_NO_OBJECT) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: requested object does not exist\n", __FUNCTION__);
        return status;
    }

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING , "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return status;
    }
   
    *port_number = base_port_mgmt_get_port_number.port_number;
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:got port number:%d for obj:0x%X\n", __FUNCTION__, *port_number, object_id);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_get_object_enclosure_number(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_number_t * enclosure_number)
 *****************************************************************
 * @brief
 *   This function returns the enclosure the object is on.
 *
 * @param object_id         - object to query.
 * @param enclosure_number  - what enclosure_number object is on.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_object_enclosure_number (fbe_object_id_t object_id, fbe_enclosure_number_t * enclosure_number)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_enclosure_mgmt_get_enclosure_number_t  base_enclosure_mgmt_get_enclosure_number;
    fbe_api_control_operation_status_info_t         status_info;;
    
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER,
                                                 &base_enclosure_mgmt_get_enclosure_number,
                                                 sizeof (fbe_base_enclosure_mgmt_get_enclosure_number_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return status;
    }
   
    *enclosure_number = base_enclosure_mgmt_get_enclosure_number.enclosure_number;
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:got encl slot:%d for obj:0x%X\n", __FUNCTION__, *enclosure_number, object_id);

    return status;
}

/*!****************************************************************************
 * @fn      fbe_api_get_object_component_id ()
 *
 * @brief
 *  This function 
 *
 * @param object_id - object to query
 * @param component_id - pointer to a component_id for return data.
 *
 * @return success or failure
 *
 * HISTORY
 *  04/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_get_object_component_id (fbe_object_id_t object_id, fbe_component_id_t * component_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_enclosure_mgmt_get_component_id_t      base_enclosure_mgmt_get_component_id;
    fbe_api_control_operation_status_info_t         status_info;;
    
    fbe_zero_memory(&base_enclosure_mgmt_get_component_id, 
                    sizeof(fbe_base_enclosure_mgmt_get_component_id_t));

    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_COMPONENT_ID,
                                                 &base_enclosure_mgmt_get_component_id,
                                                 sizeof (fbe_base_enclosure_mgmt_get_component_id_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__,
            status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *component_id = base_enclosure_mgmt_get_component_id.component_id;
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:got encl slot:%d for obj:0x%X\n", __FUNCTION__, *component_id, object_id);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_get_object_drive_number(
 *     fbe_object_id_t object_id, 
 *     fbe_enclosure_slot_number_t * drive_number)
 *****************************************************************
 * @brief
 *   This function returns the drive slot number.
 *
 * @param object_id     - object to query.
 * @param drive_number  - slot number return.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_object_drive_number(fbe_object_id_t object_id, fbe_enclosure_slot_number_t * drive_number)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_enclosure_mgmt_get_slot_number_t       get_slot_number;
    fbe_api_control_operation_status_info_t         status_info;;
    
    status = fbe_api_common_send_control_packet (FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER,
                                                 &get_slot_number,
                                                 sizeof (fbe_base_enclosure_mgmt_get_slot_number_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_TRAVERSE,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
       fbe_trace_level_t severity = FBE_TRACE_LEVEL_ERROR;

       if (status == FBE_STATUS_NO_OBJECT) {
           severity = FBE_TRACE_LEVEL_WARNING;  /* call may fail because object was recently destroyed. */
       }

       fbe_api_trace (severity, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return status;
    }
   
    *drive_number = get_slot_number.enclosure_slot_number;
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:got drive slot:%d for obj:0x%X\n", __FUNCTION__, *drive_number, object_id);

    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_object_type(
 *     fbe_object_id_t object_id, 
 *     fbe_topology_object_type_t *object_type, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function returns the type of the object 
 *   (drive, port etc.).
 *
 * @param object_id     - object to query.
 * @param object_type   - the type of object.
 * @param package_id    - package ID.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_object_type (fbe_object_id_t object_id, fbe_topology_object_type_t *object_type, fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_mgmt_get_object_type_t             get_object_type;    
    fbe_api_control_operation_status_info_t         status_info;;

    get_object_type.object_id = object_id;

    status = fbe_api_common_send_control_packet (FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_TYPE,
                                                 &get_object_type,
                                                 sizeof (fbe_topology_mgmt_get_object_type_t),
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);

   if (status == FBE_STATUS_NO_OBJECT) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: requested object does not exist\n", __FUNCTION__);
        return status;
   }

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return status;
    }
   
    *object_type = get_object_type.topology_object_type;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_object_lifecycle_state(
 *     fbe_object_id_t object_id, 
 *     fbe_lifecycle_state_t * lifecycle_state, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function returns the state the object is on.
 *
 * @param object_id         - object to query.
 * @param lifecycle_state   - the state of the object.
 * @param package_id        - package ID.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_object_lifecycle_state (fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state, fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_lifecycle_state_t      base_object_mgmt_get_lifecycle_state;   
    fbe_api_control_operation_status_info_t         status_info;;

    status = fbe_api_common_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                 &base_object_mgmt_get_lifecycle_state,
                                                 sizeof (fbe_base_object_mgmt_get_lifecycle_state_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);

    if (status == FBE_STATUS_NO_OBJECT) {
        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s: requested object does not exist\n", __FUNCTION__);
        return status;
    }

    /* If object is in SPECIALIZE STATE topology will return FBE_STATUS_BUSY */
    if (status == FBE_STATUS_BUSY) {
        *lifecycle_state = FBE_LIFECYCLE_STATE_SPECIALIZE;
        return FBE_STATUS_OK;
    }

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        return status;
    }
   
    *lifecycle_state = base_object_mgmt_get_lifecycle_state.lifecycle_state;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_object_class_id(
 *     fbe_object_id_t object_id, 
 *     fbe_class_id_t *class_id, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function returns the class of the object.
 *
 * @param object_id     - object to query.
 * @param class_id      - the state of the object.
 * @param package_id    - package ID.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_object_class_id (fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_class_id_t             base_object_mgmt_get_class_id;  
    fbe_api_control_operation_status_info_t         status_info;
   

    status = fbe_api_common_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                 &base_object_mgmt_get_class_id,
                                                 sizeof (fbe_base_object_mgmt_get_class_id_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
       fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_ERROR;

       if (status == FBE_STATUS_NO_OBJECT) {   /* this is an expected case for discovered objects. must have been destroyed. */
           trace_level = FBE_TRACE_LEVEL_WARNING;
       }

        fbe_api_trace (trace_level, "%s: pkg:%d obj:0x%x, pkt err:%d, pkt qual:%d, payload err:%d, payload qual:%d\n", __FUNCTION__,
                       package_id, object_id, status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);       
        
        return status;
    }
   
    *class_id = base_object_mgmt_get_class_id.class_id;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_destroy_object(
 *     fbe_object_id_t object_id, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function issues a destroy request to the object.
 *
 * @param object_id     - object to query.
 * @param package_id    - package ID.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    12/17/08: bphilbin    created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_destroy_object (fbe_object_id_t object_id, fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t         status_info = {0};

    status = fbe_api_common_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);


        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_discovery_edge_info(
 *     fbe_object_id_t object_id, 
 *     fbe_api_get_discovery_edge_info_t *edge_info)
 *****************************************************************
 * @brief
 *   This function returns the information about the edge.
 *
 * @param object_id - object to query.
 * @param edge_info - information about the edge.
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_discovery_edge_info (fbe_object_id_t object_id, fbe_api_get_discovery_edge_info_t *edge_info)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_discovery_transport_control_get_edge_info_t *    discovery_edge_info = NULL;
    fbe_api_control_operation_status_info_t              status_info = {0};

    discovery_edge_info = fbe_api_allocate_memory (sizeof (fbe_discovery_transport_control_get_edge_info_t));
    if (discovery_edge_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:not able to allocate memory for packet", __FUNCTION__);
        return status;
    }
    // cleared in alloc.  fbe_set_memory(discovery_edge_info,  0, sizeof(*discovery_edge_info));

    status = fbe_api_common_send_control_packet (FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                 discovery_edge_info,
                                                 sizeof (fbe_discovery_transport_control_get_edge_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

   if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (discovery_edge_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    edge_info->path_attr = discovery_edge_info->base_edge_info.path_attr;
    edge_info->path_state = discovery_edge_info->base_edge_info.path_state;
    edge_info->client_id = discovery_edge_info->base_edge_info.client_id;
    edge_info->transport_id = discovery_edge_info->base_edge_info.transport_id;
    edge_info->client_index = discovery_edge_info->base_edge_info.client_index;
    edge_info->server_id = discovery_edge_info->base_edge_info.server_id;
    edge_info->server_index = discovery_edge_info->base_edge_info.server_index;
    
    fbe_api_free_memory (discovery_edge_info);

    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_ssp_edge_info(
 *     fbe_object_id_t object_id,
 *     fbe_api_get_ssp_edge_info_t *edge_info)
 *****************************************************************
 * @brief
 *   This function returns the information about the SSP edge.
 *
 * @param object_id     - object to query
 * @param edge_info     - pointer to the SSP edge data structure
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_ssp_edge_info (fbe_object_id_t object_id, fbe_api_get_ssp_edge_info_t *edge_info)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_ssp_transport_control_get_edge_info_t *          ssp_edge_info = NULL;
    fbe_api_control_operation_status_info_t              status_info = {0};

    if (object_id >= FBE_OBJECT_ID_INVALID || edge_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    ssp_edge_info = fbe_api_allocate_memory (sizeof (fbe_ssp_transport_control_get_edge_info_t));
    if (ssp_edge_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:not able to allocate memory for packet", __FUNCTION__);
        return status;
    }
    // cleared in alloc.  fbe_set_memory(ssp_edge_info,  0, sizeof(*ssp_edge_info));

    status = fbe_api_common_send_control_packet (FBE_SSP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                 ssp_edge_info,
                                                 sizeof (fbe_ssp_transport_control_get_edge_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (ssp_edge_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    edge_info->path_attr = ssp_edge_info->base_edge_info.path_attr;
    edge_info->path_state = ssp_edge_info->base_edge_info.path_state;
    edge_info->client_id = ssp_edge_info->base_edge_info.client_id;
    edge_info->transport_id = ssp_edge_info->base_edge_info.transport_id;
    edge_info->client_index = ssp_edge_info->base_edge_info.client_index;
    edge_info->server_id = ssp_edge_info->base_edge_info.server_id;
    edge_info->server_index = ssp_edge_info->base_edge_info.server_index;

    fbe_api_free_memory (ssp_edge_info);

    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_block_edge_info(
 *     fbe_object_id_t object_id,
 *     fbe_edge_index_t edge_index,
 *     fbe_api_get_block_edge_info_t *edge_info,
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function returns the information about the block edge.
 *
 * @param object_id     - object to query
 * @param edge_index    - edge index
 * @param edge_info     - pointer to the block edge data structure
 * @param package_id    - package ID
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/14/08: sharel    created
 *  10/22/09: dhaval  Updated to use with base config class in SEP.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_block_edge_info (fbe_object_id_t object_id,
                                          fbe_edge_index_t edge_index,
                                          fbe_api_get_block_edge_info_t *edge_info,
                                          fbe_package_id_t package_id)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_block_transport_control_get_edge_info_t *        block_edge_info = NULL;
//    fbe_u32_t                                            qualifier = 0;
    fbe_api_control_operation_status_info_t              status_info = {0};

    if (object_id >= FBE_OBJECT_ID_INVALID || edge_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (edge_index == FBE_EDGE_INDEX_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    block_edge_info = fbe_api_allocate_memory (sizeof (fbe_block_transport_control_get_edge_info_t));
    if (block_edge_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:not able to allocate memory for packet", __FUNCTION__);
        return status;
    }
    // cleared in alloc.  fbe_set_memory(block_edge_info,  0, sizeof(*block_edge_info));

    /* Set the passed-in edge index to base edge information. */
    block_edge_info->base_edge_info.client_index = edge_index;

    /* Send the block transport control code to get the edge information.
     * (Base config handles this userper command).
     */
    status = fbe_api_common_send_control_packet (FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                 block_edge_info,
                                                 sizeof (fbe_block_transport_control_get_edge_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        fbe_api_free_memory (block_edge_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edge_info->path_attr = block_edge_info->base_edge_info.path_attr;
    edge_info->path_state = block_edge_info->base_edge_info.path_state;
    edge_info->client_id = block_edge_info->base_edge_info.client_id;
    edge_info->transport_id = block_edge_info->base_edge_info.transport_id;
    edge_info->client_index = block_edge_info->base_edge_info.client_index;
    edge_info->server_id = block_edge_info->base_edge_info.server_id;
    edge_info->server_index = block_edge_info->base_edge_info.server_index;
    edge_info->capacity = block_edge_info->capacity;
    edge_info->exported_block_size = block_edge_info->exported_block_size;
    edge_info->imported_block_size = block_edge_info->imported_block_size;
    edge_info->offset = block_edge_info->offset;

    fbe_api_free_memory (block_edge_info);

    return status;

}

/*!***************************************************************
 * @fn fbe_api_set_ssp_edge_hook(
 *     fbe_object_id_t object_id, 
 *     fbe_api_set_edge_hook_t *hook_info)
 *****************************************************************
 * @brief
 *   This function set the hook on SSP edge of the given object.
 *
 * @param object_id     - object to query
 * @param hook_info     - pointer to the Edge Hook data structure
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    01/08/09: guov    created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_ssp_edge_hook (fbe_object_id_t object_id, fbe_api_set_edge_hook_t *hook_info)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_transport_control_set_edge_tap_hook_t *      hook = NULL;
    fbe_api_control_operation_status_info_t              status_info = {0};

    hook = fbe_api_allocate_memory (sizeof (fbe_transport_control_set_edge_tap_hook_t));
    if (hook == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:not able to allocate memory for packet", __FUNCTION__);
        return status;
    }
    // cleared in alloc.  fbe_set_memory(hook,  0, sizeof(*hook));
    hook->edge_tap_hook = hook_info->hook;

    status = fbe_api_common_send_control_packet (FBE_SSP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK,
                                                 hook,
                                                 sizeof (fbe_transport_control_set_edge_tap_hook_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (hook);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_api_free_memory (hook);

    return status;
}


/*!***************************************************************
 * @fn fbe_api_set_stp_edge_hook(
 *     fbe_object_id_t object_id, 
 *     fbe_api_set_edge_hook_t *hook_info)
 *****************************************************************
 * @brief
 *   This function set the hook on STP edge of the given object.
 *
 * @param object_id     - object to query
 * @param hook_info     - pointer to the Edge Hook data structure
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    02/05/09: Peter Puhov created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_stp_edge_hook (fbe_object_id_t object_id, fbe_api_set_edge_hook_t *hook_info)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_transport_control_set_edge_tap_hook_t *          hook = NULL;
    fbe_api_control_operation_status_info_t              status_info = {0};

    hook = fbe_api_allocate_memory (sizeof (fbe_transport_control_set_edge_tap_hook_t));
    if (hook == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:not able to allocate memory for packet", __FUNCTION__);
        return status;
    }
    // cleared in alloc.  fbe_set_memory(hook,  0, sizeof(*hook));
    hook->edge_tap_hook = hook_info->hook;

    status = fbe_api_common_send_control_packet (FBE_STP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK,
                                                 hook,
                                                 sizeof (fbe_transport_control_set_edge_tap_hook_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (hook);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_api_free_memory (hook);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_get_total_objects(
 *     fbe_u32_t *total_objects, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function returns the total number of objects in the system.
 *
 * @param total_objects             - pointer to total objects
 * @param package_id                - package ID
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    10/16/08: sharel  created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_total_objects(fbe_u32_t *total_objects, fbe_package_id_t package_id)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_total_objects_t             get_total;/* this structure is small enough to be on the stack*/
    fbe_api_control_operation_status_info_t              status_info = {0};

    get_total.total_objects = 0;

    status = fbe_api_common_send_control_packet_to_service (FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS,
                                                            &get_total,
                                                            sizeof (fbe_topology_control_get_total_objects_t),
                                                            FBE_SERVICE_ID_TOPOLOGY,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *total_objects = get_total.total_objects;

    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_all_objects_in_state(
 *     fbe_lifecycle_state_t requested_lifecycle_state, 
 *     fbe_object_id_t obj_array[], 
 *     fbe_u32_t array_length,
 *     fbe_u32_t *total_objects, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function returns to the array all the objects that are 
 *   in a certain state.
 *
 * @param requested_lifecycle_state - state to look for
 * @param obj_array                 - object array to return
 * @param array_length              - size of the array
 * @param total_objects             - pointer to total objects to return
 * @param package_id                - package ID
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    11/14/08: sharel  created
 *    
 ****************************************************************/
fbe_status_t fbe_api_get_all_objects_in_state (fbe_lifecycle_state_t requested_lifecycle_state, 
                                               fbe_object_id_t obj_array[], 
                                               fbe_u32_t array_length,
                                               fbe_u32_t *total_objects, 
                                               fbe_package_id_t package_id)
{
    fbe_u32_t               existing_objects = 0;
    fbe_object_id_t *       object_list = NULL;
    fbe_u32_t               object_idx = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lifecycle_state_t   lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_u32_t               object_count = 0;

    if (total_objects == NULL || obj_array == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    *total_objects = 0;/*reset the user data*/

    status = fbe_api_get_total_objects(&object_count, package_id);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    object_list = (fbe_object_id_t *)fbe_api_allocate_memory ((sizeof (fbe_object_id_t)* object_count));
    if (object_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*first lets get all the objects in the system*/
    status = fbe_api_enumerate_objects (object_list, object_count, &existing_objects, package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(object_list);
        return status;
    }

    /*then let's see which one of them is in the requested state*/
    for (object_idx = 0; object_idx < existing_objects; object_idx++) {
        status = fbe_api_get_object_lifecycle_state(object_list[object_idx], &lifecycle_state, package_id);
        if (status != FBE_STATUS_OK) {
            fbe_api_free_memory(object_list);
            return status;
        }else if (lifecycle_state == requested_lifecycle_state) {
            if (*total_objects < array_length) {
            obj_array[(*total_objects)] = object_list[object_idx];
            (*total_objects)++;
            }else{
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, User array size:%d, smaller than actual object count:%d\n", __FUNCTION__, array_length, existing_objects);
                fbe_api_free_memory(object_list);
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }


    fbe_api_free_memory(object_list);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_get_logical_drive_object_state(
 *     fbe_u32_t port, 
 *     fbe_u32_t enclosure, 
 *     fbe_u32_t slot, 
 *     fbe_lifecycle_state_t *lifecycle_state_out)
 *****************************************************************
 * @brief
 *   This function returns the state of the logical drive based 
 *   on a port/encl/slot interface.
 *
 * @param port                  - port info
 * @param enclosure             - enclosure info
 * @param slot                  - drive slot
 * @param lifecycle_state_out   - state of the logical drive
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  11/21/08: sharel    created
 *
 ****************************************************************/
fbe_status_t fbe_api_get_logical_drive_object_state (fbe_u32_t port, 
                                                     fbe_u32_t enclosure, 
                                                     fbe_u32_t slot, 
                                                     fbe_lifecycle_state_t *lifecycle_state_out)
{
    fbe_object_id_t *           obj_list = NULL;
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_lifecycle_state_t       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_port_number_t           port_num = 0;
    fbe_enclosure_number_t      enlcosure_num = 0;
    fbe_enclosure_slot_number_t         drive_num = 0;
    fbe_u32_t                   total_objects = 0;
    fbe_u32_t                   count = 0;

    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_set_memory(obj_list, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num ,FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
        fbe_api_free_memory(obj_list);
        return status;
    }

    /*let's find the object we want*/
    for (obj_count = 0; obj_count < object_num; obj_count++) {
        count = 0;
        /*now, let's find from all these objects a drive that has all the port/encl/slot criteria for our drive*/
        do {
            if (count != 0) {
                fbe_api_sleep(50);/*wait for object to get out of specialized state*/
            }
            if (count == 20) {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            count++;
            status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        } while (status == FBE_STATUS_BUSY);

        if (status != FBE_STATUS_OK) {
            *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
            fbe_api_free_memory(obj_list);
            return status;
        }

        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE) {
            /*we have a drive, let's see it's properties are the one we look for*/
            status = fbe_api_get_object_port_number (obj_list[obj_count], &port_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            status = fbe_api_get_object_enclosure_number (obj_list[obj_count], &enlcosure_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            status = fbe_api_get_object_drive_number (obj_list[obj_count], &drive_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            if ((drive_num == slot) && (enlcosure_num == enclosure) &&(port_num == port)) {
                status = fbe_api_get_object_lifecycle_state(obj_list[obj_count], &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
                if (status != FBE_STATUS_OK) {
                    fbe_api_free_memory(obj_list);
                    return status;
                }else {
                    *lifecycle_state_out = lifecycle_state;
                    fbe_api_free_memory(obj_list);
                    return FBE_STATUS_OK;
                }
            }
        }
    }

    *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
    fbe_api_free_memory(obj_list);
    return FBE_STATUS_GENERIC_FAILURE;


}

/*!***************************************************************
 * @fn fbe_api_get_all_enclosure_object_ids( 
 *     fbe_object_id_t *object_id_list, 
 *     fbe_u32_t buffer_size, 
 *     fbe_u32_t *enclosure_count)
 *****************************************************************
 * @brief
 *   This function returns the list of enclosure Object IDs as
 *   seen by the physical package.
 *
 * @param object_id_list - Buffer to store the enclosure object
 *                      IDs
 * @param buffer_size - Size of the buffer passed in
 * @param enclosure_count - Number of enclosure seen 
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  02/27/10: Ashok Tamilarasan created
 *
 ****************************************************************/
fbe_status_t fbe_api_get_all_enclosure_object_ids( fbe_object_id_t *object_id_list, 
                                                   fbe_u32_t buffer_size, 
                                                   fbe_u32_t *enclosure_count)
{
    fbe_object_id_t *           obj_list = NULL;
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_u32_t                   total_objects = 0;
    fbe_u32_t                   count = 0;


    *enclosure_count = 0;
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num ,FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(obj_list);
        return status;
    }

    /*let's find the object we want*/
    for (obj_count = 0; obj_count < object_num; obj_count++) {
        count = 0;
        do {
            if (count != 0) {
                fbe_api_sleep(50);/*wait for object to get out of specialized state*/
            }
            if (count == 20) {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            count++;

            status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        } while (status == FBE_STATUS_BUSY);

        if (status != FBE_STATUS_OK) {
            fbe_api_free_memory(obj_list);
            return status;
        }

        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) {
            if(*enclosure_count > buffer_size) {
                fbe_api_free_memory(obj_list);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            object_id_list[(*enclosure_count)] = obj_list[obj_count];
            (*enclosure_count)++;   
        }
    }

    fbe_api_free_memory(obj_list);
    return FBE_STATUS_OK;
}
/*!***************************************************************
 * @fn fbe_api_get_all_enclosure_lcc_object_ids( 
 *     fbe_object_id_t enclObjectId,
 *     fbe_object_id_t *client_lcc_obj_list, 
 *     fbe_u32_t buffer_size, 
 *     fbe_u32_t *lcc_count)
 *****************************************************************
 * @brief
 *   This function returns the list of lcc Object IDs for the
 *   parent enclosure as seen by the physical package.
 * 
 * @param enclObjectId - the object id for the enclosure to search
 * @param client_lcc_obj_list - Buffer to store the enclosure object
 *                      IDs
 * @param buffer_size - max number of lcc objects per enclosure.
 * @param lcc_count - Number of LCC objects for the enclosure
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  08/10/11: GB created
 ****************************************************************/
fbe_status_t fbe_api_get_enclosure_lcc_object_ids(fbe_object_id_t enclObjectId,
                                                   fbe_object_id_t *client_lcc_obj_list, 
                                                   fbe_u32_t buffer_size, 
                                                   fbe_u32_t *lcc_count)
{
    fbe_object_id_t *           obj_list = NULL;
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_u32_t                   total_objects = 0;
    fbe_u32_t                   count = 0;
    fbe_api_get_discovery_edge_info_t    edge_info = {0};


    *lcc_count = 0;
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_set_memory(obj_list,  (fbe_u8_t)0xFF, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num ,FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(obj_list);
        return status;
    }

    // find the lcc objects
    for (obj_count = 0; obj_count < object_num; obj_count++) 
    {
        count = 0;
        do 
        {
            if (count != 0) {
                fbe_api_sleep(50);// wait for object to get out of specialized state
            }
            if (count == 20) {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            count++;
            status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        } while (status == FBE_STATUS_BUSY);

        if (status != FBE_STATUS_OK) {
            fbe_api_free_memory(obj_list);
            return status;
        }

        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_LCC){
            status = fbe_api_get_discovery_edge_info (obj_list[obj_count], &edge_info);
            if (status != FBE_STATUS_OK) {
                fbe_api_free_memory(obj_list);
                return status;
            }
            // if parent obj is this encl count it
            if(edge_info.server_id == enclObjectId)
            {
                if(*lcc_count > buffer_size) {
                    fbe_api_free_memory(obj_list);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                client_lcc_obj_list[(*lcc_count)] = obj_list[obj_count];
                (*lcc_count)++;   
            }
        }
    }

    fbe_api_free_memory(obj_list);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_get_all_drive_object_ids( 
 *     fbe_object_id_t *object_id_list, 
 *     fbe_u32_t buffer_size, 
 *     fbe_u32_t *drive_count)
 *****************************************************************
 * @brief
 *   This function returns the list of drive Object IDs as
 *   seen by the physical package.
 *
 * @param object_id_list - Buffer to store the drive object
 *                      IDs
 * @param buffer_size - Size of the buffer passed in
 * @param drive_count - Number of drive seen 
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  02/27/10: Ashok Tamilarasan created
 *
 ****************************************************************/
fbe_status_t fbe_api_get_all_drive_object_ids( fbe_object_id_t *object_id_list, 
                                                   fbe_u32_t buffer_size, 
                                                   fbe_u32_t *drive_count)
{
    fbe_object_id_t *           obj_list = NULL;
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_u32_t                   total_objects = 0;
    fbe_u32_t                   count = 0;

    *drive_count = 0;
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num ,FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(obj_list);
        return status;
    }

    /*let's find the object we want*/
    for (obj_count = 0; obj_count < object_num; obj_count++) {
        count = 0;
        do {
            if (count != 0) {
                fbe_api_sleep(50);/*wait for object to get out of specialized state*/
            }
            if (count == 20) {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            count++;
            status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        } while (status == FBE_STATUS_BUSY);

        if (status != FBE_STATUS_OK) {
            fbe_api_free_memory(obj_list);
            return status;
        }

        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
            if(*drive_count > buffer_size) {
                fbe_api_free_memory(obj_list);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            object_id_list[(*drive_count)] = obj_list[obj_count];
            (*drive_count)++;   
        }
    }

    fbe_api_free_memory(obj_list);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_get_physical_drive_object_state(
 *     fbe_u32_t port, 
 *     fbe_u32_t enclosure, 
 *     fbe_u32_t slot, 
 *     fbe_lifecycle_state_t *lifecycle_state_out)
 *****************************************************************
 * @brief
 *   This function returns the state of the physical drive based 
 *   on a port/encl/slot interface.
 *
 * @param port                  - port info
 * @param enclosure             - enclosure info
 * @param slot                  - drive slot
 * @param lifecycle_state_out   - state of the physical drive
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  11/21/08: sharel    created
 *
 ****************************************************************/
fbe_status_t fbe_api_get_physical_drive_object_state (fbe_u32_t port, 
                                                      fbe_u32_t enclosure, 
                                                      fbe_u32_t slot, 
                                                      fbe_lifecycle_state_t *lifecycle_state_out)
{
    fbe_object_id_t *           obj_list = NULL;
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_lifecycle_state_t       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_port_number_t           port_num = 0;
    fbe_enclosure_number_t      enlcosure_num = 0;
    fbe_enclosure_slot_number_t         drive_num = 0;
    fbe_u32_t                   total_objects  =0;
    fbe_u32_t                   count = 0;

    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
        fbe_api_free_memory(obj_list);
        return status;
    }

    /*let's find the object we want*/
    for (obj_count = 0; obj_count < object_num; obj_count++) {
        count = 0;
        /*now, let's find from all these objects a drive that has all the port/encl/slot criteria for our drive*/
        do {
            if (count != 0) {
                fbe_api_sleep(50);/*wait for object to get out of specialized state*/
            }
            if (count == 20) {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            count++;
            status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        } while (status == FBE_STATUS_BUSY);

        if (status != FBE_STATUS_OK) {
            *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
            fbe_api_free_memory(obj_list);
            return status;
        }

        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
            /*we have a drive, let's see it's properties are the one we look for*/
            status = fbe_api_get_object_port_number (obj_list[obj_count], &port_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            status = fbe_api_get_object_enclosure_number (obj_list[obj_count], &enlcosure_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            status = fbe_api_get_object_drive_number (obj_list[obj_count], &drive_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            if ((drive_num == slot) && (enlcosure_num == enclosure) &&(port_num == port)) {
                status = fbe_api_get_object_lifecycle_state(obj_list[obj_count], &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
                if (status != FBE_STATUS_OK) {
                    fbe_api_free_memory(obj_list);
                    return status;
                }else {
                    *lifecycle_state_out = lifecycle_state;
                    fbe_api_free_memory(obj_list);
                    return FBE_STATUS_OK;
                }
            }
        }
    }

    *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
    fbe_api_free_memory(obj_list);
    return FBE_STATUS_GENERIC_FAILURE;


}

/*!***************************************************************
 * @fn fbe_api_get_enclosure_object_state(
 *     fbe_u32_t port, 
 *     fbe_u32_t enclosure,
 *     fbe_lifecycle_state_t *lifecycle_state_out)
 *****************************************************************
 * @brief
 *   This function returns the state of the enclosure based on a 
 *   port/encl interface.
 *
 * @param port                  - port info
 * @param enclosure             - enclosure info
 * @param lifecycle_state_out   - Port state
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  11/21/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_enclosure_object_state (fbe_u32_t port, fbe_u32_t enclosure, fbe_lifecycle_state_t *lifecycle_state_out)
{
    fbe_object_id_t *           obj_list = NULL;
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_lifecycle_state_t       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_port_number_t           port_num = 0;
    fbe_enclosure_number_t      enlcosure_num = 0;
    fbe_u32_t                   total_objects = 0;
    fbe_u32_t                   count = 0;

    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
        fbe_api_free_memory(obj_list);
        return status;
    }

    /*let's find the object we want*/
    for (obj_count = 0; obj_count < object_num; obj_count++) {
        count = 0;
        /*now, let's find from all these objects a drive that has all the port/encl/slot criteria for our drive*/
       do {
            if (count != 0) {
                fbe_api_sleep(50);/*wait for object to get out of specialized state*/
            }
            if (count == 20) {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            count++;
            status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        } while (status == FBE_STATUS_BUSY);

        if (status != FBE_STATUS_OK) {
            *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
            fbe_api_free_memory(obj_list);
            return status;
        }

        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) {
            /*we have a drive, let's see it's properties are the one we look for*/
            status = fbe_api_get_object_port_number (obj_list[obj_count], &port_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            status = fbe_api_get_object_enclosure_number (obj_list[obj_count], &enlcosure_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            if ((enlcosure_num == enclosure) && (port_num == port)) {
                status = fbe_api_get_object_lifecycle_state(obj_list[obj_count], &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
                if (status != FBE_STATUS_OK) {
                    fbe_api_free_memory(obj_list);
                    return status;
                }else {
                    *lifecycle_state_out = lifecycle_state;
                    fbe_api_free_memory(obj_list);
                    return FBE_STATUS_OK;
                }
            }
        }
    }

    *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
    fbe_api_free_memory(obj_list);
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!***************************************************************
 * @fn fbe_api_get_port_object_state(
 *     fbe_u32_t port, 
 *     fbe_lifecycle_state_t *lifecycle_state_out)
 *****************************************************************
 * @brief
 *   This function returns the state of the port based on a port number.
 *
 * @param port                  - port info
 * @param lifecycle_state_out   - Port state
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  11/21/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_port_object_state (fbe_u32_t port, fbe_lifecycle_state_t *lifecycle_state_out)
{
    fbe_object_id_t *           obj_list = NULL;
    fbe_status_t                status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   object_num = 0;
    fbe_u32_t                   obj_count = 0;
    fbe_lifecycle_state_t       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_port_number_t           port_num = 0;
    fbe_u32_t                   total_objects = 0;
    fbe_u32_t                   count = 0;


    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    obj_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * total_objects);
    if (obj_list == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_set_memory(obj_list,  (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * total_objects);

    status = fbe_api_enumerate_objects (obj_list, total_objects, &object_num, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
        fbe_api_free_memory(obj_list);
        return status;
    }

    /*let's find the object we want*/
    for (obj_count = 0; obj_count < object_num; obj_count++) {
        count = 0;
        /*now, let's find from all these objects a drive that has all the port/encl/slot criteria for our drive*/
        do {
            if (count != 0) {
                fbe_api_sleep(50);/*wait for object to get out of specialized state*/
            }
            if (count == 20) {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            count++;
            status = fbe_api_get_object_type (obj_list[obj_count], &obj_type, FBE_PACKAGE_ID_PHYSICAL);
        } while (status == FBE_STATUS_BUSY);

        if (status != FBE_STATUS_OK) {
            *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
            fbe_api_free_memory(obj_list);
            return status;
        }

        if (obj_type == FBE_TOPOLOGY_OBJECT_TYPE_PORT) {
            /*we have a drive, let's see it's properties are the one we look for*/
            status = fbe_api_get_object_port_number (obj_list[obj_count], &port_num);
            if (status != FBE_STATUS_OK) {
                *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
                fbe_api_free_memory(obj_list);
                return status;
            }

            if ((port_num == port)) {
                status = fbe_api_get_object_lifecycle_state(obj_list[obj_count], &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
                if (status != FBE_STATUS_OK) {
                    fbe_api_free_memory(obj_list);
                    return status;
                }else {
                    *lifecycle_state_out = lifecycle_state;
                    fbe_api_free_memory(obj_list);
                    return FBE_STATUS_OK;
                }
            }
        }
    }

    *lifecycle_state_out = FBE_LIFECYCLE_STATE_INVALID;
    fbe_api_free_memory(obj_list);
    return FBE_STATUS_GENERIC_FAILURE;

}

/*!***************************************************************
 * @fn fbe_api_get_stp_edge_info(
 *     fbe_object_id_t object_id, 
 *     fbe_api_get_stp_edge_info_t *edge_info)
 *****************************************************************
 * @brief
 *   This function returns the information about the edge.
 *
 * @param object_id     - object to query
 * @param edge_info     - information about the STP edge
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  12/29/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_stp_edge_info (fbe_object_id_t object_id, fbe_api_get_stp_edge_info_t *edge_info)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_stp_transport_control_get_edge_info_t *          stp_edge_info = NULL;
    fbe_api_control_operation_status_info_t              status_info = {0};

    if (object_id >= FBE_OBJECT_ID_INVALID || edge_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }


    stp_edge_info = fbe_api_allocate_memory (sizeof (fbe_stp_transport_control_get_edge_info_t));
    if (stp_edge_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:not able to allocate memory for packet", __FUNCTION__);
        return status;
    }

    status = fbe_api_common_send_control_packet (FBE_STP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                 stp_edge_info,
                                                 sizeof (fbe_stp_transport_control_get_edge_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info, 
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (stp_edge_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    edge_info->path_attr = stp_edge_info->base_edge_info.path_attr;
    edge_info->path_state = stp_edge_info->base_edge_info.path_state;
    edge_info->client_id = stp_edge_info->base_edge_info.client_id;
    edge_info->transport_id = stp_edge_info->base_edge_info.transport_id;
    edge_info->client_index = stp_edge_info->base_edge_info.client_index;
    edge_info->server_id = stp_edge_info->base_edge_info.server_id;
    edge_info->server_index = stp_edge_info->base_edge_info.server_index;

    fbe_api_free_memory (stp_edge_info);

    return status;

}

/*!***************************************************************
 * @fn fbe_api_get_smp_edge_info(
 *     fbe_object_id_t object_id, 
 *     fbe_api_get_smp_edge_info_t *edge_info)
 *****************************************************************
 * @brief
 *   This function returns the information about the edge.
 *
 * @param object_id     - object to query
 * @param edge_info     - information about the edge
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  12/29/08: sharel    created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_smp_edge_info (fbe_object_id_t object_id, fbe_api_get_smp_edge_info_t *edge_info)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_smp_transport_control_get_edge_info_t *          smp_edge_info = NULL;
    fbe_api_control_operation_status_info_t              status_info = {0};

    if (object_id >= FBE_OBJECT_ID_INVALID || edge_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    smp_edge_info = fbe_api_allocate_memory (sizeof (fbe_smp_transport_control_get_edge_info_t));
    if (smp_edge_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:not able to allocate memory for packet", __FUNCTION__);
        return status;
    }
    // cleared in alloc.  fbe_set_memory(smp_edge_info,  0, sizeof(*smp_edge_info));

    status = fbe_api_common_send_control_packet (FBE_SMP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                                 smp_edge_info,
                                                 sizeof (fbe_smp_transport_control_get_edge_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info, 
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        fbe_api_free_memory (smp_edge_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    edge_info->path_attr = smp_edge_info->base_edge_info.path_attr;
    edge_info->path_state = smp_edge_info->base_edge_info.path_state;
    edge_info->client_id = smp_edge_info->base_edge_info.client_id;
    edge_info->transport_id = smp_edge_info->base_edge_info.transport_id;
    edge_info->client_index = smp_edge_info->base_edge_info.client_index;
    edge_info->server_id = smp_edge_info->base_edge_info.server_id;
    edge_info->server_index = smp_edge_info->base_edge_info.server_index;

    fbe_api_free_memory (smp_edge_info);

    return status;

}

/*!***************************************************************
 * @fn fbe_api_set_object_to_state(
 *     fbe_object_id_t object_id, 
 *     fbe_lifecycle_state_t requested_lifecycle_state, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function sets an object in a specific state.
 *
 * @param object_id         - object to query
 * @param requested_lifecycle_state - the state we want
 * @param package_id        - package ID
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    03/20/09: sharel  created
 *    
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_object_to_state(fbe_object_id_t object_id, fbe_lifecycle_state_t requested_lifecycle_state, fbe_package_id_t package_id)
{
    fbe_status_t                                         status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t              status_info = {0};
    fbe_base_object_control_code_t                       base_obj_opcode = FBE_BASE_OBJECT_CONTROL_CODE_INVALID;

    switch (requested_lifecycle_state) {
    case FBE_LIFECYCLE_STATE_ACTIVATE:
        base_obj_opcode = FBE_BASE_OBJECT_CONTROL_CODE_SET_ACTIVATE_COND;
        break;
    case FBE_LIFECYCLE_STATE_HIBERNATE:
        base_obj_opcode = FBE_BASE_OBJECT_CONTROL_CODE_SET_HIBERNATE_COND;
        break;
    case FBE_LIFECYCLE_STATE_OFFLINE:
        base_obj_opcode = FBE_BASE_OBJECT_CONTROL_CODE_SET_OFFLINE_COND;
        break;
    case FBE_LIFECYCLE_STATE_FAIL:
        base_obj_opcode = FBE_BASE_OBJECT_CONTROL_CODE_SET_FAIL_COND;
        break;
    case FBE_LIFECYCLE_STATE_DESTROY:
        base_obj_opcode = FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND;
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s:unable to set obj to state:%d\n",
                        __FUNCTION__, requested_lifecycle_state);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    status = fbe_api_common_send_control_packet (base_obj_opcode,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);
    

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_get_object_death_reason(
 *     fbe_object_id_t object_id, 
 *     fbe_object_death_reason_t *death_reason, 
 *     const fbe_u8_t **death_reason_str, 
 *     fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function checks why an object is at FAILED state.
 *
 * @param object_id         - object to query
 * @param death_reason      - return results pointer
 * @param death_reason_str  - a pointer to a string for the reason
 * @param package_id        - package ID
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *    06/11/09: sharel  created
 *    
 ****************************************************************/
fbe_status_t fbe_api_get_object_death_reason(fbe_object_id_t object_id, 
                                             fbe_object_death_reason_t *death_reason, 
                                             const fbe_u8_t **death_reason_str, 
                                             fbe_package_id_t package_id)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t             status_info = {0};
    fbe_discovery_transport_control_get_death_reason_t  get_death_reason = {0};
    death_reason_to_str_t *                             map_to_str = death_opcode_to_str_table;

    if (object_id >= FBE_OBJECT_ID_INVALID || death_reason == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }


    status = fbe_api_common_send_control_packet (FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DEATH_REASON,
                                                 &get_death_reason,
                                                 sizeof (fbe_discovery_transport_control_get_death_reason_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *death_reason = get_death_reason.death_reason;
    if (death_reason_str != NULL) {
        while (map_to_str->death_reason != FBE_DEATH_REASON_INVALID) {
            if (map_to_str->death_reason == get_death_reason.death_reason) {
                *death_reason_str = map_to_str->death_reason_str;
                return status;
            }
    
            map_to_str++;
        }
    
        *death_reason_str = map_to_str->death_reason_str;/*this would populate the last defult message*/
    }

    return status;
}

/*!********************************************************************
 * @fn fbe_api_get_total_objects_of_class(
 *     fbe_class_id_t class_id, 
 *     fbe_package_id_t package_id, 
 *     fbe_u32_t *total_objects_p)
 *********************************************************************
 * @brief return a count of all objects with this 
 *
 * @param class_id - class id to get count of.
 * @param package_id - package to look in.
 * @param total_objects_p - Total number of objects (out).
 *
 * @return fbe_status_t
 *
 * @version
 *  1/8/2010 - Created. Rob Foley
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_total_objects_of_class(fbe_class_id_t class_id, 
                                                             fbe_package_id_t package_id, 
                                                             fbe_u32_t *total_objects_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info = {0};
    fbe_topology_control_get_total_objects_of_class_t total_objects = {0};

    total_objects.total_objects = 0;
    total_objects.class_id = class_id;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                                           &total_objects,
                                                           sizeof(fbe_topology_control_get_total_objects_of_class_t),
                                                           FBE_SERVICE_ID_TOPOLOGY,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *total_objects_p = total_objects.total_objects;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_enumerate_class(
 *     fbe_class_id_t class_id,
 *     fbe_package_id_t package_id,
 *     fbe_object_id_t ** obj_array_p, 
 *     fbe_u32_t *num_objects_p, 
 *****************************************************************
 * @brief
 *   This function returns a list of all objects in the system - the caller must free the memory allocated for the objects
 *
 * @param class_id          - class ID
 * @param package_id        - package ID
 * @param obj_array_p       - array object to fill
 * @param num_objects_p     - pointer to actual objec
 *
 * @return fbe_status_t - success or failure.
 *
 * @version
 *  10/10/08: sharel  created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_enumerate_class(fbe_class_id_t class_id,
                                                  fbe_package_id_t package_id,
                                                  fbe_object_id_t ** obj_array_p, 
                                                  fbe_u32_t *num_objects_p )
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               alloc_size = {0};
    fbe_api_control_operation_status_info_t status_info = {0};
    fbe_topology_control_enumerate_class_t  enumerate_class = {0};/*structure is small enough to be on the stack*/
    
    /* one for the entry and one for the NULL termination.
     */
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_sg_element_t *                      temp_sg_list = NULL;

    status = fbe_api_get_total_objects_of_class(class_id, package_id, num_objects_p);
    if ( status != FBE_STATUS_OK ){
        return status;
    }

    if ( *num_objects_p == 0 ){
        *obj_array_p  = NULL;
        return status;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    temp_sg_list = sg_list;

    /* Allocate memory for the objects */
    alloc_size = sizeof (**obj_array_p) * *num_objects_p;
    *obj_array_p = fbe_api_allocate_memory (alloc_size);
    if (*obj_array_p == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        fbe_api_free_memory (sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    /*set up the sg list to point to the list of objects the user wants to get*/
    temp_sg_list->address = (fbe_u8_t *)*obj_array_p;
    temp_sg_list->count = alloc_size;
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    enumerate_class.number_of_objects = *num_objects_p;
    enumerate_class.class_id = class_id;
    enumerate_class.number_of_objects_copied = 0;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_to_service_with_sg_list(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                                                        &enumerate_class,
                                                                        sizeof (fbe_topology_control_enumerate_class_t),
                                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        sg_list,
                                                                        FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT, /* sg entries*/
                                                                        &status_info,
                                                                        package_id);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        fbe_api_free_memory (sg_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ( enumerate_class.number_of_objects_copied < *num_objects_p )
    {
        *num_objects_p = enumerate_class.number_of_objects_copied;
    }
    
    fbe_api_free_memory (sg_list);
    return FBE_STATUS_OK;

}

/*!********************************************************************
 * @fn fbe_api_get_total_objects_of_all_raid_groups(
 *     fbe_package_id_t package_id, 
 *     fbe_u32_t *total_objects_p)
 *********************************************************************
 * @brief return a count of all raid groups in the system.
 *
 * @param package_id - package to look in.
 * @param total_objects_p - Total number of objects (out).
 *
 * @return fbe_status_t
 *
 * @version
 *  04/30/2012 - Created. Vera Wang
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_total_objects_of_all_raid_groups(fbe_package_id_t package_id, 
                                                                       fbe_u32_t *total_objects_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t                          rg_class_id = {0};
    fbe_u32_t                               rg_count = {0};

    *total_objects_p = 0;

    for(rg_class_id = FBE_CLASS_ID_RAID_FIRST + 1; rg_class_id < FBE_CLASS_ID_RAID_LAST; rg_class_id++)
    {
        status = fbe_api_get_total_objects_of_class(rg_class_id, package_id, &rg_count);
        if (status != FBE_STATUS_OK) 
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get total_object_of_class with error:%d.\n", __FUNCTION__,
                           status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        (*total_objects_p) += rg_count;
    }

#ifndef NO_EXT_POOL_ALIAS
    status = fbe_api_get_total_objects_of_class(FBE_CLASS_ID_EXTENT_POOL, package_id, &rg_count);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Failed to get total_object_of_class with error:%d.\n", __FUNCTION__,
                       status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    (*total_objects_p) += rg_count;
#endif

    return status;
}


