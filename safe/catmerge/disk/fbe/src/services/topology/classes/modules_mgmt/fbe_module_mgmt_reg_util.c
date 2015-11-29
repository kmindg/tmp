/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_reg_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management Registry related utility
 *  functionalities.
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   02-Sep-2010 - Created. bphilbin
 *
 ***************************************************************************/

#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_types.h"
#include "EmcPAL.h"

#include "fbe/fbe_module_mgmt_reg_util.h"
#include "fbe/fbe_module_mgmt_mapping.h"

static void fbe_module_mgmt_reg_get_all_cpd_params(fbe_module_mgmt_t *module_mgmt);
static fbe_bool_t fbe_module_mgmt_reg_check_iom_groups_differ(fbe_module_mgmt_t *module_mgmt, 
                                                       SP_ID sp_id, fbe_u32_t iom_num, 
                                                       fbe_u32_t port_num);
static fbe_bool_t fbe_module_mgmt_reg_set_port_params(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port);
static fbe_bool_t fbe_module_mgmt_reg_set_default_param_entry(fbe_u32_t PortParam);
static void fbe_module_mgmt_reg_convertCPDPortRoleSubRole(fbe_u32_t logical_number,
                                                fbe_char_t *port_role_subrole_string,  
                                                fbe_u32_t buffer_size, 
                                                fbe_ioport_role_t *port_role, 
                                                fbe_port_subrole_t *port_subrole, 
                                                fbe_bool_t ascii_to_enum);
static void fbe_module_mgmt_reg_convertCPDIOMGroup(fbe_char_t *iom_group_string, 
                                             fbe_u32_t buffer_size, 
                                             fbe_iom_group_t *iom_group, 
                                             fbe_bool_t ascii_to_enum);
static void fbe_module_mgmt_reg_parse_string(fbe_module_mgmt_t *module_mgmt,
                                             fbe_u32_t port_param,
                                             fbe_char_t *buffer, 
                                             fbe_u32_t buffer_size,
                                             fbe_char_t *iom_group_string,
                                             fbe_char_t *port_role_subrole_string,
                                             fbe_bool_t ascii_to_enum,
                                             fbe_u32_t core_num,
                                             fbe_u32_t phy_map,
                                             fbe_u8_t port_count);
static void fbe_module_mgmt_reg_parse_sas_string(fbe_module_mgmt_t *module_mgmt,
                                      fbe_u32_t port_param,
                                      fbe_char_t *buffer, 
                                      fbe_u32_t buffer_size,
                                      fbe_char_t *iom_group_string,
                                      fbe_char_t *port_role_subrole_string);

static fbe_bool_t fbe_module_mgmt_reg_scan_sas_string(char *buffer, 
                             fbe_u32_t buffer_size, 
                             fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info, 
                             fbe_char_t *iom_group_string,
                             fbe_char_t *port_role_subrole_string);

static fbe_bool_t fbe_module_mgmt_reg_scan_string(char *buffer, 
                             fbe_u32_t buffer_size, 
                             fbe_u32_t *bus, 
                             fbe_u32_t *func, 
                             fbe_u32_t *slot, 
                             fbe_char_t *iom_group_string,
                             fbe_char_t *port_role_subrole_string, 
                             fbe_u32_t *logical_port_number);
fbe_bool_t fbe_module_mgmt_reg_scan_string_with_core_num(char *buffer,
                             fbe_u32_t buffer_size,
                             fbe_u32_t *bus,
                             fbe_u32_t *func,
                             fbe_u32_t *slot,
                             fbe_char_t *iom_group_string,
                             fbe_char_t *port_role_subrole_string,
                             fbe_u32_t *logical_port_number,
                             fbe_u32_t *core_num);
static void fbe_module_mgmt_reg_set_port_reg_info(fbe_module_mgmt_t *module_mgmt, 
                                           fbe_u32_t iom_num, fbe_u32_t port_num);
static fbe_u32_t fbe_module_mgmt_reg_get_port_param_number(fbe_module_mgmt_t *module_mgmt, 
                                                    fbe_u32_t bus, fbe_u32_t func, 
                                                    fbe_u32_t slot);
static fbe_bool_t fbe_module_mgmt_reg_port_config_exists(fbe_module_mgmt_t *module_mgmt, 
                                                fbe_u32_t portparam);

static fbe_bool_t fbe_module_mgmt_reg_scan_string_for_hex(fbe_char_t stop, fbe_u32_t length, 
                                             fbe_char_t *buffer, fbe_u32_t *index, 
                                             fbe_u32_t *result);

static void fbe_module_mgmt_remove_port_param_entry(fbe_module_mgmt_t *module_mgmt, fbe_u32_t portparam);

/**************************************************************************
 *          fbe_module_mgmt_reg_get_all_cpd_params
 * ************************************************************************
 *  @brief
 *  This function is called during bootup to read all the CPD instance 
 *  parameters from the registry.
 *
 *
 * @param module_mgmt - object context
 *
 * @return none
 *
 *  History:
 *  08-Feb-07: Brion Philbin - Created
 *  28-Jan-08: Rajesh V - Modified as part of getting PCI data from SPECL
 *  17-Sep-10: Brion Philbin - Rewritten for fbe_module_mgmt
 * ***********************************************************************/
void fbe_module_mgmt_reg_get_all_cpd_params(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t                           portparam = 0;
    fbe_char_t                          port_param_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_module_mgmt_port_reg_info_t     *p_iom_port_reg_info;
    fbe_char_t                          iom_group_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t                          port_role_subrole_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_status_t                        status = FBE_STATUS_OK;



    for(portparam = 0; portparam < MAX_PORT_REG_PARAMS; portparam++)
    {
        p_iom_port_reg_info = &module_mgmt->iom_port_reg_info[portparam];
        fbe_set_memory(port_param_string, 0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH);
        fbe_set_memory(port_role_subrole_string, 0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH);
        fbe_set_memory(iom_group_string, 0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH);
    
        /* 
         * Query the registry for the CPD entry for this port.
         */

        status = fbe_module_mgmt_get_cpd_params(portparam, port_param_string);
        if(status != FBE_STATUS_OK)
        {
            /*
             * We have come to the last valid port param value in the registry.
             * Any more required PortParam entries will need to be written.
             * PortParams are expected to be in sequential order and if any
             * entry is missing that is assumed to be the last entry.
             */
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "mod_mgmt_reg_get_cpd_params registry found %d  PortParam entries\n", portparam);           
            break;
        }

        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "Read portparam %d with str %s\n",
                                  portparam, port_param_string);


        fbe_module_mgmt_reg_parse_string(module_mgmt,
                                         portparam,
                                         port_param_string, 
                                         FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                                         iom_group_string,
                                         port_role_subrole_string,
                                         TRUE,
                                         INVALID_CORE_NUMBER,
                                         INVALID_PHY_MAP,
                                         INVALID_PORT_U8);

        /* 
         * Convert the string we found in the registry to enum values
         * that are used in Flare.  These are stored in the global
         * iom_port_reg_info.
         */        
        fbe_module_mgmt_reg_convertCPDPortRoleSubRole(INVALID_PORT_U8,
            port_role_subrole_string,  
            FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
            &p_iom_port_reg_info->port_role, 
            &p_iom_port_reg_info->port_subrole, 
            TRUE);

        fbe_module_mgmt_reg_convertCPDIOMGroup(iom_group_string, 
            FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
            &p_iom_port_reg_info->iom_group, 
            TRUE);

        if( p_iom_port_reg_info->iom_group == FBE_IOM_GROUP_C )
        {
            // sas ports have some additional parameters to read in

            fbe_module_mgmt_reg_parse_sas_string(module_mgmt,
                    portparam,
                    port_param_string, 
                    FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                    iom_group_string,
                    port_role_subrole_string);

#if 0
            csx_p_sscanf(port_param_string, 
                    "{[%d.%d.%d] J(%s,%s,%d) U(%d) P(%d) F(%x)}",             
                    &p_iom_port_reg_info->pci_data.bus,             
                    &p_iom_port_reg_info->pci_data.device, 
                    &p_iom_port_reg_info->pci_data.function,
                    iom_group_string, 
                    port_role_subrole_string, 
                    &p_iom_port_reg_info->logical_port_number,
                    &p_iom_port_reg_info->core_num,
                    &p_iom_port_reg_info->port_count,
                    &p_iom_port_reg_info->phy_mapping);
#endif
        }
        
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
        FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_INFO,
        "Portparam %d - Role:%d, Logical:%d, Group:%d, [%d.%d.%d], Core num: %d\n",
        portparam,
        p_iom_port_reg_info->port_role,
        p_iom_port_reg_info->logical_port_number,
        p_iom_port_reg_info->iom_group,
        p_iom_port_reg_info->pci_data.bus, p_iom_port_reg_info->pci_data.device, p_iom_port_reg_info->pci_data.function,
        p_iom_port_reg_info->core_num);

    }
    module_mgmt->reg_port_param_count = portparam;
    return;
}




