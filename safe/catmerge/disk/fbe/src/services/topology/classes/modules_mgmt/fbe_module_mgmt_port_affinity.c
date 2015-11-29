/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_port_affinity.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management functionality to affine interrupts
 *  for configured ports to processor cores.
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   29-Mar-2011 - Created. bphilbin - created from cm_flexport_reg.c
 *
 ***************************************************************************/

#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_board_interface.h"



/****************************************************** 
 ******    LOCAL FUNCTIONS                        *****
 ******************************************************/
static fbe_u32_t fbe_module_mgmt_read_configured_interrupt_affinity_list(fbe_module_mgmt_t *module_mgmt);
static void fbe_module_mgmt_clear_affinity_list(fbe_module_mgmt_t *module_mgmt, fbe_u32_t default_mode);
static fbe_u32_t fbe_module_mgmt_read_miniport_instances(fbe_module_mgmt_t *module_mgmt);
static void fbe_module_mgmt_check_instances_with_affinity_list(fbe_module_mgmt_t *module_mgmt,
                                                        fbe_u32_t instance_count, 
                                                        fbe_u32_t configured_count);
static void fbe_module_mgmt_update_configured_affinity_setting(fbe_module_mgmt_t *module_mgmt,
                                                        fbe_u32_t configured_count, 
                                                        fbe_bool_t default_mode);
static void fbe_module_mgmt_add_entries_to_affinity_list(fbe_module_mgmt_t *module_mgmt,
                                                  fbe_u32_t instance_count, 
                                                  fbe_u32_t *configured_count,
                                                  fbe_bool_t default_mode);
static fbe_bool_t fbe_module_mgmt_get_pci_addr_from_device_instance(fbe_module_mgmt_t *module_mgmt,
                                                             fbe_module_mgmt_miniport_instance_string_t *instance_string, 
                                                             SPECL_PCI_DATA *pci_info);
static fbe_bool_t fbe_module_mgmt_update_removed_affinity_entries(fbe_module_mgmt_t *module_mgmt,
                                                           fbe_u32_t configured_count);
static fbe_bool_t fbe_module_mgmt_update_processor_affinity_settings(fbe_module_mgmt_t *module_mgmt,
                                                              fbe_u32_t configured_count);
static fbe_u32_t fbe_module_mgmt_remove_unused_entries_in_affinity_list(fbe_module_mgmt_t *module_mgmt,
                                                                 fbe_u32_t configured_count);
static void fbe_module_mgmt_update_affinity_list_in_registry(fbe_module_mgmt_t *module_mgmt,
                                                      fbe_u32_t configured_count);
static void fbe_module_mgmt_check_and_set_msi_message_limit(fbe_module_mgmt_t *module_mgmt, 
                                                            SPECL_PCI_DATA *pci_info, 
                                                            fbe_u32_t interrupt_data_index);



/*
 * This is used to store the device ids and instances read in from the 
 * various miniport Registry entries.  The memory is allocated for this 
 * based upon the Counts within the various miniport Keys. 
 */
static fbe_module_mgmt_miniport_instance_string_t *local_instance_strings;

/*
 * These are the name strings for the various miniport driver entries in the Registry.
 */

static fbe_char_t *fbe_module_mgmt_local_miniport_registry_string[] = 
{
    "fcdmtl",
    "saspmc",
    "ebdrv",
    "b06bdrv",
    "fcoedmql",
    "fcdmql",
    "iscsiql"
};

#define MINIPORT_REG_MAX (sizeof(fbe_module_mgmt_local_miniport_registry_string)/sizeof(fbe_char_t *))

#define PLATFORM_CORE_COUNT_DEFAULT 2



