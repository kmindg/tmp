/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_object_map_interface.c
 ***************************************************************************
 *
 *  Description
 *      Object table managament interface
 *      
 *
 *  History:
 *      06/27/07    sharel  Created
 *    
 ***************************************************************************/
#include "fbe_api_object_map.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_logical_drive_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe_block_transport.h"
#include "EmcPAL_Memory.h"

#ifdef C4_INTEGRATED
#include "fbe/fbe_physical_package.h"
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */

/**************************************
                Local variables
**************************************/
#if 0 /*FBE3 not needed anymore, using notification interface of FBE API*/
static fbe_notification_element_t notification_element;
#endif

typedef enum object_map_thread_flag_e{
    OBJECT_MAP_THREAD_RUN,
    OBJECT_MAP_THREAD_STOP,
    OBJECT_MAP_THREAD_DONE
}object_map_thread_flag_t;

typedef struct notification_info_s{
    fbe_queue_element_t                     queue_element;
    fbe_notification_type_t              	 mask;
    commmand_response_callback_function_t   callback_func;      
}notification_info_t;

typedef struct fbe2api_state_map_s{
    fbe_lifecycle_state_t           fbe_state;
    fbe_notification_type_t       api_state;
}fbe2api_state_map_t;

#if 0
static fbe2api_state_map_t      state_map [] = 
{
    /*the fist two are removed for now since when we get them we might not have all the information to give the
    registered module about the location of this object since we get them at the first tiem when the object is 
    not stable yet
    {FBE_LIFECYCLE_STATE_SPECIALIZE, FBE_API_STATE_SPECIALIZE},      
    {FBE_LIFECYCLE_STATE_ACTIVATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE},*/             
    {FBE_LIFECYCLE_STATE_READY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY},               
    {FBE_LIFECYCLE_STATE_HIBERNATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE} ,           
    {FBE_LIFECYCLE_STATE_OFFLINE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE},             
    {FBE_LIFECYCLE_STATE_FAIL, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL},                
    {FBE_LIFECYCLE_STATE_DESTROY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY},
    {FBE_LIFECYCLE_STATE_PENDING_READY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY}, 
    {FBE_LIFECYCLE_STATE_PENDING_ACTIVATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE},
    {FBE_LIFECYCLE_STATE_PENDING_HIBERNATE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE} ,
    {FBE_LIFECYCLE_STATE_PENDING_OFFLINE, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE},
    {FBE_LIFECYCLE_STATE_PENDING_FAIL, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL},
    {FBE_LIFECYCLE_STATE_PENDING_DESTROY, FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY},
    {FBE_LIFECYCLE_STATE_INVALID, FBE_NOTIFICATION_TYPE_INVALID} 
};
#endif

static fbe_thread_t                 object_map_thread_handle;
static object_map_thread_flag_t     object_map_thread_flag;
static fbe_semaphore_t              update_semaphore;
static fbe_spinlock_t               object_map_update_queue_lock;
static fbe_queue_head_t             object_map_update_queue_head;
static fbe_queue_head_t             object_map_event_registration_queue;
static fbe_spinlock_t               object_map_event_registration_queue_lock;
static fbe_bool_t                   map_interface_initialized = FALSE;
static fbe_notification_registration_id_t reg_id;
#define FBE_OBJECT_MAP_MAX_SEMAPHORE_COUNT 0x7FFFFFFF 

/*******************************************
                Local functions
********************************************/
static void object_map_interface_thread_func(void * context);
static void object_map_interface_dispatch_queue(void);
static fbe_status_t object_map_validate_index_input (fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t disk_num);
static fbe_status_t object_map_validate_component_index_input (fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t component_num);
static fbe_status_t object_map_interface_update_all(void);
static fbe_status_t object_map_interface_add_object (fbe_object_id_t object_id);
static fbe_status_t object_map_interface_add_board_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state);
static fbe_status_t object_map_interface_add_port_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state);
static fbe_status_t object_map_interface_add_enclosure_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state);
static fbe_status_t object_map_interface_add_disk_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state);
static fbe_status_t object_map_interface_add_logical_drive_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state);
static fbe_status_t object_map_interface_register_notification_element(void);
static fbe_status_t object_map_interface_unregister_notification_element(void);
static void object_map_interface_change_object_state (fbe_object_id_t object_id, fbe_lifecycle_state_t object_state);
static void get_object_location (update_object_msg_t * update_msg);
//static fbe_notification_type_t object_map_interface_translate_fbe_state_to_shim_state (update_object_msg_t * update_request);
static void object_map_interface_send_notification (update_object_msg_t * update_request);
static fbe_status_t object_map_interface_notification_function(update_object_msg_t * update_object_msg, void * context);
static fbe_status_t register_notification_element_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static void empty_event_registration_queue(void);

/*********************************************************************
 *            fbe_api_object_map_interface_init ()
 *********************************************************************
 *
 *  Description:
 *    This function is used for initializing the map and filling it
 *
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/27/07: sharel   created
 *    
 *********************************************************************/

fbe_status_t fbe_api_object_map_interface_init (void)
{
    EMCPAL_STATUS        nt_status;
    fbe_status_t        fbe_status;

    if (map_interface_initialized == FBE_FALSE) {
        /*initialize a queue of update requests and the lock to it*/
        fbe_queue_init(&object_map_update_queue_head);
        fbe_spinlock_init(&object_map_update_queue_lock);
    
        /*init event registration*/
        fbe_queue_init(&object_map_event_registration_queue);
        fbe_spinlock_init(&object_map_event_registration_queue_lock);
    
        /*initialize the semaphore that will control the thread*/
        fbe_semaphore_init(&update_semaphore, 0, 0x7FFFFFFF);
    
        object_map_thread_flag = OBJECT_MAP_THREAD_RUN;
    
        /*start the thread that will execute updates from the queue*/
        nt_status = fbe_thread_init(&object_map_thread_handle, "fbe_obj_map", object_map_interface_thread_func, NULL);
    
        if (nt_status != EMCPAL_STATUS_SUCCESS) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start notification thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        fbe_status  = fbe_api_object_map_init();/*clear the map*/
        if (fbe_status != FBE_STATUS_OK) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        /*register for notification from physical package*/
        fbe_status = object_map_interface_register_notification_element();
        if (fbe_status != FBE_STATUS_OK) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        fbe_status  = object_map_interface_update_all();/*fill the map with objects*/
        if (fbe_status != FBE_STATUS_OK) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s completed - Map is initialized !\n", __FUNCTION__); 
        map_interface_initialized = TRUE;
        return FBE_STATUS_OK;
    }else {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s Map is already initialized, can't do it twice w/o destroy\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

}
/*********************************************************************
 *            fbe_api_object_map_interface_destroy ()
 *********************************************************************
 *
 *  Description:
 *    frees used resources
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/27/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_destroy (void)
{
    if (map_interface_initialized == FBE_TRUE){

        /*flag the theread we should stop*/
        object_map_thread_flag = OBJECT_MAP_THREAD_STOP;
        
        /*change the state of the sempahore so the thread will not block*/
        fbe_semaphore_release(&update_semaphore, 0, 1, FALSE);
        
        /*wait for it to end*/
        fbe_thread_wait(&object_map_thread_handle);
        fbe_thread_destroy(&object_map_thread_handle);
        
        /*destroy all other resources*/
        fbe_semaphore_destroy(&update_semaphore);
        
        fbe_spinlock_destroy(&object_map_update_queue_lock);
        fbe_queue_destroy (&object_map_update_queue_head);

        object_map_interface_unregister_notification_element();
        
        /*check if someone is still registered to get notifications*/
        if (!fbe_queue_is_empty(&object_map_event_registration_queue)){
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Some elements are still registered for events, emptying queueu\n", __FUNCTION__); 
            empty_event_registration_queue();
        }
        
        fbe_queue_destroy(&object_map_event_registration_queue);
        fbe_spinlock_destroy(&object_map_event_registration_queue_lock);

        fbe_api_object_map_destroy();

        map_interface_initialized = FALSE;
        return FBE_STATUS_OK;
    
    }else{
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: map not initialized, can't destroy\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
}

