/***************************************************************************
 *  fbe_terminator.c
 ***************************************************************************
 *
 *  Description
 *      this is where the logic of the terminator seats
 *
 *
 *  History:
 *      06/12/08    sharel  Created
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_terminator.h"
#include "terminator_board.h"
#include "terminator_port.h"
#include "fbe_terminator_device_registry.h"
#include "terminator_enclosure.h"
#include "terminator_virtual_phy.h"
#include "terminator_drive.h"
#include "terminator_sas_io_api.h"
#include "terminator_sas_port.h"
#include "terminator_miniport_api_private.h"
#include "fbe_terminator_file_api.h"
#include "terminator_login_data.h"
#include "fbe_terminator_common.h"
#include "terminator_fc_port.h"
#include "terminator_fc_enclosure.h"
#include "terminator_fc_drive.h"
#include "fbe/fbe_file.h"
#include "EmcPAL_Memory.h"
#include "terminator_simulated_disk.h"
#include "fbe_terminator_persistent_memory.h"
#include "terminator_drive_sas_write_buffer_timer.h"

//#include <direct.h>

typedef struct terminator_endlosure_firmware_activate_context_s
{
    terminator_enclosure_firmware_new_rev_record_t new_rev_record;
    fbe_terminator_device_ptr_t virtual_phy_ptr;
    fbe_bool_t update_rev;
}terminator_endlosure_firmware_activate_context_t;

/* AR 551859: We will have more than one thread to call terminator_enclosure_firmware_activate()
 *  from port_io_thread_func(), so we should maintain a queue for all the enclosure_firmware_activate_thread
 *  elements.
 */
typedef struct terminator_encl_firmware_activate_thread_queue_element_s
{
    fbe_queue_element_t queue_element;
    fbe_thread_t        enclosure_firmware_activate_thread;
}terminator_encl_firmware_activate_thread_queue_element_t;

static fbe_spinlock_t   terminator_encl_firmware_activate_thread_queue_lock;
static fbe_queue_head_t terminator_encl_firmware_activate_thread_queue_head;
static fbe_bool_t       terminator_encl_firmware_activate_thread_started = FBE_FALSE;

#define TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN 64

/**********************************/
/*        local variables         */
/**********************************/
terminator_board_t *board_object;
fbe_terminator_io_mode_t terminator_io_mode;
fbe_bool_t  terminator_b_is_io_completion_at_dpc = FBE_FALSE;
static fbe_bool_t need_update_enclosure_firmware_rev = FBE_TRUE;
static fbe_bool_t need_update_enclosure_resume_prom_checksum = FBE_TRUE;

/******************************/
/*     local function         */
/*****************************/
static fbe_status_t terminator_activate_board(terminator_board_t * self);

/*********************************************************************
 *            terminator_init ()
 *********************************************************************
 *
 *  Description: initialize all internal data
 *
 *  Inputs:
 *
 *  Return Value: success or failure
 *
 *  History:
 *    6/12/08: sharel   created
 *
 *********************************************************************/
fbe_status_t terminator_init (void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    board_object = NULL;
    terminator_io_mode = FBE_TERMINATOR_IO_MODE_DISABLED;

    status = fbe_terminator_persistent_memory_init();
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_init();
    RETURN_ON_ERROR_STATUS;

    status = terminator_sas_enclosure_io_api_init();
    RETURN_ON_ERROR_STATUS;

    status = terminator_simulated_drive_class_init();
    RETURN_ON_ERROR_STATUS;

    status = terminator_sas_drive_write_buffer_timer_init();
    RETURN_ON_ERROR_STATUS;

    fbe_spinlock_init(&terminator_encl_firmware_activate_thread_queue_lock);
    fbe_queue_init(&terminator_encl_firmware_activate_thread_queue_head);

    //base_reset_device_id();
    status = FBE_STATUS_OK;
    return FBE_STATUS_OK;
}

fbe_status_t terminator_insert_board (fbe_terminator_board_info_t *board_info)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_terminator_board_info_t *attributes = NULL;

    board_object = board_new();
    //base_component_assign_id((base_component_t *)board_object, base_generate_device_id());
    base_component_set_component_type(&board_object->base, TERMINATOR_COMPONENT_TYPE_BOARD);
    attributes = board_info_new(board_info);
    base_component_assign_attributes(&board_object->base, attributes);
    //base_reset_device_id();
    return status;
}
fbe_status_t terminator_remove_board (void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    if (board_object == NULL)
    {
        return status;
    }

    for (index = 0; index < FBE_PMC_SHIM_MAX_PORTS; index++)
    {
        status = fbe_terminator_miniport_api_remove_port(index);
        if (status == FBE_STATUS_OK)
        {   /* now start the thread to do the logout callback */
#if 0 /* SAFEBUG - we already do this in fbe_terminator_miniport_api_remove_port... don't we? */
            fbe_terminator_miniport_api_device_logged_out (index, 0);
#endif
        }

#if 0 /* SAFEBUG - we can end up destroying semaphore without cleaning up the threads */
        fbe_terminator_miniport_api_clean_sempahores(index);
#endif
    }

    /* destroy the board now */
    status = board_destroy(board_object);
    board_object = NULL;
    return status;
}

//fbe_status_t terminator_insert_sas_port (fbe_u32_t port_number, fbe_terminator_sas_port_info_t *port_info)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    terminator_port_t * matching_port = NULL;
//    terminator_port_t *new_port = NULL;
//    terminator_sas_port_info_t *attributes = NULL;
//    fbe_queue_element_t * queue_element = NULL;
//
//    /*Check if the board object exist */
//    if (board_object == NULL)
//    {
//        return status;
//    }
//
//    /* Check if there is a port with same number exist already. If so return error */
//    matching_port = (terminator_port_t *)base_component_get_child_by_id((base_component_t *)board_object, port_number);
//    if (matching_port != NULL)
//    {
//        return status;
//    }
//    new_port = port_new();
//    //base_component_assign_id((base_component_t *)new_port, port_number);
//    base_component_set_component_type(&new_port->base, TERMINATOR_COMPONENT_TYPE_PORT);
//    port_set_type(new_port, port_info->port_type);
//    status = port_set_miniport_port_index(new_port, port_number);
//
//    /* Create a sas port info block and attach it to the attribute,
//     * when accessing the info, take it out from attribute */
//    attributes = sas_port_info_new(port_info);
//
//    base_component_assign_attributes(&new_port->base, attributes);
//    /* Add this port to the board's child_list */
//    queue_element = base_component_get_queue_element(&new_port->base);
//    base_component_add_child(&board_object->base, queue_element);
//    return status;
//}

/*this function inserts an enclosure and returns it's id for future use by the user*/
//fbe_status_t terminator_insert_sas_enclosure (fbe_u32_t port_number, fbe_terminator_sas_encl_info_t *encl_info, fbe_terminator_device_ptr_t *encl_ptr)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    terminator_port_t * matching_port = NULL;
//    terminator_enclosure_t * new_enclosure = NULL;
//    terminator_sas_enclosure_info_t *attributes = NULL;
//    base_component_t * enclosure_base = NULL;
//    fbe_cpd_shim_callback_login_t * login_data = NULL;
//    /* get the port object from the board child list */
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//
//    /* Let's create an enclosure object */
//    new_enclosure = enclosure_new();
//    enclosure_base = (base_component_t *)new_enclosure;
//    *encl_ptr = new_enclosure;
////  *encl_id = base_generate_device_id();
////  base_component_assign_id(enclosure_base, *encl_id);
//    /* Create a sas enclosure info block and attach it to the attribute*/
//    attributes = sas_enclosure_info_new(encl_info);
//    base_component_assign_attributes(enclosure_base, attributes);
//    login_data = sas_enclosure_info_get_login_data(attributes);
//
//
//    /* Now set the enclsoure type with the user given type and UID */
//    sas_enclosure_set_enclosure_type(new_enclosure, encl_info->encl_type);
//    sas_enclosure_set_enclosure_uid(new_enclosure, encl_info->uid);
//    /* Now, ask the port to add this enclosure */
//    status = port_add_sas_enclosure(matching_port, new_enclosure);
//
//    /* Set the login_pending flag for this enclosure and it's virtual phy,
//    so the miniport api thread will pick it up and log them in */
//    status = terminator_set_device_login_pending(new_enclosure);
//    if (status != FBE_STATUS_OK)
//    {
//        //terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set_login_pending failed on device 0x%p!\n", __FUNCTION__, new_enclosure);
//    }
//    return status;
//}

fbe_status_t terminator_get_board_type(fbe_board_type_t *board_type)
{
    if (board_object == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *board_type = get_board_type(board_object);

    return FBE_STATUS_OK;
}

fbe_status_t terminator_get_board_info(fbe_terminator_board_info_t *board_info)
{
    fbe_terminator_board_info_t *temp_board_info;

    if (board_object == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    temp_board_info = get_board_info(board_object);

    fbe_copy_memory(board_info, temp_board_info, sizeof(fbe_terminator_board_info_t));

    return FBE_STATUS_OK;
}

/*
 * This function is used to get and remove a stored specl sfi mask data from queue in board_infol.
 * Note that you may need to call this function several times until it returns FBE_STATUS_GENERIC_FAILURE to get all mask data in the queue.
 */
fbe_status_t terminator_pop_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA specl_sfi_mask_data_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if(board_object == NULL || specl_sfi_mask_data_ptr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = pop_specl_sfi_mask_data(board_object, specl_sfi_mask_data_ptr);

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: specl_sfi_mask_data status: 0x%X struct_number: 0x%X\n",
                     __FUNCTION__, specl_sfi_mask_data_ptr->maskStatus, specl_sfi_mask_data_ptr->structNumber);

    return status;
}

fbe_status_t terminator_push_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA specl_sfi_mask_data_ptr)
{
    if (board_object == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return push_specl_sfi_mask_data(board_object, specl_sfi_mask_data_ptr);
}

fbe_status_t terminator_get_specl_sfi_mask_data_queue_count(fbe_u32_t *cnt)
{
    if (board_object == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return get_specl_sfi_mask_data_queue_count(board_object, cnt);
}

fbe_status_t terminator_enumerate_ports (fbe_terminator_port_shim_backend_port_info_t port_list[], fbe_u32_t *total_ports)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_terminator_device_ptr_t      backend_port_list[FBE_PMC_SHIM_MAX_PORTS];
    fbe_u32_t       index = 0;
    terminator_port_t * matching_port = NULL;
    fbe_port_type_t port_type;

    *total_ports = board_list_backend_port_number(board_object, backend_port_list);
    /* fill in the attributes of the port */
    for (index = 0; index < *total_ports ; index++)
    {
       matching_port = backend_port_list[index];
       if (matching_port != NULL)
       {
            port_type = port_get_type(matching_port);
            switch(port_type)
            {
                case FBE_PORT_TYPE_SAS_PMC:
                    status = port_get_sas_port_attributes(matching_port, &port_list[index]);
                    break;
                case FBE_PORT_TYPE_ISCSI: /* TODO: Implement later.*/
                case FBE_PORT_TYPE_FC_PMC:
                case FBE_PORT_TYPE_FCOE:
                    status = port_get_fc_port_attributes(matching_port, &port_list[index]);
                    break;
            }
       }
    }
    return status;
}

fbe_status_t terminator_enumerate_devices (fbe_terminator_device_ptr_t handle, fbe_terminator_device_ptr_t device_list[], fbe_u32_t *total_devices)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /*return all device id's on this port into the array and fills total_devices
    we assume the array is big enough*/

    if (handle == NULL)
    {
        return status;
    }
    status = port_enumerate_devices(handle, device_list, total_devices);
    return status;
}

fbe_status_t terminator_get_device_login_data (fbe_terminator_device_ptr_t device_ptr, fbe_cpd_shim_callback_login_t *login_data)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    status = get_device_login_data(device_ptr, login_data);
    return status;
}

fbe_status_t terminator_get_port_type (fbe_terminator_device_ptr_t handle, fbe_port_type_t *port_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL)
    {
        return status;
    }
    *port_type = port_get_type(handle);
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_get_port_address (fbe_terminator_device_ptr_t handle, fbe_address_t *port_address)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_port_type_t port_type;

    if (handle == NULL)
    {
        return status;
    }
    port_type = port_get_type(handle);
    switch (port_type)
    {
    case FBE_PORT_TYPE_SAS_LSI:
    case FBE_PORT_TYPE_SAS_PMC:
        port_address->sas_address = sas_port_get_address(handle);
        status = FBE_STATUS_OK;
        break;
     case FBE_PORT_TYPE_FC_PMC:
        port_address->diplex_address= fc_port_get_address(handle);
        status = FBE_STATUS_OK;
        break;
    case FBE_PORT_TYPE_ISCSI:
    case FBE_PORT_TYPE_FCOE:
         /* TODO: Implement later*/
         status = FBE_STATUS_OK;
         break;
    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s address not support for port type %d\n", __FUNCTION__, port_type);
    }
    return status;
}

fbe_status_t terminator_get_hardware_info (fbe_terminator_device_ptr_t handle, fbe_cpd_shim_hardware_info_t *hardware_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = port_get_hardware_info(handle, hardware_info);
    return status;
}

fbe_status_t terminator_register_keys (fbe_terminator_device_ptr_t handle, fbe_base_port_mgmt_register_keys_t * port_register_keys_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = port_register_keys(handle, port_register_keys_p);
    return status;
}

fbe_status_t terminator_reestablish_key_handle(fbe_terminator_device_ptr_t handle, 
                                               fbe_base_port_mgmt_reestablish_key_handle_t * port_reestablish_key_handle_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = port_reestablish_key_handle(handle, port_reestablish_key_handle_p);
    return status;
}
fbe_status_t terminator_unregister_keys (fbe_terminator_device_ptr_t handle, fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = port_unregister_keys(handle, port_unregister_keys_p);
    return status;
}

fbe_status_t terminator_get_sfp_media_interface_info (fbe_terminator_device_ptr_t handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = port_get_sfp_media_interface_info(handle, sfp_media_interface_info);
    return status;
}

fbe_status_t terminator_get_port_link_info (fbe_terminator_device_ptr_t handle, fbe_cpd_shim_port_lane_info_t *port_link_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = port_get_port_link_info(handle, port_link_info);
    return status;
}

fbe_status_t terminator_get_port_configuration (fbe_terminator_device_ptr_t handle,  fbe_cpd_shim_port_configuration_t *port_config_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = port_get_port_configuration(handle, port_config_info);
    return status;
}



fbe_status_t terminator_is_in_logout_state (fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *logout_state)
{
    terminator_component_state_t component_state;
    if (device_ptr ==NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    component_state = base_component_get_state(device_ptr);
    if ((component_state == TERMINATOR_COMPONENT_STATE_LOGOUT_PENDING)||
        (component_state == TERMINATOR_COMPONENT_STATE_LOGOUT_COMPLETE))
    {
        *logout_state = FBE_TRUE; 
    }
    else
    {
        *logout_state = FBE_FALSE; 
    }
    return FBE_STATUS_OK;
}


fbe_status_t terminator_is_login_pending (fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *pending)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (device_ptr == NULL)
    {
        return  FBE_STATUS_NO_DEVICE;
    }
    *pending = base_component_get_login_pending(device_ptr);
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_clear_login_pending (fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (device_ptr == NULL)
    {
        return status;
    }
    base_component_set_login_pending(device_ptr, FBE_FALSE);
    status = FBE_STATUS_OK;
    return status;
}
fbe_status_t terminator_set_device_logout_pending (fbe_terminator_device_ptr_t device_ptr)
{
    /* find the devices to logout and add them to logout queue, but do not remove them from the tree */
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;

    status = terminator_get_port_ptr(device_ptr, &port_ptr);
    if ((status != FBE_STATUS_OK)||(port_ptr == NULL))
    {
    	/* Did not find a port, return a failure status */
		status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }

    /* take the device and any of its child devices for the tree and add them to port logout queue */
    status = port_logout_device(port_ptr, device_ptr);
    return status;
}


fbe_status_t terminator_remove_device_from_miniport_api_device_table (fbe_u32_t port_number, fbe_terminator_device_ptr_t device_ptr)
{
    /* find the devices to remove and remove them one at a time, but do not remove them from the tree */
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t   index;

    /*get the table index*/
    status = terminator_get_miniport_sas_device_table_index(device_ptr, &index);
    /*remove the entry*/
    status = fbe_terminator_miniport_api_device_table_remove (port_number, index);
    return status;
}

fbe_status_t terminator_destroy(void)
{
    fbe_status_t    status = FBE_STATUS_OK;

    status = terminator_sas_drive_write_buffer_timer_destroy();
    RETURN_ON_ERROR_STATUS;

    status = terminator_remove_board();
    RETURN_ON_ERROR_STATUS;

    terminator_simulated_drive_class_destroy();

    status = fbe_terminator_persistent_memory_destroy();
    RETURN_ON_ERROR_STATUS;

    fbe_spinlock_destroy(&terminator_encl_firmware_activate_thread_queue_lock);
    fbe_queue_destroy(&terminator_encl_firmware_activate_thread_queue_head);

    return status;
}

fbe_status_t terminator_set_speed (fbe_terminator_device_ptr_t handle, fbe_port_speed_t speed)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (handle == NULL)
    {
        return status;
    }
    status = port_set_speed(handle, speed);
    return status;
}

fbe_status_t terminator_get_port_info (fbe_terminator_device_ptr_t handle, fbe_port_info_t * port_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (handle == NULL)
    {
        return status;
    }
    port_info->link_speed = port_get_speed(handle);
    port_info->maximum_transfer_bytes = port_get_maximum_transfer_bytes(handle);
    port_info->maximum_sg_entries = port_get_maximum_sg_entries(handle);
    port_info->enc_mode = FBE_PORT_ENCRYPTION_MODE_WRAPPED_KEKS_DEKS;
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_destroy_all_devices(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * enclosure_base = NULL;
    //fbe_u32_t miniport_port_index;

    if (handle == NULL)
    {
        /* Did not find a port with matching port number, return a failure status */
        return status;
    }

    if (base_component_get_child_count(handle) != 0)
    {
        /* port should only have one enclosure attached to it, so find it and destroy the enclosure
         * and all it's children */
        /* miniport api should logout all the devices on the logout queue */
        enclosure_base = base_component_get_first_child(handle);
        if(enclosure_base != NULL)
        {
            status = port_destroy_device(handle, enclosure_base);
        }
    }
    //port_get_miniport_port_index(handle, &miniport_port_index);
    //status = termiantor_destroy_disk_files(miniport_port_index);
    /*destroy the port object itself */
    status = port_destroy(handle);
    return status;
}

fbe_status_t terminator_get_device_type (fbe_terminator_device_ptr_t device_ptr, terminator_component_type_t *device_type)
{
    /*return the device type per id*/
    *device_type = base_component_get_component_type((base_component_t*)device_ptr);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_send_io(fbe_terminator_device_ptr_t handle, fbe_terminator_io_t *terminator_io)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_handle;

    if (handle == NULL)
    {
        /* Did not find a port with matching port number, return a failure status */
        return status;
    }

    /* add the device to the port io queue, so the sequence of the device is remembered */
    terminator_get_port_ptr(handle, &port_handle);

    /* If port handle is NULL, just exit with the status */
    if (port_handle == NULL)
    {
        return status;
    }

    status = port_enqueue_io(port_handle, handle, &terminator_io->queue_element);
    return status;
}

//fbe_status_t terminator_set_login_pending (fbe_u32_t port_number, fbe_terminator_device_ptr_t device_ptr)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    terminator_port_t * matching_port = NULL;
//
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//
//    /* 1)find the device
//     * 2)clear itself and all it's children's logout_complete
//     * 3)set itself and all it's children's login_pending
//     * 4)miniport_api event thread will pick the device we marked and log them in.
//     * a timing issue can occur herer.  miniport api thread can be in the middle of checking the device list
//     * and we started changing the login_pending.  Therefore, we should lock the port, and miniport_api
//     * should lock the port during enum_devices
//     */
//    status = port_login_device(matching_port, device_ptr);
//    return status;
//}
fbe_status_t terminator_set_device_login_pending (fbe_terminator_device_ptr_t component_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_handle;
    /* 1)find the device
     * 2)clear itself and all it's children's logout_complete
     * 3)set itself and all it's children's login_pending
     * 4)miniport_api event thread will pick the device we marked and log them in.
     * a timing issue can occur herer.  miniport api thread can be in the middle of checking the device list
     * and we started changing the login_pending.  Therefore, we should lock the port, and miniport_api
     * should lock the port during enum_devices
     */
    status = terminator_get_port_ptr(component_handle, &port_handle);
    status = port_login_device(port_handle, component_handle);
    return status;
}

/*
 * The following functions gets the device id at the queue front
 * of the LOGOUT QUEUE, which has the device ids of the
 * devices waiting to be logged out.
 */
fbe_status_t terminator_get_front_device_from_logout_queue(fbe_terminator_device_ptr_t port_ptr,
                                                           fbe_terminator_device_ptr_t *device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (port_ptr == NULL)
    {
        return status;
    }
    status = port_front_logout_device(port_ptr, device_ptr);
    return status;
}

/*
 * The following function pops the device at the queue front
 * of the LOGOUT QUEUE, which has the device ids of the
 * devices waiting to be logged out.
 */
fbe_status_t terminator_pop_device_from_logout_queue(fbe_terminator_device_ptr_t handle,
                                                     fbe_terminator_device_ptr_t *device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL)
    {
        return status;
    }
    status = port_pop_logout_device(handle, device_id);
    return status;
}
/*
 * The following function gets the next device from logout queue
 */
fbe_status_t terminator_get_next_device_from_logout_queue(fbe_terminator_device_ptr_t handle,
                                                          fbe_terminator_device_ptr_t device_id,
                                                          fbe_terminator_device_ptr_t *next_device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL)
    {
        return status;
    }
    status = port_get_next_logout_device(handle, device_id, next_device_id);
    return status;
}
/*
 * The following  function sets  the logout_complete flag for a given
 * device.
 */
fbe_status_t terminator_set_device_logout_complete(fbe_terminator_device_ptr_t device_ptr, fbe_bool_t flag)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (device_ptr == NULL)
    {
        return status;
    }
    base_component_set_logout_complete(device_ptr, flag);
    status = FBE_STATUS_OK;
    return status;
}

/*
 * The following  function gets  the logout_complete flag for a given
 * device.
 */