/**************************************************************************
 *    fbe_module_mgmt_check_and_update_interrupt_affinities
 **************************************************************************
 *
 *  @brief
 *   This is the entry point to checking the interrupt affinity settings.
 *   This function checks the configured interrupt affinity settings for
 *   the miniports and updates them when necessary.
 *    
 *
 *    @param force_affinity_setting_update - force an update of the affinity settings
 *                                    in the Registry.
 *    @param mode - Use the default (highest) or a dynamic method of core assignment.
 *
 *  RETURN VALUES/ERRORS:
 *      TRUE or FALSE - registry changes were made to affinity settings,
 *                      a reboot is required.
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_bool_t fbe_module_mgmt_check_and_update_interrupt_affinities(fbe_module_mgmt_t *module_mgmt, 
                                                                 fbe_bool_t force_affinity_setting_update, 
                                                                 fbe_u32_t mode,
                                                                 fbe_bool_t clear_affinity_settings)
{
    fbe_u32_t instance_count = 0;
    fbe_u32_t configured_count = 0;
    fbe_bool_t changes_made = FALSE;
    fbe_bool_t entry_removed = FALSE;
    fbe_bool_t default_mode = FALSE;
    fbe_u32_t reset_port_interrupt_affinity;

    local_instance_strings = NULL;


    if(mode == FBE_MODULE_MGMT_AFFINITY_SETTING_MODE_DYNAMIC)
    {
        default_mode = FALSE;
    }
    else if(mode == FBE_MODULE_MGMT_AFFINITY_SETTING_MODE_DEFAULT)
    {
        default_mode = TRUE;
    }

    fbe_module_mgmt_get_reset_port_interrupt_affinity(&reset_port_interrupt_affinity);

    if( (reset_port_interrupt_affinity == 1) || clear_affinity_settings)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: resetting port interrupt affinity settings.\n");
        fbe_module_mgmt_clear_affinity_list(module_mgmt, FBE_MODULE_MGMT_AFFINITY_SETTING_MODE_DYNAMIC);
        default_mode = FALSE;
        fbe_module_mgmt_set_reset_port_interrupt_affinity(0);
    }
    else
    {
        //read in our saved config under the ESP Registry key
        configured_count = fbe_module_mgmt_read_configured_interrupt_affinity_list(module_mgmt);
    }

    //scan all miniport drivers' keys for the current hardware instance strings
    instance_count = fbe_module_mgmt_read_miniport_instances(module_mgmt);

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: update port interrupt affinities force:%s configured:%d instance:%d, default_mode %d\n",
                          (force_affinity_setting_update?"TRUE":"FALSE"), 
                          configured_count, instance_count, default_mode);

    //check if we have any new or removed hardware
    fbe_module_mgmt_check_instances_with_affinity_list(module_mgmt, instance_count, configured_count);

    //update our list with new hardware if we have any
    fbe_module_mgmt_add_entries_to_affinity_list(module_mgmt, instance_count, &configured_count, default_mode);

    //update the affinity setting in our list for removed hardware
    entry_removed = fbe_module_mgmt_update_removed_affinity_entries(module_mgmt, configured_count);

    //if requested by the caller or a configuration change has happened, go update the configured affinity settings
    if( (force_affinity_setting_update) ||
        (module_mgmt->configuration_change_made) )
    {
        fbe_module_mgmt_update_configured_affinity_setting(module_mgmt, configured_count, default_mode);
    }

    //update the processor affinity settings in the Registry
    changes_made = fbe_module_mgmt_update_processor_affinity_settings(module_mgmt, configured_count);
    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: update port interrupt affinities entry_removed:%s changes_made:%s\n", 
                          (entry_removed?"TRUE":"FALSE"), 
                          (changes_made?"TRUE":"FALSE"));

    //remove any entries for missing hardware from our list
    if(entry_removed)
    {
        configured_count = fbe_module_mgmt_remove_unused_entries_in_affinity_list(module_mgmt, configured_count);
    }

    if(changes_made || entry_removed)
    {
        fbe_module_mgmt_update_affinity_list_in_registry(module_mgmt, configured_count);
    }

    if(local_instance_strings != NULL) {
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, local_instance_strings);
    }

    return changes_made;
}

/**************************************************************************
 *    fbe_module_mgmt_read_configured_interrupt_affinity_list
 **************************************************************************
 *
 *  @brief
 *   This function reads in the configured affinity settings from the registry
 *   breaks it apart into its relevant pieces and saves them in the global list.
 *    
 *
 *  @param module_mgmt - object context
 *
 *  @return fbe_u32_t - number of instances discovered
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_u32_t fbe_module_mgmt_read_configured_interrupt_affinity_list(fbe_module_mgmt_t *module_mgmt)
{
    fbe_char_t affinity_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t temp_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_u32_t instance_number = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;

    /*
     * We will read through all the instance strings we have saved previously here. 
     * The format for the strings are defined as PCI Address\Core#\Device ID\InstanceID.
     */
    

    while(status == FBE_STATUS_OK) 
    {
        fbe_set_memory(affinity_string, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
        status = fbe_module_mgmt_get_configured_interrupt_affinity_string(&instance_number, affinity_string);
        /* 
         * Pull out the PCI information from the string, this is in the form of
         * [bus.slot.func] 
         */ 
        if(status == FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "ModMgmt: read_configured_interrupt_affinity_list string:%s\n",
                                  affinity_string);
            index = 0;

            if(!fbe_module_mgmt_reg_scan_string_for_string('[', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, affinity_string, &index, temp_string))
            {
                 fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number, __LINE__);
                return 0;
            }
            if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                     affinity_string, &index, 
                                                     &module_mgmt->configured_interrupt_data[instance_number].pci_data.bus))
            {
                  fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number,
                          __LINE__);
                return 0;
            }
            if(!fbe_module_mgmt_reg_scan_string_for_int('.', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                     affinity_string, &index, 
                                                     &module_mgmt->configured_interrupt_data[instance_number].pci_data.device))
            {
                 fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number,
                          __LINE__);
                return 0;
            }
            if(!fbe_module_mgmt_reg_scan_string_for_int(']', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                     affinity_string, &index, 
                                                     &module_mgmt->configured_interrupt_data[instance_number].pci_data.function))
            {
                  fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number,
                          __LINE__);
                return 0;
            }
    
            /*
             * Read to the first backslash delimiter.
             */
    
            if(!fbe_module_mgmt_reg_scan_string_for_string('\\', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    affinity_string, &index, temp_string))
            {
                 fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number,
                          __LINE__);
                return 0;
            }
    
            /*
             * The next character set should be the core assignment, a single integer.
             */
    
            if(!fbe_module_mgmt_reg_scan_string_for_int('\\', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                 affinity_string, &index, 
                                                 &module_mgmt->configured_interrupt_data[instance_number].processor_core))
            {
                  fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number,
                          __LINE__);
               return 0;
            }
            
            /*
             * The next character set will be the hardware identification string 
             * VendorID, Device ID, Subsystem ID and Revision. 
             */
    
            if(!fbe_module_mgmt_reg_scan_string_for_string('\\', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    affinity_string, &index, 
                                                    module_mgmt->configured_interrupt_data[instance_number].device_identity_string))
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number,
                          __LINE__);
                return 0;
            }
    
            /*
             * The next character set will be the instance ID.  This is pretty much 
             * the reason why all this information is retrieved from the Registry 
             * and not the hardware. It is a Windows thing and not gauranteed to be in 
             * any specific format. 
             */
    
            if(!fbe_module_mgmt_reg_scan_string_for_string(0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    affinity_string, &index, 
                                                    module_mgmt->configured_interrupt_data[instance_number].device_instance_string))
            {
                 fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_configured_interrupt_affinity detected a bad string for instance:%d line:%d", 
                         instance_number,
                          __LINE__);
                return 0;
            }
        } // end of - if SUCCESS
        instance_number++;
    }// end of - while success

    // decrement the instance number by 1 to get the instance count to return
    return (instance_number - 1);
}