/*********************************************************************
 *            object_map_interface_update_all ()
 *********************************************************************
 *
 *  Description: query the physical package for all the objects in the list
                 and fill the object map based on their location in the tree
 *
 *  Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *    6/27/07: sharel   created
 *   
 *********************************************************************/
static fbe_status_t object_map_interface_update_all(void)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           obj_id_index = 0;   
    fbe_u32_t           total_objects = 0;
    fbe_object_id_t     * object_list = (fbe_object_id_t *)fbe_api_allocate_memory(sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);

    if (object_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't allocate memory for discovery...\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    EmcpalZeroMemory(object_list, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS );
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s: Starting to build object map...\n", __FUNCTION__); 

    status = fbe_api_enumerate_objects (object_list, FBE_MAX_PHYSICAL_OBJECTS, &total_objects, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        fbe_api_free_memory(object_list);
        return status;
    }
    
    /*now that we have all the objects we go over the list and put them in the right location*/
    for (obj_id_index = 0; obj_id_index < total_objects; obj_id_index ++){  
        status  = object_map_interface_add_object (object_list[obj_id_index]);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                           "%s: Can't add object id %X to map, map is I N V A L I D !!!!\n", __FUNCTION__, object_list[obj_id_index]); 
            
            fbe_api_free_memory(object_list);

            return status;
        }
    }

    fbe_api_free_memory(object_list);
    return FBE_STATUS_OK;

}


/*********************************************************************
 *            enumerate_objects_completion ()
 *********************************************************************
 *
 *  Description: completion function for the enumerate packet that was sent
 *
 *  Inputs: completing packet
            completion context with relevant completion information (sepahore to take down)
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/29/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t  enumerate_objects_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}


/*********************************************************************
 *            object_map_interface_add_port_object ()
 *********************************************************************
 *
 *  Description: adds a new port object
 *              
 *
 *  Inputs: object_id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/29/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_add_port_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
{
    fbe_port_number_t               port_number = 0;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_port_role_t					port_role = FBE_PORT_ROLE_INVALID;
    
        status = fbe_api_get_object_port_number (object_id, &port_number);
        if (status != FBE_STATUS_OK) {
            return status;
    }
    else{
        /* Uncommitted ports could have port_number == 0xFFFF */
        /* Even uncommitted ports will be in READY state to handle ESP IOCTLS */
        if (port_number == 0xFFFF){
            return FBE_STATUS_OK;
        }

        status = fbe_api_port_get_port_role(object_id,&port_role);
        if ((status != FBE_STATUS_OK) || (port_role != FBE_PORT_ROLE_BE)){
            /* We are currently supporting only BE ports.*/
            return FBE_STATUS_OK;
        }
        }

        return fbe_api_object_map_add_port_object (object_id, port_number, lifecycle_state);
    
}

/*********************************************************************
 *            object_map_interface_add_enclosure_object ()
 *********************************************************************
 *
 *  Description: adds a new enclosure objects
 *               
 *
 *  Inputs: object_id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/29/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_add_enclosure_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
{
    fbe_port_number_t               port_number = 0;
    fbe_enclosure_number_t          enclosure_number = 0;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

        status = fbe_api_get_object_port_number (object_id, &port_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    
        status = fbe_api_get_object_enclosure_number (object_id, &enclosure_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }

        return fbe_api_object_map_add_enclosure_object (object_id, port_number, enclosure_number, lifecycle_state);

}


/*!****************************************************************************
 * @fn      object_map_interface_add_lcc_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
 *
 * @brief
 *  This function adds a new LCC object.
 *
 * @param object_id
 *
 * @return success of failure
 *
 * HISTORY
 *  04/07/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
static fbe_status_t object_map_interface_add_lcc_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
{
    fbe_port_number_t               port_number = 0;
    fbe_enclosure_number_t          enclosure_number = 0;
    fbe_component_id_t              component_id = 0;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_api_get_object_port_number (object_id, &port_number);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = fbe_api_get_object_enclosure_number (object_id, &enclosure_number);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = fbe_api_get_object_component_id(object_id, &component_id);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return fbe_api_object_map_add_lcc_object (object_id, port_number, enclosure_number, component_id, lifecycle_state);
}


/*********************************************************************
 *            object_map_interface_add_disk_object ()
 *********************************************************************
 *
 *  Description: add a new drive object
 *
 *  Inputs: object_id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/29/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_add_disk_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
{
    fbe_port_number_t               port_number = 0;
    fbe_enclosure_number_t          enclosure_number = 0;
    fbe_enclosure_slot_number_t                 drive_number = 0;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
  
        status  = fbe_api_get_object_port_number (object_id, &port_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        status = fbe_api_get_object_enclosure_number (object_id, &enclosure_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    
        status = fbe_api_get_object_drive_number (object_id, &drive_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }

        return fbe_api_object_map_add_physical_drive_object (object_id, port_number, enclosure_number, drive_number, lifecycle_state);
    

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_object_map_interface_get_port_obj_id ()
 *********************************************************************
 *
 *  Description: API to get the object id of a port, using the port number
 *
 *  Inputs: port number
 *          return pointer for object id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    7/03/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_port_obj_id(fbe_u32_t port_num,fbe_object_id_t *obj_id_ptr)
{
    /*make sure we got reasonable values*/
    if (object_map_validate_index_input(port_num, 0, 0) != FBE_STATUS_OK){
        *obj_id_ptr = FBE_OBJECT_ID_INVALID;
        return  FBE_STATUS_GENERIC_FAILURE;
    } else {
#ifdef C4_INTEGRATED
        if (call_from_fbe_cli_user) {
            fbe_api_fbe_cli_rdevice_output_value_t output_value; 
            fbe_status_t status;
            status = fbe_api_object_map_interface_get_for_fbe_cli( FBE_CLI_MAP_INTERFACE_GET_PORT,
                                                                  port_num, 0,  0, 0, 0, &output_value);
            *obj_id_ptr = output_value.val32_1;
            return status;
        }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
        return  fbe_api_object_map_get_port_obj_id(port_num, obj_id_ptr);
    }

}

