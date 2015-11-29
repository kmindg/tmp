/**************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/
/***************************************************************************
 *  fbe_terminator_por.c
 ***************************************************************************
 *
 *  Description
 *      terminator port class
 *
 *
 *  History:
 *      06/17/08    guov    Created
 *      02/07/11    Srini M Added new function to set affinity based on
 *                          algorithm (setting affinity to highest core)
 *
 ***************************************************************************/

#include "fbe/fbe_memory.h"
#include "terminator_port.h"
#include "terminator_login_data.h"
#include "terminator_drive.h"
#include "terminator_virtual_phy.h"
#include "terminator_fc_port.h"
#include "terminator_enclosure.h"
#include "terminator_fc_enclosure.h"
#include "fbe/fbe_topology_interface.h"
#include "terminator_simulated_disk_remote_file_simple.h"

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#include "ntddk.h"
#include "k10ntddk.h"
#endif // !(UMODE_ENV) && !(SIMMODE_ENV)

#include "csx_ext.h"

/******************************/
/*     local function         */
/******************************/
static void port_io_thread_func(void * context);
static void port_process_io_block(terminator_port_t * self);
static fbe_status_t mark_payload_abort(fbe_terminator_device_ptr_t device_ptr, fbe_terminator_io_t * terminator_io);
static fbe_status_t port_dequeue_io(terminator_port_t * self, fbe_terminator_device_ptr_t *handle);

/* For test purposes we may want to slow down all I/O completions */
static fbe_u32_t terminator_port_global_completion_delay = 0; /* 5 milisecond delay for all I/O's */

fbe_u32_t terminator_port_io_thread_count = TERMINATOR_PORT_IO_DEFAULT_THREAD_COUNT;
fbe_u32_t terminator_port_count           = 0;

/* Sets global completion delay in miliseconds */
fbe_status_t
fbe_terminator_miniport_api_set_global_completion_delay(fbe_u32_t global_completion_delay)
{
    terminator_port_global_completion_delay = global_completion_delay;
    return FBE_STATUS_OK;
}

/* Port object constructor:
 * Initialize common port attributes here.
 * Note:
 * Any attributes that belong to a special type of port are stored in ..._port_info block.
 * Need to be constructed separately, and attached to base componenent's attribute pointer.
 */
terminator_port_t * port_new(void)
{
    terminator_port_t * new_port = NULL;
    fbe_u32_t i;

    new_port = (terminator_port_t *)fbe_terminator_allocate_memory(sizeof(terminator_port_t));
    if (new_port == NULL)
    {
        /* Can't allocate the memory we need */
        return new_port;
    }
    base_component_init(&new_port->base);
    base_component_set_component_type(&new_port->base, TERMINATOR_COMPONENT_TYPE_PORT);

    //fbe_spinlock_init(&new_port->update_lock);
	fbe_mutex_init(&new_port->update_mutex);
    new_port->port_type = FBE_PORT_TYPE_INVALID;
    new_port->port_speed = FBE_PORT_SPEED_INVALID;
    port_set_maximum_transfer_bytes(new_port, FBE_CPD_USER_SHIM_MAX_TRANSFER_LENGTH);
    port_set_maximum_sg_entries(new_port, FBE_CPD_USER_SHIM_MAX_SG_ENTRIES);
    new_port->reset_begin = FBE_FALSE;
    new_port->reset_completed = FBE_FALSE;
    new_port->miniport_port_index = FBE_PORT_NUMBER_INVALID;
    fbe_queue_init(&new_port->logout_queue_head);
    fbe_spinlock_init(&new_port->logout_queue_lock);

    /* setup the io threads */
    /* all thread share the same semaphore */
    fbe_semaphore_init(&new_port->io_update_semaphore, 0, FBE_SEMAPHORE_MAX);
    for (i = 0; i < terminator_port_io_thread_count; i++)
    {
        new_port->io_thread_context[i].port_p = new_port;
        new_port->io_thread_context[i].io_thread_id = i;
        port_io_thread_init(&new_port->io_thread[i]);
        port_io_thread_start(&new_port->io_thread[i], port_io_thread_func, &new_port->io_thread_context[i]);
    }
    fbe_queue_init(&new_port->io_queue_head);
    fbe_queue_init(&new_port->keys_queue_head);
    fbe_spinlock_init(&new_port->keys_queue_spinlock);
    return new_port;
}
/* Port object deconstructor:
 * Release all the memory port object holds
 */
fbe_status_t port_destroy(terminator_port_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_queue_element_t * q_element = NULL;
    base_component_t * to_be_removed = NULL;
    logout_queue_element_t * logout_q_element = NULL;
    io_queue_element_t * io_q_element = NULL;
    terminator_key_info_t  *key_info = NULL;

    /* Clean up the logout queue.  Nothing should be left in this queue. */
    while (!fbe_queue_is_empty(&self->logout_queue_head))
    {
        logout_q_element = (logout_queue_element_t *)fbe_queue_pop(&self->logout_queue_head);
        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s port logout queue is not empty. Has device %p.\n",
                         __FUNCTION__, logout_q_element->device_id);
        fbe_terminator_free_memory(logout_q_element);
    }
    /*now we also need to stop the thread that is in charge of working with this port*/
    status = port_io_thread_destroy(self);

    fbe_semaphore_destroy(&self->io_update_semaphore);

    //fbe_spinlock_destroy(&self->update_lock);
	fbe_mutex_destroy(&self->update_mutex);
    fbe_spinlock_destroy(&self->logout_queue_lock);

   
    to_be_removed = (base_component_t *)self;

    /* Clean up the io queue.  Nothing should be left in this queue. */
    while (!fbe_queue_is_empty(&self->io_queue_head))
    {
        io_q_element = (io_queue_element_t *)fbe_queue_pop(&self->io_queue_head);
        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s port io queue is not empty.\n",
                         __FUNCTION__);
        fbe_terminator_free_memory(io_q_element);
    }

    while (!fbe_queue_is_empty(&self->keys_queue_head))
    {
        key_info = (terminator_key_info_t *)fbe_queue_pop(&self->keys_queue_head);
        terminator_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s port key queue is not empty.\n",
                         __FUNCTION__);
        fbe_terminator_free_memory(key_info);
    }

     fbe_spinlock_destroy(&self->keys_queue_spinlock);

    /* All children of the this port should be removed and destroyed by now.
     * Remove this port from it's parent and destroy it */
    q_element = base_component_get_queue_element(to_be_removed);
    status = base_component_remove_child(q_element);
    status = base_component_destroy(to_be_removed, FBE_TRUE);
    return status;
}

/* Common attribute setters and getters */
fbe_status_t port_set_type(terminator_port_t * self, fbe_port_type_t type)
{
    self->port_type = type;
    return FBE_STATUS_OK;
}
fbe_port_type_t port_get_type(terminator_port_t * self)
{
    return self->port_type;
}

fbe_status_t port_set_speed(terminator_port_t * self, fbe_port_speed_t port_speed)
{
    self->port_speed = port_speed;
    return FBE_STATUS_OK;
}
fbe_port_speed_t port_get_speed(terminator_port_t * self)
{
    return self->port_speed;
}

