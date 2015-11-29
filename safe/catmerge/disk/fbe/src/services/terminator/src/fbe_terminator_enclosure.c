/**************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

#include "terminator_enclosure.h"
#include "terminator_fc_enclosure.h"
#include "terminator_login_data.h"
#include "terminator_virtual_phy.h"
#include "terminator_drive.h"
#include "terminator_sas_io_api.h"
#include "terminator_port.h"
#include "fbe_terminator_common.h"
#include "fbe_terminator_device_registry.h"

static fbe_status_t enclosure_fill_in_login_data(terminator_enclosure_t * self);

/* Constructor for enclosure */
terminator_enclosure_t * enclosure_new(void)
{
    terminator_enclosure_t * new_enclosure = NULL;

    new_enclosure = (terminator_enclosure_t *)fbe_terminator_allocate_memory(sizeof(terminator_enclosure_t));
    if (new_enclosure == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for enclosure at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return NULL;
    }

    base_component_init(&new_enclosure->base);
    base_component_set_component_type(&new_enclosure->base, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
    base_component_set_reset_function(&new_enclosure->base, terminator_miniport_device_reset_common_action);

    new_enclosure->encl_protocol = FBE_ENCLOSURE_TYPE_INVALID;

    return new_enclosure;
}

fbe_status_t enclosure_destroy(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t *info = NULL;
    info = (terminator_sas_enclosure_info_t *)base_component_get_attributes(&self->base);
    if (info != NULL)
        fbe_terminator_free_memory (info);
    fbe_terminator_free_memory (self);
    return FBE_STATUS_OK;
}

/* this is the constructor of sas_enclosure_info */
terminator_sas_enclosure_info_t * sas_enclosure_info_new(fbe_terminator_sas_encl_info_t *sas_encl_info)
{
    fbe_cpd_shim_callback_login_t   *login_data = NULL;
    terminator_sas_enclosure_info_t *new_info   = 
        (terminator_sas_enclosure_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sas_enclosure_info_t));
    if (new_info == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for sas enclosure info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return NULL;
    }

    new_info->enclosure_type = sas_encl_info->encl_type;

    login_data = sas_enclosure_info_get_login_data(new_info);
    cpd_shim_callback_login_init(login_data);
    cpd_shim_callback_login_set_device_address(login_data, sas_encl_info->sas_address);
    cpd_shim_callback_login_set_device_type(login_data, FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE);

    fbe_copy_memory(new_info->uid, sas_encl_info->uid, sizeof(new_info->uid));

    new_info->encl_number = sas_encl_info->encl_number;
    new_info->backend     = sas_encl_info->backend_number;

    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;
    new_info->logical_parent_type             = TERMINATOR_COMPONENT_TYPE_INVALID;
    new_info->connector_id                    = sas_encl_info->connector_id;

    return new_info;
}

/* this function constructs sas_enclosure_info in assumption that some of initialization would be done later */
terminator_sas_enclosure_info_t * allocate_sas_enclosure_info()
{
    fbe_cpd_shim_callback_login_t   *login_data = NULL;
    terminator_sas_enclosure_info_t *new_info   = 
        (terminator_sas_enclosure_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sas_enclosure_info_t));
    if (new_info == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for sas enclosure info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return NULL;
    }

    new_info->enclosure_type = FBE_SAS_ENCLOSURE_TYPE_INVALID;

    login_data = sas_enclosure_info_get_login_data(new_info);
    cpd_shim_callback_login_init(login_data);
    cpd_shim_callback_login_set_device_type(login_data, FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE);

    fbe_zero_memory(&new_info->uid, FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE * sizeof(fbe_u8_t));

    new_info->encl_number = FBE_ENCLOSURE_NUMBER_INVALID;
    new_info->backend     = FBE_BACKEND_INVALID;

    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;
    new_info->logical_parent_type             = TERMINATOR_COMPONENT_TYPE_INVALID;
    new_info->connector_id                    = FBE_ENCLOSURE_VALUE_INVALID;

    return new_info;
}

fbe_status_t sas_enclosure_set_enclosure_type(terminator_enclosure_t * self, fbe_sas_enclosure_type_t type)
{
    terminator_sas_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->enclosure_type = type;
    return FBE_STATUS_OK;
}
fbe_sas_enclosure_type_t sas_enclosure_get_enclosure_type(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_SAS_ENCLOSURE_TYPE_INVALID;
    }
    return (info->enclosure_type);
}

/*********************************************************************
*          sas_enclosure_set_logical_parent_type ()
*********************************************************************
*
*  Description:
*   Logical parent is the parent in the tree hierarchy, in contrast with the physical parent the enclosure connects to.
*   e.g. for the nth (n > 1) enclosure in the main enclosure chain, its logical parent is the port and its physical parent
*   is the previous enclosure.
*
*  Inputs:
*   self: enclosure pointer
*   type: logical parent component type
*
*  Return Value:
*   FBE_STATUS_OK if successfully set or FBE_STATUS_GENERIC_FAILURE otherwise
*
*********************************************************************/
fbe_status_t sas_enclosure_set_logical_parent_type(terminator_enclosure_t * self, terminator_component_type_t type)
{
    terminator_sas_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->logical_parent_type = type;
    return FBE_STATUS_OK;
}