/*********************************************************************
 *            fbe_api_object_map_interface_get_enclosure_obj_id ()
 *********************************************************************
 *
 *  Description: API to get the object id of an lcc, using the port + lcc number
 *
 *  Inputs: port number
 *          lcc number
 *          return pointer for object id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    7/03/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_enclosure_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_object_id_t *obj_id_ptr)
{
    /*make sure we got reasonable values*/
    if (object_map_validate_index_input(port_num, encl_pos, 0) != FBE_STATUS_OK){
        return  FBE_STATUS_GENERIC_FAILURE;
    } else {
#ifdef C4_INTEGRATED
        if (call_from_fbe_cli_user) {
            fbe_api_fbe_cli_rdevice_output_value_t output_value; 
            fbe_status_t status;
            status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_ENCL,
                                                                     port_num, encl_pos,  0, 0, 0, &output_value);
            *obj_id_ptr = output_value.val32_1;
            return status;
        }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
        return fbe_api_object_map_get_enclosure_obj_id(port_num, encl_pos, obj_id_ptr);
    }

}


/*********************************************************************
 *            fbe_api_object_map_interface_get_component_obj_id ()
 *********************************************************************
 *
 *  Description: API to get the object id of an lcc, using the port + enclosure + component_id
 *
 *  Inputs: port number
 *          enclosure position
 *          return pointer for object id
 *
 *  Return Value: success or failure
 *
 *  History:
 *     05/08/10: Gerry Fredette   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_component_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t component_num, fbe_object_id_t *obj_id_ptr)
{
    /*make sure we got reasonable values*/
    if (object_map_validate_component_index_input(port_num, encl_pos, component_num) != FBE_STATUS_OK){
        return  FBE_STATUS_GENERIC_FAILURE;
    } else {
        return fbe_api_object_map_get_component_obj_id(port_num, encl_pos, component_num, obj_id_ptr);
    }

}


/*********************************************************************
 *            fbe_api_object_map_interface_get_logical_drive_obj_id ()
 *********************************************************************
 *
 *  Description: API to get the object id of logical drive, using the port + lcc + drive slot number
 *
 *  Inputs: port number
 *          encl position
 *          drive slot
 *          return pointer for object id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    7/03/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_logical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr)
{
    /*make sure we got reasonable values*/
    if (object_map_validate_index_input(port_num, encl_pos, disk_num) != FBE_STATUS_OK){
        return  FBE_STATUS_GENERIC_FAILURE;
    } else {
#ifdef C4_INTEGRATED
        if (call_from_fbe_cli_user) {
            fbe_api_fbe_cli_rdevice_output_value_t output_value; 
            fbe_status_t status;
            status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_LDRV,
                                                                  port_num, encl_pos,  disk_num, 0, 0, &output_value);
            *obj_id_ptr = output_value.val32_1;
            return status;
        }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
        return fbe_api_object_map_get_logical_drive_obj_id(port_num, encl_pos, disk_num, obj_id_ptr);
    }

}


/*********************************************************************
 *            object_map_validate_index_input ()
 *********************************************************************
 *
 *  Description: make sure all indexes are within range
 *
 *  Inputs: port, encl position and disk slot number
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/20/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_validate_index_input (fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t disk_num)
{
    if (port_num >= FBE_API_PHYSICAL_BUS_COUNT ||
        encl_pos >= FBE_API_MAX_ALLOC_ENCL_PER_BUS||
        disk_num >= FBE_API_ENCLOSURE_SLOTS){

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s: input is incorrect: port=%d,encl_pos=%d,disk_num=%d\n",
                       __FUNCTION__, port_num, encl_pos, disk_num); 

        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        return FBE_STATUS_OK;
    }

}


/*********************************************************************
 *            object_map_validate_component_index_input ()
 *********************************************************************
 *
 *  Description: make sure all indexes are within range
 *
 *  Inputs: port, encl position and component number
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    5/20/10: Gerry Fredette   created
 *    
 *********************************************************************/
static fbe_status_t object_map_validate_component_index_input (fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t component_num)
{
	if (port_num >= FBE_API_PHYSICAL_BUS_COUNT ||
		encl_pos >= FBE_API_MAX_ALLOC_ENCL_PER_BUS||
		component_num >= FBE_API_MAX_ENCL_COMPONENTS){

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
                       "%s: input is incorrect: port=%d,encl_pos=%d,component_num=%d\n",
                       __FUNCTION__, port_num, encl_pos, component_num); 

        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        return FBE_STATUS_OK;
    }
}

/*********************************************************************
 *            fbe_api_object_map_interface_object_exists ()
 *********************************************************************
 *
 *  Description: does this object exist in the map
 *
 *  Inputs: object id
 *
 *
 *  Return Value:  ture if object exists
 *
 *  History:
 *    7/12/07: sharel   created
 *    
 *********************************************************************/
fbe_bool_t fbe_api_object_map_interface_object_exists (fbe_object_id_t object_id)
{
    return fbe_api_object_map_object_id_exists (object_id);
}

/*********************************************************************
 *            fbe_api_object_map_interface_refresh_map ()
 *********************************************************************
 *
 *  Description: reload the configuration into the map in case things changed
 *
 *  Inputs: non
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    7/30/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_refresh_map (void)
{
    fbe_status_t        status;

    /*start with clear tables*/
    status = fbe_api_object_map_reset_all_tables();

    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*and reload all the configuration*/
    return object_map_interface_update_all();


}

/*********************************************************************
 *            object_map_interface_register_notification_element ()
 *********************************************************************
 *
 *  Description: register for getting notification on object changes   
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_register_notification_element(void)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
	#if 0 /*FBE3 - not needed anymore, we will register ourselvs with the FBE API notificaiton*/
    fbe_u32_t                           qualifier = 0;
    fbe_packet_t *                      packet = NULL;
    fbe_semaphore_t                     sem;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    
    notification_element.notification_function = object_map_interface_notification_function;
    notification_element.notification_context = NULL;

    /* Allocate packet */
    packet = (fbe_packet_t *) fbe_api_allocate_memory (sizeof (fbe_packet_t));
    
    if(packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&sem, 0, 1);
 
    fbe_transport_initialize_packet(packet);

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    fbe_payload_control_build_operation (control_operation,
                                         FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
                                         &notification_element,
                                         sizeof(fbe_notification_element_t));

    fbe_transport_set_completion_function (packet, register_notification_element_completion,  &sem);

    /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_NOTIFICATION,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID); 

    status = fbe_api_common_send_control_packet_to_driver(packet);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to send packet, status:%d\n", __FUNCTION__, status);

        fbe_transport_destroy_packet(packet);
        fbe_api_free_memory(packet);
        return status;
    }


    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to register for events\n", __FUNCTION__, status); 

        fbe_transport_destroy_packet(packet);
        fbe_api_free_memory(packet);
        return status;
    }
    
    fbe_transport_destroy_packet(packet);
    fbe_api_free_memory(packet);
	#endif /*FBE3*/

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_ALL,
																  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL,
																  FBE_TOPOLOGY_OBJECT_TYPE_ALL,
																  (fbe_api_notification_callback_function_t)object_map_interface_notification_function,
																  NULL,
																  &reg_id);



    if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to register for events: status %d\n", __FUNCTION__, status); 
	}

    return status;
}


