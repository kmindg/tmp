/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management utility functionalities.
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   19-Jul-2010 - Created. bphilbin
 *
 ***************************************************************************/
#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_file.h"
#include "EspMessages.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_module_info.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_power_supply_interface.h"
#include "fbe/fbe_module_mgmt_mapping.h"
#include "generic_utils_lib.h"

static fbe_status_t fbe_module_mgmt_faultIoPortLed(fbe_module_mgmt_t *module_mgmt,
                                                   fbe_u8_t iom_num, 
                                                   fbe_u8_t port_num);
static fbe_status_t fbe_module_mgmt_faultIoModuleLed(fbe_module_mgmt_t *module_mgmt,
                                                     fbe_u8_t iom_num,
                                                     fbe_bool_t faulted);
static fbe_bool_t fbe_module_mgmt_port_sfp_faulted(fbe_port_state_t    port_state,
                                                   fbe_port_substate_t port_substate);
fbe_bool_t fbe_module_mgmt_is_port_link_optimal(fbe_module_mgmt_t *module_mgmt, fbe_u32_t iom_num, fbe_u32_t port_num);

#define FBE_MODULE_MGMT_IN_FAMILY_CONVERSION_SLIC_SLOT_SHIFT 2
#define FBE_MODULE_MGMT_SLIC_TYPE_STRING_LENGTH 256

/*!**************************************************************
 * fbe_module_mgmt_get_io_port_index()
 ****************************************************************
 * @brief
 *  This function gets port index based on port and slot.  Port
 *  index is used to locate a port in the module_mgmt list of ports
 *  it is a global index across all io modules.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param slot - slot
 * @param port - port 
 *
 * @return fbe_u8_t port_index
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u8_t fbe_module_mgmt_get_io_port_index(fbe_u8_t slot, fbe_u8_t port)
{
    fbe_u8_t port_index = 0;

    port_index = (slot * MAX_IO_PORTS_PER_MODULE) + port;
    return port_index;
}
/******************************************************
 * end fbe_module_mgmt_get_io_port_index() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_set_module_state()
 ****************************************************************
 * @brief
 *  This function sets the specified module with the specified module state
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param module_state - module state to set
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_module_state(fbe_module_mgmt_t *module_mgmt, 
                                      SP_ID sp_id, fbe_u8_t iom_num, 
                                      fbe_module_state_t module_state)
{
    module_mgmt->io_module_info[iom_num].logical_info[sp_id].module_state = module_state;
    return;
}

/*!**************************************************************
 * fbe_module_mgmt_get_module_state()
 ****************************************************************
 * @brief
 *  This function gets the module state for the specified module.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_module_state_t fbe_module_mgmt_get_module_state(fbe_module_mgmt_t *module_mgmt, 
                                                    SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].logical_info[sp_id].module_state;
}

/*!**************************************************************
 * fbe_module_mgmt_set_module_substate()
 ****************************************************************
 * @brief
 *  This function sets the specified module with the specified module state
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param module_substate - module substate to set
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_module_substate(fbe_module_mgmt_t *module_mgmt, 
                                      SP_ID sp_id, fbe_u8_t iom_num, 
                                      fbe_module_substate_t module_substate)
{
    module_mgmt->io_module_info[iom_num].logical_info[sp_id].module_substate = module_substate;
    return;
}

/*!**************************************************************
 * fbe_module_mgmt_get_module_substate()
 ****************************************************************
 * @brief
 *  This function gets the module substate for the specified module.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_module_substate_t fbe_module_mgmt_get_module_substate(fbe_module_mgmt_t *module_mgmt, 
                                                    SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].logical_info[sp_id].module_substate;
}

/*!**************************************************************
 * fbe_module_mgmt_set_port_state()
 ****************************************************************
 * @brief
 *  This function sets the specified port with the specified port state
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param slot - slot
 * @param port - port
 * @param port_state - port state to set
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_port_state(fbe_module_mgmt_t *module_mgmt, 
                                    SP_ID sp_id, fbe_u8_t iom_num, 
                                    fbe_u8_t port_num, fbe_port_state_t 
                                    port_state)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_state = port_state;
    return;
}
/******************************************************
 * end fbe_module_mgmt_set_port_state() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_port_state()
 ****************************************************************
 * @brief
 *  This function returns a port state for the specified port.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param slot - slot
 * @param port - port
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_port_state_t fbe_module_mgmt_get_port_state(fbe_module_mgmt_t *module_mgmt, 
                                    SP_ID sp_id, fbe_u8_t iom_num, 
                                    fbe_u8_t port_num)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_state;
    
}
/******************************************************
 * end fbe_module_mgmt_get_port_state() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_set_port_substate()
 ****************************************************************
 * @brief
 *  This function sets the specified port with the specified port 
 *  substate
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param port - port
 * @param port_state - port substate to set
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_port_substate(fbe_module_mgmt_t *module_mgmt, 
                                       SP_ID sp_id, fbe_u8_t iom_num, 
                                       fbe_u8_t port_num, fbe_port_substate_t 
                                       port_substate)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_substate = port_substate;
    return;
}
/******************************************************
 * end fbe_module_mgmt_set_port_substate() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_port_substate()
 ****************************************************************
 * @brief
 *  This function gets the substate for the specified port.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param port - port
 *
 * @return port_substate
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_port_substate_t fbe_module_mgmt_get_port_substate(fbe_module_mgmt_t *module_mgmt, 
                                       SP_ID sp_id, fbe_u8_t iom_num, 
                                       fbe_u8_t port_num)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_substate;

}
/******************************************************
 * end fbe_module_mgmt_get_port_substate() 
 ******************************************************/



/*!**************************************************************
 * fbe_module_mgmt_get_platform_limit()
 ****************************************************************
 * @brief
 *  This function takes a port protocol and role and returns a platform
 *  limit for the maximum number of those ports that are configurable.
 *
 * @param port_protocol - protocol for the port
 * @param port_role - FE, BE or Uncommitted
 *
 * @return fbe_u8_t - port limit
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u8_t fbe_module_mgmt_get_platform_limit(fbe_module_mgmt_t *module_mgmt,
                                            IO_CONTROLLER_PROTOCOL port_protocol, 
                                            fbe_ioport_role_t port_role,
                                            fbe_module_slic_type_t slic_type)
{
    fbe_environment_limits_platform_port_limits_t platform_port_limits;

    fbe_set_memory(&platform_port_limits, 0, sizeof(fbe_environment_limits_platform_port_limits_t));
    fbe_environment_limit_get_platform_port_limits(&platform_port_limits);
    
    switch(port_protocol)
    {
    case IO_CONTROLLER_PROTOCOL_SAS:
        if(port_role == IOPORT_PORT_ROLE_BE)
        {
            return platform_port_limits.max_sas_be;
        }
        else if(port_role == IOPORT_PORT_ROLE_FE)
        {
            return platform_port_limits.max_sas_fe;
        }
        break;
    case IO_CONTROLLER_PROTOCOL_FCOE:
        return platform_port_limits.max_fcoe_fe;
        break;
    case IO_CONTROLLER_PROTOCOL_FIBRE:
        return platform_port_limits.max_fc_fe;
        break;
    case IO_CONTROLLER_PROTOCOL_ISCSI:
        {
            if(platform_port_limits.max_combined_iscsi_fe > 0)
            {
                return platform_port_limits.max_combined_iscsi_fe;
            }
            else if( (slic_type == FBE_SLIC_TYPE_ISCSI_10G_V2) ||
                (slic_type == FBE_SLIC_TYPE_ISCSI_COPPER) ||
                (slic_type == FBE_SLIC_TYPE_ISCSI_10G)    ||
                (slic_type == FBE_SLIC_TYPE_4PORT_ISCSI_10G) ||
                (slic_type == FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G) ||
                (slic_type == FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G) ||
                (slic_type == FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G))
            {
                return platform_port_limits.max_iscsi_10g_fe;
            }
            else
            {
                return platform_port_limits.max_iscsi_1g_fe;
            }
            break;
        }
            
    default:
        return 0xFF;
        break;
    }
    return 0xFF;
}
/******************************************************
 * end fbe_module_mgmt_get_platform_limit() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_get_port_role()
 ****************************************************************
 * @brief
 *  This function returns a port role for the specified port.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param port_num - port number on that io module
 *
 * @return fbe_ioport_role_t - port role (FE/BE/UNC)
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_ioport_role_t fbe_module_mgmt_get_port_role(fbe_module_mgmt_t *module_mgmt, 
                                              SP_ID sp_id, fbe_u8_t iom_num, 
                                              fbe_u8_t port_num)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.port_role;
}
/******************************************************
 * end fbe_module_mgmt_get_port_role() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_set_port_role()
 ****************************************************************
 * @brief
 *  This function sets the port role for the specified port.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param port_num - port number on that io module
 * @param port_role - port role to set
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_port_role(fbe_module_mgmt_t *module_mgmt, 
                                   SP_ID sp_id, fbe_u8_t iom_num, 
                                   fbe_u8_t port_num, fbe_ioport_role_t port_role)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.port_role = port_role;
}
/******************************************************
 * end fbe_module_mgmt_get_port_role() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_derive_port_role()
 ****************************************************************
 * @brief
 *  This function derives a port role based on the properties of
 *  the io module and the port.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_ioport_role_t fbe_module_mgmt_derive_port_role(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID sp_id, fbe_u8_t iom_num, 
                                                 fbe_u8_t port_num)
{
    fbe_module_slic_type_t  slic_type = FBE_SLIC_TYPE_UNKNOWN;
    fbe_ioport_role_t       port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
    IO_CONTROLLER_PROTOCOL  controller_protocol = IO_CONTROLLER_PROTOCOL_UNKNOWN;

    slic_type = fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num);

    if (slic_type == FBE_SLIC_TYPE_NA)
    {
        controller_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt, sp_id, iom_num, port_num);
        /* This code assumes all SAS ports are BE on the mezzanine */
        switch(controller_protocol)
        {
        case IO_CONTROLLER_PROTOCOL_SAS:
            port_role = FBE_PORT_ROLE_BE;
            break;
        case IO_CONTROLLER_PROTOCOL_FIBRE:
        case IO_CONTROLLER_PROTOCOL_ISCSI:
        case IO_CONTROLLER_PROTOCOL_FCOE:
            port_role = FBE_PORT_ROLE_FE;
            break;
        case IO_CONTROLLER_PROTOCOL_AGNOSTIC:
            port_role = FBE_PORT_ROLE_FE; // so far all CNA ports are only FE ports, if we have to we can look at SFP type for this
            break;
        default:
            port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
            break;
        }
    }
    else
    {
        int i = 0;
        for (i = 0; i < fbe_slic_type_property_map_size; i++)
        {
            if (fbe_slic_type_property_map[i].slic_type == slic_type)
            {
                port_role = fbe_slic_type_property_map[i].role;
                break;
            }
        }
    }

    /*
     * Temporary hack until the subroles are set correctly.
     */
    if(port_role == IOPORT_PORT_ROLE_FE)
    {
        fbe_module_mgmt_set_port_subrole(module_mgmt, sp_id, iom_num, 
                                   port_num, FBE_PORT_SUBROLE_NORMAL);
    }
    else if(port_role == IOPORT_PORT_ROLE_BE)
    {
        fbe_module_mgmt_set_port_subrole(module_mgmt, sp_id, iom_num, 
                                   port_num, FBE_PORT_SUBROLE_NORMAL);
    }

    return port_role;
}
/******************************************************
 * end fbe_module_mgmt_derive_port_role() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_get_port_subrole()
 ****************************************************************
 * @brief
 *  This function returns a port subrole for the specified port.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param port_num - port number on that io module
 *
 * @return fbe_port_role_t - port role (FE/BE/UNC)
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_port_subrole_t fbe_module_mgmt_get_port_subrole(fbe_module_mgmt_t *module_mgmt, 
                                              SP_ID sp_id, fbe_u8_t iom_num, 
                                              fbe_u8_t port_num)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.port_subrole;
}
/******************************************************
 * end fbe_module_mgmt_get_port_subrole() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_set_port_subrole()
 ****************************************************************
 * @brief
 *  This function sets the port subrole for the specified port.
 *
 * @param module_mgmt - context
 * @param sp - sp
 * @param iom_num - io module index number
 * @param port_num - port number on that io module
 * @param port_role - port role to set
 *
 * @return none
 *
 * @author
 *  21-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_port_subrole(fbe_module_mgmt_t *module_mgmt, 
                                   SP_ID sp_id, fbe_u8_t iom_num, 
                                   fbe_u8_t port_num, fbe_port_subrole_t port_subrole)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.port_subrole = port_subrole;
}
/******************************************************
 * end fbe_module_mgmt_get_port_subrole() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_iom_group()
 ****************************************************************
 * @brief
 *  This function gets an io module group based on the specified
 *  port information
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 *
 * @return - fbe_iom_group_t - io module group
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_iom_group_t fbe_module_mgmt_get_iom_group(fbe_module_mgmt_t *module_mgmt, 
                                              SP_ID sp_id, fbe_u8_t iom_num, 
                                              fbe_u8_t port_num)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.iom_group;
}
/******************************************************
 * end fbe_module_mgmt_get_iom_group() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_set_iom_group()
 ****************************************************************
 * @brief
 *  This function sets an io module group based on the specified
 *  port information and io module group
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 * @param - iom_group - io module group
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_iom_group(fbe_module_mgmt_t *module_mgmt, 
                                   SP_ID sp_id, fbe_u8_t iom_num, 
                                   fbe_u8_t port_num, 
                                   fbe_iom_group_t iom_group)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.iom_group = iom_group;
    return;
}
/******************************************************
 * end fbe_module_mgmt_set_iom_group() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_derive_iom_group()
 ****************************************************************
 * @brief
 *  This function derives an io module group for the specified
 *  port information based on SLIC type and in the case of
 *  mezzanines, protocol.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 * @param - iom_group - io module group
 *
 * @return - io module group
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_iom_group_t fbe_module_mgmt_derive_iom_group(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID sp_id, fbe_u8_t iom_num, 
                                                 fbe_u8_t port_num,
                                                 HW_MODULE_TYPE unique_id)
{
    fbe_module_slic_type_t  slic_type = FBE_SLIC_TYPE_UNKNOWN;
    fbe_iom_group_t iom_group = FBE_IOM_GROUP_UNKNOWN;
    IO_CONTROLLER_PROTOCOL  controller_protocol = IO_CONTROLLER_PROTOCOL_UNKNOWN;

    slic_type = fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num);

    /* FBE_SLIC_TYPE_NA is used for the embedded/on-board controllers */
    if (slic_type == FBE_SLIC_TYPE_NA)   
    {
        controller_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt, sp_id, iom_num, port_num);
        /* Using Unique ID to decide if SAS Ports are on BEM, others are assumes on mezzanine*/
        if((unique_id >= IRONHIDE_6G_SAS && unique_id <= IRONHIDE_6G_SAS_REV_C) || 
           (((unique_id >= BEACHCOMBER_PCB && unique_id <= BEACHCOMBER_PCB_SIM) ||
             (unique_id >= MERIDIAN_SP_ESX &&  unique_id <= MERIDIAN_SP_HYPERV) ||
             (unique_id == TUNGSTEN_SP)) && controller_protocol == IO_CONTROLLER_PROTOCOL_SAS))
        {
            iom_group = FBE_IOM_GROUP_K;
        }
        else if( (((unique_id >= OBERON_SP_85W_REV_A && unique_id <= OBERON_SP_E5_2609V3_REV_B) || unique_id ==OBERON_SP_E5_2660V3_REV_B) && controller_protocol == IO_CONTROLLER_PROTOCOL_SAS) ||
                 (unique_id == LAPETUS_12G_SAS_REV_A) || (unique_id == LAPETUS_12G_SAS_REV_B))
        {
            iom_group = FBE_IOM_GROUP_N;  // these are 12G ports
        }
        else
        {
            /* This code assumes all SAS ports are BE on the mezzanine */
            switch(controller_protocol)
            {
            case IO_CONTROLLER_PROTOCOL_SAS:
                iom_group = FBE_IOM_GROUP_C;
                break;
            case IO_CONTROLLER_PROTOCOL_FIBRE:
              if ((unique_id >= OBERON_SP_85W_REV_A && unique_id <= OBERON_SP_E5_2609V3_REV_B) ||  unique_id ==OBERON_SP_E5_2660V3_REV_B)
              {
                iom_group = FBE_IOM_GROUP_M;
              }
              else
              {
                iom_group = FBE_IOM_GROUP_D;
              }
                break;
            #ifdef C4_INTEGRATED
            
            /*  This is VNXe KH Beachcomber or Sentry HW, mezzanine iscsi ports
             */
            case IO_CONTROLLER_PROTOCOL_ISCSI:
                switch (module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
                {
                case BEACHCOMBER_PCB:
                case BEACHCOMBER_PCB_REV_B:
                case BEACHCOMBER_PCB_REV_B_CPU_6_CORE:
                case BEACHCOMBER_PCB_REV_B_BCM57840_16GB:
                case BEACHCOMBER_PCB_REV_B_BCM57840_24GB:
                case BEACHCOMBER_PCB_REV_C_4C:
                case BEACHCOMBER_PCB_REV_C_6C:
                case BEACHCOMBER_PCB_REV_D_4C:
                case BEACHCOMBER_PCB_REV_D_6C:
                case BEACHCOMBER_PCB_FC_2C:
                case BEACHCOMBER_PCB_SIM:
                case OBERON_SP_85W_REV_A:
                case OBERON_SP_105W_REV_A:
                case OBERON_SP_120W_REV_A:
                case OBERON_SP_E5_2603V3_REV_B:
                case OBERON_SP_E5_2630V3_REV_B:
                case OBERON_SP_E5_2609V3_REV_B:
                case OBERON_SP_E5_2660V3_REV_B:
                case MERIDIAN_SP_ESX:
                case TUNGSTEN_SP:
                        iom_group = FBE_IOM_GROUP_E;
                    break;
                default : 
                    iom_group = FBE_IOM_GROUP_G;
                    break;
                }
                break;
            #endif /* C4_INTEGRATED - C4HW */
            case IO_CONTROLLER_PROTOCOL_AGNOSTIC:
                /*
                 * The protocol for these ports are determined by the SFP type.
                 */
                iom_group = fbe_module_mgmt_derive_iom_group_from_sfp(module_mgmt, sp_id, iom_num, port_num);
                break;
            
            default:
                iom_group = FBE_IOM_GROUP_UNKNOWN;
                break;
            }            
        }
    }
    else
    {
        int i;
        for (i = 0; i < fbe_slic_type_property_map_size; i++)
        {
            if (fbe_slic_type_property_map[i].slic_type == slic_type)
            {
                iom_group = fbe_slic_type_property_map[i].group;
                break;
            }
        }
    }

    return iom_group;
}
/******************************************************
 * end fbe_module_mgmt_derive_iom_group() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_derive_iom_group_from_sfp()
 ****************************************************************
 * @brief
 *  This function derives an io module group for the specified
 *  port information based on the SFP type in the port.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 * @param - iom_group - io module group
 *
 * @return - io module group
 *
 * @author
 *  05-Jan-2015:  Created. bphilbin 
 *
 ****************************************************************/
fbe_iom_group_t fbe_module_mgmt_derive_iom_group_from_sfp(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID sp_id, fbe_u8_t iom_index, 
                                                 fbe_u8_t port_num)
{
    fbe_u32_t port_index = 0;
    fbe_u32_t supported_speeds = 0;
    fbe_u32_t compliance_code = 0;
    fbe_u32_t max_bit_rate = 0;
    fbe_u32_t fc_support_speeds = 0;
    fbe_u32_t first_cna_port_iom_index, first_cna_port_port_num;
    fbe_iom_group_t first_cna_port_iom_group;
    fbe_iom_group_t current_port_iom_group = FBE_IOM_GROUP_UNKNOWN;


    if(module_mgmt->local_sp != sp_id)
    {
        // this is only valid for local ports
        return FBE_IOM_GROUP_UNKNOWN;
    
    }
    port_index = fbe_module_mgmt_get_io_port_index(iom_index, port_num);
    
    if(module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.inserted == FBE_FALSE)
    {
        // no sfp inserted for this port, cannot determine the iom_group
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s SP:%d IOM:%d Port:%d SFP not inserted\n",
                                  __FUNCTION__, sp_id, iom_index, port_num);
        return FBE_IOM_GROUP_UNKNOWN;
    }

    compliance_code = decodeComplianceCode(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);
    max_bit_rate = decodeMaxBitRate(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);
    fc_support_speeds = getFibreChannelSpeeds(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s SP:%d IOM:%d Port:%d comp_code:%d max_br:%d supp_spd:%d type:%d\n",
                          __FUNCTION__, sp_id, iom_index, port_num, compliance_code, max_bit_rate, fc_support_speeds, 
                          module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    supported_speeds = getSupportedSpeeds(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                          module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    /*
     * We determine the sfp type based on the capable speeds.  Basically if it supports 10Gbps then it is iSCSI, else it is FC.
     */
    if(supported_speeds == SPEED_TEN_GIGABIT)
    {
        current_port_iom_group = FBE_IOM_GROUP_P;
    }
    else
    {
        current_port_iom_group = FBE_IOM_GROUP_M;
    }
    
    //Get the first CNA port iom_group, other CNA port must be the same with it.
    if(fbe_module_mgmt_get_first_cna_port(module_mgmt, &first_cna_port_iom_index, &first_cna_port_port_num) != FBE_STATUS_OK)
    {
        // no cna ports found, that should not happened
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s could not find first CNA port.\n",
                                  __FUNCTION__);
        return FBE_IOM_GROUP_UNKNOWN;
    }

    first_cna_port_iom_group =  fbe_module_mgmt_get_iom_group(module_mgmt, module_mgmt->local_sp, first_cna_port_iom_index, first_cna_port_port_num);

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s first CNA port iom_num: %d, port_num: %d, iom_group: %d\n",
                              __FUNCTION__, first_cna_port_iom_index, first_cna_port_port_num, first_cna_port_iom_group);
    // If current port is the first CNA port, then set it's iom_group by SFP type.
    // For other CNA ports, if the SFP type is not the same as first CNA port, do not
    // set iom_group for it.
    if((first_cna_port_iom_index == iom_index && first_cna_port_port_num == port_num) ||
        (current_port_iom_group == first_cna_port_iom_group))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Setting SP:%d IOM:%d Port:%d to %d based on SFP supp_spd:%d\n",
                                  __FUNCTION__, sp_id, iom_index, port_num, current_port_iom_group, supported_speeds);
        return current_port_iom_group;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Skip setting SP:%d IOM:%d Port:%d iom_group due to SFP type not match.\n",
                                  __FUNCTION__, sp_id, iom_index, port_num);
        return FBE_IOM_GROUP_UNKNOWN;
    }
    
}

/*!**************************************************************
 * fbe_module_mgmt_derive_sfp_protocols()
 ****************************************************************
 * @brief
 *  This function derives the bitmask of supported protocols for the
 *  specified SFP.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 * @param - iom_group - io module group
 *
 * @return - fbe_sfp_protocol_t - bitmask of supported protocols
 *
 * @author
 *  31-July-2015:  Created. bphilbin 
 *
 ****************************************************************/