/*********************************************************************
*          sas_enclosure_get_logical_parent_type ()
*********************************************************************
*
*  Description:
*   Logical parent is the parent in the tree hierarchy, in contrast with the physical parent the enclosure connects to.
*   e.g. for the nth (n > 1) enclosure in the main enclosure chain, its logical parent is the port and its physical parent
*   is the previous enclosure.
*
*  Inputs:
*   self: enclosure pointer
*
*  Return Value:
*   enclosure type
*
*********************************************************************/
terminator_component_type_t sas_enclosure_get_logical_parent_type(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_SAS_ENCLOSURE_TYPE_INVALID;
    }
    return (info->logical_parent_type);
}

fbe_status_t sas_enclosure_set_enclosure_uid(terminator_enclosure_t * self, fbe_u8_t *uid)
{
    terminator_sas_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(&info->uid, FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE);
    fbe_copy_memory(&info->uid, uid, sizeof(info->uid));
    return FBE_STATUS_OK;
}

fbe_u8_t *sas_enclosure_get_enclosure_uid(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return (info->uid);
}

void sas_enclosure_info_set_login_data(terminator_sas_enclosure_info_t * self, fbe_cpd_shim_callback_login_t *login_data)
{
    fbe_move_memory(&self->login_data, login_data, sizeof(fbe_cpd_shim_callback_login_t));
}

fbe_cpd_shim_callback_login_t * sas_enclosure_info_get_login_data(terminator_sas_enclosure_info_t * self)
{
    return (&self->login_data);
}

terminator_enclosure_t * enclosure_get_last_enclosure(base_component_t * self)
{
    base_component_t * last_enclosure = NULL;
    base_component_t * child = NULL;

    last_enclosure = child = self;

    do
    {
        child = base_component_get_child_by_type(child, TERMINATOR_COMPONENT_TYPE_ENCLOSURE);
        if (child != NULL)
        {
            last_enclosure = child;
        }
    }while(child != NULL);
    return (terminator_enclosure_t *)last_enclosure;
}

terminator_enclosure_t * enclosure_get_last_sibling_enclosure(base_component_t * self)
{
    base_component_t * last_enclosure = NULL;
    base_component_t * child = NULL;
    terminator_sas_enclosure_info_t *attributes = NULL;

    last_enclosure = self;
    child = last_enclosure;

    while(child != NULL)
    {
        child = (base_component_t *)fbe_queue_front(&last_enclosure->child_list_head);
        while (child != NULL)
        {
            if (child->component_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
            {
                attributes = base_component_get_attributes(child);
                if(attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_PORT)
                {
                    last_enclosure = child;
                    break;
                }
            }
            child = (base_component_t *)fbe_queue_next(&last_enclosure->child_list_head, &child->queue_element);
        }
    }

    return (terminator_enclosure_t *)last_enclosure;
}

fbe_status_t sas_enclosure_get_connector_id_by_enclosure(terminator_enclosure_t * self, terminator_enclosure_t * connected_encl, fbe_u32_t *connector_id)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
	base_component_t *self_parent, *connected_parent;
	terminator_sas_enclosure_info_t *child_attributes, *parent_attributes;
    fbe_u8_t      max_conn_id_count = 0;


	base_component_get_parent(&self->base, &self_parent);
	base_component_get_parent(&connected_encl->base, &connected_parent);

	/* upstream enclosure of the connected_encl */
	if(connected_parent == &self->base)
	{
		*connector_id = CONN_IS_UPSTREAM;
		status = FBE_STATUS_OK;
	}
	else if(self_parent == &connected_encl->base)
	{
		switch(sas_enclosure_get_logical_parent_type(self))
		{
		/* downstream enclosure of the connected_encl */
		case TERMINATOR_COMPONENT_TYPE_PORT:
			*connector_id = CONN_IS_DOWNSTREAM;
			status = FBE_STATUS_OK;
			break;
		/* EE expander */
		case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
			child_attributes = base_component_get_attributes(&self->base);
		    parent_attributes = base_component_get_attributes(&connected_encl->base);
            status = terminator_max_conn_id_count(parent_attributes->enclosure_type, &max_conn_id_count);
            if(status != FBE_STATUS_OK)
            {
                return status;
            }

			if(child_attributes->connector_id < max_conn_id_count)
			{
				*connector_id = child_attributes->connector_id;
				status = FBE_STATUS_OK;
			}
			break;
		default:
			break;
		}
	}

	return status;
}

fbe_status_t enclosure_enumerate_devices(terminator_enclosure_t * self, fbe_terminator_device_ptr_t device_list[], fbe_u32_t *total_devices)
{
    fbe_status_t status = FBE_STATUS_OK ;
    base_component_t * self_base = &self->base;
    base_component_t * child = NULL;


    if ((self == NULL)||(device_list == NULL)||(total_devices == NULL))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    child = base_component_get_first_child(self_base);
    while (child != NULL)
    {
        switch(child->component_type){
        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
            /* add this enclosure to the list and recursively get the children of this enclosure */
            device_list[*total_devices] = child;
            *total_devices = *total_devices + 1;
            status = enclosure_enumerate_devices((terminator_enclosure_t *)child, device_list, total_devices);
            break;
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
        case TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY:
            device_list[*total_devices] = child;
            ++(*total_devices);
            break;
        default:
            /* Fill with some unlikely number so we know something is wrong */
            device_list[*total_devices] = NULL;
        }
        child = base_component_get_next_child(self_base, child);
    }
    return status;
}