/**************************************************************************
 *          fbe_module_mgmt_reg_check_and_update_reg
 * ************************************************************************
 * @brief
 *  This function checks if a port needs to be updated in the
 *  Registry.
 *
 *
 * @param module_mgmt - object context
 *
 * @return fbe_bool_t - if registry has been updated.
 *
 *  NOTES:
 *      -- First if role and logical number of a portparam in registry are 
 *      not found in the data base, then it is overwritten with the UNC
 *      string and corresponding portparam number. This situation
 *      arises when we clear the data-base port info using NAVI, but
 *      the registry entries still exists. 
 *      -- If for any initialized ports with inserted compatible IOM's the 
 *      values differ from the data base, we over-write the registy entries.
 *      See 188058 for more info.
 *
 *  History:
 *  23-Mar-07: Brion Philbin - Created
 *  18-Jan-08: Rajesh V - Modified as part of getting dynamic PCI data from SPECL
 *  17-Sep-10: Brion Philbin - Rewritten for fbe_module_mgmt
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_check_and_update_registry(fbe_module_mgmt_t *module_mgmt)
{
    fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info;
    fbe_bool_t registry_updated = FALSE;
    fbe_u32_t iom_num=0, port_num=0;
    fbe_u32_t portparam = 0;
    fbe_module_slic_type_t slic_type;
    SPECL_PCI_DATA port_pci_info;
    fbe_u32_t phy_map;
    fbe_u32_t combined_port_count;
    fbe_u32_t num_ports;
    fbe_u32_t pci_function_offset;


    /* First read all the entries in the Registry. */
    fbe_module_mgmt_reg_get_all_cpd_params(module_mgmt);

    /* Check that the default entry is populated */
    if(module_mgmt->reg_port_param_count == 0)
    {
        fbe_module_mgmt_reg_set_default_param_entry(0);
        module_mgmt->reg_port_param_count++;
    }
        
    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        pci_function_offset = 0;
        /* Make sure this is an IO module that we want to configure data about. */
        slic_type = fbe_module_mgmt_get_slic_type(module_mgmt, module_mgmt->local_sp, iom_num);
        if((slic_type == FBE_SLIC_TYPE_UNSUPPORTED) || (slic_type == FBE_SLIC_TYPE_UNKNOWN) ||
           (!fbe_module_mgmt_is_iom_power_good(module_mgmt, module_mgmt->local_sp, iom_num)))
            //(!cm_flexports_is_iom_type_ok(iom_num, LOCAL_IOM_INFO)))
        {
            continue;
        }
        combined_port_count = fbe_module_mgmt_get_io_module_configured_combined_port_count(module_mgmt, module_mgmt->local_sp, iom_num);
        if(combined_port_count > 0)
        {
            combined_port_count = combined_port_count / 2;
        }
        num_ports = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt,module_mgmt->local_sp,iom_num);
        num_ports = num_ports - combined_port_count;
        for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
        {
            port_pci_info = fbe_module_mgmt_get_pci_info(module_mgmt, module_mgmt->local_sp, iom_num, port_num);

            if( (port_pci_info.bus == 0) && (port_pci_info.device == 0) && (port_pci_info.function == 0))
            {
                // skip over any 0.0.0 ports
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "mod_mgmt_reg_chk_update_registry iom:%d,port:%d,skipping due to invalid PCI address.\n",
                              iom_num, port_num);
                continue;
            }

            p_iom_port_reg_info = fbe_module_mgmt_reg_get_port_info(module_mgmt, 
                                                                    port_pci_info.bus,
                                                                    port_pci_info.device,
                                                                    port_pci_info.function);

            if(p_iom_port_reg_info == NULL)
            {
                if( (fbe_module_mgmt_is_port_initialized(module_mgmt, module_mgmt->local_sp, iom_num, port_num)) &&
                    (fbe_module_mgmt_is_port_configured_combined_port(module_mgmt, module_mgmt->local_sp, iom_num, port_num)) &&
                    (fbe_module_mgmt_is_port_second_combined_port(module_mgmt, module_mgmt->local_sp, iom_num, port_num)) )
                {
                    // we don't want to write out the registry settings for the second part of a combined port pair
                    
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "mod_mgmt_reg_chk_update_registry iom:%d,port:%d,skipping due to second combined port.\n",
                              iom_num, port_num);
                    continue;
                }
                /*
                * A corresponding PortParam entry with the specified PCI address
                * was not found.  Add a new entry for this address.
                */
                fbe_module_mgmt_reg_set_port_reg_info(module_mgmt, iom_num, port_num);
                registry_updated = TRUE; 
            }
            else if( (fbe_module_mgmt_is_port_initialized(module_mgmt, module_mgmt->local_sp, iom_num, port_num)) && 
                     (port_num < (num_ports+combined_port_count)))
            {
                fbe_ioport_role_t port_role;
                fbe_port_subrole_t port_subrole;
                fbe_bool_t iom_groups_differ = FALSE;
                fbe_u32_t logical_number;
                fbe_u32_t core_num = INVALID_CORE_NUMBER;

                portparam = fbe_module_mgmt_reg_get_port_param_number(module_mgmt, 
                                                                      port_pci_info.bus,
                                                                      port_pci_info.device,
                                                                      port_pci_info.function);
                port_role = fbe_module_mgmt_get_port_role(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
                port_subrole = fbe_module_mgmt_get_port_subrole(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
                iom_groups_differ = fbe_module_mgmt_reg_check_iom_groups_differ(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
                logical_number = fbe_module_mgmt_get_port_logical_number(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
                core_num = fbe_module_mgmt_generate_affinity_setting(module_mgmt, FALSE, &p_iom_port_reg_info->pci_data);

                if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt, module_mgmt->local_sp, iom_num, port_num))
                {
                    if(fbe_module_mgmt_is_port_second_combined_port(module_mgmt, module_mgmt->local_sp, iom_num, port_num))
                    {
                        /* 
                         * The second port in a combined port pair will have the same portal number as the first
                         * they look like a single port so there is nothing to do here.
                         */
                        continue;
                    }
                    phy_map = fbe_module_mgmt_get_combined_port_phy_mapping(module_mgmt, module_mgmt->local_sp, 
                            iom_num, port_num);
                }
                else
                {
                    phy_map = fbe_module_mgmt_get_port_phy_mapping(module_mgmt,module_mgmt->local_sp,
                                                                                iom_num,port_num);
                }

                // We should clear the CPD reg info for the port on wrong slic.
                // The check was put at here for we need port iom_num and port_num to check port state,
                // and it is easy to get such info in this loop
                if(fbe_module_mgmt_get_port_state(module_mgmt, module_mgmt->local_sp, iom_num, port_num) == FBE_PORT_STATE_FAULTED &&
                        fbe_module_mgmt_get_port_substate(module_mgmt, module_mgmt->local_sp, iom_num, port_num) == FBE_PORT_SUBSTATE_INCORRECT_MODULE)
                {
                    if(p_iom_port_reg_info->iom_group != FBE_IOM_GROUP_UNKNOWN)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s iom: %d, port: %d incorrect module, clear setting\n",
                                __FUNCTION__, iom_num, port_num);

                        fbe_module_mgmt_reg_set_port_reg_info(module_mgmt, iom_num, port_num);
                        registry_updated = TRUE;
                    }
                }
                // We will want to update registry when database port info differs
                // from that of in registry.
                else if ((iom_groups_differ) ||
                    (p_iom_port_reg_info->port_role != port_role) ||
                    (p_iom_port_reg_info->port_subrole != port_subrole) ||
                    (p_iom_port_reg_info->core_num == INVALID_CORE_NUMBER) ||
                    (p_iom_port_reg_info->core_num != core_num) ||
                    (p_iom_port_reg_info->logical_port_number != logical_number))
                {
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "mod_mgmt_reg_chk_update_registry iom:%d,port:%d,role:%d,subrole:%d\n",
                              iom_num, port_num,port_role,port_subrole);
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "mod_mgmt_reg_chk_update_registry reg_info_role:%d, reg_info_sub_role:%d iom_groups_differ:%d core_num:%d\n",
                              p_iom_port_reg_info->port_role, p_iom_port_reg_info->port_subrole, iom_groups_differ, p_iom_port_reg_info->core_num);
                    fbe_module_mgmt_reg_set_port_reg_info(module_mgmt, iom_num, port_num);
                    registry_updated = TRUE;
                }
                else if( (p_iom_port_reg_info->iom_group == FBE_IOM_GROUP_C) &&
                         ((fbe_module_mgmt_get_slic_type(module_mgmt,module_mgmt->local_sp, iom_num) == FBE_SLIC_TYPE_6G_SAS_3) ||
                          (fbe_module_mgmt_get_slic_type(module_mgmt,module_mgmt->local_sp, iom_num) == FBE_SLIC_TYPE_12G_SAS)) && 
                         ((p_iom_port_reg_info->phy_mapping != phy_map) ||
                          (p_iom_port_reg_info->port_count != num_ports)) )
                {
                    // sas port specific to update the additional parameters
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "mod_mgmt_reg_chk_update_registry iom:%d,port:%d,role:%d,updating due to SAS param change.\n",
                              iom_num, port_num,port_role);
                    
                    fbe_module_mgmt_reg_set_port_reg_info(module_mgmt, iom_num, port_num);
                    registry_updated = TRUE;

                }
            }
            else
            {
                //port is not initialized, make sure the phy mapping and port counts are correct.
                p_iom_port_reg_info->port_count = num_ports;
                p_iom_port_reg_info->phy_mapping = fbe_module_mgmt_get_port_phy_mapping(module_mgmt,module_mgmt->local_sp,iom_num,port_num);

                // If port is not initialized and core num not set, set the core num.
                if (p_iom_port_reg_info->core_num == INVALID_CORE_NUMBER)
                {
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "mod_mgmt_reg_chk_update_registry core_num:%d\n",
                              p_iom_port_reg_info->core_num);
                    fbe_module_mgmt_reg_set_port_reg_info(module_mgmt, iom_num, port_num);
                    registry_updated = TRUE;
                }
            }
        } // End of - FOR_EACH_PORT
    } // End of - FOR_EACH_IOM

    /* First check through each parameter to see if any need to be removed. */
    for(portparam = 0; portparam < module_mgmt->reg_port_param_count; portparam++)
    {
        if((module_mgmt->iom_port_reg_info[portparam].port_role != IOPORT_PORT_ROLE_UNINITIALIZED) ||
        (module_mgmt->iom_port_reg_info[portparam].port_subrole != FBE_PORT_SUBROLE_UNINTIALIZED))
        {
            if(!(fbe_module_mgmt_reg_port_config_exists(module_mgmt, portparam)))
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "mod_mgmt_reg_chk_update_registry PortParam:%d with logNum:%d, role:%d cleared in reg\n",
                                  portparam, 
                                  module_mgmt->iom_port_reg_info[portparam].logical_port_number,
                                  module_mgmt->iom_port_reg_info[portparam].port_role);
                module_mgmt->iom_port_reg_info[portparam].iom_group = FBE_IOM_GROUP_UNKNOWN;
                module_mgmt->iom_port_reg_info[portparam].port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
                module_mgmt->iom_port_reg_info[portparam].port_subrole = FBE_PORT_SUBROLE_UNINTIALIZED;
                module_mgmt->iom_port_reg_info[portparam].logical_port_number = portparam; 
                fbe_module_mgmt_reg_set_port_params(module_mgmt, portparam);
                registry_updated = TRUE;
            }
            else
            {
              fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "mod_mgmt_reg_chk_update_registry PortParam:%d with logNum:%d, role:%d does not differ\n",
                                    portparam, 
                                    module_mgmt->iom_port_reg_info[portparam].logical_port_number,
                                    module_mgmt->iom_port_reg_info[portparam].port_role);
            }
        }    
    }
    
    return(registry_updated);
}