/*********************************************************************
 *            object_map_interface_notification_function ()
 *********************************************************************
 *
 *  Description: function that is called every time there is an event in physical package   
 *
 *  Inputs: object_id - the object that had the event
 *          fbe_lifecycle_state_t what is the new state of the object
 *          context - any context we saved when we registered
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    9/10/07:sharel changes to support object state instead of notification type
 *    
 *********************************************************************/
fbe_status_t object_map_interface_notification_function(update_object_msg_t * update_object_msg, void * context)
{
    update_object_msg_t *   update_msg = NULL;

   /*we allocate here and free after the message is used from the queue*/
    update_msg = (update_object_msg_t *) fbe_api_allocate_memory (sizeof (update_object_msg_t));

    if (update_msg == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for map update\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*copy theinformation we have now*/
    update_msg->object_id = update_object_msg->object_id;
    EmcpalCopyMemory(&update_msg->notification_info, &update_object_msg->notification_info, sizeof (fbe_notification_info_t));

    fbe_spinlock_lock (&object_map_update_queue_lock);
    fbe_queue_push(&object_map_update_queue_head, &update_msg->queue_element);
    fbe_spinlock_unlock (&object_map_update_queue_lock);
    fbe_semaphore_release(&update_semaphore, 0, 1, FALSE);
        
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            register_notification_element_completion ()
 *********************************************************************
 *
 *  Description: callback to know we are done with register/unregister    
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t register_notification_element_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}


/*********************************************************************
 *            object_map_interface_unregister_notification_element ()
 *********************************************************************
 *
 *  Description: remove ourselvs from events notification    
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_unregister_notification_element(void)
{
	#if 0 /*FBE3 - we just unregister witht the FBE API itself*/
    fbe_packet_t *                      packet = NULL;  
    fbe_semaphore_t                     sem;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    /* Allocate packet */
    packet = (fbe_packet_t *) fbe_api_allocate_memory (sizeof (fbe_packet_t));
    
    if(packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,"%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_init(&sem, 0, 1);
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    fbe_payload_control_build_operation (control_operation,
                                         FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
                                         &notification_element,
                                         sizeof(fbe_notification_element_t));

    fbe_transport_set_completion_function (packet, register_notification_element_completion,  &sem);

        /* Set packet address */
    fbe_transport_set_address(  packet,
                                FBE_PACKAGE_ID_PHYSICAL,
                                FBE_SERVICE_ID_NOTIFICATION,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID); 

    status = fbe_api_common_send_control_packet_to_driver(packet);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to send packet, status:%d\n", __FUNCTION__, status);
        fbe_transport_destroy_packet(packet);
        fbe_api_free_memory(packet);
        return status;
    }


    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to unregister events, status:%d\n", __FUNCTION__, status); 
        fbe_transport_destroy_packet(packet);
        fbe_api_free_memory(packet);
        return status;
    }
    
    fbe_transport_destroy_packet(packet);
    fbe_api_free_memory(packet);
    #endif /*FBE3*/
	fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

	status = fbe_api_notification_interface_unregister_notification((fbe_api_notification_callback_function_t)object_map_interface_notification_function, reg_id);
	if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to unregister events, status:%d\n", __FUNCTION__, status); 
	}

    return status;
}

/*********************************************************************
 *            object_map_interface_add_object ()
 *********************************************************************
 *
 *  Description: add an object to the map
 *
 *  Inputs: object_id
 * 
 *
 *  Return Value: success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_add_object (fbe_object_id_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;    
    fbe_topology_object_type_t                  object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_lifecycle_state_t                       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /*first get the object type*/
    status = fbe_api_get_object_type (object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK) {
        /*let's retry first, we may have got here becuase we got a notification about a new object which is now is specialized mode
        and it would take it a few more cpu cycles to know it's type*/
        fbe_api_sleep (1);/*sleep 1 msec*/
        status = fbe_api_get_object_type (object_id, &object_type, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to get object type, status = %d\n", __FUNCTION__, status); 
            return status;
        }
    }

    /*let's get the state of teh object ot see if we can add it to the map*/
    status = fbe_api_get_object_lifecycle_state (object_id, &lifecycle_state, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*we insert the object to the map only when it's ready, if it's not, when it would become ready we would add it via notification*/
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY) && 
        (lifecycle_state != FBE_LIFECYCLE_STATE_FAIL) && 
        (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_FAIL)) {
        return FBE_STATUS_OK;
    }

    /*based on the object type we found we send a request to the map to change the state*/
    switch (object_type){
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:     
        status = object_map_interface_add_board_object (object_id, lifecycle_state);            
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_PORT:     
        status = object_map_interface_add_port_object (object_id, lifecycle_state);         
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = object_map_interface_add_enclosure_object (object_id, lifecycle_state);
        break;          
    case FBE_TOPOLOGY_OBJECT_TYPE_LCC:
        status = object_map_interface_add_lcc_object (object_id, lifecycle_state);
        break;          
    case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
        status = object_map_interface_add_disk_object (object_id, lifecycle_state);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE:
        status = object_map_interface_add_logical_drive_object (object_id, lifecycle_state);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: invalid object type received from physical_package, got type:%llu\n", __FUNCTION__, (unsigned long long)object_type); 
        status =  FBE_STATUS_GENERIC_FAILURE;
    }

    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Error in adding object type:%llu, obj_id:%d\n",
                        __FUNCTION__, (unsigned long long)object_type, object_id );
    }
    return status;
    
}

/*********************************************************************
 *            object_map_interface_thread_func ()
 *********************************************************************
 *
 *  Description: the thread that will dispatch the state changes from the queue
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static void object_map_interface_thread_func(void * context)
{
    EMCPAL_STATUS           nt_status;

    FBE_UNREFERENCED_PARAMETER(context);
    
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 

    while(1)    
    {
        /*block until someone takes down the semaphore*/
        nt_status = fbe_semaphore_wait(&update_semaphore, NULL);
        if(object_map_thread_flag == OBJECT_MAP_THREAD_RUN) {
            /*check if there are any notifictions waiting on the queue*/
            object_map_interface_dispatch_queue();
        } else {
            break;
        }
    }

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 

    object_map_thread_flag = OBJECT_MAP_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


/*********************************************************************
 *            object_map_interface_dispatch_queue ()
 *********************************************************************
 *
 *  Description: deque objects and call the function to change their state *
 *
 *  Return Value: none
 *
 *  History:
 *    8/07/07: sharel   created
 *    
 *********************************************************************/