fbe_sfp_protocol_t fbe_module_mgmt_derive_sfp_protocols(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID sp_id, fbe_u8_t iom_index, 
                                                 fbe_u8_t port_num)
{
    fbe_u32_t port_index = 0;
    fbe_u32_t supported_speeds = 0;
    fbe_u32_t compliance_code = 0;
    fbe_u32_t max_bit_rate = 0;
    fbe_u32_t fc_support_speeds = 0;


    if(module_mgmt->local_sp != sp_id)
    {
        // this is only valid for local ports
        return FBE_IOM_GROUP_UNKNOWN;
    
    }
    port_index = fbe_module_mgmt_get_io_port_index(iom_index, port_num);
    
    if(module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.inserted == FBE_FALSE)
    {
        // no sfp inserted for this port, cannot determine the iom_group
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s SP:%d IOM:%d Port:%d SFP not inserted\n",
                                  __FUNCTION__, sp_id, iom_index, port_num);
        return FBE_IOM_GROUP_UNKNOWN;
    }

    compliance_code = decodeComplianceCode(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);
    max_bit_rate = decodeMaxBitRate(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);
    fc_support_speeds = getFibreChannelSpeeds(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s SP:%d IOM:%d Port:%d comp_code:%d max_br:%d supp_spd:%d type:%d\n",
                          __FUNCTION__, sp_id, iom_index, port_num, compliance_code, max_bit_rate, fc_support_speeds, 
                          module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    supported_speeds = getSupportedSpeeds(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                          module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    /*
     * We determine the sfp protocol based on the capable speeds.  Basically if it supports 10Gbps then it is iSCSI, 16, 8 or 4 is FC
     */
    if(supported_speeds == SPEED_TEN_GIGABIT)
    {
        return FBE_SFP_PROTOCOL_ISCSI;
    }
    else if ( (supported_speeds & SPEED_SIXTEEN_GIGABIT) ||
              (supported_speeds & SPEED_EIGHT_GIGABIT) ||
              (supported_speeds & SPEED_FOUR_GIGABIT))
    {
        return FBE_SFP_PROTOCOL_FC;
    }
    else
    {
        return FBE_SFP_PROTOCOL_UNKNOWN;
    }
    
}

/*!**************************************************************
 * fbe_module_mgmt_check_all_module_and_port_states()
 ****************************************************************
 * @brief
 *  This function checks all io module and port states and substates.
 * 
 * @param - module_mgmt - context
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_check_all_module_and_port_states(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t iom_num, port_num, sp_id = 0;
    fbe_u32_t spCount=SP_ID_MAX;
//    fbe_u32_t adjusted_iom_num;

    if (module_mgmt->base_environment.isSingleSpSystem == TRUE)
    {
        spCount=SP_COUNT_SINGLE;
    }
    else
    {
        spCount=SP_ID_MAX;
    }

    for(sp_id=0; sp_id < spCount; sp_id++)
    {
        for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
         {
//            adjusted_iom_num = 
//                fbe_module_mgmt_device_type_and_slot_to_iom_num(
//                    module_mgmt->io_module_info[iom_num].physical_info[sp_id].type, 
//                    iom_num);
//            fbe_module_mgmt_check_module_state(module_mgmt, sp_id, adjusted_iom_num);
            fbe_module_mgmt_check_module_state(module_mgmt, sp_id, iom_num);
            for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
            {
                if(port_num != INVALID_PORT_U8)
                {
                    fbe_module_mgmt_check_port_state(module_mgmt,sp_id,iom_num,port_num);
                }
            }
        }
    }
    return;
}


/*!**************************************************************
 * fbe_module_mgmt_check_module_state()
 ****************************************************************
 * @brief
 *  This function checks the module and port logical and physical information
 *  and derives and sets a logical state and substate for the module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_check_module_state(fbe_module_mgmt_t *module_mgmt, 
                                   SP_ID sp_id, fbe_u8_t iom_num)
{
    fbe_module_state_t state, prev_state;
    fbe_module_substate_t substate;
    fbe_u32_t log_event = 0;
    fbe_char_t expected_slic_type_string[FBE_MODULE_MGMT_SLIC_TYPE_STRING_LENGTH];
    fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_status_t    status;
    fbe_device_physical_location_t pLocation = {0};
    fbe_u64_t device_type;
    fbe_u32_t slot;

    if(module_mgmt->discovering_hardware == FBE_TRUE)
    {
        // We are still in the process of discovering the hardware, do not derive state information yet.
        return;
    }


    fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
    pLocation.sp = sp_id;
    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, sp_id, iom_num, &slot, &device_type);
    pLocation.slot = slot;
    /* Get the device string */
    status = fbe_base_env_create_device_string(device_type, 
                                               &pLocation, 
                                               &(deviceStr[0]), 
                                               FBE_DEVICE_STRING_LENGTH);
    prev_state = fbe_module_mgmt_get_module_state(module_mgmt,sp_id,iom_num);

    if(fbe_module_mgmt_is_iom_initialized(module_mgmt, sp_id, iom_num))
    {
        if(!fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num))
        {
            state = MODULE_STATE_MISSING;
            substate = MODULE_SUBSTATE_MISSING;
            log_event = ESP_INFO_IO_MODULE_REMOVED;
        }
        else if(!fbe_module_mgmt_is_iom_type_ok(module_mgmt, sp_id, iom_num))
        {
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_INCORRECT_MODULE;
            log_event = ESP_ERROR_IO_MODULE_INCORRECT;
        }
        else if(fbe_module_mgmt_get_iom_power_status(module_mgmt, sp_id, iom_num) == FBE_POWER_STATUS_POWER_OFF)
        {
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_POWERED_OFF;
            log_event = ESP_ERROR_IO_MODULE_FAULTED;
        }
        else if(fbe_module_mgmt_get_iom_power_status(module_mgmt, sp_id, iom_num) == FBE_POWER_STATUS_POWERUP_FAILED)
        {
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_POWERUP_FAILED;
            log_event = ESP_ERROR_IO_MODULE_FAULTED;
        }
        else if(fbe_module_mgmt_get_internal_fan_fault(module_mgmt, sp_id, iom_num) == TRUE)
        {
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_INTERNAL_FAN_FAULTED;
            log_event = ESP_ERROR_IO_MODULE_FAULTED;
        }
        else if (fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num) == FBE_SLIC_TYPE_UNSUPPORTED)
        {            
            state = MODULE_STATE_UNSUPPORTED_MODULE;
            substate = MODULE_SUBSTATE_UNSUPPORTED_MODULE;
            // make sure the unsupported slic has power disabled.
            fbe_module_mgmt_check_and_power_down_slic(module_mgmt, sp_id, iom_num);
        }
        else if (!fbe_module_mgmt_is_iom_supported(module_mgmt, sp_id, iom_num))
        {
            // IO module is not supported in current release
            state = MODULE_STATE_UNSUPPORTED_MODULE;
            substate = MODULE_SUBSTATE_UNSUPPORTED_NOT_COMMITTED;
            log_event = ESP_ERROR_IO_MODULE_INCORRECT;
        }
        else if(fbe_module_mgmt_is_iom_hwErrMonFault(module_mgmt, sp_id, iom_num))
        {
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_HW_ERR_MON_FAULT;
            log_event = ESP_ERROR_IO_MODULE_FAULTED;
        }
        else
        {
            state = MODULE_STATE_ENABLED;
            substate = MODULE_SUBSTATE_GOOD;
            log_event = ESP_INFO_IO_MODULE_ENABLED;
        }
    }
    else // port is not initialized
    {
        if(!fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num))
        {
            state = MODULE_STATE_EMPTY;
            substate = MODULE_SUBSTATE_NOT_PRESENT;
        }
        else if (fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num) == FBE_SLIC_TYPE_UNSUPPORTED)
        {            
            state = MODULE_STATE_UNSUPPORTED_MODULE;
            substate = MODULE_SUBSTATE_UNSUPPORTED_MODULE;
            // make sure unsupported slic has power disabled.
            fbe_module_mgmt_check_and_power_down_slic(module_mgmt, sp_id, iom_num);
            log_event = ESP_ERROR_IO_MODULE_UNSUPPORT;
        }
        else if(fbe_module_mgmt_get_iom_power_status(module_mgmt, sp_id, iom_num) == FBE_POWER_STATUS_POWERUP_FAILED)
        {
            /* 
             * The IO module tried to power up but failed. We need to 
             * indicate the IO module has a hardware fault and log a message(DIMS 181509). 
             */
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_POWERUP_FAILED;
            log_event = ESP_ERROR_IO_MODULE_FAULTED;
        }
        else if(fbe_module_mgmt_get_internal_fan_fault(module_mgmt, sp_id, iom_num) == TRUE)
        {
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_INTERNAL_FAN_FAULTED;
            log_event = ESP_ERROR_IO_MODULE_FAULTED;
        }
        else if (!fbe_module_mgmt_is_iom_supported(module_mgmt, sp_id, iom_num))
        {
            // IO module is not supported in this release
            state = MODULE_STATE_UNSUPPORTED_MODULE;
            substate = MODULE_SUBSTATE_UNSUPPORTED_NOT_COMMITTED;
            log_event = ESP_ERROR_IO_MODULE_UNSUPPORT;
        }
        else if(fbe_module_mgmt_is_iom_hwErrMonFault(module_mgmt, sp_id, iom_num))
        {
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_HW_ERR_MON_FAULT;
            log_event = ESP_ERROR_IO_MODULE_FAULTED;
        }
        else if(fbe_module_mgmt_is_iom_over_limit(module_mgmt, sp_id, iom_num))
        {
            // the logging and state change is all handled inside the function called above
            state = MODULE_STATE_FAULTED;
            substate = MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS;
        }
        else
        {
            state = MODULE_STATE_ENABLED;
            substate = MODULE_SUBSTATE_GOOD;
        }
    }
    /* Log event if detect IO module failed in fault register */
    if(fbe_module_mgmt_is_iom_peerboot_fault(module_mgmt, sp_id, iom_num))
    {
        state = MODULE_STATE_FAULTED;
        substate = MODULE_SUBSTATE_FAULT_REG_FAILED;
        /* IO module's FaultReg fault is logged in fault register state change process */
        log_event = 0;
    }

    if( (state != fbe_module_mgmt_get_module_state(module_mgmt, sp_id, iom_num)) ||
        (substate != fbe_module_mgmt_get_module_substate(module_mgmt, sp_id, iom_num)))
    {
        /*
         * The state or substate has changed, log an event if necessary and trace
         * the change.
         */
        fbe_module_mgmt_set_module_state(module_mgmt, sp_id, iom_num, state);
        fbe_module_mgmt_set_module_substate(module_mgmt, sp_id, iom_num, substate);
        
        /* Log an event. */
        switch (log_event)
        {
        case ESP_INFO_IO_MODULE_REMOVED:
        case ESP_INFO_IO_MODULE_ENABLED:
        case ESP_ERROR_IO_MODULE_FAULTED:
            fbe_event_log_write(log_event, 
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0],
                                fbe_module_mgmt_module_substate_to_string(fbe_module_mgmt_get_module_substate(module_mgmt, sp_id, iom_num)));
            break;
        case ESP_ERROR_IO_MODULE_INCORRECT:
            if(module_mgmt->io_module_info[iom_num].physical_info[sp_id].type == FBE_DEVICE_TYPE_BACK_END_MODULE)
            {
                fbe_copy_memory(expected_slic_type_string, "2 Port 6G SAS Base Module", 26);
            }
            else
            {
                fbe_module_mgmt_generate_slic_list_string(fbe_module_mgmt_get_expected_slic_type(module_mgmt,sp_id,iom_num), expected_slic_type_string);
            }
            fbe_event_log_write(log_event, 
                                NULL, 0, 
                                "%s %s %s",
                                &deviceStr[0],
                                fbe_module_mgmt_slic_type_to_string(fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num)), 
                                expected_slic_type_string);
            break;
        case ESP_ERROR_IO_MODULE_UNSUPPORT:
            fbe_event_log_write(log_event,
                                NULL, 0,
                                "%s",
                                &deviceStr[0]);
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Setting %s state:%s, substate:%s\n",
                                  __FUNCTION__, &deviceStr[0],  
                                  fbe_module_mgmt_module_state_to_string(state), 
                                  fbe_module_mgmt_module_substate_to_string(substate));
            break;
        }

        // Update Module Fault LED
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, state:%s, substate:%s\n",
                              __FUNCTION__, &deviceStr[0],
                              fbe_module_mgmt_module_state_to_string(state), 
                              fbe_module_mgmt_module_substate_to_string(substate));
        if((sp_id == module_mgmt->local_sp) &&
           ((module_mgmt->io_module_info[iom_num].physical_info[sp_id].type == FBE_DEVICE_TYPE_IOMODULE)
           ||(module_mgmt->io_module_info[iom_num].physical_info[sp_id].type == FBE_DEVICE_TYPE_BACK_END_MODULE)))
        {
            if (state == MODULE_STATE_FAULTED)
            {
                fbe_module_mgmt_faultIoModuleLed(module_mgmt, iom_num, TRUE);
            }
            else if (state == MODULE_STATE_ENABLED)
            {
                fbe_module_mgmt_faultIoModuleLed(module_mgmt, iom_num, FALSE);
            }
        }

    }

    if( ((prev_state == MODULE_STATE_FAULTED) &&
        (state != MODULE_STATE_FAULTED)) ||
        ((prev_state != MODULE_STATE_FAULTED) &&
         (state == MODULE_STATE_FAULTED)))
    {

        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "ModMgmt: updating EnclFaultLed for %s.\n",
                                  &deviceStr[0]);
        /*
         * Check if Enclosure Fault LED needs updating
         */
        status = fbe_module_mgmt_update_encl_fault_led(module_mgmt, &pLocation, device_type, FBE_ENCL_FAULT_LED_IO_MODULE_FLT);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "ModMgmt: error updating EnclFaultLed for %s status 0x%X.\n",
                                  &deviceStr[0], status);
        }
    }
    return;
}

/*!**************************************************************
 * fbe_module_mgmt_port_sfp_faulted()
 ****************************************************************
 * @brief
 *  This function returns if we could set IO Port LED.
 *
 * @param - port_state    - Port state
 * @param - port_substate - Port sub-state
 * 
 * @return - fbe_bool_t   - Port SFP faulted or not
 * 
 * @author
 *  23-Jul-2014:  Created. Tommy Miao
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_port_sfp_faulted(fbe_port_state_t port_state, fbe_port_substate_t port_substate)
{
     if (port_state != FBE_PORT_STATE_FAULTED)
     {
         return false;
     }

     return (port_substate == FBE_PORT_SUBSTATE_FAULTED_SFP
         ||  port_substate == FBE_PORT_SUBSTATE_UNSUPPORTED_SFP
         ||  port_substate == FBE_PORT_SUBSTATE_SFP_READ_ERROR);
}

/*!**************************************************************
 * fbe_module_mgmt_check_port_state()
 ****************************************************************
 * @brief
 *  This function checks the port logical and physical information
 *  and derives and sets a logical state and substate for the port.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
 void fbe_module_mgmt_check_port_state(fbe_module_mgmt_t *module_mgmt, 
                                            SP_ID sp_id, fbe_u8_t iom_num, 
                                            fbe_u8_t port_num)
 {
    fbe_u32_t log_event = 0;
    fbe_port_state_t newstate, prevstate;
    fbe_port_substate_t newsubstate, prevsubstate;
    fbe_port_sfp_condition_type_t sfp_condition;
    fbe_port_sfp_sub_condition_type_t sfp_sub_condition;
    fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_status_t    status;
    fbe_device_physical_location_t pLocation = {0};
    fbe_u64_t device_type;
    fbe_u32_t slot;

    if(module_mgmt->discovering_hardware == FBE_TRUE)
    {
        // We are still in the process of discovering the hardware, do not derive state information yet.
        return;
    }


    fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
    pLocation.sp = sp_id;
    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, sp_id, iom_num, &slot, &device_type);
    pLocation.slot = slot;
    /* Get the device string */
    status = fbe_base_env_create_device_string(device_type, 
                                               &pLocation, 
                                               &(deviceStr[0]), 
                                               FBE_DEVICE_STRING_LENGTH);

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: CheckPortstate for %s Port %d\n",
                              &deviceStr[0], port_num);
    prevstate = fbe_module_mgmt_get_port_state(module_mgmt,sp_id,iom_num,port_num);
    prevsubstate = fbe_module_mgmt_get_port_substate(module_mgmt,sp_id,iom_num,port_num);


    sfp_condition = fbe_module_mgmt_get_port_sfp_condition(module_mgmt,sp_id,iom_num,port_num);
    sfp_sub_condition = fbe_module_mgmt_get_port_sfp_subcondition(module_mgmt,sp_id,iom_num,port_num);

     if(fbe_module_mgmt_is_port_initialized(module_mgmt,sp_id,iom_num,port_num))
     {
        if(!fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num))
        {
            newstate = FBE_PORT_STATE_MISSING;
            newsubstate = FBE_PORT_SUBSTATE_MISSING_MODULE;
        }
        else if(!fbe_module_mgmt_is_iom_type_ok(module_mgmt, sp_id, iom_num))
        {
            newstate = FBE_PORT_STATE_FAULTED;
            newsubstate = FBE_PORT_SUBSTATE_INCORRECT_MODULE;
            log_event = ESP_ERROR_IO_PORT_FAULTED;
        }
        else if(fbe_module_mgmt_get_port_sfp_capable(module_mgmt,sp_id,iom_num,port_num) != FBE_MGMT_STATUS_TRUE)
        {
            newstate = FBE_PORT_STATE_ENABLED;
            newsubstate = FBE_PORT_SUBSTATE_GOOD;
            log_event = ESP_INFO_IO_PORT_ENABLED;
        }
        else if(!fbe_module_mgmt_is_iom_power_good(module_mgmt,sp_id,iom_num))
        {
            newstate = FBE_PORT_STATE_UNKNOWN;
            newsubstate = FBE_PORT_SUBSTATE_MODULE_POWERED_OFF;
        }

        /* Add SFP Fault handling */
        else if(sfp_condition == FBE_PORT_SFP_CONDITION_FAULT)
        {
            newstate = FBE_PORT_STATE_FAULTED;
            // Need to check for incorrect sfp type and unsupported sfp
            // type when sfp_condition==CPD_SFP_CONDITION_FAULT.
            
            if(sfp_sub_condition == FBE_PORT_SFP_SUBCONDITION_UNSUPPORTED)
            {
                newsubstate = FBE_PORT_SUBSTATE_UNSUPPORTED_SFP;
            }
            else if(sfp_sub_condition == FBE_PORT_SFP_SUBCONDITION_DEVICE_ERROR)
            {
                newsubstate = FBE_PORT_SUBSTATE_SFP_READ_ERROR;
            }
            else
            {
                newsubstate = FBE_PORT_SUBSTATE_FAULTED_SFP;
            }

            log_event = ESP_ERROR_IO_PORT_FAULTED;

            // fault the port
            if (fbe_module_mgmt_could_set_io_port_led(module_mgmt, iom_num, sp_id))
            {
                fbe_module_mgmt_faultIoPortLed(module_mgmt, iom_num, port_num);
            }
        }
        else if(sfp_condition == FBE_PORT_SFP_CONDITION_REMOVED)
        {
            newstate = FBE_PORT_STATE_MISSING;
            newsubstate = FBE_PORT_SUBSTATE_MISSING_SFP;
        }
        else if(fbe_module_mgmt_check_combined_port_fault(module_mgmt,sp_id,iom_num,port_num))
        {
            newstate = FBE_PORT_STATE_FAULTED;
            newsubstate = FBE_PORT_SUBSTATE_FAULTED_SFP;
        }
        else
        {
            newstate = FBE_PORT_STATE_ENABLED;
            newsubstate = FBE_PORT_SUBSTATE_GOOD;
            log_event = ESP_INFO_IO_PORT_ENABLED;
        }
     }
     else
     {
    
        /* For uninitialized ports 
         *  Must derive the port role and protocol information
         *  it is used by subordinate functions so it needs to be 
         *  initialized
         */

        fbe_module_mgmt_derive_port_role(module_mgmt, sp_id, iom_num, port_num);

        if(!fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num))
        {
            newstate = FBE_PORT_STATE_UNKNOWN;
            newsubstate = FBE_PORT_SUBSTATE_MODULE_NOT_PRESENT;
        }
        else if(fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num) == FBE_SLIC_TYPE_UNKNOWN)
        {
            newstate = FBE_PORT_STATE_UNKNOWN;
            newsubstate = FBE_PORT_SUBSTATE_MODULE_READ_ERROR;
        }
        else if(fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num) == FBE_SLIC_TYPE_UNSUPPORTED)
        {
            newstate = FBE_PORT_STATE_UNKNOWN;
            newsubstate = FBE_PORT_SUBSTATE_UNSUPPORTED_MODULE;
        }
        else if(sfp_condition == FBE_PORT_SFP_CONDITION_FAULT)
        {
            newstate = FBE_PORT_STATE_FAULTED;
            // Need to check for incorrect sfp type and unsupported sfp
            // type when sfp_condition==CPD_SFP_CONDITION_FAULT.

            if(sfp_sub_condition == FBE_PORT_SFP_SUBCONDITION_UNSUPPORTED)
            {
                newsubstate = FBE_PORT_SUBSTATE_UNSUPPORTED_SFP;
            }
            else if(sfp_sub_condition == FBE_PORT_SFP_SUBCONDITION_DEVICE_ERROR)
            {
                newsubstate = FBE_PORT_SUBSTATE_SFP_READ_ERROR;
            }
            else
            {
                newsubstate = FBE_PORT_SUBSTATE_FAULTED_SFP;
            }

            log_event = ESP_ERROR_IO_PORT_FAULTED;

            if (fbe_module_mgmt_could_set_io_port_led(module_mgmt, iom_num, sp_id))
            {
                fbe_module_mgmt_faultIoPortLed(module_mgmt, iom_num, port_num);
            }
        }
        else if(fbe_module_mgmt_check_combined_port_fault(module_mgmt,sp_id,iom_num,port_num))
        {
            newstate = FBE_PORT_STATE_FAULTED;
            newsubstate = FBE_PORT_SUBSTATE_FAULTED_SFP;
        }         
#if 0  // maybe we check here if we can map the port object to the board object data?
        else if((NTBEConvertFlarePort2SCSIPort(IOMPORT2PORTADDR(iom_num, port_num)) == INVALID_PORT_NUMBER) &&
                (port_num < IOM_INFO_NUM_PORTS(LOCAL_IOM_INFO,iom_num))  &&
                (IOM_INFO_PWR_STATUS(LOCAL_IOM_INFO, iom_num) == IOM_POWER_STATUS_POWER_ON))
        {
            port_config_info->state = PORT_STATE_FAULTED;
            port_config_info->substate = PORT_SUBSTATE_HW_FAULT;
            // we need not log event as this port is uninitialized.
        }
#endif
        else if (!fbe_module_mgmt_check_cna_ports_match(module_mgmt, sp_id, iom_num, port_num))
        {
            newstate = FBE_PORT_STATE_FAULTED;
            newsubstate = FBE_PORT_SUBSTATE_INCORRECT_SFP_TYPE;
        }
        else
        {
            /*
             * This is required as the initial state and substate of each port
             * is PORT_STATE_UNKNOWN & PORT_SUBSTATE_IOM_NOT_PRESENT respectively.
             */
            fbe_module_mgmt_set_port_state(module_mgmt, sp_id, iom_num, port_num, FBE_PORT_STATE_UNINITIALIZED);
            fbe_module_mgmt_set_port_substate(module_mgmt, sp_id, iom_num, port_num, FBE_PORT_SUBSTATE_UNINITIALIZED);

            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Setting %s, Port %d, UNINITIALIZED before checking limits.\n",
                                __FUNCTION__, &deviceStr[0], port_num);


            /* 
             * In the case where the port looks ok we check to see if it exceeds
             * maximum configurable limits, if it does we will fault it and set
             * substate to EXCEEDS_MAX, if it does not then we set it to UNINITIALIZED.
             */
            fbe_module_mgmt_check_platform_limits(module_mgmt, sp_id, iom_num, port_num);
            newstate = fbe_module_mgmt_get_port_state(module_mgmt, sp_id, iom_num, port_num);
            newsubstate = fbe_module_mgmt_get_port_substate(module_mgmt, sp_id, iom_num, port_num);
            log_event = FBE_FALSE;

        }
     }
     if( ((fbe_module_mgmt_get_port_state(module_mgmt, sp_id, iom_num, port_num) != newstate) ||
          (fbe_module_mgmt_get_port_substate(module_mgmt, sp_id, iom_num, port_num) != newsubstate)) &&
         // Do not log a bunch of IOM_NP traces when we first boot up for removed IO modules.
          (!( (newstate == FBE_PORT_STATE_UNKNOWN) && 
             (fbe_module_mgmt_get_port_state(module_mgmt, sp_id, iom_num, port_num) == FBE_PORT_STATE_UNINITIALIZED) &&
             (newsubstate == FBE_PORT_SUBSTATE_MODULE_NOT_PRESENT)  &&
             (fbe_module_mgmt_get_port_substate(module_mgmt, sp_id, iom_num, port_num) == FBE_PORT_SUBSTATE_UNINITIALIZED)) ))
     {
         fbe_module_mgmt_set_port_state(module_mgmt, sp_id, iom_num, port_num, newstate);
         fbe_module_mgmt_set_port_substate(module_mgmt, sp_id, iom_num, port_num, newsubstate);
    
         fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Setting %s, Port %d, state %s, substate %s\n",
                                __FUNCTION__, &deviceStr[0], port_num, 
                                fbe_module_mgmt_port_state_to_string(newstate),
                                fbe_module_mgmt_port_substate_to_string(newsubstate));
         if(log_event)
         {
            // log an event
             fbe_event_log_write(log_event,
                                 NULL, 0,
                                 "%s %d %s", 
                                 &deviceStr[0],
                                 port_num,
                                 fbe_module_mgmt_port_substate_to_string(newsubstate));
                    
         }
     }

    if ((fbe_module_mgmt_port_sfp_faulted(prevstate, prevsubstate) && newstate  != FBE_PORT_STATE_FAULTED)
     || (fbe_module_mgmt_port_sfp_faulted(newstate,  newsubstate)  && prevstate != FBE_PORT_STATE_FAULTED))
    {

        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "ModMgmt: updating EnclFaultLed for %s Port:%d.\n",
                                  &deviceStr[0], port_num);
        /*
         * Check if Enclosure Fault LED needs updating
         */
        status = fbe_module_mgmt_update_encl_fault_led(module_mgmt, &pLocation, device_type, FBE_ENCL_FAULT_LED_IO_PORT_FLT);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "ModMgmt: error updating EnclFaultLed for %s Port:%d, status 0x%X.\n",
                                  &deviceStr[0], port_num, status);
        }
    }

    /* if SFP fault is cleared on the port, we need to reset the port LED
     */
    if (fbe_module_mgmt_port_sfp_faulted(prevstate, prevsubstate) && newstate != FBE_PORT_STATE_FAULTED)
    {
        if (fbe_module_mgmt_could_set_io_port_led(module_mgmt, iom_num, sp_id))
        {
            status = fbe_module_mgmt_setIoPortLedBasedOnLink(module_mgmt, iom_num, port_num);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, IoPortLed failed, Sp %d, mod %d, port %d, status 0x%x\n",
                                      __FUNCTION__,
                                      module_mgmt->local_sp,
                                      iom_num,
                                      port_num,
                                      status);
            }
        }
    }

    return;
 }