fbe_status_t sas_enclosure_generate_virtual_phy(terminator_enclosure_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_virtual_phy_t * new_v_phy = NULL;
    terminator_sas_virtual_phy_info_t * attributes = NULL;
    fbe_sas_address_t v_phy_sas_address = FBE_SAS_ADDRESS_INVALID;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    base_component_t * v_phy_base = NULL;

    terminator_sas_enclosure_info_t *enclosure_attributes = NULL;
    fbe_cpd_shim_callback_login_t * enclosure_login_data = NULL;
    fbe_miniport_device_id_t parent_device_id = CPD_DEVICE_ID_INVALID;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_sas_enclosure_type_t encl_type;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_u8_t            phy_number = 0;
    fbe_miniport_device_id_t cpd_device_id;

    new_v_phy = virtual_phy_new();
    v_phy_base = (base_component_t *)new_v_phy;
    v_phy_sas_address = sas_enclosure_calculate_virtual_phy_sas_address(self);
    encl_type = sas_enclosure_get_enclosure_type(self);
//    terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sas_enclosure_get_enclosure_type returned %u\n", __FUNCTION__, encl_type);
    attributes = sas_virtual_phy_info_new(encl_type, v_phy_sas_address);
    if(attributes == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sas_virtual_phy_info_new() failed\n", __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_terminator_free_memory (new_v_phy);
        return(status);
    }
    base_component_assign_attributes(v_phy_base, attributes);

    login_data = sas_virtual_phy_info_get_login_data(attributes);

    enclosure_attributes = base_component_get_attributes(&self->base);
    enclosure_login_data = sas_enclosure_info_get_login_data(enclosure_attributes);
    parent_device_id = cpd_shim_callback_login_get_device_id(enclosure_login_data);
    parent_device_address = cpd_shim_callback_login_get_device_address(enclosure_login_data);
    enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(enclosure_login_data)+1;
    status = sas_enclosure_generate_sas_virtual_phy_phy_number(&phy_number, self);
    if (status != FBE_STATUS_OK){
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sas_enclosure_generate_sas_virtual_phy_phy_number() failed\n", __FUNCTION__);

        fbe_terminator_free_memory (new_v_phy);
        return(status);
    }

    cpd_shim_callback_login_set_parent_device_id(login_data, parent_device_id);
    cpd_shim_callback_login_set_parent_device_address(login_data, parent_device_address);
    cpd_shim_callback_login_set_enclosure_chain_depth(login_data, enclosure_chain_depth);
    cpd_shim_callback_login_set_phy_number(login_data, phy_number);

    status = base_component_add_child_to_front (&self->base, base_component_get_queue_element(v_phy_base));
    /* Use the new device id format */
    terminator_add_device_to_miniport_sas_device_table(new_v_phy);
    terminator_get_cpd_device_id_from_miniport(new_v_phy, &cpd_device_id);
    cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);
    return status;
}

/* the sas address of the virtual phy depends on the enclosure type
 * Bullet encl, decrement 1 from the enclosure SAS address
 * Viper encl, the last 6 bits is 0x3E.
 */
fbe_sas_address_t sas_enclosure_calculate_virtual_phy_sas_address(terminator_enclosure_t * self)
{
    fbe_sas_address_t enclosure_sas_address;
    fbe_sas_address_t v_phy_sas_address;

    enclosure_sas_address = sas_enclosure_get_sas_address(self);
    if (enclosure_sas_address == FBE_SAS_ADDRESS_INVALID)
        return FBE_SAS_ADDRESS_INVALID;

    switch(sas_enclosure_get_enclosure_type(self)){
        case FBE_SAS_ENCLOSURE_TYPE_BULLET:
            v_phy_sas_address = enclosure_sas_address - 1;
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
        case FBE_SAS_ENCLOSURE_TYPE_RHEA:
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
            v_phy_sas_address = (enclosure_sas_address & VIPER_VIRTUAL_PHY_SAS_ADDR_MASK)
                                 + VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS;
            break;

        default:
            // not sure if this is the best return value here.
            return FBE_SAS_ADDRESS_INVALID;
            break;
    }
    return v_phy_sas_address;
}

fbe_sas_address_t sas_enclosure_get_sas_address(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t * info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_sas_address_t address = FBE_SAS_ADDRESS_INVALID;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return address;
    }
    login_data = sas_enclosure_info_get_login_data(info);
    address = cpd_shim_callback_login_get_device_address(login_data);
    return address;
}

