/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_object_map.c
 ***************************************************************************
 *
 *  Description
 *      Object table managament
 *      
 *
 *  History:
 *      06/19/07    sharel  Created
 *    
 ***************************************************************************/
#include "fbe_api_object_map.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_api_common.h"

/**************************************
                Local Structures
**************************************/



/**************************************
                Local variables
**************************************/

/*This is the root of the table*/
static board_object_map_t       fbe_api_sp_board;

/*Lock for table updates*/
static fbe_spinlock_t       map_update_lock;
static fbe_bool_t           map_initialized = FBE_FALSE;

/* Enclosure address to physical position translation table & associated lock. */
static encl_physical_position_t encl_addr_to_physical_pos[FBE_API_ENCLOSURE_COUNT];

/*******************************************
                Local functions
********************************************/
static object_map_base_object_t * object_map_get_object_ptr (fbe_object_id_t object_id);

/*********************************************************************
 *            fbe_api_object_map_init ()
 *********************************************************************
 *
 *  Description:
 *    This function is used for initializing the data structures
 *    of the table and other internal parameters
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/20/07: sharel   created
 *    
 *********************************************************************/

fbe_status_t fbe_api_object_map_init (void)
{
    
    /*initialize the map lock*/
    fbe_spinlock_init(&map_update_lock);
  
    /*mark all objects as invalid*/
    fbe_api_object_map_reset_all_tables();

    map_initialized = TRUE;

    fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                   "fbe_api_object_map_init done, arrays sizes:bus=%d,lcc=%d,drive=%d\n",FBE_API_PHYSICAL_BUS_COUNT, FBE_API_ENCLOSURES_PER_BUS, FBE_API_ENCLOSURE_SLOTS); 

    return FBE_STATUS_OK;

}
/*********************************************************************
 *            fbe_api_object_map_destroy ()
 *********************************************************************
 *
 *  Description:
 *    frees the table's used resources
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/20/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_destroy (void)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*destroy the table lock*/
    fbe_spinlock_destroy(&map_update_lock);

    map_initialized = FBE_FALSE;

    return FBE_STATUS_OK;
}


/*********************************************************************
 *            fbe_api_object_map_get_port_obj_id ()
 *********************************************************************
 *
 *  Description: Get the object id of the specific port object
 *
 *  Inputs: port number and a pointer to the object id
 *
 *  Return Value:  success or failure
 *  History:
 *    6/20/07: sharel   created
 *    
 *********************************************************************/

fbe_status_t fbe_api_object_map_get_port_obj_id(fbe_u32_t port_num,fbe_object_id_t *obj_id_ptr)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_spinlock_lock (&map_update_lock);
    *obj_id_ptr = fbe_api_sp_board.port_object_list[port_num].port_object.object_id;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
    
}

/*********************************************************************
 *            fbe_api_object_map_get_board_obj_id ()
 *********************************************************************
 *
 *  Description: Get the object id of the root board object
 *
 *  Inputs: port number and a pointer to the object id
 *
 *  Return Value:  success or failure
 *  History:
 *    8/21/07: sharel   created
 *    
 *********************************************************************/

fbe_status_t fbe_api_object_map_get_board_obj_id(fbe_object_id_t *obj_id_ptr)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_spinlock_lock (&map_update_lock);
    *obj_id_ptr = fbe_api_sp_board.board_object.object_id;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
    
}


