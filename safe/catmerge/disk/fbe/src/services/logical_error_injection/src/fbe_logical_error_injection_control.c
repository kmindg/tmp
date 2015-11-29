/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_error_injection_control.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for sending control packets.
 *
 * @version
 *   12/22/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raid_library_proto.h"
#include "fbe_logical_error_injection_private.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe_database.h"
#include "fbe_logical_error_injection_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


static fbe_status_t fbe_logical_error_injection_send_control_packet(fbe_payload_control_operation_opcode_t control_code,
                                                                fbe_payload_control_buffer_t buffer,
                                                                fbe_payload_control_buffer_length_t buffer_length,
                                                                fbe_object_id_t object_id,
                                                                fbe_packet_attr_t attr,
                                                                fbe_package_id_t package_id);

static fbe_status_t fbe_logical_error_injection_check_error_record_match(fbe_packet_t* packet_p,
                                                                         fbe_raid_fruts_t *fruts_p,
                                                                         fbe_bool_t* inject_error_p);
static fbe_status_t 
fbe_logical_error_injection_check_block_io_error_record_match(fbe_packet_t* packet_p,
                                                              fbe_bool_t* error_injected_p);


static fbe_status_t fbe_logical_error_injection_delay_io(fbe_packet_t * packet_p, 
                                                         fbe_bool_t * inject_error);



/*!**************************************************************
 * fbe_logical_error_injection_send_control_packet()
 ****************************************************************
 * @brief
 *  Send a control packet.
 *
 * @param control_code
 * @param buffer
 * @param buffer_length
 * @param object_id
 * @param attr
 * @param package_id
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_logical_error_injection_send_control_packet(
    fbe_payload_control_operation_opcode_t control_code,
    fbe_payload_control_buffer_t buffer,
    fbe_payload_control_buffer_length_t buffer_length,
    fbe_object_id_t object_id,
    fbe_packet_attr_t attr,
    fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_transport_allocate_packet failed\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation(control_operation,
                                        control_code,
                                        buffer,
                                        buffer_length);
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not.
     */
    fbe_transport_set_packet_attr(packet_p, attr);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet_p);

    /* Wait for completion.
     * The packet is always completed so the status above need not be checked.
     */
    fbe_transport_wait_completion(packet_p);

    /* Save the status for returning.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_release_packet(packet_p);

    return status;
}
/******************************************
 * end fbe_logical_error_injection_send_control_packet()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_get_total_objects_of_class()
 ****************************************************************
 * @brief
 *  Get count of all objects of a class.
 *
 * @param class_id - class to enumerate.
 * @param package_id - Package to enumerate.
 * @param total_objects_p - output count of objects.      
 *
 * @return - fbe_status_t
 *
 * @author
 *  1/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_get_total_objects_of_class(fbe_class_id_t class_id,
                                                                    fbe_package_id_t package_id,
                                                                    fbe_u32_t *total_objects_p)
{   
    fbe_status_t status;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_topology_control_get_total_objects_of_class_t get_total;

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s: fbe_transport_allocate_packet() failed\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    get_total.class_id = class_id;

    fbe_payload_control_build_operation(control_p,
                                        FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                        &get_total,
                                        sizeof(fbe_topology_control_get_total_objects_of_class_t));
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Our callback is always called, no need to check status here.
     */
    fbe_service_manager_send_control_packet(packet_p);
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_release_packet(packet_p);

    *total_objects_p = get_total.total_objects;
    return status;
}
/******************************************
 * end fbe_logical_error_injection_get_total_objects_of_class()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_enumerate_class_objects()
 ****************************************************************
 * @brief
 *  Enumerate all the objects of a given class.
 *
 * @param class_id - class to enumerate.
 * @param package_id - Package to enumerate.
 * @param enumerate_class_p - ptr to structure for enumerating
 *                            a class.               
 *
 * @return - fbe_status_t
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_enumerate_class_objects(fbe_class_id_t class_id,
                                               fbe_package_id_t package_id,
                                               fbe_object_id_t *object_list_p,
                                               fbe_u32_t total_objects,
                                               fbe_u32_t *actual_num_objects_p)
{   
    fbe_status_t status;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_topology_control_enumerate_class_t enumerate_class;
    fbe_sg_element_t sg[FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT];
    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s: fbe_transport_allocate_packet() failed\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (package_id == FBE_PACKAGE_ID_PHYSICAL)
    {
        fbe_transport_initialize_packet(packet_p);
    }
    else
    {
        fbe_transport_initialize_sep_packet(packet_p);
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_ex_set_sg_list(payload_p, &sg[0], 2);

    sg[0].address = (fbe_u8_t *)object_list_p;
    sg[0].count = total_objects * sizeof(fbe_object_id_t);
    sg[1].address = NULL;
    sg[1].count = 0;

    enumerate_class.number_of_objects = total_objects;
    enumerate_class.class_id = class_id;

    fbe_payload_control_build_operation(control_p,
                                        FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                        &enumerate_class,
                                        sizeof(fbe_topology_control_enumerate_class_t));
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Set packet address */
    fbe_transport_set_address(packet_p,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    /* Our callback is always called, no need to check status here.
     */
    fbe_service_manager_send_control_packet(packet_p);
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_release_packet(packet_p);

    *actual_num_objects_p = enumerate_class.number_of_objects_copied;
    return status;
}
/******************************************
 * end fbe_logical_error_injection_enumerate_class_objects()
 ******************************************/

/*!***************************************************************************
 *          fbe_logical_error_injection_enumerate_system_objects()
 *****************************************************************************
 *
 * @brief   Enumerate all the system objects
 *
 * @param   class_id - class to enumerate.
 * @param   system_object_list_p - ptr to structure for enumerating a class.
 * @param   total_objects - Max objects that can be enumerated
 * @param   actual_num_objects_p - Pointer to actual number of objects in list               
 *
 * @return - fbe_status_t
 *
 * @author
 *  04/07/2010  Ron Proulx  - Created
 *
 *****************************************************************************/