/**************************************************************************
 *          fbe_module_mgmt_reg_check_iom_groups_differ
 * ************************************************************************
 *  @brief
 *  This function checks if the miniport driver name matches what is
 *  saved in the persistent storage.
 *
 * @param module_mgmt - object context
 * @param sp_id
 * @param iom_num - io module number
 * @param port_num - port number within that io module
 *
 * @return fbe_bool_t - if io module groups are different.
 *
 *
 *  History:
 *  17-Sep-10: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_check_iom_groups_differ(fbe_module_mgmt_t *module_mgmt, 
                                                       SP_ID sp_id, fbe_u32_t iom_num, 
                                                       fbe_u32_t port_num)
{
    SPECL_PCI_DATA port_pci_info;
    fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info;
    fbe_iom_group_t port_iom_group, reg_iom_group;

    port_pci_info = fbe_module_mgmt_get_pci_info(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
    p_iom_port_reg_info = fbe_module_mgmt_reg_get_port_info(module_mgmt,
                                                            port_pci_info.bus,
                                                            port_pci_info.device,
                                                            port_pci_info.function);

    port_iom_group = fbe_module_mgmt_get_iom_group(module_mgmt,sp_id, iom_num, port_num);
    reg_iom_group = p_iom_port_reg_info->iom_group;

    if(port_iom_group == reg_iom_group)
    {
        return FALSE;
    }

    /*
     * The fun here is that we have separate IOM_GROUPs for certain ports, however
     * they chan share the same miniport driver.  Here we have to group those
     * together to verify that the miniport driver name is appropriate for the 
     * configured io module group. 
     */ 


    // 1 iSCSI driver for 7 different iSCSI IOM_GROUPs
    if ((port_iom_group == FBE_IOM_GROUP_E
      || port_iom_group == FBE_IOM_GROUP_G
      || port_iom_group == FBE_IOM_GROUP_I
      || port_iom_group == FBE_IOM_GROUP_J
      || port_iom_group == FBE_IOM_GROUP_L
      || port_iom_group == FBE_IOM_GROUP_O
      || port_iom_group == FBE_IOM_GROUP_Q
      || port_iom_group == FBE_IOM_GROUP_S)
      && reg_iom_group  == FBE_IOM_GROUP_E)
    {
        return FALSE;
    }
    // iscsiql driver for both CNA and MaelstromX
    else if (port_iom_group == FBE_IOM_GROUP_R
            && reg_iom_group == FBE_IOM_GROUP_P)
    {
        return FALSE;
    }
    // 1 SAS driver for 3 different SAS IOM_GROUPs
    else if ((port_iom_group == FBE_IOM_GROUP_C
           || port_iom_group == FBE_IOM_GROUP_K
           || port_iom_group == FBE_IOM_GROUP_N)
           && reg_iom_group  == FBE_IOM_GROUP_C)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/**************************************************************************
 *          fbe_module_mgmt_reg_set_port_params
 * ************************************************************************
 *  @brief
 *  This function converts a set of information about a port into a valid
 *  registry string which is then written out to the registry.
 *
 *
 * @param module_mgmt - object context
 * @param port - Port Parameter Number
 *   
 *
 * @return fbe_bool_t - successful completion
 *
 *  History:
 *  05-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_set_port_params(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port)
{
    fbe_char_t port_param_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info;
    fbe_char_t iom_group_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t port_role_subrole_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_u32_t core_num = INVALID_CORE_NUMBER;
    fbe_u32_t phy_map = INVALID_PHY_MAP;
    fbe_u8_t port_count = INVALID_PORT_U8;
    fbe_u64_t device_type;
    fbe_u32_t slot;
    fbe_u32_t iom_num;
    fbe_u32_t port_index;

    
    fbe_set_memory(port_param_string, 0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH);
    fbe_set_memory(iom_group_string, 0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH);
    fbe_set_memory(port_role_subrole_string, 0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH);
    /* Get the port information for this port. */
    p_iom_port_reg_info = &module_mgmt->iom_port_reg_info[port];
    
    /* 
     * Convert the enum values to strings that can be written 
     * to the Registry.
     */
    fbe_module_mgmt_reg_convertCPDPortRoleSubRole(p_iom_port_reg_info->logical_port_number,
            port_role_subrole_string,  
            FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
            &p_iom_port_reg_info->port_role, 
            &p_iom_port_reg_info->port_subrole, 
            FALSE);

    fbe_module_mgmt_reg_convertCPDIOMGroup(iom_group_string, 
                FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                &p_iom_port_reg_info->iom_group, 
                FALSE);

    port_index = fbe_module_mgmt_get_port_index_from_pci_info(module_mgmt, 
                                                              p_iom_port_reg_info->pci_data.bus, 
                                                              p_iom_port_reg_info->pci_data.device, 
                                                              p_iom_port_reg_info->pci_data.function);


    if(port_index != INVALID_PORT_U8)
    {
        device_type = module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].deviceType;
        slot = module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].slotNumOnBlade;
        iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(device_type, slot);
        core_num = fbe_module_mgmt_generate_affinity_setting(module_mgmt, FALSE, &p_iom_port_reg_info->pci_data);
    
        if((fbe_module_mgmt_get_slic_type(module_mgmt, module_mgmt->local_sp, iom_num) == FBE_SLIC_TYPE_6G_SAS_3) ||
           (fbe_module_mgmt_get_slic_type(module_mgmt, module_mgmt->local_sp, iom_num) == FBE_SLIC_TYPE_12G_SAS))
        {            
            phy_map = p_iom_port_reg_info->phy_mapping;
            port_count = p_iom_port_reg_info->port_count;
        }        
    }
    else
    {
        //we will set highest core num for UNC ports
        core_num = fbe_module_mgmt_generate_affinity_setting(module_mgmt, FALSE, &p_iom_port_reg_info->pci_data);
    }
    /*
     * Convert all the separate strings and information to 
     * one formatted string for the Registry.
     */
    fbe_module_mgmt_reg_parse_string(module_mgmt,
                                     port,
                                     port_param_string, 
                                     FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                                     iom_group_string,
                                     port_role_subrole_string,
                                     FALSE,
                                     core_num,
                                     phy_map,
                                     port_count);


    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "ModMgmt: SetPort %d with string %s\n",
                                  port, port_param_string);
    /* Write that string out to the Registry */

    if(fbe_module_mgmt_set_cpd_params(port, port_param_string) 
        != FBE_STATUS_OK)
    {
        return FALSE;
    }

    return TRUE;
}

