/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_reg_io_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management utility Registry read/write
 *  functionalities.
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   02-Sep-2010 - Created. bphilbin
 *
 ***************************************************************************/
#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_registry.h"


#define FBE_MODULE_MGMT_SLIC_UPGRADE_KEY "ESPSLICsMarkedForUpgrade"
#define FBE_MODULE_MGMT_NETCONF_REINITIALIZE_KEY "NetconfReinitialize"
#define FBE_PCI_REG_LOCATION_INFORMATION "LocationInformation"
#define FBE_PROCESSOR_AFFINITY_KEY "AssignmentSetOverride"
#define FBE_MODULE_MGMT_ENABLE_PORT_INTERRUPT_AFFINITY "ESPEnablePortInterruptAffinity"
#define FBE_MSI_MESSAGE_NUMBER_LIMIT_KEY "MessageNumberLimit"
#define FBE_MODULE_MGMT_REBOOT_REQUIRED "AdditionalRebootRequired"


 /*
  * The default will be to use a string UNKNOWN when no Registry entry is found for
  * a particular port.
  */
#define CPD_PORT_PARAMS_DEFAULT_VALUE  "UNKNOWN"


/**************************************************************************
 *          fbe_module_mgmt_set_cpd_params
 * ************************************************************************
 *  @brief
 *  Called to set the value of registry key - PortParams#.  Because
 *  this function directly writes to the registry it must be called
 *  at init time.  It may lead to a panic waiting for the write to
 *  complete if called at run-time.
 *
 *  @param port - PortParam# to store in the registry.
 *  @param port_param_string - string entry to write to the PortParams# Key.
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  08-Feb-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_cpd_params(fbe_u32_t port, fbe_char_t *port_param_string)
{
    fbe_status_t    status;
    fbe_char_t      PortParamsKeyStr[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_sprintf(PortParamsKeyStr, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "PortParams%d",port);
    
    /*
     * Set the registry key, if we fail here we will return this status
     * as this is the critical operation.
     */    
    status = fbe_registry_write(NULL,
                                CPD_REG_PATH,
                                PortParamsKeyStr, 
                                FBE_REGISTRY_VALUE_SZ,
                                port_param_string, 
                                (fbe_u32_t)strlen(port_param_string));

    return status;
}