/**************************************************************************
 *    fbe_module_mgmt_clear_affinity_list
 **************************************************************************
 *
 *  @brief
 *   This function deletes each entry in the configured interrupt affinity
 *   list.
 *    
 *
 *  @param module_mgmt - object context
 *  @param default_mode - which mode to use to generate affinity settings (default/dynamic)
 *
 *  RETURN VALUES/ERRORS:
 *      none
 *
 *  HISTORY:
 *      07-Oct-10 - bphilbin - created.
  **************************************************************************/
void fbe_module_mgmt_clear_affinity_list(fbe_module_mgmt_t *module_mgmt, fbe_u32_t default_mode)
{
    fbe_u32_t instance_num = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_char_t configured_affinity_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];


    while(status == FBE_STATUS_OK) 
    {
        // check if the entry exists, if it does then delete the entry
        status = fbe_module_mgmt_get_configured_interrupt_affinity_string(&instance_num, configured_affinity_string);
        if(status == FBE_STATUS_OK)
        {
          status = fbe_module_mgmt_clear_configured_interrupt_affinity_string(&instance_num);
          instance_num++;
        }
    }

    // reset the default affinity setting
    fbe_module_mgmt_set_enable_port_interrupt_affinity(default_mode);
    return;
}
/**************************************************************************
 *    fbe_module_mgmt_read_miniport_instances
 **************************************************************************
 *
 *  @brief
 *   This function gathers a list of device instances for all the supported
 *   miniport drivers.  The size of this list is based upon the count parameter
 *   in the in the Enum key for the miniports.  
 *    
 *
 *  @param module_mgmt - object context
 *
 *  @return fbe_u32_t - number of entries in the list
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_u32_t fbe_module_mgmt_read_miniport_instances(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t miniport_num, miniport_instance_num;
    fbe_u32_t index = 0;
    fbe_u32_t miniport_instance_count[MINIPORT_REG_MAX];
    fbe_u32_t overall_instance_count = 0;
    fbe_u32_t instance_index = 0;
    fbe_char_t temp_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t temp_buffer[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    // zero out the array
    fbe_set_memory(miniport_instance_count, 0, (sizeof(fbe_u32_t) * MINIPORT_REG_MAX));

    /* 
     * First get the instance count from each miniport so we know the size of
     * the list. This count is located in the various miniport registry entries 
     * under their Enum key. 
     */
    for(miniport_num = 0; miniport_num < MINIPORT_REG_MAX; miniport_num++) 
    {
       fbe_module_mgmt_get_miniport_instance_count(fbe_module_mgmt_local_miniport_registry_string[miniport_num],
                                &miniport_instance_count[miniport_num]);
       overall_instance_count += miniport_instance_count[miniport_num];
    }

    if(overall_instance_count > MAX_PORT_REG_PARAMS) 
    {
        /*
         * This is unexpected, we should never be able to have more ports than are 
         * configurable present in the Registry. 
         */
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                 "ModMgmt: read_miniport_instances Error, exceeded the max config instance_count: %d\n",
            overall_instance_count);
        return 0;
    }

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "ModMgmt: read_miniport_instances overall count %d\n", 
                          overall_instance_count);

    /*
     * Allocate the list of device instances based on the overall count we have just found.
     */
    local_instance_strings = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_module_mgmt_miniport_instance_string_t) * overall_instance_count);
    if(local_instance_strings == NULL) 
    {
        /*
         * We were not able to allocate the memory, we cannot go any further here.
         */
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                 "ModMgmt: read_miniport_instances Error, unable to allocate memory.\n");
        return 0;
    }
    fbe_set_memory(local_instance_strings, 0, (sizeof(fbe_module_mgmt_miniport_instance_string_t) * overall_instance_count));

    /*
     * Fill in the list with the device ID and instance ID based on the instance 
     * count from each miniport registry key. 
     */

    for(miniport_num = 0; miniport_num < MINIPORT_REG_MAX; miniport_num++) 
    {
        for(miniport_instance_num = 0; 
             miniport_instance_num < miniport_instance_count[miniport_num]; 
             miniport_instance_num++) 
        {
            /*
             * The device identification and instance ID are combined into 
             * one string here, we will read them in and then break them apart. 
             * The string is in the format of PCI\Device IDs\Instance IDs.  We 
             * will strip off the PCI and save the device and instance IDs separately. 
             */
            fbe_module_mgmt_get_miniport_instance_string(fbe_module_mgmt_local_miniport_registry_string[miniport_num], 
                                      &miniport_instance_num, temp_string);

            local_instance_strings[instance_index].has_match_in_registry_list = FALSE;

            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                   "ModMgmt: miniport:%d instance:%d string:%s\n",
                                  miniport_num, miniport_instance_num, temp_string);

            // set the string index to the start of the string
            index = 0;

            // Dump the "PCI\"
            if(!fbe_module_mgmt_reg_scan_string_for_string('\\', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    temp_string, &index, 
                                                    temp_buffer))
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO, 
                         "ModMgmt: read_miniport_instances detected a bad string for miniport:%d instance:%d line:%d",
                         miniport_num, 
                         miniport_instance_num,
                          __LINE__);
                instance_index++;
                continue;
            }

            if(!fbe_module_mgmt_reg_scan_string_for_string('\\', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    temp_string, &index, 
                                                    local_instance_strings[instance_index].device_identity_string))
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_miniport_instances detected a bad string for miniport:%d instance:%d line:%d",
                         miniport_num, 
                         miniport_instance_num,
                          __LINE__);
                instance_index++;
                continue;
            }
            if(!fbe_module_mgmt_reg_scan_string_for_string(0, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    temp_string, &index, 
                                                    local_instance_strings[instance_index].device_instance_string))
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: read_miniport_instances detected a bad string for miniport:%d instance:%d line:%d",
                         miniport_num, 
                         miniport_instance_num,
                          __LINE__);
                instance_index++;
                continue;
            }

            instance_index++;
        }// end of - for miniport instance number
    }// end of - for miniport number
    return overall_instance_count;
}