/**************************************************************************
 *          fbe_module_mgmt_reg_set_default_param_entry
 * ************************************************************************
 *  @brief
 *  This function writes out the default port param entry into the specified
 *  port param entry location.
 *
 *
 * @param PortParam - Port Parameter Number
 *   
 * @return fbe_bool_t - successful completion
 *
 *  History:
 *  11-Mar-09: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_set_default_param_entry(fbe_u32_t PortParam)
{    
#if !defined(UMODE_ENV)
    if(fbe_module_mgmt_set_cpd_params(PortParam, PORT_PARAMS_DEFAULT_STRING) 
        != FBE_STATUS_OK)
    {
        return FALSE;
    }
#endif

    return TRUE;
}


/**************************************************************************
 *          fbe_module_mgmt_reg_convertCPDPortRoleSubRole
 * ************************************************************************
 *  @brief
 *  This function converts a CPD registry entry in string to/from a 
 *  a flare useable enum value for a port role and subrole.  
 *
 * @param logical_number - port logical number, BE/FE #
 * @param port_role_subrole_string - string value for a cpd PortParams registry entry
 * @param buffer size - size of the supplied buffer
 * @param port_role - pointer to enum for port role
 * @param port_subrole - pointer to enum for port subrole
 * @param ascii_to_enum - T/F - T = convert from ASCII to Enum
 *                        F = convert from Enum to ASCII
 *   
 *
 * @return none
 *
 *  History:
 *  23-Feb-07: Brion Philbin - Created
 * ***********************************************************************/
void fbe_module_mgmt_reg_convertCPDPortRoleSubRole(fbe_u32_t logical_number,
                                                fbe_char_t *port_role_subrole_string,  
                                                fbe_u32_t buffer_size, 
                                                fbe_ioport_role_t *port_role, 
                                                fbe_port_subrole_t *port_subrole, 
                                                fbe_bool_t ascii_to_enum)
{    
    if(ascii_to_enum)
    {
        /*
         * The registry read must have been successful, convert the string
         * to the enums.
         * The Port Role and Subrole can be found in the second entry in
         * the PortParams string value.  This is easily found by searching
         * for the only entry that is in between two commas.
         */

        if(strncmp(port_role_subrole_string, BOOT_STRING, strlen(BOOT_STRING)) == 0)
        {
            *port_role = FBE_PORT_ROLE_BE;
            *port_subrole = FBE_PORT_SUBROLE_NORMAL;
        }
        else if(strncmp(port_role_subrole_string, BE_NORM_STRING, strlen(BE_NORM_STRING)) == 0)
        {
            *port_role = FBE_PORT_ROLE_BE;
            *port_subrole = FBE_PORT_SUBROLE_NORMAL;
        }
        else if(strncmp(port_role_subrole_string, FE_NORM_STRING, strlen(FE_NORM_STRING)) == 0)
        {
            *port_role = FBE_PORT_ROLE_FE;
            *port_subrole = FBE_PORT_SUBROLE_NORMAL;
        }
        else if(strncmp(port_role_subrole_string, FE_SPECIAL_STRING, strlen(FE_SPECIAL_STRING)) == 0)
        {
            *port_role = FBE_PORT_ROLE_FE;
            *port_subrole = FBE_PORT_SUBROLE_SPECIAL;
        }
        else
        {
            /* Anything else is assumed to be uninitialized */
            *port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
            *port_subrole = FBE_PORT_SUBROLE_UNINTIALIZED;
        }           
    }
    else
    {
        /*
         * We need to apped the port role and subrole to the buffer 
         * string so it can later be written out to the Registry.
         */
        
        // Zero out the buffer string
        fbe_set_memory(port_role_subrole_string, 0, buffer_size);

        if(*port_role == IOPORT_PORT_ROLE_BE)
        {
            if(logical_number == 0)
            {
                strncpy(port_role_subrole_string, BOOT_STRING, 
                    buffer_size-1);
            }
            else
            {
                strncpy(port_role_subrole_string, BE_NORM_STRING, 
                    buffer_size-1);
            }
        }
        else if(*port_role == IOPORT_PORT_ROLE_FE)
        {
            if(*port_subrole == FBE_PORT_SUBROLE_NORMAL)
            {
                strncpy(port_role_subrole_string, FE_NORM_STRING, 
                    buffer_size-1);
            }
            else
            {
                strncpy(port_role_subrole_string, FE_SPECIAL_STRING, 
                    buffer_size-1);
            }
        }
        else
        {
            strncpy(port_role_subrole_string, UNC_STRING, 
                buffer_size-1);
        }
    }
    return;

}

