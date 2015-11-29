/**************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 **************************************************************************/

/***************************************************************************
 *  fbe_terminator_fc_drive.c
 ***************************************************************************
 *
 *  Description
 *      terminator fc drive class
 *
 *
 *  History:
 *      09/23/08    Dipak Patel    Created
 *
 ***************************************************************************/

#include "terminator_drive.h"
#include "terminator_fc_drive.h"
#include "terminator_login_data.h"
#include "terminator_sas_io_api.h"
#include "terminator_enclosure.h"
#include "fbe_terminator_device_registry.h"
#include "fbe_terminator.h"
#include "terminator_board.h"
#include "terminator_virtual_phy.h"
#include "terminator_port.h"
#include "fbe_terminator_common.h"

/**************************************************************************
 *  allocate_fc_drive_info()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function allocate memory for FC drive info.
 *
 *  PARAMETERS:
 *
 *  RETURN VALUES/ERRORS:
 *      Pointer to terminator_fc_drive_info_t stucture
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
terminator_fc_drive_info_t * allocate_fc_drive_info(void)
{
    terminator_fc_drive_info_t *new_info = NULL;
   
    new_info = (terminator_fc_drive_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_fc_drive_info_t));
    if (new_info == NULL)
    {
        return NULL;
    }    
    fbe_zero_memory(&new_info->drive_serial_number, sizeof(new_info->drive_serial_number));
    fbe_zero_memory(&new_info->product_id, sizeof(new_info->product_id));
    new_info->slot_number = INVALID_SLOT_NUMBER;
    new_info->drive_type = FBE_SAS_DRIVE_INVALID;
    new_info->backend_number = -1;
    new_info->encl_number = -1;    
    return new_info;
}
/**************************************************************************
 *  fc_drive_info_new()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function allocate memory for FC drive info.
 *    Same as previous function but with assigned data 
 *
 *  PARAMETERS:
 *
 *  RETURN VALUES/ERRORS:
 *      Pointer to terminator_fc_drive_info_t stucture
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
terminator_fc_drive_info_t * fc_drive_info_new(fbe_terminator_fc_drive_info_t *fc_drive_info)
{
    terminator_fc_drive_info_t *new_info = NULL;
    
    new_info = (terminator_fc_drive_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_fc_drive_info_t));
    if (new_info == NULL)
    {
        return new_info;
    }
    
    fbe_zero_memory(&new_info->drive_serial_number, sizeof(new_info->drive_serial_number));
    fbe_zero_memory(&new_info->product_id, sizeof(new_info->product_id));
    /* set new_info fields */
    fbe_copy_memory(new_info->drive_serial_number, fc_drive_info->drive_serial_number, sizeof(new_info->drive_serial_number));
    fbe_copy_memory(new_info->product_id, fc_drive_info->product_id, sizeof(new_info->product_id));
    new_info->drive_type = fc_drive_info->drive_type;
    new_info->backend_number = fc_drive_info->backend_number;
    new_info->encl_number = (fbe_u32_t)fc_drive_info->encl_number; /* this should be changed */
    new_info->slot_number = INVALID_SLOT_NUMBER;
    return new_info;
}
/**************************************************************************
 *  fc_drive_set_drive_type()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets drive type for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *      fbe_fc_drive_type_t type
 *
 *  RETURN VALUES/ERRORS:
 *      NONE   
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_drive_set_drive_type(terminator_drive_t * self, fbe_fc_drive_type_t type)
{
    terminator_fc_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    info->drive_type = type;
}
/**************************************************************************
 *  fc_drive_get_drive_type()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets drive type for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_fc_drive_type_t drive type
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_fc_drive_type_t fc_drive_get_drive_type(terminator_drive_t * self)
{
    terminator_fc_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_FC_DRIVE_INVALID;
    }
    return (info->drive_type);
}
/**************************************************************************
 *  fc_drive_set_slot_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets slot number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *      fbe_u32_t slot_number
 *
 *  RETURN VALUES/ERRORS:
 *      None  
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_drive_set_slot_number(terminator_drive_t * self, fbe_u32_t slot_number)
{
    terminator_fc_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    info->slot_number = slot_number;
}
/**************************************************************************
 *  fc_drive_get_slot_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets slot number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t slot_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_drive_get_slot_number(terminator_drive_t * self)
{
    terminator_fc_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return INVALID_SLOT_NUMBER;
    }
    return (info->slot_number);
}
/**************************************************************************
 *  fc_drive_info_get_serial_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets serial number for FC drive info.
 *
 *  PARAMETERS:
 *      terminator_fc_drive_info_t * self 
 *      fbe_u8_t *serial_number
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_drive_info_get_serial_number(terminator_fc_drive_info_t * self, fbe_u8_t *serial_number)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(serial_number, &self->drive_serial_number, sizeof(self->drive_serial_number));
    return FBE_STATUS_OK;
}
/**************************************************************************
 *  fc_drive_info_set_serial_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets serial number for FC drive info.
 *
 *  PARAMETERS:
 *      terminator_fc_drive_info_t * self 
 *      fbe_u8_t *serial_number
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_status_t fc_drive_info_set_serial_number(terminator_fc_drive_info_t * self, fbe_u8_t *serial_number)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(&self->drive_serial_number, serial_number, sizeof(self->drive_serial_number));
    return FBE_STATUS_OK;
}

/**************************************************************************
 *  fc_drive_info_get_product_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets product id for FC drive info.
 *
 *  PARAMETERS:
 *      terminator_fc_drive_info_t * self
 *      fbe_u8_t *product_id
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jun-17-2010: Bo Gao created.
 **************************************************************************/