fbe_status_t terminator_get_device_logout_complete(fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *flag)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (device_ptr == NULL)
    {
        return status;
    }
    *flag = base_component_get_logout_complete(device_ptr);
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_destroy_device(fbe_terminator_device_ptr_t device_ptr)
{
    /*destroy the devices from bottom all the way to the device*/
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;

    status = terminator_get_port_ptr(device_ptr, &port_ptr);
    if (status != FBE_STATUS_OK)
    {
        /* Did not find a port, return a failure status */
        return status;
    }
    status = port_destroy_device(port_ptr, device_ptr);

    return status;
}

fbe_status_t terminator_unmount_device(fbe_terminator_device_ptr_t device_ptr)
{
    /*remove the devices from bottom all the way to the device*/
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;

    status = terminator_get_port_ptr(device_ptr, &port_ptr);
    if (status != FBE_STATUS_OK)
    {
        /* Did not find a port, return a failure status */
        return status;
    }
    status = port_unmount_device(port_ptr, device_ptr);

    return status;
}

/* This function sets up 4 bytes eses status element */
fbe_status_t terminator_set_drive_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u32_t slot_number,
    ses_stat_elem_array_dev_slot_struct ses_stat_elem)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* The port configuration is locked so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_drive_eses_status(virtual_phy_handle, slot_number, ses_stat_elem);

    return status;
}
/* This function gets the 4 bytes eses status element */
fbe_status_t terminator_get_drive_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u32_t slot_number,
    ses_stat_elem_array_dev_slot_struct *ses_stat_elem)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* The port configuration is locked so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_drive_eses_status(virtual_phy_handle, slot_number, ses_stat_elem);

    return status;
}

/**************************************************************************
 *  terminator_insert_sas_drive()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    Terminator interface for inserting a SAS drive with the given
 *    properties/parameters.
 *
 *  PARAMETERS:
 *      port number, device id of the enclosure, drive slot number in the
 *      enclosure, Info of the SAS drive to be inserted, device ID of the
 *      inserted drive to be returned.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      ALGORITHM:
 *
 *      When Terminator Initializes "aka" EMA boots UP --->
 *       All drive slots will be "POWERED ON"
 *           // i.e. "DEVICE OFF" bit in Array device slot status element is NOT SET.
 *       ALL Drive Slot Status will be "NOT INSTALLED".
 *       ALL PHY STATUS WILL BE "OK".
 *       "Phy ready" bit in PHY Status is set to false.
 *
 *      WHEN DRIVE IS INSERTED ---->
 *          Slot Status is SET to OK.
 *          Corresponding PHY status NOT CHANGED.
 *          IF (("DEVICE OFF" BIT is NOT SET in the Existing Array Device Slot Status element)
 *           AND (PHY STATUS IS OK)). // meaning drive slot can be powered on && Phy enabled
 *               GENERATE LOGIN
 *               Set Phy ready bit to true.
 *          ENDIF
 *
 *  HISTORY:
 *      Created
 **************************************************************************/
fbe_status_t terminator_insert_sas_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sas_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    terminator_port_t * matching_port = NULL;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct exp_phy_stat;
    base_component_t *virtual_phy = NULL;
    fbe_u8_t phy_id;
    fbe_sas_enclosure_type_t encl_type;
    fbe_bool_t generate_drive_login = FBE_FALSE;

    /* 1) add the drive to lot */
    status = terminator_create_sas_drive(encl_ptr, slot_number, drive_info, drive_ptr);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* 2) set the drive insert-bit in enclosure */
    /* make sure this is an enclosure */
    if (base_component_get_component_type(encl_ptr)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return status;
    }

    // Get the related virtual phy  of the enclosure
    virtual_phy =
        base_component_get_child_by_type(encl_ptr, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    if(virtual_phy == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    // update if needed
    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t *)&virtual_phy, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    //fbe_mutex_lock(&matching_port->update_mutex);

    // Get the current ESES status of the drive slot & set the status code to OK
    status = sas_virtual_phy_get_drive_eses_status((terminator_virtual_phy_t *)virtual_phy,
        slot_number, &drive_slot_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_eses_status failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = sas_virtual_phy_set_drive_eses_status((terminator_virtual_phy_t *)virtual_phy,
        slot_number, drive_slot_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_set_drive_eses_status failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy, &encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }

    // Get the corresponding PHY status
    status = sas_virtual_phy_get_drive_slot_to_phy_mapping(slot_number, &phy_id, encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_slot_to_phy_mapping failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }
    status = sas_virtual_phy_get_phy_eses_status((terminator_virtual_phy_t *)virtual_phy, phy_id, &exp_phy_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set sas_virtual_phy_get_phy_eses_status failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }

    // Login is generated only if the below conditions are statisfied.
    if((!drive_slot_stat.dev_off) &&
       (exp_phy_stat.cmn_stat.elem_stat_code == SES_STAT_CODE_OK))
    {
        //Set the "Phy Ready" bit and generate login for the drive
        exp_phy_stat.phy_rdy = FBE_TRUE;
        status = sas_virtual_phy_set_phy_eses_status((terminator_virtual_phy_t *)virtual_phy, phy_id, exp_phy_stat);
        if (status != FBE_STATUS_OK)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set sas_virtual_phy_set_phy_eses_status failed!\n", __FUNCTION__);
            //fbe_mutex_unlock(&matching_port->update_mutex);
            return status;
        }
        generate_drive_login = FBE_TRUE;
    }

    //fbe_mutex_unlock(&matching_port->update_mutex);
    /* 3) set the login_pending flag, terminator api will wake up the mini_port_api thread to process the login */
    if(generate_drive_login)
    {
        status = terminator_set_device_login_pending(*drive_ptr);
        if (status != FBE_STATUS_OK)
        {
            //terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set_login_pending failed on device %d!\n", __FUNCTION__, *drive_ptr);
        }
    }
    return status;
}

fbe_status_t fbe_terminator_reinsert_drive(fbe_terminator_api_device_handle_t enclosure_handle,
                                          fbe_u32_t slot_number,
                                          fbe_terminator_api_device_handle_t drive_handle)
{
    tdr_device_ptr_t drive_ptr, encl_ptr;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    tdr_status_t tdr_status = TDR_STATUS_GENERIC_FAILURE;
    char buffer[TERMINATOR_DEVICE_ATTRIBUTE_BUFFER_LEN];
    terminator_sas_drive_info_t *attributes = NULL;

    /* Backend number */
    memset(buffer, 0, sizeof(buffer));
    status = fbe_terminator_api_get_device_attribute(enclosure_handle, "Backend_number", buffer, sizeof(buffer));
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s get enclosure backend failed. status: 0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_terminator_api_set_device_attribute(drive_handle, "Backend_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s set drive backend failed. status: 0x%x\n", __FUNCTION__, status);
        return status;
    }

    /* Enclosure number */
    memset(buffer, 0, sizeof(buffer));
    status = fbe_terminator_api_get_device_attribute(enclosure_handle, "Enclosure_number", buffer, sizeof(buffer));
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s get enclosure failed. status: 0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_terminator_api_set_device_attribute(drive_handle, "Enclosure_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s set drive enclosure failed. status: 0x%x\n", __FUNCTION__, status);
        return status;
    }

    /*Slot number */
    memset(buffer, 0, sizeof(buffer));
    _snprintf(buffer, sizeof(buffer), "%u", slot_number);
    status = fbe_terminator_api_set_device_attribute(drive_handle, "Slot_number", buffer);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s set drive slot failed. status: 0x%x\n", __FUNCTION__, status);
        return status;
    }

    tdr_status = fbe_terminator_device_registry_get_device_ptr(drive_handle, &drive_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s get drive registry failed. status: 0x%x\n", __FUNCTION__, tdr_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &encl_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s get drive pointer failed. status: 0x%x\n", __FUNCTION__, tdr_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_insert_device(encl_ptr, drive_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s insert device failed. status: 0x%x\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    attributes = (terminator_sas_drive_info_t *)base_component_get_attributes((base_component_t *)drive_ptr);
    attributes->miniport_sas_device_table_index = INVALID_TMSDT_INDEX; // need a new cpd_device_id

    // active the drive
    status = terminator_activate_device(drive_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s activate device failed. status: 0x%x\n", __FUNCTION__, status);
    }
    return status;
}

/**************************************************************************
 *  fbe_terminator_remove_drive()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    Terminator interface for removing a SAS drive.
 *
 *  PARAMETERS:
 *      port number and device ID of the drive.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *   ALGORITHM
 *
 *   WHEN DRIVE IS REMOVED
 *      Set slot Status to "NOT INSTALLED ".
 *      Set Phy ready bit to  false.
 *      if the Phy element status code is NOT SES_STAT_CODE_UNAVAILABLE
 *          change Phy element status code to SES_STAT_CODE_OK
 *      IF (("DEVICE OFF" BIT is NOT SET in the Existing Array Device Slot Status element)
 *       AND (DRIVE STILL LOGGED IN)).  // As drive could have been already logged out because the phy was disabled using ctrl page.
 *          Generate Logout for the drive
 *      ENDIF
 *
 *  HISTORY:
 *      Created
 **************************************************************************/
fbe_status_t fbe_terminator_remove_drive(fbe_terminator_device_ptr_t drive_ptr)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_terminator_device_ptr_t virtual_phy_handle;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct phy_stat;
    fbe_u32_t slot_number;
    fbe_u8_t phy_id;
    fbe_sas_enclosure_type_t encl_type;
    fbe_bool_t generate_drive_logout = FBE_FALSE, drive_logged_in = FBE_FALSE;

    status = terminator_get_drive_slot_number(drive_ptr, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s terminator_get_drive_slot_number failed\n", __FUNCTION__);
        return status;
    }
    //Get the virtual phy handle.
    status = terminator_get_drive_parent_virtual_phy_ptr(drive_ptr, &virtual_phy_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }
    // update if needed
    status = terminator_update_drive_parent_device(&virtual_phy_handle, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
        return status;
    }

    // Set the status code of the Drive slot ESES status to not installed.
    status = sas_virtual_phy_get_drive_eses_status(virtual_phy_handle,
        slot_number, &drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = sas_virtual_phy_set_drive_eses_status(virtual_phy_handle,
        slot_number, drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    // Get the Corresponding PHY eses status and clear the PHY RDY bit.
    status = sas_virtual_phy_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, &phy_id, encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    status = sas_virtual_phy_get_phy_eses_status(virtual_phy_handle,
        phy_id, &phy_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    if(phy_stat.cmn_stat.elem_stat_code != SES_STAT_CODE_UNAVAILABLE)
    {
        phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    }
    phy_stat.phy_rdy = FBE_FALSE;
    status = sas_virtual_phy_set_phy_eses_status(virtual_phy_handle,
        phy_id, phy_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    // Get the current LOGIN state of the drive
    status = terminator_virtual_phy_is_drive_in_slot_logged_in(virtual_phy_handle,
                                                                slot_number,
                                                                &drive_logged_in);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    if((!drive_slot_stat.dev_off) &&
       (drive_logged_in))
    {
        generate_drive_logout = FBE_TRUE;
    }

    if(generate_drive_logout)
    {
        // Generate a logout and destroy the drive object
        status = fbe_terminator_remove_device(drive_ptr);
    }
    else
    {
        // Just destroy the drive object without generating a logout.
        status = terminator_destroy_device(drive_ptr);
    }
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    return(status);
}

/**************************************************************************
 *  fbe_terminator_pull_drive()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    Terminator interface for pulling a drive.
 *
 *  PARAMETERS:
 *      pointer of the drive.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  HISTORY:
 *      Created
 **************************************************************************/
fbe_status_t fbe_terminator_pull_drive(fbe_terminator_device_ptr_t drive_ptr)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_terminator_device_ptr_t virtual_phy_handle;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct phy_stat;
    fbe_u32_t slot_number;
    fbe_u8_t phy_id;
    fbe_sas_enclosure_type_t encl_type;
    fbe_bool_t generate_drive_logout = FBE_FALSE, drive_logged_in = FBE_FALSE;
    fbe_u32_t  port_index;

    status = terminator_get_drive_slot_number(drive_ptr, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s terminator_get_drive_slot_number failed\n", __FUNCTION__);
        return status;
    }
    //Get the virtual phy handle.
    status = terminator_get_drive_parent_virtual_phy_ptr(drive_ptr, &virtual_phy_handle);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s terminator_get_enclosure_virtual_phy_ptr failed\n", __FUNCTION__);
        return(status);
    }

    // Set the status code of the Drive slot ESES status to not installed.
    status = sas_virtual_phy_get_drive_eses_status(virtual_phy_handle,
        slot_number, &drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_NOT_INSTALLED;
    status = sas_virtual_phy_set_drive_eses_status(virtual_phy_handle,
        slot_number, drive_slot_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy_handle, &encl_type);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    // Get the Corresponding PHY eses status and clear the PHY RDY bit.
    status = sas_virtual_phy_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, &phy_id, encl_type);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    status = sas_virtual_phy_get_phy_eses_status(virtual_phy_handle,
        phy_id, &phy_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    if(phy_stat.cmn_stat.elem_stat_code != SES_STAT_CODE_UNAVAILABLE)
    {
        phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    }
    phy_stat.phy_rdy = FBE_FALSE;
    status = sas_virtual_phy_set_phy_eses_status(virtual_phy_handle,
        phy_id, phy_stat);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    // Get the current LOGIN state of the drive
    status = terminator_virtual_phy_is_drive_in_slot_logged_in(virtual_phy_handle,
                                                                slot_number,
                                                                &drive_logged_in);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    if((!drive_slot_stat.dev_off) &&
       (drive_logged_in))
    {
        generate_drive_logout = FBE_TRUE;
    }

    if(generate_drive_logout)
    {
        status = terminator_get_port_index(drive_ptr, &port_index);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        /* remove from the device table */
        status = terminator_remove_device_from_miniport_api_device_table (port_index, drive_ptr);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
             "%s: Remove device 0x%p from port 0x%X miniport device table failed.\n",
             __FUNCTION__, drive_ptr, port_index);
            return status;
        }
        /* remove all the children from the device table, if any */
        status = base_component_remove_all_children_from_miniport_api_device_table(drive_ptr, port_index);
        if (status != FBE_STATUS_OK) {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
             "%s: Remove children of device 0x%p from port 0x%X miniport device table failed.\n",
             __FUNCTION__, drive_ptr, port_index);
            return status;
        }
        // Generate a logout and unmount the drive object
        status = fbe_terminator_unmount_device(drive_ptr);
    }
    else
    {
        // Just unmount the drive object without generating a logout.
        status = terminator_unmount_device(drive_ptr);
    }
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    return(status);
}

fbe_status_t fbe_terminator_remove_device (fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t          complete = FBE_FALSE;
    fbe_u32_t           port_index;
    fbe_terminator_device_ptr_t port_ptr;

    status = terminator_get_port_index(device_ptr, &port_index);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* remove from the device table */
    status = terminator_remove_device_from_miniport_api_device_table (port_index, device_ptr);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
         "%s: Remove device 0x%p from port 0x%X miniport device table failed.\n",
         __FUNCTION__, device_ptr, port_index);
        return status;
    }
    /* remove all the children from the device table, if any */
    status = base_component_remove_all_children_from_miniport_api_device_table(device_ptr, port_index);
    if (status != FBE_STATUS_OK) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
         "%s: Remove children of device 0x%p from port 0x%X miniport device table failed.\n",
         __FUNCTION__, device_ptr, port_index);
        return status;
    }
    /*we set the logout flag and let the thread do the rest*/
    status = terminator_set_device_logout_pending(device_ptr);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    status = fbe_terminator_miniport_api_device_state_change_notify(port_index);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = terminator_get_port_ptr(device_ptr, &port_ptr);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_terminator_wait_on_port_logout_complete(port_ptr);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* make sure the device is logout complete */
    while(complete != FBE_TRUE)
    {
        status = terminator_get_device_logout_complete(device_ptr, &complete);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }
    /* Now, destroy the devices */
    status = terminator_destroy_device (device_ptr);
    return status;
}

fbe_status_t fbe_terminator_unmount_device (fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t          complete = FBE_FALSE;
    fbe_u32_t           port_index;
    fbe_terminator_device_ptr_t port_ptr;

    /*we set the logout flag and let the thread do the rest*/
    status = terminator_set_device_logout_pending(device_ptr);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    status = terminator_get_port_index(device_ptr, &port_index);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    status = fbe_terminator_miniport_api_device_state_change_notify(port_index);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = terminator_get_port_ptr(device_ptr, &port_ptr);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_terminator_wait_on_port_logout_complete(port_ptr);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* make sure the device is logout complete */
    while(complete != FBE_TRUE)
    {
        status = terminator_get_device_logout_complete(device_ptr, &complete);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
    }

    /* Now, unmount the devices */
    status = terminator_unmount_device (device_ptr);
    return status;
}

fbe_status_t terminator_create_sas_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sas_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t is_slot_available = FBE_FALSE;

    if (encl_ptr == NULL)
    {
        return status;
    }
    /* make sure this is an enclosure */
    if (base_component_get_component_type(encl_ptr)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return status;
    }

    /* check if the slot number is valid and not in use */
    status = sas_enclosure_is_slot_available((terminator_enclosure_t *)encl_ptr, slot_number, &is_slot_available);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    if(is_slot_available != FBE_TRUE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = sas_enclosure_insert_sas_drive((terminator_enclosure_t *)encl_ptr, slot_number, drive_info, drive_ptr);
    return status;
}

/* abort the IOs on this device*/
fbe_status_t terminator_abort_all_io(fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;

    if (device_ptr == NULL)
    {
        /* the device ptr is invalid, return a failure status */
        return status;
    }
    status = terminator_get_port_ptr(device_ptr, &port_ptr);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Mark IO abort */
    status = port_abort_device_io(port_ptr, device_ptr);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_get_port_device_id(fbe_u32_t port_number,
                                               fbe_terminator_api_device_handle_t *device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//  terminator_port_t * matching_port = NULL;
//
//  matching_port = board_get_port_by_number(board_object, port_number);
//  if (matching_port == NULL)
//  {
//      /* Did not find a port with matching port number, return a failure status */
//      *device_id = DEVICE_HANDLE_INVALID;
//      return status;
//  }
//  *device_id = sas_port_get_device_id(matching_port);
//  status = FBE_STATUS_OK;
    return status;
}
//fbe_status_t terminator_get_sas_enclosure_device_id(fbe_u32_t port_number,
//                                                    fbe_u8_t enclosure_chain_depth,
//                                                    fbe_terminator_api_device_handle_t *device_id)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * matching_device = NULL;
//    terminator_port_t * matching_port = NULL;
//
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        /* Did not find a port with matching port number, return a failure status */
//        *device_id = DEVICE_HANDLE_INVALID;
//        return status;
//    }
//    matching_device = port_get_enclosure_by_chain_depth(matching_port, enclosure_chain_depth);
//    if (matching_device == NULL)
//    {
//        /* Did not find matching enclosure, return a failure status */
//        *device_id = DEVICE_HANDLE_INVALID;
//        return status;
//    }
//    *device_id = base_component_get_id(matching_device);
//    status = FBE_STATUS_OK;
//    return status;
//}

fbe_status_t terminator_get_sas_drive_device_ptr(fbe_terminator_device_ptr_t encl_ptr, fbe_u32_t slot_number, fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (encl_ptr == NULL)
    {
        *drive_ptr = NULL;
        return status;
    }
    /* make sure this is an enclosure */
    if (base_component_get_component_type(encl_ptr)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        *drive_ptr = NULL;
        return status;
    }

    status = sas_enclosure_get_device_ptr_by_slot((terminator_enclosure_t *)encl_ptr, slot_number, drive_ptr);

    return status;
}

fbe_status_t terminator_get_drive_slot_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *slot_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /* make sure this is an drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }
    switch(drive_get_protocol((terminator_drive_t *)device_ptr))
    {
    case FBE_DRIVE_TYPE_SAS:
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        *slot_number = sas_drive_get_slot_number((terminator_drive_t *)device_ptr);
        return FBE_STATUS_OK;
    case FBE_DRIVE_TYPE_SATA:
    case FBE_DRIVE_TYPE_SATA_FLASH_HE:  /* note,  this is really a SAS drive. */
        *slot_number = sata_drive_get_slot_number((terminator_drive_t *)device_ptr);
        return FBE_STATUS_OK;
    case FBE_DRIVE_TYPE_FIBRE:
        *slot_number = fc_drive_get_slot_number((terminator_drive_t *)device_ptr);
        return FBE_STATUS_OK;    
    default:
        /* support only SAS and SATA drives */
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_status_t terminator_get_drive_enclosure_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *enclosure_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /* make sure this is an drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }
    switch(drive_get_protocol((terminator_drive_t *)device_ptr))
    {
    case FBE_DRIVE_TYPE_SAS:
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        *enclosure_number = sas_drive_get_enclosure_number((terminator_drive_t *)device_ptr);
        return FBE_STATUS_OK;
    case FBE_DRIVE_TYPE_SATA:
    case FBE_DRIVE_TYPE_SATA_FLASH_HE:
        *enclosure_number = sata_drive_get_enclosure_number((terminator_drive_t *)device_ptr);
        return FBE_STATUS_OK;
    case FBE_DRIVE_TYPE_FIBRE:
        *enclosure_number = fc_drive_get_enclosure_number((terminator_drive_t *)device_ptr);
        return FBE_STATUS_OK;    
    default:
        /* support only SAS and SATA drives */
        return FBE_STATUS_GENERIC_FAILURE;
    }
}


/*********************************************************************
* terminator_get_virtual_phy_id_by_encl_id ()
*********************************************************************
*
*  Description:
*   This gives the virtual phy id corresponding to the given enclosure
*   id.
*
*  Inputs:
*   port number, enclosure id and virtual phy id mem(ptr) to fill.
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/
//fbe_status_t terminator_get_virtual_phy_id_by_encl_id(fbe_u32_t port_number, fbe_terminator_api_device_handle_t enclosure_id, fbe_terminator_api_device_handle_t *virtual_phy_id)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t *enclosure = NULL;
//    base_component_t * child = NULL;
//    terminator_port_t * matching_port = NULL;
//
//    *virtual_phy_id = DEVICE_HANDLE_INVALID;
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//    //enclosure = port_get_device_by_device_id(matching_port, enclosure_id);
//    if ((enclosure == NULL) ||
//        (base_component_get_component_type(enclosure) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE))
//    {
//        return(status);
//    }
//
//    // Get the related virtual phy  of the enclosure
//    child = base_component_get_child_by_type(enclosure, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
//    if(child == NULL)
//    {
//        return(status);
//    }
//
//    *virtual_phy_id = base_component_get_id(child);
//    status = FBE_STATUS_OK;
//    return(status);
//}
/*********************************************************************
* terminator_get_enclosure_virtual_phy_ptr ()
*********************************************************************
*
*  Description:
*   This gives the virtual phy handle(object pointer) to the given enclosure
*   id.
*
*  Inputs:
*   port number, enclosure id and object pointer to fill.
*
*  Return Value:
*   success or failure.
*
*  History:
*    Sep08  created
*
*********************************************************************/