static void object_map_interface_dispatch_queue(void)
{
    update_object_msg_t         * update_request = NULL;
	fbe_status_t				status;
	fbe_lifecycle_state_t		state;
    
    while (1) {
         fbe_spinlock_lock(&object_map_update_queue_lock);
        /*wo we have something on the queue ?*/
        if (!fbe_queue_is_empty(&object_map_update_queue_head)) {
            /*get the next item from the queue*/
            update_request = (update_object_msg_t *) fbe_queue_pop(&object_map_update_queue_head);
            fbe_spinlock_unlock(&object_map_update_queue_lock); 

			status = fbe_notification_convert_notification_type_to_state(update_request->notification_info.notification_type, &state);
			if (status != FBE_STATUS_OK) {
				 fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
					   "%s:notification,obj:0x%X can't map state:0x%llX\n",
						__FUNCTION__, update_request->object_id, (unsigned long long)update_request->notification_info.notification_type); 

			}else if (update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE) {
			
				fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW,
							   "%s:notification,obj:0x%X type:0x%llX\n",
								__FUNCTION__, update_request->object_id, (unsigned long long)update_request->notification_info.notification_type); 
			}

            if (update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE) {

                /*before we do anything with the object we want to remember where was it
                 located because if we removed it, we may need to know where it was so flare can be updated*/
                if (state == FBE_LIFECYCLE_STATE_DESTROY ||
                    state == FBE_LIFECYCLE_STATE_PENDING_DESTROY) {
                    object_map_get_object_type(update_request->object_id, &update_request->object_type);
                    get_object_location (update_request);
                }
    
                /*and call the function to change this object's state based on what the notification was*/
                object_map_interface_change_object_state (update_request->object_id, state);
    
                /*for new objects we check their location after they were added or existed there*/
                if (state != FBE_LIFECYCLE_STATE_DESTROY &&
                    state != FBE_LIFECYCLE_STATE_PENDING_DESTROY) {
                    object_map_get_object_type(update_request->object_id, &update_request->object_type);
                    get_object_location (update_request);
                }
            }else if ((update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_END_OF_LIFE) ||
                      (update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_RECOVERY) ||
                      (update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO) || 
                     (update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_CALLHOME)){

                /*technically, it should always be a logical drive but we populate it because the upper DH
                checks each message to make sure it was received from the logical drive only*/
                object_map_get_object_type(update_request->object_id, &update_request->object_type);
                get_object_location (update_request);
            }

            /*time to send notifications to all the Flare modules if the state is one they registered for
            but we do it only if the object is discovered. we can have a case of a drive for example that sends us a end_of_life
            message while it's initializing. In this case we don't have it in the map and we can't send notificaiton about it to Flare*/
            if (fbe_api_object_map_object_discovered (update_request->object_id)) {
                object_map_get_object_type(update_request->object_id, &update_request->object_type); 
                 if ((update_request->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE || 
                     (update_request->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE)) && 
                     (update_request->encl_addr ==  FBE_ENCLOSURE_NUMBER_INVALID)) { 
                         // do nothing  
                         /*fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                                        "%s:obj:0x%X is in state:%s, but FBE_ENCLOSURE_NUMBER_INVALID !!!\n",
                                        __FUNCTION__, update_request->object_id, fbe_api_state_to_string(update_request->notification_info.notification_data.lifecycle_state)); */
                 } else { 
                     object_map_interface_send_notification (update_request);
                 }
            }
            
            /*and delete it, we don't need it anymore*/
            fbe_api_free_memory (update_request);

        } else {
            fbe_spinlock_unlock(&object_map_update_queue_lock); 
            break;
        }
    }
       
}

/*********************************************************************
 *            object_map_interface_change_object_state ()
 *********************************************************************
 *
 *  Description: change an object state (i.e. create a new one or removing 
 *               an existing one, using different functions, based on the notifiaction type)
 *
 *  Inputs: object_id
 *          notification_type - what happened
 *
 *
 *  History:
 *    8/08/07: sharel   created
 *    9/10/07:sharel changes to support object state instead of notification type
 *    
 *********************************************************************/
static void object_map_interface_change_object_state (fbe_object_id_t object_id, fbe_lifecycle_state_t object_state)
{
    fbe_bool_t                          object_discovered = FBE_FALSE;

    object_discovered = fbe_api_object_map_object_discovered (object_id);
    if (!object_discovered && (object_state == FBE_LIFECYCLE_STATE_READY || object_state == FBE_LIFECYCLE_STATE_PENDING_FAIL)) {
        /*if we get a notification for that, it means we have a new object in the system and we need to add it to the map
        during the addition, we would also set the object state as part of discovery.*/
        object_map_interface_add_object (object_id);
        return;
    }else if (!object_discovered) {
        /*if we don't have the location of the object yet (port, encl, etc.) we can't update the map
        we would do that only when the object becomes ready and can give us all the information*/
        
		return;
    }

    
    /*we can finally set the new state of the object*/
    object_map_set_object_state (object_id, object_state);
}

/*********************************************************************
 *            fbe_api_object_map_interface_register_notification ()
 *********************************************************************
 *
 *  Description: register a function that will alert flare component of state changes
 *               When using this function the user MUST use the fbe_notification_type_t and not the
 *               fbe version because the fbe version is not a bit mask
 *
 *  Inputs: state_mask - the states we want notification for
 *          callback_func - the function to call
 *
 *
 *  History:
 *    8/08/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_register_notification(fbe_notification_type_t state_mask, commmand_response_callback_function_t callback_func)
{
    notification_info_t *       notification_info = (notification_info_t *)fbe_api_allocate_memory (sizeof (notification_info_t));

    if (notification_info == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:unable to get memory to register even callback\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (callback_func == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:callback function is NULL\n", __FUNCTION__); 
        fbe_api_free_memory (notification_info);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*stick this info in out queue for later use*/
    notification_info->mask = state_mask;
    notification_info->callback_func = callback_func;

    fbe_spinlock_lock(&object_map_event_registration_queue_lock);
    fbe_queue_push (&object_map_event_registration_queue, &notification_info->queue_element);
    fbe_spinlock_unlock(&object_map_event_registration_queue_lock);

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:callback_func:%p, mask:%d\n", __FUNCTION__, (void *)callback_func, (int)state_mask);
    return FBE_STATUS_OK;
}

/*********************************************************************
 *            object_map_interface_add_board_object ()
 *********************************************************************
 *
 *  Description: add the root board object
 *
 *  Inputs: object_id           
 *
 *  Return Value: success or failure
 *
 *  History:
 *    8/21/07: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_add_board_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
{
    return fbe_api_object_map_add_board_object (object_id, lifecycle_state);
}


/*********************************************************************
 *            get_object_location ()
 *********************************************************************
 *
 *  Description: return the physical location of the object in the map
 *               
 *
 *  Inputs: update_msg -  the data structure to fill
 *
 *  Return Value: none
 *
 *  History:
 *   9/15/07: sharel    created 
 *    
 *********************************************************************/
static void get_object_location (update_object_msg_t * update_msg)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t               temp_index = 0;

    status = fbe_api_object_map_get_object_port_index (update_msg->object_id, &temp_index);

    if (status != FBE_STATUS_OK) {
        update_msg->port = -1;
    } else {
        update_msg->port = temp_index;
    }

    status = fbe_api_object_map_get_object_encl_index (update_msg->object_id, &temp_index);

    if (status != FBE_STATUS_OK) {
        update_msg->encl = -1;
    } else {
        update_msg->encl = temp_index;
    }

    status =  fbe_api_object_map_get_object_encl_address (update_msg->object_id, &temp_index);

    if (status != FBE_STATUS_OK) {
        update_msg->encl_addr = -1;
    } else {
        update_msg->encl_addr = temp_index;
    }

    status =  fbe_api_object_map_get_object_drive_index (update_msg->object_id, &temp_index);

    if (status != FBE_STATUS_OK) {
        update_msg->drive = -1;
    } else {
        update_msg->drive = temp_index;
    }
}
/*********************************************************************
 *            object_map_interface_add_logical_drive_object ()
 *********************************************************************
 *
 *  Description: add a new drive object
 *
 *  Inputs: object_id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    5/22/08: sharel   created
 *    
 *********************************************************************/