fbe_status_t fbe_logical_error_injection_enumerate_system_objects(fbe_class_id_t class_id,
                                                fbe_object_id_t *system_object_list_p,
                                                fbe_u32_t total_objects,
                                                fbe_u32_t *actual_num_objects_p)
{   
    fbe_status_t status;
    fbe_packet_t * packet_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_database_control_enumerate_system_objects_t enumerate_system_objects;
    fbe_sg_element_t sg[FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT];

    /*! @todo Currently enumerating by class id isn't supported.
     */
    if (class_id != FBE_CLASS_ID_INVALID)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: class_id: 0x%x currently not supported.\n", __FUNCTION__, class_id);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* Allocate packet.
     */
    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: fbe_transport_allocate_packet() failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_ex_set_sg_list(sep_payload_p, &sg[0], 2);

    sg[0].address = (fbe_u8_t *)system_object_list_p;
    sg[0].count = total_objects * sizeof(fbe_object_id_t);
    sg[1].address = NULL;
    sg[1].count = 0;

    enumerate_system_objects.class_id = class_id;
    enumerate_system_objects.number_of_objects = total_objects;
    enumerate_system_objects.number_of_objects_copied = 0;

    fbe_payload_control_build_operation(control_p,
                                        FBE_DATABASE_CONTROL_CODE_ENUMERATE_SYSTEM_OBJECTS,
                                        &enumerate_system_objects,
                                        sizeof(fbe_database_control_enumerate_system_objects_t));
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Set packet address (configuration service lives in SEP_0) */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);

    /* Our callback is always called, no need to check status here.
     */
    fbe_service_manager_send_control_packet(packet_p);
    fbe_transport_wait_completion(packet_p);

    /* Save status before we release packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_release_packet(packet_p);

    *actual_num_objects_p = enumerate_system_objects.number_of_objects_copied;
    return status;
}
/********************************************************
 * end fbe_logical_error_injection_enumerate_system_objects()
 ********************************************************/

/*!***************************************************************************
 *          fbe_logical_error_injection_is_system_object()
 *****************************************************************************
 *
 * @brief   Check if the object id is system object by walk the list of
 *          system objects passed.
 *
 * @param   object_id_to_check - Object id to check if system object
 * @param   system_object_list_p - ptr to list of system objects
 * @param   num_system_objects - Number of system objects in list passed
 *
 * @return - fbe_status_t
 *
 * @author
 *  04/09/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_logical_error_injection_is_system_object(fbe_object_id_t object_id_to_check,
                                      fbe_object_id_t *system_object_list_p,
                                      fbe_u32_t num_system_objects)
{
    fbe_bool_t  b_found = FBE_FALSE;
    fbe_u32_t   object_index;

    /* Simply walk the system object list and set found.
     */
    for (object_index = 0; 
         ((system_object_list_p != NULL) && (object_index < num_system_objects)); 
         object_index++)
    {
        if (object_id_to_check == system_object_list_p[object_index])
        {
            b_found = FBE_TRUE;
            break;
        }
    }

    return(b_found);
}
/********************************************************
 * end fbe_logical_error_injection_is_system_object()
 ********************************************************/

/*!***************************************************************************
 *          fbe_logical_error_injection_enumerate_class_exclude_system_objects()
 *****************************************************************************
 *
 * @brief   Enumerate all the objects of a given class but exclude any system
 *          objects
 *
 * @param   class_id - class to enumerate.
 * @param   package_id - Package to enumerate.
 * @param   non_system_object_list_p - ptr to structure for enumerating a class.
 * @param   max_class_objects - Max objects that can be enumerated
 * @param   actual_num_objects_p - Pointer to actual number of objects in list               
 *
 * @return - fbe_status_t
 *
 * @author
 *  04/07/2010  Ron Proulx  - Created
 *
 *****************************************************************************/