fbe_status_t terminator_get_enclosure_virtual_phy_ptr(fbe_terminator_device_ptr_t enclosure_ptr, fbe_terminator_device_ptr_t * virtual_phy_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * child = NULL;

    // Get the related virtual phy  of the enclosure
    child = base_component_get_child_by_type(enclosure_ptr, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    if(child == NULL)
    {
        return(status);
    }

    *virtual_phy_handle = child;
    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t terminator_get_phy_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u32_t phy_number,
    ses_stat_elem_exp_phy_struct *exp_phy_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_phy_eses_status(virtual_phy_handle, phy_number, exp_phy_stat);

    return status;
}

fbe_status_t terminator_set_phy_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u32_t phy_number,
    ses_stat_elem_exp_phy_struct exp_phy_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_phy_eses_status(virtual_phy_handle, phy_number, exp_phy_stat);
    return status;
}

// Set the power supply element status ( complete element structure)
fbe_status_t terminator_set_ps_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_ps_id ps_id,
    ses_stat_elem_ps_struct ps_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_ps_eses_status(virtual_phy_handle, ps_id, ps_stat);

    return status;
}

// Get the power supply eses status ( complete element structure)
fbe_status_t terminator_get_ps_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_ps_id ps_id,
    ses_stat_elem_ps_struct *ps_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_ps_eses_status(virtual_phy_handle, ps_id, ps_stat);

    return status;
}

fbe_status_t terminator_get_emcEnclStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                           ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_emcEnclStatus(virtual_phy_handle, emcEnclStatusPtr);

    return status;
}

fbe_status_t terminator_set_emcEnclStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                           ses_pg_emc_encl_stat_struct *emcEnclStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_emcEnclStatus(virtual_phy_handle, emcEnclStatusPtr);

    return status;
}

fbe_status_t terminator_get_emcPsInfoStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_emcPsInfoStatus(virtual_phy_handle, emcPsInfoStatusPtr);

    return status;
}

fbe_status_t terminator_set_emcPsInfoStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_ps_info_elem_struct *emcPsInfoStatusPtr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_emcPsInfoStatus(virtual_phy_handle, emcPsInfoStatusPtr);

    return status;
}

fbe_status_t terminator_get_emcGeneralInfoDirveSlotStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                             fbe_u8_t drive_slot)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_emcGeneralInfoDirveSlotStatus(virtual_phy_handle, emcGeneralInfoDirveSlotStatusPtr,drive_slot);

    return status;
}

fbe_status_t terminator_set_emcGeneralInfoDirveSlotStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                             fbe_u8_t drive_slot)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_emcGeneralInfoDirveSlotStatus(virtual_phy_handle, emcGeneralInfoDirveSlotStatusPtr,drive_slot);

    return status;
}

// Set the SAS connector element status ( complete element structure)
fbe_status_t terminator_set_sas_conn_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct sas_conn_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_sas_conn_eses_status(virtual_phy_handle, sas_conn_id, sas_conn_stat);

    return status;
}

// Get the SAS connector eses status ( complete element structure)
fbe_status_t terminator_get_sas_conn_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct *sas_conn_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_sas_conn_eses_status(virtual_phy_handle, sas_conn_id, sas_conn_stat);

    return status;
}

// Get the cooling element status ( complete element structure)
fbe_status_t terminator_get_cooling_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_cooling_id cooling_id,
    ses_stat_elem_cooling_struct *cooling_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_cooling_eses_status(virtual_phy_handle, cooling_id, cooling_stat);

    return status;
}

// Set the cooling element status ( complete element structure)
fbe_status_t terminator_set_cooling_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_cooling_id cooling_id,
    ses_stat_elem_cooling_struct cooling_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_cooling_eses_status(virtual_phy_handle, cooling_id, cooling_stat);
    return status;
}


// Get the temp_sensor element status ( complete element structure)
fbe_status_t terminator_get_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_temp_sensor_eses_status(virtual_phy_handle,
        temp_sensor_id, temp_sensor_stat);

    return status;
}

// Get the temp sensor element status ( complete element structure)
fbe_status_t terminator_set_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_temp_sensor_eses_status(virtual_phy_handle,
        temp_sensor_id, temp_sensor_stat);
    return status;
}




// Get the overall temp_sensor element status ( complete element structure)
fbe_status_t terminator_get_overall_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_get_overall_temp_sensor_eses_status(virtual_phy_handle,
        temp_sensor_id, temp_sensor_stat);

    return status;
}

// Set the overall temp sensor element status ( complete element structure)
fbe_status_t terminator_set_overall_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct temp_sensor_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    /* Caller should lock the port configuration so no one else can change it while we modify configuration */
    status = sas_virtual_phy_set_overall_temp_sensor_eses_status(virtual_phy_handle,
        temp_sensor_id, temp_sensor_stat);

    return status;
}