fbe_status_t port_set_maximum_transfer_bytes(terminator_port_t * self, fbe_u32_t maximum_transfer_bytes)
{
    self->maximum_transfer_bytes = maximum_transfer_bytes;
    return FBE_STATUS_OK;
}
fbe_u32_t port_get_maximum_transfer_bytes(terminator_port_t * self)
{
    return self->maximum_transfer_bytes;
}

fbe_status_t port_set_maximum_sg_entries(terminator_port_t * self, fbe_u32_t maximum_sg_entries)
{
    self->maximum_sg_entries = maximum_sg_entries;
    return FBE_STATUS_OK;
}
fbe_u32_t port_get_maximum_sg_entries(terminator_port_t * self)
{
    return self->maximum_sg_entries;
}

/* SAS port specific attributes */
terminator_sas_port_info_t * sas_port_info_new(fbe_terminator_sas_port_info_t *user_data)
{
    terminator_sas_port_info_t *new_info = NULL;

    new_info = (terminator_sas_port_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sas_port_info_t));
    if(new_info == NULL)
    {
        return NULL;
    }
    new_info->sas_address                     = user_data->sas_address;
    new_info->io_port_number                  = user_data->io_port_number;
    new_info->portal_number                   = user_data->portal_number;
    new_info->backend_number                  = user_data->backend_number;
    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;

    return new_info;
}

/* creates terminator_sas_port_info_t structure, that should be initialized */
terminator_sas_port_info_t * allocate_sas_port_info(void)
{
    terminator_sas_port_info_t *new_info = NULL;

    new_info = (terminator_sas_port_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sas_port_info_t));
    if(new_info == NULL)
    {
        return NULL;
    }
    new_info->sas_address = FBE_SAS_ADDRESS_INVALID;
    new_info->io_port_number = -1; /* what values should be used instead of -1 ??? */
    new_info->portal_number = -1;
    new_info->backend_number = -1;
    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;
    return new_info;
}

/* sas port specific attribute setters and getters */

fbe_sas_address_t sas_port_get_address(terminator_port_t * self)
{
    terminator_sas_port_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_SAS_ADDRESS_INVALID;
    }
    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_SAS_ADDRESS_INVALID;
    }
    return (info->sas_address);
}

void sas_port_set_address(terminator_port_t * self, fbe_sas_address_t sas_address)
{
    terminator_sas_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->sas_address = sas_address;
    }
    return;
}

fbe_port_connect_class_t port_get_connect_class(terminator_port_t * self)
{
    terminator_port_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
    }
    info = (terminator_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
    }
    return (info->connect_class);
}

fbe_status_t port_set_connect_class(terminator_port_t * self, fbe_port_connect_class_t connect_class)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->connect_class = connect_class;
    return FBE_STATUS_OK;
}

fbe_port_role_t port_get_role(terminator_port_t * self)
{
    terminator_port_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_CPD_SHIM_PORT_ROLE_INVALID;
    }
    info = (terminator_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_CPD_SHIM_PORT_ROLE_INVALID;
    }
    return (info->port_role);
}

fbe_status_t port_set_role(terminator_port_t * self, fbe_port_role_t port_role)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->port_role = port_role;
    return FBE_STATUS_OK;
}

fbe_status_t port_get_hardware_info(terminator_port_t * self, fbe_cpd_shim_hardware_info_t *hdw_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    CHECK_AND_COPY_STRUCTURE(hdw_info, &(info->hdw_info));
    return FBE_STATUS_OK;
}