/**************************************************************************
 *    fbe_module_mgmt_check_instances_with_affinity_list
 **************************************************************************
 *
 * @brief
 *   This function checks the configured interrupt affinity settings for
 *   the miniports against the list of miniport instances we found in the
 *   Registry.  This is to maintain the list of PCI address, core #,
 *   device IDs and instance IDs in our Registry.
 *    
 *
 * @param module_mgmt - object context
 * @param instance_count - number of instances discovered
 * @param configured_count - number of entires in the list of configured
 *                           devices
 *
 * @return none
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
void fbe_module_mgmt_check_instances_with_affinity_list(fbe_module_mgmt_t *module_mgmt,
                                                        fbe_u32_t instance_count, 
                                                        fbe_u32_t configured_count)
{
    fbe_u32_t instance_index = 0;
    fbe_u32_t configured_index = 0;

    /*
     * Sanity test the arguments.
     */
    if((instance_count > 0) && (local_instance_strings == NULL)) 
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                               "ModMgmt: check_and_update_instances has configured_count:%d with NULL instance_strings.\n",
                              instance_count);
        /*
         * Force the count to 0 so that we do not use the data.  Perhaps in
         * the future we could clean out the Registry Key if this is detected.
         */
        instance_count = 0;
    }

    for(instance_index = 0; instance_index < instance_count; instance_index++) 
    {
        for(configured_index = 0; configured_index < configured_count; configured_index++) 
        {
            /*
             * The device instance found in the Miniport's Registry key match a something that 
             * has already been configured, set no need to update our list.
             */
            if((strncmp(local_instance_strings[instance_index].device_identity_string, 
                       module_mgmt->configured_interrupt_data[configured_index].device_identity_string, 
                       strlen(local_instance_strings[instance_index].device_identity_string)) == 0) &&
               (strncmp(local_instance_strings[instance_index].device_instance_string, 
                        module_mgmt->configured_interrupt_data[configured_index].device_instance_string, 
                        strlen(local_instance_strings[instance_index].device_instance_string)) == 0))
            {
               fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "ModMgmt: check_and_update_instances has found matching"
                                     " instance index %d with configured index %d\n",
                                     instance_index, configured_index);

                local_instance_strings[instance_index].has_match_in_registry_list = TRUE;
                module_mgmt->configured_interrupt_data[configured_index].present_in_system = TRUE;
                break;
            }
        }
    }

    return;
}

/**************************************************************************
 *    fbe_module_mgmt_update_configured_affinity_setting
 **************************************************************************
 *
 * @brief
 *   This function goes through the list of configured interrupt affinities
 *   and generates a new interrupt affinity if the hardware is present
 *   in the system.
 *    
 *
 * @param module_mgmt - object context
 * @param fbe_u32_t configured_count - number of entires in the list of configured
 *                                     devices
 * @param fbe_bool_t default_mode - which mode to use to generate affinity settings (default/dynamic)
 *
 * @return none
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
void fbe_module_mgmt_update_configured_affinity_setting(fbe_module_mgmt_t *module_mgmt,
                                                        fbe_u32_t configured_count, 
                                                        fbe_bool_t default_mode)
{
    fbe_u32_t configured_index;
    for(configured_index = 0; configured_index < configured_count; configured_index++)
    {
        if(module_mgmt->configured_interrupt_data[configured_index].present_in_system)
        {
            /*
             * Go update the processor core assignment and set the flag to update this 
             * setting in the Registry. 
             */
            module_mgmt->configured_interrupt_data[configured_index].processor_core = 
                fbe_module_mgmt_generate_affinity_setting(module_mgmt, default_mode,
                    &module_mgmt->configured_interrupt_data[configured_index].pci_data);

            module_mgmt->configured_interrupt_data[configured_index].modify_needed = TRUE;

        }
    }
    return;
}


