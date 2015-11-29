/***************************************************************************
 *  fbe_terminator_sas_port.c
 ***************************************************************************
 *
 *  Description
 *      terminator sas port class
 *      
 *
 *  History:
 *      09/09/08    guov    Created
 *    
 ***************************************************************************/
#include "terminator_sas_port.h"

/******************************/
/*     local function         */
/******************************/

fbe_status_t fbe_terminator_sas_port_info_get_io_port_number(fbe_terminator_sas_port_info_t *port_info, fbe_u32_t *io_port_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    *io_port_number = port_info->io_port_number;
    return status;
}

fbe_status_t fbe_terminator_sas_port_info_get_portal_number(fbe_terminator_sas_port_info_t *port_info, fbe_u32_t *portal_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    *portal_number = port_info->portal_number;
    return status;
}

fbe_status_t fbe_terminator_sas_port_info_get_backend_number(fbe_terminator_sas_port_info_t *port_info, fbe_u32_t *backend_number)
{
    fbe_status_t status = FBE_STATUS_OK;
    *backend_number = port_info->backend_number;
    return status;
}

fbe_status_t fbe_terminator_sas_port_info_get_sas_address(fbe_terminator_sas_port_info_t *port_info, fbe_sas_address_t *sas_address)
{
    fbe_status_t status = FBE_STATUS_OK;
    *sas_address = port_info->sas_address;
    return status;
}