fbe_status_t port_register_keys(terminator_port_t * self, fbe_base_port_mgmt_register_keys_t * port_register_keys_p)
{
    terminator_port_info_t *info = NULL;
    terminator_key_info_t  *key_info = NULL;
    fbe_u32_t index;
    fbe_u8_t *key_handle_p = NULL;

    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(port_register_keys_p->num_of_keys > FBE_PORT_MAX_KEYS_PER_REQUEST)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    key_handle_p = (fbe_u8_t*)port_register_keys_p->key_ptr;
    for(index = 0; index < port_register_keys_p->num_of_keys; index++)
    {
        key_info = (terminator_key_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_key_info_t));
        
        terminator_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s idx: %d/%d key: %s mp_key_handle: %p\n",
                         __FUNCTION__, index, port_register_keys_p->num_of_keys, key_handle_p, key_info);
        fbe_copy_memory(key_info->keys, key_handle_p, FBE_ENCRYPTION_KEY_SIZE);
        port_register_keys_p->mp_key_handle[index] = CSX_CAST_PTR_TO_PTRMAX(key_info);
        fbe_spinlock_lock(&self->keys_queue_spinlock);
        fbe_queue_push(&self->keys_queue_head, &key_info->queue_element);
        fbe_spinlock_unlock(&self->keys_queue_spinlock);
        key_handle_p += FBE_ENCRYPTION_KEY_SIZE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t port_unregister_keys(terminator_port_t * self, fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p)
{
    terminator_port_info_t *info = NULL;
    terminator_key_info_t  *key_info = NULL;
    fbe_u32_t index;

    if(self == NULL || 
       ((info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL) ||
       port_unregister_keys_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(index = 0; index < port_unregister_keys_p->num_of_keys; index++)
    {
        key_info = (terminator_key_info_t *)(fbe_ptrhld_t)port_unregister_keys_p->mp_key_handle[index];

        if(key_info == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_spinlock_lock(&self->keys_queue_spinlock);
        fbe_queue_remove(&key_info->queue_element);
        fbe_spinlock_unlock(&self->keys_queue_spinlock);
    
        fbe_terminator_free_memory(key_info);
    }

    return FBE_STATUS_OK;
}


fbe_status_t port_reestablish_key_handle(terminator_port_t * self, fbe_base_port_mgmt_reestablish_key_handle_t * port_reestablish_key_handle_p)
{
    terminator_port_info_t *info = NULL;
    terminator_key_info_t  *key_info = NULL;
    fbe_bool_t element_on_queue;

    if(self == NULL || 
       ((info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL) ||
       port_reestablish_key_handle_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    key_info = (terminator_key_info_t *)(fbe_ptrhld_t)port_reestablish_key_handle_p->mp_key_handle;


	/* In simulation SP can be rebooted and the memory can be lost  we may want to persist it somehow on a file system */
	return FBE_STATUS_OK;

    if (key_info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&self->keys_queue_spinlock);
    element_on_queue = fbe_queue_is_element_on_queue(&key_info->queue_element);
    fbe_spinlock_unlock(&self->keys_queue_spinlock);

    if(element_on_queue)
    {
        return FBE_STATUS_OK;
    }
    else
    {
       return FBE_STATUS_GENERIC_FAILURE;
    }
}
fbe_status_t port_set_hardware_info(terminator_port_t * self, fbe_cpd_shim_hardware_info_t *hdw_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    CHECK_AND_COPY_STRUCTURE(&(info->hdw_info), hdw_info);
    return FBE_STATUS_OK;
}

fbe_status_t port_get_sfp_media_interface_info(terminator_port_t * self, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    CHECK_AND_COPY_STRUCTURE(sfp_media_interface_info, &(info->sfp_media_interface_info));
    return FBE_STATUS_OK;
}

fbe_status_t port_set_sfp_media_interface_info(terminator_port_t * self, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    CHECK_AND_COPY_STRUCTURE(&(info->sfp_media_interface_info), sfp_media_interface_info);
    return FBE_STATUS_OK;
}

fbe_status_t port_get_port_link_info(terminator_port_t * self, fbe_cpd_shim_port_lane_info_t *port_lane_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    CHECK_AND_COPY_STRUCTURE(port_lane_info, &(info->port_lane_info));
    return FBE_STATUS_OK;
}

fbe_status_t port_get_encryption_key(terminator_port_t *self, fbe_key_handle_t key_handle, fbe_u8_t *key_buffer)
{
    terminator_key_info_t  *key_info = NULL;
    if(self == NULL || (key_handle == FBE_INVALID_KEY_HANDLE) || (key_buffer == NULL))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    key_info = (terminator_key_info_t  *)(fbe_ptrhld_t)key_handle;
    fbe_copy_memory(key_buffer, &key_info->keys, FBE_ENCRYPTION_KEY_SIZE);
    return FBE_STATUS_OK;
}

fbe_status_t port_set_port_link_info(terminator_port_t * self, fbe_cpd_shim_port_lane_info_t *port_lane_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    CHECK_AND_COPY_STRUCTURE(&(info->port_lane_info), port_lane_info);
    port_set_speed(self,port_lane_info->link_speed);
    return FBE_STATUS_OK;
}

fbe_status_t port_get_port_configuration(terminator_port_t * self, fbe_cpd_shim_port_configuration_t *port_config_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (port_config_info == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    port_config_info->connect_class = info->connect_class;
    port_config_info->port_role = info->port_role;
    switch(port_config_info->connect_class){
        case FBE_CPD_SHIM_CONNECT_CLASS_FC:
        case FBE_CPD_SHIM_CONNECT_CLASS_ISCSI:
        case FBE_CPD_SHIM_CONNECT_CLASS_FCOE:
            port_config_info->flare_bus_num = fc_port_get_backend_number(self);
            break;
        case FBE_CPD_SHIM_CONNECT_CLASS_SAS:
            port_config_info->flare_bus_num = sas_port_get_backend_number(self);
            break;
    }
    return FBE_STATUS_OK;
}

fbe_status_t port_set_port_configuration(terminator_port_t * self, fbe_cpd_shim_port_configuration_t *port_config_info)
{
    terminator_port_info_t *info = NULL;
    if(self == NULL || (info = (terminator_port_info_t *)base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (port_config_info == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    info->connect_class = port_config_info->connect_class;
    info->port_role = port_config_info->port_role;    
    switch(port_config_info->connect_class){
        case FBE_CPD_SHIM_CONNECT_CLASS_FC:
        case FBE_CPD_SHIM_CONNECT_CLASS_ISCSI:
        case FBE_CPD_SHIM_CONNECT_CLASS_FCOE:
            fc_port_set_backend_number(self,port_config_info->flare_bus_num);
            break;
        case FBE_CPD_SHIM_CONNECT_CLASS_SAS:
            sas_port_set_backend_number(self,port_config_info->flare_bus_num);
            break;
    }

    return FBE_STATUS_OK;
}
//fbe_terminator_api_device_handle_t sas_port_get_device_id(terminator_port_t * self)
//{
//    terminator_sas_port_info_t *info = NULL;
//    if (self == NULL)
//    {
//        return -1;
//    }
//    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
//    if (info == NULL)
//    {
//        return -1;
//    }
//    return (info->device_id);
//}

static fbe_u32_t terminator_port_get_backend_number(terminator_port_t * self)
{
    terminator_sas_port_info_t *info = NULL;
    if (self == NULL)
    {
        return -1;
    }
    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return -1;
    }
    return (info->backend_number);
}

fbe_u32_t sas_port_get_backend_number(terminator_port_t * self)
{
    return terminator_port_get_backend_number(self);
}

void sas_port_set_backend_number(terminator_port_t * self, fbe_u32_t port_number)
{
    terminator_sas_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->backend_number = port_number;
    }
    return;
}

static fbe_u32_t terminator_port_get_io_port_number(terminator_port_t * self)
{
    terminator_sas_port_info_t *info = NULL;
    if (self == NULL)
    {
        return -1;
    }
    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return -1;
    }
    return (info->io_port_number);
}

fbe_u32_t sas_port_get_io_port_number(terminator_port_t * self)
{
    return terminator_port_get_io_port_number(self);
}

static fbe_u32_t terminator_port_get_portal_number(terminator_port_t * self)
{
    terminator_sas_port_info_t *info = NULL;
    if (self == NULL)
    {
        return -1;
    }

    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return -1;
    }
    return (info->portal_number);
}

fbe_u32_t sas_port_get_portal_number(terminator_port_t * self)
{
    return terminator_port_get_portal_number(self);
}

void  sas_port_set_io_port_number(terminator_port_t * self, fbe_u32_t io_port_number)
{
    terminator_sas_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->io_port_number = io_port_number;
    }
    return;
}

void sas_port_set_portal_number(terminator_port_t * self, fbe_u32_t portal_number)
{
    terminator_sas_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_sas_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->portal_number = portal_number;
    }
    return;
}


/* Port object methods */
terminator_enclosure_t *port_get_last_enclosure(terminator_port_t * self)
{
    base_component_t * child = NULL;
    terminator_enclosure_t * last_enclosure = NULL;

    child = base_component_get_first_child(&self->base);
    if ((child != NULL) && (child->component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE))
    {
        last_enclosure = enclosure_get_last_sibling_enclosure(child);
    }
    return last_enclosure;
}

fbe_status_t port_enumerate_devices(terminator_port_t * self, fbe_terminator_device_ptr_t device_list[], fbe_u32_t *total_devices)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * child = NULL;
    *total_devices = 0;

    /* Lock the port configuration so no one else can change it while we getting configuration */
    port_lock_update_lock(self) ;

    child = base_component_get_first_child(&self->base);
    if (child == NULL)
    {
        status = FBE_STATUS_OK;
    }
    else
    {
        /* Add a lock here, since the login_pending and logout flag can be changed at any time now.
         * We want to wait until the change complete then get the device list
         */
        device_list[*total_devices] = child;
        *total_devices = *total_devices + 1;
        status = enclosure_enumerate_devices((terminator_enclosure_t *)child, device_list, total_devices);
    }

    port_unlock_update_lock(self) ;

    return status;
}

//base_component_t * port_get_device_by_device_id(terminator_port_t * self, fbe_terminator_api_device_handle_t device_id)
//{
//    base_component_t * matching_device = NULL;
//    base_component_t * child = NULL;
//    base_component_t * self_base = &self->base;
//
//    if (self == NULL)
//    {
//        return matching_device;
//    }
//    if (base_component_get_id(self_base) == device_id)
//    {
//        return self_base;
//    }
//    matching_device = base_component_get_child_by_id(self_base, device_id);
//    if (matching_device == NULL)
//    {
//        child = base_component_get_child_by_type(self_base, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
//        if (child == NULL)
//        {
//            return matching_device;
//        }
//        matching_device = enclosure_get_device_by_device_id((terminator_enclosure_t *)child, device_id);
//    }
//    return matching_device;
//}

fbe_status_t port_add_sas_enclosure(terminator_port_t * self, terminator_enclosure_t * new_enclosure, fbe_bool_t new_virtual_phy)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * enclosure_base = NULL;
    terminator_enclosure_t * last_enclosure = NULL;
    terminator_sas_enclosure_info_t *attributes = NULL;
    fbe_miniport_device_id_t parent_device_id = CPD_DEVICE_ID_INVALID;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_miniport_device_id_t cpd_device_id;

    enclosure_base = &new_enclosure->base;

    /* Lock the port configuration so no one else can change it while we modify configuration */
    port_lock_update_lock(self) ;

    /* the first enclosure of a port connects to the port, the rest connects to the last enclosure */
    if (base_component_get_first_child(&self->base) == NULL)
    {
        parent_device_address = sas_port_get_address(self);
        parent_device_id = CPD_DEVICE_ID_INVALID;/*sas_port_get_device_id(self)*/
        enclosure_chain_depth = 0;
        /* Add this enclosure to the port's child_list */
        status = base_component_add_child(&self->base, base_component_get_queue_element(enclosure_base));
    }
    else /* this is the case where enclosure should be added to the last enclosure */
    {
        last_enclosure = port_get_last_enclosure(self);
        if (last_enclosure == NULL)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            port_unlock_update_lock(self) ;
            return status;
        }
        attributes = base_component_get_attributes(&last_enclosure->base);
        login_data = sas_enclosure_info_get_login_data(attributes);
        parent_device_id = cpd_shim_callback_login_get_device_id(login_data);
        parent_device_address = cpd_shim_callback_login_get_device_address(login_data);
        enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(login_data) + 1;
        /* Add this enclosure to the last enclosure's child_list */
        status = base_component_add_child(&last_enclosure->base, base_component_get_queue_element(enclosure_base));
    }
    attributes = base_component_get_attributes(enclosure_base);
    login_data = sas_enclosure_info_get_login_data(attributes);
    /* Use the new device id format */
    terminator_add_device_to_miniport_sas_device_table(new_enclosure);
    status = terminator_get_cpd_device_id_from_miniport(new_enclosure,
                                                        &cpd_device_id);
    cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);

    cpd_shim_callback_login_set_parent_device_id(login_data, parent_device_id);
    cpd_shim_callback_login_set_parent_device_address(login_data, parent_device_address);
    cpd_shim_callback_login_set_enclosure_chain_depth(login_data, enclosure_chain_depth);

    if(new_virtual_phy)
    {
        /* Generate and attach the virtual phy of this enclosure */
        status = sas_enclosure_generate_virtual_phy(new_enclosure);
    }