fbe_status_t sas_enclosure_set_backend_number(terminator_enclosure_t * self, fbe_u32_t backend_number)
{
    terminator_sas_enclosure_info_t * info = NULL;
    if(self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if((info = base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->backend = backend_number;
    return FBE_STATUS_OK;
}

fbe_status_t sas_enclosure_set_enclosure_number(terminator_enclosure_t * self, fbe_u32_t enclosure_number)
{
    terminator_sas_enclosure_info_t * info = NULL;
    if(self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if((info = base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->encl_number = enclosure_number;
    return FBE_STATUS_OK;
}

fbe_u32_t sas_enclosure_get_enclosure_number(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t * info = NULL;
    if(self == NULL)
    {
        return -1;
    }
    if((info = base_component_get_attributes(&self->base)) == NULL)
    {
        return -1;
    }
    return (fbe_u32_t)(info->encl_number);
}

fbe_u32_t sas_enclosure_get_connector_id(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t * info = NULL;
    if(self == NULL)
    {
        return -1;
    }
    if((info = base_component_get_attributes(&self->base)) == NULL)
    {
        return -1;
    }
    return info->connector_id;
}

fbe_status_t sas_enclosure_set_connector_id(terminator_enclosure_t * self, fbe_u32_t connector_id)
{
    terminator_sas_enclosure_info_t * info = NULL;
    if(self == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if((info = base_component_get_attributes(&self->base)) == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->connector_id = connector_id;
    return FBE_STATUS_OK;
}

fbe_u32_t sas_enclosure_get_backend_number(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t * info = NULL;
    if(self == NULL)
    {
        return -1;
    }
    if((info = base_component_get_attributes(&self->base)) == NULL)
    {
        return -1;
    }
    return info->backend;
}

fbe_status_t sas_enclosure_insert_sata_drive(terminator_enclosure_t * self,
                                            fbe_u32_t slot_number,
                                            fbe_terminator_sata_drive_info_t *drive_info,
                                            fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_drive_t * new_drive = NULL;
    terminator_sata_drive_info_t * attributes = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    base_component_t * drive_base = NULL;

    terminator_sas_enclosure_info_t *enclosure_attributes = NULL;
    fbe_cpd_shim_callback_login_t * enclosure_login_data = NULL;
    fbe_miniport_device_id_t parent_device_id = CPD_DEVICE_ID_INVALID;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_u8_t            phy_number = 0;
    fbe_miniport_device_id_t cpd_device_id;

    new_drive = drive_new();
    drive_set_protocol(new_drive, FBE_DRIVE_TYPE_SATA);
    drive_base = (base_component_t *)new_drive;

    attributes = sata_drive_info_new(drive_info);
    base_component_assign_attributes(drive_base, attributes);
    sata_drive_set_slot_number(new_drive, slot_number);
    sata_drive_set_drive_type(new_drive, drive_info->drive_type);
    sata_drive_info_set_serial_number(attributes, drive_info->drive_serial_number);
    sata_drive_info_set_product_id(attributes, drive_info->product_id);

    login_data = sata_drive_info_get_login_data(attributes);
    *drive_ptr = new_drive;

    enclosure_attributes = base_component_get_attributes(&self->base);
    enclosure_login_data = sas_enclosure_info_get_login_data(enclosure_attributes);
    parent_device_id = cpd_shim_callback_login_get_device_id(enclosure_login_data);
    parent_device_address = cpd_shim_callback_login_get_device_address(enclosure_login_data);
    enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(enclosure_login_data)+1;
    status = sas_enclosure_calculate_sas_device_phy_number(&phy_number, self, slot_number);
    if (status != FBE_STATUS_OK) {
        fbe_terminator_free_memory(new_drive);
        fbe_terminator_free_memory(drive_info);
        return status;
    }
    cpd_shim_callback_login_set_parent_device_id(login_data, parent_device_id);
    cpd_shim_callback_login_set_parent_device_address(login_data, parent_device_address);
    cpd_shim_callback_login_set_enclosure_chain_depth(login_data, enclosure_chain_depth);
    cpd_shim_callback_login_set_phy_number(login_data, phy_number);


    status = base_component_add_child_to_front (&self->base, base_component_get_queue_element(drive_base));
    /* Use the new device id format */
    terminator_add_device_to_miniport_sas_device_table(new_drive);
    terminator_get_cpd_device_id_from_miniport(new_drive, &cpd_device_id);
    cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);
    return status;
}

fbe_status_t sas_enclosure_generate_sas_virtual_phy_phy_number(fbe_u8_t *phy_number, terminator_enclosure_t * self)
{
    switch(sas_enclosure_get_enclosure_type(self))
    {
        case FBE_SAS_ENCLOSURE_TYPE_BULLET:
            /* Need to confirm this is true for bullet
             * The virtual PHY will always be 1 greater than the max physical PHY ID.
             * With a 24 port expander, PHYs 0-23 are physical and 24 is virtual.*/
            *phy_number = BULLET_MAX_PHYSICAL_PHY_ID + 1;
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
        case FBE_SAS_ENCLOSURE_TYPE_RHEA:
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
            /* The virtual PHY will always be 1 greater than the max physical PHY ID.
             * With a 24 port expander, PHYs 0-23 are physical and 24 is virtual.*/
            *phy_number = SAS_MAX_PHYSICAL_PHY_ID + 1;
            break;

        default:
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return FBE_STATUS_OK;
}

fbe_status_t sas_enclosure_calculate_sas_device_phy_number(fbe_u8_t *phy_number, terminator_enclosure_t * self, fbe_u32_t slot_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_enclosure_t * current_enclosure = self;
    // we need to slot number local to EE to calculate its phy number
    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t *)&current_enclosure, &slot_number);
    if (status != FBE_STATUS_OK){
        return(status);
    }

    switch(sas_enclosure_get_enclosure_type(self))
    {
        case FBE_SAS_ENCLOSURE_TYPE_BULLET:
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_VIPER);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MAGNUM:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_MAGNUM);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_BUNKER);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_CITADEL);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_DERRINGER);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE);
            break;	 	
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP);
            break;	 	
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP);
            break;	 	
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP);
            break;	 		 	
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_FALLBACK);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_BOXWOOD);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_KNOT);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_PINECONE);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_STEELJAW);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_RAMHORN);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_ANCHO);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_CALYPSO);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_RHEA:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_RHEA);
            break;
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_MIRANDA);
            break;
       case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
            status = terminator_get_drive_slot_to_phy_mapping((fbe_u8_t)slot_number, phy_number, FBE_SAS_ENCLOSURE_TYPE_TABASCO);
            break;
        default:
            break;
    }
    return status;
}