// This functions does NOT return an error if the drive is not inserted. Just does not fill the sas address...
fbe_status_t terminator_get_sas_address_by_slot_number_and_virtual_phy_handle(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t dev_slot_num,
    fbe_u64_t *sas_address)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * parent_enclosure = NULL;
    fbe_terminator_device_ptr_t matching_port = NULL;
    fbe_bool_t is_slot_available = FBE_FALSE;

    fbe_terminator_device_ptr_t drive_ptr;

    *sas_address = 0;
    terminator_get_port_ptr(virtual_phy_handle, &matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    base_component_get_parent(virtual_phy_handle, &parent_enclosure);
    if(base_component_get_component_type(parent_enclosure) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return(status);
    }
    // get the slot number for the enclosure
    status = terminator_update_enclosure_drive_slot_number(virtual_phy_handle, (fbe_u32_t *)&dev_slot_num);
    RETURN_ON_ERROR_STATUS;

    status = sas_enclosure_is_slot_available((terminator_enclosure_t *)parent_enclosure, dev_slot_num, &is_slot_available);
    // As there is no drive, just return WITHOUT error.
    if (status == FBE_STATUS_OK && is_slot_available == FBE_TRUE)
    {
        return status;
    }

    status = sas_enclosure_get_device_ptr_by_slot((terminator_enclosure_t *)parent_enclosure, dev_slot_num, &drive_ptr);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    if((drive_ptr == NULL) ||
       (base_component_get_component_type(drive_ptr) != TERMINATOR_COMPONENT_TYPE_DRIVE))
    {
        return(status);
    }

    //get login data
    *sas_address = sas_drive_get_sas_address((terminator_drive_t *) drive_ptr);
    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t terminator_get_sas_drive_info(fbe_terminator_device_ptr_t device_ptr,
    fbe_terminator_sas_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (device_ptr == NULL)
    {
        return status;
    }
    /* make sure this is a drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }
    drive_info->sas_address = sas_drive_get_sas_address((terminator_drive_t*)device_ptr);
    drive_info->drive_type = sas_drive_get_drive_type((terminator_drive_t*)device_ptr);
    drive_info->capacity = drive_get_capacity((terminator_drive_t*)device_ptr);
    drive_info->block_size = drive_get_block_size((terminator_drive_t*)device_ptr);
    fbe_copy_memory(&drive_info->drive_serial_number,
    sas_drive_get_serial_number((terminator_drive_t*)device_ptr),
    sizeof(drive_info->drive_serial_number));
    fbe_copy_memory(&drive_info->product_id, sas_drive_get_product_id((terminator_drive_t*)device_ptr), sizeof(drive_info->product_id));
    drive_info->backend_number = sas_drive_get_backend_number((terminator_drive_t*)device_ptr);
    drive_info->encl_number = sas_drive_get_enclosure_number((terminator_drive_t*)device_ptr);
    drive_info->slot_number = sas_drive_get_slot_number((terminator_drive_t*)device_ptr);
    status = FBE_STATUS_OK;

    return status;
}


fbe_status_t terminator_get_sata_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                            fbe_terminator_sata_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (device_ptr == NULL)
    {
        return status;
    }
    /* make sure this is a drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }
    drive_info->sas_address = sata_drive_get_sas_address((terminator_drive_t*)device_ptr);
    drive_info->drive_type = sata_drive_get_drive_type((terminator_drive_t*)device_ptr);
    drive_info->capacity = drive_get_capacity((terminator_drive_t*)device_ptr);
    drive_info->block_size = drive_get_block_size((terminator_drive_t*)device_ptr);

    fbe_copy_memory(&drive_info->drive_serial_number,
    sata_drive_get_serial_number((terminator_drive_t*)device_ptr),
    sizeof(drive_info->drive_serial_number));
    fbe_copy_memory(&drive_info->product_id, sata_drive_get_product_id((terminator_drive_t*)device_ptr), sizeof(drive_info->product_id));
    drive_info->backend_number = sata_drive_get_backend_number((terminator_drive_t*)device_ptr);
    drive_info->encl_number = sata_drive_get_enclosure_number((terminator_drive_t*)device_ptr);
    drive_info->slot_number = sata_drive_get_slot_number((terminator_drive_t*)device_ptr);

    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_set_sas_drive_info(fbe_terminator_device_ptr_t device_ptr,
    fbe_terminator_sas_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (device_ptr == NULL)
    {
        return status;
    }
    /* make sure this is a drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }

    sas_drive_set_sas_address((terminator_drive_t*)device_ptr, drive_info->sas_address);
    sas_drive_set_drive_type((terminator_drive_t*)device_ptr, drive_info->drive_type);
    sas_drive_info_set_serial_number(base_component_get_attributes(device_ptr), drive_info->drive_serial_number);
    sas_drive_info_set_product_id(base_component_get_attributes(device_ptr), drive_info->product_id);
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_set_sata_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                            fbe_terminator_sata_drive_info_t *drive_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (device_ptr == NULL)
    {
        return status;
    }
    /* make sure this is a drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }
    sata_drive_set_sas_address((terminator_drive_t*)device_ptr, drive_info->sas_address);
    sata_drive_set_drive_type((terminator_drive_t*)device_ptr, drive_info->drive_type);
    drive_set_capacity((terminator_drive_t*)device_ptr, drive_info->capacity);
    drive_set_block_size((terminator_drive_t*)device_ptr, drive_info->block_size);

    sata_drive_info_set_serial_number(base_component_get_attributes(device_ptr), drive_info->drive_serial_number);
    sata_drive_info_set_product_id(base_component_get_attributes(device_ptr), drive_info->product_id);
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_set_drive_product_id(fbe_terminator_device_ptr_t device_ptr,
                                            const fbe_u8_t * product_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_drive_type_t drive_type;

    if (device_ptr == NULL)
    {
        return status;
    }
    /* make sure this is a drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }

    drive_type = drive_get_protocol(device_ptr);
    switch(drive_type)
    {
    case FBE_DRIVE_TYPE_SAS:
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        sas_drive_set_product_id(device_ptr, product_id);
        break;
    case FBE_DRIVE_TYPE_SATA:
    case FBE_DRIVE_TYPE_SATA_FLASH_HE:
        sata_drive_set_product_id(device_ptr, product_id);
        break;
    case FBE_DRIVE_TYPE_FIBRE:
        fc_drive_set_product_id(device_ptr, product_id);
        break;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_get_drive_product_id(fbe_terminator_device_ptr_t device_ptr,
                                            fbe_u8_t * product_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_drive_type_t drive_type;
    fbe_u8_t *buff = NULL;

    if (device_ptr == NULL)
    {
        return status;
    }
    /* make sure this is a drive */
    if (base_component_get_component_type(device_ptr)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return status;
    }

    drive_type = drive_get_protocol(device_ptr);

    switch(drive_type)
    {
    case FBE_DRIVE_TYPE_SAS:
    case FBE_DRIVE_TYPE_SAS_FLASH_HE:
    case FBE_DRIVE_TYPE_SAS_FLASH_ME:
    case FBE_DRIVE_TYPE_SAS_FLASH_LE:
    case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        buff = sas_drive_get_product_id(device_ptr);
        break;
    case FBE_DRIVE_TYPE_SATA:
    case FBE_DRIVE_TYPE_SATA_FLASH_HE:
        buff = sata_drive_get_product_id(device_ptr);
        break;
    case FBE_DRIVE_TYPE_FIBRE:
        buff = fc_drive_get_product_id(device_ptr);
        break;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(buff != NULL)
    {
        fbe_copy_memory(product_id, buff, TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_SIZE);
        status = FBE_STATUS_OK;
    }
    return status;
}

fbe_status_t terminator_verify_drive_product_id(fbe_terminator_device_ptr_t device_ptr,
                                                fbe_u8_t * product_id)
{
    fbe_status_t     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_drive_type_t drive_type;

    if (device_ptr == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* make sure this is a drive */
    if (base_component_get_component_type(device_ptr) != TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_type = drive_get_protocol(device_ptr);
    switch (drive_type)
    {
        case FBE_DRIVE_TYPE_SAS:
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            status = sas_drive_verify_product_id(product_id);
            break;
        case FBE_DRIVE_TYPE_SATA:
            status = sata_drive_verify_product_id(product_id);
            break;
        case FBE_DRIVE_TYPE_FIBRE: /* Verification of FC drive product ID not supported. */
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

/* From Rajesh, this function waits for the port logout_queue to be empty before returns */
fbe_status_t fbe_terminator_wait_on_port_logout_complete(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    fbe_bool_t port_logout_empty = FBE_FALSE;

    if (handle == NULL)
    {
        return status;
    }

    while(port_logout_empty!=FBE_TRUE)
    {
        status = port_is_logout_queue_empty(handle, &port_logout_empty);
        RETURN_ON_ERROR_STATUS;
        csx_p_thr_yield();
    }

    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_get_reset_begin(fbe_terminator_device_ptr_t handle, fbe_bool_t *reset_begin)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (handle == NULL)
    {
        return status;
    }
    *reset_begin = port_get_reset_begin(handle);

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t terminator_get_reset_completed(fbe_terminator_device_ptr_t handle, fbe_bool_t *reset_completed)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (handle == NULL)
    {
        return status;
    }
    *reset_completed = port_get_reset_completed(handle);

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t terminator_clear_reset_begin(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL)
    {
        return status;
    }
    port_set_reset_begin(handle, FBE_FALSE);

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t terminator_clear_reset_completed(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL)
    {
        return status;
    }
    port_set_reset_completed(handle, FBE_FALSE);

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t terminator_set_reset_begin(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL)
    {
        return status;
    }
    port_set_reset_begin(handle, FBE_TRUE);

    status = FBE_STATUS_OK;
    return(status);
}

fbe_status_t terminator_set_reset_completed(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (handle == NULL)
    {
        return status;
    }
    port_set_reset_completed(handle, FBE_TRUE);
    status = FBE_STATUS_OK;
    return(status);
}

/* This functions blocks till the respective reset flag ( completed or begin) is cleared.
 * This may be useful when we want to decide on issuing a reset-complete after issuing a
 * reset-start. Because event thread may be already running at the time we issued
 * a reset-start and the thread may be executing instructions after the check for
 * reset-start and thus checks for reset-complete. We may not want it to send reset-complete
 * BEFORE issuing - 1. reset-start and 2. logging out all devices. Look at the function
 * miniport_api_process_events() to see the sequence for reset-start, logout,
 * reset-start, and login.
 */
fbe_status_t fbe_terminator_wait_on_port_reset_clear(fbe_terminator_device_ptr_t handle,
    fbe_cpd_shim_driver_reset_event_type_t reset_event)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t   reset_begin_set = FBE_TRUE;
    fbe_bool_t   reset_completed_set = FBE_TRUE;

    switch(reset_event)
    {
        case FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_BEGIN:
            while(reset_begin_set)
            {
                status = terminator_get_reset_begin(handle, &reset_begin_set);
                if(status != FBE_STATUS_OK)
                {
                    break;
                }
            }
            break;
        case FBE_PMC_SHIM_DRIVER_RESET_EVENT_TYPE_COMPLETED:
            while(reset_completed_set)
            {
                status = terminator_get_reset_completed(handle, &reset_completed_set);
                if(status != FBE_STATUS_OK)
                {
                    break;
                }
            }
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return(status);
}
/*
 * This function logs out all devices on port and NOT port itself. It does
 * not wake up the event thread but just sets the logout pending.
 */
fbe_status_t fbe_terminator_logout_all_devices_on_port(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * enclosure_base = NULL;

    if (handle == NULL)
    {
        /* Did not find a port with matching port number, return a failure status */
        return status;
    }

    if (base_component_get_child_count(handle) != 0)
    {
        // port should only have one enclosure attached to it, so just log it out which
        // should be sufficient.
        enclosure_base = base_component_get_first_child(handle);
        if(enclosure_base != NULL)
        {
            status = port_logout_device(handle, enclosure_base);
        }
    }
    else
    {
        //no devices on port. should be ok.
        status = FBE_STATUS_OK;
    }
    return(status);
}
/*
 * This function logs in all devices on port and NOT port itself. It does
 * not wake up the event thread but just sets the login pending.
 */
fbe_status_t fbe_terminator_login_all_devices_on_port(fbe_terminator_device_ptr_t handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * enclosure_base = NULL;

    if (handle == NULL)
    {
        /* Did not find a port with matching port number, return a failure status */
        return status;
    }

    if (base_component_get_child_count(handle) != 0)
    {
        // port should only have one enclosure attached to it, so just log it in which
        // should be sufficient.
        enclosure_base = base_component_get_first_child(handle);
        if(enclosure_base != NULL)
        {
            status = port_login_device(handle, enclosure_base);
        }
    }
    else
    {
        //no devices on port. should be ok.
        status = FBE_STATUS_OK;
    }
    return(status);
}

/*
 * This function returns the status of the downstream enclosure(expansion)
 * for the given virtual phy id. The status (inserted or not
 * inserted) is indicated in by the "inserted" parameter passed.
 */
fbe_status_t terminator_get_downstream_wideport_device_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_bool_t *inserted)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t *parent_encl = NULL;
    base_component_t *downstream_encl = NULL;

    *inserted = FBE_FALSE;
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    base_component_get_parent(virtual_phy_handle, &parent_encl);
    if(base_component_get_component_type(parent_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return(status);
    }

    downstream_encl = base_component_get_child_by_type(parent_encl,
                                                       TERMINATOR_COMPONENT_TYPE_ENCLOSURE);

    // we need to find the next downstream enclosure, not EEs
    while((downstream_encl != NULL) && 
          ((base_component_get_component_type(downstream_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE) ||
           (sas_enclosure_get_logical_parent_type((terminator_enclosure_t *)downstream_encl) != TERMINATOR_COMPONENT_TYPE_PORT)))
    {
        downstream_encl = base_component_get_next_child(parent_encl, downstream_encl);
    }


    if(downstream_encl != NULL)
    {
        //downstream enclosure exists

        // The second check of checking if the enclosure state is LOGIN COMPLETE
        // is only temparary. We need to actually set and check state (NOT EQUALS)
        // TERMINATOR_COMPONENT_STATE_NOT_PRESENT & clear it appropriately. The
        // phy status (and not connector) should change if enclosure is logged out.
        // Removed the second check. - PHE
        *inserted = FBE_TRUE;
    }
    status = FBE_STATUS_OK;
    return(status);
}

/*!*************************************************************************
* @fn terminator_get_child_expander_wideport_device_status_by_connector_id(
*    fbe_terminator_device_ptr_t virtual_phy_handle,
*    fbe_u8_t conn_id,
*    fbe_bool_t *inserted)
***************************************************************************
*
* @brief
*       This function gets device status of child expander port(Edge Expander)
*       on the basis of connector ID.
*
* @param    virtual_phy_handle - handle to virtual phy.
* @param    conn_id - internal connector ID.
* @param    *inserted - pointer to value where we will update if device 
*                       is inserted or not.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   28-APR-2010: Created -Dipak Patel
*
***************************************************************************/
fbe_status_t terminator_get_child_expander_wideport_device_status_by_connector_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t conn_id,
    fbe_bool_t *inserted)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t *parent_encl = NULL;
    base_component_t *downstream_encl = NULL;
    terminator_sas_enclosure_info_t *attributes = NULL;

    *inserted = FBE_FALSE;
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = base_component_get_parent(virtual_phy_handle, &parent_encl);
    RETURN_ON_ERROR_STATUS;
    if(base_component_get_component_type(parent_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    downstream_encl = base_component_get_first_child(parent_encl);
    
    while (downstream_encl != NULL)
    {
        if(base_component_get_component_type(downstream_encl) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
        {
            attributes = base_component_get_attributes(downstream_encl);
            if(attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE && attributes->connector_id == conn_id)
            {
                *inserted = FBE_TRUE;
                return (FBE_STATUS_OK);
            }
        }
        downstream_encl = base_component_get_next_child(parent_encl, downstream_encl);
    }
    /* As we want to report elem_stat_code in all the cases, we will always 
       return FBE_STATUS_OK eventhough we are not able to find match. */

    return FBE_STATUS_OK;;
}

/*
 * This function returns the status of the upstream enclosure(primary)
 * for the given virtual phy id. The status (inserted or not
 * inserted) is indicated in by the "inserted" parameter passed.
 */
fbe_status_t terminator_get_upstream_wideport_device_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_bool_t *inserted)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t *parent_encl = NULL;

    *inserted = FBE_FALSE;
    // make sure we got a virtual phy device handle.
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    base_component_get_parent(virtual_phy_handle, &parent_encl);
    if(base_component_get_component_type(parent_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return(status);
    }
    //As there exists always some thing on the upstream port of the enclosure,
    // for now return the upstream wideport status as OK. In future we may have
    // to consider the SAS cable state--etc.
    *inserted = FBE_TRUE;
    status = FBE_STATUS_OK;
    return(status);
}

/*
 * The following two functions DO NOT return an error
 * if no device is present upstream or downstream
 * respectively. They just put sas address to zero
 * and return FBE_STATUS_OK. Error is returned only
 * if some sanity check on the provided parameters fails
 */

/* This functions gets the sas address of the enclosure
 * that is attached upstream given a virtual PHY device
 * id(primary port)
 */
fbe_status_t terminator_get_upstream_wideport_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t *parent_encl = NULL;

    fbe_cpd_shim_callback_login_t * login_data = NULL;

    *sas_address = 0;

    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    base_component_get_parent(virtual_phy_handle, &parent_encl);
    if(base_component_get_component_type(parent_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return(status);
    }
    login_data = sas_enclosure_info_get_login_data(base_component_get_attributes(parent_encl));
    *sas_address = login_data->parent_device_address;

    status = FBE_STATUS_OK;
    return(status);
}

/* This functions gets the sas address of the enclosure
 * that is attached downstream given a virtual PHY device
 * id. (expansion port)
 */
fbe_status_t terminator_get_downstream_wideport_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t *parent_encl = NULL;
    base_component_t *downstream_encl = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    *sas_address = 0;
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    base_component_get_parent(virtual_phy_handle, &parent_encl);

    if(base_component_get_component_type(parent_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return(status);
    }

    downstream_encl = base_component_get_child_by_type(parent_encl,
                                                       TERMINATOR_COMPONENT_TYPE_ENCLOSURE);

    // we need to find the next downstream enclosure, not EEs
    while((downstream_encl != NULL) && 
          ((base_component_get_component_type(downstream_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE) ||
           (sas_enclosure_get_logical_parent_type((terminator_enclosure_t *)downstream_encl) != TERMINATOR_COMPONENT_TYPE_PORT)))
    {
        downstream_encl = base_component_get_next_child(parent_encl, downstream_encl);
    }


    if(downstream_encl != NULL)
    {
        //downstream enclosure exists
        login_data = sas_enclosure_info_get_login_data(base_component_get_attributes(downstream_encl));
        *sas_address = login_data->device_address;
    }

    status = FBE_STATUS_OK;
    return(status);
}



/*!****************************************************************************
 * @fn      fbe_status_t terminator_map_position_max_conns_to_range_conn_id()
 *
 * @brief
 *  This function maps a connector into one of several possible ranges and connector id. 
 *
 * @param position - connector position
 * @param max_conns - maximum connectors per range
 * @param return_range (OUTPUT)- pointer to the return connector range enum 
 * @param conn_id (OUTPUT) - pointer to the return connector id. 

 *
 * @return
 *  FBE_STATUS_OK if the connector is in a valid range.
 * @Note the return_range is set to illegal even if the return status
 * is FAILURE.
 * @Note This function should not be used to calculate range for Voyager_ee as for Voyager
 * ee primary port we are expecting connector is DOWNSTREAM but this function will assume
 * UPSTREAM.
 *
 * HISTORY
 *  05/11/10 :  Gerry Fredette -- Created.
 *  26-Nov-2-13: PHE - Added the support for Viking.
 *
 *****************************************************************************/
fbe_status_t terminator_map_position_max_conns_to_range_conn_id(
    fbe_sas_enclosure_type_t encl_type,
    fbe_u8_t position, 
    fbe_u8_t max_conns, 
    terminator_conn_map_range_t *return_range, 
    fbe_u8_t *conn_id)
{
    if(encl_type == FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP)
    {
        if(position <= 4)
        {
            // Indicates the connector belongs to the primary port
            *return_range = CONN_IS_DOWNSTREAM;
            *conn_id = 0;
        }
        else if(position >= 5 && position <= 9)
        {
            // Indicates the connector belongs to the primary port
            *return_range = CONN_IS_UPSTREAM;
            *conn_id = 1;
        }
        else if(position >= 10 && position <= 33) 
        {
            // search child list to match connector id. for SXP0, SXP1, SXP2, or SXP3 (Viking)
            *return_range = CONN_IS_INTERNAL_RANGE1;
            *conn_id = 2 + (position - 10)/6;
        }
        else if(position >= 34 && position <= 43) 
        {
            // search child list to match connector id. for SXP0, SXP1, SXP2, or SXP3 (Viking)
            *return_range = CONN_IS_RANGE0;
            *conn_id = 6 + (position - 34)/5;
        }
        else
        {
            *return_range = CONN_IS_ILLEGAL;
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if(encl_type == FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP)
    {
        if(position <= 4)
        {
            // Indicates the connector belongs to the primary port
            *return_range = CONN_IS_DOWNSTREAM;
            *conn_id = 0;
        }
        else if(position >= 5 && position <= 9)
        {
            // Indicates the connector belongs to the primary port
            *return_range = CONN_IS_UPSTREAM;
            *conn_id = 1;
        }
        else if(position >= 20 && position <= 28) 
        {
            // search child list to match connector id. for SXP0, SXP1, SXP2, or SXP3 (Viking)
            *return_range = CONN_IS_INTERNAL_RANGE1;
            *conn_id = 4;
        }
        else if(position >= 10 && position <= 19) 
        {
            // search child list to match connector id. for SXP0, SXP1, SXP2, or SXP3 (Viking)
            *return_range = CONN_IS_RANGE0;
            *conn_id = 2 + (position - 10)/5;
        }
        else
        {
            *return_range = CONN_IS_ILLEGAL;
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if(encl_type == FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP)
    {
        if(position <= 4)
        {
            // Indicates the connector belongs to the primary port
            *return_range = CONN_IS_DOWNSTREAM;
            *conn_id = 0;
        }
        else if(position >= 5 && position <= 9)
        {
            // Indicates the connector belongs to the primary port
            *return_range = CONN_IS_UPSTREAM;
            *conn_id = 1;
        }
        else if(position >= 20 && position <= 37) 
        {
            // search child list to match connector id. for SXP0, SXP1, SXP2, or SXP3 (Viking)
            *return_range = CONN_IS_INTERNAL_RANGE1;
            *conn_id = 4 + (position - 20)/9; // this is magic to get 4 and 5 as connector id
        }
        else if(position >= 10 && position <= 19) 
        {
            // search child list to match connector id. for SXP0, SXP1, SXP2, or SXP3 (Viking)
            *return_range = CONN_IS_RANGE0;
            *conn_id = 2 + (position - 10)/5;
        }
        else
        {
            *return_range = CONN_IS_ILLEGAL;
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else 
    {
        if(position < max_conns)
        {
            // Indicates the connector belongs to the extension port
            *return_range = CONN_IS_DOWNSTREAM;
        }
        else if(position >= max_conns && (position < max_conns * 2))
        {
            // Indicates the connector belongs to the primary port
            *return_range = CONN_IS_UPSTREAM;
        }
        else if((position >= max_conns * 2) && (position < max_conns * 4))
        {
            // search child list to match connector id. 
            *return_range = CONN_IS_RANGE0;
        }
        else if((position >= max_conns * 4) && (position < max_conns * 6))
        {
            // search child list to match connector id. for EE0 or EE1 (Voyager)
            *return_range = CONN_IS_INTERNAL_RANGE1;
        }
        else
        {
            *return_range = CONN_IS_ILLEGAL;
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        if (conn_id)
        {
            // The conn_id is 0 for downstream, 1 for upstream and 2, 3, ... for children.
            // EE0 is 2, EE1 is 3.   EE3 is 4 and EE4 is 5.
            *conn_id = position / max_conns;  
        }
    }

    return FBE_STATUS_OK;
}


/*!*************************************************************************
* @fn terminator_get_child_expander_wideport_sas_address_by_connector_id(
*    fbe_terminator_device_ptr_t virtual_phy_handle,
*    fbe_u8_t conn_id,
*    fbe_u64_t *sas_address)
***************************************************************************
*
* @brief
*       This function gets sas address of child expander port(Edge Expander)
*       on the basis of connector ID.
*
* @param    virtual_phy_handle - handle to virtual phy.
* @param    *conn_id - pointer to internal connector ID.
* @param    *sas_address - pointer to value where we will update sas_address.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   28-APR-2010: Created -Dipak Patel
*
***************************************************************************/
fbe_status_t terminator_get_child_expander_wideport_sas_address_by_connector_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t conn_id,
    fbe_u64_t *sas_address)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t *parent_encl = NULL;
    base_component_t *downstream_encl = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    terminator_sas_enclosure_info_t *attributes = NULL;

    *sas_address = 0;
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = base_component_get_parent(virtual_phy_handle, &parent_encl);
    RETURN_ON_ERROR_STATUS;
    if(base_component_get_component_type(parent_encl) != TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    downstream_encl = base_component_get_first_child(parent_encl);
    while (downstream_encl != NULL)
    {
        if(base_component_get_component_type(downstream_encl) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
        {
            attributes = base_component_get_attributes(downstream_encl);
            if(attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE && attributes->connector_id == conn_id)
            {
                login_data = sas_enclosure_info_get_login_data(base_component_get_attributes(downstream_encl));
                *sas_address = login_data->device_address;
                
                return (FBE_STATUS_OK);
            }
        }
        downstream_encl = base_component_get_next_child(parent_encl, downstream_encl);
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

//fbe_status_t fbe_terminator_get_sas_enclosure_info(fbe_u32_t port_number,
//                                               fbe_terminator_api_device_handle_t encl_id,
//                                               fbe_terminator_sas_encl_info_t   *encl_info)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * matching_device = NULL;
//    terminator_port_t * matching_port = NULL;
//    fbe_u8_t *uid = NULL;
//
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//    //matching_device = port_get_device_by_device_id(matching_port, encl_id);
//    if (matching_device == NULL)
//    {
//        return status;
//    }
//    /* make sure this is a drive */
//    if (base_component_get_component_type(matching_device)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
//    {
//        return status;
//    }
//    encl_info->sas_address = sas_enclosure_get_sas_address((terminator_enclosure_t*)matching_device);
//    encl_info->encl_type = sas_enclosure_get_enclosure_type((terminator_enclosure_t*)matching_device);
//    uid = sas_enclosure_get_enclosure_uid((terminator_enclosure_t*)matching_device);
//    fbe_copy_memory(&encl_info->uid, uid, sizeof (encl_info->uid));
//    status = FBE_STATUS_OK;
//    return status;
//}
//fbe_status_t fbe_terminator_set_sas_enclosure_info(fbe_u32_t port_number,
//                                               fbe_terminator_api_device_handle_t encl_id,
//                                               fbe_terminator_sas_encl_info_t   *encl_info)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * matching_device = NULL;
//    terminator_port_t * matching_port = NULL;
//
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//   // matching_device = port_get_device_by_device_id(matching_port, encl_id);
//    if (matching_device == NULL)
//    {
//        return status;
//    }
//    /* make sure this is a drive */
//    if (base_component_get_component_type(matching_device)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
//    {
//        return status;
//    }
//    status = sas_enclosure_set_sas_address((terminator_enclosure_t*)matching_device, encl_info->sas_address);
//    if (status!=FBE_STATUS_OK){
//        return status;
//    }
//    status = sas_enclosure_set_enclosure_type((terminator_enclosure_t*)matching_device, encl_info->encl_type);
//    if (status!=FBE_STATUS_OK){
//        return status;
//    }
//    status = sas_enclosure_set_enclosure_uid((terminator_enclosure_t*)matching_device, encl_info->uid);
//    return status;
//}


fbe_status_t fbe_terminator_set_io_mode(fbe_terminator_io_mode_t io_mode)
{
    terminator_io_mode = io_mode;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_get_io_mode(fbe_terminator_io_mode_t *io_mode)
{
    *io_mode = terminator_io_mode;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_set_io_completion_at_dpc(fbe_bool_t b_is_io_completion_at_dpc)
{
    terminator_trace(FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s changing terminator I/O completion IRQL from: %d to: %d\n", 
                     __FUNCTION__, terminator_b_is_io_completion_at_dpc, b_is_io_completion_at_dpc);
    terminator_b_is_io_completion_at_dpc = b_is_io_completion_at_dpc;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_get_io_completion_at_dpc(fbe_bool_t *b_is_io_completion_at_dpc_p)
{
    *b_is_io_completion_at_dpc_p = terminator_b_is_io_completion_at_dpc;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_terminator_set_io_global_completion_delay(fbe_u32_t global_completion_delay)
{
    return fbe_terminator_miniport_api_set_global_completion_delay(global_completion_delay);
}

fbe_status_t terminator_destroy_disk_files(fbe_u32_t port_number)
{
    fbe_u32_t device_id = 0;
    fbe_u8_t drive_name[12];
    int count = 0;

    /* create the file name search string, which we use wildcard * */
    fbe_zero_memory(drive_name, sizeof(drive_name));
    for(device_id = 0; device_id < 250; device_id ++) // core_config.h ENCLOSURES_PER_BUS * ENCLOSURE_SLOTS
    {
        count = _snprintf(drive_name, sizeof(drive_name), "disk%d_%d", port_number, device_id);
        if (( count < 0 ) || (sizeof(drive_name) == count )) {
            fbe_debug_break();
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* find the file first */
        if(fbe_file_find(drive_name) != FBE_FILE_ERROR)
        {
            /* delete the file now */
            fbe_file_delete(drive_name);
        }
    }
    return FBE_STATUS_OK;
}

/*
 * The below two functions are just wrappers for corresponding Virtual phy Functions.
 */
fbe_bool_t terminator_phy_corresponds_to_drive_slot(
    fbe_u8_t phy_id,
    fbe_u8_t *drive_slot,
    fbe_sas_enclosure_type_t encl_type)
{
    return(sas_virtual_phy_phy_corresponds_to_drive_slot(phy_id, drive_slot, encl_type));
}

fbe_bool_t terminator_phy_corresponds_to_connector(
    fbe_u8_t phy_id,
    fbe_u8_t *connector,
    fbe_u8_t *connector_id,
    fbe_sas_enclosure_type_t encl_type)
{
    return(sas_virtual_phy_phy_corresponds_to_connector(phy_id, connector, connector_id, encl_type));
}
/*
 * End of wrappers for the virtual phy related PHY functions
 */
fbe_status_t terminator_get_drive_slot_to_phy_mapping(fbe_u8_t drive_slot, fbe_u8_t *phy_id, fbe_sas_enclosure_type_t encl_type)
{
    return(sas_virtual_phy_get_drive_slot_to_phy_mapping(drive_slot, phy_id, encl_type));
}

/**************************************************************************
 *  terminator_get_encl_sas_address_by_virtual_phy_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    The following funtions returns the sas address of the enclosure/exp
 *
 *  PARAMETERS:
 *      port number, virtual phy device id and pointer to sas address to be
 *          filled.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Aug08: created.
 **************************************************************************/
fbe_status_t terminator_get_encl_sas_address_by_virtual_phy_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    *sas_address = 0;

    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }
    // The encl sas address is stored as parent address in the virtual phy login data.
    login_data = sas_virtual_phy_info_get_login_data(base_component_get_attributes(virtual_phy_handle));
    *sas_address = cpd_shim_callback_login_get_parent_device_address(login_data);

    status = FBE_STATUS_OK;
    return(status);
}


fbe_status_t terminator_virtual_phy_get_enclosure_type(fbe_terminator_device_ptr_t handle, fbe_sas_enclosure_type_t *encl_type)
{
    return (sas_virtual_phy_get_enclosure_type(handle, encl_type));
}
fbe_status_t terminator_virtual_phy_get_enclosure_uid(fbe_terminator_device_ptr_t handle, fbe_u8_t ** uid)
{
    return (sas_virtual_phy_get_enclosure_uid(handle, uid));
}
fbe_status_t terminator_get_port_index(fbe_terminator_device_ptr_t component_handle, fbe_u32_t * port_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_handle = NULL;;

    status = terminator_get_port_ptr(component_handle, &port_handle);
    if ( port_handle != NULL)
    {
        status = port_get_miniport_port_index(port_handle, port_index);
    }
    return status;
}
fbe_status_t terminator_get_port_miniport_port_index(fbe_terminator_device_ptr_t component_handle, fbe_u32_t *port_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_handle = NULL;;

    status = terminator_get_port_ptr(component_handle, &port_handle);
    if ( port_handle != NULL)
    {
        status = port_get_miniport_port_index(port_handle, port_index);
    }
    return status;
}


fbe_status_t terminator_get_port_ptr(fbe_terminator_device_ptr_t component_handle,
                                        fbe_terminator_device_ptr_t *matching_port_handle)
{
    base_component_t * parent = NULL;
    terminator_component_type_t type;

    if(component_handle == NULL) return FBE_STATUS_GENERIC_FAILURE;

    type = base_component_get_component_type(component_handle);
    switch(type)
    {
    case TERMINATOR_COMPONENT_TYPE_PORT:
        *matching_port_handle = component_handle;
        break;
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
    case TERMINATOR_COMPONENT_TYPE_DRIVE:
    case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
        base_component_get_parent(component_handle, &parent);
        if (parent == NULL)
        {
            *matching_port_handle = NULL;
            return FBE_STATUS_GENERIC_FAILURE;
        }
        while(base_component_get_component_type(parent)!= TERMINATOR_COMPONENT_TYPE_PORT)
        {
            base_component_get_parent(parent, &parent);
            if (parent == NULL)
            {
                *matching_port_handle = NULL;
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        if (base_component_get_component_type(parent)== TERMINATOR_COMPONENT_TYPE_PORT)
        {
            *matching_port_handle = parent;
        }
        break;
    case TERMINATOR_COMPONENT_TYPE_BOARD:
    default:
        *matching_port_handle = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    return FBE_STATUS_OK;
}

fbe_status_t terminator_max_drive_slots(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_drive_slots)
{
    return(sas_virtual_phy_max_drive_slots(encl_type, max_drive_slots));
}

fbe_status_t terminator_max_phys(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_phys)
{
    return(sas_virtual_phy_max_phys(encl_type, max_phys));
}

fbe_status_t terminator_max_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_lccs)
{
    return(sas_virtual_phy_max_lccs(encl_type, max_lccs));
}

fbe_status_t terminator_max_ee_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ee_lccs)
{
    return(sas_virtual_phy_max_ee_lccs(encl_type, max_ee_lccs));
}

fbe_status_t terminator_max_ext_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ext_cooling_elems)
{
    return(sas_virtual_phy_max_ext_cooling_elems(encl_type, max_ext_cooling_elems));
}

fbe_status_t terminator_max_bem_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_bem_cooling_elems)
{
    return(sas_virtual_phy_max_bem_cooling_elems(encl_type, max_bem_cooling_elems));
}

fbe_status_t terminator_max_conns_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    return(sas_virtual_phy_max_conns_per_lcc(encl_type, max_conns));
}

fbe_status_t terminator_max_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    return(sas_virtual_phy_max_conns_per_port(encl_type, max_conns));
}

fbe_status_t terminator_max_conn_id_count(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conn_id_count)
{
    return(sas_virtual_phy_max_conn_id_count(encl_type, max_conn_id_count));
}

fbe_status_t terminator_max_single_lane_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    return(sas_virtual_phy_max_single_lane_conns_per_port(encl_type, max_conns));
}

fbe_status_t terminator_max_temp_sensors_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_temp_sensor_elems_count)
{
    return(sas_virtual_phy_max_temp_sensor_elems(encl_type, max_temp_sensor_elems_count));
}

/* This function frees the dynamically allocated
 * memory pointed to by the given pointer if the
 * pointer is not NULL
 */
void terminator_free_mem_on_not_null(void *ptr)
{
    if(ptr != NULL)
    {
        fbe_terminator_free_memory(ptr);
    }
    return;
}

fbe_status_t terminator_drive_get_type(fbe_terminator_device_ptr_t handle, fbe_sas_drive_type_t *drive_type)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *drive_type = sas_drive_get_drive_type(handle);
        return FBE_STATUS_OK;
    }
}
fbe_status_t terminator_drive_get_serial_number(fbe_terminator_device_ptr_t handle, fbe_u8_t **serial_number)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        *serial_number = sas_drive_get_serial_number(handle);
        return FBE_STATUS_OK;
    }
}

fbe_status_t terminator_drive_get_state(fbe_terminator_device_ptr_t handle, terminator_sas_drive_state_t * drive_state)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        sas_drive_get_state(handle, drive_state);
        return FBE_STATUS_OK;
    }
}

fbe_status_t terminator_drive_set_state(fbe_terminator_device_ptr_t handle, terminator_sas_drive_state_t  drive_state)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        sas_drive_set_state(handle, drive_state);
        return FBE_STATUS_OK;
    }
}

fbe_status_t terminator_sas_drive_get_default_page_info(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *default_info_p)
{
    return sas_drive_get_default_page_info(drive_type, default_info_p);
}

fbe_status_t terminator_sas_drive_set_default_page_info(fbe_sas_drive_type_t drive_type, const fbe_terminator_sas_drive_type_default_info_t *default_info_p)
{
    return sas_drive_set_default_page_info(drive_type, default_info_p);
}

fbe_status_t terminator_sas_drive_set_default_field(fbe_sas_drive_type_t drive_type, fbe_terminator_drive_default_field_t field, fbe_u8_t *data, fbe_u32_t size)
{
    return sas_drive_set_default_field(drive_type, field, data, size);
}

//fbe_status_t terminator_get_device_id(fbe_terminator_device_ptr_t handle,
//                                      fbe_terminator_api_device_handle_t *device_id)
//{
//    if (handle == NULL)
//    {
//        return FBE_STATUS_GENERIC_FAILURE;
//    }
//    else
//    {
//        *device_id = base_component_get_id(handle);
//        return FBE_STATUS_OK;
//    }
//}
//
fbe_status_t terminator_lock_port(fbe_terminator_device_ptr_t port_handle)
{
    if (port_handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    port_lock_update_lock(port_handle);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_unlock_port(fbe_terminator_device_ptr_t port_handle)
{
    if (port_handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    port_unlock_update_lock(port_handle);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_drive_increment_error_count(fbe_terminator_device_ptr_t handle)
{
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        drive_increment_error_count(handle);
        return FBE_STATUS_OK;
    }
}
//fbe_status_t terminator_drive_get_error_count(fbe_u32_t port_number,
//                                              fbe_terminator_api_device_handle_t drive_id,
//                                              fbe_u32_t *const error_count_p)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * matching_device = NULL;
//    terminator_port_t * matching_port = NULL;
//
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//    //matching_device = port_get_device_by_device_id(matching_port, drive_id);
//    if (matching_device == NULL)
//    {
//        return status;
//    }
//    /* make sure this is a drive */
//    if (base_component_get_component_type(matching_device)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
//    {
//        return status;
//    }
//    *error_count_p = drive_get_error_count((terminator_drive_t*)matching_device);
//    return FBE_STATUS_OK;
//}

//fbe_status_t terminator_drive_clear_error_count(fbe_u32_t port_number,
//                                                fbe_terminator_api_device_handle_t drive_id)
//{  fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * matching_device = NULL;
//    terminator_port_t * matching_port = NULL;
//
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//    //matching_device = port_get_device_by_device_id(matching_port, drive_id);
//    if (matching_device == NULL)
//    {
//        return status;
//    }
//    /* make sure this is a drive */
//    if (base_component_get_component_type(matching_device)!=TERMINATOR_COMPONENT_TYPE_DRIVE)
//    {
//        return status;
//    }
//    drive_clear_error_count((terminator_drive_t*)matching_device);
//    return FBE_STATUS_OK;
//}
/*********************************************************************
*        terminator_mark_page_unsupported()
*********************************************************************
*
*  This functions marks the given page as unsupported for the ESES
*   code.
*
*  Inputs:
*   cdb_opcode: "receive diagnostic" or "send diagnostic" opcode
*       indicating either "status" page or "control" page.
*   diag_page_code: Corresponding  diagnostic page code to be
*       to be marked as unsupported.
*
*  Return Value: success or failure
*
*  History:
*    Oct08 created
*
*********************************************************************/
fbe_status_t terminator_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code)
{
    return(sas_virtual_phy_mark_eses_page_unsupported(cdb_opcode, diag_page_code));
}

/*********************************************************************
*        terminator_is_eses_page_supported()
*********************************************************************
*
*  This functions determines if the given page is supported or not
*   (from the user perspective as we provide an API for the user
*    to be able to Unsupport and support the existing pages).
*
*  Inputs:
*   cdb_opcode: "receive diagnostic" or "send diagnostic" opcode
*       indicating either "status" page or "control" page.
*   diag_page_code: Corresponding  diagnostic page code to be
*       to be marked as unsupported.
*
*  Return Value: success or failure
*
*  History:
*    Oct08 created
*
*********************************************************************/
fbe_bool_t terminator_is_eses_page_supported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code)
{
    return(sas_virtual_phy_is_eses_page_supported(cdb_opcode, diag_page_code));
}

/**************************************************************************
 *  terminator_virtual_phy_is_drive_slot_available()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function determines if the corresponding drive slot is available
 *      (NO drive inserted) or Unavailable (drive inserted)
 *
 *  PARAMETERS:
 *    virtual phy object handle, slot number and return boolean indicating the
 *      drive availability.
 *    drive_slot is local to the component (enclosure, or EEs)
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-20-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_virtual_phy_is_drive_slot_available(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            fbe_u8_t drive_slot,
                                                            fbe_bool_t *available)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_enclosure_t * parent_enclosure = NULL;

    *available = FBE_FALSE;

    if(base_component_get_component_type((base_component_t *)virtual_phy_handle) != TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Get parent enclosure object
    status = base_component_get_parent(virtual_phy_handle,(base_component_t **)&parent_enclosure);
    RETURN_ON_ERROR_STATUS;

    // get the slot number for the enclosure
    status = terminator_update_enclosure_drive_slot_number(virtual_phy_handle, (fbe_u32_t *)&drive_slot);
    RETURN_ON_ERROR_STATUS;

    // This function returns error status if slot is NOT available.
    status = sas_enclosure_is_slot_available(parent_enclosure, drive_slot, available);
    return(status);
}

/**************************************************************************
 *  terminator_virtual_phy_logout_drive_in_slot()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function generates a LOGOUT for the drive in the given slot.
 *
 *  PARAMETERS:
 *    virtual phy object handle and corresponding slot number
 *    drive_slot is local to the component (enclosure, or EEs)
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      Returns an error if NO drive is inserted in the slot.
 *
 *  HISTORY:
 *      Oct-20-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_virtual_phy_logout_drive_in_slot(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                         fbe_u8_t drive_slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_terminator_device_ptr_t port_handle = NULL;
    terminator_enclosure_t * parent_enclosure = NULL;
    fbe_terminator_device_ptr_t drive_ptr;
    fbe_u32_t port_number;

    if(base_component_get_component_type((base_component_t *)virtual_phy_handle) != TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Get the corresponding Port object
    status = terminator_get_port_ptr(virtual_phy_handle, &port_handle);
    RETURN_ON_ERROR_STATUS;

    // Get the corresponding parent enclosure object
    status = base_component_get_parent(virtual_phy_handle, (base_component_t **)&parent_enclosure);
    RETURN_ON_ERROR_STATUS;

    // get the slot number for the enclosure
    status = terminator_update_enclosure_drive_slot_number(virtual_phy_handle, (fbe_u32_t *)&drive_slot);
    RETURN_ON_ERROR_STATUS;

    // Get drive device id from the enclosure object
    status = sas_enclosure_get_device_ptr_by_slot(parent_enclosure, drive_slot, &drive_ptr);
    RETURN_ON_ERROR_STATUS;
    if(drive_ptr == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Add the drive object to the logout queue
    status = port_logout_device((terminator_port_t *)port_handle, drive_ptr);
    RETURN_ON_ERROR_STATUS;

    status = port_get_miniport_port_index((terminator_port_t *)port_handle, &port_number);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    // Generate a logout for the drive by waking up the event thread
    status = fbe_terminator_miniport_api_device_logged_out(port_number, drive_ptr);
    return status;
}



/**************************************************************************
 *  terminator_virtual_phy_login_drive_in_slot()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function generates a LOGIN for the drive in the given slot.
 *
 *  PARAMETERS:
 *    virtual phy object handle and corresponding slot number
 *    drive_slot is local to the component (enclosure, or EEs)
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      Returns an error if NO drive is inserted in the slot.
 *
 *  HISTORY:
 *      Oct-20-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_virtual_phy_login_drive_in_slot(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                        fbe_u8_t drive_slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_terminator_device_ptr_t port_handle = NULL;
    terminator_enclosure_t * parent_enclosure = NULL;
    fbe_terminator_device_ptr_t drive_ptr;
    fbe_u32_t port_number;

    if(base_component_get_component_type((base_component_t *)virtual_phy_handle) != TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Get the corresponding Port object
    status = terminator_get_port_ptr(virtual_phy_handle, &port_handle);
    RETURN_ON_ERROR_STATUS;

    // Get the corresponding parent enclosure object
    status = base_component_get_parent(virtual_phy_handle, (base_component_t **)&parent_enclosure);
    RETURN_ON_ERROR_STATUS;

    // get the slot number for the enclosure
    status = terminator_update_enclosure_drive_slot_number(virtual_phy_handle, (fbe_u32_t *)&drive_slot);
    RETURN_ON_ERROR_STATUS;

    // Get drive device id from the enclosure object
    status = sas_enclosure_get_device_ptr_by_slot(parent_enclosure, drive_slot, &drive_ptr);
    RETURN_ON_ERROR_STATUS;

    // Add the drive object to the logout queue
    status = port_login_device((terminator_port_t *)port_handle, drive_ptr);
    RETURN_ON_ERROR_STATUS;

    status = port_get_miniport_port_index((terminator_port_t *)port_handle, &port_number);
    RETURN_ON_ERROR_STATUS;

    // Generate a login for the drive by waking up the event thread.
    status = fbe_terminator_miniport_api_device_state_change_notify(port_number);
    return status;
}

fbe_status_t terminator_check_sas_enclosure_type(fbe_sas_enclosure_type_t encl_type)
{
    return(sas_virtual_phy_check_enclosure_type(encl_type));
}

/**************************************************************************
 *  terminator_get_device_state()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function returns the state of a particular device (with respect
 *       to LOGIN/LOGOUT)
 *
 *  PARAMETERS:
 *     Port number, device id and state to be returned.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-20-2008: Rajesh V created.
 **************************************************************************/
//fbe_status_t terminator_get_device_state(fbe_u32_t port_number,
//                                         fbe_terminator_api_device_handle_t device_id,
//                                         terminator_component_state_t *state)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t *device = NULL;
//    terminator_port_t * matching_port = NULL;
//
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return status;
//    }
//
//    //device = port_get_device_by_device_id(matching_port, device_id);
//    if(device == NULL)
//    {
//        return(status);
//    }
//
//    *state = base_component_get_state(device);
//
//    return(FBE_STATUS_OK);
//}

/**************************************************************************
 *  terminator_virtual_phy_is_drive_in_slot_logged_in()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function indicates if the drive in corresponding slot is logged
 *      in or not.
 *
 *  PARAMETERS:
 *      Virtual phy object handle, slot number, boolean indicating drive
 *          logged in status(TRUE-logged in, FALSE otherwise)
 *    drive_slot is local to the component (enclosure, or EEs)
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *      Returns an error if the slot passed in does not have a drive inserted.
 *
 *  HISTORY:
 *      Oct-20-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_virtual_phy_is_drive_in_slot_logged_in(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                               fbe_u8_t drive_slot,
                                                               fbe_bool_t *drive_logged_in)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_terminator_device_ptr_t port_handle = NULL;
    terminator_enclosure_t * parent_enclosure = NULL;
    terminator_component_state_t drive_state;
    fbe_terminator_device_ptr_t drive_ptr;

    *drive_logged_in = FBE_FALSE;

    if(base_component_get_component_type((base_component_t *)virtual_phy_handle) != TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    // Get the corresponding Port object
    status = terminator_get_port_ptr(virtual_phy_handle, &port_handle);
    RETURN_ON_ERROR_STATUS;

    // Get the corresponding parent enclosure object
    status = base_component_get_parent(virtual_phy_handle, (base_component_t **)&parent_enclosure);
    RETURN_ON_ERROR_STATUS;

    // Get the slot number for the enclosure
    status = terminator_update_enclosure_drive_slot_number(virtual_phy_handle, (fbe_u32_t *)&drive_slot);
    RETURN_ON_ERROR_STATUS;

    // Get drive device id from the enclosure object
    status = sas_enclosure_get_device_ptr_by_slot(parent_enclosure, drive_slot, &drive_ptr);
    RETURN_ON_ERROR_STATUS;
    if(drive_ptr == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

     drive_state =  base_component_get_state((base_component_t *)drive_ptr);

     if((drive_state == TERMINATOR_COMPONENT_STATE_LOGIN_PENDING) ||
        (drive_state == TERMINATOR_COMPONENT_STATE_LOGIN_COMPLETE))
     {
        *drive_logged_in = FBE_TRUE;  // or drive about to login
     }

     return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_block_size(fbe_terminator_device_ptr_t handle,
                                      fbe_block_size_t * block_size)
{
    * block_size = 0;
    if (handle == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

fbe_status_t terminator_get_port_ptr_by_port_portal(fbe_u32_t io_port_number, fbe_u32_t io_portal_number, fbe_terminator_device_ptr_t *handle)
{
    *handle = base_component_get_first_child(&board_object->base);
    while (*handle != NULL)
    {
        if (port_is_matching(*handle, io_port_number, io_portal_number) == FBE_TRUE)
            break;
        *handle = base_component_get_next_child(&board_object->base, *handle);
    }
    return FBE_STATUS_OK;
}

/*The following are the new interfaces */
fbe_status_t terminator_create_board(fbe_terminator_device_ptr_t *device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_board_t * new_board_handle;
    void * attributes;
    if((new_board_handle = board_new()) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Assign ID for board device - do we need this in new API? Old API performed this */
    //base_component_assign_id(&new_board_handle->base, base_generate_device_id());
   //base_reset_device_id();
    /* allocate memory for attributes */
    if((attributes = allocate_board_info()) == NULL)
    {
        fbe_terminator_free_memory(new_board_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    base_component_assign_attributes(&new_board_handle->base, attributes);
    *device_ptr = new_board_handle;
    status = FBE_STATUS_OK;
    return status;
}
fbe_status_t terminator_create_port(const char * type,fbe_terminator_device_ptr_t *device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t * new_port;
    
    if((new_port = port_new()) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set port protocol to invalid - creation routine must set it explicitly */
    status = port_set_type(new_port, FBE_PORT_TYPE_INVALID);
    
    base_component_assign_attributes(&new_port->base, NULL);
    base_component_set_component_type(&new_port->base, TERMINATOR_COMPONENT_TYPE_PORT);
    *device_ptr = new_port;

    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_create_enclosure(const char * type,fbe_terminator_device_ptr_t *device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    terminator_enclosure_t * new_enclosure;

    if((new_enclosure = enclosure_new()) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* set enclosure protocol to invalid - creation routine must set it explicitly */
    status = enclosure_set_protocol(new_enclosure, FBE_ENCLOSURE_TYPE_INVALID);

    base_component_assign_attributes(&new_enclosure->base, NULL);
    base_component_set_component_type(&new_enclosure->base, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
  
    /* return handle */
    *device_ptr = new_enclosure;
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_create_drive(fbe_terminator_device_ptr_t *device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    //fbe_u64_t drive_id;
    terminator_drive_t * new_drive_handle;

    if((new_drive_handle = drive_new()) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* set drive protocol to invalid - creation routine must set it explicitly */
    drive_set_protocol(new_drive_handle, FBE_DRIVE_TYPE_INVALID);
    /* generate and assign ID */
    //drive_id = base_generate_device_id();
    //base_component_assign_id(&new_drive_handle->base, drive_id);
    /* attributes depend on drive's protocol, caller should use another function to initialize them */
    base_component_assign_attributes(&new_drive_handle->base, NULL);
    /* return handle to newly created drive */
    *device_ptr = new_drive_handle;
    status = FBE_STATUS_OK;
    return status;
}

fbe_status_t terminator_destroy_drive(fbe_terminator_device_ptr_t device_handle)
{
    terminator_drive_t * drive = (terminator_drive_t *)device_handle;
    drive_free(drive);

    return FBE_STATUS_OK;
}

fbe_status_t terminator_set_device_attributes(fbe_terminator_device_ptr_t device_ptr, void * attributes)
{
    base_component_assign_attributes(device_ptr, attributes);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_insert_device(fbe_terminator_device_ptr_t parent_device,
                                      fbe_terminator_device_ptr_t child_device)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    terminator_component_type_t type = base_component_get_component_type(child_device);
    /* insert device into parent */
    switch(type)
    {
    case TERMINATOR_COMPONENT_TYPE_BOARD:
        /* boards are not inserted */
        break;

    case TERMINATOR_COMPONENT_TYPE_PORT:
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
    case TERMINATOR_COMPONENT_TYPE_DRIVE:
        status = terminator_insert_device_new(parent_device, child_device);
        break;

    default:
        //need proper trace statement here, maybe
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;
}

fbe_status_t terminator_insert_device_new(fbe_terminator_device_ptr_t parent_device,
                                      fbe_terminator_device_ptr_t child_device)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_queue_element_t * element;
    terminator_component_type_t parent_type;
    terminator_component_type_t child_type;
    fbe_enclosure_type_t encl_protocol;
    fbe_u8_t max_conn_id_count = 0;

    /* obtain type for both of them*/
    parent_type = base_component_get_component_type(parent_device);
    child_type = base_component_get_component_type(child_device);
    /* additional efforts needed in that case */
    if(parent_type == TERMINATOR_COMPONENT_TYPE_PORT &&
        child_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        encl_protocol = enclosure_get_protocol((terminator_enclosure_t *)child_device);
        switch(encl_protocol)
        {
            case FBE_ENCLOSURE_TYPE_SAS:
                status = port_add_sas_enclosure(parent_device, child_device, FBE_TRUE);
                if(status == FBE_STATUS_OK)
                {
                    return sas_enclosure_set_logical_parent_type(child_device, TERMINATOR_COMPONENT_TYPE_PORT);
                }
            case FBE_ENCLOSURE_TYPE_FIBRE:
                return port_add_fc_enclosure(parent_device, child_device);
            break;
            default:
                return FBE_STATUS_GENERIC_FAILURE; 
        }        
    }

    if(parent_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE &&
        child_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        terminator_sas_enclosure_info_t *child_attributes = base_component_get_attributes(&((terminator_enclosure_t *)child_device)->base),
                *parent_attributes = base_component_get_attributes(&((terminator_enclosure_t *)parent_device)->base);

        status = terminator_max_conn_id_count(parent_attributes->enclosure_type, &max_conn_id_count);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }

        if(child_attributes->connector_id < max_conn_id_count)
        {
            fbe_bool_t result;

            status = sas_enclosure_is_connector_available(parent_device, child_attributes->connector_id, &result);
            if(status != FBE_STATUS_OK || !result)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        status = enclosure_add_sas_enclosure(parent_device, child_device, FBE_TRUE);
        if(status != FBE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        return sas_enclosure_set_logical_parent_type(child_device, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
    }

    /* additional efforts needed in that case */
    if(parent_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE &&
        child_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        fbe_bool_t is_slot_available = FBE_FALSE;
        /* extract slot number from drive attributes */
        fbe_u32_t slot_number;
        fbe_drive_type_t drive_type;
        void * attributes = base_component_get_attributes(&((terminator_drive_t *)child_device)->base);
        /* make  sure attributes are available before using them */
        if(attributes == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: drive attributes are not available\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        drive_type = drive_get_protocol((terminator_drive_t *)child_device);

        switch(drive_type)
        {
        case FBE_DRIVE_TYPE_SAS:
        case FBE_DRIVE_TYPE_SAS_FLASH_HE:
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            slot_number = ((terminator_sas_drive_info_t *)attributes)->slot_number;
            if(sas_enclosure_is_slot_available(parent_device, slot_number, &is_slot_available) != FBE_STATUS_OK
               || is_slot_available != FBE_TRUE)
            {
                terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: slot %d is not available\n", __FUNCTION__, slot_number);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
        case FBE_DRIVE_TYPE_SATA:
        case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            slot_number = ((terminator_sata_drive_info_t *)attributes)->slot_number;
            if(sas_enclosure_is_slot_available(parent_device, slot_number, &is_slot_available) != FBE_STATUS_OK
                || is_slot_available != FBE_TRUE)
            {
                terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: slot %d is not available\n", __FUNCTION__, slot_number);

                return FBE_STATUS_GENERIC_FAILURE;
            }
        break;
        case FBE_DRIVE_TYPE_FIBRE:
            slot_number = ((terminator_fc_drive_info_t *)attributes)->slot_number;
                if(fc_enclosure_is_slot_available(parent_device, slot_number, &is_slot_available) != FBE_STATUS_OK
                    || is_slot_available != FBE_TRUE)
                {
                    terminator_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: slot %d is not available\n", __FUNCTION__, slot_number);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            break;    
        default:
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Set enclosure state change to true for only FC enclosure */
        encl_protocol = enclosure_get_protocol((terminator_enclosure_t *)parent_device);
        if(encl_protocol == FBE_ENCLOSURE_TYPE_FIBRE && drive_type == FBE_DRIVE_TYPE_FIBRE)
        {
            status = fc_enclosure_set_state_change((terminator_enclosure_t *)parent_device, FBE_TRUE);
        }

        if(((base_component_t*)child_device)->component_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
        {
            set_terminator_simulated_drive_flag_to_drive_reinserted((base_component_t*)child_device);
        }
        /* these two lines fix sobo_4 */
        element = base_component_get_queue_element(child_device);
        return base_component_add_child_to_front(parent_device, element);
    }
    element = base_component_get_queue_element(child_device);
    return base_component_add_child(parent_device, element);
}

fbe_status_t terminator_activate_device(fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t status;

    fbe_u32_t port_number = -1;
    fbe_u32_t enclosure_number = -1;
    fbe_u32_t slot_number = -1;

    fbe_drive_type_t drive_protocol;


    terminator_component_type_t type = base_component_get_component_type(device_ptr);
    void *attributes                 = base_component_get_attributes(device_ptr);
    if(attributes == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    switch(type)
    {
    case TERMINATOR_COMPONENT_TYPE_BOARD:
        status = terminator_activate_board(device_ptr);
        break;

    case TERMINATOR_COMPONENT_TYPE_PORT:
        status = port_activate(device_ptr);
        break;

    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        status = enclosure_activate(device_ptr);
        break;

    case TERMINATOR_COMPONENT_TYPE_DRIVE:
        drive_protocol = drive_get_protocol((terminator_drive_t *)device_ptr);

        switch(drive_protocol)
        {
            case FBE_DRIVE_TYPE_SAS:
            case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            case FBE_DRIVE_TYPE_SAS_FLASH_RI:
                port_number      = ((terminator_sas_drive_info_t  *)attributes)->backend_number;
                enclosure_number = ((terminator_sas_drive_info_t  *)attributes)->encl_number;
                slot_number = ((terminator_sas_drive_info_t  *)attributes)->slot_number;
                break;    
            case FBE_DRIVE_TYPE_SATA:
            case FBE_DRIVE_TYPE_SATA_FLASH_HE:
                port_number      = ((terminator_sata_drive_info_t  *)attributes)->backend_number;
                enclosure_number = ((terminator_sata_drive_info_t  *)attributes)->encl_number;
                slot_number = ((terminator_sata_drive_info_t  *)attributes)->slot_number;
                break;

            case FBE_DRIVE_TYPE_FIBRE:
                port_number      = ((terminator_fc_drive_info_t  *)attributes)->backend_number;
                enclosure_number = ((terminator_fc_drive_info_t  *)attributes)->encl_number;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                break;      
        }
        
        /* Temp Hack: for Titan ZFRT, we generate the drives on file since the InMemory drives are too small for Flare DB*/
        /* check the io mode.  If the io is enabled then create the drive file */
        if ((terminator_io_mode == FBE_TERMINATOR_IO_MODE_ENABLED)&&
            (terminator_simulated_drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_LOCAL_FILE))
        {
            /* for use with drive file*/
            fbe_u8_t drive_name[TERMINATOR_DISK_FULLPATH_STRING_SIZE];
            fbe_u64_t drive_size_in_bytes;
            int count = 0;

            /* Calculate the file size base on the capacity and block_size.
             */
            drive_size_in_bytes = drive_get_capacity((terminator_drive_t *)device_ptr) * drive_get_block_size((terminator_drive_t *)device_ptr);

            /* Create the disk file name.
            * The disk naming convention ( and thus the corresponding file name) is
            * "disk<Port number>_<Enclosure_number>_<slot_number>"
            */
            fbe_zero_memory(drive_name, sizeof(drive_name));
            count = _snprintf(drive_name, sizeof(drive_name), TERMINATOR_DISK_FULLPATH_FORMAT_STRING,
                terminator_file_api_get_disk_storage_dir(), port_number, enclosure_number, slot_number);
            if (( count < 0 ) || (sizeof(drive_name) == count )) {
                fbe_debug_break();
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Create the disk file.
            */
            status = terminator_create_disk_file(drive_name, drive_size_in_bytes);
            if(status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s, failed to create drive file\n", __FUNCTION__);
                return status;
            }
        }

        // create the remote drive file for TERMINATOR_SIMULATED_DRIVE_TYPE_VMWARE_REMOTE_FILE mode
        //used for VNX VMware simualtion mode only.
        if ((terminator_io_mode == FBE_TERMINATOR_IO_MODE_ENABLED)&&
                (terminator_simulated_drive_type == TERMINATOR_SIMULATED_DRIVE_TYPE_VMWARE_REMOTE_FILE))
        {
            fbe_lba_t max_lba = drive_get_capacity((terminator_drive_t *)device_ptr);
            fbe_block_size_t block_size = drive_get_block_size((terminator_drive_t *)device_ptr);

            status = terminator_simulated_disk_remote_file_simple_create(port_number, 
                    enclosure_number, slot_number, block_size, max_lba);
            if(status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s, failed to create drive file\n", __FUNCTION__);
                return status;
            }
        }


        /* now activate it */
        switch(drive_protocol)
        {
            case FBE_DRIVE_TYPE_SAS:
            case FBE_DRIVE_TYPE_SAS_FLASH_HE: 
            case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            case FBE_DRIVE_TYPE_SATA:
            case FBE_DRIVE_TYPE_SATA_FLASH_HE:
                status = drive_activate(device_ptr);
                break;
            /* FC drive don't required login as SAS drive. Incase of FC drive we will initiate
               loop-need-discovery event. Currently it is not implemented */
            case FBE_DRIVE_TYPE_FIBRE:
                status = FBE_STATUS_OK;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                break;      
        }           
        break;

    default:
        //need proper trace statement here, maybe
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }


    return status;
}

static fbe_status_t terminator_activate_board(terminator_board_t * self)
{
    board_object = self;
    return FBE_STATUS_OK;
}

/* Sata Drive */
fbe_status_t terminator_create_sata_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sata_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t is_slot_available = FBE_FALSE;

    if (encl_ptr == NULL)
    {
        return status;
    }
    /* make sure this is an enclosure */
    if (base_component_get_component_type(encl_ptr)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return status;
    }

    /* check if the slot number is valid and not in use */
    status = sas_enclosure_is_slot_available((terminator_enclosure_t *)encl_ptr, slot_number, &is_slot_available);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    if(is_slot_available != FBE_TRUE)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    status = sas_enclosure_insert_sata_drive((terminator_enclosure_t *)encl_ptr, slot_number, drive_info, drive_ptr);
    return status;
}

fbe_status_t terminator_insert_sata_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sata_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    terminator_port_t * matching_port = NULL;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct exp_phy_stat;
    base_component_t *virtual_phy = NULL;
    fbe_u8_t phy_id;
    fbe_sas_enclosure_type_t encl_type;

    /* 1) add the drive to lot */
    status = terminator_create_sata_drive(encl_ptr, slot_number, drive_info, drive_ptr);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    /* 2) set the drive insert-bit in enclosure */
    /* make sure this is an enclosure */
    if (base_component_get_component_type(encl_ptr)!=TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        return status;
    }

    // Get the related virtual phy  of the enclosure
    virtual_phy =
        base_component_get_child_by_type(encl_ptr, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    if(virtual_phy == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    // update if needed
    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t *)&virtual_phy, &slot_number);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    //fbe_mutex_lock(&matching_port->update_mutex);

    fbe_zero_memory(&drive_slot_stat, sizeof(ses_stat_elem_array_dev_slot_struct));
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    status = sas_virtual_phy_set_drive_eses_status((terminator_virtual_phy_t *)virtual_phy,
        slot_number, drive_slot_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set enclosure_drive_status failed!\n", __FUNCTION__);
       // fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }

    fbe_zero_memory(&exp_phy_stat, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat.link_rdy = 0x1;
    exp_phy_stat.phy_rdy = 0x1;

    status = terminator_virtual_phy_get_enclosure_type(virtual_phy, &encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s terminator_virtual_phy_get_enclosure_type failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }

    status = sas_virtual_phy_get_drive_slot_to_phy_mapping(slot_number, &phy_id, encl_type);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_slot_to_phy_mapping failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }
    status = sas_virtual_phy_set_phy_eses_status((terminator_virtual_phy_t *)virtual_phy, phy_id, exp_phy_stat);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set enclosure_drive_status failed!\n", __FUNCTION__);
        //fbe_mutex_unlock(&matching_port->update_mutex);
        return status;
    }

    //fbe_mutex_unlock(&matching_port->update_mutex);
    /* 3) set the login_pending flag, terminator api will wake up the mini_port_api thread to process the login */
    status = terminator_set_device_login_pending(*drive_ptr);
    if (status != FBE_STATUS_OK)
    {
        //
        //terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set_login_pending failed on device %d!\n", __FUNCTION__, *drive_ptr);
    }
    return status;
}

fbe_status_t terminator_eses_increment_config_page_gen_code(fbe_terminator_device_ptr_t virtual_phy_handle)
{
    sas_virtual_phy_increment_gen_code(virtual_phy_handle);
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_initialize_eses_page_info(fbe_sas_enclosure_type_t encl_type, terminator_vp_eses_page_info_t *eses_page_info)
{
    return(eses_initialize_eses_page_info(encl_type, eses_page_info));
}

fbe_status_t terminator_get_eses_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                           terminator_vp_eses_page_info_t **vp_eses_page_info)
{
    return(sas_virtual_phy_get_eses_page_info(virtual_phy_handle, vp_eses_page_info));
}

fbe_status_t terminator_eses_set_ver_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        ses_ver_desc_struct ver_desc)
{
    return(sas_virtual_phy_set_ver_desc(virtual_phy_handle, subencl_type, side, comp_type, ver_desc));
}

fbe_status_t terminator_eses_set_ver_num(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        CHAR *ver_num)
{
    return(sas_virtual_phy_set_ver_num(virtual_phy_handle, subencl_type, side, comp_type, ver_num));
}

fbe_status_t terminator_eses_get_ver_num(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        CHAR *ver_num)
{
    return(sas_virtual_phy_get_ver_num(virtual_phy_handle, subencl_type, side, comp_type, ver_num));
}

fbe_status_t terminator_set_enclosure_firmware_activate_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t time_interval)
{
    return sas_virtual_phy_set_activate_time_interval(virtual_phy_ptr, time_interval);
}

fbe_status_t terminator_get_enclosure_firmware_activate_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t *time_interval)
{
    return sas_virtual_phy_get_activate_time_interval(virtual_phy_ptr, time_interval);
}

fbe_status_t terminator_set_enclosure_firmware_reset_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t time_interval)
{
    return sas_virtual_phy_set_reset_time_interval(virtual_phy_ptr, time_interval);
}

fbe_status_t terminator_get_enclosure_firmware_reset_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t *time_interval)
{
    return sas_virtual_phy_get_reset_time_interval(virtual_phy_ptr, time_interval);
}

fbe_status_t terminator_eses_get_subencl_ver_desc_position(terminator_vp_eses_page_info_t *eses_page_info,
                                                        ses_subencl_type_enum subencl_type,
                                                        terminator_eses_subencl_side side,
                                                        fbe_u8_t *ver_desc_start_index,
                                                        fbe_u8_t *num_ver_descs)
{
    return(eses_get_subencl_ver_desc_position(eses_page_info,
                                              subencl_type,
                                              side,
                                              ver_desc_start_index,
                                              num_ver_descs));
}



fbe_status_t terminator_eses_set_download_microcode_stat_page_stat_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                         fbe_download_status_desc_t download_stat_desc)
{
    return(sas_virtual_phy_set_download_microcode_stat_desc(virtual_phy_handle, download_stat_desc));
}

fbe_status_t terminator_eses_get_download_microcode_stat_page_stat_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                         fbe_download_status_desc_t *download_stat_desc)
{
    return(sas_virtual_phy_get_download_microcode_stat_desc(virtual_phy_handle, download_stat_desc));
}

fbe_status_t terminator_eses_get_vp_download_microcode_ctrl_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                      vp_download_microcode_ctrl_diag_page_info_t **vp_download_ctrl_page_info)
{
    return(sas_virtual_phy_get_vp_download_microcode_ctrl_page_info(virtual_phy_handle, vp_download_ctrl_page_info));
}

fbe_status_t terminator_eses_set_download_status(fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t status)
{
    return(sas_virtual_phy_set_firmware_download_status(virtual_phy_handle, status));
}

fbe_status_t terminator_eses_get_download_status(fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t *status)
{
    return(sas_virtual_phy_get_firmware_download_status(virtual_phy_handle, status));
}

fbe_status_t terminator_eses_get_ver_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        ses_ver_desc_struct *ver_desc)
{
    return(sas_virtual_phy_get_ver_desc(virtual_phy_handle, subencl_type, side, comp_type, ver_desc));
}



fbe_status_t terminator_eses_get_subencl_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                            ses_subencl_type_enum subencl_type,
                                            terminator_eses_subencl_side side,
                                            fbe_u8_t *subencl_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_vp_eses_page_info_t *vp_eses_page_info;

    status = terminator_get_eses_page_info(virtual_phy_handle,
                                           &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    status = eses_get_subencl_id(vp_eses_page_info,
                                 subencl_type,
                                 side,
                                 subencl_id);
    return(status);
}

fbe_status_t terminator_virtual_phy_get_unit_attention(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                       fbe_u8_t *unit_attention)
{
    return(sas_virtual_phy_get_unit_attention(virtual_phy_handle, unit_attention));
}

fbe_status_t terminator_virtual_phy_set_unit_attention(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                       fbe_u8_t unit_attention)
{
    return(sas_virtual_phy_set_unit_attention(virtual_phy_handle, unit_attention));
}

/* Get the encl(Enclosure element in Local LCC Subenclosure) eses status
 */
fbe_status_t terminator_get_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_stat_elem_encl_struct *encl_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t *matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, (fbe_terminator_device_ptr_t *)&matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
     fbe_mutex_lock(&matching_port->update_mutex);


    status = sas_virtual_phy_get_encl_eses_status(virtual_phy_handle, encl_stat);

     fbe_mutex_unlock(&matching_port->update_mutex);


    return status;
}

/* Set the Encl(Enclosure element in Local LCC Subenclosure) eses status
 */
fbe_status_t terminator_set_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_stat_elem_encl_struct encl_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t *matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, (fbe_terminator_device_ptr_t *)&matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    fbe_mutex_lock(&matching_port->update_mutex);


    status = sas_virtual_phy_set_encl_eses_status(virtual_phy_handle, encl_stat);

    fbe_mutex_unlock(&matching_port->update_mutex);


    return status;
}

/* Get the Chassis(Enclosure element in Chassis Subenclosure) eses status
 */
fbe_status_t terminator_get_chassis_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                     ses_stat_elem_encl_struct *encl_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t *matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, (fbe_terminator_device_ptr_t *)&matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    fbe_mutex_lock(&matching_port->update_mutex);

    status = sas_virtual_phy_get_chassis_encl_eses_status(virtual_phy_handle, encl_stat);

    fbe_mutex_unlock(&matching_port->update_mutex);

    return status;
}

/* Set the Chassis(Enclosure element in Chassis Subenclosure) eses status
 */
fbe_status_t terminator_set_chassis_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                     ses_stat_elem_encl_struct encl_stat)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t *matching_port = NULL;

    terminator_get_port_ptr(virtual_phy_handle, (fbe_terminator_device_ptr_t *)&matching_port);
    if (matching_port == NULL)
    {
        return status;
    }

    /* make sure this is a virtual phy */
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    fbe_mutex_lock(&matching_port->update_mutex);

    status = sas_virtual_phy_set_chassis_encl_eses_status(virtual_phy_handle, encl_stat);

    fbe_mutex_unlock(&matching_port->update_mutex);

    return status;
}

fbe_status_t terminator_register_device_reset_function(fbe_terminator_device_ptr_t device_ptr,
                                                       fbe_terminator_device_reset_function_t reset_function)
{
    base_component_set_reset_function(device_ptr, reset_function);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_get_device_reset_function(fbe_terminator_device_ptr_t device_ptr,
                                                  fbe_terminator_device_reset_function_t *reset_function)
{
    *reset_function = base_component_get_reset_function(device_ptr);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_set_device_reset_delay(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t delay_in_ms)
{
    base_component_set_reset_delay(device_ptr, delay_in_ms);
    return FBE_STATUS_OK;
}
fbe_status_t terminator_get_device_reset_delay(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *delay_in_ms)
{
    *delay_in_ms = base_component_get_reset_delay(device_ptr);
    return FBE_STATUS_OK;
}

fbe_status_t terminator_logout_device(fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t *matching_port = NULL;

    status = terminator_get_port_ptr(device_ptr, (fbe_terminator_device_ptr_t *)&matching_port);
    if (matching_port != NULL)
    {
        status = port_logout_device(matching_port, device_ptr);
    }
    return status;
}

fbe_status_t terminator_login_device(fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_port_t * matching_port = NULL;

    status = terminator_get_port_ptr(device_ptr, (fbe_terminator_device_ptr_t *)&matching_port);
    if (matching_port != NULL)
    {
        status = port_login_device(matching_port, device_ptr);
    }
    return status;
}

//fbe_status_t terminator_get_device_handle_by_device_id(fbe_u32_t port_number,
//                                                       fbe_terminator_api_device_handle_t device_id,
//                                                       fbe_terminator_device_ptr_t *handle)
//{
//    terminator_port_t* matching_port;
//    matching_port = board_get_port_by_number(board_object, port_number);
//    if (matching_port == NULL)
//    {
//        return FBE_STATUS_GENERIC_FAILURE;
//    }
//    //*handle = port_get_device_by_device_id(matching_port, device_id);
//    return FBE_STATUS_OK;
//}

/**************************************************************************
 *  terminator_eses_set_buf()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the data in a particular buffer in the given
 *    subenclosure.
 *
 *  PARAMETERS:
 *    virtual phy object handle, subenclosure type, subenclosure side, buffer
 *    type of the buffer to be set, other buffer attributes to be considered,
 *    buffer to be copied from, length of the buffer from which data is to be copied.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jan-09-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_eses_set_buf(fbe_terminator_device_ptr_t virtual_phy_handle,
                                    ses_subencl_type_enum subencl_type,
                                    fbe_u8_t side,
                                    ses_buf_type_enum buf_type,
                                    fbe_bool_t consider_writable,
                                    fbe_u8_t writable,
                                    fbe_bool_t consider_buf_index,
                                    fbe_u8_t buf_index,
                                    fbe_bool_t consider_buf_spec_info,
                                    fbe_u8_t buf_spec_info,
                                    fbe_u8_t *buf,
                                    fbe_u32_t len)
{
    fbe_u8_t buf_id;
    terminator_vp_eses_page_info_t *vp_eses_page_info;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    status = eses_get_buf_id_by_subencl_info(vp_eses_page_info, 
                                             subencl_type, 
                                             side, 
                                             buf_type,
                                             consider_writable,
                                             writable,
                                             consider_buf_index,
                                             buf_index,
                                             consider_buf_spec_info,
                                             buf_spec_info,
                                             &buf_id);
    RETURN_ON_ERROR_STATUS;

    status = sas_virtual_phy_set_buf(virtual_phy_handle, buf_id, buf, len);
    return(status);
}


/**************************************************************************
 *  terminator_get_buf_id_by_subencl_info()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets the ID of the particular buffer in the given
 *    subenclosure.
 *
 *  PARAMETERS:
 *    virtual phy object handle, subenclosure type, subenclosure side, buffer
 *    type of the buffer , other buffer attributes to be considered,
 *    buffer Id to be returned.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-16-2009: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_get_buf_id_by_subencl_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                   ses_subencl_type_enum subencl_type,
                                                   fbe_u8_t side,
                                                   ses_buf_type_enum buf_type,
                                                   fbe_bool_t consider_writable,
                                                   fbe_u8_t writable,
                                                   fbe_bool_t consider_buf_index,
                                                   fbe_u8_t buf_index,
                                                   fbe_bool_t consider_buf_spec_info,
                                                   fbe_u8_t buf_spec_info,
                                                   fbe_u8_t *buf_id)
{
    terminator_vp_eses_page_info_t *vp_eses_page_info;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = terminator_get_eses_page_info(virtual_phy_handle, &vp_eses_page_info);
    RETURN_ON_ERROR_STATUS;

    status = eses_get_buf_id_by_subencl_info(vp_eses_page_info, 
                                             subencl_type, 
                                             side, 
                                             buf_type,
                                             consider_writable,
                                             writable,
                                             consider_buf_index,
                                             buf_index,
                                             consider_buf_spec_info,
                                             buf_spec_info,
                                             buf_id);
    return(status);
}

/**************************************************************************
 *  terminator_set_bufid2_buf_area_to_bufid1()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the buffer area of buffer with buf_id2 to point to
 *    the buffer area of buffer with buf_id1. So any reads or writes to
 *    either of these two buffers operate on the SAME buffer area(memory).
 *
 *  PARAMETERS:
 *    virtual phy object handle,
 *    buf_id1 - The buffer ID of the first buffer
 *    buf_id2 - The buffer ID of the second buffer
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Oct-16-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_set_bufid2_buf_area_to_bufid1(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                      fbe_u8_t buf_id1,
                                                      fbe_u8_t buf_id2)
{
    return(sas_virtual_phy_set_bufid2_buf_area_to_bufid1(virtual_phy_handle, 
                                                         buf_id1, 
                                                         buf_id2));
}

/**************************************************************************
 *  terminator_eses_set_buf_by_buf_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets(writes) the data in the buffer with the given buffer ID.
 *
 *  PARAMETERS:
 *    virtual phy object handle, Buffer ID of the buffer to be set, buffer to be
 *    copied from, length of the buffer from which data is to be copied.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jan-26-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_eses_set_buf_by_buf_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                            fbe_u8_t buf_id,
                                            fbe_u8_t *buf,
                                            fbe_u32_t len)
{
    return(sas_virtual_phy_set_buf(virtual_phy_handle, buf_id, buf, len));
}


/**************************************************************************
 *  terminator_eses_get_buf_info_by_buf_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets the pointer to the "buffer information"(which contains
 *    various attributes of the buffer and the buffer itself) of a buffer.
 *
 *  PARAMETERS:
 *    virtual phy object handle, buffer Identifier, pointer to the buffer info.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jan-09-2008: Rajesh V created.
 **************************************************************************/
fbe_status_t terminator_eses_get_buf_info_by_buf_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u8_t buf_id,
                                                    terminator_eses_buf_info_t **buf_info)
{
    return(sas_virtual_phy_get_buf_info_by_buf_id(virtual_phy_handle, buf_id, buf_info));
}

/**************************************************************************
 *  terminator_add_device_to_miniport_sas_device_table()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function adds any entry to the miniport sas device table and
 *  returns the index of this device.
 *
 *  PARAMETERS:
 *    port object handle, device_id, the index (OUT).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_add_device_to_miniport_sas_device_table(const fbe_terminator_device_ptr_t device_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       port_index;
    base_component_t *base_ptr = NULL;
    terminator_component_type_t comp_type;
    void * attributes = NULL;
    fbe_drive_type_t drive_type;

    if (device_handle != NULL)
    {
        base_ptr = (base_component_t *)device_handle;
        /* get the port number of this device */
        status = terminator_get_port_miniport_port_index(device_handle, &port_index);

        /* Get Address of miniport_sas_device_index according to component type.*/ 
        comp_type =  base_component_get_component_type((base_component_t *) device_handle);
        attributes =  base_component_get_attributes(device_handle);

        switch(comp_type)
        {
            case TERMINATOR_COMPONENT_TYPE_PORT:
                status = fbe_terminator_miniport_api_device_table_add(port_index, 
                                                  device_handle,
                                                  (&((terminator_sas_port_info_t *)attributes)->miniport_sas_device_table_index));            
            break;

            case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
                status = fbe_terminator_miniport_api_device_table_add (port_index,
                                                  device_handle,
                                                  (&((terminator_sas_enclosure_info_t *)attributes)->miniport_sas_device_table_index));            
            break;

            case TERMINATOR_COMPONENT_TYPE_DRIVE:
                drive_type = drive_get_protocol((terminator_drive_t *)device_handle); 

                if (drive_type == FBE_DRIVE_TYPE_SAS ||
                    drive_type == FBE_DRIVE_TYPE_SAS_FLASH_HE ||
                    drive_type == FBE_DRIVE_TYPE_SAS_FLASH_ME ||
                    drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE ||
                    drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
                {
                    status = fbe_terminator_miniport_api_device_table_add (port_index, 
                                                  device_handle,
                                                  (&((terminator_sas_drive_info_t *)attributes)->miniport_sas_device_table_index));
                }
                else if(drive_type == FBE_DRIVE_TYPE_SATA ||
                        drive_type == FBE_DRIVE_TYPE_SATA_FLASH_HE )
                {
                    status = fbe_terminator_miniport_api_device_table_add (port_index, 
                                                  device_handle,
                                                  (&((terminator_sata_drive_info_t *)attributes)->miniport_sas_device_table_index));              
                }
                else
                {
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                break;

            case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
                status = fbe_terminator_miniport_api_device_table_add (port_index, 
                                                  device_handle,
                                                  (&((terminator_sas_virtual_phy_info_t *)attributes)->miniport_sas_device_table_index));            
                break;

            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
        }
    }
    return status;
}
/**************************************************************************
 *  terminator_get_miniport_sas_device_table_index()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets the miniport sas device table index of this device.
 *
 *  PARAMETERS:
 *    port object handle, device_id, the index (OUT).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_get_miniport_sas_device_table_index(const fbe_terminator_device_ptr_t device_ptr,
                                                            fbe_u32_t *device_table_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_component_type_t comp_type;
    void * attributes = NULL;
    fbe_drive_type_t drive_type;
    

    comp_type =  base_component_get_component_type((base_component_t *) device_ptr);
    attributes =  base_component_get_attributes(device_ptr);

    switch(comp_type)
    {
        case TERMINATOR_COMPONENT_TYPE_PORT:            
            *device_table_index = (((terminator_sas_port_info_t *)attributes)->miniport_sas_device_table_index);
            break;

        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
            *device_table_index = (((terminator_sas_enclosure_info_t *)attributes)->miniport_sas_device_table_index);
            break;

        case TERMINATOR_COMPONENT_TYPE_DRIVE:
           drive_type = drive_get_protocol((terminator_drive_t *)device_ptr); 
           if (drive_type == FBE_DRIVE_TYPE_SAS ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_HE ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_ME ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
           {
              *device_table_index = (((terminator_sas_drive_info_t *)attributes)->miniport_sas_device_table_index);
           }
           else if(drive_type == FBE_DRIVE_TYPE_SATA ||
                   drive_type == FBE_DRIVE_TYPE_SATA_FLASH_HE)
           {
              *device_table_index = (((terminator_sata_drive_info_t *)attributes)->miniport_sas_device_table_index);
           }
           else
           {
              *device_table_index = INVALID_TMSDT_INDEX;
           }
           break;
        case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
            *device_table_index = (((terminator_sas_virtual_phy_info_t *)attributes)->miniport_sas_device_table_index);
            break; 
        
        default:
            *device_table_index = INVALID_TMSDT_INDEX;
            break;    
    }    
    status = FBE_STATUS_OK;
    return status;
}

/**************************************************************************
 *  terminator_set_miniport_sas_device_table_index()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the miniport sas device table index of this device.
 *
 *  PARAMETERS:
 *    port object handle, device_id, the index (IN).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_set_miniport_sas_device_table_index(const fbe_terminator_device_ptr_t device_ptr,
                                                            const fbe_miniport_device_id_t cpd_device_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t device_table_index = (fbe_u32_t)(INDEX_BIT_MASK&cpd_device_id) ;
     terminator_component_type_t comp_type;
    void * attributes = NULL;
    fbe_drive_type_t drive_type;
    
    if (device_ptr == NULL)
    {
        return  FBE_STATUS_NO_DEVICE;
    }

    comp_type =  base_component_get_component_type((base_component_t *) device_ptr);
    attributes =  base_component_get_attributes(device_ptr);

    switch(comp_type)
    {
        case TERMINATOR_COMPONENT_TYPE_PORT:            
            (((terminator_sas_port_info_t *)attributes)->miniport_sas_device_table_index) = device_table_index;
        break;
        
        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
            (((terminator_sas_enclosure_info_t *)attributes)->miniport_sas_device_table_index) = device_table_index;
        break;
        
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
           drive_type = (((terminator_drive_t *)attributes)->drive_protocol);
           if (drive_type == FBE_DRIVE_TYPE_SAS ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_HE ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_ME ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE ||
               drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
              (((terminator_sas_drive_info_t *)attributes)->miniport_sas_device_table_index) = device_table_index;
           else if(drive_type == FBE_DRIVE_TYPE_SATA ||
                   drive_type == FBE_DRIVE_TYPE_SATA_FLASH_HE) 
              (((terminator_sata_drive_info_t *)attributes)->miniport_sas_device_table_index) = device_table_index;           
         break;            
    }
    status = FBE_STATUS_OK;
    return status;
}

/**************************************************************************
 *  terminator_set_miniport_port_index()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets the miniport port index of this port.
 *
 *  PARAMETERS:
 *    port object handle, port_index(IN).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_set_miniport_port_index(const fbe_terminator_device_ptr_t port_handle,
                                                const fbe_u32_t port_index)
{
    return  port_set_miniport_port_index(port_handle, port_index);
}
/**************************************************************************
 *  terminator_get_miniport_port_index()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets the miniport port index of this port.
 *
 *  PARAMETERS:
 *    port object handle, port_index(OUT).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_get_miniport_port_index(const fbe_terminator_device_ptr_t port_handle,
                                                fbe_u32_t *port_index)
{
    return  port_get_miniport_port_index(port_handle, port_index);
}

/**************************************************************************
 *  terminator_get_cpd_device_id_from_miniport()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets cpd_device_id from the miniport port device table.
 *
 *  PARAMETERS:
 *    object handle, cpd_device_id(OUT).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_get_cpd_device_id_from_miniport(const fbe_terminator_device_ptr_t device_handle,
                                                        fbe_miniport_device_id_t *cpd_device_id)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       port_index;
    base_component_t *base_ptr = NULL;
     fbe_u32_t device_table_index;       

    if (device_handle != NULL)
    {
        base_ptr = (base_component_t *)device_handle;
        /* get the port number of this device */
        status = terminator_get_port_miniport_port_index(device_handle, &port_index);
        
       status = terminator_get_miniport_sas_device_table_index(device_handle, &device_table_index);
        /* then get the cpd_device_id that has the generation ids */
        status = fbe_terminator_miniport_api_get_cpd_device_id(port_index,
                                                      device_table_index,
                                                      cpd_device_id);
    }
    return status;
}
/**************************************************************************
 *  terminator_update_device_cpd_device_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets cpd_device_id from the miniport port device table.
 *
 *  PARAMETERS:
 *    object handle, cpd_device_id(OUT).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_update_device_cpd_device_id(const fbe_terminator_device_ptr_t device_handle)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       port_index;
    base_component_t *base_ptr = NULL;
    base_component_t *child_ptr = NULL;
    fbe_miniport_device_id_t cpd_device_id;
    void * attributes = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_u32_t device_table_index;
    terminator_component_type_t comp_type;    

    if (device_handle != NULL)
    {
        base_ptr = (base_component_t *)device_handle;
        /* get the port number of this device */
        status = terminator_get_port_miniport_port_index(device_handle, &port_index);
        comp_type =  base_component_get_component_type((base_component_t *) device_handle);
        
        status = terminator_get_miniport_sas_device_table_index(device_handle, &device_table_index);
        /* then get the cpd_device_id that has the generation ids */
        status = fbe_terminator_miniport_api_get_cpd_device_id(port_index,
                                                      device_table_index,
                                                      &cpd_device_id);
        switch(comp_type)
        {
        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
            attributes = base_component_get_attributes(base_ptr);
            if (attributes == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            login_data = sas_enclosure_info_get_login_data(attributes);
            if (login_data == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Use the new device id format */
            cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);
            /* update the parent device id of the children */
            child_ptr = base_component_get_first_child(base_ptr);
            while (child_ptr != NULL)
            {
                terminator_update_device_parent_cpd_device_id(child_ptr, cpd_device_id);
                child_ptr = base_component_get_next_child(base_ptr, child_ptr);
            }
            break;
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
            attributes = base_component_get_attributes(base_ptr);
            if (attributes == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            login_data = sas_drive_info_get_login_data(attributes);
            if (login_data == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Use the new device id format */
            cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);
            break;
        case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
            attributes = base_component_get_attributes(base_ptr);
            if (attributes == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            login_data = sas_virtual_phy_info_get_login_data(attributes);
            if (login_data == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Use the new device id format */
            cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
    }
    return status;
}

/**************************************************************************
 *  terminator_update_device_parent_cpd_device_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets parent cpd_device_id of the given device.
 *
 *  PARAMETERS:
 *    object handle.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_update_device_parent_cpd_device_id(const fbe_terminator_device_ptr_t device_handle, const fbe_miniport_device_id_t cpd_device_id)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t *base_ptr = NULL;
    void * attributes = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    if (device_handle != NULL)
    {
        base_ptr = (base_component_t *)device_handle;
        switch(base_component_get_component_type(base_ptr))
        {
        case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
            attributes = base_component_get_attributes(base_ptr);
            if (attributes == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            login_data = sas_virtual_phy_info_get_login_data(attributes);
            if (login_data == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Use the new device id format */
            cpd_shim_callback_login_set_parent_device_id(login_data, cpd_device_id);
            break;
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
            attributes = base_component_get_attributes(base_ptr);
            if (attributes == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            login_data = sas_drive_info_get_login_data(attributes);
            if (login_data == NULL)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Use the new device id format */
            cpd_shim_callback_login_set_parent_device_id(login_data, cpd_device_id);
            break;
        default:
            /* unsupported device type*/
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
    }
    return status;
}
/**************************************************************************
 *  terminator_reserve_cpd_device_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets cpd_device_id from the miniport port device table.
 *
 *  PARAMETERS:
 *    object handle, cpd_device_id(OUT).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_reserve_cpd_device_id(const fbe_u32_t       port_index,
                                              const fbe_miniport_device_id_t cpd_device_id)
{
    return fbe_terminator_miniport_api_device_table_reserve(port_index, cpd_device_id);
}

/**************************************************************************
 *  terminator_get_drive_parent_virtual_phy_ptr()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets virtual_phy_handle of the parent enclosure.
 *
 *  PARAMETERS:
 *    port_number, id, virtual_phy_handle(OUT).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Feb-03-2009: Vicky G created.
 **************************************************************************/
fbe_status_t terminator_get_drive_parent_virtual_phy_ptr(fbe_terminator_device_ptr_t device_ptr, fbe_terminator_device_ptr_t *virtual_phy_handle)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * parent_enclosure = NULL;

    *virtual_phy_handle = NULL;

    if (device_ptr != NULL)
    {
        status = base_component_get_parent(device_ptr, &parent_enclosure);
        if (parent_enclosure!= NULL)
        {
            // Get the related virtual phy  of the enclosure
            *virtual_phy_handle = base_component_get_child_by_type(parent_enclosure, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
        }
    }
    return status;
}

fbe_status_t terminator_virtual_phy_power_cycle_lcc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u32_t power_cycle_time_in_ms)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t port_num;
    terminator_enclosure_t *parent_encl = NULL;
    fbe_miniport_device_id_t encl_cpd_device_id;

    // make sure we got a virtual phy device handle.
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    // Get the corresponding port number (index).
    status = terminator_get_port_index(virtual_phy_handle, &port_num);
    RETURN_ON_ERROR_STATUS;

    // Get parent enclosure object
    status = base_component_get_parent(virtual_phy_handle, (base_component_t **)&parent_encl);
    RETURN_ON_ERROR_STATUS;

    // Get the CPD device ID of the enclosure, which is exposed
    // to the clients that are using terminator miniport interface.
    status = terminator_get_cpd_device_id_from_miniport(parent_encl, &encl_cpd_device_id);
    RETURN_ON_ERROR_STATUS;

    // Set the reset delay time (same as power cycle time for *NOW*)
    status = terminator_set_device_reset_delay(parent_encl, power_cycle_time_in_ms);
    RETURN_ON_ERROR_STATUS;

    // Issue the reset
    status = fbe_terminator_miniport_api_reset_device(port_num, encl_cpd_device_id);
    RETURN_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

/*********************************************************************
*            terminator_virtual_phy_power_cycle_drive ()
*********************************************************************
*
*  Description:
*    This function get the device pointer, set the power cycle drive function for reset thread
*    and invoke reset thread.
*
*  Inputs:
*   status_elements_start_ptr - pointer to the start of general information elements
*   virtual phy object handle  - virtual phy object handle
*   num_general_inf_elems   - number of general inf. elements
*   sense_buffer                   - Buffer to be filled with error.
*
*  Return Value:
*       success or failure
*
*  History:
*      July-03-2009: Dipak Patel created.
*
*********************************************************************/
fbe_status_t terminator_virtual_phy_power_cycle_drive(fbe_terminator_device_ptr_t virtual_phy_handle,
                                         fbe_u32_t power_cycle_time_in_ms,
                                         fbe_u32_t  drive_slot_number)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t port_num;
    terminator_enclosure_t *parent_encl = NULL;
    fbe_miniport_device_id_t drive_cpd_device_id;
    fbe_terminator_device_ptr_t drive_ptr = NULL;

    // make sure we got a virtual phy device handle.
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        return status;
    }

    // Get the corresponding port number (index).
    status = terminator_get_port_index(virtual_phy_handle, &port_num);
    RETURN_ON_ERROR_STATUS;

    // Get parent enclosure object
    status = base_component_get_parent(virtual_phy_handle, (base_component_t **)&parent_encl);
    RETURN_ON_ERROR_STATUS;

    //get the drive pointer based on slot and parent enclosure.
    status = terminator_get_sas_drive_device_ptr((fbe_terminator_device_ptr_t) parent_encl, drive_slot_number, &drive_ptr);
    RETURN_ON_ERROR_STATUS;

    // Get the CPD device ID of the drive, which is exposed
    // to the clients that are using terminator miniport interface.
    status = terminator_get_cpd_device_id_from_miniport(drive_ptr, &drive_cpd_device_id);
    RETURN_ON_ERROR_STATUS;

    // Set the reset delay time (same as power cycle time for *NOW*)
    status = terminator_set_device_reset_delay(drive_ptr, power_cycle_time_in_ms);
    RETURN_ON_ERROR_STATUS;

    status = terminator_register_device_reset_function(drive_ptr,
                                                       terminator_miniport_drive_power_cycle_action);

    // Issue the power cycle into reset thread context.
    status = fbe_terminator_miniport_api_reset_device(port_num, drive_cpd_device_id);
    RETURN_ON_ERROR_STATUS;

    return(FBE_STATUS_OK);
}

/*********************************************************************
*            terminator_enclosure_firmware_download ()
*********************************************************************
*
*  Description:
*    This function should be called when enclosure firmware download command arrives.
*    It keeps new revision numbers for each component in queue.
*
*  Inputs:
 *    virtual_phy_handle: virtual phy object pointer
 *    subencl_type
 *    side
 *    slot_num
 *    comp_type: component type
 *    new_rev_number: CHAR[5]
*
*  Return Value:
*       success or failure
*
*  History:
*      2010-8-12: Bo Gao created.
*
*********************************************************************/
fbe_status_t terminator_enclosure_firmware_download(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                    ses_subencl_type_enum subencl_type,
                                                                    terminator_sp_id_t side,
                                                                    ses_comp_type_enum  comp_type,
                                                                    fbe_u32_t slot_num,
                                                                    CHAR   *new_rev_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    // make sure we got a virtual phy device handle.
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: get virtual phy object pointer failed\n",
			 __FUNCTION__);
        return status;
    }

    // update or push new revision record
    status  = sas_virtual_phy_update_enclosure_firmware_download_record(virtual_phy_handle, subencl_type, side, comp_type, slot_num, new_rev_number);
    RETURN_ON_ERROR_STATUS;

    return FBE_STATUS_OK;
}

// enclosure firmware activating thread function.
static void terminator_enclosure_firmware_activate_thread_func(void *context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_endlosure_firmware_activate_context_t *new_rev_context = (terminator_endlosure_firmware_activate_context_t *)context;
    fbe_u8_t download_status;
    fbe_terminator_device_ptr_t enclosure_ptr, logical_parent_ptr;
    terminator_sas_virtual_phy_info_t *virtual_phy_info;
    terminator_sas_enclosure_info_t *encl_attributes = NULL;

    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    virtual_phy_info = (terminator_sas_virtual_phy_info_t *)base_component_get_attributes((base_component_t *)new_rev_context->virtual_phy_ptr);
    if(virtual_phy_info == NULL)
    {
        terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: get virtual phy attributes failed\n",
			 __FUNCTION__);
        return;
    }

    // delay before activate
    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s sleep before activating\n", __FUNCTION__);
    fbe_thread_delay(virtual_phy_info->activate_time_intervel);

    // check download status, if it's not a good status terminate the activate process
    status = terminator_eses_get_download_status(new_rev_context->virtual_phy_ptr, &download_status);
    if(status != FBE_STATUS_OK)
    {
        terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: get download status failed\n",
			 __FUNCTION__);
        return;
    }

    if(download_status < ESES_DOWNLOAD_STATUS_ERROR_PAGE_FIELD)
    {
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s start activating\n", __FUNCTION__);
        // if sub-enclosure type is LCC, log out it and all its children first
        if(new_rev_context->new_rev_record.subencl_type == SES_SUBENCL_TYPE_LCC)
        {
            status = sas_virtual_phy_get_enclosure_ptr(new_rev_context->virtual_phy_ptr, &enclosure_ptr);
            if(status != FBE_STATUS_OK)
            {
                terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: get enclosure pointer failed\n",
			         __FUNCTION__);
                return;
            }

            encl_attributes = base_component_get_attributes(enclosure_ptr);
            if(encl_attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_PORT)
            {
                status = terminator_get_port_ptr(enclosure_ptr, &logical_parent_ptr);
                if(status != FBE_STATUS_OK)
                {
                    terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
                    terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s: get port pointer failed\n",
				     __FUNCTION__);
                    return;
                }
            }
            else if(encl_attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
            {
                base_component_get_parent(enclosure_ptr, (base_component_t **) &logical_parent_ptr);
            }
            else
            {
                terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: unsupported enclosure logical parent type: %d\n",
				 __FUNCTION__, encl_attributes->logical_parent_type);
                return;
            }

            status = fbe_terminator_unmount_device(enclosure_ptr);
            if(status != FBE_STATUS_OK)
            {
                terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
                terminator_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: log out LCC sub-enclosure failed\n",
                                 __FUNCTION__);
                return;
            }
        }
    }
    else
    {
        /* The user injected the activation error.*/
        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: bad download status: 0x%x, activating terminates\n",
                         __FUNCTION__, download_status);

        fbe_thread_exit(EMCPAL_STATUS_SUCCESS);

        return;
    }

    // delay before reset
    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s sleep before resetting\n", __FUNCTION__);
    fbe_thread_delay(virtual_phy_info->reset_time_intervel);

    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s start resetting\n", __FUNCTION__);

    // update firmware revision number if required
    if(new_rev_context->update_rev)
    {
        status = terminator_eses_set_ver_num(new_rev_context->virtual_phy_ptr,
                                                new_rev_context->new_rev_record.subencl_type,
                                                new_rev_context->new_rev_record.slot_num,
                                                new_rev_context->new_rev_record.comp_type,
                                                new_rev_context->new_rev_record.new_rev_number);
        if(status != FBE_STATUS_OK)
        {
            terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: could not set ver desc for subencl_type: %d, side: %d, comp_type: %d\n",
                             __FUNCTION__, new_rev_context->new_rev_record.subencl_type,
                             new_rev_context->new_rev_record.side,
                             new_rev_context->new_rev_record.comp_type);
            return;
        }

    }
    
    // increment generation code in the configuration page
    status = terminator_eses_increment_config_page_gen_code(new_rev_context->virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not increment config page gen code for subencl_type: %d, side: %d, comp_type: %d\n",
                         __FUNCTION__, new_rev_context->new_rev_record.subencl_type,
                         new_rev_context->new_rev_record.side,
                         new_rev_context->new_rev_record.comp_type);
        return;
    }

    // if sub-enclosure type is LCC, log in it and all its children back
    if(new_rev_context->new_rev_record.subencl_type == SES_SUBENCL_TYPE_LCC)
    {
        if(encl_attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_PORT)
        {
            status = port_add_sas_enclosure(logical_parent_ptr, enclosure_ptr, FBE_FALSE);
        }
        else if(encl_attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
        {
            status = enclosure_add_sas_enclosure(logical_parent_ptr, enclosure_ptr, FBE_FALSE);
        }
        else
        {
            terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: unsupported enclosure logical parent type: %d\n",
			     __FUNCTION__, encl_attributes->logical_parent_type);
            return;
        }

        if(status != FBE_STATUS_OK)
        {
            terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: port add enclosure failed\n",
			      __FUNCTION__);
            return;
        }
        /* Activate the enclosure */
        status = enclosure_activate(enclosure_ptr);
        if(status != FBE_STATUS_OK)
        {
            terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: log in LCC sub-enclosure failed\n",
                             __FUNCTION__);
            return;
        }
    }

