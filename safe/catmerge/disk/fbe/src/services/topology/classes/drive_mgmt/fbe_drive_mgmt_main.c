/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_mgmt_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the drive
 *  Management object. This includes the create and
 *  destroy methods for the drive management object, as well
 *  as the event entry point, load and unload methods.
 * 
 * @ingroup drive_mgmt_class_files
 * 
 * @version
 *   25-Feb-2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_environment_limit.h"
#include "fbe_topology.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_drive_mgmt_private.h"

/* Class Member Data */

/* The following policy table contains a policy name and value indicating if it
*  is enabled by default.  The Drive Mgmt class provides methods for testing if
*  a policy is enabled and changing its value.  Since these methods can be accessed
*  by multiple contexts, there is a lock for guarding the data.  In general, all
*  mutable class/instance data should be protected.
*/
/* AJ: I know that C4_INTEGRATED should not be used in fbe code at this level.
       This is a very temporary stop-gap work-around that will be removed within a few day.
       */
#ifndef ALAMOSA_WINDOWS_ENV
/* Keep EQ checking off for KH.*/
fbe_drive_mgmt_policy_t fbe_drive_mgmt_policy_table[] = {
{FBE_DRIVE_MGMT_POLICY_VERIFY_CAPACITY,    FBE_DRIVE_MGMT_POLICY_OFF}, // default settings
{FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING, FBE_DRIVE_MGMT_POLICY_OFF}, // now handled by Database Service
{FBE_DRIVE_MGMT_POLICY_SET_SYSTEM_DRIVE_IO_MAX, FBE_DRIVE_MGMT_POLICY_OFF}, //default setting
{FBE_DRIVE_MGMT_POLICY_VERIFY_FOR_SED_COMPONENTS, FBE_DRIVE_MGMT_POLICY_ON}, //default setting - Turn on Sed drive kill policy
//, add more policies here.
{FBE_DRIVE_MGMT_POLICY_LAST}  // marks end of policy table
};

#else
/* Enable EQ checking for Rockies. */
fbe_drive_mgmt_policy_t fbe_drive_mgmt_policy_table[] = {
{FBE_DRIVE_MGMT_POLICY_VERIFY_CAPACITY,    FBE_DRIVE_MGMT_POLICY_OFF}, // default settings
{FBE_DRIVE_MGMT_POLICY_VERIFY_ENHANCED_QUEUING, FBE_DRIVE_MGMT_POLICY_OFF}, // now handled by Database Service
{FBE_DRIVE_MGMT_POLICY_SET_SYSTEM_DRIVE_IO_MAX, FBE_DRIVE_MGMT_POLICY_ON}, //default setting
{FBE_DRIVE_MGMT_POLICY_VERIFY_FOR_SED_COMPONENTS, FBE_DRIVE_MGMT_POLICY_OFF}, //default setting
//, add more policies here.
{FBE_DRIVE_MGMT_POLICY_LAST}  // marks end of policy table
};


#endif /* ALAMOSA_WINDOWS_ENV - ARCH - configuration_differences */
fbe_spinlock_t fbe_drive_mgmt_policy_table_lock;

/* Class methods forward declaration */
fbe_status_t fbe_drive_mgmt_load(void);
fbe_status_t fbe_drive_mgmt_unload(void);

/* Export class methods  */
fbe_class_methods_t fbe_drive_mgmt_class_methods = { FBE_CLASS_ID_DRIVE_MGMT,
                                                    fbe_drive_mgmt_load,
                                                    fbe_drive_mgmt_unload,
                                                    fbe_drive_mgmt_create_object,
                                                    fbe_drive_mgmt_destroy_object,
                                                    fbe_drive_mgmt_control_entry,
                                                    fbe_drive_mgmt_event_entry,
                                                    NULL,
                                                    fbe_drive_mgmt_monitor_entry
                                                    };
 
/*!***************************************************************
 * fbe_drive_mgmt_load()
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
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_load(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "DMO: %s entry\n", __FUNCTION__);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_drive_mgmt_t) < FBE_MEMORY_CHUNK_SIZE);

    /* assert cmi message sizes */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(dmo_cmi_msg_standard_t) < FBE_MEMORY_CHUNK_SIZE);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(dmo_cmi_msg_start_t) < FBE_MEMORY_CHUNK_SIZE);

    return fbe_drive_mgmt_monitor_load_verify();
}
/******************************************
 * end fbe_drive_mgmt_load()
 ******************************************/

/*!***************************************************************
 * fbe_drive_mgmt_unload()
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
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_drive_mgmt_unload(void)
{
    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "DMO: %s entry\n", __FUNCTION__);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_drive_mgmt_unload()
 ******************************************/

/*!***************************************************************
 * fbe_drive_mgmt_create_object()
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
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_drive_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_status_t status;
    fbe_drive_mgmt_t * drive_mgmt;

    /* Call parent constructor */
    status = fbe_base_environment_create_object(packet, object_handle);
    if (status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    drive_mgmt = (fbe_drive_mgmt_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) drive_mgmt, FBE_CLASS_ID_DRIVE_MGMT);

    //uncomment to turn on debugging
    //fbe_base_object_set_trace_level((fbe_base_object_t *) drive_mgmt, FBE_TRACE_LEVEL_DEBUG_HIGH);    
    fbe_drive_mgmt_init(drive_mgmt);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_drive_mgmt_create_object()
 ******************************************/

