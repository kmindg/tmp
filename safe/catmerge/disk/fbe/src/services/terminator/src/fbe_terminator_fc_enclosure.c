/**************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/***************************************************************************
 *  fbe_terminator_fc_enclosure.c
 ***************************************************************************
 *
 *  Description
 *      terminator fc enclosure class
 *
 *
 *  History:
 *      09/23/08    Dipak Patel    Created
 *
 ***************************************************************************/
#include "terminator_enclosure.h"
#include "terminator_fc_enclosure.h"
#include "terminator_login_data.h"
#include "terminator_virtual_phy.h"
#include "terminator_drive.h"
#include "terminator_fc_drive.h"
#include "terminator_sas_io_api.h"
#include "terminator_port.h"
#include "fbe_terminator_common.h"

/**************************************************************************
 *  fc_enclosure_info_new()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function is the constructor of fc_enclosure_info with defined data.
 *
 *  PARAMETERS:
 *      fbe_terminator_fc_encl_info_t *fc_encl_info
 *
 *  RETURN VALUES/ERRORS:
 *      Pointer to terminator_fc_enclosure_info_t stucture
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
terminator_fc_enclosure_info_t * fc_enclosure_info_new(fbe_terminator_fc_encl_info_t *fc_encl_info)
{
    terminator_fc_enclosure_info_t *new_info = NULL;
    
    new_info = (terminator_fc_enclosure_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_fc_enclosure_info_t));
    if (new_info == NULL)
        return new_info;
    fbe_zero_memory(&new_info->uid, sizeof(new_info->uid));
    /* set new_info's fields */
    new_info->enclosure_type = fc_encl_info->encl_type;
    new_info->backend = fc_encl_info->backend_number;
    new_info->encl_number = fc_encl_info->encl_number;
    new_info->encl_state_change = FBE_FALSE;
    fbe_copy_memory(new_info->uid, fc_encl_info->uid, sizeof(new_info->uid));
    return new_info;
}
/**************************************************************************
 *  allocate_fc_enclosure_info()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function allocates memory of fc_enclosure_info with defaults.
 *
 *  PARAMETERS:
 *
 *  RETURN VALUES/ERRORS:
 *      Pointer to terminator_fc_enclosure_info_t stucture
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
terminator_fc_enclosure_info_t * allocate_fc_enclosure_info()
{
    terminator_fc_enclosure_info_t *new_info = NULL;
   
    new_info = (terminator_fc_enclosure_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_fc_enclosure_info_t));
    if (new_info == NULL)
    {
        return new_info;
    }

    fbe_zero_memory(&new_info->uid, sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE);
    /* set new_info's fields */
    new_info->enclosure_type = FBE_FC_ENCLOSURE_TYPE_INVALID;
    new_info->backend = -1;
    new_info->encl_number = -1;
    new_info->encl_state_change = FBE_FALSE;
    return new_info;
}
/**************************************************************************
 *  fc_enclosure_set_enclosure_type()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets enclosure type for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_fc_enclosure_type_t type
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_set_enclosure_type(terminator_enclosure_t * self, fbe_fc_enclosure_type_t type)
{
    terminator_fc_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    info->enclosure_type = type;
    return FBE_STATUS_OK;
}
/**************************************************************************
 *  fc_enclosure_set_state_change()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets state change for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_bool_t encl_state_change
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_set_state_change(terminator_enclosure_t * self, fbe_bool_t encl_state_change)
{
    terminator_fc_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if(info->encl_state_change != encl_state_change)
    {
       info->encl_state_change = encl_state_change;
    }   
    return FBE_STATUS_OK;
}
/**************************************************************************
 *  fc_enclosure_get_state_change()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets state change for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_bool_t encl_state_change
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_get_state_change(terminator_enclosure_t * self, fbe_bool_t *encl_state_change)
{
    terminator_fc_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    *encl_state_change =info->encl_state_change;      
    return FBE_STATUS_OK;
}
/**************************************************************************
 *  fc_enclosure_get_enclosure_type()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets enclosure type for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_fc_enclosure_type_t enclosure_type
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_fc_enclosure_type_t fc_enclosure_get_enclosure_type(terminator_enclosure_t * self)
{
    terminator_fc_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_FC_ENCLOSURE_TYPE_INVALID;
    }
    return (info->enclosure_type);
}
/**************************************************************************
 *  fc_enclosure_set_enclosure_uid()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets enclosure uid for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_u8_t *uid
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_set_enclosure_uid(terminator_enclosure_t * self, fbe_u8_t *uid)
{
    terminator_fc_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&info->uid, uid, sizeof(info->uid));
    return FBE_STATUS_OK;
}
/**************************************************************************
 *  fc_enclosure_get_enclosure_uid()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets enclosure uid for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u8_t  uid
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u8_t *fc_enclosure_get_enclosure_uid(terminator_enclosure_t * self)
{
    terminator_fc_enclosure_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return (info->uid);
}
/**************************************************************************
 *  fc_enclosure_set_backend_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets backend number for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_u32_t backend_number
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_set_backend_number(terminator_enclosure_t * self, fbe_u32_t backend_number)
{
    terminator_fc_enclosure_info_t * info = NULL;
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
/**************************************************************************
 *  fc_enclosure_set_enclosure_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets enclosure number for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_u32_t enclosure_number
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_set_enclosure_number(terminator_enclosure_t * self, fbe_u32_t enclosure_number)
{
    terminator_fc_enclosure_info_t * info = NULL;
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
/**************************************************************************
 *  fc_enclosure_get_enclosure_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets enclosure number for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t - encl_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_enclosure_get_enclosure_number(terminator_enclosure_t * self)
{
    terminator_fc_enclosure_info_t * info = NULL;
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
/**************************************************************************
 *  fc_enclosure_get_backend_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets backend number for fc enclosure.
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t - backend
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_enclosure_get_backend_number(terminator_enclosure_t * self)
{
    terminator_fc_enclosure_info_t * info = NULL;
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
/**************************************************************************
 *  fc_enclosure_is_slot_available()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function checks if slot number is available
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_u32_t slot_number
 *      fbe_bool_t * result
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_is_slot_available(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_bool_t * result)
{
    fbe_status_t       status = FBE_STATUS_GENERIC_FAILURE;
    base_component_t * child = NULL;

    //
    // terminator_max_drive_slots() accepts the first parameter as type
    //  fbe_sas_enclosure_type_t, and we don't have a similar function
    //  to get the max drive slots for type fbe_fc_enclosure_type_t now.
    //
#if 0
    fbe_fc_enclosure_type_t encl_type;
    fbe_u8_t max_drive_slots;

    encl_type = fc_enclosure_get_enclosure_type(self);
    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    if(status != FBE_STATUS_OK)
    {
        *result = FBE_FALSE;
        return status;
    }

    /* Should be a valid slot number */
    if (slot_number >= max_drive_slots)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        *result = FBE_FALSE;
        return status;
    }
