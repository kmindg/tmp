/***************************************************************************
 *  fbe_terminator_drive.c
 ***************************************************************************
 *
 *  Description
 *      terminator drive class
 *
 *
 *  History:
 *      06/24/08    guov    Created
 *
 ***************************************************************************/

#include "terminator_drive.h"
#include "terminator_login_data.h"
#include "terminator_sas_io_api.h"
#include "fbe_terminator_common.h"

terminator_sata_drive_info_t * allocate_sata_drive_info()
{
    terminator_sata_drive_info_t *new_info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    new_info = (terminator_sata_drive_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sata_drive_info_t));
    if (new_info == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for sata drive info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return NULL;
    }

    fbe_zero_memory(new_info, sizeof(terminator_sata_drive_info_t));

    login_data = sata_drive_info_get_login_data(new_info);
    cpd_shim_callback_login_init(login_data);
    cpd_shim_callback_login_set_device_type(login_data, FBE_PMC_SHIM_DEVICE_TYPE_STP);
    fbe_zero_memory(&new_info->drive_serial_number, sizeof(new_info->drive_serial_number));
    fbe_zero_memory(&new_info->product_id, sizeof(new_info->product_id));
    new_info->slot_number = INVALID_SLOT_NUMBER;
    new_info->drive_type = FBE_SATA_DRIVE_INVALID;
    new_info->backend_number = -1;
    new_info->encl_number = -1;
    new_info->miniport_sas_device_table_index = INVALID_TMSDT_INDEX;
    return new_info;
}

terminator_sata_drive_info_t * sata_drive_info_new(fbe_terminator_sata_drive_info_t *sata_drive_info)
{
    fbe_cpd_shim_callback_login_t *login_data = NULL;
    terminator_sata_drive_info_t  *new_info   = (terminator_sata_drive_info_t *)fbe_terminator_allocate_memory(sizeof(terminator_sata_drive_info_t));
    if (!new_info)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "%s failed to allocate memory for sata drive info at line %d.\n",
                         __FUNCTION__,
                         __LINE__);

        return NULL;
    }

    fbe_zero_memory(new_info, sizeof(terminator_sata_drive_info_t));

    login_data = sata_drive_info_get_login_data(new_info);
    cpd_shim_callback_login_init(login_data);
    cpd_shim_callback_login_set_device_address(login_data, sata_drive_info->sas_address);
    cpd_shim_callback_login_set_device_type(login_data, FBE_PMC_SHIM_DEVICE_TYPE_STP);
    fbe_zero_memory(&new_info->drive_serial_number, sizeof(new_info->drive_serial_number));
    fbe_zero_memory(&new_info->product_id, sizeof(new_info->product_id));
    return new_info;
}

fbe_cpd_shim_callback_login_t * sata_drive_info_get_login_data(terminator_sata_drive_info_t * self)
{
    return (&self->login_data);
}

void sata_drive_set_drive_type(terminator_drive_t * self, fbe_sata_drive_type_t type)
{
    terminator_sata_drive_info_t * info = NULL;
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    info->drive_type = type;
}

fbe_sata_drive_type_t sata_drive_get_drive_type(terminator_drive_t * self)
{
    terminator_sata_drive_info_t * info = NULL;
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return FBE_SATA_DRIVE_INVALID;
    }
    return (info->drive_type);
}

void sata_drive_set_slot_number(terminator_drive_t * self, fbe_u32_t slot_number)
{
    terminator_sata_drive_info_t * info = NULL;

    self->slot_number = slot_number;
    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    info->slot_number = slot_number;
}
fbe_u32_t sata_drive_get_slot_number(terminator_drive_t * self)
{
    terminator_sata_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return INVALID_SLOT_NUMBER;
    }
    return (info->slot_number);
}

fbe_sas_address_t sata_drive_get_sas_address(terminator_drive_t * self)
{
    terminator_sata_drive_info_t * info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;
    fbe_sas_address_t address = FBE_SAS_ADDRESS_INVALID;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return address;
    }
    login_data = sata_drive_info_get_login_data(info);
    address = cpd_shim_callback_login_get_device_address(login_data);
    return address;
}