fbe_status_t fc_drive_info_get_product_id(terminator_fc_drive_info_t * self, fbe_u8_t *product_id)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(product_id, &self->product_id, sizeof(self->product_id));
    return FBE_STATUS_OK;
}

/**************************************************************************
 *  fc_drive_info_set_product_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets product id for FC drive info.
 *
 *  PARAMETERS:
 *      terminator_fc_drive_info_t * self
 *      fbe_u8_t *product_id
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_status_t - success or failure
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jun-17-2010: Bo Gao created.
 **************************************************************************/
fbe_status_t fc_drive_info_set_product_id(terminator_fc_drive_info_t * self, fbe_u8_t *product_id)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(&self->product_id, product_id, sizeof(self->product_id));
    return FBE_STATUS_OK;
}

/**************************************************************************
 *  fc_drive_get_serial_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets serial number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u8_t  drive_serial_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u8_t * fc_drive_get_serial_number(terminator_drive_t * self)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return info->drive_serial_number;
}
/**************************************************************************
 *  fc_drive_set_serial_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets serial number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *      fbe_u8_t *serial_number
 *
 *  RETURN VALUES/ERRORS:
 *      None 
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_drive_set_serial_number(terminator_drive_t * self, const fbe_u8_t * serial_number)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL || serial_number == NULL)
    {
        return;
    }
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        fbe_zero_memory(info->drive_serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
        strncpy(info->drive_serial_number, serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE - 1);
    }
}

/**************************************************************************
 *  fc_drive_get_product_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets product id for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u8_t  product_id
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jun-17-2010: Bo Gao created.
 **************************************************************************/
fbe_u8_t * fc_drive_get_product_id(terminator_drive_t * self)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return NULL;
    }
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return info->product_id;
}

/**************************************************************************
 *  fc_drive_set_product_id()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets product id for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self
 *      fbe_u8_t * product_id
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Jun-17-2010: Bo Gao created.
 **************************************************************************/
void fc_drive_set_product_id(terminator_drive_t * self, const fbe_u8_t * product_id)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL || product_id == NULL)
    {
        return;
    }
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        fbe_zero_memory(info->product_id, sizeof(info->product_id));
        strncpy(info->product_id, product_id, sizeof(info->product_id) - 1);
    }
}

/**************************************************************************
 *  fc_drive_set_backend_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets backend number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *      fbe_u32_t backend_number
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_drive_set_backend_number(terminator_drive_t * self, fbe_u32_t backend_number)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        info->backend_number = backend_number;
    }
    return;
}
/**************************************************************************
 *  fc_drive_set_enclosure_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function sets enclosure number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *      fbe_u32_t encl_number
 *
 *  RETURN VALUES/ERRORS:
 *      None
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
void fc_drive_set_enclosure_number(terminator_drive_t * self, fbe_u32_t encl_number)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return;
    }
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        info->encl_number = encl_number;
    }
    return;
}
/**************************************************************************
 *  fc_drive_get_backend_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets backend number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t backend_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_drive_get_backend_number(terminator_drive_t * self)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return -1;
    }
    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return -1;
    }
    return info->backend_number;
}
/**************************************************************************
 *  fc_drive_get_enclosure_number()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    This function gets enclosure number for FC drive.
 *
 *  PARAMETERS:
 *      terminator_drive_t * self 
 *
 *  RETURN VALUES/ERRORS:
 *      fbe_u32_t encl_number
 *
 *  NOTES:
 *
 *  HISTORY:
 *      Sep-23-2009: Dipak Patel created.
 **************************************************************************/
fbe_u32_t fc_drive_get_enclosure_number(terminator_drive_t * self)
{
    terminator_fc_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return -1;
    }
    info = base_component_get_attributes(&self->base);
    if(info == NULL)
    {
        return -1;
    }
    return info->encl_number;
}