/*!**************************************************************
 * fbe_module_mgmt_is_iom_over_limit()
 ****************************************************************
 * @brief
 *  This function checks the specified io module exceeds the limits
 *  for this platform.  This is only applicable to Jetfire M4 or M5
 *  systems with Octane power supplies.
 * 
 * @param - none
 *
 * @return - TRUE - if IO module exceeds the platform limit.
 *
 * @author
 *  30-October-2013:  Created. bphilbin 
 *
 ****************************************************************/
 fbe_bool_t fbe_module_mgmt_is_iom_over_limit(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num)
 {
     fbe_u32_t configured_slic_count = 0;
     fbe_u32_t unconfigured_slic_count = 0;
     fbe_u32_t temp_iom_num;
     fbe_bool_t iom_over_limit = FBE_FALSE;
     fbe_u32_t slot;
     fbe_u64_t device_type;

     fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
     fbe_status_t    status;
     fbe_device_physical_location_t pLocation = {0};

     

     if(!fbe_module_mgmt_is_jetfire_with_octane_system(module_mgmt))
     {
         // This slic limit check is only for jetfire systems running with octane power supplies.
         return FALSE;
     }

     for(temp_iom_num = 0; temp_iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; temp_iom_num++)
     {
         if((fbe_module_mgmt_get_slic_type(module_mgmt,sp_id,temp_iom_num) != FBE_SLIC_TYPE_NA) &&
            (fbe_module_mgmt_is_iom_initialized(module_mgmt,sp_id,temp_iom_num)))
         {
             configured_slic_count++;
         }
     }

     fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ModMgmt: Configured SLIC count %d.\n",
                                      configured_slic_count);

     for(temp_iom_num = 0; temp_iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; temp_iom_num++)
     {
         if( (fbe_module_mgmt_get_slic_type(module_mgmt,sp_id,temp_iom_num) != FBE_SLIC_TYPE_NA) &&
             (fbe_module_mgmt_is_iom_inserted(module_mgmt,sp_id,temp_iom_num)) &&
             (!fbe_module_mgmt_is_iom_initialized(module_mgmt,sp_id,temp_iom_num)))
         {
             unconfigured_slic_count++;
             fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ModMgmt: IOM %d Configured SLIC count %d Unconfigured SLIC count %d, limit %d.\n",
                                      temp_iom_num, configured_slic_count, unconfigured_slic_count, module_mgmt->platform_hw_limit.max_slics);
             fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
             pLocation.sp = sp_id;
             fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, sp_id, temp_iom_num, &slot, &device_type);
             pLocation.slot = slot;
             /* Get the device string */
             status = fbe_base_env_create_device_string(device_type, 
                                                       &pLocation, 
                                                       &(deviceStr[0]), 
                                                       FBE_DEVICE_STRING_LENGTH);

             if((unconfigured_slic_count + configured_slic_count) > module_mgmt->platform_hw_limit.max_slics)
             {
                 // This IO Module exceeds the platform limit
                 if(fbe_module_mgmt_get_module_substate(module_mgmt, sp_id, temp_iom_num) != MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS)
                 {
                     fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ModMgmt: setting IOM %d FAULTED EXCEEDS PLATFORM LIMIT.\n",
                                      temp_iom_num);
                 }
                 if( (fbe_module_mgmt_get_module_state(module_mgmt, sp_id, temp_iom_num) != MODULE_STATE_FAULTED)&&
                      (fbe_module_mgmt_get_module_substate(module_mgmt, sp_id, temp_iom_num) != MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS))
                 {
                     // New fault for this slic, log the event here
                     fbe_event_log_write(ESP_ERROR_IO_MODULE_FAULTED, 
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0],
                                fbe_module_mgmt_module_substate_to_string(MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS));
                 }
                 fbe_module_mgmt_set_module_state(module_mgmt, sp_id, temp_iom_num, MODULE_STATE_FAULTED);
                 fbe_module_mgmt_set_module_substate(module_mgmt, sp_id, temp_iom_num, MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS);

                 

                 fbe_module_mgmt_check_and_power_down_slic(module_mgmt, sp_id, temp_iom_num);
                 
                 if(temp_iom_num == iom_num)
                 {
                     //The specified io module exceeds the platform limit
                     iom_over_limit = FBE_TRUE;
                 }
             }
             else if( (fbe_module_mgmt_get_module_state(module_mgmt, sp_id, temp_iom_num) == MODULE_STATE_FAULTED)&&
                      (fbe_module_mgmt_get_module_substate(module_mgmt, sp_id, temp_iom_num) == MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS))
             {
                 // this slic is no longer faulted, log the event
                 fbe_event_log_write(ESP_INFO_IO_MODULE_ENABLED, 
                            NULL, 0, 
                            "%s %s", 
                            &deviceStr[0],
                            fbe_module_mgmt_module_substate_to_string(MODULE_SUBSTATE_GOOD));

                 // This IO module was faulted the last time we checked, it is no longer faulted change the state
                 fbe_module_mgmt_set_module_state(module_mgmt, sp_id, temp_iom_num, MODULE_STATE_ENABLED);
                 fbe_module_mgmt_set_module_substate(module_mgmt, sp_id, temp_iom_num, MODULE_SUBSTATE_GOOD);

                 if(fbe_module_mgmt_get_iom_power_enabled(module_mgmt, sp_id, temp_iom_num) == FBE_FALSE)
                 {
                     // re-enable power to any SLICs if they are not overlimit
                     fbe_api_board_set_IOModulePersistedPowerState(module_mgmt->board_object_id,
                                                                     sp_id, 
                                                                     slot,
                                                                     device_type,
                                                                     TRUE);
                     // reboot the SP if we have changed the persisted power state of the slic
                     module_mgmt->reboot_sp = REBOOT_LOCAL;
                     fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "ModMgmt: Setting IOM %d, to power on, requesting an SP reboot.\n",
                                            temp_iom_num);
                 }

                 fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ModMgmt: setting IOM %d ENABLED/GOOD it no longer exceeds SLIC limits.\n",
                                      temp_iom_num);
             }
         }
     }
     
     return iom_over_limit;
 }


  /*!**************************************************************
 * fbe_module_mgmt_is_iom_over_limit()
 ****************************************************************
 * @brief
 *  This function checks the system information to see if we are
 *  a Jetfire M4 or M5 platform with Octane power supplies.
 * 
 * @param - none
 *
 * @return - TRUE - Jetfire M4 or M5 with Octane PS.
 *
 * @author
 *  30-October-2013:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_jetfire_with_octane_system(fbe_module_mgmt_t *module_mgmt)
{
    fbe_board_mgmt_platform_info_t boardPlatformInfo;
    fbe_status_t status;
    fbe_u32_t psCount = 0;
    fbe_u32_t slot = 0, temp_slot = 0;
    fbe_power_supply_info_t ps_info = {0};

    status = fbe_api_board_get_platform_info(module_mgmt->board_object_id, 
                                             &boardPlatformInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s error identifying the platform type status 0x%x.\n",
                              __FUNCTION__, status);
    }

    if( (boardPlatformInfo.hw_platform_info.platformType == SPID_DEFIANT_M4_HW_TYPE) ||
        (boardPlatformInfo.hw_platform_info.platformType == SPID_DEFIANT_M5_HW_TYPE) )
    {
        status = fbe_api_power_supply_get_ps_count(module_mgmt->board_object_id, &psCount);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to get ps count status: 0x%X.\n",
                                  __FUNCTION__, status);

            return FBE_FALSE;
        }
        
        /*
         * Here we are looping to find the local power supply, if it is not inserted then we have
         * to use the peer's power supply information to determine the type.  There is no gaurantee
         * the local power supply will be slot 0 or 1 so we have to loop through these and then 
         * possibly back if for instance slot 1 is the local but it is not inserted.
         */
        for(slot = 0; slot < psCount; slot++)
        {
            fbe_set_memory(&ps_info, 0, sizeof(fbe_power_supply_info_t));
            ps_info.slotNumOnEncl = slot;    
            status = fbe_api_power_supply_get_power_supply_info(module_mgmt->board_object_id, &ps_info);

            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to get ps info for slot %d status: 0x%x.\n",
                                      __FUNCTION__, slot, status);
    
                return FBE_FALSE;
            }
            if((ps_info.isLocalFru == FBE_TRUE) &&
               (ps_info.bInserted == FBE_FALSE) &&
               (slot == 0))
            {
                // PS not Inserted here check the peer's PS to determine the type
                fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s skipping PS %d local but not inserted.\n",
                              __FUNCTION__, slot);
                continue;
            }
            else if((ps_info.isLocalFru == FBE_TRUE) &&
               (ps_info.bInserted == FBE_TRUE))
            {
                if(ps_info.uniqueId == AC_ACBEL_OCTANE)
                {
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s PS %d is local and Octane.\n",
                                  __FUNCTION__, slot);

                    return FBE_TRUE;
                }
                else
                {
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s PS %d is local and not Octane.\n",
                                  __FUNCTION__, slot);
                    return FBE_FALSE;
                }
            }
            else if(slot > 0) 
            {
                // Check slot 0 again, it isn't local but the local slot 1 doesn't have a PS
                temp_slot = 0;
                fbe_set_memory(&ps_info, 0, sizeof(fbe_power_supply_info_t));
                ps_info.slotNumOnEncl = temp_slot;    
                status = fbe_api_power_supply_get_power_supply_info(module_mgmt->board_object_id, &ps_info);
    
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s failed to get ps info for slot %d status: 0x%x.\n",
                                          __FUNCTION__, temp_slot, status);
        
                    return FBE_FALSE;
                }
                if(ps_info.uniqueId == AC_ACBEL_OCTANE)
                {
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s PS %d is Peer's PS and Octane.\n",
                                  __FUNCTION__, temp_slot);

                    return FBE_TRUE;
                }
                else
                {
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s PS %d is Peer's PS and not Octane.\n",
                                  __FUNCTION__, temp_slot);
                    return FBE_FALSE;
                }
            }
        }
        
    }
    return FBE_FALSE;

}

/*!**************************************************************
 * fbe_module_mgmt_check_combined_port_fault()
 ****************************************************************
 * @brief
 *  This function checks the specified port to see if it is a combined
 *  port that has possibly been miscabled.  This happens when they 
 *  use the combined port cable in ports 1 and 2 instead of 0 and 1 or
 *  2 and 3.
 * 
 * @param - none
 *
 * @return - TRUE - if combined port is faulted.
 *
 * @author
 *  19-September-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_check_combined_port_fault(fbe_module_mgmt_t *module_mgmt, 
                                                     SP_ID sp_id, fbe_u8_t iom_num, 
                                                     fbe_u8_t port_num)
{
    /*
     * The combined connector must be in ports 0 and 1 or 2 and 3, and not in 1 and 2.
     */
    if(!fbe_module_mgmt_is_port_combined_port(module_mgmt, sp_id, iom_num, 0) &&
       fbe_module_mgmt_is_port_combined_port(module_mgmt, sp_id, iom_num, 1) &&
       fbe_module_mgmt_is_port_combined_port(module_mgmt, sp_id, iom_num, 2) &&
       !fbe_module_mgmt_is_port_combined_port(module_mgmt, sp_id, iom_num, 3) &&
       ((port_num == 1) || (port_num == 2)) )
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Combined port fault detected on SP %d, IOM %d, Port %d\n",
                               __FUNCTION__, sp_id, iom_num, port_num);

        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_get_combined_port_count()
 ****************************************************************
 * @brief
 *  This function returns the combined port count for the specified
 *  io module.
 * 
 * @param - none
 *
 * @return - combined port count
 *
 * @author
 *  19-September-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_combined_port_count(fbe_module_mgmt_t *module_mgmt, 
                                                     SP_ID sp_id, fbe_u8_t iom_num)
{
    fbe_u32_t num_ports = 0;
    fbe_u32_t port_num = 0;
    fbe_u32_t combined_port_count = 0;
    fbe_u32_t secondary_combined_port;

    num_ports = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt,sp_id,iom_num);

    for(port_num = 0; port_num < num_ports; port_num++)
    {
        if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt, sp_id, iom_num, port_num))
        {
            // if this is a combined port and both ports are configured then count them
           secondary_combined_port = fbe_module_mgmt_get_secondary_combined_port(module_mgmt,
                                                                                 sp_id, iom_num, 
                                                                                 port_num);
           if( (fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, port_num)) &&
                (fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, secondary_combined_port)) )
            {
                combined_port_count++;
            }
        }
    }
    return combined_port_count;
}

/*!**************************************************************
 * fbe_module_mgmt_get_secondary_combined_port()
 ****************************************************************
 * @brief
 *  This function returns next port number in a combined port
 *  pairing.
 * 
 * @param - none
 *
 * @return - port number
 *
 * @author
 *  19-September-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_secondary_combined_port(fbe_module_mgmt_t *module_mgmt,
                                                      SP_ID sp_id, fbe_u8_t iom_num,
                                                      fbe_u8_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port;
}

/*!**************************************************************
 * fbe_module_mgmt_get_port_phy_mapping()
 ****************************************************************
 * @brief
 *  This function returns the phy mapping for the specified port
 * 
 * @param - none
 *
 * @return - phy map
 *
 * @author
 *  19-September-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_port_phy_mapping(fbe_module_mgmt_t *module_mgmt,
                                               SP_ID sp_id, fbe_u8_t iom_num,
                                               fbe_u8_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].phyMapping;
}

fbe_u32_t fbe_module_mgmt_get_combined_port_phy_mapping(fbe_module_mgmt_t *module_mgmt,
                                               SP_ID sp_id, fbe_u8_t iom_num,
                                               fbe_u8_t port_num)
{
    fbe_u8_t port_index;
    fbe_u8_t secondary_port_num;
    fbe_u8_t secondary_port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);

    //check if port is config as combined port
    if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt, sp_id, iom_num, port_num))
    {
		//Get secondary combined port, now it is hard coded,
		//we should make a new function to do this later.
		//TODO: write a new function to get combined port by config port info.
		switch(port_num)
		{
			case 0:
				secondary_port_num = 1;
				break;
			case 1:
				secondary_port_num = 0;
				break;
			case 2:
				secondary_port_num = 3;
				break;
			case 3:
				secondary_port_num = 2;
				break;
		}
    }
    else
    {
    	//we do not have a combined port
    	secondary_port_num = INVALID_PORT_U8;
    }

    secondary_port_index = fbe_module_mgmt_get_io_port_index(iom_num, secondary_port_num);

    // combined phy map only applies when a combined port is configured
    if( (fbe_module_mgmt_is_port_initialized(module_mgmt,sp_id,iom_num,port_num)) &&
        (fbe_module_mgmt_is_port_initialized(module_mgmt,sp_id,iom_num,secondary_port_num)))
    {
        return (module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].phyMapping |
                module_mgmt->io_port_info[secondary_port_index].port_physical_info.port_comp_info[sp_id].phyMapping);
    }
    else
    {
        return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].phyMapping;
    }

}

/* This private enum will only be used in the following scsi_slic_type_match function
 */
typedef enum
{
    FBE_ISCSI_SLIC_TYPE_PRIVATE_UNKNOWN,

    FBE_ISCSI_SLIC_TYPE_PRIVATE_1G,
    FBE_ISCSI_SLIC_TYPE_PRIVATE_10G,
} fbe_iscsi_slic_type_private_t;

static fbe_iscsi_slic_type_private_t translate_iscsi_slic_type_to_private(fbe_module_slic_type_t iscsi_slic_type)
{
    switch (iscsi_slic_type)
    {
    case FBE_SLIC_TYPE_ISCSI_1G:
    case FBE_SLIC_TYPE_4PORT_ISCSI_1G:
    case FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G:
        return FBE_ISCSI_SLIC_TYPE_PRIVATE_1G;
    case FBE_SLIC_TYPE_ISCSI_10G:
    case FBE_SLIC_TYPE_ISCSI_COPPER:
    case FBE_SLIC_TYPE_ISCSI_10G_V2:
    case FBE_SLIC_TYPE_4PORT_ISCSI_10G:
    case FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G:
    case FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G:
    case FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G:
        return FBE_ISCSI_SLIC_TYPE_PRIVATE_10G;
    default:
        return FBE_ISCSI_SLIC_TYPE_PRIVATE_UNKNOWN;
    }
}

static fbe_bool_t iscsi_slic_type_match(fbe_module_slic_type_t iscsi_slic_type_1,
                                        fbe_module_slic_type_t iscsi_slic_type_2)
{
    fbe_iscsi_slic_type_private_t iscsi_slic_type_private_1 = translate_iscsi_slic_type_to_private(iscsi_slic_type_1),
                                  iscsi_slic_type_private_2 = translate_iscsi_slic_type_to_private(iscsi_slic_type_2);

    /* both are iSCSI 1G
     */
    if (iscsi_slic_type_private_1 == FBE_ISCSI_SLIC_TYPE_PRIVATE_1G
     && iscsi_slic_type_private_2 == FBE_ISCSI_SLIC_TYPE_PRIVATE_1G)
    {
        return FBE_TRUE;
    }

    /* both are iSCSI 10G
     */
    if (iscsi_slic_type_private_1 == FBE_ISCSI_SLIC_TYPE_PRIVATE_10G
     && iscsi_slic_type_private_2 == FBE_ISCSI_SLIC_TYPE_PRIVATE_10G)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!**************************************************************
 * fbe_module_mgmt_check_platform_limits()
 ****************************************************************
 * @brief
 *  This function checks if the specified slic has power disabled
 *  if not it will disable power to the slic and request a reboot.
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  06-November-2013:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_check_and_power_down_slic(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num)
{
    fbe_u32_t slot;
    fbe_u64_t device_type;
    fbe_status_t status;
    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, sp_id, iom_num, &slot, &device_type);

   
    status = fbe_api_board_set_IOModulePersistedPowerState(module_mgmt->board_object_id, sp_id, 
                                                           slot, device_type, FALSE); 
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "ModMgmt: Failed Setting IOM %d, to power off status 0x%x.\n",
                                iom_num, status);

    }
    else if(fbe_module_mgmt_get_iom_power_enabled(module_mgmt, sp_id, iom_num) == FBE_TRUE)
    {
        
        module_mgmt->reboot_sp = REBOOT_LOCAL;
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "ModMgmt: Setting IOM %d, to power off, requesting an SP reboot.\n",
                                iom_num);
    }
    return;
}

    
/*!**************************************************************
 * fbe_module_mgmt_check_platform_limits()
 ****************************************************************
 * @brief
 *  This function checks the current modules management config
 *  against this platform's limits and faults/unfaults ports
 *  when necessary.
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
 void fbe_module_mgmt_check_platform_limits(fbe_module_mgmt_t *module_mgmt, 
                                            SP_ID sp_id, fbe_u8_t iom_num, 
                                            fbe_u8_t port_num)
 {
     fbe_u8_t temp_iom_num, temp_port_num, uninit_port_count, init_port_count, platform_limit;
     IO_CONTROLLER_PROTOCOL port_protocol;
     fbe_ioport_role_t port_role;
     fbe_iom_group_t iom_group;
     fbe_iom_group_t temp_iom_group;
     fbe_module_slic_type_t slic_type;
     fbe_module_slic_type_t temp_slic_type;
     fbe_environment_limits_platform_port_limits_t platform_port_limits;
    fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_status_t    status;
    fbe_device_physical_location_t pLocation = {0};
    fbe_u64_t device_type;
    fbe_u32_t slot;
    fbe_u32_t port_count;
    fbe_bool_t wrong_slic;

    if(sp_id != module_mgmt->local_sp)
    {
        // not valid for peer SP, we don't know individual protocols per port on the peer
        return;
    }

    port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt, sp_id, iom_num, port_num);
    port_role = fbe_module_mgmt_get_port_role(module_mgmt, sp_id, iom_num, port_num);
    slic_type = fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num);

    platform_limit = fbe_module_mgmt_get_platform_limit(module_mgmt, port_protocol, port_role, slic_type);
    iom_group = fbe_module_mgmt_get_iom_group(module_mgmt, sp_id, iom_num, port_num);

     fbe_set_memory(&platform_port_limits, 0, sizeof(fbe_environment_limits_platform_port_limits_t));
     fbe_environment_limit_get_platform_port_limits(&platform_port_limits);

     port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt, sp_id, iom_num, port_num);
     port_role = fbe_module_mgmt_get_port_role(module_mgmt, sp_id, iom_num, port_num);
     slic_type = fbe_module_mgmt_get_slic_type(module_mgmt, sp_id, iom_num);

     platform_limit = fbe_module_mgmt_get_platform_limit(module_mgmt, port_protocol, port_role, slic_type);
     iom_group = fbe_module_mgmt_get_iom_group(module_mgmt, sp_id, iom_num, port_num);

     // Special check for slic upgrade, if we are doing slic convert,
     // the following check may cause extra slic get power off.
     // So we will skip the limit check for convert ports.
     // Port limits check will be done after slic upgrade finish.
     if(module_mgmt->slic_upgrade == FBE_MODULE_MGMT_SLIC_UPGRADE_LOCAL_SP &&
        ((iom_group == FBE_IOM_GROUP_G) || (iom_group == FBE_IOM_GROUP_E) ||
        (iom_group == FBE_IOM_GROUP_I) || (iom_group == FBE_IOM_GROUP_J)))
     {
         fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "ModMgmt: Slic Upgrade in progress, skip Limits check for SP:%d IOM %d, Port %d Protocl:%s Role:%s\n",
                                sp_id, iom_num, port_num,
                                fbe_module_mgmt_protocol_to_string(port_protocol),
                                fbe_module_mgmt_port_role_to_string(port_role));
         return;
     }

     fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                    "ModMgmt: Check Limits for SP:%d IOM %d, Port %d Protocl:%s Role:%s\n",
                                                    sp_id, iom_num, port_num, 
                                                    fbe_module_mgmt_protocol_to_string(port_protocol),
                                                    fbe_module_mgmt_port_role_to_string(port_role));
    if( (port_protocol == IO_CONTROLLER_PROTOCOL_UNKNOWN) || (port_role == IOPORT_PORT_ROLE_UNINITIALIZED))
    {
        return;
    }

    init_port_count = 0;
    uninit_port_count = 0;

    /* 
     * First loop through all ports to get the initialized port count for what is configured.
     */
     for(temp_iom_num = 0; temp_iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; temp_iom_num++)
     {
         for(temp_port_num = 0; temp_port_num < MAX_IO_PORTS_PER_MODULE; temp_port_num++)
         {
             temp_iom_group = fbe_module_mgmt_get_iom_group(module_mgmt, sp_id, temp_iom_num, temp_port_num);
             /* 
              * The configured information is based on IO module groups.  There can be more than one
              * io module group for a given limit. 
              */ 
             if(fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, temp_iom_num, temp_port_num))
             {
                 // FC groups share a limit
                 if((iom_group == FBE_IOM_GROUP_A)||(iom_group == FBE_IOM_GROUP_D) || (iom_group == FBE_IOM_GROUP_M)) 
                 {
                     if((temp_iom_group == FBE_IOM_GROUP_A) || (temp_iom_group == FBE_IOM_GROUP_D) || (temp_iom_group == FBE_IOM_GROUP_M))
                     {
                         init_port_count++;
                     }
                 }
                 else if(iom_group == temp_iom_group)
                 {
                     init_port_count++;
                 }
                 else if(platform_port_limits.max_combined_iscsi_fe > 0)
                 {
                     // There is a combined 1G and 10G iSCSI limit for this platform
                     if (temp_iom_group == FBE_IOM_GROUP_E
                         || temp_iom_group == FBE_IOM_GROUP_I
                         || temp_iom_group == FBE_IOM_GROUP_J
                         || temp_iom_group == FBE_IOM_GROUP_L
                         || temp_iom_group == FBE_IOM_GROUP_O
                         || temp_iom_group == FBE_IOM_GROUP_P
                         || temp_iom_group == FBE_IOM_GROUP_Q
                         || temp_iom_group == FBE_IOM_GROUP_R
                         || temp_iom_group == FBE_IOM_GROUP_S)
                     {
                         if (temp_iom_group == FBE_IOM_GROUP_E
                         || temp_iom_group == FBE_IOM_GROUP_I
                         || temp_iom_group == FBE_IOM_GROUP_J
                         || temp_iom_group == FBE_IOM_GROUP_L
                         || temp_iom_group == FBE_IOM_GROUP_O
                         || temp_iom_group == FBE_IOM_GROUP_P
                         || temp_iom_group == FBE_IOM_GROUP_Q
                         || temp_iom_group == FBE_IOM_GROUP_R
                         || temp_iom_group == FBE_IOM_GROUP_S)
                         {
                             init_port_count++;
                         }
                     }

                 }
                else if (iom_group == FBE_IOM_GROUP_E
                      || iom_group == FBE_IOM_GROUP_I
                      || iom_group == FBE_IOM_GROUP_J
                      || iom_group == FBE_IOM_GROUP_L
                      || iom_group == FBE_IOM_GROUP_P
                      || iom_group == FBE_IOM_GROUP_Q
                      || iom_group == FBE_IOM_GROUP_R
                      || iom_group == FBE_IOM_GROUP_S)
                {
                    // 10g iSCSI groups share a limit
                    if (temp_iom_group == FBE_IOM_GROUP_E
                     || temp_iom_group == FBE_IOM_GROUP_I
                     || temp_iom_group == FBE_IOM_GROUP_J
                     || temp_iom_group == FBE_IOM_GROUP_L
                     || temp_iom_group == FBE_IOM_GROUP_P
                     || temp_iom_group == FBE_IOM_GROUP_Q
                     || temp_iom_group == FBE_IOM_GROUP_R
                     || iom_group == FBE_IOM_GROUP_S)
                    {
                        init_port_count++;
                    }
                }
                 
             }
             
         }
     }

     /*
      * Now loop through what is physically present and get the uninitialized port count.  If/when this 
      * exceeds the limit, fault the ports exceeding the limit. 
      */
     for(temp_iom_num = 0; temp_iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; temp_iom_num++)
     {
         temp_slic_type = fbe_module_mgmt_get_slic_type(module_mgmt,sp_id,temp_iom_num);
         port_count = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt,sp_id,temp_iom_num);
         wrong_slic = FBE_FALSE;
         fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                    "ModMgmt: IOM %d Type: %s PortCount:%d\n",
                                                    temp_iom_num, 
                                                    fbe_module_mgmt_slic_type_to_string(temp_slic_type), 
                                                    port_count);
        for(temp_port_num = 0; temp_port_num <port_count; temp_port_num++)
        {
            if( (fbe_module_mgmt_get_port_state(module_mgmt,sp_id,temp_iom_num, temp_port_num) == FBE_PORT_STATE_FAULTED) &&
                (fbe_module_mgmt_get_port_substate(module_mgmt,sp_id,temp_iom_num, temp_port_num) == FBE_PORT_SUBSTATE_INCORRECT_MODULE))
            {
                //don't count the ports if the slic is the wrong type, these won't be used when configuring and do not count
                wrong_slic = FBE_TRUE;
            }
        }

        if(wrong_slic)
        {
            //wrong slic type detected in this slot, don't count it against the limits.
            continue;
        }

        for(temp_port_num = 0; temp_port_num < port_count; temp_port_num++)
        {
            /* most limits are set per protocol, iSCSI has 1G and 10G variations
             * so explicit checks are needed
             */
            fbe_ioport_role_t temp_port_role = fbe_module_mgmt_get_port_role(module_mgmt,
                                                                             sp_id,
                                                                             temp_iom_num,
                                                                             temp_port_num);

            IO_CONTROLLER_PROTOCOL temp_port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt,
                                                                                          sp_id,
                                                                                          temp_iom_num,
                                                                                          temp_port_num);

            if (temp_port_role != port_role)
            {
                continue;
            }

            if ((temp_port_protocol == port_protocol && port_protocol != IO_CONTROLLER_PROTOCOL_ISCSI)
              || iscsi_slic_type_match(slic_type, temp_slic_type))
            {
                if(!fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, temp_iom_num, temp_port_num))
                {
                    uninit_port_count++;
                    pLocation.sp = sp_id;
                    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, sp_id, temp_iom_num, &slot, &device_type);
                    pLocation.slot = slot;

                    if((init_port_count+uninit_port_count) > platform_limit)
                    {
                        if(!((fbe_module_mgmt_get_port_state(module_mgmt, sp_id, 
                                                             temp_iom_num, temp_port_num) == 
                                                             FBE_PORT_STATE_FAULTED) &&
                             (fbe_module_mgmt_get_port_substate(module_mgmt, sp_id, 
                                                                temp_iom_num, temp_port_num) == 
                                                                FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS)))
                        {
                            /*
                             * This port exceeds the platform limit.  Fault the port.
                             */
                            fbe_module_mgmt_set_port_state(module_mgmt, sp_id, 
                                                           temp_iom_num, temp_port_num, 
                                                           FBE_PORT_STATE_FAULTED);
                            fbe_module_mgmt_set_port_substate(module_mgmt, sp_id, 
                                                              temp_iom_num, temp_port_num, 
                                                              FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS);

                            fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));

                            /* Get the device string */
                            status = fbe_base_env_create_device_string(device_type, 
                                                                       &pLocation, 
                                                                       &(deviceStr[0]), 
                                                                       FBE_DEVICE_STRING_LENGTH);

                            fbe_event_log_write(ESP_ERROR_IO_PORT_FAULTED,
                                 NULL, 0,
                                 "%s %d %s", 
                                 &deviceStr[0],
                                 temp_port_num,
                                 fbe_module_mgmt_port_substate_to_string(FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS));

                            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "ModMgmt: Setting IOM %d, Port %d %s %s, overlimit, init count:%d, uninit count:%d, limit:%d\n",
                                                    temp_iom_num, temp_port_num, 
                                                    fbe_module_mgmt_protocol_to_string(port_protocol),
                                                    fbe_module_mgmt_port_role_to_string(port_role),
                                                    init_port_count,
                                                    uninit_port_count,
                                                    platform_limit);
                             if(temp_slic_type != FBE_SLIC_TYPE_NA)
                             {
                                 // disable power to overlimit SLICs.
                                 fbe_api_board_set_IOModulePersistedPowerState(module_mgmt->board_object_id,
                                                                                 sp_id, 
                                                                                 slot,
                                                                                 device_type,
                                                                                 FALSE);
                                if(fbe_module_mgmt_get_iom_power_enabled(module_mgmt, sp_id, temp_iom_num) == FBE_TRUE)
                                {
                                    module_mgmt->reboot_sp = REBOOT_LOCAL;
                                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "ModMgmt: Setting IOM %d, to power off, requesting an SP reboot.\n",
                                                    temp_iom_num);
                                }
                            }
                        }
                    }
                    else if((fbe_module_mgmt_get_port_state(module_mgmt, sp_id, 
                                                            temp_iom_num, temp_port_num) == 
                                                               FBE_PORT_STATE_FAULTED) &&
                             (fbe_module_mgmt_get_port_substate(module_mgmt, sp_id, 
                                                                temp_iom_num, temp_port_num) == 
                                                                   FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS))
                    {
                        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "ModMgmt: Setting IOM %d, Port %d %s %s, ok, init count:%d, uninit count:%d, limit:%d\n",
                                                    temp_iom_num, temp_port_num, 
                                                    fbe_module_mgmt_protocol_to_string(port_protocol),
                                                    fbe_module_mgmt_port_role_to_string(port_role),
                                                    init_port_count,
                                                    uninit_port_count,
                                                    platform_limit);
                        fbe_module_mgmt_set_port_state(module_mgmt, sp_id, temp_iom_num, 
                                                       temp_port_num, FBE_PORT_STATE_UNINITIALIZED);
                        fbe_module_mgmt_set_port_substate(module_mgmt, sp_id, temp_iom_num, 
                                                          temp_port_num, FBE_PORT_SUBSTATE_UNINITIALIZED);
                        if(temp_slic_type != FBE_SLIC_TYPE_NA)
                        {
                            // re-enable power to any SLICs that were overlimit in the past in case they were powered off
                            fbe_api_board_set_IOModulePersistedPowerState(module_mgmt->board_object_id,
                                                                          sp_id, 
                                                                          slot,
                                                                          device_type,
                                                                          TRUE);
                            if(fbe_module_mgmt_get_iom_power_enabled(module_mgmt, sp_id, temp_iom_num) == FBE_FALSE)
                            {
                                // reboot the SP if we have changed the persisted power state of the slic
                                module_mgmt->reboot_sp = REBOOT_LOCAL;
                                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "ModMgmt: Setting IOM %d, to power on, requesting an SP reboot.\n",
                                                    temp_iom_num);
                            }
                        }

                     }
                     else if( (temp_slic_type != FBE_SLIC_TYPE_NA) &&
                         (fbe_module_mgmt_get_iom_power_enabled(module_mgmt, sp_id, temp_iom_num) == FBE_FALSE) &&
                              (fbe_module_mgmt_get_module_state(module_mgmt,sp_id,temp_iom_num) != MODULE_STATE_UNSUPPORTED_MODULE) &&
                              (fbe_module_mgmt_get_module_state(module_mgmt,sp_id,temp_iom_num) != MODULE_STATE_FAULTED))
                     {
                         fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "ModMgmt: Setting IOM %d, Port %d %s %s, ok, init count:%d, uninit count:%d, limit:%d\n",
                                                    temp_iom_num, temp_port_num, 
                                                    fbe_module_mgmt_protocol_to_string(port_protocol),
                                                    fbe_module_mgmt_port_role_to_string(port_role),
                                                    init_port_count,
                                                    uninit_port_count,
                                                    platform_limit);
                         // re-enable power to any SLICs if they are not overlimit
                         fbe_api_board_set_IOModulePersistedPowerState(module_mgmt->board_object_id,
                                                                         sp_id, 
                                                                         slot,
                                                                         device_type,
                                                                         TRUE);
                         // reboot the SP if we have changed the persisted power state of the slic
                         module_mgmt->reboot_sp = REBOOT_LOCAL;
                         fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                FBE_TRACE_LEVEL_INFO,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "ModMgmt: Setting IOM %d, to power on, requesting an SP reboot.\n",
                                                temp_iom_num);
                     }
                 }
             }
             else
             {
                 fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                FBE_TRACE_LEVEL_INFO,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "ModMgmt: IOM %d Port %d is not being counted.\n",
                                                temp_iom_num, temp_port_num);

             }
         }
     }
     return;
 }
 /******************************************************
 * end fbe_module_mgmt_check_platform_limits() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_slic_type()
 ****************************************************************
 * @brief
 *  This function returns a SLIC type for the specified io module
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 *
 * @return - fbe_module_slic_type_t - SLIC type
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
 fbe_module_slic_type_t fbe_module_mgmt_get_slic_type(fbe_module_mgmt_t *module_mgmt, 
                                                      SP_ID sp_id, fbe_u8_t iom_num)
 {
     return module_mgmt->io_module_info[iom_num].logical_info[sp_id].slic_type;
 }
 /******************************************************
 * end fbe_module_mgmt_get_slic_type() 
 ******************************************************/

 /*!**************************************************************
 * fbe_module_mgmt_set_slic_type()
 ****************************************************************
 * @brief
 *  This function sets a SLIC type for the specified io module
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - slic_type - SLIC type
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
 void fbe_module_mgmt_set_slic_type(fbe_module_mgmt_t *module_mgmt, 
                                                      SP_ID sp_id, 
                                                      fbe_u8_t iom_num,
                                                      fbe_module_slic_type_t slic_type)
 {
     module_mgmt->io_module_info[iom_num].logical_info[sp_id].slic_type = slic_type;
     return;
 }
 /******************************************************
 * end fbe_module_mgmt_set_slic_type() 
 ******************************************************/


 /*!**************************************************************
 * fbe_module_mgmt_derive_slic_type()
 ****************************************************************
 * @brief
 *  This function derives a SLIC type based upon the specified
 *  unique id.
 * 
 * @param - unique id - unique identifier for supported slic
 *
 * @return - slic_type
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *  5-Sept.-2014:  Refactor. Sam Hu
 *
 ****************************************************************/