/*********************************************************************
 *            fbe_api_object_map_get_enclosure_obj_id ()
 *********************************************************************
 *
 *  Description: Get the object id of the specific enclosure on a 
 *               specific port object
 *
 *  Inputs: port and enclosure position and a pointer to the object id
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/20/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_get_enclosure_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_object_id_t *obj_id_ptr)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_spinlock_lock (&map_update_lock);
    *obj_id_ptr = fbe_api_sp_board.port_object_list[port_num].enclosure_object_list[encl_pos].enclosure_object.object_id;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
    
}

/*********************************************************************
 *            fbe_api_object_map_get_component_obj_id ()
 *********************************************************************
 *
 * Description:  Get the object id of the specific component inside an
 * enclosure on a specific port object.
 *
 *  Inputs: port, enclosure position and component number and a pointer to the object id
 *
 *  Return Value:  success or failure
 *
 *  History:
 *     05/08/10: Gerry Fredette   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_get_component_obj_id(fbe_u32_t port_num, fbe_u32_t encl_pos, fbe_u32_t component_num, fbe_object_id_t *obj_id_ptr)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_spinlock_lock (&map_update_lock);
    *obj_id_ptr = fbe_api_sp_board.port_object_list[port_num].enclosure_object_list[encl_pos].component_object_list[component_num].component_object.object_id;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;

}

/*********************************************************************
 *            fbe_api_object_map_get_logical_drive_obj_id ()
 *********************************************************************
 *
 *  Description: Get the object id of the specific logical disk connected to
                 a specific lcc on a pecific port object
 *
 *  Inputs: port, lcc and disk slot number and a pointer to the object id
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    6/20/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_get_logical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t lcc_num, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_spinlock_lock (&map_update_lock);
    *obj_id_ptr = fbe_api_sp_board.port_object_list[port_num].enclosure_object_list[lcc_num].disk_fbe_object_list[disk_num].logical_disk.logical_disk_object.object_id;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
    

}


/*********************************************************************
 *            fbe_api_object_map_reset_all_tables ()
 *********************************************************************
 *
 *  Description: set all object ids to FBE_OBJECT_ID_INVALID
 *
 *  Inputs: 
 *
 *  Return Value: 
 *
 *  History:
 *    6/20/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_reset_all_tables()
{
    fbe_u32_t       port_index = 0; 
    fbe_u32_t       encl_index = 0;
    fbe_u32_t       comp_index = 0;
    fbe_u32_t       drive_index = 0;
    port_object_map_t      *port_ptr = NULL;
    enclosure_object_map_t *encl_ptr = NULL;
    component_object_map_t *comp_ptr = NULL;
    disk_object_map_t      *disk_ptr = NULL;

    fbe_spinlock_lock (&map_update_lock);

    fbe_api_sp_board.board_object.object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_sp_board.board_object.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_api_sp_board.board_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
    fbe_api_sp_board.board_object.discovered = FBE_FALSE;

    
    /*all objects must start as invalids*/
    for (port_index = 0; port_index < FBE_API_PHYSICAL_BUS_COUNT; port_index ++){
        port_ptr = &fbe_api_sp_board.port_object_list[port_index];
    
        port_ptr->port_object.object_id = FBE_OBJECT_ID_INVALID;
        port_ptr->port_object.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
        port_ptr->port_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
        port_ptr->port_object.discovered = FBE_FALSE;
        
        for (encl_index = 0; encl_index < FBE_API_MAX_ALLOC_ENCL_PER_BUS; encl_index ++){
            encl_ptr = &port_ptr->enclosure_object_list[encl_index];

            encl_ptr->enclosure_object.object_id = FBE_OBJECT_ID_INVALID;
            encl_ptr->enclosure_object.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
            encl_ptr->enclosure_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
            encl_ptr->enclosure_object.discovered = FBE_FALSE;
            encl_ptr->encl_addr = FBE_ENCLOSURE_NUMBER_INVALID;

            for (comp_index = 0; comp_index < FBE_API_MAX_ENCL_COMPONENTS; comp_index ++){
                comp_ptr = &encl_ptr->component_object_list[comp_index];
                
                comp_ptr->component_object.object_id = FBE_OBJECT_ID_INVALID;
                comp_ptr->component_object.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
                comp_ptr->component_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
                comp_ptr->component_object.discovered = FBE_FALSE;
                comp_ptr->port = FBE_PORT_NUMBER_INVALID;
                comp_ptr->encl = FBE_ENCLOSURE_NUMBER_INVALID;
                comp_ptr->component_id = FBE_ENCLOSURE_NUMBER_INVALID;
            }

            for (drive_index = 0; drive_index < FBE_API_ENCLOSURE_SLOTS; drive_index ++){
                disk_ptr = &encl_ptr->disk_fbe_object_list[drive_index];

                /*logical reset*/
                disk_ptr->logical_disk.logical_disk_object.object_id = FBE_OBJECT_ID_INVALID;
                disk_ptr->logical_disk.logical_disk_object.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
                disk_ptr->logical_disk.logical_disk_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
                disk_ptr->logical_disk.logical_disk_object.discovered = FBE_FALSE;
                disk_ptr->logical_disk.near_end_of_life = FBE_FALSE;
                disk_ptr->logical_disk.encl_addr = FBE_ENCLOSURE_NUMBER_INVALID;                

                /*physical reset*/
                disk_ptr->physical_disk_object.object_id = FBE_OBJECT_ID_INVALID;
                disk_ptr->physical_disk_object.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
                disk_ptr->physical_disk_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
                disk_ptr->physical_disk_object.discovered = FBE_FALSE;
                disk_ptr->encl_addr = FBE_ENCLOSURE_NUMBER_INVALID;
            }
        }
    }

    for(encl_index = 0; encl_index < FBE_API_ENCLOSURE_COUNT; encl_index++)
    {
        encl_addr_to_physical_pos[encl_index].port = FBE_PORT_NUMBER_INVALID;
        encl_addr_to_physical_pos[encl_index].encl_pos = FBE_ENCLOSURE_NUMBER_INVALID;
    }
    fbe_spinlock_unlock (&map_update_lock);    

    return FBE_STATUS_OK;
}

/********************************************************************
 *            fbe_api_object_map_add_board_object ()
 *********************************************************************
 *
 *  Description: update the board object id in the map
 *
 *  Inputs: object_id and state
 *
 *  Return Value: success or failure
 *
 *  History:
 *    8/21/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_add_board_object (fbe_object_id_t new_object_id, fbe_lifecycle_state_t lifecycle_state)
{
    object_map_base_object_t *      object_ptr = NULL;
    board_object_map_t *            board_ptr = NULL;
    
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                                "%s: attempt to add an object to an uninitialized map !\n", __FUNCTION__); 
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (fbe_api_object_map_object_discovered(new_object_id)) {
        object_ptr = object_map_get_object_ptr(new_object_id);
        board_ptr = (board_object_map_t *)object_ptr;

        if (board_ptr->board_object.object_id == new_object_id) {
            /*same object, no need to update, just leave*/
            fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                                    "%s:object(0x%X)was just added by another thread\n", __FUNCTION__, new_object_id); 

            fbe_api_sp_board.board_object.lifecycle_state = lifecycle_state;
            return FBE_STATUS_OK;
        }else {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Obj ID 0x%X already exist\n", __FUNCTION__, new_object_id); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_spinlock_lock (&map_update_lock);
    fbe_api_sp_board.board_object.object_id = new_object_id;
    fbe_api_sp_board.board_object.lifecycle_state = lifecycle_state;
    fbe_api_sp_board.board_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_BOARD;
    fbe_api_sp_board.board_object.discovered = FBE_TRUE;
    fbe_spinlock_unlock (&map_update_lock);
    
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_MEDIUM,
            "obj_map_add_brd:added object 0x%X for board\n", 
            new_object_id);
    
    return FBE_STATUS_OK;
}