fbe_status_t fbe_logical_error_injection_enumerate_class_exclude_system_objects(fbe_class_id_t class_id,
                                                              fbe_package_id_t package_id,
                                                              fbe_object_id_t *non_system_object_list_p,
                                                              fbe_u32_t max_class_objects,
                                                              fbe_u32_t *actual_num_objects_p)
{   
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       object_index;
    fbe_u32_t       total_class_objects = 0;
    fbe_u32_t       max_system_objects = 512 / sizeof(fbe_object_id_t);
    fbe_u32_t       total_system_objects = 0;
    fbe_u32_t       non_system_objects = 0;
    fbe_object_id_t *class_obj_list_p = NULL;
    fbe_object_id_t *system_obj_list_p = NULL;

    /* Allocate a list so that we can remove the system objects.  The size
     * is simply the maximum class objects allowed
     */
    class_obj_list_p = (fbe_object_id_t *)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * max_class_objects);
    if (class_obj_list_p == NULL) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: cannot allocate memory for enumerate of class id: 0x%x package: %d objs: %d\n", 
                                                  __FUNCTION__, class_id, package_id, max_class_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_set_memory(class_obj_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * max_class_objects);

    /* Now enumerate all the objects for the class specified into the list
     * that we have allocated.
     */
    status = fbe_logical_error_injection_enumerate_class_objects(class_id, 
                                               package_id, 
                                               class_obj_list_p, 
                                               max_class_objects, 
                                               &total_class_objects);
    if (status != FBE_STATUS_OK) 
    {
        fbe_memory_native_release(class_obj_list_p);
        return(status);
    }

    /* Now enumerate all the system objects.
     */
    system_obj_list_p = (fbe_object_id_t *)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * max_system_objects);
    if (system_obj_list_p == NULL) 
    {
        fbe_memory_native_release(class_obj_list_p);
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s: cannot allocate memory for enumerate of class id: 0x%x package: %d objs: %d\n", 
                                                  __FUNCTION__, class_id, package_id, max_system_objects);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_set_memory(system_obj_list_p, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * max_system_objects);
    
    /*! @todo Currently cannot enumerate system objects by class.
     */
    if (package_id == FBE_PACKAGE_ID_SEP_0)
    {
        status = fbe_logical_error_injection_enumerate_system_objects(FBE_CLASS_ID_INVALID, 
                                                                      system_obj_list_p, 
                                                                      max_system_objects, 
                                                                      &total_system_objects);
        if (status != FBE_STATUS_OK)
        {
            fbe_memory_native_release(class_obj_list_p);
            fbe_memory_native_release(system_obj_list_p);
            return(status);
        }
    }

    /* Now walk thru the list of object ids for the requested class and only
     * copy the non-system objects into the passed buffer.
     */
    for (object_index = 0; object_index < total_class_objects; object_index++)
    {
        /* Invoke method that will check if the current object is a system
         * object or not.
         */
        if ((package_id == FBE_PACKAGE_ID_SEP_0)                                             &&
            (fbe_logical_error_injection_is_system_object(class_obj_list_p[object_index],
                                                          system_obj_list_p,
                                                          total_system_objects) == FBE_TRUE)     )
        {
            /* Don't copy this object id.
             */
        }
        else
        {
            /* Copy the object id to the list passed.
             */
            non_system_object_list_p[non_system_objects] = class_obj_list_p[object_index];
            non_system_objects++;
        }
    }

    /* Now update the actual number of objects in the list and free the 
     * memory we allocated.
     */
    *actual_num_objects_p = non_system_objects;
    fbe_memory_native_release(class_obj_list_p);
    fbe_memory_native_release(system_obj_list_p);

    /* Always return the status.
     */
    return(status);
}
/*************************************************************************
 * end fbe_logical_error_injection_enumerate_class_exclude_system_objects()
 *************************************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_set_block_edge_hook()
 ****************************************************************
 * @brief
 *  Setup the edge hook for a given object.
 *
 * @param object_id - object to set hook for.
 * @param hook_info_p - Information on the hook to set.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_set_block_edge_hook(fbe_object_id_t object_id, 
                                                             fbe_package_id_t package_id,
                                                             fbe_transport_control_set_edge_tap_hook_t *hook_info_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_logical_error_injection_send_control_packet(FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK,
                                                             hook_info_p,
                                                             sizeof(fbe_transport_control_set_edge_tap_hook_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             package_id);
    return status;
}
/******************************************
 * end fbe_logical_error_injection_set_block_edge_hook()
 ******************************************/


/*!**************************************************************
 * fbe_logical_error_injection_validate_block_operation()
 ****************************************************************
 *
 * @brief
 *    This function checks that the specified packet contains a
 *    valid  block operation payload for either a physical or a
 *    storage extent (sep) package.
 *
 * @param   in_packet_p   -  pointer to control packet
 *
 * @return  fbe_status_t  -  FBE_STATUS_GENERIC_FAILURE
 *                           FBE_STATUS_OK
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_status_t
fbe_logical_error_injection_validate_block_operation( fbe_packet_t* in_packet_p )
{
    fbe_payload_block_operation_t*  block_operation_p;
    fbe_payload_ex_t*               payload_p;

    /* insure packet's current level is valid
     */
    if ( (in_packet_p->current_level >= 0) && (in_packet_p->current_level < FBE_PACKET_STACK_SIZE) )
    {
        /* check for a valid block operation
         */
        /* check for package payload in packet
         */
        payload_p = fbe_transport_get_payload_ex( in_packet_p );
        if ( payload_p == NULL )
        {
            /* trace package payload error
             */
            fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_ERROR, 
                                                       FBE_TRACE_MESSAGE_ID_INFO,
                                                       "%s no payload in packet %p\n", 
                                                       __FUNCTION__, in_packet_p       );
                
            /* return generic failure status
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
            
        /* check for block operation in payload
         */
        block_operation_p = fbe_payload_ex_get_block_operation( payload_p );
        if ( block_operation_p == NULL )
        {
            /* trace block operation error
             */
            fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_INFO,
                                                       "%s no block operation in packet %p\n", 
                                                       __FUNCTION__, in_packet_p               );
                
            /* return generic failure status
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }
        

        /* block operation payload is valid, return success status
         */
        return FBE_STATUS_OK;
    }
    else
    {
        /* trace unexpected packet level error
         */
        fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_ERROR, 
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s packet level is %d in packet %p\n", 
                                                   __FUNCTION__, in_packet_p->current_level, in_packet_p );

        /* return generic failure status
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

}   /* end fbe_logical_error_injection_validate_block_operation() */


/*!**************************************************************
 * fbe_logical_error_injection_validate_fruts()
 ****************************************************************
 * @brief
 *  Validate and return the fruts ptr.
 *
 * @param  packet_p - Packet to inject on.          
 * @param fruts_pp - Ptr to fruts to return.     
 *
 * @return fbe_status_t   
 *
 * @author
 *  1/5/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_validate_fruts(fbe_packet_t * packet_p,
                                                        fbe_raid_fruts_t **fruts_pp)
{
    fbe_raid_fruts_t *fruts_p = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if ((packet_p->current_level >= 0) && (packet_p->current_level < FBE_PACKET_STACK_SIZE))
    {
        fruts_p = (fbe_raid_fruts_t *)packet_p->stack[packet_p->current_level].completion_context;
        if ((fruts_p->common.flags & FBE_RAID_COMMON_FLAG_TYPE_ALL_STRUCT_TYPE_MASK) == FBE_RAID_COMMON_FLAG_TYPE_FRU_TS)
        {
            fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
            /* Appears to be a valid fruts.
             */
            if (siots_p == NULL)
            {
                /* Parent not set in fruts.  Not a valid fruts.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_SIOTS) ||
                     fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS))
            {
                /* Siots appears valid.  
                 */
                status = FBE_STATUS_OK;
            }
            else
            {
                /* Unexpected siots flags.
                 */ 
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                          FBE_TRACE_MESSAGE_ID_INFO,
                                                          "%s unexpected siots common flags 0x%x siots: %p fruts: %p packet: %p\n", 
                                                          __FUNCTION__, siots_p->common.flags, siots_p, fruts_p, packet_p);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    else
    {
        /* Packet level unexpected.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s packet level is %d packet %p\n", 
                                                  __FUNCTION__, packet_p->current_level, packet_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    /* Only return the fruts if we validated it.
     */
    if (status == FBE_STATUS_OK)
    {
        *fruts_pp = fruts_p;
    }
    else
    {
        *fruts_pp = NULL;
    }
    return status;
}
/******************************************
 * end fbe_logical_error_injection_validate_fruts()
 ******************************************/