/****************

    // update firmware revision number if required
    if(new_rev_context->update_rev)
    {
        status = terminator_eses_set_ver_num(new_rev_context->virtual_phy_ptr,
                                                new_rev_context->new_rev_record.subencl_type,
                                                new_rev_context->new_rev_record.side,
                                                new_rev_context->new_rev_record.comp_type,
                                                new_rev_context->new_rev_record.new_rev_number);
        if(status != FBE_STATUS_OK)
        {
            terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: could not set ver desc for subencl_type: %d, side: %d, comp_type: %d\n",
                             __FUNCTION__, new_rev_context->new_rev_record.subencl_type,
                             new_rev_context->new_rev_record.side,
                             new_rev_context->new_rev_record.comp_type);
            return;
        }

    }
    
    // increment generation code in the configuration page
    status = terminator_eses_increment_config_page_gen_code(new_rev_context->virtual_phy_ptr);
    if(status != FBE_STATUS_OK)
    {
        terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_ACTIVATE_FAILED);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: could not increment config page gen code for subencl_type: %d, side: %d, comp_type: %d\n",
                         __FUNCTION__, new_rev_context->new_rev_record.subencl_type,
                         new_rev_context->new_rev_record.side,
                         new_rev_context->new_rev_record.comp_type);
        return;
    }
**********************************/

    // set download  status to good one
    status = terminator_eses_set_download_status(new_rev_context->virtual_phy_ptr, ESES_DOWNLOAD_STATUS_NONE);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: set download status failed\n",
			 __FUNCTION__);
        return;
    }

    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s done\n", __FUNCTION__);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*********************************************************************