/********************************************************************
 *            fbe_api_object_map_add_port_object ()
 *********************************************************************
 *
 *  Description: update a port object in the map
 *
 *  Inputs: object_id and state
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/22/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_add_port_object (fbe_object_id_t new_object_id, fbe_port_number_t port_number, fbe_lifecycle_state_t lifecycle_state)
{
    object_map_base_object_t *      object_ptr = NULL;
    port_object_map_t *             port_ptr = NULL;
    
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                                "%s: attempt to add an object to an uninitialized map !\n", __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_api_object_map_object_discovered(new_object_id)) {
        object_ptr = object_map_get_object_ptr(new_object_id);
        port_ptr = (port_object_map_t *)object_ptr;

        if (port_ptr->port == port_number && port_ptr->port_object.object_id == new_object_id) {
            /*same object, no need to update, just leave*/
            fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                                    "%s:object(0x%X)was just added by another thread\n", __FUNCTION__, new_object_id); 

            port_ptr->port_object.lifecycle_state = lifecycle_state;
            return FBE_STATUS_OK;
        }else {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                    "%s:Obj ID 0x%X already exist in port:%d\n", 
                    __FUNCTION__, new_object_id, port_ptr->port);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_spinlock_lock (&map_update_lock);
    port_ptr = &fbe_api_sp_board.port_object_list[port_number];

    port_ptr->port_object.object_id = new_object_id;
    port_ptr->port_object.lifecycle_state = lifecycle_state;
    port_ptr->port_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PORT;
    port_ptr->port_object.discovered = FBE_TRUE;
    port_ptr->port = port_number;
    fbe_spinlock_unlock (&map_update_lock);

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_MEDIUM,
            "obj_map_add_port: added object 0x%X in port %d\n", 
            new_object_id, port_number);

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_object_map_add_enclosure_object ()
 *********************************************************************
 *
 *  Description: update an enclosure object in the map
 *
 *  Inputs: object_id and state
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/22/07: sharel   created
 *   
 *********************************************************************/
fbe_status_t fbe_api_object_map_add_enclosure_object (fbe_object_id_t new_object_id, fbe_port_number_t port_number, fbe_enclosure_number_t encl_number, fbe_lifecycle_state_t lifecycle_state)

{   
    object_map_base_object_t *      object_ptr = NULL;
    enclosure_object_map_t *        encl_ptr = NULL;

    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                                "%s: attempt to add an object to an uninitialized map !\n", __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_api_object_map_object_discovered(new_object_id)) {
        object_ptr = object_map_get_object_ptr(new_object_id);
        encl_ptr = (enclosure_object_map_t *)object_ptr;

        if (encl_ptr->port == port_number && encl_ptr->encl == encl_number && encl_ptr->enclosure_object.object_id == new_object_id) {
            /*same object, no need to update, just leave*/
            fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                                    "%s:object(0x%X)was just added by another thread\n", __FUNCTION__, new_object_id); 

            encl_ptr->enclosure_object.lifecycle_state = lifecycle_state;
            return FBE_STATUS_OK;
        }else {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                    "%s:Obj ID 0x%X already exist in port:%d, encl:%d\n", 
                    __FUNCTION__, new_object_id, encl_ptr->port, encl_ptr->encl);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_spinlock_lock (&map_update_lock);
    encl_ptr = &fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[encl_number];

    encl_ptr->enclosure_object.object_id = new_object_id;
    encl_ptr->enclosure_object.lifecycle_state = lifecycle_state;
    encl_ptr->enclosure_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE;
    encl_ptr->enclosure_object.discovered = FBE_TRUE;
    encl_ptr->port = port_number;
    encl_ptr->encl = encl_number;
    fbe_spinlock_unlock (&map_update_lock);

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_MEDIUM,
            "obj_map_add_encl: added object 0x%X in port:%d, encl_number:%d\n", 
            new_object_id, port_number, encl_number);

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_object_map_add_lcc_object ()
 *********************************************************************
 *
 *  Description: update a lcc object in the map
 *
 *  Inputs: object_id and state
 *
 *  Return Value: success or failure
 *
 *  History:
 *    05/07/10: Gerry Fredette created
 *   
 *********************************************************************/
fbe_status_t fbe_api_object_map_add_lcc_object (fbe_object_id_t new_object_id, fbe_port_number_t port_number, 
    fbe_enclosure_number_t encl_number, fbe_component_id_t component_id, fbe_lifecycle_state_t lifecycle_state)

{
    object_map_base_object_t *      object_ptr = NULL;
    enclosure_object_map_t *        encl_ptr = NULL;
    component_object_map_t *        comp_ptr = NULL;

    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                                "%s: attempt to add an object to an uninitialized map !\n", __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_api_object_map_object_discovered(new_object_id)) {
        object_ptr = object_map_get_object_ptr(new_object_id);
        comp_ptr = (component_object_map_t *) object_ptr;

        if (comp_ptr->component_id == component_id &&
            comp_ptr->port == port_number &&
            comp_ptr->encl == encl_number &&
            comp_ptr->component_object.object_id == new_object_id)
        {
            /*same object, no need to update, just leave*/
            fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                                    "%s:object(0x%X)was just added by another thread\n", __FUNCTION__, new_object_id); 

            comp_ptr->component_object.lifecycle_state = lifecycle_state;
            return FBE_STATUS_OK;
        }else {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Obj ID 0x%X already exist in port:%d, encl:%d, compid:%d\n", 
                    __FUNCTION__, new_object_id, comp_ptr->port, comp_ptr->encl, comp_ptr->component_id); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_spinlock_lock (&map_update_lock);
    encl_ptr = &fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[encl_number];
    comp_ptr = &encl_ptr->component_object_list[component_id];
    
    comp_ptr->component_object.object_id = new_object_id; 
    comp_ptr->component_object.lifecycle_state = lifecycle_state;
    comp_ptr->component_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LCC;
    comp_ptr->component_object.discovered = FBE_TRUE;
    comp_ptr->port = port_number;
    comp_ptr->encl = encl_number;
    comp_ptr->component_id = component_id;
    fbe_spinlock_unlock (&map_update_lock);

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_MEDIUM,
            "obj_map_add_lcc: added object 0x%X in port:%d, encl_number:%d, compid %d\n", 
            new_object_id, port_number, encl_number, component_id);

    return FBE_STATUS_OK;
}



