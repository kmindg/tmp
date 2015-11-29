#include "terminator_login_data.h"


/* Init callback_login data structure */
void cpd_shim_callback_login_init(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return;
    login_data->device_type = FBE_PMC_SHIM_DEVICE_TYPE_INVALID;
    login_data->device_id = CPD_DEVICE_ID_INVALID;
    login_data->device_address = FBE_SAS_ADDRESS_INVALID;
    login_data->parent_device_id = CPD_DEVICE_ID_INVALID;
    login_data->parent_device_address = FBE_SAS_ADDRESS_INVALID;
    cpd_shim_callback_login_init_device_locator(login_data);
}
/* device_type setter */
void cpd_shim_callback_login_set_device_type(fbe_cpd_shim_callback_login_t * login_data, fbe_cpd_shim_discovery_device_type_t device_type)
{
    if (login_data == NULL)
        return;
    login_data->device_type = device_type;
}
/* device_type getter*/
fbe_cpd_shim_discovery_device_type_t cpd_shim_callback_login_get_device_type(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return FBE_PMC_SHIM_DEVICE_TYPE_INVALID;
    return (login_data->device_type);
}

/* device_id setter */
void cpd_shim_callback_login_set_device_id(fbe_cpd_shim_callback_login_t * login_data, fbe_miniport_device_id_t device_id)
{
    if (login_data == NULL)
        return;
    login_data->device_id = device_id;
}
/* device_id getter*/
fbe_miniport_device_id_t cpd_shim_callback_login_get_device_id(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return CPD_DEVICE_ID_INVALID;
    return (login_data->device_id);
}

/* device_address setter*/
void cpd_shim_callback_login_set_device_address(fbe_cpd_shim_callback_login_t * login_data, fbe_sas_address_t device_address)
{
    if (login_data == NULL)
        return;
    login_data->device_address = device_address;
}
/* device_address getter*/
fbe_sas_address_t cpd_shim_callback_login_get_device_address(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return FBE_SAS_ADDRESS_INVALID;
    return (login_data->device_address);
}
/* parent_device_id setter*/
void cpd_shim_callback_login_set_parent_device_id(fbe_cpd_shim_callback_login_t * login_data, fbe_miniport_device_id_t device_id)
{
    if (login_data == NULL)
        return;
    login_data->parent_device_id = device_id;
}
/* parent_device_id getter*/
fbe_miniport_device_id_t cpd_shim_callback_login_get_parent_device_id(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return CPD_DEVICE_ID_INVALID;
    return (login_data->parent_device_id);
}
/* parent_device_address setter*/
void cpd_shim_callback_login_set_parent_device_address(fbe_cpd_shim_callback_login_t * login_data, fbe_sas_address_t device_address)
{
    if (login_data == NULL)
        return;
    login_data->parent_device_address = device_address;
}
/* parent_device_address getter*/
fbe_sas_address_t cpd_shim_callback_login_get_parent_device_address(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return CPD_DEVICE_ID_INVALID;
    return (login_data->parent_device_address);
}

/*****************************************
* the following deal with device_locator *
******************************************/
/* Initialize the device_locator */
void cpd_shim_callback_login_init_device_locator(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return;
    fbe_zero_memory(&login_data->device_locator, sizeof(fbe_cpd_shim_device_locator_t));
}

/* device_locator setter*/
void cpd_shim_callback_login_set_device_locator(fbe_cpd_shim_callback_login_t * login_data, fbe_cpd_shim_device_locator_t * device_locator)
{
    if (login_data == NULL)
        return;
    fbe_move_memory(&login_data->device_locator, device_locator, sizeof(fbe_cpd_shim_device_locator_t));
}
/* enclosure_chain_depth getter*/
fbe_cpd_shim_device_locator_t * cpd_shim_callback_login_get_device_locator(fbe_cpd_shim_callback_login_t * login_data)
{
    if (login_data == NULL)
        return NULL;
    return (&login_data->device_locator);
}

/* enclosure_chain_depth setter*/
void cpd_shim_callback_login_set_enclosure_chain_depth(fbe_cpd_shim_callback_login_t * login_data, fbe_u8_t enclosure_chain_depth)
{
    fbe_cpd_shim_device_locator_t * locator = NULL;
    if (login_data == NULL)
        return;
    locator = &login_data->device_locator;
    locator->enclosure_chain_depth = enclosure_chain_depth;
}
/* enclosure_chain_depth getter*/
fbe_u8_t cpd_shim_callback_login_get_enclosure_chain_depth(fbe_cpd_shim_callback_login_t * login_data)
{
    fbe_cpd_shim_device_locator_t * locator = NULL;
    if (login_data == NULL)
        return INVALID_LOCATOR_DATA;
    locator = &login_data->device_locator;
    return (locator->enclosure_chain_depth);
}
/* enclosure_chain_width setter*/
void cpd_shim_callback_login_set_enclosure_chain_width(fbe_cpd_shim_callback_login_t * login_data, fbe_u8_t enclosure_chain_width)
{
    fbe_cpd_shim_device_locator_t * locator = NULL;
    if (login_data == NULL)
        return;
    locator = &login_data->device_locator;
    locator->enclosure_chain_width = enclosure_chain_width;
}
/* enclosure_chain_width getter*/
fbe_u8_t cpd_shim_callback_login_get_enclosure_chain_width(fbe_cpd_shim_callback_login_t * login_data)
{
    fbe_cpd_shim_device_locator_t * locator = NULL;
    if (login_data == NULL)
        return INVALID_LOCATOR_DATA;
    locator = &login_data->device_locator;
    return (locator->enclosure_chain_width);
}
/* phy_number setter*/
void cpd_shim_callback_login_set_phy_number(fbe_cpd_shim_callback_login_t * login_data, fbe_u8_t phy_number)
{
    fbe_cpd_shim_device_locator_t * locator = NULL;
    if (login_data == NULL)
        return;
    locator = &login_data->device_locator;
    locator->phy_number = phy_number;
}
/* phy_number getter*/
fbe_u8_t cpd_shim_callback_login_get_phy_number(fbe_cpd_shim_callback_login_t * login_data)
{
    fbe_cpd_shim_device_locator_t * locator = NULL;
    if (login_data == NULL)
        return INVALID_LOCATOR_DATA;
    locator = &login_data->device_locator;
    return (locator->phy_number);
}