/**************************************************************************
 *    fbe_module_mgmt_add_entries_to_affinity_list
 **************************************************************************
 *
 *  @brief
 *   This function adds any new instances to our list and sets the appropriate
 *   affinity for these.
 *    
 *
 * @param module_mgmt - object context
 * @param fbe_u32_t instance_count - number of device instances read from miniport
 *                             Registry Keys
 * @param fbe_u32_t configured_count - number of entires in the list of configured
 *                               devices
 * @param fbe_bool_t default_mode - mode to generate affinity setting (default vs. dynamic)
 *
 * @return none
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
void fbe_module_mgmt_add_entries_to_affinity_list(fbe_module_mgmt_t *module_mgmt,
                                                  fbe_u32_t instance_count, 
                                                  fbe_u32_t *configured_count,
                                                  fbe_bool_t default_mode)
{
    fbe_u32_t instance_index = 0;
    SPECL_PCI_DATA pci_info;
    fbe_u32_t affinity_setting = 0;

    for(instance_index = 0; instance_index < instance_count; instance_index++) 
    {
        if(local_instance_strings[instance_index].has_match_in_registry_list == FALSE)
        {
            // clear the pci information
            fbe_set_memory(&pci_info, 0, sizeof(SPECL_PCI_DATA));
            //grab pci information from the registry based on the instance info
            if(fbe_module_mgmt_get_pci_addr_from_device_instance(module_mgmt, &local_instance_strings[instance_index], 
                                                               &pci_info))
            {
                // based on this pci address lookuup flexport settings and generate an affinity setting
                affinity_setting = fbe_module_mgmt_generate_affinity_setting(module_mgmt, default_mode, &pci_info);
    
                //update the configured list with this new entry
                strncpy(module_mgmt->configured_interrupt_data[*configured_count].device_identity_string, 
                        local_instance_strings[instance_index].device_identity_string, 
                        strlen(local_instance_strings[instance_index].device_identity_string));
                strncpy(module_mgmt->configured_interrupt_data[*configured_count].device_instance_string, 
                        local_instance_strings[instance_index].device_instance_string, 
                        strlen(local_instance_strings[instance_index].device_instance_string));
                fbe_copy_memory(&module_mgmt->configured_interrupt_data[*configured_count].pci_data, 
                                &pci_info, sizeof(SPECL_PCI_DATA));
                module_mgmt->configured_interrupt_data[*configured_count].present_in_system = TRUE;
                module_mgmt->configured_interrupt_data[*configured_count].processor_core = affinity_setting;
                module_mgmt->configured_interrupt_data[*configured_count].modify_needed = TRUE;
                //fbe_module_mgmt_check_and_set_msi_message_limit(module_mgmt, &pci_info, *configured_count);
                (*configured_count)++;
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "ModMgmt: add_entries_to_affinity_list unable "
                                     "to locate instance:%d \n",
                                     instance_index);
            }
        }
    }
    return;
}



/**************************************************************************
 *    fbe_module_mgmt_get_pci_addr_from_device_instance
 **************************************************************************
 *
 *  @brief
 *   This function retrieves the PCI information the specified device instance
 *   from the Registry, then parses the result and saves it as a pci address.
 *    
 *
 * @param module_mgmt - object context
 * @param fbe_module_mgmt_miniport_instance_string_t *instance_string - this is the device ID and
 *                                                instance strings
 * @param SPECL_PCI_DATA *pci_info - return data, this is filled in with the retrieved
 *                         pci address
 *
 * @return fbe_bool_t - PCI address found.
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_bool_t fbe_module_mgmt_get_pci_addr_from_device_instance(fbe_module_mgmt_t *module_mgmt,
                                                             fbe_module_mgmt_miniport_instance_string_t *instance_string, 
                                                             SPECL_PCI_DATA *pci_info)
{
    fbe_char_t pci_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t temp_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_u32_t index = 0;

    fbe_set_memory(pci_string, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));

    fbe_module_mgmt_get_pci_location_from_instance(instance_string->device_identity_string, 
                               instance_string->device_instance_string, 
                               pci_string);

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: get_pci_addr_from_device_instance found:\n");
    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                          "device:%s\n", instance_string->device_identity_string);
    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "instance:%s\n", instance_string->device_instance_string);
    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                          "PCI:%s\n", pci_string);



    /*
     * Skip past the irrelevant location information from the string and 
     * get right to the PCI address in the form of (bus,slot,func) 
     */

    if(!fbe_module_mgmt_reg_scan_string_for_string('(', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    pci_string, &index, 
                                                    temp_string))
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "ModMgmt: get_pci_addr_from_device detected a bad string at line %d:\n",
                         __LINE__);
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_INFO,
                              "%s\n",pci_string);
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    pci_string, &index, 
                                                    &pci_info->bus))
    {
         fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                 "ModMgmt: get_pci_addr_from_device detected a bad string at line %d:\n",
                 __LINE__);
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                 "%s\n",pci_string);
        return FALSE;
    }
    if(!fbe_module_mgmt_reg_scan_string_for_int(',', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    pci_string, &index, 
                                                    &pci_info->device))
    {
       fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                 "ModMgmt: get_pci_addr_from_device detected a bad string at line %d:\n",
                 __LINE__);
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                 "%s\n",pci_string);
        return FALSE;
    }

    if(!fbe_module_mgmt_reg_scan_string_for_int(')', FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, 
                                                    pci_string, &index, 
                                                    &pci_info->function))
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: get_pci_addr_from_device detected a bad string at line %d:\n",
                              __LINE__);
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s\n",pci_string);
        return FALSE;
    }

    return TRUE;
}