fbe_status_t sas_enclosure_is_slot_available(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_bool_t * result)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * child = NULL;

    /* Check if this slot is used by any other drive */
    child = base_component_get_first_child(&self->base);
    while (child != NULL)
    {
        switch (base_component_get_component_type(child))
        {
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
            if (sas_drive_get_slot_number((terminator_drive_t *)child)== slot_number)
            {
                status = FBE_STATUS_OK;
                *result = FBE_FALSE;
                return status;
            }
            break;
        default:
            break;
        }
        child = base_component_get_next_child(&self->base, child);
    }
    status = FBE_STATUS_OK;
    *result = FBE_TRUE;
    return status;
}

fbe_status_t sas_enclosure_is_connector_available(terminator_enclosure_t * self, fbe_u32_t connector_id, fbe_bool_t * result)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * child = NULL;
    fbe_sas_enclosure_type_t encl_type;
    fbe_u8_t max_conn_id_count = 0;

    *result = FBE_TRUE;

    encl_type = sas_enclosure_get_enclosure_type(self);
    status = terminator_max_conn_id_count(encl_type, &max_conn_id_count);
    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    /* Should be a valid connector id */
    if (connector_id >= max_conn_id_count)
    {
    	*result = FBE_FALSE;
    	status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    /* 0 for upstream enclosure */
    if(connector_id == 0)
    {
    	/* Currently connector 0 cannot be explicitly connected to */
    }
    else if(connector_id == 1)
    {
    	/* Currently there is no case to explicitly connect to a enclosure's connector 1 since all will be connected to the last enclosure in the chain */
    }
    else
    {
		/* expander */
		child = base_component_get_first_child(&self->base);
		while (child != NULL)
		{
			if(base_component_get_component_type(child) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
			{
				terminator_sas_enclosure_info_t *attributes = base_component_get_attributes(child);
				if(attributes->logical_parent_type == TERMINATOR_COMPONENT_TYPE_ENCLOSURE && attributes->connector_id == connector_id)
				{
					*result = FBE_FALSE;
					break;
				}
			}
			child = base_component_get_next_child(&self->base, child);
		}
    }
    return status;
}

fbe_status_t sas_enclosure_call_io_api(terminator_enclosure_t * self, fbe_terminator_io_t * terminator_io)
{
    /* TODO: should log a trace here */
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t sas_enclosure_get_device_ptr_by_slot(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_terminator_device_ptr_t *device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t local_slot = slot_number;

    base_component_t * child = NULL;
    *device_ptr = NULL;
    
    /* we need to find the owner of the slot */
    status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t *)&self, &local_slot);
    RETURN_ON_ERROR_STATUS;

    child = base_component_get_first_child(&self->base);
    while (child != NULL)
    {
        if(base_component_get_component_type(child)==TERMINATOR_COMPONENT_TYPE_DRIVE)
        {
            /* drive still reference by the slot_number for the enclosure */
            if (sas_drive_get_slot_number((terminator_drive_t *)child)== slot_number)
            {
                *device_ptr = child;
                status = FBE_STATUS_OK;
                return status;
            }
        }
        child = base_component_get_next_child(&self->base, child);
    }
    return status;
}
fbe_status_t sas_enclosure_set_sas_address(terminator_enclosure_t * self, fbe_sas_address_t sas_address)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_sas_enclosure_info_t * info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return status;
    }
    login_data = sas_enclosure_info_get_login_data(info);
    cpd_shim_callback_login_set_device_address(login_data, sas_address);
    status = FBE_STATUS_OK;
    return status;
}
fbe_status_t sas_enclosure_update_virtual_phy_sas_address(terminator_enclosure_t * self)
{
    fbe_sas_address_t v_phy_sas_address = FBE_SAS_ADDRESS_INVALID;
    base_component_t * matching_device = NULL;
    /* get the virtual phy of this enclosure */
    matching_device = base_component_get_child_by_type(&self->base, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    if (matching_device == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Calculate the virtual phy sas address */
    v_phy_sas_address = sas_enclosure_calculate_virtual_phy_sas_address(self);
    /* set the sas address */
    return sas_virtual_phy_set_sas_address((terminator_virtual_phy_t *)matching_device, v_phy_sas_address);
}

fbe_status_t sas_enclosure_insert_sas_drive(terminator_enclosure_t * self,
                                            fbe_u32_t slot_number,
                                            fbe_terminator_sas_drive_info_t *drive_info,
                                            fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_drive_t * new_drive = NULL;
    terminator_sas_drive_info_t * attributes = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    base_component_t * drive_base = NULL;

    terminator_sas_enclosure_info_t *enclosure_attributes = NULL;
    fbe_cpd_shim_callback_login_t * enclosure_login_data = NULL;
    fbe_miniport_device_id_t parent_device_id = CPD_DEVICE_ID_INVALID;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_u8_t            phy_number = 0;
    fbe_miniport_device_id_t cpd_device_id;

    new_drive = drive_new();
    drive_set_protocol(new_drive, FBE_DRIVE_TYPE_SAS);
    drive_base = (base_component_t *)new_drive;

    attributes = sas_drive_info_new(drive_info);
    base_component_assign_attributes(drive_base, attributes);
    sas_drive_set_slot_number(new_drive, slot_number);

    login_data = sas_drive_info_get_login_data(attributes);
    *drive_ptr = new_drive;

    enclosure_attributes = base_component_get_attributes(&self->base);
    enclosure_login_data = sas_enclosure_info_get_login_data(enclosure_attributes);
    parent_device_id = cpd_shim_callback_login_get_device_id(enclosure_login_data);
    parent_device_address = cpd_shim_callback_login_get_device_address(enclosure_login_data);
    enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(enclosure_login_data)+1;
    status = sas_enclosure_calculate_sas_device_phy_number(&phy_number, self, slot_number);
    if (status != FBE_STATUS_OK){
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: sas_enclosure_calculate_sas_device_phy_number() failed\n", __FUNCTION__);
        fbe_terminator_free_memory (drive_info);
        fbe_terminator_free_memory (new_drive);
        return(status);
    }
    cpd_shim_callback_login_set_parent_device_id(login_data, parent_device_id);
    cpd_shim_callback_login_set_parent_device_address(login_data, parent_device_address);
    cpd_shim_callback_login_set_enclosure_chain_depth(login_data, enclosure_chain_depth);
    cpd_shim_callback_login_set_phy_number(login_data, phy_number);


    status = base_component_add_child_to_front (&self->base, base_component_get_queue_element(drive_base));
    /* Use the new device id format */
    terminator_add_device_to_miniport_sas_device_table(new_drive);
    terminator_get_cpd_device_id_from_miniport(new_drive, &cpd_device_id);
    cpd_shim_callback_login_set_device_id(login_data, cpd_device_id);

    return status;
}


fbe_status_t enclosure_activate(terminator_enclosure_t * self)
{
    fbe_status_t status;
    fbe_u32_t port_index;
    /* Set the login_pending flag for this enclosure and it's virtual phy,
    so the miniport api thread will pick it up and log them in */
    status = terminator_set_device_login_pending (self);
    status = terminator_get_port_index(self, &port_index);
    status = fbe_terminator_miniport_api_device_state_change_notify(port_index);

    /* fill in login data */
    status = enclosure_fill_in_login_data(self);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: enclosure_fill_in_login_data failed!\n", __FUNCTION__);
        return status;
    }
    /* update drive and phy status */
    status = enclosure_update_drive_and_phy_status(self);
    if (status != FBE_STATUS_OK)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s: enclosure_update_drive_and_phy_status failed!\n", __FUNCTION__);
        return status;
    }
    /* mark as login pending */
    base_component_set_state(&self->base, TERMINATOR_COMPONENT_STATE_LOGIN_PENDING);

    return status;
}