/*!**************************************************************
 * fbe_logical_error_injection_block_io_packet_completion()
 ****************************************************************
 * 
 * @brief
 *    This is a test completion function to handle injecting
 *    errors on the specified block operation.
 * 
 * @param   in_packet_p   -  pointer to control packet
 * @param   in_context    -  context for block operation
 *
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @author
 *    04/16/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_status_t
fbe_logical_error_injection_block_io_packet_completion( fbe_packet_t*                   in_packet_p, 
                                                        fbe_packet_completion_context_t in_context  )
{
    fbe_status_t                    status;

    /* Set the base edge as it was when we came in so we can figure out which object we are injecting for.
     */
    in_packet_p->base_edge = (fbe_base_edge_t*)in_context;
    
    /* check for a valid block operation payload
     */
    status = fbe_logical_error_injection_validate_block_operation( in_packet_p );

    /* if this block operation payload validated
     * successfully then try to inject an error
     */
    if ( status == FBE_STATUS_OK )
    {
        fbe_logical_error_injection_block_io_inject_all( in_packet_p );
    }

    /* always return success to continue completing the packet upstream
     */
    return FBE_STATUS_OK;

}   /* end fbe_logical_error_injection_block_io_packet_completion() */


/*!**************************************************************
 * fbe_logical_error_injection_add_block_io_packet_completion()
 ****************************************************************
 *
 * @brief
 *    This function checks that the specified packet contains a
 *    valid  block operation payload for either a physical or a
 *    storage extent package. If one is found, it adds an error
 *    injection completion function to the completion stack for
 *    this block operation.
 *
 * @param   in_packet_p   -  pointer to control packet
 *
 * @return  fbe_status_t  -  FBE_STATUS_CONTINUE
 *                           FBE_STATUS_GENERIC_FAILURE
 *                           FBE_STATUS_OK
 *
 * @author
 *    05/12/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_status_t
fbe_logical_error_injection_add_block_io_packet_completion( fbe_packet_t* in_packet_p )
{
    fbe_packet_completion_context_t    context;
    fbe_status_t                       status;
    
    /* check for a valid block operation payload
     */
    status = fbe_logical_error_injection_validate_block_operation( in_packet_p );
    
    /* if unable to validate either fruts or block operation
     * then set failure status in packet and complete packet
     */
    if ( status != FBE_STATUS_OK )
    {
        fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_ERROR, 
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s unable to validate fruts or block operation for packet %p\n", 
                                                   __FUNCTION__, in_packet_p
                                                  );
 
        fbe_transport_set_status( in_packet_p, status, 0 );
        fbe_transport_complete_packet_async( in_packet_p );
        return status;
    }

    /* found a valid block operation payload; so retrieve
     * completion context for this packet's current level
     */
    context = fbe_logical_error_injection_get_completion_context( in_packet_p );
        
    /* now, add error injection completion function to
     * the completion stack for this block operation
     */
    status = fbe_transport_set_completion_function( in_packet_p,
                                                    fbe_logical_error_injection_block_io_packet_completion,
                                                    in_packet_p->base_edge);
    
    /* if unable to add error injection completion function
     * then set failure status in packet and complete packet
     */
    if ( status != FBE_STATUS_OK )
    {
        fbe_logical_error_injection_service_trace( FBE_TRACE_LEVEL_ERROR, 
                                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                   "%s unable to set completion %p\n", __FUNCTION__, in_packet_p
                                                 );
        
        fbe_transport_set_status( in_packet_p, status, 0 );
        fbe_transport_complete_packet_async( in_packet_p );
    }
    
    /* otherwise, let the block operation continue downstream;
     * it will complete to us later and we will inject the error 
     */
    else
    {
        status = FBE_STATUS_CONTINUE;
    }

    return status;
}   // end fbe_logical_error_injection_add_block_io_packet_completion()