//#if NOT_SURE_IF_BO_ADDED_THIS_OR_NAIZONG_REMOVED_IT
    /* update child enclosure chain depth in case it's an existing enclosure with subtree */
    status = sas_enclosure_update_child_enclosure_chain_depth(new_enclosure);
    if (status != FBE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        port_unlock_update_lock(self) ;
        return status;
    }
//#endif

    status = FBE_STATUS_OK;
    port_unlock_update_lock(self) ;
    return status;
}
//fbe_status_t port_get_device_login_data (terminator_port_t * self, fbe_terminator_api_device_handle_t device_id, fbe_cpd_shim_callback_login_t *login_data)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * matching_device = NULL;
//
//    matching_device = port_get_device_by_device_id(self, device_id);
//    if (matching_device == NULL)
//    {
//        return status;
//    }
//    status = get_device_login_data(matching_device, login_data);
//    status = FBE_STATUS_OK;
//    return status;
//}
fbe_status_t port_destroy_device(terminator_port_t * self,
                                fbe_terminator_device_ptr_t component_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_queue_element_t * q_element = NULL;

    /* Lock the port configuration so no one else can change it while we modify configuration */
    port_lock_update_lock(self) ;

    status = base_component_remove_all_children(component_handle);

    /* All children of the this device should be removed and destroyed by now.
     * Remove this device from it's parent and destroy it */
    q_element = base_component_get_queue_element(component_handle);
    status = base_component_remove_child(q_element);
    status = base_component_destroy(component_handle, FBE_TRUE);
    port_unlock_update_lock(self) ;
    return status;
}

fbe_status_t port_unmount_device(terminator_port_t * self,
                                fbe_terminator_device_ptr_t component_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_queue_element_t * q_element = NULL;

    /* Lock the port configuration so no one else can change it while we modify configuration */
    port_lock_update_lock(self) ;

    if(((base_component_t*)component_handle)->component_type == TERMINATOR_COMPONENT_TYPE_DRIVE)
    {
        set_terminator_simulated_drive_flag_to_drive_pulled((base_component_t*)component_handle);
    }
    
    /* Remove this device from it's parent but do NOT destroy it */
    q_element = base_component_get_queue_element(component_handle);
    status = base_component_remove_child(q_element);
    port_unlock_update_lock(self) ;
    return status;
}

fbe_status_t port_logout_device(terminator_port_t * self, fbe_terminator_device_ptr_t component_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    /* allocate a logout_queue_element */
    logout_queue_element_t * logout_q_element =
        (logout_queue_element_t *)fbe_terminator_allocate_memory(sizeof(logout_queue_element_t));
    if(logout_q_element == NULL)
    {
        return status;
    }
    /* Lock the port configuration so no one else can change it while we modify configuration */
    port_lock_update_lock(self) ;
    fbe_spinlock_lock(&self->logout_queue_lock);
    status = base_component_add_all_children_to_logout_queue(component_handle, &self->logout_queue_head);
    /* All children of the this device should be added to the logout queue by now.
     * add itself to logout queue */
    /* fill the device id and add it to logout q */
    fbe_queue_element_init(&logout_q_element->queue_element);
    logout_q_element->device_id = component_handle;

    /* add it to logout queue */
    fbe_queue_push(&self->logout_queue_head, &logout_q_element->queue_element);
    // set component state
    base_component_set_state((base_component_t *)component_handle, TERMINATOR_COMPONENT_STATE_LOGOUT_PENDING);

    fbe_spinlock_unlock(&self->logout_queue_lock);
    port_unlock_update_lock(self) ;
    return status;
}

fbe_status_t port_logout(terminator_port_t * self)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* allocate a logout_queue_element */
    logout_queue_element_t * logout_q_element =
        (logout_queue_element_t *)fbe_terminator_allocate_memory(sizeof(logout_queue_element_t));
    if(logout_q_element == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Lock the port configuration so no one else can change it while we modify configuration */
    port_lock_update_lock(self) ;
    fbe_spinlock_lock(&self->logout_queue_lock);
    /* All children of the this device should be added to the logout queue by now.
     * add itself to logout queue */
    /* fill the device id and add it to logout q */
    fbe_queue_element_init(&logout_q_element->queue_element);
    logout_q_element->device_id = self;
    /* add it to logout queue */
    fbe_queue_push(&self->logout_queue_head, &logout_q_element->queue_element);
    // set component state
    base_component_set_state((base_component_t *)self, TERMINATOR_COMPONENT_STATE_LOGOUT_PENDING);

    fbe_spinlock_unlock(&self->logout_queue_lock);
    port_unlock_update_lock(self) ;
    return status;
}