/*********************************************************************
 *            fbe_api_object_map_add_physical_drive_object ()
 *********************************************************************
 *
 *  Description: update a disk object in the map
 *
 *  Inputs: object_id and state
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/22/07: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_add_physical_drive_object (fbe_object_id_t new_object_id,
                                                   fbe_port_number_t port_number,
                                                   fbe_enclosure_number_t lcc_number,
                                                   fbe_enclosure_slot_number_t disk_number,
                                                   fbe_lifecycle_state_t lifecycle_state)
{   
    object_map_base_object_t *      object_ptr = NULL;
    disk_object_map_t *             physical_ptr = NULL;
    
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                                "%s: attempt to add an object to an uninitialized map !\n", __FUNCTION__); 

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_api_object_map_object_discovered(new_object_id)) {
        object_ptr = object_map_get_object_ptr(new_object_id);
        physical_ptr = (disk_object_map_t *)object_ptr;

        if (physical_ptr->port == port_number && physical_ptr->encl == lcc_number &&  physical_ptr->slot == disk_number && physical_ptr->physical_disk_object.object_id == new_object_id) {
            /*same object, no need to update, just leave*/
            fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                                    "%s:object(0x%X)was just added by another thread\n", __FUNCTION__, new_object_id); 
            fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].physical_disk_object.lifecycle_state = lifecycle_state;
            return FBE_STATUS_OK;
        }else {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Obj ID 0x%X already exist in port:%d, encl:%d, drive:%d\n", __FUNCTION__, new_object_id, physical_ptr->port, physical_ptr->encl, physical_ptr->slot); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_spinlock_lock (&map_update_lock);
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].physical_disk_object.object_id = new_object_id;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].physical_disk_object.lifecycle_state = lifecycle_state;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].physical_disk_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].physical_disk_object.discovered = FBE_TRUE;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].port = port_number;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].encl = lcc_number;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].slot = disk_number;
    fbe_spinlock_unlock (&map_update_lock);

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_MEDIUM,
            "obj_map_add_physdrv: added object 0x%X in port:%d, lcc:%d, drive:%d\n", 
            new_object_id, port_number, lcc_number, disk_number);
    
    return FBE_STATUS_OK;

}

/*********************************************************************
 *            fbe_api_object_map_object_id_exists ()
 *********************************************************************
 *
 *  Description: check if this object id already exists in the map
 *
 *  Inputs: object_id 
 *
 *  Return exits or not
 *
 *  History:
 *    6/22/07: sharel   created
 *    8/08/07: sharel   changed to using object_map_get_object_ptr
 *********************************************************************/
fbe_bool_t fbe_api_object_map_object_id_exists (fbe_object_id_t object_id)
{

    object_map_base_object_t *      object_ptr = object_map_get_object_ptr(object_id);
    fbe_bool_t                      discovered = FBE_FALSE;

    if (object_ptr != NULL) {
        fbe_spinlock_lock (&map_update_lock);
        discovered = object_ptr->discovered;
        fbe_spinlock_unlock (&map_update_lock);
    }

    if (object_ptr == NULL || !discovered) {
        return FALSE;
    } else {
        return TRUE;
    }
}

fbe_status_t fbe_api_object_map_get_object_port_index (fbe_object_id_t object_id, fbe_s32_t *index)
{
    object_map_base_object_t *      object_ptr =  object_map_get_object_ptr(object_id);
    logical_disk_object_map_t *     logical_ptr = NULL;
    disk_object_map_t *             physical_ptr = NULL;
    enclosure_object_map_t *        encl_ptr = NULL;
    port_object_map_t *             port_ptr = NULL;

    if (object_ptr == NULL) {
        *index = -1;
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock (&map_update_lock);
    if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE) {
        logical_ptr = (logical_disk_object_map_t *)object_ptr;
        *index  = logical_ptr->port;
    } else if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
        physical_ptr = (disk_object_map_t *)object_ptr;;
        *index  = physical_ptr->port;
    }else if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE){
        encl_ptr = (enclosure_object_map_t *)object_ptr;
        *index = encl_ptr->port;
    }else if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PORT){
        port_ptr = (port_object_map_t *)object_ptr;
        *index = port_ptr->port;
    }else {
        *index = -1;
    }
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_api_object_map_get_object_encl_index (fbe_object_id_t object_id,  fbe_s32_t *index)
{
    object_map_base_object_t *      object_ptr =  object_map_get_object_ptr(object_id);
    logical_disk_object_map_t *     logical_ptr = NULL;
    disk_object_map_t *             physical_ptr = NULL;
    enclosure_object_map_t *        encl_ptr = NULL;

    if (object_ptr == NULL) {
        *index = -1;
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock (&map_update_lock);
    if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE) {
        logical_ptr = (logical_disk_object_map_t *)object_ptr;
        *index  = logical_ptr->encl;
    } else if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
        physical_ptr = (disk_object_map_t *)object_ptr;;
        *index  = physical_ptr->encl;
    }else if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE){
        encl_ptr = (enclosure_object_map_t *)object_ptr;
        *index = encl_ptr->encl;
    }else {
        *index = -1;
    }
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
}

fbe_status_t  fbe_api_object_map_get_object_drive_index (fbe_object_id_t object_id,  fbe_s32_t *index)
{
    object_map_base_object_t *      object_ptr =  object_map_get_object_ptr(object_id);
    logical_disk_object_map_t *     logical_ptr = NULL;
    disk_object_map_t *             physical_ptr = NULL;

    if (object_ptr == NULL) {
        *index = -1;
        return FBE_STATUS_OK;
    }
    
    fbe_spinlock_lock (&map_update_lock);
    if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE) {
        logical_ptr = (logical_disk_object_map_t *)object_ptr;
        *index  = logical_ptr->slot;
    } else if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
        physical_ptr = (disk_object_map_t *)object_ptr;
        *index  = physical_ptr->slot;
    }else {
        *index = -1;
    }
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
}


fbe_status_t  fbe_api_object_map_get_object_encl_address (fbe_object_id_t object_id,  fbe_s32_t *index)
{
    object_map_base_object_t *      object_ptr =  object_map_get_object_ptr(object_id);
    disk_object_map_t *             physical_ptr = NULL;
    logical_disk_object_map_t *     logical_ptr = NULL;

    if (object_ptr == NULL) {
        *index = -1;
        return FBE_STATUS_OK;
    }
    
    fbe_spinlock_lock (&map_update_lock);

    if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE) {
        logical_ptr = (logical_disk_object_map_t *)object_ptr;
        *index  = logical_ptr->encl_addr;
    } else if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE) {
        physical_ptr = (disk_object_map_t *)object_ptr;
        *index  = physical_ptr->encl_addr;
    }else {
        *index = -1;
    }

    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
}