static fbe_status_t enclosure_fill_in_login_data(terminator_enclosure_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * parent = NULL;
    terminator_sas_enclosure_info_t *attributes = NULL;
    terminator_sas_enclosure_info_t *self_attributes = NULL;
    fbe_miniport_device_id_t parent_device_id = CPD_DEVICE_ID_INVALID;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_u8_t            phy_number = 0;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_sas_enclosure_type_t encl_type;

    /* first get the parent device */
    status = base_component_get_parent(&self->base, &parent);
    if (parent == NULL)
    {
        return status;
    }
    switch(base_component_get_component_type(parent))
    {
    case TERMINATOR_COMPONENT_TYPE_PORT:
        parent_device_address = sas_port_get_address((terminator_port_t *)parent);
        parent_device_id = CPD_DEVICE_ID_INVALID;/*sas_port_get_device_id(self)*/
        enclosure_chain_depth = 0;
        break;
    case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
        self_attributes = base_component_get_attributes(&self->base);
        attributes = base_component_get_attributes(parent);
        login_data = sas_enclosure_info_get_login_data(attributes);
        parent_device_id = cpd_shim_callback_login_get_device_id(login_data);
        parent_device_address = cpd_shim_callback_login_get_device_address(login_data);
        enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(login_data) + 1;

        switch (self_attributes->logical_parent_type)
        {
             // connect to upstream enclosure
        case TERMINATOR_COMPONENT_TYPE_PORT:
            // We will pick the first lane (i.e. connector 0) in the expansion port list.
            encl_type = sas_enclosure_get_enclosure_type((terminator_enclosure_t * )parent);
            status = sas_virtual_phy_get_individual_conn_to_phy_mapping
                            (0,            // the first lane
                            CONN_IS_DOWNSTREAM, 
                            &phy_number, 
                            encl_type);
            break;
            // connect to ICM
        case TERMINATOR_COMPONENT_TYPE_ENCLOSURE:
            encl_type = sas_enclosure_get_enclosure_type((terminator_enclosure_t * )parent);
            status =  sas_virtual_phy_get_individual_conn_to_phy_mapping
                            (0,            // the first lane
                            (self_attributes->connector_id), 
                            &phy_number, 
                            encl_type);
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }

        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
        break;
    }
    attributes = base_component_get_attributes(&self->base);
    login_data = sas_enclosure_info_get_login_data(attributes);
    cpd_shim_callback_login_set_parent_device_id(login_data, parent_device_id);
    cpd_shim_callback_login_set_parent_device_address(login_data, parent_device_address);
    cpd_shim_callback_login_set_enclosure_chain_depth(login_data, enclosure_chain_depth);
    cpd_shim_callback_login_set_phy_number(login_data, phy_number);
    return status;
}
fbe_status_t enclosure_update_drive_and_phy_status(terminator_enclosure_t * self)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    ses_stat_elem_array_dev_slot_struct drive_slot_stat;
    ses_stat_elem_exp_phy_struct        exp_phy_stat;
    base_component_t                    *virtual_phy = NULL;
    fbe_u8_t                            phy_id;
    fbe_sas_enclosure_type_t            encl_type;
    base_component_t                    * child = NULL;
    fbe_u32_t                           slot_number;

    // Get the related virtual phy  of the enclosure
    virtual_phy = base_component_get_child_by_type(&self->base, TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY);
    if(virtual_phy == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* set the drive slot status to ok */
    fbe_zero_memory(&drive_slot_stat, sizeof(ses_stat_elem_array_dev_slot_struct));
    drive_slot_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    /* set the phy status to ok */
    fbe_zero_memory(&exp_phy_stat, sizeof(ses_stat_elem_exp_phy_struct));
    exp_phy_stat.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    exp_phy_stat.link_rdy = 0x1;
    exp_phy_stat.phy_rdy = 0x1;

    /* set status to OK.  Enclosure may not have any drive, when it is activated. */
    status = FBE_STATUS_OK;

    encl_type = sas_enclosure_get_enclosure_type(self);

    /* go thru the child list and get the slot number*/
    child = base_component_get_first_child(&self->base);
    while(child != NULL)
    {
        if (base_component_get_component_type(child) == TERMINATOR_COMPONENT_TYPE_DRIVE)
        {
            slot_number = sas_drive_get_slot_number((terminator_drive_t *)child);
            // I only expect slot_number updated here.
            status = terminator_update_drive_parent_device((fbe_terminator_device_ptr_t *)&self, &slot_number);  //GJF I think this is supposed to be child not self
            if (status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s terminator_update_drive_parent_device failed!\n", __FUNCTION__);
                return status;
            }
            /* Set the drive insert bit*/
            status = sas_virtual_phy_set_drive_eses_status((terminator_virtual_phy_t *)virtual_phy,
                slot_number, drive_slot_stat);
            if (status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set enclosure_drive_status failed!\n", __FUNCTION__);
                return status;
            }

            status = sas_virtual_phy_get_drive_slot_to_phy_mapping(slot_number, &phy_id, encl_type);
            if (status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s sas_virtual_phy_get_drive_slot_to_phy_mapping failed!\n", __FUNCTION__);
                return status;
            }
            /* Set phy status */
            status = sas_virtual_phy_set_phy_eses_status((terminator_virtual_phy_t *)virtual_phy, phy_id, exp_phy_stat);
            if (status != FBE_STATUS_OK)
            {
                terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s set enclosure_drive_status failed!\n", __FUNCTION__);
                return status;
            }
        }
        /* get next one */
        child = base_component_get_next_child(&self->base,  child);
    }
    return status;
}