void sata_drive_set_sas_address(terminator_drive_t * self, fbe_sas_address_t address)
{
    terminator_sata_drive_info_t * info = NULL;
    fbe_cpd_shim_callback_login_t * login_data = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return;
    }
    login_data = sata_drive_info_get_login_data(info);
    cpd_shim_callback_login_set_device_address(login_data, address);
    return;
}

fbe_status_t sata_drive_call_io_api(terminator_drive_t * self, fbe_terminator_io_t * terminator_io)
{
    return fbe_terminator_sas_drive_payload (terminator_io, self);
}
/* fbe_u8_t *serial_number is the buffer we should copy the serial number to */
fbe_status_t sata_drive_info_get_serial_number(terminator_sata_drive_info_t * self, fbe_u8_t *serial_number)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(serial_number, &self->drive_serial_number, sizeof(self->drive_serial_number));
    return FBE_STATUS_OK;
}
fbe_status_t sata_drive_info_set_serial_number(terminator_sata_drive_info_t * self, fbe_u8_t *serial_number)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(&self->drive_serial_number, serial_number, sizeof(self->drive_serial_number));
    return FBE_STATUS_OK;
}

fbe_status_t sata_drive_info_get_product_id(terminator_sata_drive_info_t * self, fbe_u8_t *product_id)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(product_id, &self->product_id, sizeof(self->product_id));
    return FBE_STATUS_OK;
}
fbe_status_t sata_drive_info_set_product_id(terminator_sata_drive_info_t * self, fbe_u8_t *product_id)
{
    if (self == NULL)
        return FBE_STATUS_GENERIC_FAILURE;
    fbe_copy_memory(&self->product_id, product_id, sizeof(self->product_id));
    return FBE_STATUS_OK;
}

fbe_u8_t * sata_drive_get_serial_number(terminator_drive_t * self)
{
    terminator_sata_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return info->drive_serial_number;
}

/* Additional getters and setters required for TCM integration */
void sata_drive_set_serial_number(terminator_drive_t * self, const fbe_u8_t * serial_number)
{
    terminator_sata_drive_info_t * info = NULL;
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

fbe_u8_t * sata_drive_get_product_id(terminator_drive_t * self)
{
    terminator_sata_drive_info_t * info = NULL;

    info = base_component_get_attributes(&self->base);
    if (info == NULL)
    {
        return NULL;
    }
    return info->product_id;
}

void sata_drive_set_product_id(terminator_drive_t * self, const fbe_u8_t * product_id)
{
    terminator_sata_drive_info_t * info = NULL;
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

fbe_status_t sata_drive_verify_product_id(fbe_u8_t * product_id)
{
    const fbe_u8_t * sata_drive_product_id_valid_substrings[] = {
        "CLAR"
    };

    size_t sata_drive_product_id_valid_substrings_num = TERMINATOR_ARRAY_SIZE(sata_drive_product_id_valid_substrings);
    size_t i = 0;

    if (NULL == product_id)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (i = 0; i < sata_drive_product_id_valid_substrings_num; ++i)
    {
        if (strstr(product_id + TERMINATOR_SCSI_INQUIRY_PRODUCT_ID_LEADING_OFFSET,
                   sata_drive_product_id_valid_substrings[i]))
        {
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

void sata_drive_set_backend_number(terminator_drive_t * self, fbe_u32_t backend_number)
{
    terminator_sata_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return;
    }
    self->backend_number = backend_number;
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        info->backend_number = backend_number;
    }
    return;
}
void sata_drive_set_enclosure_number(terminator_drive_t * self, fbe_u32_t encl_number)
{
    terminator_sata_drive_info_t * info = NULL;
    if(self == NULL)
    {
        return;
    }
    self->encl_number = encl_number;
    info = base_component_get_attributes(&self->base);
    if(info != NULL)
    {
        info->encl_number = encl_number;
    }
    return;
}

fbe_u32_t sata_drive_get_backend_number(terminator_drive_t * self)
{
    terminator_sata_drive_info_t * info = NULL;
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

fbe_u32_t sata_drive_get_enclosure_number(terminator_drive_t * self)
{
    terminator_sata_drive_info_t * info = NULL;
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