static fbe_status_t object_map_interface_add_logical_drive_object (fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
{
    fbe_port_number_t               port_number = 0;
    fbe_enclosure_number_t          enclosure_number = 0;
    fbe_enclosure_slot_number_t                 drive_number = 0;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_get_block_edge_info_t   get_edge_info;

        status =  fbe_api_get_object_port_number (object_id, &port_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }

        status = fbe_api_get_object_enclosure_number (object_id, &enclosure_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    
        status = fbe_api_get_object_drive_number (object_id, &drive_number);
        if (status != FBE_STATUS_OK) {
            return status;
        }

    status = fbe_api_object_map_add_logical_drive_object (object_id, port_number, enclosure_number, drive_number, lifecycle_state);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*for logical drives we check if the drive recommanded proactive sparing while the client did not even come up
    in this case, we want to set the flag and it's up to the client to check it and do what it usualy does*/
    status = fbe_api_get_block_edge_info(object_id, 0, &get_edge_info, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    if (get_edge_info.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE) {
        status = fbe_api_object_map_set_proactive_sparing_recommanded(object_id, FBE_TRUE);
        if (status != FBE_STATUS_OK) {
            return status;
        }

    }

    return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_api_object_map_interface_get_physical_drive_obj_id ()
 *********************************************************************
 *
 *  Description: API to get the object id of the physical drive, using the port + lcc + drive slot number
 *
 *  Inputs: port number
 *          lcc number
 *          drive slot
 *          return pointer for object id
 *
 *  Return Value: success or failure
 *
 *  History:
 *    5/22/08: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_physical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr)
{
    /*make sure we got reasonable values*/
    if (object_map_validate_index_input(port_num, encl_pos, disk_num) != FBE_STATUS_OK){
        return  FBE_STATUS_GENERIC_FAILURE;
    } else {
#ifdef C4_INTEGRATED
        if (call_from_fbe_cli_user) {
            fbe_api_fbe_cli_rdevice_output_value_t output_value; 
            fbe_status_t status;
            status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_PDRV,
                                                                  port_num, encl_pos,  disk_num, 0, 0, &output_value);
            *obj_id_ptr = output_value.val32_1;
            return status;
        }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
        return fbe_api_object_map_get_physical_drive_obj_id(port_num, encl_pos, disk_num, obj_id_ptr);
    }

}

/*********************************************************************
 *            fbe_api_object_map_interface_get_object_lifecycle_state ()
 *********************************************************************
 *
 *  Description: get the lifecycle state of the object
 *
 *  Inputs: object id and return buffer
 *
 *  Return Value: success or failure
 *
 *  History:
 *    7/18/08: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_object_lifecycle_state (fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state)
{
#ifdef C4_INTEGRATED
    if (call_from_fbe_cli_user) {
        fbe_api_fbe_cli_rdevice_output_value_t output_value; 
        fbe_status_t status;
        status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_OBJL,
                0, 0, 0, 0, 0, &output_value);
        *lifecycle_state = output_value.val32_1;
        return status;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
    return object_map_get_object_lifecycle_state (object_id, lifecycle_state);
}


/*********************************************************************
 *            fbe_api_object_map_interface_get_board_obj_id ()
 *********************************************************************
 *
 *  Description: do we have any queued IOs for this FRU ?
 *
 *  Inputs: object id, return values
 *
 *  Return Value: success or failure
 *
 *  History:
 *    9/2/08: sharel    created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_board_obj_id(fbe_object_id_t *obj_id_ptr)
{
    if (obj_id_ptr == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
#ifdef C4_INTEGRATED
    if (call_from_fbe_cli_user) {
        fbe_api_fbe_cli_rdevice_output_value_t output_value; 
        fbe_status_t status;
        status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_BORD,
                0, 0, 0, 0, 0, &output_value);
        *obj_id_ptr = output_value.val32_1;
        return status;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
    return fbe_api_object_map_get_board_obj_id (obj_id_ptr);


}

/*********************************************************************
 *            fbe_api_object_map_interface_get_object_type ()
 *********************************************************************
 *
 *  Description: get the type of the object in the map
 *
 *
 *  Return Value: success or failure
 *
 *  History:
 *    9/15/08: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_object_type(fbe_object_id_t object_id, fbe_topology_object_type_t *object_type)
{
#ifdef C4_INTEGRATED
    if (call_from_fbe_cli_user) {
        fbe_api_fbe_cli_rdevice_output_value_t output_value; 
        fbe_status_t status;
        status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_TYPE,
                0, 0, 0, object_id, 0, &output_value);
        *object_type = output_value.val64;
        return status;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
    return object_map_get_object_type(object_id, object_type);

}

/*********************************************************************
 *            object_map_interface_translate_fbe_state_to_shim_state ()
 *********************************************************************
 *
 *  Description: map between the fbe state and the shim state.
 *  this is needed because FBE state (fbe_lifecycle_state_t)is serving as index
 *  and we can't do a bit map with it
 *
 *
 *  Return Value: equivalent fbe_notification_type_t
 *
 *  History:
 *    10/03/08: sharel  created
 *    
 *********************************************************************/
#if 0/*depracated*/
static fbe_notification_type_t object_map_interface_translate_fbe_state_to_shim_state (update_object_msg_t * update_request)
{
    fbe2api_state_map_t *       map  = state_map;

    /*if we have to translate a lifecycle state we pick the lifecycle state path*/
    if (update_request->notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE) {
        while (map->fbe_state != FBE_LIFECYCLE_STATE_INVALID) {
            if (map->fbe_state == update_request->notification_info.notification_data.lifecycle_state) {
                return map->api_state;
            }
    
            map++;
        }
    
    }else {
        /*if it's not a lifecycle state, it might be one of the other special cases*/
        switch (update_request->notification_info.notification_type) {
        case FBE_NOTIFICATION_TYPE_END_OF_LIFE:
            return FBE_NOTIFICATION_TYPE_END_OF_LIFE;
            break;
        case FBE_NOTIFICATION_TYPE_RECOVERY:
            return FBE_NOTIFICATION_TYPE_RECOVERY;
            break;
        case FBE_NOTIFICATION_TYPE_CALLHOME:
            return FBE_NOTIFICATION_TYPE_CALLHOME;
        case FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO:
            return FBE_API_STATE_OBJECT_IO_TIMER_CHECK_4_ACTIVATE;
            break;
        default:
            return FBE_NOTIFICATION_TYPE_INVALID;
            break;
        }


    }
    return FBE_NOTIFICATION_TYPE_INVALID;
}
#endif
/*********************************************************************
 *            object_map_interface_send_notification ()
 *********************************************************************
 *
 *  Description: send notification to registered Flare modules
 *
 *  Return Value: None
 *
 *  History:
 *    10/03/08: sharel  created
 *    
 *********************************************************************/
static void object_map_interface_send_notification (update_object_msg_t * update_request)
{
    notification_info_t *       notification_info = NULL;

    fbe_spinlock_lock(&object_map_event_registration_queue_lock);
    notification_info = (notification_info_t *)fbe_queue_front(&object_map_event_registration_queue);
    fbe_spinlock_unlock(&object_map_event_registration_queue_lock);
    while (notification_info != NULL) {
        /*check if we have a state match and if we support this state notification*/
        if(update_request->notification_info.notification_type & notification_info->mask){
            notification_info->callback_func ((void *)update_request);
        }

        /*and go to the next one*/
        fbe_spinlock_lock(&object_map_event_registration_queue_lock);
        notification_info = (notification_info_t *)fbe_queue_next(&object_map_event_registration_queue, &notification_info->queue_element);
        fbe_spinlock_unlock(&object_map_event_registration_queue_lock);
    }
     
}

/*********************************************************************
 *            fbe_api_object_map_interface_get_total_logical_drives ()
 *********************************************************************
 *
 *  Description: return total number of drives on a port
 *
 *  Inputs: port_number - backend port number
 *          total_drives  - total drives on this port
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/24/08: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_total_logical_drives(fbe_u32_t port_number, fbe_u32_t *total_drives)
{
    if (total_drives == NULL || object_map_validate_index_input(port_number, 0, 0) != FBE_STATUS_OK){
        return  FBE_STATUS_GENERIC_FAILURE;
    }

#ifdef C4_INTEGRATED
    if (call_from_fbe_cli_user) {
        fbe_api_fbe_cli_rdevice_output_value_t output_value; 
        fbe_status_t status;
        status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_NUML,
                port_number, 0, 0, 0, 0, &output_value);
        *total_drives = output_value.val32_1;
        return status;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */

    return fbe_api_object_map_get_total_logical_drives (port_number, total_drives);

}

/*********************************************************************
 *            fbe_api_object_map_interface_get_object_handle ()
 *********************************************************************
 *
 *  Description: return the address of the object_map_base_object_t area of
 *               this object id, but as a handle to the outside world
 *
 *  Inputs: object_id
 *
 *  Return Value: handle
 *
 *  History:
 *    11/26/08: sharel  created
 *    
 *********************************************************************/
fbe_api_object_map_handle fbe_api_object_map_interface_get_object_handle (fbe_object_id_t object_id)
{
    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return 0;
    }

    return fbe_api_object_map_get_object_handle(object_id);
}