fbe_status_t enclosure_add_sas_enclosure(terminator_enclosure_t * self, terminator_enclosure_t * new_enclosure, fbe_bool_t new_virtual_phy)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * enclosure_base = NULL;
    terminator_sas_enclosure_info_t *attributes = NULL;
    fbe_miniport_device_id_t parent_device_id = CPD_DEVICE_ID_INVALID;
    fbe_sas_address_t parent_device_address = FBE_SAS_ADDRESS_INVALID;
    fbe_u8_t            enclosure_chain_depth = 0;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_miniport_device_id_t cpd_device_id;
    fbe_terminator_device_ptr_t port_ptr;
    fbe_terminator_api_device_handle_t port_handle;
	fbe_u32_t port_number;

	if (self == NULL)
	{
		status = FBE_STATUS_GENERIC_FAILURE;
		return status;
	}

    enclosure_base = &new_enclosure->base;

    /* Lock the port configuration so no one else can change it while we modify configuration */
    attributes = base_component_get_attributes(&self->base);
    port_number = ((terminator_sas_enclosure_info_t *)attributes)->backend;
    fbe_terminator_api_get_port_handle(port_number, &port_handle);
    (void) fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    port_lock_update_lock(port_ptr);

	attributes = base_component_get_attributes(&self->base);
	login_data = sas_enclosure_info_get_login_data(attributes);
	parent_device_id = cpd_shim_callback_login_get_device_id(login_data);
	parent_device_address = cpd_shim_callback_login_get_device_address(login_data);
	enclosure_chain_depth = cpd_shim_callback_login_get_enclosure_chain_depth(login_data) + 1;
	/* Add this enclosure to the parent enclosure's child_list */
	status = base_component_add_child(&self->base, base_component_get_queue_element(enclosure_base));

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

    status = FBE_STATUS_OK;
    port_unlock_update_lock(port_ptr) ;
    return status;
}
/**************************************************************************
 *  enclosure_set_protocol()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets enclosure protocol.
 *
 *  PARAMETERS:
 *    terminator_enclosure_t *self
 *    fbe_enclosure_type_t   protocol.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t enclosure_set_protocol(terminator_enclosure_t * self, fbe_enclosure_type_t protocol)
{
    self->encl_protocol = protocol;
    return FBE_STATUS_OK;
}
/**************************************************************************
 *  enclosure_get_protocol()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets enclosure protocol.
 *
 *  PARAMETERS:
 *    terminator_enclosure_t *self
 *
 *  RETURN VALUES/ERRORS:
 *    fbe_enclosure_type_t encl_protocol.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_enclosure_type_t enclosure_get_protocol(terminator_enclosure_t * self)
{
    return self->encl_protocol;
}
/**************************************************************************
 *  terminator_enclosure_initialize_protocol_specific_data()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function initialize protocol specific data for enclosure.
 *
 *  PARAMETERS:
 *    terminator_enclosure_t * encl_handle.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t terminator_enclosure_initialize_protocol_specific_data(terminator_enclosure_t * encl_handle)
{
    void * attributes = NULL;

    fbe_enclosure_type_t encl_type = enclosure_get_protocol(encl_handle);

    switch(encl_type)
    {
    case FBE_ENCLOSURE_TYPE_SAS:
        if((attributes = allocate_sas_enclosure_info()) == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    case FBE_ENCLOSURE_TYPE_FIBRE:
        if((attributes = allocate_fc_enclosure_info()) == NULL)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        break;
    default:
        /* enclosures that are not supported now */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    base_component_assign_attributes(&encl_handle->base, attributes);
    return FBE_STATUS_OK;
}

