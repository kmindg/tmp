/**************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/
/***************************************************************************
 *  fbe_terminator_fc_port.c
 ***************************************************************************
 *
 *  Description
 *      terminator FC port class
 *
 *
 *  History:
 *      09/23/09    Dipak Patel    Created
 *
 ***************************************************************************/
#include "fbe/fbe_memory.h"
#include "terminator_port.h"
#include "terminator_fc_port.h"
#include "terminator_fc_enclosure.h"
#include "terminator_login_data.h"
#include "terminator_drive.h"
#include "fbe_diplex.h"


/**************************************************************************
 *  fc_port_info_new()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function is the constructor of fc_port_info with defined data.
 *
 *  PARAMETERS:
 *      fbe_terminator_fc_port_info_t *user_data
 *
 *  RETURN VALUES/ERRORS:
 *      Pointer to terminator_fc_port_info_t stucture
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
terminator_fc_port_info_t * fc_port_info_new(fbe_terminator_fc_port_info_t *user_data)
{
    terminator_fc_port_info_t *new_info = NULL;

    new_info = (terminator_fc_port_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_fc_port_info_t));
    if(new_info == NULL)
    {
        return NULL;
    }
    new_info->diplex_address = user_data->diplex_address;
    new_info->io_port_number = user_data->io_port_number;
    new_info->portal_number = user_data->portal_number;
    new_info->backend_number = user_data->backend_number;
    return new_info;
}
/**************************************************************************
 *  allocate_fc_port_info()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function allocates memory of fc_port_info with defaults.
 *
 *  PARAMETERS:
 *
 *  RETURN VALUES/ERRORS:
 *      Pointer to terminator_fc_port_info_t stucture
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
terminator_fc_port_info_t * allocate_fc_port_info()
{
    terminator_fc_port_info_t *new_info = NULL;

    new_info = (terminator_fc_port_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_fc_port_info_t));
    if(new_info == NULL)
    {
        return NULL;
    }
    new_info->diplex_address = FBE_DIPLEX_ADDRESS_INVALID;
    new_info->io_port_number = -1; /* what values should be used instead of -1 ??? */
    new_info->portal_number = -1;
    new_info->backend_number = -1;
    return new_info;
}
/**************************************************************************
 *  fc_port_get_address()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets address for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_diplex_address_t - diplex_address
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_diplex_address_t fc_port_get_address(terminator_port_t * self)
{
    terminator_fc_port_info_t *info = NULL;
    if (self == NULL)
    {
        return FBE_DIPLEX_ADDRESS_INVALID;
    }
    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_DIPLEX_ADDRESS_INVALID;
    }
    return (info->diplex_address);
}
/**************************************************************************
 *  fc_port_set_address()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets address for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *      fbe_diplex_address_t diplex_address
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_port_set_address(terminator_port_t * self, fbe_diplex_address_t diplex_address)
{
    terminator_fc_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->diplex_address = diplex_address;
    }
    return;
}
/**************************************************************************
 *  fc_port_get_backend_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets backend number for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t - backend_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_port_get_backend_number(terminator_port_t * self)
{
    terminator_fc_port_info_t *info = NULL;
    if (self == NULL)
    {
        return -1;
    }
    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return -1;
    }
    return (info->backend_number);
}
/**************************************************************************
 *  fc_port_set_backend_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets backend number for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *      fbe_u32_t port_number
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_port_set_backend_number(terminator_port_t * self, fbe_u32_t port_number)
{
    terminator_fc_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->backend_number = port_number;
    }
    return;
}
/**************************************************************************
 *  fc_port_get_io_port_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets io port number for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t - io_port_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_port_get_io_port_number(terminator_port_t * self)
{
    terminator_fc_port_info_t *info = NULL;
    if (self == NULL)
    {
        return -1;
    }
    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return -1;
    }
    return (info->io_port_number);
}
/**************************************************************************
 *  fc_port_get_portal_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets portal number for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t - portal_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_port_get_portal_number(terminator_port_t * self)
{
    terminator_fc_port_info_t *info = NULL;
    if (self == NULL)
    {
        return -1;
    }

    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return -1;
    }
    return (info->portal_number);
}
/**************************************************************************
 *  fc_port_set_io_port_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets io port number for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *      fbe_u32_t io_port_number
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void  fc_port_set_io_port_number(terminator_port_t * self, fbe_u32_t io_port_number)
{
    terminator_fc_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->io_port_number = io_port_number;
    }
    return;
}
/**************************************************************************
 *  fc_port_set_portal_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets portal number for fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *      fbe_u32_t portal_number
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_port_set_portal_number(terminator_port_t * self, fbe_u32_t portal_number)
{
    terminator_fc_port_info_t *info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = (terminator_fc_port_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
    {
        info->portal_number = portal_number;
    }
    return;
}
/**************************************************************************
 *  port_add_fc_enclosure()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function adds fc enclosure on given port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *      terminator_enclosure_t * new_enclosure
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t port_add_fc_enclosure(terminator_port_t * self, terminator_enclosure_t * new_enclosure)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * enclosure_base = NULL;
    terminator_enclosure_t * last_enclosure = NULL;
    enclosure_base = &new_enclosure->base;


    port_lock_update_lock(self) ;

    if (base_component_get_first_child(&self->base) == NULL)
    {
        status = base_component_add_child(&self->base, base_component_get_queue_element(enclosure_base));
    }
    else 
    {
        last_enclosure = port_get_last_enclosure(self);
        if (last_enclosure == NULL)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            port_unlock_update_lock(self) ;
            return status;
        }
        status = base_component_add_child(&last_enclosure->base, base_component_get_queue_element(enclosure_base));
    }

    status = FBE_STATUS_OK;
    port_unlock_update_lock(self) ;
    return status;
}
/**************************************************************************
 *  fc_port_process_io_block()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function io came on fc port
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_port_process_io_block(terminator_port_t * self)
{
    fbe_terminator_io_t * terminator_io = NULL;

    /* lock tree
     * Note that we are going to dequeue from the port's io queue and from the device io
     * queue under the protection of this lock.
     * The enqueue to both these queues also occurs under protection of the port lock.
     */
    port_lock_update_lock(self);
    
    base_component_lock_io_queue((base_component_t*)self);
    if (base_component_io_queue_is_empty((base_component_t*)self)!= FBE_TRUE)
    {
        /* pop an IO block from port io queue */
        terminator_io = (fbe_terminator_io_t *) base_component_io_queue_pop((base_component_t*)self);
    }
    base_component_unlock_io_queue((base_component_t*)self);
    /* unlock the tree */
    port_unlock_update_lock(self);

    if (terminator_io == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s: did not find a io block!, port 0x%X\n", __FUNCTION__, self->miniport_port_index);
        return;
    }   

    /* Process IO*/
    fc_port_call_io_api(self, terminator_io);
    
    /* 6)call back to complete io using fbe_terminator_miniport_api_complete_io() */
    fbe_terminator_miniport_api_complete_io (self->miniport_port_index, terminator_io);
}
/**************************************************************************
 *  port_get_fc_port_attributes()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets FC port attributes
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *      fbe_terminator_port_shim_backend_port_info_t * port_info
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t port_get_fc_port_attributes(terminator_port_t * self, fbe_terminator_port_shim_backend_port_info_t * port_info)
{
    port_info->port_type = port_get_type(self);
    switch(port_info->port_type)
    {
    case FBE_PORT_TYPE_ISCSI:/* TEMP HACK*** TODO: Implement later for iScsi*/
        port_info->backend_port_number = fc_port_get_backend_number(self);
        port_info->port_number = fc_port_get_io_port_number(self);
        port_info->portal_number = fc_port_get_portal_number(self);
        //HACK--- Hardcoding values for now.
        port_info->port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
        port_info->connect_class = FBE_CPD_SHIM_CONNECT_CLASS_ISCSI;
        break;

    case FBE_PORT_TYPE_FCOE:
        port_info->backend_port_number = fc_port_get_backend_number(self);
        port_info->port_number = fc_port_get_io_port_number(self);
        port_info->portal_number = fc_port_get_portal_number(self);
        //HACK--- Hardcoding values for now.
        port_info->port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
        port_info->connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FCOE;
        break;

    case FBE_PORT_TYPE_FC_PMC:
        port_info->backend_port_number = fc_port_get_backend_number(self);
        port_info->port_number = fc_port_get_io_port_number(self);
        port_info->portal_number = fc_port_get_portal_number(self);
        //HACK--- Hardcoding values for now.
        port_info->port_role = FBE_CPD_SHIM_PORT_ROLE_FE;
        port_info->connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FC;
        break;
    default:
        port_info->backend_port_number = -1;
        port_info->port_number = -1;
        port_info->portal_number = -1;
        break;
    }
    return FBE_STATUS_OK;
}
/**************************************************************************
 *  fc_port_call_io_api()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function calls diplex routines according to diplex commands
 *    currently commmented 
 *
 *  PARAMETERS:
 *      terminator_port_t * self
 *      fbe_terminator_io_t * terminator_io
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_port_call_io_api(terminator_port_t * self, fbe_terminator_io_t * terminator_io)
{
    /*fbe_payload_diplex_operation_t * payload_diplex_operation = NULL;
    fbe_u8_t  diplex_opcode;
    fbe_status_t status;

    if(terminator_io->payload == NULL) {       
        return FBE_STATUS_GENERIC_FAILURE;
     }
    
     payload_diplex_operation = fbe_payload_get_diplex_operation(terminator_io->payload);
    
    if (payload_diplex_operation == NULL) {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: Error ! fbe_payload_get_diplex_operation failed \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    diplex_opcode = payload_diplex_operation->diplex_buffer[0];
    
    switch(diplex_opcode)
    {
        case FBE_DIPLEX_OPCODE_LCC_POLL:
                status = terminator_diplex_poll_handler(self, terminator_io->payload);            
        break;
        case FBE_DIPLEX_OPCODE_LCC_READ:
                status = terminator_diplex_read_handler(self, terminator_io->payload);
        break;
        case FBE_DIPLEX_OPCODE_LCC_WRITE:
                status = terminator_diplex_write_handler(self, terminator_io->payload);
        break; 
        default:
                 return FBE_STATUS_GENERIC_FAILURE;
        break;   
     }*/
       //return status;
       return FBE_STATUS_OK;
}