fbe_module_slic_type_t fbe_module_mgmt_derive_slic_type(fbe_module_mgmt_t * module_mgmt, HW_MODULE_TYPE unique_id)
{
    fbe_module_slic_type_t slic_type = FBE_SLIC_TYPE_UNSUPPORTED;
    int i;
    
    for (i = 0; i < fbe_ffid_property_map_size; i++)
    {
        if ((FAMILY_ID)unique_id == fbe_ffid_property_map[i].ffid)
        {
            slic_type = fbe_ffid_property_map[i].slic_type;
            break;
        }
    }

    if( (!(slic_type & module_mgmt->platform_hw_limit.supported_slic_types)) &&
        (slic_type != FBE_SLIC_TYPE_UNKNOWN))
    {
        slic_type = FBE_SLIC_TYPE_UNSUPPORTED;
    }

    return(slic_type);
}
/******************************************************
 * end fbe_module_mgmt_derive_slic_type() 
 ******************************************************/

 /*!**************************************************************
 * fbe_module_mgmt_derive_label_name()
 ****************************************************************
 * @brief
 *  This function derives a SLIC type based upon the specified
 *  unique id.
 * 
 * @param - unique id - unique identifier for supported slic
 *
 * @return - slic label name string
 *
 * @author
 *  20-July-2015:  Created. bphilbin 

 *
 ****************************************************************/
void fbe_module_mgmt_derive_label_name(fbe_module_mgmt_t * module_mgmt, HW_MODULE_TYPE unique_id, char *label_name)
{
    int i;
    
    for (i = 0; i < fbe_ffid_property_map_size; i++)
    {
        if ((FAMILY_ID)unique_id == fbe_ffid_property_map[i].ffid)
        {
            strncpy(label_name, fbe_ffid_property_map[i].label_name, strlen(fbe_ffid_property_map[i].label_name));
            return;
        }
    }

    strncpy(label_name, "Unknown", strlen("Unknown"));
    return;
    
}
/******************************************************
 * end fbe_module_mgmt_derive_slic_type() 
 ******************************************************/

/**************************************************************************
 *          cm_flexports_is_special_port_assigned
 * ************************************************************************
 *  DESCRIPTION:
 *  This function derives a protocol for the entire IO module based on the
 *  Unique ID for that module.
 *
 *
 *  PARAMETERS:
 * @param - unique id - unique identifier for the io module
 *
 *  RETURN VALUES:
 *   FBE_IO_MODULE_PROTOCOL - protocol for the io module
 *
 *  History:
 *  12-December-12: bphilbin - Created
 *  5-Sept.-2014:  Refactor. Sam Hu
 * ***********************************************************************/
FBE_IO_MODULE_PROTOCOL fbe_module_mgmt_derive_io_module_protocol(fbe_module_mgmt_t *module_mgmt, HW_MODULE_TYPE unique_id)
{
    FBE_IO_MODULE_PROTOCOL iom_protocol = FBE_IO_MODULE_PROTOCOL_UNKNOWN;
    int i;
    
    for (i = 0; i < fbe_ffid_property_map_size; i++)
    {
        if ((FAMILY_ID)unique_id == fbe_ffid_property_map[i].ffid)
        {
            iom_protocol = fbe_ffid_property_map[i].protocol;
            break;
        }
    }
    
    return(iom_protocol);
}

void fbe_module_mgmt_set_io_module_protocol(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, FBE_IO_MODULE_PROTOCOL iom_protocol)
{
    module_mgmt->io_module_info[iom_num].physical_info[sp_id].protocol = iom_protocol;
    return;
}

FBE_IO_MODULE_PROTOCOL fbe_module_mgmt_get_io_module_protocol(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp_id].protocol;
}


/*!**************************************************************
 * fbe_module_mgmt_get_port_protocol()
 ****************************************************************
 * @brief
 *  This function gets the protocol for the specified port.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @return - IO_CONTROLLER_PROTOCOL - port protocol.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
IO_CONTROLLER_PROTOCOL fbe_module_mgmt_get_port_protocol(fbe_module_mgmt_t *module_mgmt, 
                                                         SP_ID sp_id, fbe_u8_t iom_num, 
                                                         fbe_u8_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].protocol;
}
/******************************************************
 * end fbe_module_mgmt_get_port_protocol() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_port_controller_protocol()
 ****************************************************************
 * @brief
 *  This function gets the protocol for the controller to the specified port.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @return - IO_CONTROLLER_PROTOCOL - port protocol.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
IO_CONTROLLER_PROTOCOL fbe_module_mgmt_get_port_controller_protocol(fbe_module_mgmt_t *module_mgmt, 
                                                                    SP_ID sp_id, fbe_u8_t iom_num, 
                                                                    fbe_u8_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].controller_protocol;
}
/******************************************************
 * end fbe_module_mgmt_get_port_protocol() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_port_sfp_capable()
 ****************************************************************
 * @brief
 *  This function gets SFP capability for the specified port.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @return - IO_CONTROLLER_PROTOCOL - port protocol.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_mgmt_status_t fbe_module_mgmt_get_port_sfp_capable(fbe_module_mgmt_t *module_mgmt, 
                                                         SP_ID sp_id, fbe_u8_t iom_num, 
                                                         fbe_u8_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].SFPcapable;
}
/******************************************************
 * end fbe_module_mgmt_get_port_sfp_capable() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_is_port_initialized()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the specified port has a valid logical
 *  number set, meaning it is configured/initialized.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 *
 * @return - fbe_bool_t - port is initialized.
 *
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_port_initialized(fbe_module_mgmt_t *module_mgmt, 
                                               SP_ID sp_id, fbe_u8_t iom_num, 
                                               fbe_u8_t port_num)
{
    fbe_u8_t port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return (module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num != INVALID_PORT_U8);
}
/******************************************************
 * end fbe_module_mgmt_is_port_initialized() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_is_iom_initialized()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the specified IO module has a valid logical
 *  number set, meaning it is configured/initialized, for any ports
 *  on that IO module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - port is initialized.
 *
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_iom_initialized(fbe_module_mgmt_t *module_mgmt, 
                                               SP_ID sp_id, fbe_u8_t iom_num)
{
    fbe_u8_t port_num;
    for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
    {
        if(fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, port_num))
        {
            return TRUE;
        }
    }
    return FALSE;
}
/******************************************************
 * end fbe_module_mgmt_is_iom_initialized() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_is_iom_type_ok()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the specified io module is
 *  compatible with the configured IO module group.  If no ports
 *  are configured then this returns TRUE.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module type is ok.
 *
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_iom_type_ok(fbe_module_mgmt_t *module_mgmt, 
                                           SP_ID sp_id, fbe_u8_t iom_num)
{
    fbe_u8_t port_num;
    fbe_iom_group_t iom_group;
    fbe_module_slic_type_t slic_type;
    IO_CONTROLLER_PROTOCOL protocol;

    if(!fbe_module_mgmt_is_iom_initialized(module_mgmt,sp_id,iom_num))
    {
        return TRUE;
    }
    slic_type = fbe_module_mgmt_get_slic_type(module_mgmt,sp_id,iom_num);
    
    for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
    {
        /*
         * If the port has been configured and the iom group does not match what the current
         * port would be assigned, then this io module type is not ok for this configuration.
         */
        if(fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, port_num))
        {
            /*
             * We have to check the protocol on the local SP here because we don't have the peer's protocol.
             * Maybe this can be derived by specl from the unique ID of the SLIC.  If that hppens then 
             * the code here can be modified to check the protocol from either SP.
             */
            iom_group = fbe_module_mgmt_get_iom_group(module_mgmt,sp_id,iom_num,port_num);
            protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt,module_mgmt->local_sp,iom_num,port_num);

            /*
             * Special check to not fault the SLICs during an upgrade
             * procedure.
             */
            if((fbe_module_mgmt_is_iom_initialized(module_mgmt, sp_id, iom_num) &&
                    module_mgmt->slic_upgrade != FBE_MODULE_MGMT_NO_SLIC_UPGRADE ) &&
               (sp_id != module_mgmt->local_sp))
            {
                if((slic_type == FBE_SLIC_TYPE_4PORT_ISCSI_1G || slic_type == FBE_SLIC_TYPE_ISCSI_COPPER ||
                        slic_type == FBE_SLIC_TYPE_ISCSI_10G || slic_type == FBE_SLIC_TYPE_ISCSI_10G_V2 ) &&
                       ( iom_group == FBE_IOM_GROUP_G || iom_group == FBE_IOM_GROUP_E ||
                         iom_group == FBE_IOM_GROUP_I || iom_group == FBE_IOM_GROUP_J ))
                {
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                                "%s SP:%d IOM:%d Type ok iSCSI slic upgrade in progress.\n",
                                            __FUNCTION__, 
                                            sp_id, iom_num);
                    return TRUE;
                }
                else if((slic_type == FBE_SLIC_TYPE_FC_8G || slic_type == FBE_SLIC_TYPE_FC_16G ) &&
                       ( iom_group == FBE_IOM_GROUP_D || iom_group == FBE_IOM_GROUP_M))
               {
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                                "%s SP:%d IOM:%d Type ok FC slic upgrade in progress.\n",
                                            __FUNCTION__, 
                                            sp_id, iom_num);
                   return TRUE;
                }
            }

            switch(iom_group)
            {
            case FBE_IOM_GROUP_C:
                if( (slic_type != FBE_SLIC_TYPE_SAS) && (slic_type != FBE_SLIC_TYPE_6G_SAS_1)&&
                    (!( (slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_SAS))) )
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_D:
                if( (slic_type != FBE_SLIC_TYPE_FC_8G && slic_type != FBE_SLIC_TYPE_FC_8G_4S && slic_type != FBE_SLIC_TYPE_FC_8G_1S3M) && 
                    (!( (slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_FIBRE) )) )
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_E:
                if( (slic_type != FBE_SLIC_TYPE_ISCSI_10G) &&
                    !((slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_ISCSI)))
                {

                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_F:
                if(slic_type != FBE_SLIC_TYPE_FCOE)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_G:
                if( (slic_type != FBE_SLIC_TYPE_4PORT_ISCSI_1G) &&
                    (!( (slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_ISCSI) )) )
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_H:
                if(slic_type != FBE_SLIC_TYPE_6G_SAS_2)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_I:
                if(slic_type != FBE_SLIC_TYPE_ISCSI_COPPER)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_J:
                if(slic_type != FBE_SLIC_TYPE_ISCSI_10G_V2)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_K:
                if(slic_type != FBE_SLIC_TYPE_6G_SAS_3 &&
                   (!( (slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_SAS))))
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_L:
                if(slic_type != FBE_SLIC_TYPE_4PORT_ISCSI_10G)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_N:
                if(slic_type != FBE_SLIC_TYPE_12G_SAS &&
                   (!( (slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_SAS))))
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_O:
                if(slic_type != FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_Q:
                if(slic_type != FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_R:
                if(slic_type != FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_S:
                if(slic_type != FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G)
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_M:
                if((slic_type != FBE_SLIC_TYPE_FC_16G) &&
                   ( !((slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_FIBRE)) &&
                     !((slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC))))
                {
                    return FALSE;
                }
                break;
            case FBE_IOM_GROUP_P:
                if((!((slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_ISCSI)) &&
                     !((slic_type == FBE_SLIC_TYPE_NA) && (protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC))))
                {
                    return FALSE;
                }
                break;
            default:
                break;
            }
        }
    }
    return TRUE;
}
/******************************************************
 * end fbe_module_mgmt_is_iom_initialized() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_is_iom_inserted()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the specified io module is inserted.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module is inserted.
 *
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_iom_inserted(fbe_module_mgmt_t *module_mgmt, 
                                           SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.inserted;
}
/******************************************************
 * end fbe_module_mgmt_is_iom_inserted() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_is_iom_supported()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the specified io module is
 *  currently supported.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module is supported.
 *
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_iom_supported(fbe_module_mgmt_t *module_mgmt, 
                                           SP_ID sp_id, fbe_u8_t iom_num)
{
    fbe_board_mgmt_platform_info_t boardPlatformInfo;
    fbe_status_t status;

    status = fbe_api_board_get_platform_info(module_mgmt->board_object_id, 
                                             &boardPlatformInfo);

    switch(boardPlatformInfo.hw_platform_info.platformType)
    {
        case SPID_DEFIANT_K2_HW_TYPE:
        case SPID_DEFIANT_K3_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                // Base Module
                case IRONHIDE_6G_SAS:
                case IRONHIDE_6G_SAS_REV_B:
                case IRONHIDE_6G_SAS_REV_C:
                // Other SLICs
                case GLACIER:
                case GLACIER_REV_C:
                case SUPERCELL:
                case MOONLITE:
                case SNOWDEVIL:
                case DUSTDEVIL:
                case NO_MODULE:
                //@KHP_TODO: Remove the rest of these SLICs before GA. They are from Rockies support.
                case COROMANDEL:
                case HEATWAVE:
                case HYPERNOVA:
                case ERUPTION_COPPER_2PORT:
                case ERUPTION_2PORT_REV_C_84823:
                case ERUPTION_2PORT_REV_C_84833:
                case ERUPTION_REV_D:
                case ELNINO:
                case ELNINO_REV_B:
                //-----END remove-before-GA
                    return TRUE;
                    break;
                default:
                    return FALSE;
                    break;

            }
        }

        case SPID_DEFIANT_M4_HW_TYPE:
        case SPID_DEFIANT_M5_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                // Base Module
                case IRONHIDE_6G_SAS:
                case IRONHIDE_6G_SAS_REV_B:
                case IRONHIDE_6G_SAS_REV_C:
                // Other SLICs
                case GLACIER:
                case GLACIER_REV_C:
                case SUPERCELL:
                case NO_MODULE:
                case COROMANDEL:
                case HEATWAVE:
                case HYPERNOVA:
                case ELNINO:
                case ELNINO_REV_B:
                case VORTEX16:
                case VORTEX16Q:
                    return TRUE;
                    break;

                case ERUPTION_COPPER_2PORT:
                case ERUPTION_2PORT_REV_C_84823:
                case ERUPTION_2PORT_REV_C_84833:
                case ERUPTION_REV_D:
                if(fbe_module_mgmt_is_jetfire_with_octane_system(module_mgmt))
                {
                    // Eruption is not supported in a Jetfire system with Octane PS
                    return FALSE;
                }
                else
                {
                    return TRUE;
                }
                    break;
                default:
                    return FALSE;
                    break;

            }
        }
       
        case SPID_DEFIANT_M1_HW_TYPE:
        case SPID_DEFIANT_M2_HW_TYPE:
        case SPID_DEFIANT_M3_HW_TYPE:
        case SPID_DEFIANT_S1_HW_TYPE:
        case SPID_DEFIANT_S4_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                // Base Module
                case IRONHIDE_6G_SAS:
                case IRONHIDE_6G_SAS_REV_B:
                case IRONHIDE_6G_SAS_REV_C:
                // Other SLICs
                case GLACIER:
                case GLACIER_REV_C:
                case SUPERCELL:
                case COROMANDEL:
                case HEATWAVE:
                case HYPERNOVA:
                case MOONLITE:
                case SNOWDEVIL:
                case DUSTDEVIL:
                case NO_MODULE:
                case ERUPTION_COPPER_2PORT:
                case ERUPTION_2PORT_REV_C_84823:
                case ERUPTION_2PORT_REV_C_84833:
                case ERUPTION_REV_D:
                case ELNINO:
                case ELNINO_REV_B:
                case VORTEX16:
                case VORTEX16Q:
                    return TRUE;
                    break;
                default:
                    return FALSE;
                    break;

            }
        }

        case SPID_PROMETHEUS_M1_HW_TYPE:
        case SPID_PROMETHEUS_S1_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                case GLACIER:
                case GLACIER_REV_C:
                case SUPERCELL:
                case COROMANDEL:
                case HEATWAVE:
                case HYPERNOVA:
                case MOONLITE:
                case SNOWDEVIL:
                case DUSTDEVIL:
                case COLDFRONT:
                case NO_MODULE:
                case PEACEMAKER_WITH_SAS:
                case MUZZLE_WITH_SAS:
                case ERUPTION_COPPER_2PORT:
                case ERUPTION_2PORT_REV_C_84823:
                case ERUPTION_2PORT_REV_C_84833:
                case ERUPTION_REV_D:
                case ELNINO:
                case ELNINO_REV_B:
                case IRONHIDE_6G_SAS:
                case IRONHIDE_6G_SAS_REV_B:
                case IRONHIDE_6G_SAS_REV_C:
                case VORTEX16:
                case VORTEX16Q:
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    break;
            }
        }

        case SPID_NOVA1_HW_TYPE:
        case SPID_NOVA3_HW_TYPE:
        case SPID_NOVA3_XM_HW_TYPE:
        case SPID_NOVA_S1_HW_TYPE:
        case SPID_BEARCAT_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                case NO_MODULE:
                /* eSLICs */
                case ESUPERCELL:
                case EGLACIER:
                case EGLACIER_4S:
                case EGLACIER_1S3M:
                case EERUPTION:
                case EERUPTION_10GB_W_BCM84833:
                case EERUPTION_REV_B:
                case ELANDSLIDE:
                /* virtual mezzanine? */
                case PEACEMAKER_WITH_SAS:
                case BEACHCOMBER_PCB:
                case BEACHCOMBER_PCB_REV_B:
                case BEACHCOMBER_PCB_REV_B_CPU_6_CORE:
                case BEACHCOMBER_PCB_REV_B_BCM57840_16GB:
                case BEACHCOMBER_PCB_REV_B_BCM57840_24GB:
                case BEACHCOMBER_PCB_REV_C_4C:
                case BEACHCOMBER_PCB_REV_C_6C:
                case BEACHCOMBER_PCB_REV_D_4C:
                case BEACHCOMBER_PCB_REV_D_6C:
                case BEACHCOMBER_PCB_SIM:
                case BEACHCOMBER_PCB_FC_2C:
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    break;
            }
        }

        case SPID_MERIDIAN_ESX_HW_TYPE:
        case SPID_TUNGSTEN_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                case NO_MODULE:
                /* eSLICs */
                case EERUPTION:
                case EERUPTION_10GB_W_BCM84833:
                case EERUPTION_REV_B:
                /* virtual mezzanine? */
                case MERIDIAN_SP_ESX:
                case TUNGSTEN_SP:
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    break;
            }
        }

// ENCL_CLEANUP - cases for Moons
        case SPID_ENTERPRISE_HW_TYPE:
        {

            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                case GLACIER:
                case GLACIER_REV_C:
                case SUPERCELL:
                case HEATWAVE:
                case MOONLITE:
                case NO_MODULE:
                case ERUPTION_COPPER_2PORT:
                case ERUPTION_2PORT_REV_C_84823:
                case ERUPTION_2PORT_REV_C_84833:
                case ERUPTION_REV_D:
                case ELNINO:
                case ELNINO_REV_B:
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    break;
            }
        }

        case SPID_OBERON_1_HW_TYPE:
        case SPID_OBERON_2_HW_TYPE:
        case SPID_OBERON_3_HW_TYPE:
        case SPID_OBERON_4_HW_TYPE:
        case SPID_OBERON_S1_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                case NO_MODULE:
                /* SLICs */
                case DUSTDEVIL_X:
                case SNOWDEVIL_X:
                case VORTEXQ_X:
                case THUNDERHEAD_X:
                case ROCKSLIDE_X:
                case MAELSTROM_X:
                case DOWNBURST_X_RDMA:
                /* Onboard ports */
                case OBERON_SP_85W_REV_A:
                case OBERON_SP_105W_REV_A:
                case OBERON_SP_120W_REV_A:
                case OBERON_SP_E5_2603V3_REV_B:
                case OBERON_SP_E5_2630V3_REV_B:
                case OBERON_SP_E5_2609V3_REV_B:
                case OBERON_SP_E5_2660V3_REV_B:
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    break;
            }
        }

        case SPID_HYPERION_1_HW_TYPE:
        case SPID_HYPERION_2_HW_TYPE:
        case SPID_HYPERION_3_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                case NO_MODULE:
                /* SLICs */
                case DUSTDEVIL_X:
                case SNOWDEVIL_X:
                case VORTEXQ_X:
                case THUNDERHEAD_X:
                case ROCKSLIDE_X:
                /* Base Modules */
                case LAPETUS_12G_SAS_REV_A:
                case LAPETUS_12G_SAS_REV_B:
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    break;
            }
        }

        case SPID_TRITON_1_HW_TYPE:
        {
            switch(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.uniqueID)
            {
                case NO_MODULE:
                /* SLICs */
                case DUSTDEVIL_X:
                case SNOWDEVIL_X:
                case VORTEXQ_X:
                case THUNDERHEAD_X:
                case ROCKSLIDE_X:
                    return TRUE;
                    break;

                default:
                    return FALSE;
                    break;
            }
        }


        default:
            // unable to identify this new platform type, just say the SLICs are supported
            return TRUE;
            break;
    }
}
/******************************************************
 * end fbe_module_mgmt_is_iom_supported() 
 ******************************************************/
/*!**************************************************************
 * fbe_module_mgmt_is_iom_peerboot_fault()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the specified io module is faulted deteced by
 *      fault register.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module is inserted.
 *
 * @author
 *  23-July-2012:  Chengkai Hu, Created. 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_iom_peerboot_fault(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID              sp_id,
                                                 fbe_u8_t           iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.isFaultRegFail;
}
/******************************************************
 * end fbe_module_mgmt_is_iom_peerboot_fault() 
 ******************************************************/