/*!***************************************************************
 * fbe_logical_error_injection_packet_completion()
 ****************************************************************
 * 
 * @brief   This is a test completion function for a
 *          packet we are injecting errors on.
 * 
 * @param packet_p - The packet being completed.
 * @param context - The fruts ptr.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  1/5/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_packet_completion(fbe_packet_t * packet_p, 
                                                           fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *fruts_p = NULL;
    fbe_raid_fruts_t *packet_fruts_p = NULL;

    /* First get and validate the fruts associated with this packet.
     */
    fruts_p = (fbe_raid_fruts_t *)context;
    status = fbe_logical_error_injection_validate_fruts(packet_p, &packet_fruts_p);
    if (status != FBE_STATUS_OK)
    {
        /* Fruts did not get validated.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s unable to validate fruts for packet %p\n", 
                                                  __FUNCTION__, packet_p);
    }
    else if (fruts_p != packet_fruts_p)
    {
        /* Fruts returned does not match the context.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s unable to validate fruts for packet %p\n", 
                                                  __FUNCTION__, packet_p);
    }
    else
    {
        fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p); 
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
        fbe_logical_error_injection_object_t *object_p = NULL;

        /* Fetch the object ptr.
         * We lock here to prevent the object from going out of existence 
         * while we check to see if it is still enabled. 
         */
        fbe_logical_error_injection_lock();
        object_p = fbe_logical_error_injection_find_object_no_lock(object_id);

        if (object_p == NULL)
        {
            fbe_logical_error_injection_unlock();
            /* We are not injecting for this object, so not sure why we are here.
             */
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                                      "%s object_id: 0x%x was not found "
                                                      "alg: %d pos: %d lba: %llx blocks: 0x%llx opcode: %d\n",
                                                      __FUNCTION__, object_id, siots_p->algorithm, 
                                                      fruts_p->position, (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks, fruts_p->opcode);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_logical_error_injection_object_lock(object_p);
        if (!fbe_logical_error_injection_object_is_enabled(object_p))
        {
            /* This object is not injecting errors.
             */
            fbe_logical_error_injection_object_dec_in_progress(object_p);
            fbe_logical_error_injection_object_unlock(object_p);
            fbe_logical_error_injection_unlock();
            return FBE_STATUS_OK;
        }
        fbe_logical_error_injection_object_unlock(object_p);
        fbe_logical_error_injection_unlock();

        /* If we are here everything checked out OK, so try to inject the error. 
         */
        status = fbe_logical_error_injection_fruts_inject_all(fruts_p);
        if(LEI_COND(status != FBE_STATUS_OK))
        {
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "logical error injection: inject_media_error op:0x%x " 
                "pos:0x%x, Lba:0x%llX Blks:0x%llx \n",
                fruts_p->opcode, fruts_p->position,
		(unsigned long long)fruts_p->lba,
		(unsigned long long)fruts_p->blocks); 
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "%s unable to inject error for fruts %p\n", 
                                                    __FUNCTION__, fruts_p);
        }
        /* Now that we are done with injecting errors, let's decrement our in progess count.
         */
        fbe_logical_error_injection_object_lock(object_p);
        fbe_logical_error_injection_object_dec_in_progress(object_p);
        fbe_logical_error_injection_object_unlock(object_p);
    }
    /* Always return OK to continue completing the packet upstream.
     */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_logical_error_injection_packet_completion()
 **************************************/


/*!**************************************************************
 * fbe_logical_error_injection_hook_function()
 ****************************************************************
 * @brief
 *    This is the main hook function that we put onto the edge.
 *    It handles the following tasks:
 *
 *     1. Checking that the specified packet contains either a
 *        valid fruts or block operation payload*
 *     2. Adding apropriate error injection completion function
 *        to packet's completion stack
 *
 * @param  packet_p - Packet to inject on.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_hook_function(fbe_packet_t *packet_p)
{
    fbe_raid_fruts_t *fruts_p = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t      error_injected = FBE_FALSE;
    fbe_raid_siots_t *siots_p = NULL; 
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_object_id_t object_id;
	fbe_payload_ex_t * payload = NULL;
    fbe_logical_error_injection_object_t *object_p = NULL;
    
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s called with packet %p\n", __FUNCTION__, packet_p);

    /* Check if this is the `remove hook' control code.
     */
	payload = fbe_transport_get_payload_ex(packet_p);
    if (((fbe_payload_operation_header_t *)payload->current_operation)->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        fbe_payload_control_operation_t                *control_operation_p = NULL; 
        fbe_payload_control_operation_opcode_t          opcode;
        fbe_transport_control_remove_edge_tap_hook_t   *remove_hook_p = NULL;

        control_operation_p = fbe_payload_ex_get_control_operation(payload);
        fbe_payload_control_get_opcode(control_operation_p, &opcode);
        if (opcode == FBE_BLOCK_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK)
        {
            fbe_payload_control_get_buffer(control_operation_p, &remove_hook_p);
            fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                      "%s request to remove hook for obj: 0x%x\n", 
                                                      __FUNCTION__, remove_hook_p->object_id);
            object_p = fbe_logical_error_injection_find_object_no_lock(remove_hook_p->object_id);
            if (object_p == NULL)
            {
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                          "%s failed to locate obj: 0x%x for remove hook\n", 
                                                          __FUNCTION__, remove_hook_p->object_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Now destroy this object his object
             */
            status = fbe_logical_error_injection_destroy_object(object_p);
            return status;
        }
    }

    /* Check if we need to delay the I/O.
     */
    status = fbe_logical_error_injection_delay_io(packet_p, &error_injected);
    if (error_injected) 
    {
        return status; 
    }

    /* check for a valid fruts payload
     */
    status = fbe_logical_error_injection_validate_fruts(packet_p, &fruts_p);
    
    /* if unable to validate a fruts payload in this packet
     * then check for a valid block operation; if found then
     * add error injection completion function for block i/o
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_logical_error_injection_check_block_io_error_record_match(packet_p,
                                                                      &error_injected);
        /* If the error is already injected above, we are done if not, add the
         * completion */
        if(!error_injected)
        {
            status = fbe_logical_error_injection_add_block_io_packet_completion(packet_p);
        }

        return status;
    }

    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    /* Fetch the object ptr.
     * We grab this lock since it prevents the object from going out 
     * of existence while we check to see if it enabled. 
     */
    fbe_logical_error_injection_lock();
    object_p = fbe_logical_error_injection_find_object_no_lock(object_id);

    if (object_p == NULL)
    {
        fbe_logical_error_injection_unlock();
        /* We are not injecting for this object, so not sure why we are here.
         */
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                                  "%s object_id: 0x%x was not found "
                                                  "alg: %d pos: %d lba: %llx blocks: 0x%llx opcode: %d\n",
                                                  __FUNCTION__, object_id, siots_p->algorithm, 
                                                  fruts_p->position, (unsigned long long)fruts_p->lba, (unsigned long long)fruts_p->blocks, fruts_p->opcode);
        return FBE_STATUS_CONTINUE;
    }

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                              "%s obj: 0x%x called with packet %p\n", 
                                              __FUNCTION__, object_p->object_id, packet_p);

    fbe_logical_error_injection_object_lock(object_p);
    if (!fbe_logical_error_injection_object_is_enabled(object_p))
    {
        /* This object is not injecting errors.
         */
        fbe_logical_error_injection_object_unlock(object_p);
        fbe_logical_error_injection_unlock();
        return FBE_STATUS_CONTINUE;
    }
    fbe_logical_error_injection_object_unlock(object_p);

     /* If the error record matches return from here */
    status = fbe_logical_error_injection_check_error_record_match(packet_p, 
                                                                  fruts_p,
                                                                  &error_injected);
    if(error_injected)
    {
        /* Error already injected return */
        fbe_logical_error_injection_unlock();
        return status;
    }

    
    /* We inc the number of in progress errors, so the lei object does not get destroyed 
     * while we are using it. 
     */
    fbe_logical_error_injection_object_lock(object_p);
    fbe_logical_error_injection_object_inc_in_progress(object_p);
    fbe_logical_error_injection_object_unlock(object_p);
    fbe_logical_error_injection_unlock();

    /* Put ourselves on the completion stack of the fruts.
     */
    status = fbe_transport_set_completion_function(packet_p,
                                                   fbe_logical_error_injection_packet_completion,
                                                   fruts_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                  "%s unable to set completion %p\n", __FUNCTION__, packet_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet_async(packet_p);
    }
    else
    {
        /* Otherwise let the operation continue downstream.
         * It will complete to us later and we will inject the error. 
         */
        status = FBE_STATUS_CONTINUE;
    }
    
    return status;
}
/******************************************
 * end fbe_logical_error_injection_hook_function()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_check_block_io_error_record_match()
 ****************************************************************
 * @brief
 *    This checks whether there is a error record that matches our
 *    criteria. 
 *
 * @param  packet_p - Packet to inject error on.
 * @param  error_injected_p - output parameter that is true if record match 
 *
 * @return fbe_status_t   
 *
 * @author
 *  04/06/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_logical_error_injection_check_block_io_error_record_match(fbe_packet_t* packet_p,
                                                              fbe_bool_t* error_injected_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    *error_injected_p = FBE_FALSE;
#if 0 /* Disable the code for now since it is untested*/
    fbe_u32_t       err_index;
    fbe_payload_ex_t* payload_p;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_lba_t   start_lba;
    fbe_block_count_t block_count;
    fbe_lba_t   record_end_lba;
    fbe_lba_t   table_lba;

    
    /* We dont expect IO to this region but we got it. So return error */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Loop through all our error records 
     * searching for a match.
     */
    for (err_index = 0; err_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; err_index++)
    {
        /* If this fruts` position matches our record's position(s)
         * and this is the right opcode, then inject an error.
         * Mirrors will ignore the position bitmap and insert an error
         * based on the table entry. Odd entries will be position one,
         * and even will be zero.
         */
        fbe_logical_error_injection_record_t* rec_p = NULL;

        fbe_logical_error_injection_get_error_info(&rec_p, err_index);

        /* Get the normalized table start lba. 
         */
        table_lba = fbe_logical_error_injection_get_table_lba(NULL, start_lba, block_count );

        /* Get the end lba.
         */
        record_end_lba = rec_p->lba;   
        
        /* Check for overlap.
         */     
        if (!fbe_logical_error_injection_overlap(table_lba, block_count, record_end_lba, rec_p->blocks)) 
        {
            continue;
        }

        /* Switch on error type.
         */
        switch (rec_p->err_type)
        {
            case FBE_XOR_ERR_TYPE_IO_UNEXPECTED:
                {
                    fbe_payload_block_set_status(block_operation_p,FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
                    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE,0); 
                    status = FBE_STATUS_GENERIC_FAILURE;
                    fbe_logical_error_injection_inc_num_errors_injected();
                    fbe_transport_complete_packet_async(packet_p);
                    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                              "%s: Return error since IO unexpected\n",__FUNCTION__);
                    *error_injected_p = FBE_TRUE;
                }
                break;
        }
        if (*error_injected_p == FBE_TRUE)
        {
            break; /* Break out of the loop*/
        }
      
    } /* for all error injection records */  