fbe_status_t fbe_api_object_map_add_logical_drive_object (fbe_object_id_t new_object_id,
                                                           fbe_port_number_t port_number,
                                                           fbe_enclosure_number_t lcc_number,
                                                           fbe_enclosure_slot_number_t disk_number,
                                                           fbe_lifecycle_state_t lifecycle_state)
{
    object_map_base_object_t *      object_ptr = NULL;
    logical_disk_object_map_t *     logical_ptr = NULL;
    
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING,
                                "%s: attempt to add an object to an uninitialized map !\n",__FUNCTION__); 
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_api_object_map_object_discovered(new_object_id)) {
        object_ptr = object_map_get_object_ptr(new_object_id);
        logical_ptr = (logical_disk_object_map_t *)object_ptr;

        if (logical_ptr->port == port_number && logical_ptr->encl == lcc_number &&  logical_ptr->slot == disk_number && logical_ptr->logical_disk_object.object_id == new_object_id) {
            /*same object, no need to update, just leave*/
            fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                                    "%s:object(0x%X)was just added by another thread\n", __FUNCTION__, new_object_id); 

            fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.logical_disk_object.lifecycle_state = lifecycle_state;
            return FBE_STATUS_OK;
        }else {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Obj ID 0x%X already exist in port:%d, encl:%d, drive:%d\n", __FUNCTION__, new_object_id, logical_ptr->port, logical_ptr->encl, logical_ptr->slot); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_spinlock_lock (&map_update_lock);
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.logical_disk_object.object_id = new_object_id;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.logical_disk_object.lifecycle_state = lifecycle_state;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.logical_disk_object.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.logical_disk_object.discovered = FBE_TRUE;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.port = port_number;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.encl = lcc_number;
    fbe_api_sp_board.port_object_list[port_number].enclosure_object_list[lcc_number].disk_fbe_object_list[disk_number].logical_disk.slot = disk_number;
    fbe_spinlock_unlock (&map_update_lock);

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s:added object 0x%X in port:%d, lcc:%d, drive:%d\n", __FUNCTION__, new_object_id, port_number,lcc_number,disk_number);
    
    return FBE_STATUS_OK;

}

/*********************************************************************
 *            fbe_api_object_map_get_physical_drive_obj_id ()
 *********************************************************************
 *
 *  Description: Get the object id of the specific physical disk connected to
                 a specific lcc on a pecific port object
 *
 *  Inputs: port, lcc and disk slot number and a pointer to the object id
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    5/22/08: sharel   created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_get_physical_drive_obj_id(fbe_u32_t port_num, fbe_u32_t lcc_num, fbe_u32_t disk_num, fbe_object_id_t *obj_id_ptr)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_spinlock_lock (&map_update_lock);
    *obj_id_ptr = fbe_api_sp_board.port_object_list[port_num].enclosure_object_list[lcc_num].disk_fbe_object_list[disk_num].physical_disk_object.object_id;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
    

}

/*********************************************************************
 *            object_map_get_object_lifecycle_state ()
 *********************************************************************
 *
 *  Description: returns the state of the object as it shows in the table
 *
 *  Inputs: object id
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    7/18/08: sharel   created
 *    
 *********************************************************************/
fbe_status_t object_map_get_object_lifecycle_state (fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state)
{
    object_map_base_object_t *  object_ptr = object_map_get_object_ptr(object_id);
    
    if (object_ptr == NULL || lifecycle_state == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&map_update_lock);
    *lifecycle_state = object_ptr->lifecycle_state;
    fbe_spinlock_unlock (&map_update_lock);

    return FBE_STATUS_OK;
}

/*set the state of the object*/
fbe_status_t object_map_set_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t lifecycle_state)
{
    object_map_base_object_t *  object_ptr = object_map_get_object_ptr(object_id);
    logical_disk_object_map_t * logical_disk_p = NULL;
    enclosure_object_map_t *enclosure_p = NULL;

    if (object_ptr == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_spinlock_lock (&map_update_lock);

    object_ptr->lifecycle_state = lifecycle_state;/*change state*/

    /*and check for some special cases*/
    if (lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY){

        fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                                "%s:object destroyed: 0x%X\n", __FUNCTION__, object_id); 
    
        if (object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE) {
            logical_disk_p = (logical_disk_object_map_t *)object_ptr;
            logical_disk_p->near_end_of_life = FALSE;
        }
        else if(object_ptr->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) {
            enclosure_p = (enclosure_object_map_t *)object_ptr;

            /* Following function needs map_update_lock. We are currently holding
             * it. Release map update lock and latter acquire it again.
             * We can probably check status of following call. But what do we do
             * even if it has failed. Probably nothing. Expecting a review suggestion.
             */
#if 1
            fbe_spinlock_unlock (&map_update_lock);             
            fbe_api_object_map_interface_set_encl_physical_pos(FBE_PORT_NUMBER_INVALID,
                                                               FBE_ENCLOSURE_NUMBER_INVALID,
                                                               enclosure_p->encl_addr);
            fbe_spinlock_lock (&map_update_lock);            
#else
            encl_addr_to_physical_pos[encl_addr].port= port;    
            encl_addr_to_physical_pos[encl_addr].encl_pos = encl_pos;
#endif
            enclosure_p->encl_addr = FBE_ENCLOSURE_NUMBER_INVALID;            
        }
    
        object_ptr->object_id = FBE_OBJECT_ID_INVALID;
        object_ptr->object_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;
        object_ptr->discovered = FBE_FALSE;
        object_ptr = NULL;
    }

    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
}

fbe_status_t object_map_get_object_type (fbe_object_id_t object_id, fbe_topology_object_type_t *object_type)
{
    object_map_base_object_t *object_ptr = object_map_get_object_ptr(object_id);

    if (object_ptr == NULL || object_type == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&map_update_lock);
    *object_type = object_ptr->object_type;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;


}