/**************************************************************************
 *          fbe_module_mgmt_reg_convertCPDIOMGroup
 * ************************************************************************
 *  @brief
 *  This function converts a CPD registry entry in string to/from a 
 *  a flare useable enum value for a port protocol.  
 *
 *
 * @param iom_group_string - string value for a cpd PortParams registry entry
 * @param buffer size - size of the supplied buffer
 * @param iom_group - enum for group of the iom
 * @param ascii_to_enum - T/F - T = convert from ASCII to Enum
 *                        F = convert from Enum to ASCII
 *   
 * @return none
 *
 *  History:
 *  23-Feb-07: Brion Philbin - Created
 * ***********************************************************************/
void fbe_module_mgmt_reg_convertCPDIOMGroup(fbe_char_t *iom_group_string, 
                                             fbe_u32_t buffer_size, 
                                             fbe_iom_group_t *iom_group, 
                                             fbe_bool_t ascii_to_enum)
{
    if(ascii_to_enum)
    {
        int i = 0;
        /*
         * The registry read must have been successful, convert the string
         * to the enums.
         * The port Protocol is specified through the directory name value 
         * of the Registry entry.  This is the first entry in the values
         * inside the parenthesis portion of the Registry entry.
         */
        *iom_group = FBE_IOM_GROUP_UNKNOWN;

        for (i = 0; i < fbe_driver_property_map_size; i++)
        {
            if (strncmp(iom_group_string, fbe_driver_property_map[i].driver, buffer_size - 1) == 0)
            {
                *iom_group = fbe_driver_property_map[i].default_group;
                break;
            }
        }
    }
    else
    {
        int i = 0;

        fbe_set_memory(iom_group_string, 0, buffer_size);

        for (i = 0; i < fbe_iom_group_property_map_size; i++)
        {
            if (fbe_iom_group_property_map[i].group == *iom_group)
            {
                strncpy(iom_group_string, fbe_iom_group_property_map[i].driver, buffer_size - 1);
                break;
            }
        }

        if (i == fbe_iom_group_property_map_size)
        {
            strncpy(iom_group_string, UNKNOWN_PROTOCOL_STRING, buffer_size - 1);
        }
    }
    return;
}

/**************************************************************************
 *          fbe_module_mgmt_reg_parse_string
 * ************************************************************************
 *  @brief
 *  This function takes out each of the pertinant strings from the one
 *  PortParameters string found in the Registry.  If the format of the
 *  registry value changes this function needs to be modified.
 *
 * @param module_mgmt - object context
 * @param port - port number
 * @param buffer - string value for a cpd PortParams registry entry
 * @param buffer size - size of the supplied buffer
 * @param iom_group_string - pointer to a iom group string
 * @param port_role_subrole_string - pointer to a port role and 
 *                             subrole string.
 * @param ascii_to_enum - convert from ascii string to enum or the reverse
 *   
 * @return none
 *
 *  History:
 *  3-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
void fbe_module_mgmt_reg_parse_string(fbe_module_mgmt_t *module_mgmt,
                                      fbe_u32_t port_param,
                                      fbe_char_t *buffer, 
                                      fbe_u32_t buffer_size,
                                      fbe_char_t *iom_group_string,
                                      fbe_char_t *port_role_subrole_string,
                                      fbe_bool_t ascii_to_enum,
                                      fbe_u32_t core_num,
                                      fbe_u32_t phy_map,
                                      fbe_u8_t port_count)
{
    fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info;

    
    p_iom_port_reg_info = &module_mgmt->iom_port_reg_info[port_param];        
   

    if(ascii_to_enum)
    {
        /*
         * The value in the registry is in a predefined format and is read assuming
         * that format.  If the expected items are not found then the entry is
         * set to uninitialized to be recovered later.
         */
        if(!fbe_module_mgmt_reg_scan_string_with_core_num(buffer, buffer_size,
            &p_iom_port_reg_info->pci_data.bus, 
            &p_iom_port_reg_info->pci_data.function, 
            &p_iom_port_reg_info->pci_data.device, 
            iom_group_string, port_role_subrole_string, 
            &p_iom_port_reg_info->logical_port_number,
            &p_iom_port_reg_info->core_num))
        {
            if(fbe_module_mgmt_reg_scan_string(buffer, buffer_size,
            &p_iom_port_reg_info->pci_data.bus, 
            &p_iom_port_reg_info->pci_data.function, 
            &p_iom_port_reg_info->pci_data.device, 
            iom_group_string, port_role_subrole_string, 
            &p_iom_port_reg_info->logical_port_number))
        {
                p_iom_port_reg_info->core_num = INVALID_CORE_NUMBER;
            }
            else
            {
            /* Fill in the values as though this is an uninitialized port */
            fbe_set_memory(iom_group_string, 0, buffer_size); 
            strncpy(iom_group_string, UNKNOWN_PROTOCOL_STRING, buffer_size-1);

            fbe_set_memory(port_role_subrole_string, 0, buffer_size); 
            strncpy(port_role_subrole_string, UNC_STRING, buffer_size-1);
            p_iom_port_reg_info->pci_data.bus = 0; 
            p_iom_port_reg_info->pci_data.function = 0;
            p_iom_port_reg_info->pci_data.device = 0;   
            p_iom_port_reg_info->logical_port_number = INVALID_PORT_U8;
                p_iom_port_reg_info->core_num = INVALID_CORE_NUMBER;
            }
        }
    }
    else
    {
        if( (core_num != INVALID_CORE_NUMBER) &&
            (phy_map != INVALID_PHY_MAP) &&
            (port_count != INVALID_PORT_U8) )
        {
            /* Processor core affinity must be set*/
            fbe_sprintf(buffer, (buffer_size - 1),
                "{[%d.%d.%d] J(%s,%s,%d) U(%d) P(%d) F(%x)}",             
                p_iom_port_reg_info->pci_data.bus,             
                p_iom_port_reg_info->pci_data.device, 
                p_iom_port_reg_info->pci_data.function,
                iom_group_string, 
                port_role_subrole_string, 
                p_iom_port_reg_info->logical_port_number,
                core_num,port_count,phy_map);
        }
        else if(core_num != INVALID_CORE_NUMBER)
        {
            /* Processor core affinity must be set*/
            fbe_sprintf(buffer, (buffer_size - 1),
                "{[%d.%d.%d] J(%s,%s,%d) U(%d)}",             
                p_iom_port_reg_info->pci_data.bus,             
                p_iom_port_reg_info->pci_data.device, 
                p_iom_port_reg_info->pci_data.function,
                iom_group_string, 
                port_role_subrole_string, 
                p_iom_port_reg_info->logical_port_number,
                core_num);
        }
        else
        {
            fbe_sprintf(buffer, (buffer_size - 1),
                "{[%d.%d.%d] J(%s,%s,%d)}",             
                p_iom_port_reg_info->pci_data.bus,             
                p_iom_port_reg_info->pci_data.device, 
                p_iom_port_reg_info->pci_data.function,
                iom_group_string, 
                port_role_subrole_string, 
                p_iom_port_reg_info->logical_port_number);
        }
    }
    return;
} // end fbe_module_mgmt_reg_parse_string