/*!**************************************************************
 * fbe_module_mgmt_get_expected_slic_type()
 ****************************************************************
 * @brief
 *  This function returns the expected slic type for the specified
 *  io module index based on the port configuration.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_module_slic_type_t - expected slic type.
 *
 * @author
 *  23-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_module_slic_type_t fbe_module_mgmt_get_expected_slic_type(fbe_module_mgmt_t *module_mgmt, 
                                           SP_ID sp_id, fbe_u8_t iom_num)
{
    fbe_u8_t port_num;
    fbe_iom_group_t configured_group;
    if(!fbe_module_mgmt_is_iom_initialized(module_mgmt, sp_id, iom_num))
    {
        return module_mgmt->platform_hw_limit.supported_slic_types;
    }
    else
    {
        for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
        {
            if(fbe_module_mgmt_is_port_initialized(module_mgmt,sp_id,iom_num,port_num))
            {
                configured_group = fbe_module_mgmt_get_iom_group(module_mgmt,sp_id,iom_num,port_num);
            }
        }
    }
    switch(configured_group)
    {
    case FBE_IOM_GROUP_C:
        return (FBE_SLIC_TYPE_SAS|FBE_SLIC_TYPE_6G_SAS_1);
        break;
    case FBE_IOM_GROUP_D:
        return (FBE_SLIC_TYPE_FC_8G|FBE_SLIC_TYPE_FC_8G_4S|FBE_SLIC_TYPE_FC_8G_1S3M);
        break;
    case FBE_IOM_GROUP_E:
        return FBE_SLIC_TYPE_ISCSI_10G;
        break;
    case FBE_IOM_GROUP_F:
        return FBE_SLIC_TYPE_FCOE;
        break;
    case FBE_IOM_GROUP_G:
        return FBE_SLIC_TYPE_4PORT_ISCSI_1G;
        break;
    case FBE_IOM_GROUP_H:
        return FBE_SLIC_TYPE_6G_SAS_2;
        break;
    case FBE_IOM_GROUP_I:
        return  FBE_SLIC_TYPE_ISCSI_COPPER;
        break;
    case FBE_IOM_GROUP_J:
        return FBE_SLIC_TYPE_ISCSI_10G_V2;
        break;
    case FBE_IOM_GROUP_K:
        return FBE_SLIC_TYPE_6G_SAS_3;
        break;
    case FBE_IOM_GROUP_L:
        return FBE_SLIC_TYPE_4PORT_ISCSI_10G;
        break;
    case FBE_IOM_GROUP_N:
        return FBE_SLIC_TYPE_12G_SAS;
        break;
    case FBE_IOM_GROUP_Q:
        return FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G;
        break;
    case FBE_IOM_GROUP_O:
        return FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G;
        break;
    case FBE_IOM_GROUP_M:
        return FBE_SLIC_TYPE_FC_16G;
        break;
    case FBE_IOM_GROUP_R:
        return FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G;
        break;
    case FBE_IOM_GROUP_S:
        return FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G;
        break;
    
    default:
        return FBE_SLIC_TYPE_UNKNOWN;
        break;
    }
}

/*!**************************************************************
 * fbe_module_mgmt_is_iom_power_good()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the power status of the 
 *  specified io module is good.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module is powered on.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_iom_power_good(fbe_module_mgmt_t *module_mgmt, 
                                             SP_ID sp_id, fbe_u8_t iom_num)
{
    if(module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.powerStatus == FBE_POWER_STATUS_POWER_ON)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}

/*!**************************************************************
 * fbe_module_mgmt_get_iom_power_status()
 ****************************************************************
 * @brief
 *  This function returns the power status for the specified 
 *  io module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module is powered on.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_power_status_t fbe_module_mgmt_get_iom_power_status(fbe_module_mgmt_t *module_mgmt,
                                                        SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.powerStatus;
}

/*!**************************************************************
 * fbe_module_mgmt_get_iom_power_enabled()
 ****************************************************************
 * @brief
 *  This function returns the power enabled for the specified 
 *  io module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module is power enabled.
 *
 * @author
 *  11-Feb-2013:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_get_iom_power_enabled(fbe_module_mgmt_t *module_mgmt,
                                                        SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.powerEnabled;
}

/*!**************************************************************
 * fbe_module_mgmt_get_internal_fan_fault()
 ****************************************************************
 * @brief
 *  This function returns the internal fan fault status for the specified 
 *  io module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module is powered on.
 *
 * @author
 *  24-Aug-2012:  Created. Rui Chang 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_get_internal_fan_fault(fbe_module_mgmt_t *module_mgmt,
                                                        SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.internalFanFault;
}


/*!**************************************************************
 * fbe_module_mgmt_get_num_ports_present_on_iom()
 ****************************************************************
 * @brief
 *  This function returns the number of ports that are physicaly
 *  present on the specified io module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_u32_t - number of ports on the io module
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_num_ports_present_on_iom(fbe_module_mgmt_t *module_mgmt, 
                                                       SP_ID sp_id, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.ioPortCount;
}




/*!**************************************************************
 * fbe_module_mgmt_get_io_port_number()
 ****************************************************************
 * @brief
 *  This function returns the port number as it appears on the label
 *  for an io module for a specified port index to the array of
 *  port data.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - port_index - port index number
 *
 * @return - fbe_u8_t - port label number.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u8_t fbe_module_mgmt_get_io_port_number(fbe_module_mgmt_t *module_mgmt, 
                                            SP_ID sp, fbe_u8_t port_index)
{
    return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp].portNumOnModule;
}
/******************************************************
 * end fbe_module_mgmt_get_io_port_number() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_io_port_number()
 ****************************************************************
 * @brief
 *  This function returns the io module index number for a 
 *  specified port index to the array of port data.  This is 
 *  different from the slot number device type combination
 *  that is used via the FBE API.  Here we store mezzanines and 
 *  SLICs in an array of io modules together.  Mezzanines go at
 *  the front and SLICs are situated after them in the array.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - port_index - port index number
 *
 * @return - fbe_u8_t - io module index number.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u8_t fbe_module_mgmt_get_io_port_module_number(fbe_module_mgmt_t *module_mgmt, 
                                                   SP_ID sp, fbe_u8_t port_index)
{
    if(module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp].deviceType == FBE_DEVICE_TYPE_MEZZANINE)
    {
        /*
         * Mezzanine data is at the beginning of the array of io module data.
         */
        return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp].slotNumOnBlade;
    }
    else
    {
        /*
         * SLIC data is saved after mezzanine data in the array of io module data.
         */
        return FBE_ESP_SLIC_IOM_OFFSET + module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp].slotNumOnBlade;
    }
}
/******************************************************
 * end fbe_module_mgmt_get_io_port_module_number() 
 ******************************************************/


/*!**************************************************************

/*!**************************************************************
 * fbe_module_mgmt_is_iom_hwErrMonFault()
 ****************************************************************
 * @brief
 *  This function returns TRUE if HwErrMon fault is TRUE.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_bool_t - io module HwErrMon fault.
 *
 * @author
 *  4-Feb-2015:  Created. Joe Perry 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_iom_hwErrMonFault(fbe_module_mgmt_t *module_mgmt, 
                                                SP_ID sp_id, 
                                                fbe_u8_t iom_num)
{
    return (module_mgmt->io_module_info[iom_num].physical_info[sp_id].module_comp_info.hwErrMonFault);
}

/*!**************************************************************
 * fbe_module_mgmt_convert_device_type_and_slot_to_index()
 ****************************************************************
 * @brief
 *  This function converts a specified device type and slot into
 *  an io module index number into the array of io module data.
 *  This is different from the slot number device type combination
 *  that is used via the FBE API.  Here we store mezzanines and 
 *  SLICs in an array of io modules together.  Mezzanines go at
 *  the front and SLICs are situated after them in the array.
 * 
 * @param - device_type - type of io module device (mezzanine vs slic)
 * @param - slot - slot number
 *
 * @return - index - io module index number.
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_convert_device_type_and_slot_to_index(fbe_u64_t device_type, fbe_u32_t slot)
{
    /* Only for IOModule, we need the module_num conversion.
     * For all other devices (IOAnnex, Mgmt module and 
     * Mezzanine, just return the slot 
     */
    if(device_type == FBE_DEVICE_TYPE_IOMODULE)
    {
        return (FBE_ESP_SLIC_IOM_OFFSET + slot);
    }
    else
    {
        return slot;
    }
}
/******************************************************
 * end fbe_module_mgmt_device_type_and_slot_to_iom_num() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_iom_num_to_slot_and_device_type()
 ****************************************************************
 * @brief
 *  This function converts a specified device type and slot into
 *  an io module index number into the array of io module data.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - slot - pointer to a slot number (filled in by this function)
 * @param - device_type - pointer to a device type (filled in by this function)
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_iom_num_to_slot_and_device_type(fbe_module_mgmt_t *module_mgmt, SP_ID sp, 
                                                             fbe_u8_t iom_num, fbe_u32_t *slot, 
                                                             fbe_u64_t *device_type)
{
    *device_type = module_mgmt->io_module_info[iom_num].physical_info[sp].type;
    *slot = module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.slotNumOnBlade;
    return;
}
/******************************************************
 * end fbe_module_mgmt_iom_num_to_slot_and_device_type() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_set_device_type()
 ****************************************************************
 * @brief
 *  This function sets the device type for the specified io module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - device_type - device type (ie. slic vs. mezzanine)
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_device_type(fbe_module_mgmt_t *module_mgmt, 
                                SP_ID sp, fbe_u8_t iom_num, 
                                fbe_u64_t device_type)
{
    module_mgmt->io_module_info[iom_num].physical_info[sp].type = device_type;
    return;
}
/******************************************************
 * end fbe_module_mgmt_set_device_type() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_get_device_type()
 ****************************************************************
 * @brief
 *  This function sets the device type for the specified io module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_u64_t - device type
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u64_t fbe_module_mgmt_get_device_type(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp].type;
}
/******************************************************
 * end fbe_module_mgmt_get_device_type() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_slot_num()
 ****************************************************************
 * @brief
 *  This function gets the device slot number for the specified io module.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 *
 * @return - fbe_u32_t - device slot number
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_slot_num(fbe_module_mgmt_t *module_mgmt,
                                       SP_ID sp, fbe_u8_t iom_num)
{
    return module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.slotNumOnBlade;
}
/******************************************************
 * end fbe_module_mgmt_get_device_type() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_set_persistent_port_location()
 ****************************************************************
 * @brief
 *  This function sets the location information for the specified
 *  port.
 * 
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module index number
 * @param - port_num - port number
 *
 * @return - none
 *
 * @author
 *  19-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_persistent_port_location(fbe_module_mgmt_t *module_mgmt, 
                                             SP_ID sp, fbe_u8_t iom_num, fbe_u8_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);

    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.io_enclosure_number = 0;
    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port = port_num;
    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot = 
        module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.slotNumOnBlade;
    /*
     * This works for now as long as the location device types are in the lower 32 bits of the bitmask, we will need to convert
     * this if we have a new device type that gets added to the end of the device type list.
     */
    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.type_32_bit = 
        (fbe_u32_t) module_mgmt->io_module_info[iom_num].physical_info[sp].type;

}
/******************************************************
 * end fbe_module_mgmt_set_persistent_port_location() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_is_port_configured_combined_port()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the port is part of a combined
 *  set of connectors forming a high bandwidth port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @author
 *  10-Sep-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_port_configured_combined_port(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num)
{
    fbe_u32_t port_logical_number, second_port_logical_number;
    fbe_port_role_t port_role, second_port_role;
    fbe_s32_t second_port_num;

    if(!fbe_module_mgmt_is_port_initialized(module_mgmt,sp_id,iom_num, port_num))
    {
        // port isn't configured
        return FBE_FALSE;
    }
    // first check the next port in the slic
    port_logical_number = fbe_module_mgmt_get_port_logical_number(module_mgmt,sp_id,iom_num,port_num);
    port_role = fbe_module_mgmt_get_port_role(module_mgmt,sp_id,iom_num, port_num);
    second_port_num = port_num +1;
    second_port_logical_number = fbe_module_mgmt_get_port_logical_number(module_mgmt,sp_id,iom_num,second_port_num);
    second_port_role = fbe_module_mgmt_get_port_role(module_mgmt,sp_id,iom_num, second_port_num);

    if( (port_logical_number == second_port_logical_number) &&
        (port_role == second_port_role) )
    {
        return FBE_TRUE;
    }


    // now check the previous port if it exists
    second_port_num = port_num - 1;

    if(port_num == 0)
    {
        // no secondary port for this combined connector
        return FBE_FALSE;
    }

    second_port_logical_number = fbe_module_mgmt_get_port_logical_number(module_mgmt,sp_id,iom_num,second_port_num);
    second_port_role = fbe_module_mgmt_get_port_role(module_mgmt,sp_id,iom_num, second_port_num);

    if( (port_logical_number == second_port_logical_number) &&
        (port_role == second_port_role) )
    {
        return FBE_TRUE;
    }
    
    return FBE_FALSE;
}
/******************************************************
 * end fbe_module_mgmt_is_port_combined_port() 
 ******************************************************/

fbe_u32_t fbe_module_mgmt_get_io_module_configured_combined_port_count(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num)
{
    fbe_u32_t num_ports = 0;
    fbe_u32_t port_num = 0;
    fbe_u32_t combined_port_count = 0;

    num_ports = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt,sp_id,iom_num);

    for(port_num = 0; port_num < num_ports; port_num++)
    {
        if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt, sp_id, iom_num, port_num))
        {
            combined_port_count++;
        }
    }
    return combined_port_count;
}

/*!**************************************************************
 * fbe_module_mgmt_get_portal_number()
 ****************************************************************
 * @brief
 *  This function returns the portal number for the specified port
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @author
 *  10-Sep-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_portal_number(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].portal_number;
}
/******************************************************
 * end fbe_module_mgmt_get_portal_number() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_set_portal_number()
 ****************************************************************
 * @brief
 *  This function sets the portal number for the specified port
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @author
 *  10-Sep-2012:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_portal_number(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num, fbe_u32_t portal_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].portal_number = portal_num;
    return;
}
/******************************************************
 * end fbe_module_mgmt_set_portal_number() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_is_port_combined_port()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the port is part of a combined
 *  set of connectors forming a high bandwidth port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @author
 *  10-Sep-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_port_combined_port(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[sp_id].combined_port;
}
/******************************************************
 * end fbe_module_mgmt_is_port_combined_port() 
 ******************************************************/
/*!**************************************************************
 * fbe_module_mgmt_is_port_second_combined_port()
 ****************************************************************
 * @brief
 *  This function returns TRUE if the port is part of a combined
 *  set of connectors and is the second port in the ordered pair.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @author
 *  10-Sep-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_port_second_combined_port(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num)
{
    if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt, sp_id, iom_num, port_num))
    {
        if( (port_num == 1) || (port_num == 3) )
        {
            // this is the second port in the combined connector pair
            return FBE_TRUE;
        }
    }
    // not a combined connector
    return FBE_FALSE;
}



/*!**************************************************************
 * fbe_module_mgmt_get_port_logical_number()
 ****************************************************************
 * @brief
 *  This function returns the logical number assigned to the
 *  specified port
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @author
 *  16-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_port_logical_number(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num;
     
}

/*!**************************************************************
 * fbe_module_mgmt_set_port_logical_number()
 ****************************************************************
 * @brief
 *  This function sets the logical number for the specified port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @param - logical_num - logical port number
 * @author
 *  16-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_port_logical_number(fbe_module_mgmt_t *module_mgmt, 
                                             SP_ID sp_id, fbe_u32_t iom_num, 
                                             fbe_u32_t port_num, fbe_u32_t logical_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num = logical_num;
    return;
}

/*!**************************************************************
 * fbe_module_mgmt_assign_port_logical_numbers()
 ****************************************************************
 * @brief
 *  This function loops through all ports present for this SP and
 *  assigns a logical port number if one was not previously set and 
 *  the port is not faulted.
 *
 * @param - module_mgmt - context

 * @author
 *  16-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_assign_port_logical_numbers(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id)
{
    fbe_u32_t iom_num, port_num;
    fbe_u32_t num_ports_on_iom;
    fbe_ioport_role_t port_role;
    fbe_u32_t logical_number;
    fbe_bool_t changes_made = FALSE;
    fbe_u8_t deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_u8_t config_string[FBE_DEVICE_STRING_LENGTH];

    fbe_status_t    status;
    fbe_device_physical_location_t pLocation = {0};
    fbe_u64_t device_type;
    fbe_u32_t slot;
    SPECL_PCI_DATA pci_info;
    fbe_u32_t combined_port_count;

    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        if((fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num)) &&
           fbe_module_mgmt_is_iom_power_good(module_mgmt, sp_id, iom_num) &&
           (fbe_module_mgmt_get_module_state(module_mgmt,sp_id,iom_num) != MODULE_STATE_FAULTED))
        {
            num_ports_on_iom = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt, sp_id, iom_num);
            combined_port_count = 0;
            for(port_num = 0; port_num < num_ports_on_iom; port_num++)
            {
                if( (!fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, port_num)) &&
                    (fbe_module_mgmt_get_port_state(module_mgmt, sp_id, iom_num, port_num) != FBE_PORT_STATE_FAULTED) &&
                    (fbe_module_mgmt_get_iom_group(module_mgmt, sp_id, iom_num, port_num) != FBE_IOM_GROUP_UNKNOWN))
                {
                    port_role = fbe_module_mgmt_get_port_role(module_mgmt, sp_id, iom_num, port_num);
                    logical_number = fbe_module_mgmt_get_next_available_port(module_mgmt, port_role);
                    fbe_module_mgmt_set_port_logical_number(module_mgmt, sp_id, iom_num, port_num, logical_number);
                    fbe_module_mgmt_set_persistent_port_location(module_mgmt, sp_id, iom_num, port_num);

                    if(fbe_module_mgmt_is_port_combined_port(module_mgmt, sp_id, iom_num, port_num))
                    {
                        /*
                         * This is a special case for BE0 as it was already configured prior to all 
                         * other ports.  If we are detecting a combined connector on the second port then 
                         * we missed it on the first port as we would have jumped past this port under
                         * normal circumstances.
                         */
                        combined_port_count++;
                        if(port_num == 1)
                        {
                            combined_port_count++;
                            // grab the logical number from the previous port and overwrite this ports logical number
                            fbe_module_mgmt_set_port_logical_number(module_mgmt, 
                                                                    sp_id, iom_num, 
                                                                    port_num, 
                                                                    fbe_module_mgmt_get_port_logical_number(module_mgmt,sp_id,iom_num,0));
                            pci_info = fbe_module_mgmt_get_pci_info(module_mgmt,sp_id,iom_num,port_num);
                            /*
                             * We have now configured this port as a combined port.  Set the portal number
                             * to the shared portal with port 0.
                             */
                            pci_info.function = 0;
                            fbe_module_mgmt_set_portal_number(module_mgmt,sp_id,iom_num,port_num,0);
                            fbe_module_mgmt_set_pci_info(module_mgmt,sp_id,iom_num,port_num,pci_info);

                            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "%s Combined Port Setting IOM %d, Port %d, Role %d, Logical Num %d\n",
                                                    __FUNCTION__, iom_num, port_num, 
                                                    port_role, logical_number);
                        }
                        else
                        {
                            /*
                             * This port is part of a combined set of connectors forming one logical port
                             * set the next port with the same logical number.
                             */
                            pci_info = fbe_module_mgmt_get_pci_info(module_mgmt,sp_id,iom_num,port_num);
                            /*
                             * We have now configured this port as a combined port.  Set the portal number
                             * to the shared portal 1 if ports 0 and 1 are already combined and 2 if they
                             * are not already combined.
                             */

                            if( (port_num < 2) && (combined_port_count > 0))
                            {
                                // ports 0 and 1 
                                pci_info.function = 0;
                                
                            }
                            else if((port_num >= 2) && (combined_port_count >= 2))
                            {
                                // ports 0 and 1 are already configured as a combined connector set the portal to 1
                                pci_info.function = 1;
                            }
                            else if((port_num >= 2) && (combined_port_count < 2))
                            {
                                // ports 0 and 1 are not configured as combined connectors, set the portal to 2
                                pci_info.function = 2;
                            }
                            
                            fbe_module_mgmt_set_pci_info(module_mgmt,sp_id,iom_num,port_num,pci_info);
                            fbe_module_mgmt_set_portal_number(module_mgmt,sp_id,iom_num,port_num,pci_info.function);
                            port_num++;
                            fbe_module_mgmt_set_pci_info(module_mgmt,sp_id,iom_num,port_num,pci_info);
                            fbe_module_mgmt_set_portal_number(module_mgmt,sp_id,iom_num,port_num,pci_info.function);
                            if(port_num < num_ports_on_iom)
                            {
                                fbe_module_mgmt_set_port_logical_number(module_mgmt, sp_id, iom_num, port_num, logical_number);
                                fbe_module_mgmt_set_persistent_port_location(module_mgmt, sp_id, iom_num, port_num);
                                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                FBE_TRACE_LEVEL_INFO,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "%s Combined Port Setting IOM %d, Port %d, Role %d, Logical Num %d\n",
                                                __FUNCTION__, iom_num, port_num, 
                                                port_role, logical_number);
                                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                FBE_TRACE_LEVEL_INFO,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "%s setting pci function to %d combined port count %d\n",
                                                __FUNCTION__, pci_info.function, combined_port_count);
                            }
                            else
                            {
                                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                "%s Combined port IOM %d, Port %d exceeds port count for this IO module??\n",
                                                __FUNCTION__, iom_num, port_num);
                            }
                        }
                    }
                    if(combined_port_count > 0) 
                    {
                        // There are combined ports configured ahead of this port, adjust the portal number
                        pci_info = fbe_module_mgmt_get_pci_info(module_mgmt,sp_id,iom_num,port_num);
                        if(port_num == 2)
                        {
                            pci_info.function = 1;
                        }
                        else if(port_num == 3)
                        {
                            pci_info.function = 2;
                        }
                        fbe_module_mgmt_set_pci_info(module_mgmt,sp_id,iom_num,port_num,pci_info);
                        fbe_module_mgmt_set_portal_number(module_mgmt,sp_id,iom_num,port_num,pci_info.function);
                    }
                    fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
                    pLocation.sp = sp_id;
                    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, sp_id, iom_num, &slot, &device_type);
                    pLocation.slot = slot;

                    /* Get the device string */
                    status = fbe_base_env_create_device_string(device_type, 
                                                               &pLocation, 
                                                               &(deviceStr[0]), 
                                                               FBE_DEVICE_STRING_LENGTH);

                    fbe_sprintf(&config_string[0], FBE_DEVICE_STRING_LENGTH, "%s %d %s", fbe_module_mgmt_port_role_to_string(port_role),
                            fbe_module_mgmt_get_port_logical_number(module_mgmt,sp_id,iom_num,port_num),
                            fbe_module_mgmt_port_subrole_to_string(fbe_module_mgmt_get_port_subrole(module_mgmt, sp_id, iom_num, port_num)));

                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "ModMgmt: Setting IOM index %d, %s Port %d, to %s\n",
                                            iom_num, &deviceStr[0], port_num, 
                                            &config_string[0]);

                    fbe_event_log_write(ESP_INFO_IO_PORT_CONFIGURED,
                                 NULL, 0,
                                 "%s %d %s", 
                                 &deviceStr[0],
                                 port_num,
                                 &config_string[0]);
                    changes_made = TRUE;
                }
            }
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "ModMgmt: IOM index %d cannot assign port num State%d Power %d Inserted %d\n",
                                    iom_num, fbe_module_mgmt_get_module_state(module_mgmt,sp_id,iom_num), 
                                    fbe_module_mgmt_is_iom_power_good(module_mgmt, sp_id, iom_num),
                                    fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num));
        }
    }
    return changes_made;
}

/*!**************************************************************
 * fbe_module_mgmt_get_next_available_port()
 ****************************************************************
 * @brief
 *  This function retuns the next available logical port number
 *  for the specified port role.
 *
 * @param - module_mgmt - context
 * @param - role - port role (FE vs. BE)

 * @author
 *  16-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_next_available_port(fbe_module_mgmt_t *module_mgmt, 
                                                  fbe_ioport_role_t role)
{
    fbe_bool_t used_logical_num[FBE_ESP_IO_PORT_MAX_COUNT]={FALSE};
    fbe_u32_t logical_number;
    fbe_u32_t iom_num=0, port_num=0, port=0;
        
    /*
     * Cycle through each port looking for configured ports and which
     * logical port number they are configured to.
     */
    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
        {
            if((fbe_module_mgmt_get_port_role(module_mgmt, module_mgmt->local_sp, iom_num, port_num) == role) &&
                (fbe_module_mgmt_is_port_initialized(module_mgmt, module_mgmt->local_sp, iom_num, port_num)))
            {
                logical_number = fbe_module_mgmt_get_port_logical_number(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
                used_logical_num[logical_number] = FBE_TRUE;
                if(fbe_module_mgmt_is_port_combined_port(module_mgmt, module_mgmt->local_sp, iom_num, port_num))
                {
                    // combined ports take up 2 ports and will consume the next available logical port number
                    used_logical_num[logical_number+1] = FBE_TRUE;
                }
            }
        }
    }
    

    for(port = 0; port < FBE_ESP_IO_PORT_MAX_COUNT; port++)
    {
        if(!used_logical_num[port])
        {
            /*
             * Found the first available logical
             * port number.
             */
            return port;
        }
    }
    /*
     * No available logical port numbers found, return 
     * invalid, which means the port is not configured.
     */
    return INVALID_PORT_U8;
}