/**************************************************************************
 *    fbe_module_mgmt_generate_affinity_setting
 **************************************************************************
 *
 *  @brief
 *   This function takes a pci address, looks up the configured flexport
 *   information for this device and based on an algorithm returns the
 *   interrupt affinity setting for the port.
 *    
 *
 * @param module_mgmt - object context
 * @param fbe_bool_t set_default - return the default core assignment
 *                       when TRUE the 2nd arg is NULL
 * @param SPECL_PCI_DATA *pci_info - pci address
 *
 * @return fbe_u32_t - affinity setting 
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_u32_t fbe_module_mgmt_generate_affinity_setting(fbe_module_mgmt_t *module_mgmt,
                                                    fbe_bool_t set_default, 
                                                    SPECL_PCI_DATA *pci_info)
{
    fbe_u32_t iom_num;
    fbe_u32_t port_num;
    fbe_u32_t slot;
    fbe_u64_t device_type;
    fbe_u32_t num_cores = 0;
    fbe_u32_t affinity_setting = 0;
    fbe_base_board_get_base_board_info_t *base_board_info_ptr = NULL;
    fbe_u32_t port_index = INVALID_PORT_U8;
    SP_ID sp_id;
    csx_p_phys_topology_info_t *phys_topology_info;
    fbe_u32_t core_count = 0;
    fbe_u32_t core_index = 0;
    fbe_u32_t num_sockets = 0;
    fbe_u32_t core_assign_index = 0;
    fbe_u32_t socket_assign_index = 0;
    fbe_u32_t core_assign_bmask = 0;
    fbe_u32_t socket_core_count = 0;

    sp_id = module_mgmt->local_sp;
    base_board_info_ptr = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_base_board_get_base_board_info_t));
    if(base_board_info_ptr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to allocate for board object information.\n",
                              __FUNCTION__);
        // Rather than setting this to core 0, which we do not want
        return 1;
    }

    fbe_zero_memory(base_board_info_ptr, sizeof(fbe_base_board_get_base_board_info_t));
    fbe_api_board_get_base_board_info(module_mgmt->board_object_id, base_board_info_ptr);
    num_cores = base_board_info_ptr->platformInfo.processorCount;

    csx_p_phys_topology_info_query(&phys_topology_info);

    core_count = phys_topology_info->phys_cores_count;
    num_sockets = phys_topology_info->phys_packages_count;

    if(num_cores == 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Core count detected at 0 setting it to default value.\n",
                              __FUNCTION__);
        num_cores = PLATFORM_CORE_COUNT_DEFAULT;
    }

    if(pci_info != NULL)
    {
        port_index = fbe_module_mgmt_get_port_index_from_pci_info(module_mgmt,pci_info->bus, pci_info->device, pci_info->function);
        if (port_index != INVALID_PORT_U8) 
        {
            iom_num = fbe_module_mgmt_get_io_port_module_number(module_mgmt, module_mgmt->local_sp, port_index); 
            port_num = fbe_module_mgmt_get_io_port_number(module_mgmt, module_mgmt->local_sp, port_index);
            fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, module_mgmt->local_sp, iom_num, &slot, &device_type);
        }
    }

    if( (set_default) || (port_index == INVALID_PORT_U8) )
    {
      /* SAS driver has a requirement that each port on a SLIC have a unique interrupt affinity 
       * sets the default to the PCI function number, giving each port a unique value.
       */
        csx_p_phys_topology_info_free(phys_topology_info);
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, base_board_info_ptr);
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Logical Port Index invalid.  Setting Port Affinity to default:%d\n", 
                              __FUNCTION__,
                              pci_info->function);
        return (pci_info->function);
    }
    
    // This is set for FE & BE ports not UNC ports
    if(((module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.port_role == IOPORT_PORT_ROLE_FE) ||
        (module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.port_role == IOPORT_PORT_ROLE_BE)) &&
        (module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num != INVALID_PORT_U8))
    {
        if((num_sockets == 1) ||
           (base_board_info_ptr->platformInfo.cpuModule == MERIDIAN_CPU_MODULE) ||
           (base_board_info_ptr->platformInfo.cpuModule == TUNGSTEN_CPU_MODULE))
        {
            /*
             * The affinity setting is simply set round robin based on logical FE/BE number
             * and the number of cores for the system. 
             */
            affinity_setting = 
                (module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num % num_cores);

            if ((base_board_info_ptr->platformInfo.cpuModule == MERIDIAN_CPU_MODULE) ||
                (base_board_info_ptr->platformInfo.cpuModule == TUNGSTEN_CPU_MODULE))
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s logical port %u affines to core %u due to Virtual CPU module\n",
                                      __FUNCTION__,
                                      module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num,
                                      affinity_setting);
            }

        }
        else
        {
            /*
             * If we have more than one CPU socket then we have to affine things based on PCI bus address to socket location 
             * as well as the round robin approach to logical number. 
             * For our multi-socket platform, PCI bus 0x80 and greater are on socket 1, the rest are on socket 0. 
             */
            
            // First get the core number within the socket to affine the port to
            core_assign_index = 
                module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num % (core_count / num_sockets);
            // This is a temporary method of determining which socket to assign the core to
            socket_assign_index = (slot > 5 ? 0 : 1);

            // iterate through the cores on all sockets to count the cores per socket and affine the port to the correct core
            for(core_index=0;core_index<core_count;core_index++)
            {
                core_assign_bmask = 1 << core_index;
                if(core_assign_bmask & phys_topology_info->phys_packages[socket_assign_index])
                {
                    if(socket_core_count == core_assign_index)
                    {
                        affinity_setting = core_index;
                        break;
                    }
                    socket_core_count++;
                }
            }
        }
    }
    else
    {
      /* SAS driver has a requirement that each port on a SLIC have a unique interrupt affinity 
       * sets the default to the PCI function number, giving each port a unique value.
       */
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Uncommitted Port.  Setting Port Affinity to default value of :%d\n", 
                              __FUNCTION__,
                              pci_info->function);
        affinity_setting = pci_info->function;
    }
    csx_p_phys_topology_info_free(phys_topology_info);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, base_board_info_ptr);
    return affinity_setting;
}