/**************************************************************************
 *          fbe_module_mgmt_reg_parse_sas_string
 * ************************************************************************
 *  @brief
 *  This function takes out each of the pertinant strings from the one
 *  PortParameters string found in the Registry.  If the format of the
 *  registry value changes this function needs to be modified.
 *  The string looks something like this:
 *  {[148.0.2] J(saspmc,BE_NORM,6) U(14) P(4) F(f00)}
 *
 * @param module_mgmt - object context
 * @param port - port number
 * @param buffer - string value for a cpd PortParams registry entry
 * @param buffer size - size of the supplied buffer
 * @param iom_group_string - pointer to a iom group string
 * @param port_role_subrole_string - pointer to a port role and 
 *                             subrole string.
 *   
 * @return none
 *
 *  History:
 *  3-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
void fbe_module_mgmt_reg_parse_sas_string(fbe_module_mgmt_t *module_mgmt,
                                      fbe_u32_t port_param,
                                      fbe_char_t *buffer, 
                                      fbe_u32_t buffer_size,
                                      fbe_char_t *iom_group_string,
                                      fbe_char_t *port_role_subrole_string)
{
    fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info;

    
    p_iom_port_reg_info = &module_mgmt->iom_port_reg_info[port_param];        
   

    
    /*
     * The value in the registry is in a predefined format and is read assuming
     * that format.  If the expected items are not found then the entry is
     * set to uninitialized to be recovered later.
     */
    if(!fbe_module_mgmt_reg_scan_sas_string(buffer, buffer_size, p_iom_port_reg_info,
                                            iom_group_string, port_role_subrole_string))
    {
        /* Fill in just the sas specific values */
        
        // For an UNC port, we set highest core number to it.
        if(p_iom_port_reg_info->core_num == INVALID_CORE_NUMBER)
        {
           p_iom_port_reg_info->core_num = fbe_module_mgmt_generate_affinity_setting(module_mgmt, true, &p_iom_port_reg_info->pci_data);    
        }
        
        p_iom_port_reg_info->port_count = 0;
        p_iom_port_reg_info->phy_mapping = 0;
    }
    return;
} // end fbe_module_mgmt_reg_parse_string

/**************************************************************************
 *          fbe_module_mgmt_reg_scan_sas_string
 * ************************************************************************
 *  @brief
 *  This function takes out each of the pertinant strings from the 
 *  PortParameters string found in the Registry.  If the format of the
 *  registry value changes this function needs to be modified.
 *
 *
 * @param buffer - string value for a cpd PortParams registry entry
 * @param buffer size - size of the supplied buffer
 * @param bus - pci bus address
 * @param func - pci func address
 * @param slot - pci slot address
 * @param iom_group_string - pointer to a iom group string
 * @param port_role_subrole_string - pointer to a port role and 
 *                             subrole string.
 * @param logical_port_number - pointer to a logical port number
 *   
 *
 * @return fbe_bool_t - TRUE - if all values discovered in the string
 *
 *  History:
 *  3-July-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_scan_sas_string(char *buffer, 
                             fbe_u32_t buffer_size, 
                             fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info, 
                             fbe_char_t *iom_group_string,
                             fbe_char_t *port_role_subrole_string)
{
    fbe_u32_t index = 0;
    fbe_char_t temp_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_u32_t ignore;


    if(!fbe_module_mgmt_reg_scan_string_for_string('[', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, &ignore))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, &ignore))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(']', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, &ignore))
    {
        return FALSE;
    }   
    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, iom_group_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, port_role_subrole_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(')', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, &p_iom_port_reg_info->core_num))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(')', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, &p_iom_port_reg_info->port_count))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_hex(')', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, &p_iom_port_reg_info->phy_mapping))
    {
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 *          fbe_module_mgmt_reg_scan_string
 * ************************************************************************
 *  @brief
 *  This function takes out each of the pertinant strings from the 
 *  PortParameters string found in the Registry.  If the format of the
 *  registry value changes this function needs to be modified.
 *
 *
 * @param buffer - string value for a cpd PortParams registry entry
 * @param buffer size - size of the supplied buffer
 * @param bus - pci bus address
 * @param func - pci func address
 * @param slot - pci slot address
 * @param iom_group_string - pointer to a iom group string
 * @param port_role_subrole_string - pointer to a port role and 
 *                             subrole string.
 * @param logical_port_number - pointer to a logical port number
 *   
 *
 * @return fbe_bool_t - TRUE - if all values discovered in the string
 *
 *  History:
 *  3-July-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_scan_string(char *buffer, 
                             fbe_u32_t buffer_size, 
                             fbe_u32_t *bus, 
                             fbe_u32_t *func, 
                             fbe_u32_t *slot, 
                             fbe_char_t *iom_group_string,
                             fbe_char_t *port_role_subrole_string, 
                             fbe_u32_t *logical_port_number)
{
    fbe_u32_t index = 0;
    fbe_char_t temp_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];


    if(!fbe_module_mgmt_reg_scan_string_for_string('[', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, bus))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, slot))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(']', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, func))
    {
        return FALSE;
    }   
    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, iom_group_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, port_role_subrole_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(')', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, logical_port_number))
    {
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 *          fbe_module_mgmt_reg_scan_string_with_core_num
 * ************************************************************************
 *  @brief
 *  This function takes out each of the pertinant strings from the
 *  PortParameters string found in the Registry.  If the format of the
 *  registry value changes this function needs to be modified.
 *
 *
 * @param buffer - string value for a cpd PortParams registry entry
 * @param buffer size - size of the supplied buffer
 * @param bus - pci bus address
 * @param func - pci func address
 * @param slot - pci slot address
 * @param iom_group_string - pointer to a iom group string
 * @param port_role_subrole_string - pointer to a port role and
 *                             subrole string.
 * @param logical_port_number - pointer to a logical port number
 *
 *
 * @return fbe_bool_t - TRUE - if all values discovered in the string
 *
 *  History:
 *  3-July-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_scan_string_with_core_num(char *buffer,
                             fbe_u32_t buffer_size,
                             fbe_u32_t *bus,
                             fbe_u32_t *func,
                             fbe_u32_t *slot,
                             fbe_char_t *iom_group_string,
                             fbe_char_t *port_role_subrole_string,
                             fbe_u32_t *logical_port_number,
                             fbe_u32_t *core_num)
{
    fbe_u32_t index = 0;
    fbe_char_t temp_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];


    if(!fbe_module_mgmt_reg_scan_string_for_string('[', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, bus))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, slot))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(']', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, func))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, iom_group_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_string(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, port_role_subrole_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(')', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, logical_port_number))
    {
        return FALSE;
    }

    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, temp_string))
    {
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(')', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, buffer, &index, core_num))
    {
        return FALSE;
    }
    return TRUE;
}


/**************************************************************************
 *          fbe_module_mgmt_reg_scan_string_for_int
 * ************************************************************************
 *  @brief
 *  This function copies in the characters of a string up to either a
 *  specified length or to a stop character.  It saves the result in
 *  the pointer passed in.  This function assumes buffer is valid and
 *  index points to a valid location inside of buffer.
 *
 *
 * @param stop - stop character
 * @param length - maximum length to scan
 * @param buffer - character buffer to scan through
 * @param index - index pointer into the buffer (for start)
 * @param result - pointer to store the integer result
 *   
 *
 * @return fbe_bool_t - TRUE - success
 *
 *  History:
 *  3-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_scan_string_for_int(fbe_char_t stop, fbe_u32_t length, 
                                             fbe_char_t *buffer, fbe_u32_t *index, 
                                             fbe_u32_t *result)
{
    fbe_u32_t offset=0;
    fbe_char_t temp_buffer[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    /* 
     * Iterate through the buffer up to the maximum length or
     * the stop character.
     */
    while((offset < length) && (buffer[*index] != stop))
    {
        temp_buffer[offset] = buffer[*index];
        offset++;
        (*index)++;
    }
    if(offset == length)
    {
        return FALSE;
    }
    else
    {
        temp_buffer[offset] = 0; // add a trailing null onto the string
        *result = csx_p_atoi_s32(temp_buffer);
        (*index)++;
        return TRUE;
    }


}