fbe_status_t port_enqueue_io(terminator_port_t * self,
                             fbe_terminator_device_ptr_t handle,
                             fbe_queue_element_t * element_to_enqueue)
{
    io_queue_element_t  * io_queue_element;

    io_queue_element = (io_queue_element_t *)fbe_terminator_allocate_memory(sizeof(io_queue_element_t));
    if (io_queue_element == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for IO queue element at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize the IO queue element */
    fbe_queue_element_init(&io_queue_element->queue_element);
    io_queue_element->device = handle;

    /* Caller must lock the tree
     * Note that we are going to enqueue to the device's io queue and to the port's io
     * queue under the protection of the port lock.
     * The dequeue also happens under protection of the port lock.
     */
    /* enqueue the io to the device under protection of the above lock.  */
    base_component_io_queue_push(handle, element_to_enqueue);
    /* enqueue the io to the port */
    fbe_queue_push(&self->io_queue_head, &io_queue_element->queue_element);
    /* wake up io thread */
    fbe_semaphore_release(&self->io_update_semaphore, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

/* This function assumes the caller has the port lock.
 */
static fbe_status_t port_dequeue_io(terminator_port_t * self, fbe_terminator_device_ptr_t *handle)
{
    fbe_queue_element_t *io_q_element = NULL;
    io_queue_element_t  * io_queue_element = NULL;

    io_q_element = fbe_queue_pop(&self->io_queue_head);
    if (io_q_element != NULL)
    {
        io_queue_element = (io_queue_element_t *)io_q_element;
        *handle = io_queue_element->device;
        io_queue_element->device = NULL;
        fbe_terminator_free_memory(io_queue_element);
    }
    else
    {
        *handle = NULL;
    }

    return FBE_STATUS_OK;
}

/************************************/
/* Port IO thread related functions */
/************************************/
/* this thread runs in a context different than that of the caller who generated the IO*/
static void port_io_thread_func(void * context)
{
    EMCPAL_STATUS       nt_status;
    terminator_port_io_thread_context_t * io_thread_context_p = (terminator_port_io_thread_context_t *)context;
    terminator_port_t * port = io_thread_context_p->port_p;
    port_io_thread_t * port_io_thread = &port->io_thread[io_thread_context_p->io_thread_id];

#if !defined(C4_INTEGRATED) && !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
    csx_u64_t heartbeat_timestamp = 0;
#endif // !(C4_INTEGRATED) && !(UMODE_ENV) && !(SIMMODE_ENV)

    /* If we are not building code under user mode or simulation mode, then
     *  we must be building VM Simulator, so include code to set thread priority
     *  to LOW_REALTIME_PRIORITY.
     */
#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
    EmcpalThreadPrioritySet(EMCPAL_THREAD_PRIORITY_REALTIME_LOW);
#endif // !(UMODE_ENV) && !(SIMMODE_ENV)

    /* At one time with the VMsimulator we were affining to the highest core.
     * With MCR we should no longer need this affining so we are disabling this for now.
     */
    //fbe_thread_set_affinity_mask_based_on_algorithm(&port_io_thread->io_thread_handle, HIGHEST_CORE); /* SAFERELOCATED */

    for (;;)
    {
/* this code is causing panics on KH SIM, disabling for now */
#if !defined(C4_INTEGRATED) && !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
        EMCPAL_LARGE_INTEGER timeout;
        timeout.QuadPart = (-TERMINATOR_PORT_IO_TIMEOUT_SEC * CSX_XSEC_PER_SEC); /* 3 seconds (relative time) */

        /* nt_status will only be checked in the following when the port IO thread is running
         */
        nt_status = fbe_semaphore_wait(&port->io_update_semaphore, &timeout);

        if (port_io_thread->io_thread_flag != IO_THREAD_RUN)
        {
            break;
        }

        /* The max interval of two successive heartbeat messages from Terminator to DAE is
         *  (TERMINATOR_PORT_IO_TIMEOUT_SEC * 2).
         *
         * For example, if we set TERMINATOR_PORT_IO_TIMEOUT_SEC to 3 (seconds), a
         *  first heartbeat message is sent at 00:00:00, then there is a port IO coming at the
         *  time just before 00:00:03, so no heartbeat message will be sent.  If there is another
         *  port IO coming at the time just before 00:00:06 or we have met timeout, a second
         *  heartbeat message will be sent out, then the interval between first and second is about
         *  6 seconds but a little bit less than it.
         */
        if (nt_status == EMCPAL_STATUS_TIMEOUT)
        {
            send_heartbeat_message();
            heartbeat_timestamp = csx_p_get_ticks64();

            continue;
        }

        if ((csx_p_get_ticks64() - heartbeat_timestamp) >= TERMINATOR_PORT_IO_TIMEOUT_SEC * FBE_TIME_MILLISECONDS_PER_SECOND)
        {
            send_heartbeat_message();
            heartbeat_timestamp = csx_p_get_ticks64();
        }

        if (terminator_port_global_completion_delay != 0)
        {
            fbe_thread_delay(terminator_port_global_completion_delay);
        }

        port_process_io_block(port);
#else
        /*block until someone release the semaphore*/
        nt_status = fbe_semaphore_wait(&port->io_update_semaphore, NULL);

        /*make sure we are not suppose to get out of here*/
        if(port_io_thread->io_thread_flag == IO_THREAD_RUN)
        {
            if(terminator_port_global_completion_delay != 0){
                fbe_thread_delay(terminator_port_global_completion_delay);
            }
            port_process_io_block(port);
        }
        else
        {
            break;
        }
#endif // !(C4_INTEGRATED) && !(UMODE_ENV) && !(SIMMODE_ENV)
    }

    port_io_thread->io_thread_flag = IO_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void port_process_io_block(terminator_port_t * self)
{
    fbe_terminator_device_ptr_t device;
    fbe_terminator_io_t * terminator_io = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_component_type_t component_type;

    /* lock the tree
     *
     * Note that we are going to dequeue from the port's io queue and device's io
     * queue under the protection of this lock. The enqueue to both these queues
     * also occurs under protection of this lock.
     */
    port_lock_update_lock(self);
    /* get a device from the port IO queue */
    port_dequeue_io(self, &device);
    if (NULL == device)
    {
        /* unlock the tree */
        port_unlock_update_lock(self);
        terminator_trace(FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: did not find device with pending IO!, port 0x%X\n", __FUNCTION__, self->miniport_port_index);
        return;
    }
    /* lock the device IO queue */
    base_component_lock_io_queue(device);
    if (!base_component_io_queue_is_empty(device))
    {
        /* pop an IO block from the device IO queue */
        terminator_io = (fbe_terminator_io_t *)base_component_io_queue_pop(device);
    }
    /* unlock the device IO queue */
    base_component_unlock_io_queue(device);

    if (NULL == terminator_io)
    {
        /* unlock the tree */
        port_unlock_update_lock(self);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: did not find an IO block!, port 0x%X\n", __FUNCTION__, self->miniport_port_index);
        return;
    }

    /* execute IO: base on component type call the right send_io_to...
       with the correct info data */
    terminator_io->miniport_port_index = self->miniport_port_index;
    terminator_io->is_pending = 0;
    terminator_io->memory_ptr = NULL;
    terminator_io->xfer_data = NULL;
    terminator_io->block_size = 0;
    terminator_io->collision_found = 0;
    terminator_io->b_key_valid = FBE_FALSE;
    fbe_zero_memory(&terminator_io->u, sizeof(terminator_io->u));


    component_type = base_component_get_component_type((base_component_t*)device);

    switch(component_type)
    {
    /* those are the components that can accept IO */
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        status = sas_enclosure_call_io_api((terminator_enclosure_t *)device, terminator_io);
        /* unlock the tree */
        port_unlock_update_lock(self);
        break;
    case TERMINATOR_COMPONENT_TYPE_DRIVE:
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
        status = sas_drive_call_io_api((terminator_drive_t *)device, terminator_io);
        /* unlock the tree */
        port_unlock_update_lock(self);
#else
        /* unlock the tree */
        port_unlock_update_lock(self);
        status = sas_drive_call_io_api((terminator_drive_t *)device, terminator_io);
#endif // UMODE_ENV || SIMMODE_ENV
        break;
    case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
        /* unlock the tree */
        port_unlock_update_lock(self);
        status = sas_virtual_phy_call_io_api((terminator_virtual_phy_t *)device, terminator_io);
        break;
    default:
        /* unlock the tree */
        port_unlock_update_lock(self);
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: failed to match the device type\n", __FUNCTION__);
        break;
    }
    /* 6)call back to complete io using fbe_terminator_miniport_api_complete_io() */
    if (status != FBE_STATUS_PENDING) { /* CGCG - SAFEBUG - don't complete the I/O unless it is synchronous */
    fbe_terminator_miniport_api_complete_io (self->miniport_port_index, terminator_io);
    }
}

fbe_status_t port_io_thread_init(port_io_thread_t * self)
{
    self->io_thread_flag = IO_THREAD_RUN;
    return FBE_STATUS_OK;
}
fbe_status_t port_io_thread_start(port_io_thread_t * io_thread, IN fbe_thread_user_root_t  StartRoutine, IN PVOID  StartContext)
{
    EMCPAL_STATUS       nt_status;

    nt_status = fbe_thread_init(&io_thread->io_thread_handle, "fbe_term_pio", StartRoutine, StartContext);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: can't start terminator_io_thread %X\n", __FUNCTION__, nt_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t port_io_thread_destroy(terminator_port_t * self)
{
    fbe_u32_t i;

    for (i = 0; i < terminator_port_io_thread_count; i++)
    {
        /*now we also need to stop the thread that is in charge of working with this port*/
        self->io_thread[i].io_thread_flag = IO_THREAD_STOP;
    }
    for (i = 0; i < terminator_port_io_thread_count; i++)
    {
        fbe_semaphore_release(&self->io_update_semaphore, 0, 1, FALSE);
    }

    for (i = 0; i < terminator_port_io_thread_count; i++)
    {
        /*wait for it to end*/
        fbe_thread_wait(&self->io_thread[i].io_thread_handle);
        fbe_thread_destroy(&self->io_thread[i].io_thread_handle);
    }
    return FBE_STATUS_OK;
}

fbe_status_t port_login_device(terminator_port_t * self, fbe_terminator_device_ptr_t component_handle)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;


    fbe_bool_t pending = FBE_TRUE;
    fbe_bool_t logut_complete = FBE_FALSE;

    if (self == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Invalid Port ptr\n",
                         __FUNCTION__);
        return status;
    }

    /* Lock the port configuration so no one else can change it while we modify configuration */
    port_lock_update_lock(self) ;
    status = base_component_set_all_children_logout_complete(component_handle, logut_complete);
    status = base_component_set_all_children_login_pending(component_handle, pending);

    /* All children of the this device should be marked login_pending and logout_complete is cleared.
       Now set it's own flag.
     */
    base_component_set_logout_complete(component_handle, logut_complete);
    base_component_set_login_pending(component_handle, pending);
    /* we are done, release the lock*/
    port_unlock_update_lock(self) ;
    return status;
}

fbe_status_t port_pop_logout_device(terminator_port_t * self, fbe_terminator_device_ptr_t *device_id)
{
    fbe_queue_element_t *q_element = NULL;
    logout_queue_element_t *element = NULL;

    fbe_spinlock_lock(&self->logout_queue_lock);
    q_element = fbe_queue_pop(&self->logout_queue_head);
    fbe_spinlock_unlock(&self->logout_queue_lock);
    if (q_element == NULL)
    {
        *device_id = DEVICE_HANDLE_INVALID;
        return FBE_STATUS_OK;
    }
    element = (logout_queue_element_t *)q_element;
    *device_id = element->device_id;
    fbe_terminator_free_memory(element);
    return FBE_STATUS_OK;
}

fbe_status_t port_get_next_logout_device(terminator_port_t * self, const fbe_terminator_device_ptr_t device_id, fbe_terminator_device_ptr_t *next_device_id)
{
    fbe_queue_element_t *q_element = NULL;
    logout_queue_element_t *element = NULL;
    fbe_terminator_device_ptr_t current_id;

    *next_device_id = DEVICE_HANDLE_INVALID;
    q_element = fbe_queue_front(&self->logout_queue_head);
    while (q_element != NULL)
    {
        element = (logout_queue_element_t *)q_element;
        current_id = element->device_id;
        if (current_id == device_id)
        {
            q_element = fbe_queue_next(&self->logout_queue_head, q_element);
            if (q_element != NULL)
            {
                element = (logout_queue_element_t *)q_element;
                *next_device_id = element->device_id;
            }
            return FBE_STATUS_OK;
        }
        q_element = fbe_queue_next(&self->logout_queue_head, q_element);
    }
    return FBE_STATUS_OK;
}

fbe_status_t port_front_logout_device(terminator_port_t * self, fbe_terminator_device_ptr_t *device_id)
{
    fbe_queue_element_t *q_element = NULL;
    logout_queue_element_t *element = NULL;

    q_element = fbe_queue_front(&self->logout_queue_head);
    if (q_element == NULL)
    {
        *device_id = DEVICE_HANDLE_INVALID;
        return FBE_STATUS_OK;
    }
    element = (logout_queue_element_t *)q_element;
    *device_id = element->device_id;
    return FBE_STATUS_OK;
}

//fbe_status_t port_set_device_logout_complete(terminator_port_t * self, fbe_terminator_device_ptr_t device_id, fbe_bool_t flag)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * device = NULL;
//
//    device = port_get_device_by_device_id(self, device_id);
//    if (device == NULL)
//    {
//        /* Did not find a device with matching device_id, return a failure status */
//        return status;
//    }
//    base_component_set_logout_complete(device, flag);
//    status = FBE_STATUS_OK;
//    return status;
//
//}
//fbe_status_t port_get_device_logout_complete(terminator_port_t * self, fbe_terminator_device_ptr_t device_id, fbe_bool_t *flag)
//{
//    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
//    base_component_t * device = NULL;
//
//    device = port_get_device_by_device_id(self, device_id);
//    if (device == NULL)
//    {
//        /* Did not find a device with matching device_id, return a failure status */
//        return status;
//    }
//    *flag = base_component_get_logout_complete(device);
//    status = FBE_STATUS_OK;
//    return status;
//}

fbe_status_t port_is_logout_complete(terminator_port_t * self, fbe_bool_t *flag)
{
    *flag = base_component_get_logout_complete(&self->base);
    return FBE_STATUS_OK;
}
/* This function assumes the caller has the port lock.*/
static fbe_status_t port_remove_device_from_io_queue(terminator_port_t * self, fbe_terminator_device_ptr_t ptr)
{
    io_queue_element_t * io_queue_element = NULL;
    io_queue_element_t * next_queue_element = NULL;
    next_queue_element = (io_queue_element_t *)fbe_queue_front(&self->io_queue_head);

    while(next_queue_element != NULL)
    {
        io_queue_element = next_queue_element;
        next_queue_element = (io_queue_element_t *)fbe_queue_next(&self->io_queue_head, &next_queue_element->queue_element);
        if (io_queue_element->device == ptr)
        {
            /*remove from the queue*/
            fbe_queue_remove(&io_queue_element->queue_element);
            fbe_terminator_free_memory(io_queue_element);
            terminator_trace(FBE_TRACE_LEVEL_WARNING,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: device %p removed from Port %d IO Q.\n",
                             __FUNCTION__, ptr, sas_port_get_backend_number(self));
        }

    }
    return FBE_STATUS_OK;
}

fbe_status_t port_abort_device_io(terminator_port_t * self, fbe_terminator_device_ptr_t device_ptr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_terminator_io_t * terminator_io = NULL;

    /* lock the port and remove all the entry of this device from the port IO Q*/
    port_lock_update_lock(self);
    status = port_remove_device_from_io_queue(self, device_ptr);

    port_unlock_update_lock(self);

    base_component_lock_io_queue(device_ptr);
    /* go thru the io queue and abort all the io */
    while(base_component_io_queue_is_empty(device_ptr)!= FBE_TRUE)
    {
        /* pop an IO block from io queue */
        terminator_io = (fbe_terminator_io_t *) base_component_io_queue_pop(device_ptr);
        if (terminator_io == NULL)
        {
            terminator_trace(FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: find a NULL io block!, port 0x%X\n", __FUNCTION__, self->miniport_port_index);
        }
        else
        {
            terminator_trace(FBE_TRACE_LEVEL_WARNING,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: found an IO block on port 0x%X device 0x%p!\n", __FUNCTION__, self->miniport_port_index, device_ptr);
            /* Mark IO abort */
            status = mark_payload_abort(device_ptr, terminator_io);
            /* call back to complete io using fbe_terminator_miniport_api_complete_io() */
            status = fbe_terminator_miniport_api_complete_io (self->miniport_port_index, terminator_io);
        }
    }
    base_component_unlock_io_queue(device_ptr);

    return status;
}

static fbe_status_t mark_payload_abort(fbe_terminator_device_ptr_t device_ptr, fbe_terminator_io_t * terminator_io)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *payload = NULL;
    terminator_component_type_t device_type;

    payload = terminator_io->payload ;
    if (payload == NULL)
    {
        return status;
    }
    status = terminator_get_device_type(device_ptr, &device_type);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    switch(device_type)
    {
    case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
        status = cdb_mark_payload_abort(payload);
        break;
    case TERMINATOR_COMPONENT_TYPE_DRIVE:
        if (drive_get_protocol(device_ptr) == FBE_DRIVE_TYPE_SAS ||
            drive_get_protocol(device_ptr) == FBE_DRIVE_TYPE_SAS_FLASH_HE ||
            drive_get_protocol(device_ptr) == FBE_DRIVE_TYPE_SAS_FLASH_ME ||
            drive_get_protocol(device_ptr) == FBE_DRIVE_TYPE_SAS_FLASH_LE ||
            drive_get_protocol(device_ptr) == FBE_DRIVE_TYPE_SAS_FLASH_RI)
        {
            status = cdb_mark_payload_abort(payload);
        }
        else /* SATA drive, abort fis*/
        {
            status = fis_mark_payload_abort(payload);
        }
        break;
    default:
        terminator_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: Unsupported device type %d!\n", __FUNCTION__, device_type);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    return status;
}

base_component_t * port_get_enclosure_by_chain_depth(terminator_port_t * self, fbe_u8_t enclosure_chain_depth)
{
    base_component_t * child = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    child = base_component_get_first_child(&self->base);
    while(child != NULL)
    {
        login_data = sas_enclosure_info_get_login_data(base_component_get_attributes(child));
        if (enclosure_chain_depth == cpd_shim_callback_login_get_enclosure_chain_depth(login_data))
        {
            return child;
        }
        /* get the next enclosure */
        child = base_component_get_child_by_type(child, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
    }
    return child;
}

/* returns an enclosure with matching enclosure number attribute */
terminator_enclosure_t * port_get_enclosure_by_enclosure_number(terminator_port_t * self, fbe_u32_t enclosure_number)
{
    base_component_t * child = NULL;
    void * attributes = NULL;
    fbe_enclosure_type_t encl_protocol;
    child =  base_component_get_first_child(&self->base);
    while(child != NULL)
    {
        if(base_component_get_component_type(child) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
        {

           encl_protocol = enclosure_get_protocol((terminator_enclosure_t *)child);
           attributes = base_component_get_attributes(child);
            switch(encl_protocol)
            {
                case FBE_ENCLOSURE_TYPE_SAS:                    
                    if((attributes != NULL) && ((sas_enclosure_get_enclosure_number((terminator_enclosure_t *) child)) == (fbe_u64_t)enclosure_number))                    
                         return (terminator_enclosure_t *)child;                   
                break;
                
                case FBE_ENCLOSURE_TYPE_FIBRE:
                    if((attributes != NULL) && ((fc_enclosure_get_enclosure_number((terminator_enclosure_t *) child))== (fbe_u64_t)enclosure_number))                    
                         return (terminator_enclosure_t *)child;
                break;                      
             }        
        }
        /* get the next enclosure */
        child = base_component_get_child_by_type(child, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
    }
    return (terminator_enclosure_t *)child;
}

/* this function is used to generate enclosure number for old creation API */
fbe_u32_t port_get_enclosure_count(terminator_port_t * self)
{
    fbe_u32_t encl_counter = 0;
    base_component_t * child = base_component_get_first_child(&self->base);
    while(child != NULL)
    {
        if(base_component_get_component_type(child) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
        {
            encl_counter++;
        }
        /* get the next enclosure */
        child = base_component_get_child_by_type(child, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
    }
    return encl_counter;
}

/* this function walk thru the logout queue and check if the given device id is on the queue.  Queue should be locked during this operation */
fbe_status_t port_is_device_logout_pending(terminator_port_t * self, fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *pending)
{
    fbe_queue_element_t *q_element = NULL;
    logout_queue_element_t *element = NULL;
    *pending = FBE_FALSE;

    fbe_spinlock_lock(&self->logout_queue_lock);
    q_element = fbe_queue_front(&self->logout_queue_head);
    while (q_element != NULL)
    {
        element = (logout_queue_element_t *)q_element;
        if (element->device_id == device_ptr)
        {
            *pending = FBE_TRUE;
            break;
        }
        q_element = fbe_queue_next(&self->logout_queue_head, q_element);
    }
    fbe_spinlock_unlock(&self->logout_queue_lock);
    return FBE_STATUS_OK;
}

fbe_status_t port_is_logout_queue_empty(terminator_port_t * self, fbe_bool_t *is_empty)
{
    *is_empty = fbe_queue_is_empty(&self->logout_queue_head);
    return FBE_STATUS_OK;
}

fbe_status_t port_get_sas_port_attributes(terminator_port_t * self, fbe_terminator_port_shim_backend_port_info_t * port_info)
{
    port_info->port_type = port_get_type(self);
    switch(port_info->port_type)
    {
    case FBE_PORT_TYPE_SAS_LSI:
    case FBE_PORT_TYPE_SAS_PMC:
        port_info->backend_port_number = sas_port_get_backend_number(self);
        port_info->port_number = sas_port_get_io_port_number(self);
        port_info->portal_number = sas_port_get_portal_number(self);
        //HACK--- Hardcoding values for now.
        port_info->port_role = FBE_CPD_SHIM_PORT_ROLE_BE;
        port_info->connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;

        break;
    default:
        port_info->backend_port_number = -1;
        port_info->port_number = -1;
        port_info->portal_number = -1;
        break;
    }
    return FBE_STATUS_OK;
}


void port_lock_update_lock(terminator_port_t * self)
{
    fbe_mutex_lock(&self->update_mutex);
	//fbe_spinlock_lock(&self->update_lock);

}
void port_unlock_update_lock(terminator_port_t * self)
{
    fbe_mutex_unlock(&self->update_mutex);
	//fbe_spinlock_unlock(&self->update_lock);

}

fbe_bool_t port_is_matching(terminator_port_t * self, fbe_u32_t io_port_number, fbe_u32_t io_portal_number)
{
   fbe_port_type_t port_type = port_get_type(self);  
     
   if((port_type == FBE_PORT_TYPE_FC_PMC) || (port_type == FBE_PORT_TYPE_ISCSI) || (port_type == FBE_PORT_TYPE_FCOE)){ /* TEMP HACK for iScsi*/
     return ((fc_port_get_io_port_number(self)==io_port_number)&&(fc_port_get_portal_number(self)==io_portal_number));
   }else{
    return ((sas_port_get_io_port_number(self)==io_port_number)&&(sas_port_get_portal_number(self)==io_portal_number));
   }
}

fbe_bool_t port_get_reset_begin(terminator_port_t * self)
{
    return self->reset_begin;
}
fbe_bool_t port_get_reset_completed(terminator_port_t * self)
{
    return self->reset_completed;
}

void port_set_reset_begin(terminator_port_t * self, fbe_bool_t state)
{
    self->reset_begin = state;
    return;
}
void port_set_reset_completed(terminator_port_t * self, fbe_bool_t state)
{
    self->reset_completed = state;
    return;
}

fbe_status_t port_activate(terminator_port_t * self)
{
    /* Start port mini port api thread and port io thread*/
    fbe_status_t status;
    fbe_u32_t port_number;
    fbe_port_type_t  port_type;

    port_type = port_get_type(self);

    switch(port_type)
    {
        case FBE_PORT_TYPE_SAS_PMC:
            port_number = sas_port_get_backend_number(self);
        break;
        case FBE_PORT_TYPE_ISCSI: /* TODO: Implement later*/
        case FBE_PORT_TYPE_FC_PMC:
        case FBE_PORT_TYPE_FCOE:
            port_number = fc_port_get_backend_number(self);
        break;
        default:
            return FBE_STATUS_OK;
        break;    
    }   
    
    status = port_set_miniport_port_index(self, port_number);
    /* Start the terminator miniport api threads associated with this port*/
    status =  fbe_terminator_miniport_api_port_inserted(port_number);
    return status;
}

fbe_status_t port_set_miniport_port_index(terminator_port_t * self, fbe_u32_t port_index)
{
    if (self != NULL)
    {
        self->miniport_port_index = port_index;
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
fbe_status_t port_get_miniport_port_index(terminator_port_t * self, fbe_u32_t *port_index)
{
    if (self != NULL)
    {
        * port_index = self->miniport_port_index;
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/**************************************************************************
 *  terminator_port_initialize_protocol_specific_data()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function initialize protocol specific data for port.
 *
 *  PARAMETERS:
 *    terminator_port_t * port_handle 
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t terminator_port_initialize_protocol_specific_data(terminator_port_t * port_handle)
{
    fbe_port_type_t port_type = port_get_type(port_handle);
    terminator_port_info_t *attributes = NULL;
    void *type_specific_info = NULL;

    attributes = (terminator_port_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_port_info_t));
    if (attributes == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for port info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(attributes, sizeof(terminator_port_info_t));

    /*
     * TODO: Complete setting default values.
     */
    attributes->connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;
    attributes->port_role = FBE_CPD_SHIM_PORT_ROLE_BE;

    attributes->hdw_info.vendor = 0x11F8;
    attributes->hdw_info.device = 0x8001;
    
    attributes->sfp_media_interface_info.condition_type = FBE_CPD_SHIM_SFP_CONDITION_INSERTED;

    switch(port_type)
    {
    case FBE_PORT_TYPE_SAS_PMC:
        if((type_specific_info = allocate_sas_port_info()) == NULL)
        {
            fbe_terminator_free_memory(attributes);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_copy_memory(&(attributes->type_specific_info.sas_info), type_specific_info, sizeof(terminator_sas_port_info_t));
        fbe_terminator_free_memory(type_specific_info);
        break;
    case FBE_PORT_TYPE_ISCSI: /* TODO: Implement later*/
    case FBE_PORT_TYPE_FC_PMC:
    case FBE_PORT_TYPE_FCOE:
        if((type_specific_info = allocate_fc_port_info()) == NULL)
        {
            fbe_terminator_free_memory(attributes);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_copy_memory(&(attributes->type_specific_info.fc_info), type_specific_info, sizeof(terminator_fc_port_info_t));
        fbe_terminator_free_memory(type_specific_info);
        break;       
    default:
        /* ports that are not supported now */
        fbe_terminator_free_memory(attributes);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    base_component_assign_attributes(&port_handle->base, attributes);
    return FBE_STATUS_OK;
}