/*!***************************************************************
 * fbe_drive_mgmt_destroy_object()
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
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_drive_mgmt_destroy_object( fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_drive_mgmt_t * drive_mgmt;
    
    drive_mgmt = (fbe_drive_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace((fbe_base_object_t*)drive_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "DMO: %s entry\n", __FUNCTION__);

    fbe_spinlock_destroy(&drive_mgmt->drive_info_lock);
    fbe_spinlock_destroy(&drive_mgmt->fuel_gauge_info_lock);
    fbe_spinlock_destroy(&fbe_drive_mgmt_policy_table_lock);

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, drive_mgmt->drive_info_array);

   
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, drive_mgmt->xml_parser);

    /* Call parent destructor */
    status = fbe_base_environment_destroy_object(object_handle);
    return status;
}
/******************************************
 * end fbe_drive_mgmt_destroy_object()
 ******************************************/

/*!***************************************************************
 * fbe_drive_mgmt_event_entry()
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
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_drive_mgmt_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_drive_mgmt_event_entry()
 ******************************************/

/*!***************************************************************
 * fbe_drive_mgmt_init()
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
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_drive_mgmt_init(fbe_drive_mgmt_t * drive_mgmt)
{
    fbe_u32_t i;
    fbe_drive_info_t *drive_info;    
    fbe_u32_t min_poll_interval = 0;

    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "DMO: %s entry\n", __FUNCTION__);


    drive_mgmt->max_supported_drives = fbe_environment_limit_get_platform_fru_count();

    drive_mgmt->drive_info_array = (fbe_drive_info_t *)fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, drive_mgmt->max_supported_drives * sizeof(fbe_drive_info_t));
    fbe_zero_memory(drive_mgmt->drive_info_array, drive_mgmt->max_supported_drives * sizeof(fbe_drive_info_t));

    fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "DMO: %s Max drives supported = %d\n", __FUNCTION__, drive_mgmt->max_supported_drives);

    drive_info = drive_mgmt->drive_info_array;
    for (i = 0; i < drive_mgmt->max_supported_drives; i++)
    {
        fbe_drive_mgmt_init_drive(drive_info);
        drive_info++;
        //TODO: init remaining members of fwdl
    }
    fbe_spinlock_init(&drive_mgmt->drive_info_lock);
    fbe_spinlock_init(&drive_mgmt->fuel_gauge_info_lock);

    drive_mgmt->total_drive_count = 0;

    /* Initialize the fields used to track the OS drives' availability. */
    drive_mgmt->osDriveAvail = FBE_FALSE;
    drive_mgmt->startTimeForOsDriveUnavail = 0;
    drive_mgmt->osDriveUnavailRideThroughInSec = FBE_DRIVE_MGMT_OS_DRIVE_RIDE_THROUGH_TIMEOUT;

    fbe_spinlock_init(&fbe_drive_mgmt_policy_table_lock);

    /* Initialize fuel gauge structure. By default the feature is on. */
    fbe_zero_memory(&drive_mgmt->fuel_gauge_info,  sizeof(fbe_drive_mgmt_fuel_gauge_info_t));
    drive_mgmt->fuel_gauge_info.auto_log_flag = FBE_TRUE;
    drive_mgmt->fuel_gauge_info.current_index = 0;
    drive_mgmt->fuel_gauge_info.current_supported_page_index = LOG_PAGE_NONE;
    
    fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_in_progrss, FBE_FALSE);
    fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_1st_time, FBE_TRUE);
    
    /* Initialize get fuel gauge is not from fbecli dmo command. */
    drive_mgmt->fuel_gauge_info.is_fbecli_cmd = FBE_FALSE;
    
    /* Get the minimum poll interval from register. */ 
    fbe_drive_mgmt_get_fuel_gauge_poll_interval_from_registry(drive_mgmt, &min_poll_interval);

    drive_mgmt->fuel_gauge_info.min_poll_interval = min_poll_interval;
    
    /* Initialize drivegetlog structure.*/
    fbe_zero_memory(&drive_mgmt->drive_log_info,  sizeof(fbe_drive_mgmt_drive_log_info_t));

        /* init the logical crc error action*/
    fbe_drive_mgmt_get_logical_error_action(drive_mgmt, &drive_mgmt->logical_error_actions);
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %s: setting logical crc error actions=0x%x\n",__FUNCTION__, drive_mgmt->logical_error_actions);

    /* allocate and initalize dieh parser */
    drive_mgmt->xml_parser = (fbe_dmo_thrsh_parser_t*)fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, sizeof(fbe_dmo_thrsh_parser_t));
    fbe_zero_memory(drive_mgmt->xml_parser, sizeof(fbe_dmo_thrsh_parser_t));

    drive_mgmt->primary_os_drive_policy_check_failed = FBE_FALSE;
    drive_mgmt->secondary_os_drive_policy_check_failed = FBE_FALSE;

    /* Setting the value to a hardcoded value and the same as FBE_SAS_PHYSICAL_DRIVE_OUTSTANDING_IO_MAX (sas_physical_drive_private.h).
       Not the best option but did not want to alter this for all systems now.*/
    drive_mgmt->system_drive_normal_queue_depth         = 22;
    drive_mgmt->system_drive_reduced_queue_depth        = 10;
    drive_mgmt->system_drive_queue_depth_reduce_count   = 0;

    strncpy(drive_mgmt->drive_config_xml_info.version, "xxx", FBE_DMO_DRIVECONFIG_XML_MAX_VERSION_LEN);
    strncpy(drive_mgmt->drive_config_xml_info.description, "WARNING: NOT INSTALLED", FBE_DMO_DRIVECONFIG_XML_MAX_DESCRIPTION_LEN);
    drive_mgmt->drive_config_xml_info.filepath[0] = '\0';

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_drive_mgmt_init()
 ******************************************/

 