/*!**************************************************************
 * fbe_module_mgmt_upgrade_ports()
 ****************************************************************
 * @brief
 *  This function loops through all ports present for this SP and
 *  assigns a logical port number if one was not previously set and 
 *  the port is not faulted.
 *
 * @param - module_mgmt - context
 *
 * @return - TRUE - changes were made to the persistent configuration.
 *
 * @author
 *  16-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_upgrade_ports(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t iom_num, port_num;
    fbe_u32_t num_ports_on_iom;
    fbe_bool_t changes_made = FALSE;
    fbe_iom_group_t new_iom_group;
    fbe_bool_t upgradeable = FALSE;
    fbe_bool_t convertable = FALSE;
    fbe_u32_t secondary_iom_num;
    fbe_u32_t temp_port_num = 0;
    HW_MODULE_TYPE uniqueID;

    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        upgradeable = fbe_module_mgmt_is_slic_upgradeable(module_mgmt, module_mgmt->local_sp, iom_num);
        convertable = fbe_module_mgmt_is_slic_convertable(module_mgmt, module_mgmt->local_sp, iom_num);
        if((fbe_module_mgmt_is_iom_inserted(module_mgmt, module_mgmt->local_sp, iom_num)) &&
           (fbe_module_mgmt_is_iom_power_good(module_mgmt, module_mgmt->local_sp, iom_num)) &&
           (upgradeable || convertable))
        {
            if(upgradeable)
            {
                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s IOM index: %d is upgrading it's SLIC.\n",
                                            __FUNCTION__, iom_num);
                // straightforward one for one slic upgrade
                num_ports_on_iom = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt, module_mgmt->local_sp, iom_num);
                for(port_num = 0; port_num < num_ports_on_iom; port_num++)
                {
                    uniqueID = module_mgmt->io_module_info[iom_num].physical_info[module_mgmt->local_sp].module_comp_info.uniqueID;
                    new_iom_group = fbe_module_mgmt_derive_iom_group(module_mgmt, module_mgmt->local_sp, iom_num, port_num, uniqueID);
                    // we are not adding new ports, just upgrading existing ones here
                    if(fbe_module_mgmt_is_port_initialized(module_mgmt,module_mgmt->local_sp,iom_num, port_num))
                    {
                        // update the io module group field for the port configuration
                        fbe_module_mgmt_set_iom_group(module_mgmt, module_mgmt->local_sp, iom_num, port_num, new_iom_group);
                        changes_made = TRUE;
                    }
                }
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s Found a 1 to 2 SLIC Conversion at IOM index %d\n",
                                            __FUNCTION__, iom_num);
                // one 4-port slic converts to 2 2-port SLICs.
                num_ports_on_iom = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt, module_mgmt->local_sp, iom_num);
                for(secondary_iom_num = 0; secondary_iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; secondary_iom_num++)
                {
                    if((fbe_module_mgmt_get_slic_type(module_mgmt,module_mgmt->local_sp, iom_num) == fbe_module_mgmt_get_slic_type(module_mgmt,module_mgmt->local_sp, secondary_iom_num)) &&
                    (!fbe_module_mgmt_is_iom_initialized(module_mgmt,module_mgmt->local_sp,secondary_iom_num)))
                    {
                        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s IOM index: %d is the secondary IOM for the 1-2 SLIC conversion\n",
                                            __FUNCTION__, secondary_iom_num);
                        break;
                    }
                }
                for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
                {
                    
                    // we are not adding new ports, just upgrading existing ones here
                    if(fbe_module_mgmt_is_port_initialized(module_mgmt,module_mgmt->local_sp,iom_num, port_num))
                    {

                        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s port_num %d is initialized\n",
                                            __FUNCTION__, port_num);

                        if(port_num >= fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt, module_mgmt->local_sp, iom_num))
                        {
                            
                            // the port exceeds the properties of the first slic so it must go onto the second available slic
                            temp_port_num = port_num - fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt, module_mgmt->local_sp, iom_num);
                            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s configuring port_num %d as %d on secondary\n",
                                            __FUNCTION__, port_num, temp_port_num);

                            uniqueID = module_mgmt->io_module_info[secondary_iom_num].physical_info[module_mgmt->local_sp].module_comp_info.uniqueID;
                            new_iom_group = fbe_module_mgmt_derive_iom_group(module_mgmt, module_mgmt->local_sp, secondary_iom_num, temp_port_num, uniqueID);
                            // update the io module group field for the port configuration
                            fbe_module_mgmt_set_iom_group(module_mgmt, module_mgmt->local_sp, secondary_iom_num, temp_port_num, new_iom_group);
                            // copy over the port logical number and subrole to the new location
                            fbe_module_mgmt_set_port_logical_number(module_mgmt,module_mgmt->local_sp, secondary_iom_num, temp_port_num, 
                                                                    fbe_module_mgmt_get_port_logical_number(module_mgmt,module_mgmt->local_sp,iom_num,port_num));
                            fbe_module_mgmt_set_port_subrole(module_mgmt,module_mgmt->local_sp, secondary_iom_num, temp_port_num,
                                                             fbe_module_mgmt_get_port_subrole(module_mgmt,module_mgmt->local_sp,iom_num,port_num));
                            fbe_module_mgmt_set_persistent_port_location(module_mgmt,module_mgmt->local_sp, secondary_iom_num, temp_port_num);
                            // re-initialize the logical number for the now empty port location 
                            fbe_module_mgmt_set_port_logical_number(module_mgmt,module_mgmt->local_sp, iom_num, port_num, INVALID_PORT_U8);
                        }
                        else
                        {
                            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s configuring port_num %d on primary\n",
                                            __FUNCTION__, port_num);
                            uniqueID = module_mgmt->io_module_info[iom_num].physical_info[module_mgmt->local_sp].module_comp_info.uniqueID;
                            new_iom_group = fbe_module_mgmt_derive_iom_group(module_mgmt, module_mgmt->local_sp, iom_num, port_num, uniqueID);
                            // update the io module group field for the port configuration
                            fbe_module_mgmt_set_iom_group(module_mgmt, module_mgmt->local_sp, iom_num, port_num, new_iom_group);
                            
                        }
                        changes_made = TRUE;
                    }
                }
            }
        }
    }
    return changes_made;
}


/*!**************************************************************
 * fbe_module_mgmt_is_slic_upgradeable()
 ****************************************************************
 * @brief
 *  This function checks that the io module group derived for the
 *  current SLIC is compatible with the current IO module group
 *  assigned for this slot.
 *
 * @param - module_mgmt - context
 *
 * @return - TRUE - SLIC is upgradeable
 *
 * @author
 *  16-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_slic_upgradeable(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u32_t iom_num)
{
    fbe_u32_t port_num;
    fbe_iom_group_t current_iom_group, new_iom_group;
    fbe_bool_t is_upgradeable = FALSE;
    HW_MODULE_TYPE uniqueID;

    // if there are no configured ports for this slic then it can't be upgraded
    if(!fbe_module_mgmt_is_iom_initialized(module_mgmt,sp,iom_num))
    {
        return is_upgradeable;
    }

    for(port_num=0;port_num<MAX_IO_PORTS_PER_MODULE;port_num++)
    {
        /*
         * Derive the new iom_group based on the inserted slic and get the current
         * iom_group from the configured data loaded from persistent storage.
         */
        if(fbe_module_mgmt_is_port_initialized(module_mgmt, sp, iom_num, port_num))
        {
            uniqueID = module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.uniqueID;
            new_iom_group = fbe_module_mgmt_derive_iom_group(module_mgmt, sp, iom_num, port_num, uniqueID);
            current_iom_group = fbe_module_mgmt_get_iom_group(module_mgmt, sp, iom_num, port_num);
        
            // Valid SLIC upgrade paths for this release
            if( (current_iom_group == FBE_IOM_GROUP_E) &&
                (new_iom_group == FBE_IOM_GROUP_I) )
            {
                is_upgradeable = TRUE;
            }
            else if((current_iom_group == FBE_IOM_GROUP_E) &&
                (new_iom_group == FBE_IOM_GROUP_J))
            {
                is_upgradeable = TRUE;
            }
            else if((current_iom_group == FBE_IOM_GROUP_I) &&
                (new_iom_group == FBE_IOM_GROUP_J))
            {
                is_upgradeable = TRUE;
            }
            else if((current_iom_group == FBE_IOM_GROUP_J) &&
                (new_iom_group == FBE_IOM_GROUP_I))
            {
                is_upgradeable = TRUE;
            }
            else if((current_iom_group == FBE_IOM_GROUP_D) &&
                    (new_iom_group == FBE_IOM_GROUP_M))
            {
                is_upgradeable = TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }
    return is_upgradeable;
}



/*!**************************************************************
 * fbe_module_mgmt_is_slic_convertable()
 ****************************************************************
 * @brief
 *  This function checks that the current configured IOM GROUP
 *  is convertable to 2 separate SLICs and validates that the
 *  ports are available for this operation.
 *
 * @param - module_mgmt - context
 *
 * @return - TRUE - SLIC is convertable
 *
 * @author
 *  16-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_slic_convertable(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u32_t iom_num)
{
    fbe_iom_group_t current_iom_group;
    fbe_bool_t is_convertable = FBE_FALSE;
    fbe_u32_t temp_iom_num = 0;
    fbe_module_slic_type_t new_slic_type;

     // if there are no configured ports for this slic then it can't be upgraded
    if(!fbe_module_mgmt_is_iom_initialized(module_mgmt,sp,iom_num))
    {
        return is_convertable;
    }

   
    new_slic_type = fbe_module_mgmt_get_slic_type(module_mgmt,sp, iom_num);  
    current_iom_group = fbe_module_mgmt_get_iom_group(module_mgmt,sp,iom_num,0);  
    if( ((current_iom_group == FBE_IOM_GROUP_G) && (new_slic_type == FBE_SLIC_TYPE_ISCSI_COPPER)) ||
        ((current_iom_group == FBE_IOM_GROUP_G) && (new_slic_type == FBE_SLIC_TYPE_ISCSI_10G_V2)))
    {
        for(temp_iom_num = 0; temp_iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; temp_iom_num++)
        {
            if( (fbe_module_mgmt_get_slic_type(module_mgmt,sp, temp_iom_num) == new_slic_type) &&
                (!fbe_module_mgmt_is_iom_initialized(module_mgmt,module_mgmt->local_sp,temp_iom_num)))
            {
                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s can convert to IOM %d and IOM %d\n",
                                            __FUNCTION__, iom_num, temp_iom_num);
                is_convertable = FBE_TRUE;
            }
        }
    }
    return is_convertable;

}

/*!***************************************************************
 * fbe_module_mgmt_load_mgmt_port_persistent_data()
 ****************************************************************
 * @brief
 *  This is function load the Mgmt port speed data from file.
 *
 * @param mgmt_module - Ptr to  module_mgmt
 *
 * @return - void - 
 *
 * @author 
 *  2-June-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void 
fbe_module_mgmt_load_mgmt_port_persistent_data(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t   nbytes = 0;
    fbe_u32_t   mgmt_id = 0;
    fbe_char_t  persistent_storage_location[256];
    fbe_file_handle_t   fp = NULL;
    fbe_mgmt_port_config_info_t *persistent_data_p;

    fbe_zero_memory(&persistent_storage_location, 256);
    fbe_module_mgmt_get_esp_lun_location(persistent_storage_location);

    fp = fbe_file_open(persistent_storage_location, FBE_FILE_RDONLY, 0, NULL);
    if(FBE_FILE_INVALID_HANDLE == fp)
    {
         fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not access persistent storage.\n",
                                __FUNCTION__);
         fp = fbe_file_creat(persistent_storage_location, FBE_FILE_RDWR);
         if(fp == FBE_FILE_INVALID_HANDLE)
         {
             fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Could not create persistent storage.\n",
                                __FUNCTION__);
             return;
         }
    }

    persistent_data_p = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_mgmt_port_config_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP);
    fbe_set_memory(persistent_data_p, 0, (sizeof(fbe_mgmt_port_config_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP));

    fbe_file_lseek(fp, FBE_MGMT_PORT_PERSISTENT_DATA_OFFSET, 0);
    nbytes = fbe_file_read (fp, persistent_data_p, (sizeof(fbe_mgmt_port_config_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP), NULL);
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != sizeof(fbe_mgmt_port_config_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not read from persistent storage nbytes:%d \n",
                                __FUNCTION__, nbytes);
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, persistent_data_p);
        fbe_file_close(fp);
        return;
    }
    /* Copy the persistence data to Mgmt Module*/
    for(mgmt_id = 0; mgmt_id < FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP; mgmt_id++)
    {
        module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortAutoNeg = persistent_data_p[mgmt_id].mgmtPortAutoNeg;
        module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortSpeed = persistent_data_p[mgmt_id].mgmtPortSpeed;
        module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortDuplex = persistent_data_p[mgmt_id].mgmtPortDuplex;
    }
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, persistent_data_p);
    fbe_file_close(fp);
    return;
}
/********************************************************
 * end fbe_module_mgmt_load_mgmt_port_persistent_data() 
 ********************************************************/
/*!***************************************************************
 * fbe_module_mgmt_store_mgmt_port_speed_persistent_data()
 ****************************************************************
 * @brief
 *  This is function stored the Mgmt port speed data on file.
 *
 * @param mgmt_module - Ptr to  module_mgmt
 *
 * @return - void - 
 *
 * @author 
 *  2-June-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void
fbe_module_mgmt_store_mgmt_port_speed_persistent_data(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t   mgmt_id;
    fbe_u32_t   nbytes = 0;
    fbe_char_t  persistent_storage_location[256];
    fbe_file_handle_t   fp = NULL;
    fbe_mgmt_port_config_info_t  *persistent_data_p;

    /* Allocate and zero out a temporary buffer to save all the data in one location in memory. */
    persistent_data_p = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_mgmt_port_config_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP);
    fbe_set_memory(persistent_data_p, 0, (sizeof(fbe_mgmt_port_config_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP));
    fbe_set_memory(persistent_storage_location, 0, 256);

    /* Get the ESP file */
    fbe_module_mgmt_get_esp_lun_location(persistent_storage_location);

    /* Copy Mgmt port persistence data */
    for(mgmt_id =0; mgmt_id < FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP; mgmt_id++)
    {
        persistent_data_p[mgmt_id].mgmtPortAutoNeg = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortAutoNeg;
        persistent_data_p[mgmt_id].mgmtPortDuplex = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortDuplex;
        persistent_data_p[mgmt_id].mgmtPortSpeed = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortSpeed;
    }
    
    /* Write the buffer out to the persistent storage */
    fp = fbe_file_open(persistent_storage_location, FBE_FILE_WRONLY | FBE_FILE_APPEND, 0, NULL);
    if(FBE_FILE_INVALID_HANDLE == fp)
    {
         fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not access persistent storage, creating new storage.\n",
                                __FUNCTION__);
         /* The open failed, the file must not be there, so lets create a new file. */
         fp = fbe_file_open(persistent_storage_location, FBE_FILE_CREAT, 0, NULL);
         if(FBE_FILE_INVALID_HANDLE == fp)
         {
             fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Could not access persistent storage, creation failed.\n",
                                __FUNCTION__);
             fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, persistent_data_p);
             return;
         }
         else
         {
             fbe_file_close(fp);
             /* Re-open the newly created file for writing. */
             fp = fbe_file_open(persistent_storage_location, FBE_FILE_WRONLY | FBE_FILE_APPEND, 0, NULL);
             if(FBE_FILE_INVALID_HANDLE == fp)
             {
                 fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Could not access persistent storage for writing after creation.\n",
                                        __FUNCTION__);
                 fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, persistent_data_p);
                 return;
             }
         }
    }
    
    /* Initial part of file contain the IO PORT data */
    fbe_file_lseek(fp, FBE_MGMT_PORT_PERSISTENT_DATA_OFFSET, 0);
    nbytes = fbe_file_write(fp, persistent_data_p, (sizeof(fbe_mgmt_port_config_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP), NULL);

    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != sizeof(fbe_mgmt_port_config_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not write to persistent storage nbytes:%d \n",
                                __FUNCTION__, nbytes);
    }
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, persistent_data_p);
    fbe_file_close(fp);
    fbe_module_mgmt_handle_file_write_completion((fbe_base_object_t *)module_mgmt, FBE_MODULE_MGMT_FLUSH_THREAD_DELAY_IN_MSECS);
}
/**************************************************************
 * end fbe_module_mgmt_store_mgmt_port_speed_persistent_data() 
 *************************************************************/ 

//End Temporary Hack Code



/*!**************************************************************
 * fbe_module_mgmt_get_pci_info()
 ****************************************************************
 * @brief
 *  This function returns the pci address information for the
 *  specified port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
SPECL_PCI_DATA fbe_module_mgmt_get_pci_info(fbe_module_mgmt_t *module_mgmt, 
                                            SP_ID sp_id, fbe_u32_t iom_num, 
                                            fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function;
}

/*!**************************************************************
 * fbe_module_mgmt_set_pci_info()
 ****************************************************************
 * @brief
 *  This function sets the pci address information for the
 *  specified port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_pci_info(fbe_module_mgmt_t *module_mgmt, 
                                            SP_ID sp_id, fbe_u32_t iom_num, 
                                            fbe_u32_t port_num,
                                            SPECL_PCI_DATA pci_info)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    fbe_copy_memory(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].pciData,&pci_info, sizeof(SPECL_PCI_DATA));
    return;
}
/******************************************************
 * end fbe_module_mgmt_get_pci_info() 
 ******************************************************/
/*!**************************************************************
 * fbe_module_mgmt_get_pci_info()
 ****************************************************************
 * @brief
 *  This function returns the global port index to the port information
 *  given a pci bus,device,function
 *
 * @param - module_mgmt - context
 * @param - bus
 * @param - device
 * @param - function
 * 
 * @return - port_index - index to port information for the specified port
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_port_index_from_pci_info(fbe_module_mgmt_t *module_mgmt,
                                                       fbe_u32_t bus, fbe_u32_t device, fbe_u32_t function)
{
    fbe_u32_t port_index;
    SP_ID sp_id;

    sp_id = module_mgmt->local_sp;

    for(port_index = 0; port_index < FBE_ESP_IO_PORT_MAX_COUNT; port_index++)
    {
        if( (module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.bus == bus) &&
            (module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.device == device) &&
            (module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function == function))
        {
            return port_index;
        }
    }
    // if we made it here we didn't find a matching port return INVALID
    return INVALID_PORT_U8;

}

fbe_u32_t fbe_module_mgmt_get_iom_num_from_port_index(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port_index)
{
    SP_ID sp_id;
    fbe_u64_t device_type;
    fbe_u32_t slot;
    fbe_u32_t iom_num;

    sp_id = module_mgmt->local_sp;

    device_type = module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].deviceType;
    slot = module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].slotNumOnBlade;
    
    iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(device_type, slot);
    return iom_num;
}

fbe_u32_t fbe_module_mgmt_get_port_num_from_port_index(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port_index)
{
    SP_ID sp_id;
    fbe_u32_t port_num;

    sp_id = module_mgmt->local_sp;
    port_num = module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].portNumOnModule;
    
    return port_num;
}

/*!**************************************************************
 * fbe_module_mgmt_get_port_link_state()
 ****************************************************************
 * @brief
 *  This function returns the condition of the SFP in the
 *  specified port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_port_link_state_t fbe_module_mgmt_get_port_link_state(fbe_module_mgmt_t *module_mgmt, 
                                                                     SP_ID sp_id, fbe_u32_t iom_num,
                                                                     fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state;
}

/*!**************************************************************
 * fbe_module_mgmt_set_port_link_state()
 ****************************************************************
 * @brief
 *  This function sets the specified port with the link state
 *  provided.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @param - new_link_state - link state to set this port to
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_port_link_state(fbe_module_mgmt_t *module_mgmt,
                                                    SP_ID sp_id, fbe_u32_t iom_num,
                                                    fbe_u32_t port_num, fbe_port_link_state_t new_link_state)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state = new_link_state;
    return;
}

/*!**************************************************************
 * fbe_module_mgmt_set_port_link_info()
 ****************************************************************
 * @brief
 *  This function sets the specified port with the link information
 *  provided.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * @param - new_link_info - link information to set this port to
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_set_port_link_info(fbe_module_mgmt_t *module_mgmt,
                                                    SP_ID sp_id, fbe_u32_t iom_num,
                                                    fbe_u32_t port_num, fbe_port_link_information_t *new_link_info)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    fbe_copy_memory(&module_mgmt->io_port_info[port_index].port_physical_info.link_info, new_link_info, sizeof(fbe_port_link_information_t));
    return;
}

/*!**************************************************************
 * fbe_module_mgmt_get_port_link_info()
 ****************************************************************
 * @brief
 *  This function sets the specified pointer to the location of
 *  the specified port's link information
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 * 
 * @return - new_link_info - link information for this port
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_port_link_information_t fbe_module_mgmt_get_port_link_info(fbe_module_mgmt_t *module_mgmt,
                                                    SP_ID sp_id, fbe_u32_t iom_num,
                                                    fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.link_info;
}

/*!**************************************************************
 * fbe_module_mgmt_get_port_sfp_condition()
 ****************************************************************
 * @brief
 *  This function returns the condition of the SFP in the
 *  specified port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_port_sfp_condition_type_t fbe_module_mgmt_get_port_sfp_condition(fbe_module_mgmt_t *module_mgmt, 
                                                                     SP_ID sp_id, fbe_u32_t iom_num,
                                                                     fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.sfp_info.condition_type;
}

/*!**************************************************************
 * fbe_module_mgmt_get_port_sfp_subcondition()
 ****************************************************************
 * @brief
 *  This function returns the subcondition of the SFP in the
 *  specified port.
 *
 * @param - module_mgmt - context
 * @param - sp_id - sp identifier
 * @param - iom_num - io module number
 * @param - port_num - port number
 *
 * @author
 *  14-Sep-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_port_sfp_sub_condition_type_t fbe_module_mgmt_get_port_sfp_subcondition(fbe_module_mgmt_t *module_mgmt,
                                                    SP_ID sp_id, fbe_u32_t iom_num,
                                                    fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    return module_mgmt->io_port_info[port_index].port_physical_info.sfp_info.condition_additional_info;
}

/*!**************************************************************
 * fbe_module_mgmt_set_persist_ports_and_reboot()
 ****************************************************************
 * @brief
 *  This function returns the subcondition of the SFP in the
 *  specified port.
 *
 * @param - module_mgmt - context
 * 
 * @return - fbe_status_t - successfully set and reboot or not
 * 
 * @author
 *  11-Nov-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_set_persist_ports_and_reboot(fbe_module_mgmt_t *module_mgmt)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    //set the persist flag in the registry
    status = fbe_module_mgmt_set_persist_port_info(module_mgmt, TRUE);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    //reboot this sp
    status = fbe_base_env_reboot_sp((fbe_base_environment_t *) module_mgmt, module_mgmt->local_sp, FALSE, FALSE, FBE_TRUE);
    return status;
}

/*!**************************************************************
 * fbe_module_mgmt_set_upgrade_ports_and_reboot()
 ****************************************************************
 * @brief
 *  This function returns the subcondition of the SFP in the
 *  specified port.
 *
 * @param - module_mgmt - context
 * 
 * @return - fbe_status_t - successfully set and reboot or not
 * 
 * @author
 *  11-Nov-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_set_upgrade_ports_and_reboot(fbe_module_mgmt_t *module_mgmt)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_module_mgmt_upgrade_type_t upgrade_type;
    fbe_module_mgmt_cmi_msg_t cmi_msg;

    // check if we are currently performing a SLIC upgrade
    if(module_mgmt->slic_upgrade != FBE_MODULE_MGMT_SLIC_UPGRADE_PEER_SP)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "ModMgmt: SLIC Upgrade Recvd, setting upgrade flag in Registry\n");
        // set the upgrade flag in the registry
        upgrade_type = FBE_MODULE_MGMT_SLIC_UPGRADE_LOCAL_SP;
        status = fbe_module_mgmt_set_slics_marked_for_upgrade(&upgrade_type);
        if(status != FBE_STATUS_OK)
        {
            return status;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "ModMgmt: SLIC Upgrade Recvd, upgrade in progress on peer, notify and hold in reset.\n");
    }

    // send a message to the peer sp that we are performing a SLIC upgrade
    cmi_msg.base_environment_message.opcode = FBE_MODULE_MGMT_SLIC_UPGRADE_MSG;

    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)module_mgmt,
                                           sizeof(fbe_module_mgmt_cmi_msg_t),
                                          (fbe_cmi_message_t)&cmi_msg);
    

    // reboot this SP and hold it in reset
    status = fbe_base_env_reboot_sp((fbe_base_environment_t *) module_mgmt, module_mgmt->local_sp, FALSE, TRUE, FBE_TRUE);

    return status;
}


/*!**************************************************************
 * fbe_module_mgmt_reboot_to_allow_port_replace()
 ****************************************************************
 * @brief
 *  This function reboots and holds in reset to allow port replace.
 *
 * @param - module_mgmt - context
 * 
 * @return - fbe_status_t - successfully  reboot
 * 
 * @author
 *  11-Nov-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_reboot_to_allow_port_replace(fbe_module_mgmt_t *module_mgmt)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    // reboot this SP and hold it in reset
    status = fbe_base_env_reboot_sp((fbe_base_environment_t *) module_mgmt, module_mgmt->local_sp, FALSE, TRUE, FBE_TRUE);

    return status;
}


/*!**************************************************************
 * fbe_module_mgmt_convert_hellcat_lite_to_sentry()
 ****************************************************************
 * @brief
 *  This function relocates the 4 FC ports on a hellcat lite to
 *  the first SLIC slot of a hellcat or lightning system to aid
 *  in the in-family conversion process.
 *
 * @param - module_mgmt - context
 * 
 * @return - fbe_status_t - successfully set and reboot or not
 * 
 * @author
 *  11-Nov-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_convert_hellcat_lite_to_sentry(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t iom_num = 0;
    fbe_u32_t port_num = 0;
    fbe_u32_t port_index = 0;
    fbe_u32_t first_fc_port = INVALID_PORT_U8;
    SP_ID sp;
    fbe_status_t status = FBE_STATUS_OK;

    sp = module_mgmt->local_sp;
    for(port_num = 0;port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
    {
        /*
         * Move all the FC mezzanine ports to a SLIC in slot 0.
         * SAS ports stay on the mezzanine.
         */
        port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
        if(module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.iom_group == FBE_IOM_GROUP_D)
        {
            if(module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.logical_num != INVALID_PORT_U8)
            {
                if(first_fc_port == INVALID_PORT_U8)
                {
                    first_fc_port = port_num;
                }
                fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Moving Mezzanine port:%d to IOM 0 port:%d.\n",
                                    __FUNCTION__, 
                                    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port,
                                    (module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port - first_fc_port) );
                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.type_32_bit = FBE_DEVICE_TYPE_IOMODULE;
                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot = 0;
                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port = 
                    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port - first_fc_port;
            }
        }
    }

    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                            (fbe_base_object_t*)module_mgmt,
                                            FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set WRITE_PERSISTENT_DATA condition.\n",
                                __FUNCTION__);
    }

    /*
     * Clear out and re-load the persistent data to get the configuration loaded into the correct 
     * array offsets. 
     */
    fbe_module_mgmt_initialize_configured_port_info(module_mgmt);
    
    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                            (fbe_base_object_t*)module_mgmt,
                                            FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set READ_PERSISTENT_DATA condition.\n",
                                __FUNCTION__);
    }

    return status;
}