/*********************************************************************
 *            fbe_api_object_map_interface_get_logical_drive_object_handle ()
 *********************************************************************
 *
 *  Description: return the logical drive handle using the port/encl/slot arguments
 *
 *  Inputs: port/encl position/slot of the drive we want
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/26/08: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_logical_drive_object_handle (fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t disk_num, fbe_api_object_map_handle *handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = object_map_validate_index_input (port_num, encl_pos, disk_num);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return fbe_api_object_map_get_logical_drive_object_handle(port_num, encl_pos, disk_num, handle);
}

/*********************************************************************
 *            fbe_api_object_map_interface_get_obj_info_by_handle ()
 *********************************************************************
 *
 *  Description: return some object information by handle to prevent multiple locking
 *
 *  Inputs: hanldle
 *
 *  Return Value: success or failue
 *
 *  History:
 *    12/01/08: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_get_obj_info_by_handle (fbe_api_object_map_handle object_handle, fbe_api_object_map_obj_info_t *object_info)
{
    if (object_handle == 0 || object_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return fbe_api_object_map_get_obj_info_by_handle (object_handle, object_info);

}


/*********************************************************************
 *            fbe_api_object_map_interface_unregister_notification ()
 *********************************************************************
 *
 *  Description: unregister a previously registered function 
 *
 *  Inputs: callback_func - what we want to unregister
 *
 *  History:
 *    1/02/09: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_unregister_notification(commmand_response_callback_function_t callback_func)
{
    notification_info_t *       notification_info = NULL;
    notification_info_t *       next_notification_info = NULL;

    /*we need to walk the queue and look for the registered function and pop it out*/

    fbe_spinlock_lock(&object_map_event_registration_queue_lock);
    next_notification_info = (notification_info_t *)fbe_queue_front(&object_map_event_registration_queue);
    while (next_notification_info != NULL) {
        notification_info = next_notification_info;
        next_notification_info = (notification_info_t *)fbe_queue_next(&object_map_event_registration_queue, &next_notification_info->queue_element);   
        if (notification_info->callback_func == callback_func){
            fbe_queue_pop((fbe_queue_element_t *)notification_info);    
            fbe_api_free_memory (notification_info);
            fbe_spinlock_unlock(&object_map_event_registration_queue_lock);     
            return FBE_STATUS_OK;
        }
        
    }

    /*if we got here we did not find the function to unregister*/
    fbe_spinlock_unlock(&object_map_event_registration_queue_lock);     
    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: did not find function %p to unregister\n", __FUNCTION__,  (void *)callback_func); 
    return FBE_STATUS_GENERIC_FAILURE;  

}

/*********************************************************************
 *            empty_event_registration_queue ()
 *********************************************************************
 *
 *  Description: unregister any previously registered function that forgot to unregister itself
 *
 *
 *  History:
 *    1/02/09: sharel   created
 *    
 *********************************************************************/
static void empty_event_registration_queue(void)
{
    notification_info_t *       notification_info = NULL;
    
    /*we need to walk the queue and look for the registered function and pop it out*/
    fbe_spinlock_lock(&object_map_event_registration_queue_lock);
    notification_info = (notification_info_t *)fbe_queue_front(&object_map_event_registration_queue);
    if (notification_info != NULL) {
        notification_info = (notification_info_t *)fbe_queue_pop(&object_map_event_registration_queue); 
        fbe_api_free_memory (notification_info);        
    }

    /*if we got here we did not find the function to unregister*/
    fbe_spinlock_unlock(&object_map_event_registration_queue_lock);     
    return; 
}

/*********************************************************************
 *            fbe_api_object_map_interface_set_proactive_sparing_recommanded ()
 *********************************************************************
 *
 *  Description: sets the proactive sparing flag in the object.
 *               this is usually done at boot time so if the physical package
 *               client missed the notification, it would still know this drive is dying
 *
 *  Inputs: object_id - object to update
 *  Inputs: recommanded - set to TRUE if sparing is recommended, otherwise sets sparing as not recommended
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/02/09: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_set_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t recommanded)
{
    if (object_id >= FBE_OBJECT_ID_INVALID) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return fbe_api_object_map_set_proactive_sparing_recommanded(object_id, recommanded);

}

/*********************************************************************
 *            fbe_api_object_map_interface_is_proactive_sparing_recommanded ()
 *********************************************************************
 *
 *  Description: tells the user if this drive recommanded proactive sparing
 *               this is ususally used in boot time to check if we missed the notification
 *               fro proactive sparing
 *
 *  Inputs: object_id - object to check 
 *  Outputs: recommanded - TRUE if sparing is recommended, otherwise FALSE
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/02/09: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_interface_is_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t *recommanded)
{
    if ((recommanded == NULL) || (object_id >= FBE_OBJECT_ID_INVALID)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return fbe_api_object_map_is_proactive_sparing_recommanded(object_id, recommanded);
}

/*********************************************************************
 *         fbe_api_object_map_interface_set_encl_addr()
 *********************************************************************
 *
 *  Description: Sets enclosure address for specified enclosure.
 *
 *  Inputs: 
 *    port      - Port number.
 *    encl_pos  - Enclosure position.
 *    encl_addr - Enclosure address.
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/12/09: Prahlad Purohit -- Created
 *
 *********************************************************************/