*            terminator_enclosure_firmware_activate ()
*********************************************************************
*
*  Description:
*    This function should be called when enclosure firmware activate command arrives.
*    It resets if sub-enclosure type is LCC, and update revision number is required.
*    NOTE that it's not a synchronized call, it will return immediately after starting up
*    the activate thread.
*
*  Inputs:
 *    virtual_phy_handle: virtual phy object pointer
 *    subencl_type
 *    side
 *    slot_num
*
*  Return Value:
*       success or failure
*
*  History:
*      2010-8-12: Bo Gao created.
*
*********************************************************************/
fbe_status_t terminator_enclosure_firmware_activate(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                    ses_subencl_type_enum subencl_type,
                                                                    terminator_sp_id_t side,
                                                                    fbe_u32_t slot_num)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_endlosure_firmware_activate_context_t *context;
    CHAR new_rev_number[FBE_ESES_FW_REVISION_SIZE];
    terminator_sas_virtual_phy_info_t *virtual_phy_info = NULL;
    fbe_bool_t update_rev;
    ses_comp_type_enum  comp_type;
    terminator_encl_firmware_activate_thread_queue_element_t *thread_queue_element = NULL;

    // make sure we got a virtual phy device handle.
    if (base_component_get_component_type(virtual_phy_handle)!=TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: get virtual phy object pointer failed\n",
			 __FUNCTION__);
        return status;
    }

    virtual_phy_info = (terminator_sas_virtual_phy_info_t *)base_component_get_attributes((base_component_t *)virtual_phy_handle);
    if(virtual_phy_info == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: get virtual phy attributes failed\n",
			 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get matched new firmware download record
    status = sas_virtual_phy_pop_enclosure_firmware_download_record(virtual_phy_handle, subencl_type, side, slot_num, &comp_type, new_rev_number);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: pop new firmware revision number failed\n",
			 __FUNCTION__);
        return status;
    }

    context = fbe_terminator_allocate_memory(sizeof(terminator_endlosure_firmware_activate_context_t));
    if(context == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: allocate thread context memory failed\n",
			 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    context->new_rev_record.subencl_type = subencl_type;
    context->new_rev_record.side = side;
    context->new_rev_record.comp_type = comp_type;
    context->new_rev_record.slot_num = slot_num;
    fbe_copy_memory(context->new_rev_record.new_rev_number, new_rev_number, sizeof(context->new_rev_record.new_rev_number));
    context->virtual_phy_ptr = virtual_phy_handle;
    // get whether user wants to update revision number
    status = terminator_get_need_update_enclosure_firmware_rev(&update_rev);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: get need_update_enclosure_firmware_rev failed\n",
			 __FUNCTION__);

        fbe_terminator_free_memory(context);

        return status;
    }
    context->update_rev = update_rev;

    // set download status to ESES_DOWNLOAD_STATUS_UPDATING_NONVOL
    status = terminator_eses_set_download_status(virtual_phy_handle, ESES_DOWNLOAD_STATUS_UPDATING_NONVOL);
    if(status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: set download status failed\n",
			 __FUNCTION__);

        fbe_terminator_free_memory(context);

        return status;
    }

    thread_queue_element = fbe_terminator_allocate_memory(sizeof(terminator_encl_firmware_activate_thread_queue_element_t));
    if (thread_queue_element == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: allocate thread queue element memory failed\n",
                         __FUNCTION__);

        fbe_terminator_free_memory(context);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_spinlock_lock(&terminator_encl_firmware_activate_thread_queue_lock);
    fbe_queue_push(&terminator_encl_firmware_activate_thread_queue_head, &thread_queue_element->queue_element);
    fbe_spinlock_unlock(&terminator_encl_firmware_activate_thread_queue_lock);

    if (!terminator_encl_firmware_activate_thread_started)
    {
        terminator_encl_firmware_activate_thread_started = FBE_TRUE;
    }

    fbe_thread_init(&thread_queue_element->enclosure_firmware_activate_thread, "fbe_term_firm", terminator_enclosure_firmware_activate_thread_func, context);

    return FBE_STATUS_OK;
}