/*!**************************************************************
 * fbe_module_mgmt_convert_sentry_to_argonaut()
 ****************************************************************
 * @brief
 *  This function relocates the SAS and FC ports on the
 *  mezzanine of a sentry system to SLIC slots on an argonaut
 *  platform.  Any ports configured on slic slots of the
 *  sentry system are shifted over so that mezzanine ports can
 *  occupy slots 0 and 1.
 *
 * @param - module_mgmt - context
 * 
 * @return - fbe_status_t - successfully set and reboot or not
 * 
 * @author
 *  11-Nov-2010:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_convert_sentry_to_argonaut(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t iom_num = 0;
    fbe_u32_t port_num = 0;
    fbe_u32_t port_index = 0;
    fbe_u32_t first_fc_port = INVALID_PORT_U8;
    SP_ID sp;
    fbe_status_t status = FBE_STATUS_OK;

    sp = module_mgmt->local_sp;

    /*
     * Move io module data for the mezzanine and 2 SLIC slots that were 
     * on the sentry to new locations on the argonaut. 
     */
    for(iom_num = 0; iom_num <=2; iom_num++)
    {
        for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
        {
            port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    
            if(module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.logical_num != INVALID_PORT_U8)
            {
                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.type_32_bit = FBE_DEVICE_TYPE_IOMODULE;
                if(iom_num < FBE_ESP_MEZZANINE_MAX_COUNT)//mezzanine ports
                {
                    /*
                     * Mezzanine SAS ports are moved to slot 0.  The first 3 FC mezzanine ports are moved to slot 1. 
                     * FC FE 3 (a data mover port) is moved to the first port in slot 4. 
                     */
                    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.type_32_bit = FBE_DEVICE_TYPE_IOMODULE;
                    if(module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.iom_group == FBE_IOM_GROUP_C) // SAS ports
                    {
                        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Moving Mezzanine %s%d Port:%d to SLIC in Slot 0 Port %d.\n",
                                __FUNCTION__, 
                                fbe_module_mgmt_port_role_to_string(module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_role),
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.logical_num,
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port,
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port);
                        module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot = 0;
                    }
                    else // other ports (FC)
                    {
                        if(first_fc_port == INVALID_PORT_U8)
                        {
                            first_fc_port = port_index;
                        }
                        if( (module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_role == IOPORT_PORT_ROLE_FE) &&
                            (module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.logical_num == 3) )
                        {
                            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Moving Mezzanine FE 3 to SLIC in Slot 4.\n",
                                __FUNCTION__);
                            /* FE 3 is moved to slot 4 port 0*/
                            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot = 4;
                            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port = 0;
                        }
                        else
                        {
                            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Moving Mezzanine %s%d Port:%d to SLIC in Slot 1 Port %d.\n",
                                __FUNCTION__, 
                                fbe_module_mgmt_port_role_to_string(module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_role),
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.logical_num,
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port,
                                (module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port - first_fc_port));

                            /* Other ports are moved to slot 1 */
                            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot = 1;
                            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port = 
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port - first_fc_port;
                        }
                    }
                }
                else // slic ports
                {
                    /* 
                     * Ports in existing SLIC slots are shifted over 2 slots, taking up slots
                     * 2 and 3. 
                     */ 
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Moving %s%d on SLIC:%d Port:%d to SLIC in Slot %d Port %d.\n",
                                __FUNCTION__, 
                                fbe_module_mgmt_port_role_to_string(module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_role),
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.logical_num,
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot,
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port,
                                (module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot + 
                                    FBE_MODULE_MGMT_IN_FAMILY_CONVERSION_SLIC_SLOT_SHIFT),
                                module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port);
                    module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot = 
                        module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot + 
                            FBE_MODULE_MGMT_IN_FAMILY_CONVERSION_SLIC_SLOT_SHIFT;
                }
            }
        }
    }

    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                            (fbe_base_object_t*)module_mgmt,
                                            FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set WRITE_PERSISTENT_DATA condition.\n",
                                __FUNCTION__);
    }

    /*
     * Clear out and re-load the persistent data to get the configuration loaded into the correct 
     * array offsets. 
     */
    fbe_module_mgmt_initialize_configured_port_info(module_mgmt);

    status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                            (fbe_base_object_t*)module_mgmt,
                                            FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set READ_PERSISTENT_DATA condition.\n",
                                __FUNCTION__);
    }

    return status;
}

/*!**************************************************************
 * fbe_module_mgmt_initialize_configured_port_info()
 ****************************************************************
 * @brief
 *  This function initialized the port configuration information
 *  in memory.
 *
 * @param - module_mgmt - context
 * 
 * @return - none
 * 
 * @author
 *  25-Mar-2010:  Created. bphilbin 
 *
 ****************************************************************/
void fbe_module_mgmt_initialize_configured_port_info(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t port_index;
    SP_ID sp;
    for(port_index = 0; port_index < FBE_ESP_IO_PORT_MAX_COUNT; port_index++)
    {
        for(sp = SP_A; sp < SP_ID_MAX; sp++)
        {
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.logical_num = INVALID_PORT_U8;
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_subrole = FBE_PORT_SUBROLE_UNINTIALIZED;
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.iom_group = FBE_IOM_GROUP_UNKNOWN;
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.io_enclosure_number = INVALID_ENCLOSURE_NUMBER;
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.type_32_bit = FBE_DEVICE_TYPE_INVALID;
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.slot = INVALID_MODULE_NUMBER;
            module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info.port_location.port = INVALID_PORT_U8;
        }
    }
    return;
}

/*!**************************************************************
 * fbe_module_mgm_set_port_list()
 ****************************************************************
 * @brief
 *  This function validates the new configuration received from
 *  a set port command and updates the configured port information
 *  if the new settings are ok.
 *
 * @param - module_mgmt - context
 * @param - num_ports - number of ports to configure
 * @param - new_port_config - pointer to a list of new port config
 * @param - overwrite - overwrite of existing port config allowed 
 * 
 * @return - fbe_status_t
 * 
 * @author
 *  09-Sep-2011:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgm_set_port_list(fbe_module_mgmt_t *module_mgmt, 
                                          fbe_u32_t num_ports, 
                                          fbe_io_port_persistent_info_t *new_port_config,
                                          fbe_bool_t overwrite)
{
    fbe_status_t status;
    fbe_io_port_persistent_info_t temp_port_config[FBE_ESP_IO_PORT_MAX_COUNT];
    fbe_u32_t port_index;
    fbe_u32_t new_port_config_index;
    fbe_u32_t iom_num;
    SP_ID sp;
    fbe_ioport_role_t port_role;
    fbe_u32_t logical_num;
    fbe_u8_t persistent_port_index;
    fbe_u8_t port_num;
    fbe_io_port_persistent_info_t *persistent_data_p;

    sp = module_mgmt->local_sp;

    // copy old config into a temp config list
    for(port_index = 0; port_index < FBE_ESP_IO_PORT_MAX_COUNT; port_index++)
    {
        fbe_copy_memory(&temp_port_config[port_index], 
                        &module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info,
                        sizeof(fbe_io_port_persistent_info_t));
    }

    // add the new port settings to this list
    for(new_port_config_index = 0; new_port_config_index < num_ports; new_port_config_index++)
    {
        iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(new_port_config[new_port_config_index].port_location.type_32_bit, 
                                                                        new_port_config[new_port_config_index].port_location.slot);
        port_index = fbe_module_mgmt_get_io_port_index(iom_num, new_port_config[new_port_config_index].port_location.port);
        
        
        if(temp_port_config[port_index].logical_num == INVALID_PORT_U8)
        {
            // This port is not already configured, save the config
            fbe_copy_memory(&temp_port_config[port_index], 
                            &new_port_config[new_port_config_index], 
                            sizeof(fbe_io_port_persistent_info_t));
        }
        else if( (temp_port_config[port_index].logical_num != INVALID_PORT_U8) && (!overwrite))
        {
            /*
             * We cannot overwrite the already configured port, trace the conflict and return an error.
             */
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Cannot overwrite configuration for %s Slot:%d Port:%d.\n",
                                  __FUNCTION__, 
                                  fbe_module_mgmt_device_type_to_string(temp_port_config[port_index].port_location.type_32_bit),
                                  temp_port_config[port_index].port_location.slot,
                                  temp_port_config[port_index].port_location.port);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            // Overwrite the existing setting
            fbe_copy_memory(&temp_port_config[port_index], 
                            &new_port_config[new_port_config_index], 
                            sizeof(fbe_io_port_persistent_info_t));
        }
    }

    /*
     * Check for duplicate logical port entries
     */
    port_role = FBE_PORT_ROLE_BE;
    for(logical_num = 0; logical_num < 8; logical_num++)
    {
        status = fbe_module_mgmt_check_duplicate_port_in_list(module_mgmt, temp_port_config, logical_num, port_role);
        if(status != FBE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    port_role = FBE_PORT_ROLE_FE;
    for(logical_num = 0; logical_num < 20; logical_num++)
    {
        status = fbe_module_mgmt_check_duplicate_port_in_list(module_mgmt, temp_port_config, logical_num, port_role);
        if(status != FBE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if(fbe_module_mgmt_check_overlimit_port_in_list(module_mgmt,temp_port_config) != FBE_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Update the port configuration with the new combined configuration.
     */
    for(port_index = 0; port_index < FBE_ESP_IO_PORT_MAX_COUNT; port_index++)
    {
        fbe_copy_memory(&module_mgmt->io_port_info[port_index].port_logical_info[sp].port_configured_info,
                        &temp_port_config[port_index], 
                        sizeof(fbe_io_port_persistent_info_t));
    }

    /*
     * Write the configuration out to disk
     */
    // disabling file based persistence to instead use persistent service

    persistent_port_index = 0;
    persistent_data_p = (fbe_io_port_persistent_info_t *) ((fbe_base_environment_t *)module_mgmt)->my_persistent_data;
    for(port_num = 0; port_num < FBE_ESP_IO_PORT_MAX_COUNT; port_num++)
    {
        /* This is not a 1 to 1 mapping, we have a lot of possible port data but only a certain number of ports that can be
         * configured.  This will make use of all persistent data slots when possible.
         */
        if( (module_mgmt->io_port_info[port_num].port_physical_info.port_comp_info[module_mgmt->local_sp].present) ||
            (module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info.logical_num != INVALID_PORT_U8))
        {
            fbe_copy_memory(&persistent_data_p[persistent_port_index],
                        &module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info,
                        sizeof(fbe_io_port_persistent_info_t));
            persistent_port_index++;
        }
    }
    status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)module_mgmt);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s persistent write error, status: 0x%X",
                              __FUNCTION__, status);
    }

    /*
     * Set the flag indicating this SP needs to be rebooted.
     */
    module_mgmt->reboot_sp = REBOOT_LOCAL;

    status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_CHECK_REGISTRY);
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_module_mgmt_check_duplicate_port_in_list()
 ****************************************************************
 * @brief
 *  This function checks the specified port configuration list for
 *  the specified duplicate logical entries.
 *
 * @param - module_mgmt - context
 * @param - port_config_list - pointer to a list of port config info
 * @param - logical_num - port logical number
 * @param - port_role - port role fe/be/unc 
 * 
 * @return - fbe_status_t
 * 
 * @author
 *  09-Sep-2011:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_check_duplicate_port_in_list(fbe_module_mgmt_t *module_mgmt, 
                                                          fbe_io_port_persistent_info_t *port_config_list, 
                                                          fbe_u32_t logical_num, 
                                                          fbe_ioport_role_t port_role)
{
    fbe_u32_t port_index;
    fbe_bool_t port_already_found = FALSE;

    for(port_index = 0; port_index < FBE_ESP_IO_PORT_MAX_COUNT; port_index++)
    {
        if(port_already_found &&
           (port_config_list[port_index].logical_num == logical_num) &&
           (port_config_list[port_index].port_role == port_role))
        {
            // duplicate entry detected trace and return an error
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s duplicate %s %d detected in port_config_list.\n",
                              __FUNCTION__, 
                              fbe_module_mgmt_port_role_to_string(port_role),
                              logical_num);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else if((port_config_list[port_index].logical_num == logical_num) &&
                (port_config_list[port_index].port_role == port_role))
        {
            port_already_found = TRUE;
        }
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_module_mgmt_check_overlimit_port_in_list()
 ****************************************************************
 * @brief
 *  This function checks the specified port configuration list for
 *  the overlimit port entries.
 *
 * @param - module_mgmt - context
 * @param - port_config_list - pointer to a list of port config info
 * 
 * @return - fbe_status_t
 * 
 * @author
 *  09-Sep-2011:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_check_overlimit_port_in_list(fbe_module_mgmt_t *module_mgmt,
                                                         fbe_io_port_persistent_info_t *port_config_list)
{
    fbe_u32_t port_index;
    fbe_u32_t sas_be_count=0, sas_fe_count=0, iscsi_1g_count=0, iscsi_10g_count=0, fcoe_count=0, fc_count=0;
    fbe_status_t status = FBE_STATUS_OK;
    /*
     * Check the combined old and new port configuration does 
     * not violate platform limits.
     */
    for(port_index = 0; port_index < FBE_ESP_IO_PORT_MAX_COUNT; port_index++)
    {
        if(port_config_list[port_index].logical_num != INVALID_PORT_U8)
        {
            // configured port check which limit it falls under
            if(port_config_list[port_index].iom_group == FBE_IOM_GROUP_C)
            {
                sas_be_count++;
            }
            else if(port_config_list[port_index].iom_group == FBE_IOM_GROUP_H)
            {
                sas_fe_count++;
            }
            else if((port_config_list[port_index].iom_group == FBE_IOM_GROUP_D) ||
                    (port_config_list[port_index].iom_group == FBE_IOM_GROUP_M))
            {
                fc_count++;
            }
            else if(port_config_list[port_index].iom_group == FBE_IOM_GROUP_F)
            {
                fcoe_count++;
            }

            else if ((port_config_list[port_index].iom_group == FBE_IOM_GROUP_E) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_J) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_I) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_L) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_P) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_Q) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_R) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_S))
            {
                iscsi_10g_count++;
            }
            else if ((port_config_list[port_index].iom_group == FBE_IOM_GROUP_G) ||
                     (port_config_list[port_index].iom_group == FBE_IOM_GROUP_O))
            {
                iscsi_1g_count++;
            }
        }
    }

    if(sas_be_count > fbe_module_mgmt_get_platform_limit(module_mgmt, IO_CONTROLLER_PROTOCOL_SAS, FBE_PORT_ROLE_BE, FBE_SLIC_TYPE_6G_SAS_3))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s attempted to configure overlimt SAS BE ports count %d.\n",
                              __FUNCTION__, sas_be_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if(sas_fe_count > fbe_module_mgmt_get_platform_limit(module_mgmt, IO_CONTROLLER_PROTOCOL_SAS, FBE_PORT_ROLE_FE, FBE_SLIC_TYPE_6G_SAS_2))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s attempted to configure overlimt SAS FE ports count %d.\n",
                              __FUNCTION__, sas_fe_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if(fc_count > fbe_module_mgmt_get_platform_limit(module_mgmt, IO_CONTROLLER_PROTOCOL_FIBRE, FBE_PORT_ROLE_FE, FBE_SLIC_TYPE_FC_8G))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s attempted to configure overlimt FC FE ports count %d.\n",
                              __FUNCTION__, fc_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if(fcoe_count > fbe_module_mgmt_get_platform_limit(module_mgmt, IO_CONTROLLER_PROTOCOL_FCOE, FBE_PORT_ROLE_FE, FBE_SLIC_TYPE_FCOE))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s attempted to configure overlimt FCoE FE ports count %d.\n",
                              __FUNCTION__, fcoe_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if(iscsi_1g_count > fbe_module_mgmt_get_platform_limit(module_mgmt, IO_CONTROLLER_PROTOCOL_ISCSI, FBE_PORT_ROLE_FE, FBE_SLIC_TYPE_4PORT_ISCSI_1G))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s attempted to configure overlimt iSCSI 1g FE ports count %d.\n",
                              __FUNCTION__, iscsi_1g_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if(iscsi_10g_count > fbe_module_mgmt_get_platform_limit(module_mgmt, IO_CONTROLLER_PROTOCOL_ISCSI, FBE_PORT_ROLE_FE, FBE_SLIC_TYPE_ISCSI_10G_V2))
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s attempted to configure overlimt iSCSI 10g FE ports count %d.\n",
                              __FUNCTION__, iscsi_10g_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*!**************************************************************
 * fbe_module_mgmt_remove_all_ports()
 ****************************************************************
 * @brief
 *  This function destroys the persisted port configuration.
 *
 * @param - module_mgmt - context
 * 
 * @return - fbe_status_t
 * 
 * @author
 *  12-Oct-2011:  Created. bphilbin 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_remove_all_ports(fbe_module_mgmt_t *module_mgmt)
{

    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s ModMgmt: issuing request to SEP to invalidate config!!!\n",
                              __FUNCTION__);

    status = fbe_api_database_set_invalidate_config_flag();

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to invalidate the DB, status: 0x%X\n",
                               __FUNCTION__, status);
        return status;
    }

    fbe_event_log_write(ESP_WARN_IO_PORT_CONFIGURATION_DESTROYED,
                        NULL, 0, 
                        NULL);

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s ModMgmt: invalidate request successful, setting reboot both SP flag!!!\n",
                              __FUNCTION__);


    /*
     * Set the flag indicating this SP needs to be rebooted.
     */
    module_mgmt->reboot_sp = REBOOT_BOTH;

    // set the check registry function so the reboot can be processed appropriately
    status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_CHECK_REGISTRY);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to set condition to update the registry, status: 0x%X\n",
                               __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_module_mgmt_faultIoPortLed()
 ****************************************************************
 * @brief
 *  This function send a request to fault the I/O Port LED.
 *
 * @param - module_mgmt - context
 * @param - iom_num - I/O Module number
 * @param - port_num - I/O Port number
 * 
 * @return - fbe_status_t - successfully set and reboot or not
 * 
 * @author
 *  03-Mar-2011:  Created. Joe Perry 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_faultIoPortLed(fbe_module_mgmt_t *module_mgmt,
                                            fbe_u8_t iom_num, 
                                            fbe_u8_t port_num)
{
    fbe_status_t                        status;
    fbe_board_mgmt_set_iom_port_LED_t   setIomPortLed;
    IO_CONTROLLER_PROTOCOL              port_protocol;
    IO_CONTROLLER_PROTOCOL              controller_protocol;

    setIomPortLed.iom_LED.sp_id = module_mgmt->local_sp;
    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt,
            module_mgmt->local_sp,
            iom_num,
            &setIomPortLed.iom_LED.slot,
            &setIomPortLed.iom_LED.device_type);

    setIomPortLed.io_port = port_num;

    setIomPortLed.iom_LED.blink_rate = LED_BLINK_1HZ;

    port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
    controller_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt,module_mgmt->local_sp,iom_num,port_num);

    if(port_protocol == IO_CONTROLLER_PROTOCOL_ISCSI || port_protocol == IO_CONTROLLER_PROTOCOL_FCOE )
    {
        //For the CNA ports in iSCSI mode, fault LED blink in 1Hz green
        if(controller_protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC)
        {
            setIomPortLed.led_color = LED_COLOR_TYPE_GREEN;
        }
        else
        {
            setIomPortLed.led_color = LED_COLOR_TYPE_AMBER_GREEN_SYNC;
        }
    }
    else
    {
        setIomPortLed.led_color = LED_COLOR_TYPE_BLUE;
    }
    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, set IoPortLed, Sp %d, mod %d, port %d, color %d, blink %d\n",
                           __FUNCTION__,
                           module_mgmt->local_sp,
                           iom_num,
                           port_num,
                           setIomPortLed.led_color,
                           setIomPortLed.iom_LED.blink_rate);

    status = fbe_api_board_set_iom_port_LED(module_mgmt->board_object_id,
                                            &setIomPortLed);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, IoPortLed failed, Sp %d, mod %d, port %d, status 0x%x\n",
                               __FUNCTION__, 
                               module_mgmt->local_sp, 
                               iom_num,
                               port_num,
                               status);
    }

    if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt,module_mgmt->local_sp,iom_num,port_num))
    {
        // set the LED for the next port of the combined pair as well, everything but the port number is the same here
        setIomPortLed.io_port++;
        status = fbe_api_board_set_iom_port_LED(module_mgmt->board_object_id, &setIomPortLed);
    }

    return status;

}   // end of fbe_module_mgmt_faultIoPortLed


/*!**************************************************************
 * fbe_module_mgmt_faultIoModuleLed()
 ****************************************************************
 * @brief
 *  This function send a request to fault the I/O Module LED.
 *
 * @param - module_mgmt - context
 * @param - iom_num - I/O Module number
 * 
 * @return - fbe_status_t - successfully set and reboot or not
 * 
 * @author
 *  03-Mar-2011:  Created. Joe Perry 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_faultIoModuleLed(fbe_module_mgmt_t *module_mgmt,
                                              fbe_u8_t iom_num,
                                              fbe_bool_t faulted)
{
    fbe_status_t            status;
    LED_BLINK_RATE          blinkRate = LED_BLINK_OFF;
    fbe_u64_t device_type;
    fbe_u32_t slot;

    if (faulted)
    {
        blinkRate = LED_BLINK_ON;
    }

    if(module_mgmt->io_module_info[iom_num].physical_info[module_mgmt->local_sp].type == 
       FBE_DEVICE_TYPE_MEZZANINE)
    {
        // jap - need Mezzanine support
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, no Mezzanine support yet\n",
                               __FUNCTION__);
        return FBE_STATUS_OK;
    }
    else
    {
        fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt,
                module_mgmt->local_sp,
                iom_num,
                &slot,
                &device_type);
        status = fbe_api_board_set_IOModuleFaultLED(module_mgmt->board_object_id,
                                                    module_mgmt->local_sp, 
                                                    slot,
                                                    device_type,
                                                    blinkRate);
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, Sp %d, mod %d, status 0x%x\n",
                               __FUNCTION__,
                               module_mgmt->local_sp,
                               iom_num,
                               status);
    }
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, IoModuleLed failed, Sp %d, mod %d, status 0x%x\n",
                               __FUNCTION__, 
                               module_mgmt->local_sp, 
                               iom_num,
                               status);
    }

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, IoModuleFaultLed blinkRate %d, Sp %d, mod %d\n",
                           __FUNCTION__, 
                           blinkRate,
                           module_mgmt->local_sp, 
                           iom_num);

    return status;

}   // end of fbe_module_mgmt_faultIoModuleLed

/*!**************************************************************
 * fbe_module_mgmt_could_set_io_port_led()
 ****************************************************************
 * @brief
 *  This function returns if we could set IO Port LED.
 *
 * @param - module_mgmt - context
 * @param - iom_num     - I/O Module number
 * @param - sp_id       - SPA or SPB
 * 
 * @return - fbe_bool_t - Could set IO Port LED or not
 * 
 * @author
 *  23-Jul-2014:  Created. Tommy Miao
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_could_set_io_port_led(fbe_module_mgmt_t *module_mgmt,
                                                 fbe_u8_t           iom_num,
                                                 SP_ID              sp_id)
{
    return (module_mgmt->io_module_info[iom_num].physical_info[sp_id].type == FBE_DEVICE_TYPE_IOMODULE
         || module_mgmt->io_module_info[iom_num].physical_info[sp_id].type == FBE_DEVICE_TYPE_MEZZANINE
         || module_mgmt->io_module_info[iom_num].physical_info[sp_id].type == FBE_DEVICE_TYPE_BACK_END_MODULE);
}

/*!**************************************************************
 * fbe_module_mgmt_setIoPortLedBasedOnLink()
 ****************************************************************
 * @brief
 *  This function send a request to set I/O Port LED based on link
 *   state.
 *
 * @param - module_mgmt - context
 * @param - iom_num - I/O Module number
 * @param - port_num - I/O Port number
 * 
 * @return - fbe_status_t - successfully set and reboot or not
 * 
 * @author
 *  03-Mar-2011:  Created. Joe Perry 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_setIoPortLedBasedOnLink(fbe_module_mgmt_t *module_mgmt,
                                                     fbe_u8_t iom_num, 
                                                     fbe_u8_t port_num)
{
    fbe_status_t                        status;
    fbe_board_mgmt_set_iom_port_LED_t   setIomPortLed;
    fbe_bool_t                          portLinkOptimal = TRUE;
    fbe_port_link_information_t         port_link_info;
    IO_CONTROLLER_PROTOCOL              port_protocol;
    IO_CONTROLLER_PROTOCOL              controller_protocol;

    setIomPortLed.iom_LED.sp_id = module_mgmt->local_sp;
    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt,
            module_mgmt->local_sp,
            iom_num,
            &setIomPortLed.iom_LED.slot,
            &setIomPortLed.iom_LED.device_type);

    setIomPortLed.io_port = port_num;

    port_link_info = fbe_module_mgmt_get_port_link_info(module_mgmt, module_mgmt->local_sp, iom_num, port_num);

    portLinkOptimal = fbe_module_mgmt_is_port_link_optimal(module_mgmt, iom_num, port_num);


    port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
    controller_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt, module_mgmt->local_sp, iom_num, port_num);

    if( (port_link_info.link_state == FBE_PORT_LINK_STATE_DOWN) ||
        (port_link_info.link_state == FBE_PORT_LINK_STATE_INVALID) )
    {
        setIomPortLed.iom_LED.blink_rate = LED_BLINK_OFF;
    }
    else
    {
        // for all other link states the color is blue for any lane count at any speed
        setIomPortLed.iom_LED.blink_rate = LED_BLINK_ON;

        // For ethernet ports, if it is a CNA port, set port LED to green solid on, otherwise do not touch the LED
        if(port_protocol == IO_CONTROLLER_PROTOCOL_ISCSI || port_protocol == IO_CONTROLLER_PROTOCOL_FCOE )
        {
            if(controller_protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC)
            {
                setIomPortLed.led_color = LED_COLOR_TYPE_GREEN;
            }
            else
            {
                return FBE_STATUS_OK;
            }
        }
        else
        {
            setIomPortLed.led_color = LED_COLOR_TYPE_BLUE;
        }
    }

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, set IoPortLed, Sp %d, mod %d, port %d, color %d, blink %d\n",
                           __FUNCTION__,
                           module_mgmt->local_sp,
                           iom_num,
                           port_num,
                           setIomPortLed.led_color,
                           setIomPortLed.iom_LED.blink_rate);

    status = fbe_api_board_set_iom_port_LED(module_mgmt->board_object_id,
                                            &setIomPortLed);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, IoPortLed failed, Sp %d, mod %d, port %d, status 0x%x\n",
                               __FUNCTION__, 
                               module_mgmt->local_sp, 
                               iom_num,
                               port_num,
                               status);
    }

    if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt,module_mgmt->local_sp,iom_num,port_num))
    {
        // set the LED for the next port of the combined pair as well, everything but the port number is the same here
        setIomPortLed.io_port++;
        status = fbe_api_board_set_iom_port_LED(module_mgmt->board_object_id, &setIomPortLed);

    }

    return status;

}   // end of fbe_module_mgmt_setIoPortLedBasedOnLink


/*!**************************************************************
 * fbe_module_mgmt_is_port_link_optimal()
 ****************************************************************
 * @brief
 *  This function checks if the specified port is operating at
 *  its top capable speed.
 *
 * @param - module_mgmt - context
 * @param - iom_num - I/O Module number
 * @param - port_num - I/O Port number
 * 
 * @return - fbe_bool_t - port link is at it's optimal speed
 * 
 * @author
 *  20-Oct-2012:  Created. bphilbin 
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_port_link_optimal(fbe_module_mgmt_t *module_mgmt, fbe_u32_t iom_num, fbe_u32_t port_num)
{
    fbe_u8_t port_index;
    fbe_u32_t supported_speeds;
    fbe_u32_t current_speed;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    supported_speeds = module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].supportedSpeeds;
    current_speed = module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_speed;

    supported_speeds &= ~(SPEED_AUTO_SELECT | SPEED_UNCHANGED);

    // compare with the Supported Speeds
    if (current_speed & supported_speeds)
    {
        // shift out lower supported speed bits
        while (current_speed > 0)
        {
            current_speed = current_speed >> 1;
            supported_speeds = supported_speeds >> 1;
        }
        // check if any higher speeds left
        if (supported_speeds == 0)
        {
            return FBE_TRUE;
        }
        else
        {
            return FBE_FALSE;
        }
    }
    else
    {
        return FBE_FALSE;
    }
}
/*!**************************************************************
 * fbe_module_mgmt_get_port_affinity_processor_core()
 ****************************************************************
 * @brief
 *  This function returns the processor core of configured port
 *  from port affinity interrupt settings.
 *
 * @param - module_mgmt - context
 * @param - pciData - port address
 *
 * @author
 *  9-May-2012:  Created. Lin Lou
 *
 ****************************************************************/