#endif // 0

    /* Check if this slot is used by any other drive */
    child = base_component_get_first_child(&self->base);
    while (child != NULL)
    {
        switch (base_component_get_component_type(child))
        {
        case TERMINATOR_COMPONENT_TYPE_DRIVE:
            if (fc_drive_get_slot_number((terminator_drive_t *)child)== slot_number)
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

/**************************************************************************
 *  fc_enclosure_get_device_ptr_by_slot()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets device pointer by slot number
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_u32_t slot_number
 *      fbe_terminator_device_ptr_t *device_ptr
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_get_device_ptr_by_slot(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_terminator_device_ptr_t *device_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

#if 0
    fbe_fc_enclosure_type_t encl_type;
    fbe_u8_t max_drive_slots;
#endif // see comments below

    base_component_t * child = NULL;
    *device_ptr = NULL;

    //
    // terminator_max_drive_slots() accepts the first parameter as type
    //  fbe_sas_enclosure_type_t, and we don't have a similar function
    //  to get the max drive slots for type fbe_fc_enclosure_type_t now.
    //
#if 0
    encl_type = fc_enclosure_get_enclosure_type(self);
    status = terminator_max_drive_slots(encl_type, &max_drive_slots);
    RETURN_ON_ERROR_STATUS;

    /* Should be a valid slot number */
    if (slot_number >= max_drive_slots)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif // 0

    child = base_component_get_first_child(&self->base);
    while (child != NULL)
    {
        if(base_component_get_component_type(child)==TERMINATOR_COMPONENT_TYPE_DRIVE)
        {
            if (fc_drive_get_slot_number((terminator_drive_t *)child)== slot_number)
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
/**************************************************************************
 *  fc_enclosure_insert_fc_drive()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function insert fc drive in enclosure given
 *
 *  PARAMETERS:
 *      terminator_enclosure_t * self
 *      fbe_u32_t slot_number
 *      fbe_terminator_fc_drive_info_t *drive_info
 *      fbe_terminator_device_ptr_t *device_ptr
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_enclosure_insert_fc_drive(terminator_enclosure_t * self,
                                            fbe_u32_t slot_number,
                                            fbe_terminator_fc_drive_info_t *drive_info,
                                            fbe_terminator_device_ptr_t *drive_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    terminator_drive_t * new_drive = NULL;
    terminator_fc_drive_info_t * attributes = NULL;
    base_component_t * drive_base = NULL;

    new_drive = drive_new();
    drive_set_protocol(new_drive, FBE_DRIVE_TYPE_FIBRE);
    drive_base = (base_component_t *)new_drive;

    attributes = fc_drive_info_new(drive_info);
    base_component_assign_attributes(drive_base, attributes);
    fc_drive_set_slot_number(new_drive, slot_number);
    fc_drive_set_drive_type(new_drive, drive_info->drive_type);
    fc_drive_info_set_serial_number(attributes, drive_info->drive_serial_number);
    fc_drive_info_set_product_id(attributes, drive_info->product_id);
    
    *drive_ptr = new_drive;

    status = base_component_add_child_to_front (&self->base, base_component_get_queue_element(drive_base));
    
    return status;
}