/*!***************************************************************
 * fbe_drive_mgmt_policy_enabled()
 ****************************************************************
 * @brief
 *  This functions returns true if a given policy is enabled
 *
 * @param  - policy_id - Policy being queried.
 *
 * @return fbe_status_t - True if policy is enabled.
 *
 * @notes : If policy is not found in table then it is assumed to 
 *  be disabled.
 *
 * @author
 *  18-May-2010 - Created.  Wayne Garrett
 *
 ****************************************************************/
fbe_bool_t   
fbe_drive_mgmt_policy_enabled(fbe_drive_mgmt_policy_id_t policy_id)
{
    fbe_bool_t  status = FBE_FALSE;
    fbe_bool_t  policy_found = FBE_FALSE;
    fbe_drive_mgmt_policy_t* policy_table_index = NULL;
    
    fbe_spinlock_lock(&fbe_drive_mgmt_policy_table_lock);
    policy_table_index = &fbe_drive_mgmt_policy_table[0];
    while (policy_table_index->id != FBE_DRIVE_MGMT_POLICY_LAST)
    {
        if ( policy_table_index->id == policy_id )
        {
            policy_found = FBE_TRUE;
            status = (policy_table_index->value == FBE_DRIVE_MGMT_POLICY_ON) ? 
                        FBE_TRUE : FBE_FALSE;            
            break;
        }  
        policy_table_index++;
    }
    fbe_spinlock_unlock(&fbe_drive_mgmt_policy_table_lock);
    
    if (!policy_found)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_DRIVE_MGMT, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                 "DMO: %s Policy not found in table. Returning disabled.\n", __FUNCTION__);
    }
        
    return status;
}

/* Initializes drive structure
 */
void 
fbe_drive_mgmt_init_drive(fbe_drive_info_t *drive)
{
    if (!drive->dbg_remove)
    {
        drive->parent_object_id = FBE_OBJECT_ID_INVALID;
        drive->location.bus = 0;
        drive->location.enclosure = 0;
        drive->location.slot = 0;        
    }
    drive->object_id = FBE_OBJECT_ID_INVALID;
    drive->drive_type = FBE_DRIVE_TYPE_INVALID;
    drive->gross_capacity = 0;
    drive->state = FBE_LIFECYCLE_STATE_INVALID;
    drive->logical_state = FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED;
    drive->death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID;
    fbe_zero_memory(&drive->tla, FBE_SCSI_INQUIRY_TLA_SIZE+1);
    fbe_zero_memory(&drive->rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1);
    fbe_zero_memory(&drive->sn,  FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);
    drive->last_log = 0;
    drive->driveDebugAction = FBE_DRIVE_ACTION_NONE;
}

/*!***************************************************************
 * fbe_drive_mgmt_set_all_pdo_crc_actions()
 ****************************************************************
 * @brief
 *  This functions sets all the Physical Drives' CRC actions.
 *
 * @param  - drive_mgmt - Ptr to drive mgmt object
 * @param  - action     - bitmap containing actions to set.
 *
 * @return fbe_status_t - True if policy is enabled.
 *
 * @author
 *  05/11/2012  Wayne Garrett - Created. 
 *
 ****************************************************************/
void fbe_drive_mgmt_set_all_pdo_crc_actions(fbe_drive_mgmt_t *drive_mgmt, fbe_pdo_logical_error_action_t action)
{
    fbe_drive_info_t *pDriveInfo;
    fbe_u32_t i;
    fbe_status_t status;

    pDriveInfo = drive_mgmt->drive_info_array;
    for (i = 0; i < drive_mgmt->max_supported_drives; i++)
    {                
        if(pDriveInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
            /* Set logical crc actions */
            status = fbe_api_physical_drive_set_crc_action(pDriveInfo->object_id, action);
            if (FBE_STATUS_OK != status)
            {
                fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "DMO: %s: failed to set crc action, object_id 0x%x",__FUNCTION__, pDriveInfo->object_id);                
            }           
        }
        pDriveInfo++;
    }
}