/**************************************************************************
 *          fbe_module_mgmt_reg_scan_string_for_hex
 * ************************************************************************
 *  @brief
 *  This function copies in the characters of a string up to either a
 *  specified length or to a stop character.  It saves the result in
 *  the pointer passed in.  This function assumes buffer is valid and
 *  index points to a valid location inside of buffer.
 *
 *
 * @param stop - stop character
 * @param length - maximum length to scan
 * @param buffer - character buffer to scan through
 * @param index - index pointer into the buffer (for start)
 * @param result - pointer to store the integer result
 *   
 *
 * @return fbe_bool_t - TRUE - success
 *
 *  History:
 *  3-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_scan_string_for_hex(fbe_char_t stop, fbe_u32_t length, 
                                             fbe_char_t *buffer, fbe_u32_t *index, 
                                             fbe_u32_t *result)
{
    fbe_u32_t offset=0;
    fbe_char_t temp_buffer[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_bool_t failed;

    /* 
     * Iterate through the buffer up to the maximum length or
     * the stop character.
     */
    while((offset < length) && (buffer[*index] != stop))
    {
        temp_buffer[offset] = buffer[*index];
        offset++;
        (*index)++;
    }
    if(offset == length)
    {
        return FALSE;
    }
    else
    {
        temp_buffer[offset] = 0; // add a trailing null onto the string
        //*result = csx_p_atoh_s32(temp_buffer);
        *result = csx_p_cvt_string_to_s32(temp_buffer, CSX_NULL, 16, &failed);
        (*index)++;
        return TRUE;
    }

}
/**************************************************************************
 *          fbe_module_mgmt_reg_scan_string_for_string
 * ************************************************************************
 *  @brief
 *  This function copies in the characters of a string up to either a
 *  specified length or to a stop character.  It saves the result in
 *  the pointer passed in.  This function assumes buffer is valid and
 *  index points to a valid location inside of buffer.
 *
 *
 * @param stop - stop character
 * @param length - maximum length to scan
 * @param buffer - character buffer to scan through
 * @param index - index pointer into the buffer (for start)
 * @param result - pointer to store the string result
 *   
 *
 * @return fbe_bool_t - TRUE - success
 *
 *  History:
 *  3-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_bool_t fbe_module_mgmt_reg_scan_string_for_string(fbe_char_t stop, fbe_u32_t length, 
                                                fbe_char_t *buffer, fbe_u32_t *index, 
                                                fbe_char_t *result)
{
    fbe_u32_t offset=0;

    /* 
     * Iterate through the buffer up to the maximum length or
     * the stop character.
     */
    while((offset < length) && (buffer[*index] != stop))
    {
        result[offset] = buffer[*index];
        offset++;
        (*index)++;
    }
    if(offset == length)
    {
        return FALSE;
    }
    else
    {
        (*index)++;
        return TRUE;
    }
}
/**************************************************************************
 *          fbe_module_mgmt_reg_set_port_reg_info
 * ************************************************************************
 *  @brief
 *  This function converts the persistent storage configuration data
 *  to the port_reg_info information to then be written out to the registry.
 *
 *
 * @param module_mgmt - object context
 * @param iom_num - io module number
 * @param port_num - port number on that io module
 *   
 *
 * @return none
 *
 *  History:
 *  21-Mar-07: Brion Philbin - Created
 *  18-Jan-08: Rajesh V - Modified as part of getting PCI data from SPECL
 * ***********************************************************************/
