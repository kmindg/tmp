/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the module
 *  Management object. This includes the create and
 *  destroy methods for the module management object, as well
 *  as the event entry point, load and unload methods.
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   11-Mar-2010:  Created. Nayana Chaudhari
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_transport_memory.h"
#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"

/* Class methods forward declaration */
fbe_status_t fbe_module_mgmt_load(void);
fbe_status_t fbe_module_mgmt_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_module_mgmt_class_methods = { FBE_CLASS_ID_MODULE_MGMT,
                                                    fbe_module_mgmt_load,
                                                    fbe_module_mgmt_unload,
                                                    fbe_module_mgmt_create_object,
                                                    fbe_module_mgmt_destroy_object,
                                                    fbe_module_mgmt_control_entry,
                                                    fbe_module_mgmt_event_entry,
                                                    NULL,
                                                    fbe_module_mgmt_monitor_entry
                                                    };
/*!***************************************************************
 * fbe_module_mgmt_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_MODULE_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_module_mgmt_t) < FBE_MEMORY_CHUNK_SIZE);
    return fbe_module_mgmt_monitor_load_verify();
}
/******************************************
 * end fbe_module_mgmt_load()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_unload()
 ****************************************************************
 * @brief
 *  This function is called to destruct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_MODULE_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_module_mgmt_unload()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_create_object()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t 
fbe_module_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_module_mgmt_t * module_mgmt;

    fbe_topology_class_trace(FBE_CLASS_ID_MODULE_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    /* Call parent constructor */
    status = fbe_base_environment_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    module_mgmt = (fbe_module_mgmt_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) module_mgmt, FBE_CLASS_ID_MODULE_MGMT);
    fbe_module_mgmt_init(module_mgmt);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_module_mgmt_create_object()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_destroy_object()
 ****************************************************************
 * @brief
 *  This function is called to destroy any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t 
fbe_module_mgmt_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_module_mgmt_t * module_mgmt;

    module_mgmt = (fbe_module_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)module_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* TODO - Deregister FBE API notifications */

    /* Free up any memory that was allocated for this object */

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, module_mgmt->io_module_info);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, module_mgmt->io_port_info);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, module_mgmt->mgmt_module_info);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, module_mgmt->iom_port_reg_info);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, module_mgmt->configured_interrupt_data);
    /* Call parent destructor */
    status = fbe_base_environment_destroy_object(object_handle);
    return status;
}
/******************************************
 * end fbe_module_mgmt_destroy_object()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_event_entry()
 ****************************************************************
 * @brief
 *  Needs to be updated with good description
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t 
fbe_module_mgmt_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_module_mgmt_event_entry()
 ******************************************/