/**************************************************************************
 *  terminator_get_maximum_transfer_bytes_from_miniport()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets the maximum per-drive request size in bytes from
 *    the miniport port device table.
 *
 *  PARAMETERS:
 *    object handle, *maximum_transfer_bytes_p (out).
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *  07/26/2010  Ron Proulx  - Created
 **************************************************************************/
fbe_status_t terminator_get_maximum_transfer_bytes_from_miniport(const fbe_terminator_device_ptr_t device_handle,
                                                                 fbe_u32_t *maximum_transfer_bytes_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    if (device_handle != NULL)
    {
        fbe_terminator_device_ptr_t port_handle = NULL;
        fbe_port_info_t             port_info;

        /* get the port handle of this device */
        status = terminator_get_port_ptr(device_handle, &port_handle);

        /* Get the port information */
        if (status == FBE_STATUS_OK)
        {
            status = terminator_get_port_info(port_handle, &port_info);
            if (status == FBE_STATUS_OK)
            {
                *maximum_transfer_bytes_p = port_info.maximum_transfer_bytes;
            }
        }
    }
    return status;
}


fbe_status_t terminator_get_display_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                               terminator_eses_display_character_id display_character_id,
                                               ses_stat_elem_display_struct *display_stat_elem)
{
    return(sas_virtual_phy_get_diplay_eses_status(virtual_phy_handle,
                                                  display_character_id,
                                                  display_stat_elem));
}