fbe_u32_t fbe_module_mgmt_get_port_affinity_processor_core(fbe_module_mgmt_t *module_mgmt,
                                                           SPECL_PCI_DATA  pciData)
{
    fbe_u32_t       index;

    for(index=0; index<FBE_ESP_IO_PORT_MAX_COUNT; index++)
    {
        if((module_mgmt->configured_interrupt_data[index].pci_data.bus == pciData.bus) &&
           (module_mgmt->configured_interrupt_data[index].pci_data.device == pciData.device) &&
           (module_mgmt->configured_interrupt_data[index].pci_data.function == pciData.function))
        {
            return module_mgmt->configured_interrupt_data[index].processor_core;
        }
    }
    
    //If we get here, then we did not find affinity info from affinity setting.
    //So try to get affinity info from reg info
    for(index=0; index<MAX_PORT_REG_PARAMS; index++)
    {
        if((module_mgmt->iom_port_reg_info[index].pci_data.bus == pciData.bus) &&
           (module_mgmt->iom_port_reg_info[index].pci_data.device == pciData.device) &&
           (module_mgmt->iom_port_reg_info[index].pci_data.function == pciData.function))
        {
            return module_mgmt->iom_port_reg_info[index].core_num;
        }
    }
    
    return INVALID_CORE_NUMBER;
}  // end of fbe_module_mgmt_get_port_affinity_processor_core

/*!**************************************************************
 * fbe_module_mgmt_assign_port_subrole()
 ****************************************************************
 * @brief
 *  This function loops through all ports present for this SP and
 *  assigns the special subrole.
 *
 * @param - module_mgmt - context
 * sp_id - SPA or SPB

 * @author
 *  18-Jun-2012:  Created. dongz
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_assign_port_subrole(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id)
{
    fbe_u32_t iom_num, port_num;
    fbe_u32_t num_ports_on_iom;
    fbe_bool_t changes_made = FALSE;
    IO_CONTROLLER_PROTOCOL port_protocol;
    fbe_bool_t iScsi_fe_subrole_special_assigned;
    fbe_bool_t fiber_fe_subrole_special_assigned;

    //init the value check
    iScsi_fe_subrole_special_assigned =
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_B) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_E) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_G) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_I) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_J) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_L) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_O) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_P) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_Q) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_R) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_S);

    fiber_fe_subrole_special_assigned =
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_A) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_D) ||
            fbe_module_mgmt_is_special_port_assigned(module_mgmt, sp_id, FBE_IOM_GROUP_M);


    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        if((fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num)) &&
           fbe_module_mgmt_is_iom_power_good(module_mgmt, sp_id, iom_num))
        {
            num_ports_on_iom = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt, sp_id, iom_num);
            for(port_num = 0; port_num < num_ports_on_iom; port_num++)
            {
                if((!fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, port_num)) &&
                    (fbe_module_mgmt_get_port_state(module_mgmt, sp_id, iom_num, port_num) != FBE_PORT_STATE_FAULTED))

                {
                    port_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt, sp_id, iom_num, port_num);
                    if(port_protocol == IO_CONTROLLER_PROTOCOL_ISCSI && !iScsi_fe_subrole_special_assigned)
                    {
                        fbe_module_mgmt_set_port_subrole(module_mgmt, sp_id, iom_num, port_num, FBE_PORT_SUBROLE_SPECIAL);
                        iScsi_fe_subrole_special_assigned = TRUE;
                        changes_made = TRUE;
                    }
                    else if(port_protocol == IO_CONTROLLER_PROTOCOL_FIBRE && !fiber_fe_subrole_special_assigned)
                    {
                        fbe_module_mgmt_set_port_subrole(module_mgmt, sp_id, iom_num, port_num, FBE_PORT_SUBROLE_SPECIAL);
                        fiber_fe_subrole_special_assigned = TRUE;
                        changes_made = TRUE;
                    }
                    else if(port_protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC)
                    {

                        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, checking IOM:%d CNA Port %d to see if special port assignment is needed..\n",
                                              __FUNCTION__, iom_num, port_num);
                        /*
                         * CNA ports can't rely on the current operating port protocol, derive it from the IO module group that
                         * we got from the SFP type inserted into the port.
                         */
                        if( (fbe_module_mgmt_get_iom_group(module_mgmt,sp_id,iom_num,port_num) == FBE_IOM_GROUP_P) &&
                            !iScsi_fe_subrole_special_assigned)
                        {
                            fbe_module_mgmt_set_port_subrole(module_mgmt, sp_id, iom_num, port_num, FBE_PORT_SUBROLE_SPECIAL);

                            iScsi_fe_subrole_special_assigned = TRUE;
                            changes_made = TRUE;
                            fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                                  FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s, IOM:%d CNA Port %d set to special port subrole\n",
                                                  __FUNCTION__, iom_num, port_num);
                        }
                        if( (fbe_module_mgmt_get_iom_group(module_mgmt,sp_id,iom_num,port_num) == FBE_IOM_GROUP_M) &&
                            !fiber_fe_subrole_special_assigned)
                        {
                            fbe_module_mgmt_set_port_subrole(module_mgmt, sp_id, iom_num, port_num, FBE_PORT_SUBROLE_SPECIAL);

                            fiber_fe_subrole_special_assigned = TRUE;
                            changes_made = TRUE;
                            fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                                  FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s, IOM:%d CNA Port %d set to special port subrole\n",
                                                  __FUNCTION__, iom_num, port_num);
                        }
                    }
                }
            }
        }
    }
    return changes_made;
}

/**************************************************************************
 *          cm_flexports_is_special_port_assigned
 * ************************************************************************
 *  DESCRIPTION:
 *  This function returns TRUE if a special port subrole is already
 *  assigned for some FE port
 *
 *
 *  PARAMETERS:
 *   module_mgmt - context
 *   sp_id - SPA or SPB
 *   group - port's iom group (FC, ISCSI, etc.)
 *
 *  RETURN VALUES:
 *   T/F - special port subrole assigned
 *
 *  History:
 *  18-Jun-12: dongz - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_is_special_port_assigned(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_iom_group_t group)
{
    fbe_u32_t iom_num, port_num;
    fbe_ioport_role_t port_role;
    fbe_port_subrole_t port_sub_role;
    fbe_iom_group_t iom_group;

    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        if((fbe_module_mgmt_is_iom_inserted(module_mgmt, sp_id, iom_num)) &&
           fbe_module_mgmt_is_iom_power_good(module_mgmt, sp_id, iom_num))
        {
            for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
            {
                //we will skip those uninitialized ports.
                if(!fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, port_num))
                {
                    continue;
                }
                port_role = fbe_module_mgmt_get_port_role(module_mgmt, sp_id, iom_num, port_num);
                port_sub_role = fbe_module_mgmt_get_port_subrole(module_mgmt, sp_id, iom_num, port_num);
                iom_group = fbe_module_mgmt_get_iom_group(module_mgmt, sp_id, iom_num, port_num);
                if(port_role == IOPORT_PORT_ROLE_FE &&
                   port_sub_role == FBE_PORT_SUBROLE_SPECIAL &&
                   iom_group == group)
                {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

void fbe_module_mgmt_generate_slic_list_string(fbe_module_slic_type_t slic_type, fbe_char_t *types_string)
{
    fbe_u32_t mask;
    fbe_u32_t i;
    const fbe_char_t *slic_type_string;
    fbe_module_slic_type_t temp_slic_type;
    fbe_u32_t string_count = 0;

    fbe_set_memory(types_string, 0, FBE_MODULE_MGMT_SLIC_TYPE_STRING_LENGTH * sizeof(fbe_char_t));

    for(i=0;i<32;i++)
    {
        mask = 1 << i;
        if(slic_type & mask)
        {
            temp_slic_type = slic_type & mask;
            slic_type_string = fbe_module_mgmt_slic_type_to_string(temp_slic_type);
            if(string_count > 0)
            {
                csx_p_strncat(types_string, ", ", 2);
            }
            csx_p_strncat(types_string,slic_type_string, strlen(slic_type_string));
            string_count++;
        }
    }
    return;
}

/*!***************************************************************
 * @fn fbe_module_mgmt_log_bem_lcc_event()
 ****************************************************************
 * @brief
 *  This is function log the event for BEM lcc realted data change.
 *
 * @param mgmt_module - Ptr to module_mgmt
 * @param new_bem_info - Ptr to the new data  
 * @param old_bem_info - Ptr to the old data
 *
 * @return fbe_status_t 
 *
 * @author 
 *  09-Jan-2013: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_log_bm_lcc_event(fbe_module_mgmt_t * module_mgmt,
                                       fbe_board_mgmt_io_comp_info_t * pNewBmInfo,
                                       fbe_board_mgmt_io_comp_info_t * pOldBmInfo)
{
    fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_status_t    status;
    fbe_device_physical_location_t location = {0};
    fbe_base_env_resume_prom_info_t * pResumePromInfo = NULL;
    fbe_u8_t bem_sn[RESUME_PROM_PRODUCT_SN_SIZE + 1] = {'\0'};
    fbe_u8_t bem_pn[RESUME_PROM_PRODUCT_PN_SIZE + 1] = {'\0'};

    fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
    location.sp = pNewBmInfo->associatedSp;
    location.slot = pNewBmInfo->slotNumOnBlade;

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, new ecbFault %d, old ecbFault %d.\n",
                              __FUNCTION__, pNewBmInfo->lccInfo.ecbFault, pOldBmInfo->lccInfo.ecbFault);

    /* Get the device string */
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BACK_END_MODULE, 
                                               &location, 
                                               &deviceStr[0], 
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to device string for BEM, status: 0x%X.\n",
                              __FUNCTION__, status);
    }

    status = fbe_module_mgmt_get_resume_prom_info_ptr(module_mgmt,
    									FBE_DEVICE_TYPE_BACK_END_MODULE,
    									&location,
    									&pResumePromInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to get Resume Prom info for BEM, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    if (pNewBmInfo->lccInfo.inserted)
    {
        // log ecbFault event
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, new ecbFault %d, old ecbFault %d.\n",
                              __FUNCTION__, pNewBmInfo->lccInfo.ecbFault, pOldBmInfo->lccInfo.ecbFault);
        
        if(pNewBmInfo->lccInfo.ecbFault && (!pOldBmInfo->lccInfo.ecbFault || !pOldBmInfo->lccInfo.inserted))
        {
            fbe_event_log_write(ESP_ERROR_BM_ECB_FAULT_DETECTED, 
                                NULL, 
                                0, 
                                "%s",
                                &deviceStr[0]);
                                
        }
        else if(!pNewBmInfo->lccInfo.ecbFault && pOldBmInfo->lccInfo.ecbFault) 
        {
            fbe_event_log_write(ESP_INFO_BM_ECB_FAULT_CLEARED, 
                                NULL, 
                                0, 
                                "%s",
                                &deviceStr[0]);
        }

        // log fault event
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_LOW,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s, new faulted %d, additionalStatus %d, old faulted %d, additionalStatus %d.\n",
                      __FUNCTION__, 
                      pNewBmInfo->lccInfo.faulted, 
                      pNewBmInfo->lccInfo.additionalStatus,
                      pOldBmInfo->lccInfo.faulted,
                      pOldBmInfo->lccInfo.additionalStatus);
        
        if(pNewBmInfo->lccInfo.faulted && (!pOldBmInfo->lccInfo.faulted || !pOldBmInfo->lccInfo.inserted))
        {
            // there two types of LCC faults, so look at additional Status to determine type
            if (pNewBmInfo->lccInfo.additionalStatus == SES_STAT_CODE_CRITICAL)
            {
            	if(pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS)
            	{
            		fbe_copy_memory(bem_sn, pResumePromInfo->resume_prom_data.product_serial_number, RESUME_PROM_PRODUCT_SN_SIZE);
            		fbe_copy_memory(bem_pn, pResumePromInfo->resume_prom_data.product_part_number, RESUME_PROM_PRODUCT_PN_SIZE);
            	}
            	else
            	{
            		fbe_copy_memory(bem_sn, "Not Available", (fbe_u32_t)(FBE_MIN(strlen("Not Available"), RESUME_PROM_PRODUCT_SN_SIZE)));
            		fbe_copy_memory(bem_pn, "Not Available", (fbe_u32_t)(FBE_MIN(strlen("Not Available"), RESUME_PROM_PRODUCT_PN_SIZE)));
            	}

                // Critical, lcc faulted
                status = fbe_event_log_write(ESP_ERROR_LCC_FAULTED,
                                             NULL, 
                                             0,
                                             "%s %s %s",
                                             &deviceStr[0],
                                             bem_sn,
                                             bem_pn);
            }
            else
            {
                // Else, lcc fault may be caused by other components
                status = fbe_event_log_write(ESP_ERROR_LCC_COMPONENT_FAULTED,
                                             NULL, 
                                             0,
                                             "%s", 
                                             &deviceStr[0]);
            }
        }
        else if(!pNewBmInfo->lccInfo.faulted && pOldBmInfo->lccInfo.faulted) 
        {
            fbe_event_log_write(ESP_INFO_LCC_FAULT_CLEARED, 
                                NULL, 
                                0, 
                                "%s",
                                &deviceStr[0]);
        }
    }
     
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_module_mgmt_set_selected_pci_function()
 ****************************************************************
 * @brief
 *  This is function determines which set of pci data provided is
 *  the current active PCI function for this port.
 *
 * @param mgmt_module - Ptr to module_mgmt object data
 * @param port_index - Offset index to the current port information
 * @param sp_id - SP identifier
 *
 * @return fbe_status_t 
 *
 * @author 
 *  29-Dec-2014: bphilbin - Created.
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_set_selected_pci_function(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port_index, SP_ID sp_id)
{

    fbe_u32_t iom_num = 0, port_num = 0;
    fbe_board_mgmt_io_port_info_t *port_info = &module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id];
    IO_CONTROLLER_PROTOCOL protocol = IO_CONTROLLER_PROTOCOL_UNKNOWN;
    fbe_u32_t function = 0;

    iom_num = fbe_module_mgmt_get_io_port_module_number(module_mgmt, sp_id, port_index);
    port_num = fbe_module_mgmt_get_io_port_number(module_mgmt, sp_id, port_index);

    if( (port_info->pciDataCount == 1) || (!fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, iom_num, port_num)))
    {
        /*
         * There is only one pci function for this port, this is the active function
         */
        fbe_copy_memory (&module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function,
                         &port_info->pciData[0], sizeof(SPECL_PCI_DATA));
    }
    else
    {
        /*
         * This is a multi-function device with configured ports, set the current function to match the configured protocol 
         * for this port. 
         */
        protocol = fbe_module_mgmt_derive_protocol_from_iom_group(module_mgmt, fbe_module_mgmt_get_iom_group(module_mgmt, sp_id, iom_num, port_num));

        /*
         * Loop through the pci functions and find the one with the matching protocol, set this to the current function
         */
        for(function=0; function < port_info->pciDataCount; function++)
        {
            if(protocol == port_info->portPciData[function].protocol)
            {
                fbe_copy_memory (&module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function,
                         &port_info->portPciData[function].pciData, sizeof(SPECL_PCI_DATA));
                break;
            }

        }
        
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_module_mgmt_derive_protocol_from_iom_group()
 ****************************************************************
 * @brief
 *  This is function determines an expected protocol for a port
 *  based on the iom group that the port has been configured for.
 *
 * @param mgmt_module - Ptr to module_mgmt object data
 * @param iom_group - Configured IO module group
 * 
 * @return IO_CONTROLLER_PROTOCOL 
 *
 * @author 
 *  29-Dec-2014: bphilbin - Created.
 *
 ****************************************************************/
IO_CONTROLLER_PROTOCOL fbe_module_mgmt_derive_protocol_from_iom_group(fbe_module_mgmt_t *module_mgmt, fbe_iom_group_t iom_group)
{
    IO_CONTROLLER_PROTOCOL protocol = IO_CONTROLLER_PROTOCOL_UNKNOWN;
    switch(iom_group)
    {
    case FBE_IOM_GROUP_A:
    case FBE_IOM_GROUP_D:
    case FBE_IOM_GROUP_M:
        protocol = IO_CONTROLLER_PROTOCOL_FIBRE;
        break;
    case FBE_IOM_GROUP_B:
    case FBE_IOM_GROUP_E:
    case FBE_IOM_GROUP_G:
    case FBE_IOM_GROUP_I:
    case FBE_IOM_GROUP_J:
    case FBE_IOM_GROUP_L:
    case FBE_IOM_GROUP_O:
    case FBE_IOM_GROUP_P:
    case FBE_IOM_GROUP_Q:
    case FBE_IOM_GROUP_R:
    case FBE_IOM_GROUP_S:
        protocol = IO_CONTROLLER_PROTOCOL_ISCSI;
        break;
    case FBE_IOM_GROUP_C:
    case FBE_IOM_GROUP_K:
    case FBE_IOM_GROUP_N:
        protocol = IO_CONTROLLER_PROTOCOL_SAS;
        break;
    case FBE_IOM_GROUP_F:
        protocol = IO_CONTROLLER_PROTOCOL_FCOE;
        break;
    }
    return protocol;
}

/*!***************************************************************
 * @fn fbe_module_mgmt_check_cna_ports_match()
 ****************************************************************
 * @brief
 *  This is function checks that this device supports cna ports and
 *  that all those ports match.  
 *
 * @param mgmt_module - Ptr to module_mgmt object data
 * @param sp - sp id
 * @param iom_num - Configured IO module group
 * 
 * @return IO_CONTROLLER_PROTOCOL 
 *
 * @author 
 *  29-Dec-2014: bphilbin - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_check_cna_ports_match(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u32_t iom_num, fbe_u32_t port_num)
{
    IO_CONTROLLER_PROTOCOL controller_protocol = IO_CONTROLLER_PROTOCOL_UNKNOWN;
    IO_CONTROLLER_PROTOCOL first_port_protocol = IO_CONTROLLER_PROTOCOL_UNKNOWN;
    fbe_u32_t io_port;

    /* 
     * Check that all CNA ports on this io module have the same protocol
     */
    for(io_port = 0; io_port < module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.ioPortCount; io_port++)
    {
        controller_protocol = fbe_module_mgmt_get_port_controller_protocol(module_mgmt, sp, iom_num, io_port);
        if(controller_protocol == IO_CONTROLLER_PROTOCOL_AGNOSTIC)
        {
            if(first_port_protocol == IO_CONTROLLER_PROTOCOL_UNKNOWN)
            {
                first_port_protocol = fbe_module_mgmt_get_port_protocol(module_mgmt, sp, iom_num, io_port);
                if(io_port == port_num)
                {
                    // this is the first port other ports will need to match this port
                    return FBE_TRUE;
                }
            }
            else if( (first_port_protocol != fbe_module_mgmt_get_port_protocol(module_mgmt, sp, iom_num, io_port)) &&
                     (io_port == port_num))
            {
                // we do not match the first CNA port on the IO module
                return FBE_FALSE;
            }
        }
    }
    return FBE_TRUE;
}

/*!***************************************************************
 * @fn fbe_module_mgmt_check_cna_port_protocol()
 ****************************************************************
 * @brief
 *  This is function checks that the current protocol for the CNA ports
 *  match the configured protocol.  If the protocol in NVRam is changed
 *  an SP reboot is required.
 *
 * @param mgmt_module - Ptr to module_mgmt object data
 * 
 * @return IO_CONTROLLER_PROTOCOL 
 *
 * @author 
 *  29-Dec-2014: bphilbin - Created.
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_check_cna_port_protocol(fbe_module_mgmt_t *module_mgmt)
{
    fbe_iom_group_t iom_group;
    IO_CONTROLLER_PROTOCOL expected_protocol;
    fbe_board_mgmt_misc_info_t misc_info = {0};
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t iom_num = 0, port_num = 0;

    if(fbe_module_mgmt_get_first_cna_port(module_mgmt, &iom_num, &port_num) != FBE_STATUS_OK)
    {
        // no cna ports found
        return FBE_STATUS_OK;
    }

    iom_group =  fbe_module_mgmt_get_iom_group(module_mgmt, module_mgmt->local_sp, iom_num, port_num);

    fbe_api_board_get_misc_info(module_mgmt->board_object_id, &misc_info);

    switch(iom_group)
    {
    case FBE_IOM_GROUP_P:
        expected_protocol = IO_CONTROLLER_PROTOCOL_ISCSI;
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, expected protocol set to ISCSI\n",
                      __FUNCTION__);
        break;
    case FBE_IOM_GROUP_M:
        expected_protocol = IO_CONTROLLER_PROTOCOL_FIBRE;
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, expected protocol set to FIBRE\n",
                      __FUNCTION__);
        
        break;
    default:
        // not a cna port
        return FBE_STATUS_OK;
        break;
    }

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, CNA mode %d\n",
                      __FUNCTION__, misc_info.cnaMode);

    // check the protocol value in NVRam to see if we need to change the setting
    if((misc_info.cnaMode != CNA_MODE_ETHERNET) && (expected_protocol == IO_CONTROLLER_PROTOCOL_ISCSI))
    {
        // need to change CNA mode to iscsi
        fbe_api_board_set_cna_mode(module_mgmt->board_object_id, CNA_MODE_ETHERNET);
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, CNA mode mismatch, setting ETHERNET reboot required\n",
                      __FUNCTION__);
        module_mgmt->reboot_sp = REBOOT_LOCAL;
    }
    else if ((misc_info.cnaMode != CNA_MODE_FIBRE) && (expected_protocol == IO_CONTROLLER_PROTOCOL_FIBRE))
    {
        // need to change CNA mode to fibre channel
        status = fbe_api_board_set_cna_mode(module_mgmt->board_object_id, CNA_MODE_FIBRE);
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, CNA mode mismatch, setting FIBRE reboot required\n",
                      __FUNCTION__);
        module_mgmt->reboot_sp = REBOOT_LOCAL;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, CNA mode matches, nothing to do\n",
                      __FUNCTION__);
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_module_mgmt_get_first_cna_port()
 ****************************************************************
 * @brief
 *  This function finds the first CNA port for the specified io module.  
 *
 * @param mgmt_module - Ptr to module_mgmt object data
 * @param sp - sp id
 * @param iom_num - Configured IO module group
 * 
 * @return IO_CONTROLLER_PROTOCOL 
 *
 * @author 
 *  29-Dec-2014: bphilbin - Created.
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_first_cna_port(fbe_module_mgmt_t *module_mgmt, fbe_u32_t *iom_num, fbe_u32_t *port_num)
{
    fbe_u32_t temp_iom_num = 0, temp_port_num = 0;

    for(temp_iom_num = 0; temp_iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; temp_iom_num++)
    {
        if(module_mgmt->io_module_info[temp_iom_num].physical_info[module_mgmt->local_sp].module_comp_info.inserted ==
            FBE_MGMT_STATUS_TRUE)
        {
            for(temp_port_num = 0; 
                 temp_port_num < module_mgmt->io_module_info[temp_iom_num].physical_info[module_mgmt->local_sp].module_comp_info.ioPortCount; 
                 temp_port_num++)
            {
                if(fbe_module_mgmt_get_port_controller_protocol(module_mgmt,module_mgmt->local_sp,temp_iom_num,temp_port_num) ==
                   IO_CONTROLLER_PROTOCOL_AGNOSTIC)
                {
                    // this is a CNA port
                    *iom_num = temp_iom_num;
                    *port_num = temp_port_num;
                    return FBE_STATUS_OK;
                }
            }
        }
    }
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!***************************************************************
 * @fn fbe_module_mgmt_is_cna_port()
 ****************************************************************
 * @brief
 *  This function returns true if the specified port is a CNA port.  
 *
 * @param mgmt_module - Ptr to module_mgmt object data
 * @param iom_num - IO module number
 * @param port_num - Port number
 * 
 * @return T/F - specified port is a CNA port 
 *
 * @author 
 *  17-Aug-2015: bphilbin - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_cna_port(fbe_module_mgmt_t *module_mgmt, fbe_u32_t iom_num, fbe_u32_t port_num)
{
    if(fbe_module_mgmt_get_port_controller_protocol(module_mgmt,module_mgmt->local_sp,iom_num,port_num) ==
                   IO_CONTROLLER_PROTOCOL_AGNOSTIC)
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}

/*!***************************************************************
 * @fn fbe_module_mgmt_is_sfp_cna_capable()
 ****************************************************************
 * @brief
 *  This function checks to see if we can determine the CNA protocol
 *  based on the SFP.
 *
 * @param mgmt_module - Ptr to module_mgmt object data
 * @param iom_num - IO module number
 * @param port_num - Port number
 * 
 * @return T/F - specified port is a CNA port 
 *
 * @author 
 *  17-Aug-2015: bphilbin - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_module_mgmt_is_sfp_cna_capable(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID sp_id, fbe_u8_t iom_index, 
                                                 fbe_u8_t port_num)
{
    fbe_u32_t port_index = 0;
    fbe_u32_t supported_speeds = 0;
    fbe_u32_t compliance_code = 0;
    fbe_u32_t max_bit_rate = 0;
    fbe_u32_t fc_support_speeds = 0;


    if(module_mgmt->local_sp != sp_id)
    {
        // this is only valid for local ports
        return FBE_IOM_GROUP_UNKNOWN;
    
    }
    port_index = fbe_module_mgmt_get_io_port_index(iom_index, port_num);
    
    if(module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.inserted == FBE_FALSE)
    {
        // no sfp inserted for this port, cannot determine the iom_group
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s SP:%d IOM:%d Port:%d SFP not inserted\n",
                                  __FUNCTION__, sp_id, iom_index, port_num);
        return FBE_IOM_GROUP_UNKNOWN;
    }

    compliance_code = decodeComplianceCode(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);
    max_bit_rate = decodeMaxBitRate(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);
    fc_support_speeds = getFibreChannelSpeeds(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s SP:%d IOM:%d Port:%d comp_code:%d max_br:%d supp_spd:%d type:%d\n",
                          __FUNCTION__, sp_id, iom_index, port_num, compliance_code, max_bit_rate, fc_support_speeds, 
                          module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    supported_speeds = getSupportedSpeeds(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                          module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type);

    /*
     * We determine the sfp protocol based on the capable speeds.  Basically if it supports 10Gbps then it is iSCSI, 16, 8 or 4 is FC
     */
    if((supported_speeds == SPEED_TEN_GIGABIT) ||
       (supported_speeds & SPEED_SIXTEEN_GIGABIT) ||
       (supported_speeds & SPEED_EIGHT_GIGABIT) ||
       (supported_speeds & SPEED_FOUR_GIGABIT))
    {
        return FBE_TRUE;
    }
    
    return FBE_FALSE;
}