/*!***************************************************************
 * fbe_module_mgmt_init()
 ****************************************************************
 * @brief
 *   Needs to be updated with good description
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t 
fbe_module_mgmt_init(fbe_module_mgmt_t * module_mgmt)
{
    fbe_u32_t port_num, iom_num, mgmt_id;
    SP_ID sp;
    fbe_status_t status;
    fbe_u32_t persistent_storage_data_size = 0;

    fbe_topology_class_trace(FBE_CLASS_ID_MODULE_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    module_mgmt->boot_device_found = FBE_FALSE;
    module_mgmt->configuration_change_made = FBE_FALSE;
    module_mgmt->port_configuration_loaded = FBE_FALSE;
    
    module_mgmt->io_module_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_io_module_info_t) * FBE_ESP_IO_MODULE_MAX_COUNT);
    fbe_set_memory(&module_mgmt->io_module_info[0], 0, (sizeof(fbe_io_module_info_t) * FBE_ESP_IO_MODULE_MAX_COUNT));

    module_mgmt->io_port_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_io_port_info_t) * FBE_ESP_IO_PORT_MAX_COUNT);
    fbe_set_memory(&module_mgmt->io_port_info[0], 0, (sizeof(fbe_io_port_info_t) * FBE_ESP_IO_PORT_MAX_COUNT));

    module_mgmt->mgmt_module_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_mgmt_module_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP);
    fbe_set_memory(&module_mgmt->mgmt_module_info[0], 0, (sizeof(fbe_mgmt_module_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP));

    module_mgmt->iom_port_reg_info = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_module_mgmt_port_reg_info_t) * MAX_PORT_REG_PARAMS);
    fbe_set_memory(module_mgmt->iom_port_reg_info, 0, (sizeof(fbe_module_mgmt_port_reg_info_t) * MAX_PORT_REG_PARAMS));

    module_mgmt->configured_interrupt_data = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_module_mgmt_configured_interrupt_data_t) * FBE_ESP_IO_PORT_MAX_COUNT);
    fbe_set_memory(module_mgmt->configured_interrupt_data, 0, (sizeof(fbe_module_mgmt_configured_interrupt_data_t) * FBE_ESP_IO_PORT_MAX_COUNT));

    module_mgmt->total_module_count = 0;

    /* Initialized the mgmt port persistent info */
    for(mgmt_id = 0; mgmt_id < FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP; mgmt_id++)
    {
        module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_UNSPECIFIED;
        module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortSpeed = FBE_MGMT_PORT_SPEED_UNSPECIFIED;
        module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_UNSPECIFIED;

        module_mgmt->mgmt_module_info[mgmt_id].user_requested_mgmt_port_config.mgmtPortAutoNeg = FBE_PORT_AUTO_NEG_UNSPECIFIED;
        module_mgmt->mgmt_module_info[mgmt_id].user_requested_mgmt_port_config.mgmtPortSpeed = FBE_MGMT_PORT_SPEED_UNSPECIFIED;
        module_mgmt->mgmt_module_info[mgmt_id].user_requested_mgmt_port_config.mgmtPortDuplex = FBE_PORT_DUPLEX_MODE_UNSPECIFIED;
        /* Init mgmt port config */
        fbe_module_mgmt_init_mgmt_port_config_op(&(module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op));
    }

    /*
     * Set defaults for the io module logical information.
     */
    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        for(sp = SP_A; sp < SP_ID_MAX; sp++)
        {
            module_mgmt->io_module_info[iom_num].logical_info[sp].module_state = MODULE_STATE_EMPTY;
            module_mgmt->io_module_info[iom_num].logical_info[sp].module_substate = MODULE_SUBSTATE_NOT_PRESENT;
            module_mgmt->io_module_info[iom_num].logical_info[sp].slic_type = FBE_SLIC_TYPE_UNKNOWN;
            module_mgmt->io_module_info[iom_num].physical_info[sp].protocol = FBE_IO_MODULE_PROTOCOL_UNKNOWN;
        }
    }
    /*
     * Set defaults for the port logical information.
     */
    for(port_num = 0; port_num < FBE_ESP_IO_PORT_MAX_COUNT; port_num++)
    {
        /* Initialize the enumerated port numbers to invalid */
        module_mgmt->io_port_info[port_num].port_object_id = FBE_OBJECT_ID_INVALID;
        for(sp = SP_A; sp < SP_ID_MAX; sp++)
        {
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.logical_num = INVALID_PORT_U8;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.port_role = IOPORT_PORT_ROLE_UNINITIALIZED;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.port_subrole = FBE_PORT_SUBROLE_UNINTIALIZED;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_state = FBE_PORT_STATE_UNINITIALIZED;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_substate = FBE_PORT_SUBSTATE_UNINITIALIZED;
            
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.iom_group = FBE_IOM_GROUP_UNKNOWN;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.port_location.io_enclosure_number = INVALID_ENCLOSURE_NUMBER;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.port_location.type_32_bit = FBE_DEVICE_TYPE_INVALID;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.port_location.slot = INVALID_MODULE_NUMBER;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].port_configured_info.port_location.port = INVALID_PORT_U8;

            module_mgmt->io_port_info[port_num].port_physical_info.port_comp_info[sp].deviceType = FBE_DEVICE_TYPE_INVALID;

            module_mgmt->io_port_info[port_num].port_logical_info[sp].combined_port = FBE_FALSE;
            module_mgmt->io_port_info[port_num].port_logical_info[sp].secondary_combined_port = INVALID_PORT_U8;
        }
    }
    module_mgmt->reboot_sp = REBOOT_NONE;
    

    /*
     * Zero out the limits information.
     */

    fbe_set_memory(&module_mgmt->platform_hw_limit, 0, sizeof(fbe_environment_limits_platform_hardware_limits_t));
    fbe_set_memory(&module_mgmt->discovered_hw_limit, 0, sizeof(fbe_esp_module_mgmt_discovered_hardware_limits_t));
    fbe_set_memory(&module_mgmt->platform_port_limit, 0, sizeof(fbe_environment_limits_platform_port_limits_t));

    fbe_environment_limit_get_platform_hardware_limits(&module_mgmt->platform_hw_limit);

    module_mgmt->discovering_hardware = FALSE;

    if(module_mgmt->base_environment.persist_db_disabled)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Persist DB disabled. Always read ports configuration from registry.\n",
                                  __FUNCTION__);

        // If persist DB disabled for normal mode (that means SEP disabled in the system configuration)
        // then set port persist info flag in registry (it could be cleared at previous system boot).
        // We can get this situation during upgrade from DB enabled to DB disabled config.
        status = fbe_module_mgmt_set_persist_port_info(module_mgmt, TRUE);

        // Enable reading port configuration from registry because persist DB is not available
        module_mgmt->port_persist_enabled = TRUE;
    }
    else
    {
        // If persist DB enabled by system configuration
        // then set persist_port_info state based on corresponded registry flag
        status = fbe_module_mgmt_get_persist_port_info(module_mgmt, &module_mgmt->port_persist_enabled);
    }

    if((status == FBE_STATUS_OK) && (module_mgmt->port_persist_enabled))
    {
        status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_CONFIGURE_PORTS);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in setting CONFIGURE_PORTS condition, status:0x%x\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Port Persist detected in Registry setting CONFIGURE_PORTS_CONDITION \n",
                                  __FUNCTION__);
        }
    }

    status = fbe_module_mgmt_get_slics_marked_for_upgrade(&module_mgmt->slic_upgrade);

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s SLIC_UPGRADE set to %d\n",
                                  __FUNCTION__, module_mgmt->slic_upgrade);

    if((status == FBE_STATUS_OK) && (module_mgmt->slic_upgrade == FBE_MODULE_MGMT_SLIC_UPGRADE_LOCAL_SP))
    {
        status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_SLIC_UPGRADE);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in setting SLIC_UPGRADE condition, status:0x%x\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s SLIC Upgrade detected in Registry setting SLIC_UPGRADE_CONDITION \n",
                                  __FUNCTION__);
        }
    }
    status = fbe_module_mgmt_get_conversion_type(&module_mgmt->conversion_type);
    if((status == FBE_STATUS_OK) && (module_mgmt->conversion_type))
    {
        status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_IN_FAMILY_PORT_CONVERSION);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in setting CONVERSION condition, status:0x%x\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s conversion type %d from Registry setting CONVERSION condition, \n",
                                  __FUNCTION__, module_mgmt->conversion_type);
        }
    }

    persistent_storage_data_size = (FBE_MODULE_MGMT_PERSISTENT_DATA_STORE_SIZE);

    FBE_ASSERT_AT_COMPILE_TIME(FBE_MODULE_MGMT_PERSISTENT_DATA_STORE_SIZE < FBE_PERSIST_DATA_BYTES_PER_ENTRY);
    fbe_base_environment_set_persistent_storage_data_size((fbe_base_environment_t *) module_mgmt, persistent_storage_data_size);

    /* Set the condition to initialize persistent storage and read the data from disk. */
    status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_BASE_ENV_LIFECYCLE_COND_INITIALIZE_PERSISTENT_STORAGE);


    module_mgmt->port_affinity_mode = fbe_module_mgmt_port_affinity_setting();
    return status;
}
/****************************************
* end of fbe_module_mgmt_init()
****************************************/
 /*!*************************************************************************
 *  fbe_module_mgmt_init_mgmt_port_config_op()
 ***************************************************************************
 * @brief
 *  This function initialized the mgmt_port_config_info used for the management port configuration.
 *
 * @param  mgmt_port_config_op - The pointer to the fbe_mgmt_port_config_op_t.
 *
 * @return - void
 * 
 * @author
 *  24-Mar-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void 
fbe_module_mgmt_init_mgmt_port_config_op(fbe_mgmt_port_config_op_t  *mgmt_port_config_op)
{
    mgmt_port_config_op->mgmtId = 0;
    mgmt_port_config_op->mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_UNSPECIFIED;
    mgmt_port_config_op->mgmtPortSpeedRequested = FBE_MGMT_PORT_SPEED_UNSPECIFIED;
    mgmt_port_config_op->mgmtPortDuplexModeRequested = FBE_PORT_DUPLEX_MODE_UNSPECIFIED;
    mgmt_port_config_op->mgmtPortConfigInProgress = FBE_FALSE;

    mgmt_port_config_op->configRequestTimeStamp = 0;
    mgmt_port_config_op->revertEnabled = FBE_TRUE;
    
    mgmt_port_config_op->revertMgmtPortConfig = FBE_FALSE;
    mgmt_port_config_op->previousMgmtPortAutoNeg = FBE_PORT_AUTO_NEG_UNSPECIFIED;
    mgmt_port_config_op->previousMgmtPortSpeed = FBE_MGMT_PORT_SPEED_UNSPECIFIED;
    mgmt_port_config_op->previousMgmtPortDuplexMode = FBE_PORT_DUPLEX_MODE_UNSPECIFIED;
    return;
}
/**********************************************
 * end fbe_module_mgmt_init_mgmt_port_config_op()
 *********************************************/
 /*!*************************************************************************
 *  fbe_module_mgmt_convert_mgmt_port_config_request()
 ***************************************************************************
 * @brief
 *  This function converts the management port config request to populate
 *  the mgmt port op fields. 
 *
 * @param module_mgmt - The pointer to the module_mgmt.
 * @param mgmt_id - Mgmt ID
 * @param reqSource - Request source
 * @param mgmt_port_config_info - Requested port speed info
 *
 * @return - fbe_status_t -
 * 
 * @author
 *  31-May-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t
fbe_module_mgmt_convert_mgmt_port_config_request(fbe_module_mgmt_t  *module_mgmt,
                                         fbe_u32_t  mgmt_id,
                                         fbe_mgmt_port_config_request_source_t reqSource,
                                         fbe_mgmt_port_config_info_t  *mgmt_port_config_request)
{
    SP_ID               localSpID;
    fbe_mgmt_port_status_t *mgmtPortInfo;
    fbe_mgmt_module_info_t *mgmt_module_info;

    if(mgmt_id >= module_mgmt->discovered_hw_limit.num_mgmt_modules)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Invalid MGMT module ID %d, num_mgmt_modules %d\n", 
                              __FUNCTION__, 
                              mgmt_id,
                              module_mgmt->discovered_hw_limit.num_mgmt_modules);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the local SPID*/
    localSpID = module_mgmt->base_environment.spid;
    
    /* Get the port to module_mgmt MGMT port info */
    mgmtPortInfo = &(module_mgmt->mgmt_module_info->mgmt_module_comp_info[localSpID].managementPort);    

    mgmt_module_info = &(module_mgmt->mgmt_module_info[mgmt_id]);
    

    if(mgmt_module_info->mgmt_port_config_op.mgmtPortConfigInProgress)
    {
        fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, MGMT Port config request is already in progress.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    mgmt_module_info->mgmt_port_config_op.mgmtId = mgmt_id;

    switch(mgmt_port_config_request->mgmtPortAutoNeg) 
    { 
        case FBE_PORT_AUTO_NEG_ON: 
              mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = mgmt_port_config_request->mgmtPortAutoNeg;
          break;
          
        case FBE_PORT_AUTO_NEG_OFF: 
            if(mgmt_module_info->mgmt_port_config_op.mgmtPortSpeedRequested == FBE_MGMT_PORT_SPEED_1000MBPS &&
               reqSource == FBE_REQUEST_SOURCE_USER)
            {
                fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, %s turn ON auto-negotiate mode for the requested speed 1000Mbps\n\n",
                                      __FUNCTION__, 
                                      ((module_mgmt->base_environment.spid== SP_A)? "SPA management module" : "SPB management module"));
           
                fbe_event_log_write(ESP_INFO_MGMT_PORT_FORCE_AUTONEG_ON, NULL, 0, NULL);
                mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_ON;
            }
            else
            {
                mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = mgmt_port_config_request->mgmtPortAutoNeg;
            }
            break;

        case FBE_PORT_AUTO_NEG_UNSPECIFIED:
            if(reqSource == FBE_REQUEST_SOURCE_USER)
            {
                /* Use existing value if not specified */
                mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = mgmtPortInfo->mgmtPortAutoNeg;
            }else
            {
                mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_ON; 
            }
            break;             
            
          default: 
            if(reqSource == FBE_REQUEST_SOURCE_USER)
            {
                fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, invalid requested auto negotiate mode: 0x%x.\n",
                                      __FUNCTION__, 
                                      mgmt_port_config_request->mgmtPortAutoNeg);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                  mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_ON; 
            }
          break; 
    }

    /* mgmtPortSpeed */
    switch(mgmt_port_config_request->mgmtPortSpeed)
    {
            case FBE_MGMT_PORT_SPEED_10MBPS:
            case FBE_MGMT_PORT_SPEED_100MBPS:
            case FBE_MGMT_PORT_SPEED_1000MBPS:
                mgmt_module_info->mgmt_port_config_op.mgmtPortSpeedRequested = mgmt_port_config_request->mgmtPortSpeed;
            break;

            case FBE_MGMT_PORT_SPEED_UNSPECIFIED:
                if(reqSource == FBE_REQUEST_SOURCE_USER)
                {
                    mgmt_module_info->mgmt_port_config_op.mgmtPortSpeedRequested = mgmtPortInfo->mgmtPortSpeed;
                }
                else
                {
                    /* Make the AutoNet ON for not valid value */
                     mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_ON;
                }
            break;

            default:
                if(reqSource == FBE_REQUEST_SOURCE_USER)
                {
                    fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, invalid requested port speed: 0x%x.\n",
                                          __FUNCTION__, 
                                          mgmt_port_config_request->mgmtPortSpeed);

                    return FBE_STATUS_GENERIC_FAILURE;
                }
                else
                {
                   /* For invalid value make the AutoNeg request ON */
                    mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_ON;
                }
            break;
    } 

    /* Port Duplex mode */
    switch(mgmt_port_config_request->mgmtPortDuplex)
    {
        case FBE_PORT_DUPLEX_MODE_HALF:
        case FBE_PORT_DUPLEX_MODE_FULL:
            mgmt_module_info->mgmt_port_config_op.mgmtPortDuplexModeRequested = mgmt_port_config_request->mgmtPortDuplex;
        break;

        case FBE_PORT_DUPLEX_MODE_UNSPECIFIED:
            if(reqSource == FBE_REQUEST_SOURCE_USER)
            {
                mgmt_module_info->mgmt_port_config_op.mgmtPortDuplexModeRequested = mgmtPortInfo->mgmtPortDuplex;
            }else
            {
                mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_ON;
            }
            break;
             
        default:
            if(reqSource == FBE_REQUEST_SOURCE_USER)
            {
                fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s, invalid requested port duplex mode: 0x%x.\n",
                                       __FUNCTION__, 
                                      mgmt_port_config_request->mgmtPortDuplex);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
               /* For invalid value make the AutoNeg request ON */
               mgmt_module_info->mgmt_port_config_op.mgmtPortAutoNegRequested = FBE_PORT_AUTO_NEG_ON;
            }
        break;
    }

    /* Copy the current port config; if revert operation required then use it */
    mgmt_module_info->mgmt_port_config_op.previousMgmtPortAutoNeg = mgmtPortInfo->mgmtPortAutoNeg;
    mgmt_module_info->mgmt_port_config_op.previousMgmtPortSpeed = mgmtPortInfo->mgmtPortSpeed;
    mgmt_module_info->mgmt_port_config_op.previousMgmtPortDuplexMode = mgmtPortInfo->mgmtPortDuplex;

    /* Initialized port configuration */
    mgmt_module_info->mgmt_port_config_op.sendMgmtPortConfig = FBE_TRUE;

    return FBE_STATUS_OK;
}
/*********************************************************************
 *  end of fbe_module_mgmt_convert_mgmt_port_config_request()
 ********************************************************************/
 