fbe_status_t terminator_set_display_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                               terminator_eses_display_character_id display_character_id,
                                               ses_stat_elem_display_struct display_stat_elem)
{
    return(sas_virtual_phy_set_diplay_eses_status(virtual_phy_handle,
                                                  display_character_id,
                                                  display_stat_elem));
}

fbe_status_t terminator_set_drive_slot_insert_count(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u32_t slot_number,
                                                    fbe_u8_t insert_count)
{
    return(sas_virtual_phy_set_drive_slot_insert_count(virtual_phy_handle,
                                                       slot_number,
                                                       insert_count));
}

fbe_status_t terminator_get_drive_slot_insert_count(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u32_t slot_number,
                                                    fbe_u8_t *insert_count)
{
    return(sas_virtual_phy_get_drive_slot_insert_count(virtual_phy_handle,
                                                       slot_number,
                                                       insert_count));
}

fbe_status_t terminator_set_drive_power_down_count(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                   fbe_u32_t slot_number,
                                                   fbe_u8_t power_down_count)
{
    return(sas_virtual_phy_set_drive_power_down_count(virtual_phy_handle,
                                                      slot_number,
                                                      power_down_count));
}

fbe_status_t terminator_get_drive_power_down_count(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                   fbe_u32_t slot_number,
                                                   fbe_u8_t *power_down_count)
{
    return(sas_virtual_phy_get_drive_power_down_count(virtual_phy_handle,
                                                      slot_number,
                                                      power_down_count));
}

fbe_status_t terminator_clear_drive_power_down_counts(fbe_terminator_device_ptr_t virtual_phy_handle)
{
    return(sas_virtual_phy_clear_drive_power_down_counts(virtual_phy_handle));
}

fbe_status_t terminator_clear_drive_slot_insert_counts(fbe_terminator_device_ptr_t virtual_phy_handle)
{
    return(sas_virtual_phy_clear_drive_slot_insert_counts(virtual_phy_handle));
}

fbe_status_t terminator_set_need_update_enclosure_firmware_rev(fbe_bool_t need_update_rev)
{
    need_update_enclosure_firmware_rev = need_update_rev;

    return FBE_STATUS_OK;
}

fbe_status_t terminator_get_need_update_enclosure_firmware_rev(fbe_bool_t *need_update_rev)
{
    *need_update_rev = need_update_enclosure_firmware_rev;

    return FBE_STATUS_OK;
}

fbe_status_t terminator_set_need_update_enclosure_resume_prom_checksum(fbe_bool_t need_update_checksum)
{
    need_update_enclosure_resume_prom_checksum = need_update_checksum;

    return FBE_STATUS_OK;
}

fbe_status_t terminator_get_need_update_enclosure_resume_prom_checksum(fbe_bool_t *need_update_checksum)
{
    *need_update_checksum = need_update_enclosure_resume_prom_checksum;

    return FBE_STATUS_OK;
}

#include "fbe/fbe_file.h"

/*********************************************************************
*            terminator_update_enclosure_drive_slot_number ()
*********************************************************************
*
*  Description:
*    This function converts local slot number (EE or virtual phy) to enclosure slot number
*
*    NOTICE: since the function also resets slot_number if necessary,
*   slot_number should be passed in as pointer and its original value may change.
*
*  Inputs:
*   device_ptr   - device pointer where the seeking starts. 
*   slot_number - pointer to a place in which to return the slot number. Ignored if NULL.
*   ret_connector_id - pointer to a place in which to return the connector_id.  Ignored if NULL.
*
*  Return Value:
*       success or failure
*
*  History:
*      May-18-2010: CHIU created.
*
*********************************************************************/
fbe_status_t terminator_update_enclosure_drive_slot_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *slot_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_enclosure_t * parent_enclosure = NULL;
    terminator_enclosure_t * grandparent = NULL;
    fbe_u32_t connector_id;
    fbe_u8_t drive_start_slot = 0;
    fbe_sas_enclosure_type_t encl_type;

    // Get parent enclosure object
    status = base_component_get_parent(device_ptr,(base_component_t **)&parent_enclosure);
    RETURN_ON_ERROR_STATUS;
    // if this is for EE virtual phy
    if ((base_component_get_component_type(device_ptr)==TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY) &&
        (sas_enclosure_get_logical_parent_type(parent_enclosure) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE))
    {
        // get the parent of ICM
        status = base_component_get_parent(&parent_enclosure->base,(base_component_t **)&grandparent);
        RETURN_ON_ERROR_STATUS;
        status = sas_enclosure_get_connector_id_by_enclosure(parent_enclosure, grandparent, &connector_id);
        RETURN_ON_ERROR_STATUS;
        // update slot_num.
        encl_type = sas_enclosure_get_enclosure_type(grandparent);
        status = sas_virtual_phy_get_conn_id_to_drive_start_slot_mapping(encl_type, connector_id, &drive_start_slot);
        RETURN_ON_ERROR_STATUS;
        *slot_number += drive_start_slot;
    }
    // special handle for EE itself
    else if (sas_enclosure_get_logical_parent_type(device_ptr) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        status = sas_enclosure_get_connector_id_by_enclosure(device_ptr, parent_enclosure, &connector_id);
        RETURN_ON_ERROR_STATUS;
        // update slot_num.
        encl_type = sas_enclosure_get_enclosure_type(parent_enclosure);
        status = sas_virtual_phy_get_conn_id_to_drive_start_slot_mapping(encl_type, connector_id, &drive_start_slot);
        RETURN_ON_ERROR_STATUS;
        *slot_number += drive_start_slot;
    }

    return (status);
}



/*!****************************************************************************
 * @fn      terminator_get_ee_component_connector_id()
 *
 * @brief
 * This function retrieves a connector id associated with an edge
 * expander device pointer.
 *
 * @param device_ptr - device pointer should be associated with an edge expander device.
 * @param ret_connector_id - pointer to a place to store the connector id.
 *
 * @return
 *  FBE_STATUS_OK - if the connector_id was successfully retrieved.
 *
 * HISTORY
 *  06/02/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t terminator_get_ee_component_connector_id(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *ret_connector_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_enclosure_t * parent_enclosure = NULL;
    fbe_u32_t connector_id;

    // Get parent enclosure object
    status = base_component_get_parent(device_ptr,(base_component_t **)&parent_enclosure);
    RETURN_ON_ERROR_STATUS;
    // if this is for EE virtual phy
    // special handle for EE itself
    if (sas_enclosure_get_logical_parent_type(device_ptr) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        status = sas_enclosure_get_connector_id_by_enclosure(device_ptr, parent_enclosure, &connector_id);
        RETURN_ON_ERROR_STATUS;
        *ret_connector_id = connector_id;
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*********************************************************************
*            terminator_update_drive_parent_device ()
*********************************************************************
*
*  Description:
*    This function gets the parent device which manages the specified slot. 
*    virtual_phy only manages local slot number, ICM virtual phy does not own any slot.
*
*    NOTICE: since the function also resets the passed-in device pointer and slot_number if necessary,
*    device_ptr and slot_number should be passed in as pointer and their original value may change.
*
*  Inputs:
*   parent_ptr   - pointer to the device pointer where the seeking starts. It may be reset to another value
*                      if it is not the parent of the slot. NOTE that it cannot be the drive itself, should be enclosure
*                      or virtual phy.
*   slot_number - pointer to the slot number. It may be reset to the local slot number in the parent device.
*
*  Return Value:
*       success or failure
*
*  History:
*      March-23-2010: Bo Gao created.
*
*********************************************************************/
fbe_status_t terminator_update_drive_parent_device(fbe_terminator_device_ptr_t *parent_ptr, fbe_u32_t *slot_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t max_drive_slots;
    fbe_sas_enclosure_type_t encl_type;
    terminator_component_type_t device_type = base_component_get_component_type(*parent_ptr);
    base_component_t *parent = NULL, *child = NULL;

    /* get the current enclosure type and max drive slots */
    switch(device_type)
    {
    case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
        status = sas_virtual_phy_get_enclosure_type(*parent_ptr, &encl_type);
        RETURN_ON_ERROR_STATUS;
        status = sas_virtual_phy_max_drive_slots(encl_type, &max_drive_slots);
        RETURN_ON_ERROR_STATUS;
        break;
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        encl_type = sas_enclosure_get_enclosure_type(*parent_ptr);
        status = terminator_max_drive_slots(encl_type, &max_drive_slots);
        RETURN_ON_ERROR_STATUS;
        break;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (*slot_number >= max_drive_slots)
    {
        /* for the ICM case, seek for the actual EE which owns the slot */
        if(max_drive_slots == 0)
        {
            if (device_type == TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
            {
                /* ICM virtual phy */
                status = base_component_get_parent(*parent_ptr, &parent);
                RETURN_ON_ERROR_STATUS;
            }
            else
            {
                /* device_ptr is the ICM enclosure */
                parent = (base_component_t *)*parent_ptr;
            }
            child = base_component_get_first_child(parent);
            while (child != NULL)
            {
                if(child->component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
                {
                    status = terminator_update_enclosure_local_drive_slot_number(child, slot_number);
                    if (status == FBE_STATUS_OK)
                    {
                        if (device_type == TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
                        {
                            // make sure we return the same type
                            (*parent_ptr) = (terminator_virtual_phy_t *)base_component_get_child_by_type(child, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
                        }
                        else
                        {
                            (*parent_ptr) = child;
                        }
                        return status;
                    }
                }
                child = base_component_get_next_child(parent, child);
            }
        }
        // we need to adjust the slot number for EEs
        else if ((device_type == TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)||
                 (sas_enclosure_get_logical_parent_type(*parent_ptr)==TERMINATOR_COMPONENT_TYPE_ENCLOSURE))
        {
            if (device_type == TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY)
            {
                base_component_get_parent(*parent_ptr, &child);
            }
            else
            {
                child = (base_component_t *)*parent_ptr;
            }
            status = terminator_update_enclosure_local_drive_slot_number(child, slot_number);
            return status;
        }

        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*********************************************************************
*            terminator_update_enclosure_local_drive_slot_number ()
*********************************************************************
*
*  Description:
*    This function converts enclosure slot number (EE or virtual phy) to local slot number
*
*    NOTICE: since the function also resets slot_number if necessary,
*   slot_number should be passed in as pointer and its original value may change.
*
*  Inputs:
*   device_ptr   - the assumed owner of the slot. 
*   slot_number - pointer to the slot number. It may be reset to the local slot number in the parent device.
*
*  Return Value:
*       success  means a match.
*
*  History:
*      May-18-2010: CHIU created.
*
*********************************************************************/
fbe_status_t terminator_update_enclosure_local_drive_slot_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *slot_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t *parent = NULL;
    fbe_u8_t child_slot_start, child_max_drive_slots;
    fbe_u32_t child_connector_id;
    fbe_sas_enclosure_type_t child_encl_type;
    fbe_sas_enclosure_type_t parent_encl_type;
    fbe_u8_t slot = *slot_number;

    /* We should only do this for EE enclosures */
    if (sas_enclosure_get_logical_parent_type(device_ptr) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
    {
        /* get the child enclosure type and max drive slots */
        child_encl_type = sas_enclosure_get_enclosure_type((terminator_enclosure_t *)device_ptr);
        status = terminator_max_drive_slots(child_encl_type, &child_max_drive_slots);
        RETURN_ON_ERROR_STATUS;

        base_component_get_parent(device_ptr, &parent);
        status = sas_enclosure_get_connector_id_by_enclosure((terminator_enclosure_t *)device_ptr, (terminator_enclosure_t *)parent, &child_connector_id);
        RETURN_ON_ERROR_STATUS;

        /* 0 for EE0 and 30 for EE1 */
        parent_encl_type = sas_enclosure_get_enclosure_type((terminator_enclosure_t *)parent);
        status = sas_virtual_phy_get_conn_id_to_drive_start_slot_mapping(parent_encl_type, child_connector_id, &child_slot_start);
        RETURN_ON_ERROR_STATUS;
        if((slot >= child_slot_start) && (slot < (fbe_u8_t)(child_slot_start + child_max_drive_slots)))
        {
            /* map the global slot number to local slot number in EE */
            *slot_number = slot - child_slot_start;

            return FBE_STATUS_OK;
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}




/*!****************************************************************************
 * @fn      terminator_get_connector_id_list_for_parent_device()
 *
 * @brief
 * This function retrieves the list of connector ids associated with a
 * device pointer, which should be tied to an ICM.  The device type must be
 * an enclosure
 *
 * @param device_ptr - pointer to an enclosure 
 * @param conector_ids - pointer to the place to store the connector list
 *        connector_ids->num_connectors indicates how many in the list are valid
 *
 * @return
 *  FBE_STATUS_OK if all goes well.  
 *
 * HISTORY
 *  06/02/10 :  Gerry Fredette -- Created.
 *
 *****************************************************************************/
fbe_status_t terminator_get_connector_id_list_for_parent_device(fbe_terminator_device_ptr_t device_ptr, 
    fbe_term_encl_connector_list_t *connector_ids)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t max_drive_slots;
    fbe_enclosure_type_t encl_type;
    fbe_u32_t connector_id;
    terminator_component_type_t device_type = base_component_get_component_type(device_ptr);
    base_component_t *parent = NULL, *child = NULL;

    memset(connector_ids, 0, sizeof(*connector_ids));

    /* get the current enclosure type and max drive slots */
    switch(device_type)
    {
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        encl_type = sas_enclosure_get_enclosure_type(device_ptr);
        status = terminator_max_drive_slots(encl_type, &max_drive_slots);
        RETURN_ON_ERROR_STATUS;
        break;
    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* for the ICM case, seek for the actual EE which owns the slot */
    if(max_drive_slots == 0)
    {
        /* device_ptr is the ICM enclosure */
        parent = (base_component_t *)device_ptr;

        child = base_component_get_first_child(parent);
        while (child != NULL)
        {
            if(child->component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
            {
                status = terminator_get_ee_component_connector_id(child, &connector_id);
                if (status == FBE_STATUS_OK)
                {
                    if (connector_ids->num_connector_ids >= MAX_POSSIBLE_CONNECTOR_ID_COUNT)
                    {
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                    connector_ids->list[connector_ids->num_connector_ids++] = connector_id;
                }
            }
            child = base_component_get_next_child(parent, child);
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t terminator_create_disk_file(char *file_name, fbe_u64_t file_size_in_bytes)
{
    fbe_file_handle_t file_handle;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t end_offset = file_size_in_bytes - 1;
    fbe_file_status_t err_status = CSX_STATUS_SUCCESS;

    /* Open the file.  We set the flags so that we can read/write the
     * file. We assume the file exists.
     */
    file_handle = fbe_file_open(file_name, FBE_FILE_RDWR, 0, NULL);
    if (file_handle == FBE_FILE_INVALID_HANDLE)
    {
        /* The file could not be opened because it might not exist.
         * Try to create the file.
         * We set the flags so that we can read/write and create the file.
         */
        file_handle = fbe_file_open(file_name, FBE_FILE_RDWR | FBE_FILE_CREAT, 0, NULL);
        if (file_handle == FBE_FILE_INVALID_HANDLE)
        {
            /* The file could not be created.
             */
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: could not create %s\n", __FUNCTION__, file_name);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else 
    {
        /* The file already exists and hence no need to proceed further.
         * close out the file.
         */
        fbe_file_close(file_handle);
        return FBE_STATUS_OK;
    }

    /* We are going to generate the file.
     */
    terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "Terminator Generating disk file:%s\n", file_name);

    /* Write a single byte to the end of the file.
     */
    if (fbe_file_lseek(file_handle, end_offset, 0) != FBE_FILE_ERROR)
    {
        char char_to_end = 0;
        fbe_u32_t bytes_written = fbe_file_write(file_handle, &char_to_end, sizeof(char_to_end), &err_status);

        /* make sure that we are successful in writing.
         */
        if (bytes_written != 1)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: cannot zero %s status: %d\n", __FUNCTION__, file_name, err_status);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: cannot seek %s status: %d\n", __FUNCTION__, file_name, err_status);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* close out the file
     */
    fbe_file_close(file_handle);

    return status;
}

const char * terminator_file_api_get_disk_storage_dir(void)
{
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#ifdef ALAMOSA_WINDOWS_ENV
    return ".\\";
#else
    return "./";
#endif /* ALAMOSA_WINDOWS_ENV */
#else
    return terminator_disk_storage_dir;
#endif // UMODE_ENV || SIMMODE_ENV
}

/* Dual SP Unity simulator requires O_DIRECT to prevent caching of terminator disk I/O on host side
 * Other virtualized platforms cannot use this because O_DIRECT prevents using 520B block size 
 * on ext2 mounted filesystems.
 * O_DIRECT can be enabled with registry key
 */
const fbe_u32_t terminator_file_api_get_access_mode(void)
{
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    return FBE_FILE_RDWR | FBE_FILE_UNBUF;
#else
    return FBE_FILE_RDWR | FBE_FILE_UNBUF | terminator_access_mode;
#endif // UMODE_ENV || SIMMODE_ENV
}

fbe_status_t terminator_wait_for_enclosure_firmware_activate_thread(void)
{
    terminator_encl_firmware_activate_thread_queue_element_t *thread_queue_element = NULL;

    if (!terminator_encl_firmware_activate_thread_started)
    {
        return FBE_STATUS_OK;
    }

    for (;;)
    {
        fbe_spinlock_lock(&terminator_encl_firmware_activate_thread_queue_lock);
        thread_queue_element = (terminator_encl_firmware_activate_thread_queue_element_t *)fbe_queue_pop(&terminator_encl_firmware_activate_thread_queue_head);
        fbe_spinlock_unlock(&terminator_encl_firmware_activate_thread_queue_lock);

        if (thread_queue_element == NULL)
        {
            break;
        }

        fbe_thread_wait(&thread_queue_element->enclosure_firmware_activate_thread);
        fbe_thread_destroy(&thread_queue_element->enclosure_firmware_activate_thread);
        fbe_terminator_free_memory(thread_queue_element);
    }

    terminator_encl_firmware_activate_thread_started = FBE_FALSE;

    return FBE_STATUS_OK;
}