fbe_bool_t  fbe_api_object_map_object_discovered(fbe_object_id_t object_id)
{
    object_map_base_object_t *object_ptr = object_map_get_object_ptr (object_id);

    if (object_ptr == NULL) {
        return FBE_FALSE;
    }
    
    return object_ptr->discovered;

}

/*********************************************************************
 *            fbe_api_object_map_get_total_logical_drives ()
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
fbe_status_t fbe_api_object_map_get_total_logical_drives (fbe_u32_t port_number, fbe_u32_t *total_drives)
{
    fbe_u32_t   encl_index = 0;
    fbe_u32_t   logical_drive_index = 0;
    fbe_u32_t   drive_count = 0;

    for (encl_index = 0; encl_index < FBE_API_MAX_ALLOC_ENCL_PER_BUS; encl_index++) {
        for (logical_drive_index = 0; logical_drive_index < FBE_API_ENCLOSURE_SLOTS; logical_drive_index++) {
             if (fbe_api_sp_board.port_object_list[port_number].
                 enclosure_object_list[encl_index].
                 disk_fbe_object_list[logical_drive_index].logical_disk.logical_disk_object.discovered){

                 drive_count++;
             }
        }
    }

    *total_drives = drive_count;

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            object_map_get_object_ptr ()
 *********************************************************************
 *
 *  Description: return the address of the object_map_base_object_t area of this object id
 *
 *  Inputs: object_id
 *
 *  Return Value: pointer to objects
 *
 *  History:
 *    11/26/08: sharel  created
 *    
 *********************************************************************/
static object_map_base_object_t * object_map_get_object_ptr (fbe_object_id_t object_id)
{
    fbe_u32_t       port_index = 0; 
    fbe_u32_t       encl_index = 0;
    fbe_u32_t       comp_index = 0;
    fbe_u32_t       drive_index = 0;

    fbe_spinlock_lock (&map_update_lock);

    if (fbe_api_sp_board.board_object.object_id == object_id) {
        fbe_spinlock_unlock (&map_update_lock);
        return &fbe_api_sp_board.board_object;
    }
    
    for (port_index = 0; port_index < FBE_API_PHYSICAL_BUS_COUNT; port_index ++){

        if (fbe_api_sp_board.port_object_list[port_index].port_object.object_id == object_id){
            fbe_spinlock_unlock (&map_update_lock);
            return &fbe_api_sp_board.port_object_list[port_index].port_object;
        }

        for (encl_index = 0; encl_index < FBE_API_MAX_ALLOC_ENCL_PER_BUS; encl_index ++){

            if (fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].enclosure_object.object_id == object_id){
                fbe_spinlock_unlock (&map_update_lock);
                return &fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].enclosure_object;
            }
            for (comp_index = 0; comp_index < FBE_API_MAX_ENCL_COMPONENTS; comp_index++){

                if (fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].component_object_list[comp_index].component_object.object_id == object_id){
                    fbe_spinlock_unlock (&map_update_lock);
                    return &fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].component_object_list[comp_index].component_object;
                }
            }

            for (drive_index = 0; drive_index < FBE_API_ENCLOSURE_SLOTS; drive_index ++){

                if (fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].disk_fbe_object_list[drive_index].logical_disk.logical_disk_object.object_id == object_id){
                    fbe_spinlock_unlock (&map_update_lock);
                    return &fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].disk_fbe_object_list[drive_index].logical_disk.logical_disk_object;
                }

                if (fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].disk_fbe_object_list[drive_index].physical_disk_object.object_id == object_id){
                    fbe_spinlock_unlock (&map_update_lock);
                    return &fbe_api_sp_board.port_object_list[port_index].enclosure_object_list[encl_index].disk_fbe_object_list[drive_index].physical_disk_object;
                }

            }


        }
    }

    fbe_spinlock_unlock (&map_update_lock);

    /*we did not find the object*/
    return NULL;
}

/*********************************************************************
 *            fbe_api_object_map_get_object_handle ()
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
fbe_api_object_map_handle fbe_api_object_map_get_object_handle (fbe_object_id_t object_id)
{
    return (fbe_u64_t)(csx_ptrhld_t)object_map_get_object_ptr(object_id);

}

/*********************************************************************
 *            fbe_api_object_map_get_logical_drive_object_handle ()
 *********************************************************************
 *
 *  Description: return the logical drive handle using the port/encl/slot arguments
 *
 *  Inputs: port/encl/slot of the drive we want
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/26/08: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_get_logical_drive_object_handle (fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t disk_num, fbe_api_object_map_handle *handle)
{
    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    * handle =(fbe_api_object_map_handle) (csx_ptrhld_t)&fbe_api_sp_board.port_object_list[port_num].enclosure_object_list[encl_num].disk_fbe_object_list[disk_num].logical_disk.logical_disk_object;
    
    return FBE_STATUS_OK;

}

/*********************************************************************
 *            fbe_api_object_map_get_obj_info_by_handle ()
 *********************************************************************
 *
 *  Description: return the information we need at one shot to prevent
 *               multiple locking in the IO path
 *
 *  Inputs: handle
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/26/08: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_get_obj_info_by_handle (fbe_api_object_map_handle object_handle, fbe_api_object_map_obj_info_t *object_info)
{

    object_map_base_object_t *object_ptr = (object_map_base_object_t *)(csx_ptrhld_t)object_handle;

    fbe_spinlock_lock (&map_update_lock);
    object_info->object_id = object_ptr->object_id;
    object_info->object_state = object_ptr->lifecycle_state;
    fbe_spinlock_unlock (&map_update_lock);

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_object_map_is_proactive_sparing_recommanded ()
 *********************************************************************
 *
 *  Description: tells the user if this drive recommanded proactive sparing
 *               this is ususally used in boot time to check if we missed the notification
 *               fro proactive sparing
 *
 *  Inputs: port, encl and disk slot number and a pointer to the result
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/02/09: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_is_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t *recommanded)
{
    logical_disk_object_map_t *logical_ptr = NULL;

    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    logical_ptr = (logical_disk_object_map_t *)object_map_get_object_ptr(object_id);
    if (logical_ptr == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't find object ID %d in map\n", __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&map_update_lock);
    *recommanded = logical_ptr->near_end_of_life;
    fbe_spinlock_unlock (&map_update_lock);

    return FBE_STATUS_OK;
}

/*********************************************************************
 *            fbe_api_object_map_set_proactive_sparing_recommanded ()
 *********************************************************************
 *
 *  Description: sets the proactive sparing flag in the object.
 *               this is usually done at boot time so if the physical package
 *               client missed the notification, it would still know this drive is dying
 *
 *  Inputs: port, encl and disk slot number and bool value to set
 *
 *  Return Value: success or failue
 *
 *  History:
 *    03/02/09: sharel  created
 *    
 *********************************************************************/