/**************************************************************************
 *    fbe_module_mgmt_update_removed_affinity_entries
 **************************************************************************
 *
 * @brief
 *   This function goes through the list of configured devices and checks for
 *   hardware no longer present in the system.  It then sets the core assignment
 *   to default and sets a flag to update the Registry.
 *    
 *
 * @param module_mgmt - object context
 * @param configured_count - number of configured ports
 *
 * @return fbe_bool_t - entry removed
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_bool_t fbe_module_mgmt_update_removed_affinity_entries(fbe_module_mgmt_t *module_mgmt,
                                                           fbe_u32_t configured_count)
{
    fbe_u32_t configured_index = 0;
    fbe_bool_t entry_removed = FALSE;

    for(configured_index = 0; configured_index < configured_count; configured_index++) 
    {
        if(module_mgmt->configured_interrupt_data[configured_index].present_in_system == FALSE)
        {
            module_mgmt->configured_interrupt_data[configured_index].processor_core = 
                fbe_module_mgmt_generate_affinity_setting(module_mgmt, TRUE, NULL);
            module_mgmt->configured_interrupt_data[configured_index].modify_needed = TRUE;
            entry_removed = TRUE;

        }
     }
    return entry_removed;
}

/**************************************************************************
 *    fbe_module_mgmt_update_processor_affinity_settings
 **************************************************************************
 *
 *  @brief
 *   This function goes through the configured affinity settings list and
 *   applies these settings to the enumerated PCI instances in the Registry.
 *
 * @param module_mgmt - object context    
 * @param fbe_u32_t configured_count - number of configured devices
 *
 * @return fbe_bool_t - changes were made or not
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_bool_t fbe_module_mgmt_update_processor_affinity_settings(fbe_module_mgmt_t *module_mgmt,
                                                              fbe_u32_t configured_count)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t configured_index = 0;
    fbe_bool_t settings_updated = FALSE;
    fbe_u32_t old_core_num;

    for(configured_index = 0; configured_index < configured_count; configured_index++)
    {
        /*
         * If this is a newly added piece of hardware or a recently removed piece of
         * hardware, or newly confiugred, update the settings.
         */

        fbe_module_mgmt_get_processor_affinity(module_mgmt,
                        module_mgmt->configured_interrupt_data[configured_index].device_identity_string,
                module_mgmt->configured_interrupt_data[configured_index].device_instance_string,
                &old_core_num);

        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                               "%s original affinity setting: %d\n",
                               __FUNCTION__, old_core_num);

        if(module_mgmt->configured_interrupt_data[configured_index].modify_needed ||
                        old_core_num != module_mgmt->configured_interrupt_data[configured_index].processor_core)
        {
            status = fbe_module_mgmt_set_processor_affinity(module_mgmt->configured_interrupt_data[configured_index].device_identity_string,
                             module_mgmt->configured_interrupt_data[configured_index].device_instance_string,
                             module_mgmt->configured_interrupt_data[configured_index].processor_core);
            
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "ModMgmt: update_processor_affinity failed write for the following device:\n");
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s   %s  core:%d\n", 
                                     module_mgmt->configured_interrupt_data[configured_index].device_identity_string,
                                     module_mgmt->configured_interrupt_data[configured_index].device_instance_string,
                                     module_mgmt->configured_interrupt_data[configured_index].processor_core);
                return settings_updated;
            }
            settings_updated = TRUE;
        }

    }
    return settings_updated;

}

/**************************************************************************
 *    fbe_module_mgmt_remove_unused_entries_in_affinity_list
 **************************************************************************
 *
 *  @brief
 *   This function removes entries from the affinity list for devices that
 *   are no longer found in the system.
 *    
 * @param module_mgmt - object context   
 * @param fbe_u32_t configured_count - number of configured devices
 *
 * @return fbe_u32_t - new number of configured devices 
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
fbe_u32_t fbe_module_mgmt_remove_unused_entries_in_affinity_list(fbe_module_mgmt_t *module_mgmt,
                                                                 fbe_u32_t configured_count)
{
    fbe_u32_t configured_index = 0;
    fbe_u32_t new_configured_count = 0;
    fbe_module_mgmt_configured_interrupt_data_t *temp_list;

    /*
     * Allocate memory for a temporary list of entries for only devices present
     * in the system.
     */
    temp_list = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_module_mgmt_configured_interrupt_data_t) * configured_count);

    for(configured_index = 0; configured_index < configured_count; configured_index++)
    {
        /*
         * If an entry is for a device that is present in the system, save the entry 
         * in the temporary list, and keep a count for the size of this new list. 
         */
        if(module_mgmt->configured_interrupt_data[configured_index].present_in_system)
        {
            fbe_copy_memory(&temp_list[new_configured_count], 
                   &module_mgmt->configured_interrupt_data[configured_index], 
                   sizeof(fbe_module_mgmt_configured_interrupt_data_t));
            new_configured_count++;
        }
    }

    /*
     * Zero out the old list and then copy all the entriest for devices present 
     * in the system from the temporary list back into the global list. 
     */
    fbe_set_memory(module_mgmt->configured_interrupt_data, 0, 
           (sizeof(fbe_module_mgmt_configured_interrupt_data_t) * FBE_ESP_IO_PORT_MAX_COUNT));

    fbe_copy_memory(module_mgmt->configured_interrupt_data, temp_list, 
           (sizeof(fbe_module_mgmt_configured_interrupt_data_t) * new_configured_count));

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, temp_list);


    return new_configured_count;
}