/**************************************************************************
 *          fbe_module_mgmt_get_cpd_params
 * ************************************************************************
 *  @brief
 *  Called to get the value of registry key - PortParams#.  Because
 *  this function directly reads from the registry it must be called
 *  at init time.  It may lead to a panic waiting for the read to
 *  complete if called at run-time.
 *
 *  @param port - PortParam# to read from the registry.
 *  @param port_param_string - string entry to read from the PortParams# Key.
 *   
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  08-Feb-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_cpd_params(fbe_u32_t port, fbe_char_t *port_param_string)
{
    fbe_status_t        status;
    fbe_char_t                PortParamsKeyStr[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];


    fbe_sprintf(PortParamsKeyStr, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "PortParams%d",port);   

    status = fbe_registry_read(NULL,
                              CPD_REG_PATH,
                              PortParamsKeyStr,
                              port_param_string,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FBE_REGISTRY_VALUE_SZ,
                              CPD_PORT_PARAMS_DEFAULT_VALUE,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FALSE);

    return status;
}

/**************************************************************************
 *          fbe_module_mgmt_set_persist_port_info
 * ************************************************************************
 *  @brief
 *  Called to set the persis port info registry key.
 *
 *  @param module_mgmt - object context
 *  @param persist_port_info - whether or not to persist the port information
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  19-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_persist_port_info(fbe_module_mgmt_t *module_mgmt, fbe_bool_t persist_port_info)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                           ESP_REG_PATH,
                           FBE_MODULE_MGMT_PORT_PERSIST_KEY, 
                           FBE_REGISTRY_VALUE_DWORD,
                           &persist_port_info, 
                           sizeof(fbe_bool_t));

    /*
     * This data is critical and should always be accessible.  Trace 
     * an error if this fails. 
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to set registry entry status:0x%x\n",
                              __FUNCTION__, status);
    }

    status = fbe_flushRegistry();
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to flush registry status:0x%x\n",
                              __FUNCTION__, status);
    }

    return (status);
}


/**************************************************************************
 *          fbe_module_mgmt_get_persist_port_info
 * ************************************************************************
 *  @brief
 *  Called to set the persis port info registry key.
 *
 *  @param module_mgmt - object context
 *  @param persist_port_info - whether or not to persist the port information
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  19-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_persist_port_info(fbe_module_mgmt_t *module_mgmt, fbe_bool_t *persist_port_info)
{
    fbe_status_t status;
    fbe_u32_t DefaultInputValue;

    DefaultInputValue = 0;

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_MODULE_MGMT_PORT_PERSIST_KEY,
                               persist_port_info,
                               sizeof(fbe_bool_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &DefaultInputValue,
                               sizeof(fbe_bool_t),
                               TRUE);

    /*
     * This data is critical and should always be accessible.  Trace 
     * an error if this fails. 
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get registry entry status:0x%x\n",
                              __FUNCTION__, status);
    }

    return (status);
}

/**************************************************************************
 *          fbe_module_mgmt_set_netconf_reinitialize
 * ************************************************************************
 *  @brief
 *  Called to notify netconf to reinitialize the ports as they have been
 *  removed.
 *
 *  @param None.
 * 
 *  @return fbe_status_t - status
 *
 *  History:
 *  16-Apr-09: Brion Philbin - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_netconf_reinitialize(void)
{
    fbe_status_t status;
    fbe_u32_t             reinitialize = 1;

    status = fbe_registry_write(NULL,
                                ESP_REG_PATH,
                                FBE_MODULE_MGMT_NETCONF_REINITIALIZE_KEY, 
                                FBE_REGISTRY_VALUE_DWORD,
                                &reinitialize, 
                                sizeof(fbe_u32_t));

    return (status);
}

/**************************************************************************
 *          fbe_module_mgmt_set_slics_marked_for_upgrade
 * ************************************************************************
 *  @brief
 *  Called to set registry entry to mark all SLICs on this SP for upgrade 
 *  on the next boot.
 *
 *  @param marked_for_upgrade - whether or not to mark slics for upgrade
 * 
 *  @return fbe_status_t - status
 *
 *  History:
 *  27-Nov-08: Nayana Chaudhari - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_slics_marked_for_upgrade(fbe_module_mgmt_upgrade_type_t *marked_for_upgrade)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                               ESP_REG_PATH,
                               FBE_MODULE_MGMT_SLIC_UPGRADE_KEY, 
                               FBE_REGISTRY_VALUE_DWORD,
                               marked_for_upgrade, 
                               sizeof(fbe_u32_t));

    status = fbe_flushRegistry();

    return (status);
}


/**************************************************************************
 *          fbe_module_mgmt_get_slics_marked_for_upgrade
 * ************************************************************************
 *  @brief
 *  Called to get the registry entry to check if the SLICs on this SP 
 *  are marked for upgrade.
 *
 *  @param marked_for_upgrade - whether or not slics are marked for upgrade
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  27-Nov-08: Nayana Chaudhari - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_slics_marked_for_upgrade (fbe_module_mgmt_upgrade_type_t *marked_for_upgrade)
{
    fbe_status_t status;
    fbe_u32_t DefaultInputValue;

    DefaultInputValue = 0;


    status = fbe_registry_read(NULL,
                              ESP_REG_PATH,
                              FBE_MODULE_MGMT_SLIC_UPGRADE_KEY,
                              marked_for_upgrade,
                              sizeof(fbe_bool_t),
                              FBE_REGISTRY_VALUE_DWORD,
                              &DefaultInputValue,
                              sizeof(fbe_bool_t),
                              TRUE);

    return (status);
}



/**************************************************************************
 *          fbe_module_mgmt_get_conversion_type
 * ************************************************************************
 *  @brief
 *  Called to get the registry entry to check if the SLICs on this SP 
 *  are marked for in-family conversion.
 *
 *  @param conversion - in family conversion type
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  03-March-11: bphilbin - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_conversion_type(fbe_module_mgmt_conversion_type_t *conversion_type)
{
    fbe_status_t status;
    fbe_u32_t DefaultInputValue;

    DefaultInputValue = 0;


    status = fbe_registry_read(NULL,
                              ESP_REG_PATH,
                              FBE_MODULE_MGMT_IN_FAMILY_CONVERSION_KEY,
                              conversion_type,
                              sizeof(fbe_u32_t),
                              FBE_REGISTRY_VALUE_DWORD,
                              &DefaultInputValue,
                              sizeof(fbe_u32_t),
                              TRUE);

    return (status);
}

/* ***********************************************************************
 *          fbe_module_mgmt_get_miniport_instance_count
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to get the instance count from the Enum Key inside the specified
 *  miniport's Registry key.
 *
 *  PARAMETERS:
 *  fbe_char_t *miniport_name - miniport driver key name in the Registry.
 *  fbe_u32_t *instance_count - return value of Count found in miniport/Enum/Count
 *                            if none is found, 0 is the default.
 *
 *  RETURN VALUES: 
 *     none

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_miniport_instance_count(fbe_char_t *miniport_name, fbe_u32_t *instance_count)
{
    fbe_status_t    status;
    fbe_u32_t default_regvalue = 0;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%s\\%s\\Enum", SERVICES_REG_PATH, miniport_name);

    status = fbe_registry_read(NULL,
                              reg_path,
                              "Count",
                              instance_count,
                              sizeof(fbe_u32_t),
                              FBE_REGISTRY_VALUE_DWORD,
                              &default_regvalue,
                              sizeof(fbe_u32_t),
                              FALSE);
    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_get_miniport_instance_string
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to get the instance string from the Enum Key inside the specified
 *  miniport's Registry key.
 *
 *  PARAMETERS:
 *  fbe_char_t *miniport_name - miniport driver key name in the Registry.
 *  fbe_u32_t *instance_number - which instance
 *  fbe_char_t *instance_string - return value, this is the instance string found in
 *                          the registry for the specified instance number.
 *
 *  RETURN VALUES: 
 *     none

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_miniport_instance_string(fbe_char_t *miniport_name, fbe_u32_t *instance_number, fbe_char_t *instance_string)
{
    fbe_status_t    status;
    fbe_u32_t default_regvalue = 0;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t instance_name[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_set_memory(instance_name, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%s\\%s\\Enum", SERVICES_REG_PATH, miniport_name);
    fbe_sprintf(instance_name, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%d", *instance_number);

    status = fbe_registry_read(NULL,
                              reg_path,
                              instance_name,
                              instance_string,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FBE_REGISTRY_VALUE_SZ,
                              &default_regvalue,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FALSE);
    return status;
}



/* ***********************************************************************
 *          fbe_module_mgmt_get_configured_interrupt_affinity_string
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to get the configured interrupt affinity string from the FlareDrv
 *  registry key.
 *
 *  PARAMETERS:
 *  fbe_u32_t *instance_number - which instance
 *  fbe_char_t *instance_string - return value, this is the instance string found in
 *                          the registry for the specified instance number.
 *
 *  RETURN VALUES: 
 *     none

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_configured_interrupt_affinity_string(fbe_u32_t *instance_number, fbe_char_t *instance_string)
{
    fbe_status_t    status;
    fbe_u32_t default_regvalue = 0;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t instance_name[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_set_memory(instance_name, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    
    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%s\\InterruptAffinity", ESP_REG_PATH);
    fbe_sprintf(instance_name,FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%d", *instance_number);

    status = fbe_registry_read(NULL,
                              reg_path,
                              instance_name,
                              instance_string,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FBE_REGISTRY_VALUE_SZ,
                              &default_regvalue,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FALSE);
    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_set_configured_interrupt_affinity_string
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to set the configured interrupt affinity string from the FlareDrv
 *  registry key.
 *
 *  PARAMETERS:
 *  fbe_u32_t *instance_number - which instance
 *  fbe_char_t *instance_string - this is the instance string to write to
 *                          the registry for the specified instance number.
 *
 *  RETURN VALUES: 
 *     fbe_status_t

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_configured_interrupt_affinity_string(fbe_u32_t *instance_number, fbe_char_t *instance_string)
{
    fbe_status_t    status;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t instance_name[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_set_memory(instance_name, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));

    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%s\\InterruptAffinity", ESP_REG_PATH);
    fbe_sprintf(instance_name, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%d", *instance_number);

    status = fbe_registry_write(NULL,
                               reg_path,
                               instance_name, 
                               FBE_REGISTRY_VALUE_SZ,
                               (void *)instance_string, 
                               (fbe_u32_t)strlen(instance_string));
    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_clear_configured_interrupt_affinity_string
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to delete the configured interrupt affinity string from the FlareDrv
 *  registry key.
 *
 *  PARAMETERS:
 *  fbe_u32_t *instance_number - which instance
 *
 *  RETURN VALUES: 
 *     fbe_status_t

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_clear_configured_interrupt_affinity_string(fbe_u32_t *instance_number)
{
    fbe_status_t    status;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t instance_name[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_set_memory(instance_name, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));

    fbe_sprintf(instance_name, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%d", *instance_number);

    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "%s\\InterruptAffinity", ESP_REG_PATH);

    status = fbe_registry_delete_key(reg_path, instance_name);
    
    return status;
}



/* ***********************************************************************
 *          fbe_module_mgmt_get_pci_location_from_instance
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to get the PCI string saved in the enumerated devices section of the
 *  Registry for the specified device/instance id.
 *
 *  PARAMETERS:
 *  fbe_char_t *device_id - String containing Vedor, Device, Subsystem, and Revision IDs
 *  fbe_char_t *instance_id - Instance ID
 *  fbe_char_t *pci_string - return value, this string contains information about the pci
 *                     address for the specified device.
 *
 *  RETURN VALUES: 
 *     none

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
VOID fbe_module_mgmt_get_pci_location_from_instance(fbe_char_t *device_id, fbe_char_t *instance_id, fbe_char_t *pci_string)
{
    fbe_status_t    status;
    fbe_u32_t default_regvalue = 0;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                "\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\PCI\\%s\\%s", 
                device_id, instance_id);

    status = fbe_registry_read(NULL,
                              reg_path,
                              FBE_PCI_REG_LOCATION_INFORMATION,
                              (void *)pci_string,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FBE_REGISTRY_VALUE_SZ,
                              &default_regvalue,
                              FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                              FALSE);
    return;
}


/* ***********************************************************************
 *          fbe_module_mgmt_set_processor_affinity
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to set the processor affinity for the miniports
 *
 *  PARAMETERS:
 *  fbe_char_t *device_id - String containing Vedor, Device, Subsystem, and Revision IDs
 *  fbe_char_t *instance_id - Instance ID
 *  fbe_u32_t core_num - Core Number
 *
 *  RETURN VALUES: 
 *     fbe_status_t

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_processor_affinity(fbe_char_t *device_id, fbe_char_t *instance_id, fbe_u32_t core_num)
{
    fbe_status_t    status;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_u32_t device_policy = 4;
    fbe_u32_t core_num_bmask = 1;

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                  "\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\PCI\\%s\\%s\\Device Parameters\\Interrupt Management\\Affinity Policy", 
                  device_id, instance_id);

    core_num_bmask = core_num_bmask << core_num;

    status = fbe_registry_write(NULL,
                               reg_path,
                               FBE_PROCESSOR_AFFINITY_KEY,
                               FBE_REGISTRY_VALUE_BINARY,
                               (void *)&core_num_bmask,
                               (fbe_u32_t)sizeof(fbe_u32_t));

    if (status == FBE_STATUS_OK)
    {
        status = fbe_registry_write(NULL,
                                    reg_path,
                                    "DevicePolicy",
                                    FBE_REGISTRY_VALUE_DWORD,
                                    (void *)&device_policy,
                                    (fbe_u32_t)sizeof(fbe_u32_t));
    }

    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_get_processor_affinity
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to get the processor affinity for the miniports
 *
 *  PARAMETERS:
 *  fbe_module_mgmt_t *module_mgmt - Object Module mgmt
 *  fbe_char_t *device_id - String containing Vedor, Device, Subsystem, and Revision IDs
 *  fbe_char_t *instance_id - Instance ID
 *  fbe_u32_t core_num - Core Number
 *
 *  RETURN VALUES:
 *     fbe_status_t

 *
 *  NOTES:
 *
 * HISTORY:
 *  Mar-17-2014 - Created. Dongz
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_processor_affinity(fbe_module_mgmt_t *module_mgmt, fbe_char_t *device_id, fbe_char_t *instance_id, fbe_u32_t *core_num)
{
    fbe_status_t    status;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_u32_t device_policy = 0;
    fbe_u32_t core_num_bmask = 0;

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                  "\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\PCI\\%s\\%s\\Device Parameters\\Interrupt Management\\Affinity Policy",
                  device_id, instance_id);

    *core_num = FBE_MODULE_MGMT_INVALID_CORE_NUM;

    status = fbe_registry_read(NULL,
                              reg_path,
                              "DevicePolicy",
                              (void *) &device_policy,
                              (fbe_u32_t)sizeof(fbe_u32_t),
                              FBE_REGISTRY_VALUE_DWORD,
                              &device_policy,
                              (fbe_u32_t)sizeof(fbe_u32_t),
                              FALSE);

    if(status != FBE_STATUS_OK || device_policy != 4)
    {
    	return status;
    }

    status = fbe_registry_read(NULL,
                              reg_path,
                              FBE_PROCESSOR_AFFINITY_KEY,
                              (void *) &core_num_bmask,
                              (fbe_u32_t)sizeof(fbe_u32_t),
                              FBE_REGISTRY_VALUE_BINARY,
                              &core_num_bmask,
                              (fbe_u32_t)sizeof(fbe_u32_t),
                              FALSE);


    if (status == FBE_STATUS_OK)
    {
    	*core_num = 0;

    	while(core_num_bmask > 1)
    	{
    		core_num_bmask >>= 1;
    		(*core_num)++;
    	}
    }

    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_set_msi_message_limit
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to set the MSI message limit for the specified device
 *
 *  PARAMETERS:
 *  fbe_char_t *device_id - String containing Vedor, Device, Subsystem, and Revision IDs
 *  fbe_char_t *instance_id - Instance ID
 *  fbe_u32_t limit - MSI message limit
 *
 *  RETURN VALUES: 
 *     fbe_status_t

 *
 *  NOTES:
 *
 * HISTORY:
 *  Jun-14-2010 - Created. Brion Philbin
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_msi_message_limit(fbe_char_t *device_id, fbe_char_t *instance_id, fbe_u32_t limit)
{
    fbe_status_t    status;
    fbe_char_t reg_path[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    fbe_set_memory(reg_path, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
    fbe_sprintf(reg_path, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH,
                  "\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\PCI\\%s\\%s\\Device Parameters\\Interrupt Management\\MessageSignaledInterruptProperties", 
                  device_id, instance_id);

    status = fbe_registry_write(NULL,
                               reg_path,
                               FBE_MSI_MESSAGE_NUMBER_LIMIT_KEY,
                               FBE_REGISTRY_VALUE_DWORD,
                               (void *)&limit,
                               (fbe_u32_t)sizeof(fbe_u32_t));


    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_get_enable_port_interrupt_affinity
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to get the registry value for enabling interrupt affinity setting
 *  for FE ports.
 *
 *  PARAMETERS:
 *  fbe_u32_t *rebuild_avoidance_disable: actual value read from the registry
 *
 *  RETURN VALUES: 
 *          fbe_status_t - returns the status whether the registry has been updated
 *                     successfully or not
 *
 *  NOTES:
 *
 * HISTORY:
 *  Aug-11-2010 - Created. bphilbin
 *
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_enable_port_interrupt_affinity(fbe_u32_t *port_interrupt_affinity_enabled)
{
    fbe_status_t    status;
    fbe_u32_t default_regvalue = FBE_MODULE_MGMT_AFFINITY_SETTING_MODE_DYNAMIC;

    status = fbe_registry_read(NULL,
                              ESP_REG_PATH,
                              FBE_MODULE_MGMT_ENABLE_PORT_INTERRUPT_AFFINITY,
                              port_interrupt_affinity_enabled,
                              sizeof(fbe_u32_t),
                              FBE_REGISTRY_VALUE_DWORD,
                              &default_regvalue,
                              sizeof(fbe_u32_t),
                              TRUE);
    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_set_enable_port_interrupt_affinity
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to set the registry value for enabling interrupt affinity setting
 *  for FE ports.
 *
 *  PARAMETERS:
 *  fbe_u32_t *port_interrupt_affinity_enabled: value to write to the registry
 *
 *  RETURN VALUES: 
 *          fbe_status_t - returns the status whether the registry has been updated
 *                     successfully or not
 *
 *  NOTES:
 *
 * HISTORY:
 *  Aug-11-2010 - Created. bphilbin
 *
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_enable_port_interrupt_affinity(fbe_u32_t port_interrupt_affinity_enabled)
{
    fbe_status_t    status;

    status = fbe_registry_write(NULL,
                               ESP_REG_PATH,
                               FBE_MODULE_MGMT_ENABLE_PORT_INTERRUPT_AFFINITY,
                               FBE_REGISTRY_VALUE_DWORD,
                               (void *)&port_interrupt_affinity_enabled,
                                (fbe_u32_t)sizeof(fbe_u32_t));
    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_get_reset_port_interrupt_affinity
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to get the registry value for enabling interrupt affinity setting
 *  for FE ports.
 *
 *  PARAMETERS:
 *  fbe_u32_t *reset_port_interrupt_affinity: actual value read from the registry
 *
 *  RETURN VALUES: 
 *          fbe_status_t - returns the status whether the registry has been updated
 *                     successfully or not
 *
 *  NOTES:
 *
 * HISTORY:
 *  Aug-11-2010 - Created. bphilbin
 *
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_reset_port_interrupt_affinity(fbe_u32_t *reset_port_interrupt_affinity)
{
    fbe_status_t    status;
    fbe_u32_t       default_regvalue = 1;

    status = fbe_registry_read(NULL,
                              ESP_REG_PATH,
                              FBE_MODULE_MGMT_RESET_PORT_INTERRUPT_AFFINITY,
                              reset_port_interrupt_affinity,
                              sizeof(fbe_u32_t),
                              FBE_REGISTRY_VALUE_DWORD,
                              &default_regvalue,
                              sizeof(fbe_u32_t),
                              TRUE);
    return status;
}

/* ***********************************************************************
 *          fbe_module_mgmt_set_reset_port_interrupt_affinity
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to set the registry value for enabling interrupt affinity setting
 *  for FE ports.
 *
 *  PARAMETERS:
 *  fbe_u32_t reset_port_interrupt_affinity:  value written to the registry
 *
 *  RETURN VALUES: 
 *          fbe_status_t - returns the status whether the registry has been updated
 *                     successfully or not
 *
 *  NOTES:
 *
 * HISTORY:
 *  Aug-11-2010 - Created. bphilbin
 *
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_reset_port_interrupt_affinity(fbe_u32_t reset_port_interrupt_affinity)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                               ESP_REG_PATH,
                               FBE_MODULE_MGMT_RESET_PORT_INTERRUPT_AFFINITY, 
                               FBE_REGISTRY_VALUE_DWORD,
                               &reset_port_interrupt_affinity, 
                               (fbe_u32_t)sizeof(fbe_u32_t));
    return status;
}

/**************************************************************************
 *          fbe_module_mgmt_get_disable_reg_update_info
 * ************************************************************************
 *  @brief
 *  Called to set the persis port info registry key.
 *
 *  @param module_mgmt - object context
 *  @param persist_port_info - whether or not to persist the port information
 *
 *  @return fbe_status_t - status
 *
 *  History:
 *  19-Mar-07: Brion Philbin - Created
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_get_disable_reg_update_info(fbe_module_mgmt_t *module_mgmt, fbe_bool_t *disable_reg_update_info)
{
    fbe_status_t status;
    fbe_u32_t DefaultInputValue;

    DefaultInputValue = 0;

    status = fbe_registry_read(NULL,
                               ESP_REG_PATH,
                               FBE_MODULE_MGMT_DISABLE_REG_UPDATE_KEY,
                               disable_reg_update_info,
                               sizeof(fbe_bool_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &DefaultInputValue,
                               sizeof(fbe_bool_t),
                               TRUE);

    /*
     * This data is critical and should always be accessible.  Trace
     * an error if this fails.
     */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get registry entry status:0x%x\n",
                              __FUNCTION__, status);
    }

    return (status);
}

/* ***********************************************************************
 *          fbe_module_mgmt_set_reboot_required
 * ************************************************************************
 *  DESCRIPTION:
 *  Called to set the registry value to notify newSP that a reboot is needed.
 *  NewSP will initiate a clean shutdown of the SP.
 *
 *  PARAMETERS:
 *  n/a
 *
 *  RETURN VALUES: 
 *          fbe_status_t - returns the status whether the registry has been updated
 *                     successfully or not
 *
 *  NOTES:
 *
 * HISTORY:
 *  Jan-11-2013 - Created. bphilbin
 *
 *
 * ***********************************************************************/
fbe_status_t fbe_module_mgmt_set_reboot_required(void)
{
    fbe_status_t status;
    fbe_bool_t reboot_required = FBE_TRUE;

    status = fbe_registry_write(NULL,
                               NEW_SP_REG_PATH,
                               FBE_MODULE_MGMT_REBOOT_REQUIRED, 
                               FBE_REGISTRY_VALUE_DWORD,
                               &reboot_required, 
                               (fbe_u32_t)sizeof(fbe_bool_t));
    return status;
}