/**************************************************************************
 *  sas_enclosure_update_child_enclosure_chain_depth()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    Set chain depth for all child enclosures of the self enclosure.
 *
 *  PARAMETERS:
 *    self: parent enclosure.
 *
 *  RETURN VALUES/ERRORS:
 *      success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      April-02-2010: Bo Gao created.
 **************************************************************************/
fbe_status_t sas_enclosure_update_child_enclosure_chain_depth(terminator_enclosure_t * self)
{
    fbe_status_t status = FBE_STATUS_OK;
    base_component_t * child = NULL;
    fbe_u8_t self_depth = sas_enclosure_get_enclosure_chain_depth(self);

    child = base_component_get_first_child(&self->base);
    while(child != NULL)
    {
        if(base_component_get_component_type(child) == TERMINATOR_COMPONENT_TYPE_ENCLOSURE)
        {
            status = sas_enclosure_set_enclosure_chain_depth((terminator_enclosure_t *)child, self_depth + 1);
            if(status != FBE_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* recursively set child depth */
            status = sas_enclosure_update_child_enclosure_chain_depth((terminator_enclosure_t *)child);
            if(status != FBE_STATUS_OK)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        child = base_component_get_next_child(&self->base, child);
    }

    return status;
}

/**************************************************************************
 *  sas_enclosure_get_enclosure_chain_depth()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    get the enclosure chain depth.
 *
 *  PARAMETERS:
 *    self: enclosure pointer.
 *
 *  RETURN VALUES/ERRORS:
 *    enclosure chain depth. -1 if query fails.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      April-02-2010: Bo Gao created.
 **************************************************************************/
fbe_u8_t sas_enclosure_get_enclosure_chain_depth(terminator_enclosure_t * self)
{
    terminator_sas_enclosure_info_t *enclosure_attributes;
    fbe_cpd_shim_callback_login_t *enclosure_login_data;

    enclosure_attributes = base_component_get_attributes(&self->base);
    enclosure_login_data = sas_enclosure_info_get_login_data(enclosure_attributes);
    return cpd_shim_callback_login_get_enclosure_chain_depth(enclosure_login_data);
}

/**************************************************************************
 *  sas_enclosure_set_enclosure_chain_depth()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    set the enclosure chain depth.
 *
 *  PARAMETERS:
 *    self: enclosure pointer.
 *    depth: enclosure chain depth
 *
 *  RETURN VALUES/ERRORS:
 *    success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      April-02-2010: Bo Gao created.
 **************************************************************************/
fbe_status_t sas_enclosure_set_enclosure_chain_depth(terminator_enclosure_t * self, fbe_u8_t depth)
{
    fbe_status_t status = FBE_STATUS_OK;
    terminator_sas_enclosure_info_t *enclosure_attributes;
    fbe_cpd_shim_callback_login_t *enclosure_login_data;

    enclosure_attributes = base_component_get_attributes(&self->base);
    enclosure_login_data = sas_enclosure_info_get_login_data(enclosure_attributes);
    cpd_shim_callback_login_set_enclosure_chain_depth(enclosure_login_data, depth);
    return status;
}

/**************************************************************************
 *  sas_enclosure_handle_get_enclosure_chain_depth()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    get the enclosure chain depth.
 *
 *  PARAMETERS:
 *    enclosure_handle: enclosure handle.
 *
 *  RETURN VALUES/ERRORS:
 *    enclosure chain depth. -1 if query fails.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      April-02-2010: Bo Gao created.
 **************************************************************************/
fbe_u8_t sas_enclosure_handle_get_enclosure_chain_depth(fbe_terminator_api_device_handle_t enclosure_handle)
{
    tdr_status_t status;
    fbe_terminator_device_ptr_t enclosure_ptr;

    status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if(status != TDR_STATUS_OK)
    {
        return -1;
    }
    return sas_enclosure_get_enclosure_chain_depth(enclosure_ptr);
}

/**************************************************************************
 *  sas_enclosure_handle_set_enclosure_chain_depth()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    set the enclosure chain depth.
 *
 *  PARAMETERS:
 *    enclosure_handle: enclosure handle.
 *    depth: enclosure chain depth
 *
 *  RETURN VALUES/ERRORS:
 *    success or failure.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      April-02-2010: Bo Gao created.
 **************************************************************************/
fbe_status_t sas_enclosure_handle_set_enclosure_chain_depth(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u8_t depth)
{
    fbe_status_t status = FBE_STATUS_OK;
    tdr_status_t tdr_status = TDR_STATUS_OK;
    fbe_terminator_device_ptr_t enclosure_ptr;

    tdr_status = fbe_terminator_device_registry_get_device_ptr(enclosure_handle, &enclosure_ptr);
    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = sas_enclosure_set_enclosure_chain_depth(enclosure_ptr, depth);
    return status;
}