void fbe_module_mgmt_reg_set_port_reg_info(fbe_module_mgmt_t *module_mgmt, 
                                           fbe_u32_t iom_num, fbe_u32_t port_num)
{
    fbe_module_mgmt_port_reg_info_t *p_iom_port_reg_info;
    fbe_u32_t port_param = 0; 
    SPECL_PCI_DATA port_pci_addr;
    fbe_u32_t combined_port_count = 0;
    fbe_u32_t num_ports = 0;

    port_pci_addr = fbe_module_mgmt_get_pci_info(module_mgmt,module_mgmt->local_sp,iom_num,port_num);

    p_iom_port_reg_info = fbe_module_mgmt_reg_get_port_info(module_mgmt,
                                                            port_pci_addr.bus,
                                                            port_pci_addr.device,
                                                            port_pci_addr.function);
    
    
               
    if(p_iom_port_reg_info == NULL)
    {
        /*
         * No entry was found with this PCI address, we need to create a new one.
         */
        // overwrite the default parameter with this new valid parameter
        port_param = module_mgmt->reg_port_param_count - 1;
        p_iom_port_reg_info = &module_mgmt->iom_port_reg_info[port_param];            
        p_iom_port_reg_info->pci_data.bus = port_pci_addr.bus;
        p_iom_port_reg_info->pci_data.device = port_pci_addr.device;
        p_iom_port_reg_info->pci_data.function = port_pci_addr.function;
        
        
        // write the default parameter in the new last position
        fbe_module_mgmt_reg_set_default_param_entry(module_mgmt->reg_port_param_count);
        module_mgmt->reg_port_param_count++;
    }
    else
    {
        port_param = fbe_module_mgmt_reg_get_port_param_number(module_mgmt,
                                                               port_pci_addr.bus,
                                                               port_pci_addr.function,
                                                               port_pci_addr.device);
    }

    /*
     * For port combinations we need to save a value in the registry of the overall port count for 
     * a given IO Module.  The combined ports are subtracted from the overall port count for that 
     * SLIC. 
     */
    combined_port_count = fbe_module_mgmt_get_io_module_configured_combined_port_count(module_mgmt, module_mgmt->local_sp, iom_num);
    if(combined_port_count > 0)
    {
        combined_port_count = combined_port_count / 2;
    }
    num_ports = fbe_module_mgmt_get_num_ports_present_on_iom(module_mgmt,module_mgmt->local_sp,iom_num);
    num_ports = num_ports - combined_port_count;

    /*
     * Fill in the combined port information for number of ports and phy mapping.
     */
    p_iom_port_reg_info->port_count = num_ports;  
    
    if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt, module_mgmt->local_sp, iom_num, port_num))
    {
        p_iom_port_reg_info->phy_mapping = fbe_module_mgmt_get_combined_port_phy_mapping(module_mgmt,module_mgmt->local_sp,
                                                                                         iom_num,port_num);
    }
    else
    {
        p_iom_port_reg_info->phy_mapping = fbe_module_mgmt_get_port_phy_mapping(module_mgmt,module_mgmt->local_sp,
                                                                                iom_num,port_num);
    }

    //We need to clear the CPD reg info for the port on wrong slic.
    //So we will set such ports CPD reg info to "UNC" here.
    if(fbe_module_mgmt_is_port_initialized(module_mgmt, module_mgmt->local_sp, iom_num, port_num) &&
       !(fbe_module_mgmt_get_port_state(module_mgmt, module_mgmt->local_sp, iom_num, port_num) == FBE_PORT_STATE_FAULTED &&
       fbe_module_mgmt_get_port_substate(module_mgmt, module_mgmt->local_sp, iom_num, port_num) == FBE_PORT_SUBSTATE_INCORRECT_MODULE))
    {
        p_iom_port_reg_info->iom_group = fbe_module_mgmt_get_iom_group(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
        p_iom_port_reg_info->port_role = fbe_module_mgmt_get_port_role(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
        p_iom_port_reg_info->port_subrole = fbe_module_mgmt_get_port_subrole(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
        p_iom_port_reg_info->logical_port_number = fbe_module_mgmt_get_port_logical_number(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
        
        
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s PortParam:%d set to %s %d, iom group:%d\n",
                              __FUNCTION__, port_param, 
                              fbe_module_mgmt_port_role_to_string(p_iom_port_reg_info->port_role),
                              p_iom_port_reg_info->logical_port_number,
                              p_iom_port_reg_info->iom_group);
    }
    else
    {
        fbe_u8_t port_index;
        port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
        p_iom_port_reg_info->iom_group = FBE_IOM_GROUP_UNKNOWN;
        p_iom_port_reg_info->port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
        p_iom_port_reg_info->port_subrole = FBE_PORT_SUBROLE_UNINTIALIZED;
        p_iom_port_reg_info->logical_port_number = port_index; 

        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s PortParam:%d set to UNC:%d\n",
                              __FUNCTION__, port_param, 
                              p_iom_port_reg_info->logical_port_number);
    }

    fbe_module_mgmt_reg_set_port_params(module_mgmt, port_param);

    return;
}
     
/**************************************************************************
 *          fbe_module_mgmt_reg_get_port_info
 * ************************************************************************
 *  @brief
 *  This function is searches through the mappings of pci addresses to 
 *  port addresses to get the corresponding entry in the registry port info.
 *
 *
 * @param module_mgmt - object context
 * @param bus - PCI bus
 * @param func - PCI function
 * @param slot - PCI device
 *
 * @return fbe_module_mgmt_port_reg_info_t * - pointer to the registry port info entry.
 *
 *  History:
 *  24-Aug-07: Brion Philbin - Created
 *  18-Jan-08: Rajesh V - Modified as part of getting PCI data from SPECL
 * ***********************************************************************/
fbe_module_mgmt_port_reg_info_t * fbe_module_mgmt_reg_get_port_info(fbe_module_mgmt_t *module_mgmt,
                                                                    fbe_u32_t bus, fbe_u32_t slot,
                                                                    fbe_u32_t func)
{
    fbe_u32_t portparam;

    for(portparam = 0; portparam <= module_mgmt->reg_port_param_count; portparam++)
    {
        if((module_mgmt->iom_port_reg_info[portparam].pci_data.bus == bus) &&
           (module_mgmt->iom_port_reg_info[portparam].pci_data.function == func) &&
           (module_mgmt->iom_port_reg_info[portparam].pci_data.device == slot))
        {
            return &module_mgmt->iom_port_reg_info[portparam];
        }
    }
    return NULL;
}

/**************************************************************************
 *          fbe_module_mgmt_reg_get_port_param_number
 * ************************************************************************
 *  @brief
 *  This function is searches through the mappings of pci addresses to 
 *  port addresses to get the corresponding entry in the registry port info
 *  parameter number.
 *
 *
 * @param module_mgmt - object context
 * @param bus - PCI bus
 * @param func - PCI function
 * @param slot - PCI device
 *
 * @return portparam - number of the PortParams entry in the registry.
 *
 *  History:
 *  24-Aug-07: Brion Philbin - Created
 *  18-Jan-08: Rajesh V - Modified as part of getting PCI data from SPECL
 * ***********************************************************************/
fbe_u32_t fbe_module_mgmt_reg_get_port_param_number(fbe_module_mgmt_t *module_mgmt, 
                                                    fbe_u32_t bus, fbe_u32_t func, 
                                                    fbe_u32_t slot)
{
    fbe_u32_t portparam;

    for(portparam = 0; portparam <= module_mgmt->reg_port_param_count; portparam++)
    {
        if((module_mgmt->iom_port_reg_info[portparam].pci_data.bus == bus) &&
           (module_mgmt->iom_port_reg_info[portparam].pci_data.function == func) &&
           (module_mgmt->iom_port_reg_info[portparam].pci_data.device == slot))
        {
            return portparam;
        }
    }
    return INVALID_PORT_U8;
}

/**************************************************************************
 *    fbe_module_mgmt_reg_port_config_exists
 **************************************************************************
 *
 *  @brief
 *   This function retuns  true if a port with corresponding role and logical
 *   number exists in the sysconfig database also match with PCI data.
 *    
 *
 * @param module_mgmt - object context
 * @param portparam - port number
 *
 * @return - fbe_bool_t TRUE or FALSE specified port exists in the DB
 *
 *  HISTORY:
 *      17-Jan-08 - Rajesh V - created.
  **************************************************************************/
fbe_bool_t fbe_module_mgmt_reg_port_config_exists(fbe_module_mgmt_t *module_mgmt, 
                                                fbe_u32_t portparam)
{
    fbe_u32_t iom_num=0, port_num=0;
    SP_ID sp_id = module_mgmt->local_sp;
    SPECL_PCI_DATA port_pci_info;

     for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
     {
         for(port_num = 0; port_num < MAX_IO_PORTS_PER_MODULE; port_num++)
         {
            port_pci_info = fbe_module_mgmt_get_pci_info(module_mgmt,sp_id,iom_num,port_num);

            if((fbe_module_mgmt_get_port_role(module_mgmt, sp_id, iom_num, port_num) == module_mgmt->iom_port_reg_info[portparam].port_role) &&
               (fbe_module_mgmt_get_port_logical_number(module_mgmt, sp_id, iom_num, port_num) == module_mgmt->iom_port_reg_info[portparam].logical_port_number) &&
               (port_pci_info.bus == module_mgmt->iom_port_reg_info[portparam].pci_data.bus) &&
               (port_pci_info.function == module_mgmt->iom_port_reg_info[portparam].pci_data.function) &&
               (port_pci_info.device == module_mgmt->iom_port_reg_info[portparam].pci_data.device))   
            {
                return(TRUE);
            }
            else if((fbe_module_mgmt_get_port_role(module_mgmt, sp_id, iom_num, port_num) == module_mgmt->iom_port_reg_info[portparam].port_role) &&
               (fbe_module_mgmt_get_port_logical_number(module_mgmt, sp_id, iom_num, port_num) == module_mgmt->iom_port_reg_info[portparam].logical_port_number))
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s PortParam:%d Config Match, but PCI %d:%d:%d does not match.\n",
                              __FUNCTION__, portparam, port_pci_info.bus, port_pci_info.function, port_pci_info.device);

            }
        }
    }
    return(FALSE);
}

void fbe_module_mgmt_remove_port_param_entry(fbe_module_mgmt_t *module_mgmt, fbe_u32_t portparam)
{
    fbe_u32_t tempparam;
    
    for(tempparam = portparam; tempparam < module_mgmt->reg_port_param_count -1; tempparam++)
    {
        fbe_copy_memory(&module_mgmt->iom_port_reg_info[tempparam], &module_mgmt->iom_port_reg_info[tempparam+1], sizeof(fbe_module_mgmt_port_reg_info_t));
        fbe_module_mgmt_reg_set_port_params(module_mgmt, tempparam);
    }

    module_mgmt->reg_port_param_count--;

    return;
}