fbe_status_t fbe_api_object_map_set_proactive_sparing_recommanded(fbe_object_id_t object_id, fbe_bool_t recommanded)
{
    logical_disk_object_map_t *logical_ptr = NULL;

    if (map_initialized != TRUE) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    logical_ptr = (logical_disk_object_map_t *)object_map_get_object_ptr(object_id);
    if (logical_ptr == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't find object ID %d in map\n", __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock (&map_update_lock);
    logical_ptr->near_end_of_life = recommanded;
    fbe_spinlock_unlock (&map_update_lock);

    return FBE_STATUS_OK;
}


/*********************************************************************
 *           fbe_api_object_map_set_encl_addr()
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
 *    03/11/09: Prahlad Purohit -- Created
 *
 *********************************************************************/
fbe_status_t 
fbe_api_object_map_set_encl_addr(fbe_u32_t port, 
                                 fbe_u32_t encl_pos, 
                                 fbe_u32_t encl_addr)
{
    fbe_u8_t disk_index;
    
    fbe_spinlock_lock (&map_update_lock);
    fbe_api_sp_board.port_object_list[port].enclosure_object_list[encl_pos].encl_addr = encl_addr;

    for(disk_index = 0; disk_index < FBE_API_ENCLOSURE_SLOTS; disk_index++) {
        fbe_api_sp_board.port_object_list[port].enclosure_object_list[encl_pos].disk_fbe_object_list[disk_index].encl_addr = encl_addr;
        fbe_api_sp_board.port_object_list[port].enclosure_object_list[encl_pos].disk_fbe_object_list[disk_index].logical_disk.encl_addr = encl_addr;
    }
    fbe_spinlock_unlock (&map_update_lock);        
    return FBE_STATUS_OK;
}


/*********************************************************************
 *           fbe_api_object_map_get_encl_addr()
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
 *    03/11/09: Prahlad Purohit -- Created
 *
 *********************************************************************/
fbe_status_t 
fbe_api_object_map_get_encl_addr(fbe_u32_t port, 
                                 fbe_u32_t encl_pos, 
                                 fbe_u32_t *encl_addr)
{
    fbe_spinlock_lock (&map_update_lock);
    *encl_addr = fbe_api_sp_board.port_object_list[port].enclosure_object_list[encl_pos].encl_addr;
    fbe_spinlock_unlock (&map_update_lock);
    return FBE_STATUS_OK;
}

/*********************************************************************
 *           fbe_api_object_map_set_encl_physical_pos()
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
fbe_api_object_map_set_encl_physical_pos(fbe_u32_t port, 
                                         fbe_u32_t encl_pos, 
                                         fbe_u32_t encl_addr)
{
    fbe_spinlock_lock (&map_update_lock);
    encl_addr_to_physical_pos[encl_addr].port= port;    
    encl_addr_to_physical_pos[encl_addr].encl_pos = encl_pos;
    fbe_spinlock_unlock (&map_update_lock);        
    return FBE_STATUS_OK;
}

/*********************************************************************
 *           fbe_api_object_map_get_encl_physical_pos()
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
fbe_api_object_map_get_encl_physical_pos(fbe_u32_t *port, 
                                         fbe_u32_t *encl_pos, 
                                         fbe_u32_t encl_addr)
{
    fbe_spinlock_lock (&map_update_lock);
    *port = encl_addr_to_physical_pos[encl_addr].port;
    *encl_pos = encl_addr_to_physical_pos[encl_addr].encl_pos;
    fbe_spinlock_unlock (&map_update_lock);        
    return FBE_STATUS_OK;
}



/*********************************************************************
*          fbe_api_object_map_get_port_enclosure_slot_number()
**********************************************************************
*
* Description: Gets Port, Enclosure and Slot number for specified
*              Object Id 
*  Inputs:
*     port_num      - Pointer to return port number
*     encl_num      - Pointer to return enclosure number
*     slot_num      - Pointer to return slot number
*     object_id     - Object identifier
*     logical_disk_obj -Boolean to identify object type
*
*  Return - fbe_status_t the status of the operation. 
*
*  History:
*    08/03/10: Mahesh Agarkar -- Created
**********************************************************************/
fbe_status_t fbe_api_object_map_get_port_enclosure_slot_number(fbe_object_id_t object_id,
                                                               fbe_class_id_t class_id,
                                                               fbe_port_number_t *port_p,
                                                               fbe_enclosure_number_t *encl_p,
                                                               fbe_enclosure_slot_number_t *slot_p)
{
    fbe_port_number_t port_num;
    fbe_enclosure_number_t encl_num;
    fbe_enclosure_slot_number_t slot_num;
    fbe_component_id_t comp_id;

    fbe_object_id_t port_obj_id;
    fbe_object_id_t encl_obj_id;
    fbe_object_id_t encl_comp_obj_id;

    fbe_object_id_t obj_id_ptr_ldo = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t obj_id_ptr_pdo = FBE_OBJECT_ID_INVALID;

    fbe_bool_t ldo_object = FBE_FALSE;
    fbe_bool_t pdo_object = FBE_FALSE;

    port_object_map_t *port_obj_list;
    enclosure_object_map_t *enclosure_obj_list;

    *port_p = FBE_PORT_NUMBER_INVALID;
    *encl_p = FBE_ENCLOSURE_NUMBER_INVALID;
    *slot_p = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;

    /* Validate that the map is initialized */
    if (map_initialized != TRUE) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: attempt to access uninitialized map\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((class_id >= FBE_CLASS_ID_PORT_FIRST) &&
        (class_id <= FBE_CLASS_ID_PORT_LAST))
    {
        /* only port number is required */
        for(port_num = 0 ; port_num < FBE_API_PHYSICAL_BUS_COUNT ; port_num++)
        {
            port_obj_id = fbe_api_sp_board.port_object_list[port_num].port_object.object_id;
            if(port_obj_id == object_id)
            {
                /* got it*/
                *port_p = port_num;
                return FBE_STATUS_OK;
            }
        }
    }
    else if((class_id >= FBE_CLASS_ID_ENCLOSURE_FIRST) &&
            (class_id <= FBE_CLASS_ID_ENCLOSURE_LAST))
    {
        /* we want port and enclosure number*/
        for(port_num = 0 ; port_num < FBE_API_PHYSICAL_BUS_COUNT ; port_num++)
        {
            for(encl_num = 0; encl_num < FBE_API_MAX_ALLOC_ENCL_PER_BUS ; encl_num++)
            {
                encl_obj_id = fbe_api_sp_board.port_object_list[port_num].enclosure_object_list[encl_num].enclosure_object.object_id;
                if(encl_obj_id == object_id)
                {
                    /* got it*/
                    *port_p = port_num;
                    *encl_p = encl_num;
                    return FBE_STATUS_OK;
                }
            }/*end for(encl_num = 0; encl_num < FBE_API_MAX_ALLOC_ENCL_PER_BUS ; encl_num++)*/
        }/*end for(port_num = 0 ; port_num < FBE_API_PHYSICAL_BUS_COUNT ; port_num++)*/
    }/*end else if((class_id >= FBE_CLASS_ID_ENCLOSURE_FIRST) &&..*/

    else if(class_id == FBE_CLASS_ID_SAS_VOYAGER_EE_LCC)
    {
        /* we want port and enclosure number*/
        for(port_num = 0 ; port_num < FBE_API_PHYSICAL_BUS_COUNT ; port_num++)
        {
            for(encl_num = 0; encl_num < FBE_API_MAX_ALLOC_ENCL_PER_BUS ; encl_num++)
            {
                for(comp_id = 0; comp_id < FBE_API_MAX_ENCL_COMPONENTS; comp_id++)
                {
                    encl_comp_obj_id = fbe_api_sp_board.port_object_list[port_num].enclosure_object_list[encl_num].component_object_list[comp_id].component_object.object_id; 
                    if(encl_comp_obj_id == object_id)
                    {
                        /* got it*/
                        *port_p = port_num;
                        *encl_p = encl_num;
                        return FBE_STATUS_OK;
                    }
                }/*end for(comp_id = 0; comp_id < FBE_API_MAX_ENCL_COMPONENTS; comp_id++) */
            }/*end for(encl_num = 0; encl_num < FBE_API_MAX_ALLOC_ENCL_PER_BUS ; encl_num++)*/
        }/*end for(port_num = 0 ; port_num < FBE_API_PHYSICAL_BUS_COUNT ; port_num++)*/        
    }
    /* we are here to find either the LDO/PDO slot,enclosure and port number*/
    else
    {
        if((class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) &&
            (class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST))
        {
            /* we want port,enclosure and slot number for PDO*/
            pdo_object = FBE_TRUE;
        }

        else if ((class_id >= FBE_CLASS_ID_LOGICAL_DRIVE_FIRST) &&
                 (class_id <= FBE_CLASS_ID_LOGICAL_DRIVE_LAST))
        {
            /* we want port,enclosure and slot number for LDO*/
            ldo_object = FBE_TRUE;
        }

        else
        {
            /* the class_id is invalide, just return*/
            return FBE_STATUS_OK;
        }

        /* serch the Object Id in the fbe_api_sp_board */
        for(port_num = 0 ; port_num < FBE_API_PHYSICAL_BUS_COUNT ; port_num++)
        {
            port_obj_list = &fbe_api_sp_board.port_object_list[port_num];

            for(encl_num = 0; encl_num < FBE_API_MAX_ALLOC_ENCL_PER_BUS ; encl_num++)
            {
                enclosure_obj_list = &(port_obj_list->enclosure_object_list[encl_num]);

                for(slot_num = 0; slot_num < FBE_API_ENCLOSURE_SLOTS  ; slot_num++)
                {
                    if(ldo_object)
                    {
                        /*we are suppose to scan through the lDO object_ids*/
                        obj_id_ptr_ldo = enclosure_obj_list->disk_fbe_object_list[slot_num].logical_disk.logical_disk_object.object_id;
                    }
                    if(pdo_object)
                    {
                        /*we are suppose to scan through the PDO object_ids*/
                        obj_id_ptr_pdo = enclosure_obj_list->disk_fbe_object_list[slot_num].physical_disk_object.object_id;
                    }

                    if((obj_id_ptr_ldo == object_id) || (obj_id_ptr_pdo == object_id))
                    {
                        *port_p = port_num;
                        *slot_p = slot_num;
                        *encl_p = encl_num;
                        return FBE_STATUS_OK;
                    }/*end if((obj_id_ptr_ldo == object_id) || (obj_id_ptr_pdo == object_id))*/
                }/*end for(slot_num = 0; slot_num < FBE_API_ENCLOSURE_SLOTS  ; slot_num++) */
            }/*end for(enc_num = ; enc_num < FBE_API_MAX_ALLOC_ENCL_PER_BUS ; enc_num++)*/
        }/* end for(port_num = 0 ; port_num < FBE_API_PHYSICAL_BUS_COUNT ; port_num ++) */
    }/* end else...*/

    /* If we are here means we did not find valid numbers */
    return FBE_STATUS_OK;
}
/************************************************************************************
*          end - fbe_api_object_map_get_port_enclosure_slot_number()
************************************************************************************/