fbe_status_t 
fbe_api_object_map_interface_set_encl_addr(fbe_u32_t port, 
                                           fbe_u32_t encl_pos, 
                                           fbe_u32_t encl_addr)
{
    fbe_status_t status;

    status = object_map_validate_index_input (port, encl_pos, 0);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    return fbe_api_object_map_set_encl_addr(port, encl_pos, encl_addr);
}


/*********************************************************************
 *         fbe_api_object_map_interface_get_encl_addr()
 *********************************************************************
 *
 *  Description: Gets enclosure address for specified enclosure.
 *
 *  Inputs: 
 *    port       - Port number.
 *    encl_pos   - Enclosure position.
 *    *encl_addr - Pointer to return enclosure address.
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/12/09: Prahlad Purohit -- Created
 *
 *********************************************************************/

fbe_status_t 
fbe_api_object_map_interface_get_encl_addr(fbe_u32_t port, 
                                           fbe_u32_t encl_pos,
                                           fbe_u32_t *encl_addr)
{
    fbe_status_t status;

    status = object_map_validate_index_input (port, encl_pos, 0);
    if (status != FBE_STATUS_OK) {
        return status;
    }

#ifdef C4_INTEGRATED
    if (call_from_fbe_cli_user) {
        fbe_api_fbe_cli_rdevice_output_value_t output_value; 
        fbe_status_t status;
        status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_EADD,
                port, encl_pos, 0, 0, 0, &output_value);
        *encl_addr = output_value.val32_1;
        return status;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */

    return fbe_api_object_map_get_encl_addr(port, encl_pos, encl_addr);
}


/*********************************************************************
 *           fbe_api_object_map_interface_set_encl_physical_pos()
 *********************************************************************
 *
 *  Description: Sets physical position attributes for specified
 *    enclosure address.
 *
 *  Inputs: 
 *    port       - Port number.
 *    encl_pos   - Enclosure position.
 *    encl_addr  - Enclosure address.
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/11/09: Prahlad Purohit -- Created
 *
 *********************************************************************/
fbe_status_t 
fbe_api_object_map_interface_set_encl_physical_pos(fbe_u32_t port, 
                                                   fbe_u32_t encl_pos, 
                                                   fbe_u32_t encl_addr)
{
    /* Make sure the arguments supplied are correct.
     */
    if(encl_addr < FBE_API_ENCLOSURE_COUNT) {
        return fbe_api_object_map_set_encl_physical_pos(port, encl_pos, encl_addr);
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }

}


/*********************************************************************
 *           fbe_api_object_map_interface_get_encl_physical_pos()
 *********************************************************************
 *
 *  Description: Gets physical position attributes for specified
 *    enclosure address.
 *
 *  Inputs: 
 *    port      - Pointer to return port number.
 *    encl_pos  - Pointer to return enclosure position.
 *    encl_addr - Enclosure address.
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/11/09: Prahlad Purohit -- Created
 *
 *********************************************************************/
fbe_status_t 
fbe_api_object_map_interface_get_encl_physical_pos(fbe_u32_t *port, 
                                                   fbe_u32_t *encl_pos, 
                                                   fbe_u32_t encl_addr)
{
    /* Make sure the arguments supplied are correct.
     */
    if(encl_addr < FBE_API_ENCLOSURE_COUNT) {

#ifdef C4_INTEGRATED
    if (call_from_fbe_cli_user) {
        fbe_api_fbe_cli_rdevice_output_value_t output_value; 
        fbe_status_t status;
        status = fbe_api_object_map_interface_get_for_fbe_cli (FBE_CLI_MAP_INTERFACE_GET_EPOS,
                0, 0, 0, 0, encl_addr, &output_value);
        *port = output_value.val32_1;
        *encl_pos = output_value.val32_2;
        return status;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
        return fbe_api_object_map_get_encl_physical_pos(port, encl_pos, encl_addr);
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

#ifdef C4_INTEGRATED
fbe_status_t fbe_api_object_map_interface_get_for_fbe_cli (fbe_u32_t fbe_cli_map_interface_code, 
                                                           fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num,fbe_object_id_t object_id, fbe_u32_t encl_addr,
                                                           fbe_api_fbe_cli_rdevice_output_value_t *requested_value)
{
    csx_status_e                                  csx_status;
    csx_size_t                                    bytesReturned = 0;
    fbe_api_fbe_cli_rdevice_inbuff_t              fbe_cli_rdevice_in_data;
    fbe_api_fbe_cli_obj_map_if_rdevice_outbuff_t  fbe_cli_rdevice_out_data;


    fbe_cli_rdevice_in_data.control_code               = 0;
    fbe_cli_rdevice_in_data.fbe_cli_map_interface_code = fbe_cli_map_interface_code;
    fbe_cli_rdevice_in_data.port_num                   = port_num;
    fbe_cli_rdevice_in_data.encl_num                   = encl_num;
    fbe_cli_rdevice_in_data.disk_num                   = disk_num;
    fbe_cli_rdevice_in_data.object_id                  = object_id;
    fbe_cli_rdevice_in_data.encl_addr                  = encl_addr;
    fbe_cli_rdevice_in_data.attr                       = 0;
    fbe_cli_rdevice_in_data.buffer_length              = sizeof(fbe_api_fbe_cli_obj_map_if_rdevice_outbuff_t);
    fbe_cli_rdevice_out_data.fbe_status_rc             = FBE_STATUS_GENERIC_FAILURE;
    fbe_cli_rdevice_out_data.requested_value.val64     = (csx_u64_t)FBE_OBJECT_ID_INVALID;

    csx_status = csx_p_rdevice_ioctl(ppDeviceRef,
                                     FBE_CLI_GET_MAP_IOCTL,
                                     &fbe_cli_rdevice_in_data,
                                     sizeof(fbe_api_fbe_cli_rdevice_inbuff_t),
                                     &fbe_cli_rdevice_out_data,
                                     sizeof(fbe_api_fbe_cli_obj_map_if_rdevice_outbuff_t),
                                     &bytesReturned);

      if (CSX_FAILURE(csx_status) || (fbe_cli_rdevice_out_data.fbe_status_rc != FBE_STATUS_OK)) 
      {
            csx_p_display_std_note("%s: failed rdevice ioctl to PPFDPORT0 (map api) status = 0x%x (%s) ; fbe_status_rc = 0x%x\n",
                                   __FUNCTION__, csx_status,csx_p_cvt_csx_status_to_string(csx_status), 
                                   fbe_cli_rdevice_out_data.fbe_status_rc);
            return (CSX_FAILURE(csx_status) ? FBE_STATUS_GENERIC_FAILURE : fbe_cli_rdevice_out_data.fbe_status_rc);
      }
      else
      {
          if(0) csx_p_display_std_note("%s: rdevice ioctl to PPFDPORT0 (map api) succeeded...\n",__FUNCTION__);
          requested_value->val64 = fbe_cli_rdevice_out_data.requested_value.val64;
          return FBE_STATUS_OK;
      }
      return FBE_STATUS_OK;
}

#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity - C4BUG - IOCTL model mixing */