#endif
    return status; 

}
/******************************************
 * end fbe_logical_error_injection_check_block_io_error_record_match()
 ******************************************/


/*!**************************************************************
 * fbe_logical_error_injection_check_error_record_match()
 ****************************************************************
 * @brief
 *    This checks whether there is a error record that matches our
 *    criteria. It supports 2 types of error timeout error and silent drop
 *
 * @param  packet_p - Packet to inject error on.
 * @param  fruts_p  - fru to inject error on
 * @param  inject_error_p - output parameter that is true if record match and it
 *                          is a write operation     
 *
 * @return fbe_status_t   
 *
 * @author
 *  06/21/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_logical_error_injection_check_error_record_match(fbe_packet_t* packet_p,
                                                                         fbe_raid_fruts_t *fruts_p,
                                                                         fbe_bool_t* inject_error_p)
{

    fbe_u32_t       err_index;
    fbe_bool_t      is_overlap;
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      record_match = FBE_FALSE;                      


    /* Loop through all our error records 
     * searching for a match.
     */
    for (err_index = 0; err_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; err_index++)
    {
        /* If this fruts` position matches our record's position(s)
         * and this is the right opcode, then inject an error.
         * Mirrors will ignore the position bitmap and insert an error
         * based on the table entry. Odd entries will be position one,
         * and even will be zero.
         */
        fbe_logical_error_injection_record_t* rec_p = NULL;

        fbe_logical_error_injection_get_error_info(&rec_p, err_index);


        /* Handle error tables that are common to all RAID types. 
         */
        is_overlap = rec_p->pos_bitmap & (1 << fruts_p->position);
        if (is_overlap)
        {
            /*! @todo Currently we ignore any other error injection parameters
             *        (i.e. skip_count etc).
             */

            /* Switch on error type.
             */
            switch(rec_p->err_type)
            {
                case FBE_XOR_ERR_TYPE_TIMEOUT_ERR:
                    /* For `write' types of errors we want to inject the error before the
                     * data is written to disk.  Otherwise a verify etc will not detect the
                     * error.
                     */
                    record_match = FBE_TRUE;
                    if(fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE &&
                       fbe_logical_error_injection_overlap(fruts_p->lba, fruts_p->blocks, rec_p->lba, rec_p->blocks))
                    {
                        
                       /* Fail the IO request. Set the status to IO failed and qualifier to
                         * unexpected error to simulate that we have
                         * taken an error on a write 
                         */ 
                        fbe_payload_ex_t* payload_p = fbe_transport_get_payload_ex(packet_p);
                        fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
                        fbe_payload_block_set_status(block_operation_p,FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
                        /* We set the packet status to edge not enabled to fake the drive is dead
                         */
                        fbe_transport_set_status(packet_p, FBE_STATUS_EDGE_NOT_ENABLED,0); 
                        status = FBE_STATUS_EDGE_NOT_ENABLED;
                        fbe_logical_error_injection_inc_num_errors_injected();
                        fbe_transport_complete_packet_async(packet_p);
                        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                                  "Injecting error in %d position fru l/b: %llx/%llx rec l/b: %llx/%llx\n", 
                                                                  fruts_p->position,
                                                                  fruts_p->lba, fruts_p->blocks, rec_p->lba, rec_p->blocks);
                        *inject_error_p = FBE_TRUE;
                        
                    }
                    break;

                case FBE_XOR_ERR_TYPE_SILENT_DROP:
                    /* `Silently' drop the request.  Currently we only do this
                     * for write operations.
                     */
                    record_match = FBE_TRUE;
                    if(fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
                    {
                        /* `Silently' drop the write by returning success.
                         */
                        fbe_payload_ex_t* payload_p = fbe_transport_get_payload_ex(packet_p);
                        fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
                        fbe_payload_block_set_status(block_operation_p,FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
                        fbe_transport_set_status(packet_p, FBE_STATUS_OK,0); 
                        status = FBE_STATUS_OK;
                        fbe_logical_error_injection_inc_num_errors_injected();
                        fbe_transport_complete_packet_async(packet_p);
                        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                                  "%s: Silently drop write for pos: %d lba: 0x%llx blks: 0x%llx\n", 
                                                                  __FUNCTION__, fruts_p->position, fruts_p->lba, fruts_p->blocks);
                        *inject_error_p = FBE_TRUE;
                    }
                    break;

                case FBE_XOR_ERR_TYPE_INCOMPLETE_WRITE:
                    /* For `write' types of errors we want to inject the error before the
                     * data is written to disk.
                     */
                    if ((fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) || 
                        (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY))
                    {
                        if (fbe_logical_error_injection_overlap(fruts_p->lba, fruts_p->blocks, rec_p->lba, rec_p->blocks)) 
                        {
                            fbe_block_count_t blk_cnt = 0;
                            fbe_payload_ex_t* payload_p = fbe_transport_get_payload_ex(packet_p);
                            fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    
                                
                            /* Decrease block count by 1. Decreasing by random value would require us to
                             * remember that value possibly by allocating a new block operation.
                             * On completion of this block operation block count is incremented by 1 and
                             * status is set as IO failed. 
                             */
                            fbe_payload_block_get_block_count(block_operation_p, &blk_cnt);
                            if (blk_cnt > 1)
                            {
                                blk_cnt--;
                                fbe_payload_block_set_block_count(block_operation_p, blk_cnt);
    
                                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                                          "%s: Injecting incomplete write error (writing one less block) at pos %d blk_cnt %d \n", 
                                                                          __FUNCTION__, fruts_p->position, (int)blk_cnt);
                                *inject_error_p = FBE_FALSE; /* Setting this falg to false, because we want IO to continue downstream */
                            }

                            record_match = FBE_TRUE;
                        }
                    }
                    break;

                default:
                    /* There wasn't a match.
                     */
                    break;

            } /* end switch on error type to inject */

            /* If there was a match break out of loop */
            if (record_match)
            {
                break;
            }

        }  /* end if overlap */

    } /* for all error injection records */  
    
    return status; 

}
/******************************************
 * end fbe_logical_error_injection_check_error_record_match()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_delay_packet_completion()
 ****************************************************************
 * @brief
 *   Check whether we should be delaying the packet on the way up
 *
 * @param  packet_p - Packet to inject on.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/24/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_delay_packet_completion(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context)
{
    
    fbe_logical_error_injection_object_t    *object_p = NULL;
    fbe_object_id_t                         client_id;
    fbe_logical_error_injection_record_t    *rec_p = (fbe_logical_error_injection_record_t*)context;

    fbe_logical_error_injection_get_client_id(packet_p, &client_id);
    fbe_logical_error_injection_lock();
    object_p = fbe_logical_error_injection_find_object_no_lock(client_id);
    if (object_p == NULL) 
    {
        fbe_logical_error_injection_unlock();
        return FBE_STATUS_CONTINUE;
    }

    fbe_logical_error_injection_object_lock(object_p);
    if (!fbe_logical_error_injection_object_is_enabled(object_p))
    { 
        fbe_logical_error_injection_object_unlock(object_p);
        fbe_logical_error_injection_unlock();
        return FBE_STATUS_CONTINUE;
    }

    fbe_logical_error_injection_inc_num_errors_injected();
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s Delaying the IO UP packet %p obj 0x%x \n", __FUNCTION__, packet_p, client_id);

    fbe_logical_error_injection_add_packet_to_delay_queue(packet_p, client_id, rec_p);
    fbe_logical_error_injection_object_unlock(object_p);
    fbe_logical_error_injection_unlock();
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_logical_error_injection_delay_packet_completion()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_delay_io()
 ****************************************************************
 * @brief
 *   Check whether we should be delaying the packet
 *
 * @param  packet_p - Packet to inject on.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/24/2012 - Created. Deanna Heng
 *
 ****************************************************************/