/**************************************************************************
 *    fbe_module_mgmt_update_affinity_list_in_registry
 **************************************************************************
 *
 *  @brief
 *   This function removes all previous entries from the registry and
 *   creates a new set of entriest based on the global list of configured
 *   data.
 *   The format for the entries are defined as PCI Address\Core#\Device ID\InstanceID.
 *   
 * @param module_mgmt - object context    
 * @param fbe_u32_t configured_count - number of configured devices
 *
 * @return none 
 *
 *  HISTORY:
 *      14-Jun-10 - bphilbin - created.
  **************************************************************************/
void fbe_module_mgmt_update_affinity_list_in_registry(fbe_module_mgmt_t *module_mgmt,
                                                      fbe_u32_t configured_count)
{
    fbe_u32_t configured_index;
    fbe_u32_t deletion_index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_char_t configured_affinity_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];

    /*
     * Clear out the registry entries first, we will then write out all new 
     * entries.  It is assumed that entries are in numerical order starting 
     * at 0 with no gaps. 
     */

    while(status == FBE_STATUS_OK)
    {
        // check if the entry exists, if it does then delete the entry
        status = fbe_module_mgmt_get_configured_interrupt_affinity_string(&deletion_index, configured_affinity_string);
        if(status == FBE_STATUS_OK)
        {
          status = fbe_module_mgmt_clear_configured_interrupt_affinity_string(&deletion_index);
          deletion_index++;
        }
    }

    for(configured_index = 0; configured_index < configured_count; configured_index++)
    {
        /*
         * Generate the string in the standard format. 
         * "[bus.slot.func]\core affinity\ID string\Instance string "
         */
        fbe_set_memory(configured_affinity_string, 0, (sizeof(fbe_char_t) * FBE_MOD_MGMT_REG_MAX_STRING_LENGTH));
        _snprintf(configured_affinity_string, FBE_MOD_MGMT_REG_MAX_STRING_LENGTH, "[%d.%d.%d]\\%d\\%s\\%s",
                  module_mgmt->configured_interrupt_data[configured_index].pci_data.bus,
                  module_mgmt->configured_interrupt_data[configured_index].pci_data.device,
                  module_mgmt->configured_interrupt_data[configured_index].pci_data.function,
                  module_mgmt->configured_interrupt_data[configured_index].processor_core,
                  module_mgmt->configured_interrupt_data[configured_index].device_identity_string,
                  module_mgmt->configured_interrupt_data[configured_index].device_instance_string);

        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: Affinity Instance:%d %s\n",
                              configured_index, configured_affinity_string);


        status = fbe_module_mgmt_set_configured_interrupt_affinity_string(&configured_index, 
                                                      configured_affinity_string);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "ModMgmt: update_affinity_list_in_reg failed registry write for index:%d status:0x%x",
                                  configured_index, status);
            return;
        }
    }
    return;
}


/**************************************************************************
 *    fbe_module_mgmt_check_and_set_msi_message_limit
 **************************************************************************
 *
 *  @brief
 *   This function checks for an iSCSI port and will set the MSI message limit
 *   for that device in the same area of the registry as the Interrupt Affinity.
 *   
 * @param module_mgmt - object context
 * @param pci_info - PCI address information
 * @param interrupt_data_index - index into the interrupt configuration data
 *
 * @return none 
 *
 *  HISTORY:
 *      13-Dec-12 - bphilbin - created.
  **************************************************************************/
static void fbe_module_mgmt_check_and_set_msi_message_limit(fbe_module_mgmt_t *module_mgmt, 
                                                            SPECL_PCI_DATA *pci_info, 
                                                            fbe_u32_t interrupt_data_index)
{
    fbe_status_t status;
    fbe_u32_t port_index;
    fbe_u32_t iom_num;
    fbe_u32_t port_num;
    fbe_module_slic_type_t slic_type;
    fbe_u32_t message_limit = 0;

    port_index = fbe_module_mgmt_get_port_index_from_pci_info(module_mgmt, pci_info->bus, 
                                                              pci_info->device, pci_info->function);

    iom_num = fbe_module_mgmt_get_iom_num_from_port_index(module_mgmt, port_index);
    port_num = fbe_module_mgmt_get_port_num_from_port_index(module_mgmt, port_index);

    slic_type = fbe_module_mgmt_get_slic_type(module_mgmt, module_mgmt->local_sp, iom_num);
    
    if( (module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].protocol 
        == IO_CONTROLLER_PROTOCOL_ISCSI) &&
        (slic_type != FBE_SLIC_TYPE_4PORT_ISCSI_1G) )
    {
        // iSCSI ports need to have msi message limit set in the registry
        message_limit = 1;

        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: setting MSI message limit to %d for %s %d port %d.\n",
                              message_limit,
                              fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, module_mgmt->local_sp, iom_num)),
                              fbe_module_mgmt_get_slot_num(module_mgmt, module_mgmt->local_sp, iom_num),
                              port_num);
    
        status = fbe_module_mgmt_set_msi_message_limit(module_mgmt->configured_interrupt_data[interrupt_data_index].device_identity_string,
                                                       module_mgmt->configured_interrupt_data[interrupt_data_index].device_instance_string,
                                                       message_limit);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "ModMgmt: failed to set MSI message limit in Registry Status:%d\n",
                                  status);
        }
    }
    return;
}