static fbe_status_t fbe_logical_error_injection_delay_io(fbe_packet_t * packet_p, 
                                                         fbe_bool_t * inject_error)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_logical_error_injection_object_t    *object_p = NULL;
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_object_id_t                         client_id = FBE_OBJECT_ID_INVALID;
    fbe_class_id_t                          class_id = FBE_CLASS_ID_INVALID;
    fbe_lba_t                               block_lba;
    fbe_block_count_t                       block_count;
    fbe_u32_t                               rec_index;
    fbe_logical_error_injection_record_t    *rec_p = NULL;
    fbe_raid_fruts_t                        *fruts_p = NULL;
    fbe_bool_t                              ignore_pos = FBE_FALSE;
    fbe_raid_siots_t                        *siots_p = NULL; 
    fbe_raid_geometry_t                     *raid_geometry_p = NULL;
    fbe_u32_t                               position;

    status = fbe_logical_error_injection_validate_fruts(packet_p, &fruts_p);

    /* Get the appropriate class id */
    if (status == FBE_STATUS_OK) 
    {
        siots_p = fbe_raid_fruts_get_siots(fruts_p);
        raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        client_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
        position = fruts_p->position;
    } 
    else 
    {
        /* for luns we do not calculate the drive position */
        fbe_base_transport_get_client_id(packet_p->base_edge, &client_id);
        ignore_pos = FBE_TRUE;
        fbe_logical_error_injection_lock();
        object_p = fbe_logical_error_injection_find_object_no_lock(client_id);
        if (object_p == NULL)
        {
            fbe_logical_error_injection_unlock();
            return FBE_STATUS_CONTINUE;
        }
        class_id = object_p->class_id;
        fbe_logical_error_injection_unlock();
        if (class_id != FBE_CLASS_ID_LUN && class_id != FBE_CLASS_ID_VIRTUAL_DRIVE ) 
        {
            return FBE_STATUS_CONTINUE;
        }
    }

    fbe_logical_error_injection_lock();
    object_p = fbe_logical_error_injection_find_object_no_lock(client_id);
    if (object_p == NULL)
    {
        fbe_logical_error_injection_unlock();
        *inject_error = FBE_FALSE;
        return FBE_STATUS_CONTINUE;
    }

    fbe_logical_error_injection_object_lock(object_p);
    if (!fbe_logical_error_injection_object_is_enabled(object_p))
    {
        fbe_logical_error_injection_object_unlock(object_p);
        fbe_logical_error_injection_unlock();
        *inject_error = FBE_FALSE;
        return FBE_STATUS_CONTINUE;
    }

    fbe_logical_error_injection_object_unlock(object_p);
    fbe_logical_error_injection_unlock();

    /* get the block operation from payload */
    payload_p         = fbe_transport_get_payload_ex( packet_p );
    block_operation_p = fbe_payload_ex_get_block_operation( payload_p );
    fbe_payload_block_get_lba( block_operation_p, &block_lba);
    fbe_payload_block_get_block_count( block_operation_p, &block_count);

    if (block_operation_p->block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ &&
        block_operation_p->block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE  &&
        block_operation_p->block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) 
    {
        /* This isn't really an operation we would want to delay
         */
        *inject_error = FBE_FALSE;
        return FBE_STATUS_CONTINUE;
    } 

    /* For the VD to PVD edge we have to take into the offset on the pvd
     */
    if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE ) 
    {
        block_lba -= block_operation_p->block_edge_p->offset;
        position = packet_p->base_edge->client_index;
        ignore_pos = FBE_FALSE;
    }

    /* loop through error injection records
     */
    for ( rec_index = 0; rec_index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; rec_index++ )
    {
        /* get pointer to selected error injection record
         */
        fbe_logical_error_injection_get_error_info( &rec_p, rec_index );

        /* if opcode is set in error record, only inject errors for that opcode
         */
        if (   rec_p->opcode != 0
            && block_operation_p->block_opcode != rec_p->opcode) 
        {
            /* not a match, skip to the next record */
            continue;
        } 

        if (ignore_pos || rec_p->pos_bitmap & (1 << position)) 
        {
            if (fbe_logical_error_injection_overlap(block_lba, block_count, rec_p->lba, rec_p->blocks)) 
            {
                if (rec_p->err_type == FBE_XOR_ERR_TYPE_DELAY_DOWN) 
                {
                    /* Delay the packet on the way down at this moment
                     */
                    *inject_error = FBE_TRUE;
                    fbe_logical_error_injection_inc_num_errors_injected();
                    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                                              "%s Delaying the IO down %p\n", __FUNCTION__, packet_p);
                    fbe_logical_error_injection_add_packet_to_delay_queue(packet_p, client_id, rec_p);
                    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
                } else if (rec_p->err_type == FBE_XOR_ERR_TYPE_DELAY_UP) 
                {
                    /* set the completion function to inject the error on the way up and continue
                     */
                    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                                              "%s Set completion function to delay the IO up %p 0x%x\n", __FUNCTION__, packet_p, client_id);
                    status = fbe_transport_set_completion_function(packet_p,
                                                                   fbe_logical_error_injection_delay_packet_completion,
                                                                   rec_p);
                    *inject_error = FBE_TRUE;
                    return FBE_STATUS_CONTINUE;
                }
            }
        }
    }

    *inject_error = FBE_FALSE;
    return FBE_STATUS_CONTINUE;
}
/******************************************
 * end fbe_logical_error_injection_delay_io()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_get_object_class_id()
 ****************************************************************
 * @brief
 *   Get the class id of the object
 *
 * @param  object_id - ID of the object to query
 *         class_id - pointer to the class id to retrieve
 *         package_id - ID of the package where the object resides        
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/24/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t logical_error_injection_get_object_class_id (fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_class_id_t             base_object_mgmt_get_class_id;  

    status = fbe_logical_error_injection_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                         &base_object_mgmt_get_class_id,
                                                         sizeof (fbe_base_object_mgmt_get_class_id_t),
                                                         object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         package_id);

    if (status != FBE_STATUS_OK ) 
    {
       *class_id = FBE_CLASS_ID_INVALID;
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *class_id = base_object_mgmt_get_class_id.class_id;
    return status;
}
/******************************************
 * end fbe_logical_error_injection_get_object_class_id()
 ******************************************/


/*************************
 * end file fbe_logical_error_injection_control.c
 *************************/
