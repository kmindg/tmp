/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_event_log_esp_msgs.c
 ***************************************************************************
 *
 * @brief
 *  This file contain event log message string for ESP package.
 * 
 * @version
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_event_log_utils.h"

/*********************************************************************/
/*                  loacal functions                                 */
/*********************************************************************/
static fbe_status_t fbe_event_log_esp_get_info_generic_msg(fbe_u32_t msg_id, 
                                                           fbe_u8_t* msg,
                                                           fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_esp_get_warn_generic_msg(fbe_u32_t msg_id, 
                                                           fbe_u8_t *msg,
                                                           fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_esp_get_error_generic_msg(fbe_u32_t msg_id, 
                                                            fbe_u8_t *msg,
                                                            fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_esp_get_crit_generic_msg(fbe_u32_t msg_id, 
                                                           fbe_u8_t *msg,
                                                           fbe_u32_t msg_len);

/*!***************************************************************
 * @fn fbe_event_log_get_esp_msg(fbe_u32_t msg_id, 
 *                               fbe_u8_t *msg
 *                               fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function directs the appropriate function depend 
 *  upon ESP message severity type.
 *
 * @param msg Buffer to store message string
 * @param msg_id Message ID
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_event_log_get_esp_msg(fbe_u32_t msg_id, fbe_u8_t *msg, fbe_u32_t msg_len)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t mask_bit = 0xFFFFF000;

    if((msg_id & mask_bit) == ESP_INFO_GENERIC)
    {
        status = fbe_event_log_esp_get_info_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == ESP_WARN_GENERIC)
    {
        status = fbe_event_log_esp_get_warn_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == ESP_ERROR_GENERIC)
    {
        status = fbe_event_log_esp_get_error_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == ESP_CRIT_GENERIC)
    {
        status = fbe_event_log_esp_get_crit_generic_msg(msg_id, msg, msg_len);
    }
    return status; 
}
/***************************************
*   end of fbe_event_log_get_esp_msg()  
****************************************/
/*!***************************************************************
 * @fn fbe_event_log_esp_get_info_generic_msg(fbe_u32_t msg_id, 
                                              fbe_u8_t *msg,
                                              fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for ESP of
 *  ESP_INFO_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Message length for buffer
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t  
fbe_event_log_esp_get_info_generic_msg(fbe_u32_t msg_id, 
                                       fbe_u8_t *msg,
                                       fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new ESP_INFO_GENERIC message 
     * that added in EspMessages.mc file, along with the appropriate format 
     * specifier.
     */
     switch(msg_id)
    {
        case ESP_INFO_LOAD_VERSION:
            full_msg ="Driver compiled for %s.\n";
            break;

        case ESP_INFO_SPS_INSERTED:
            full_msg ="%s Inserted.\n";
            break;

        case ESP_INFO_SPS_STATE_CHANGE:
            full_msg ="%s State Changed (%s to %s).\n";
            break;

        case ESP_INFO_SPS_TEST_TIME:
            full_msg ="Scheduled SPS Test Time.\n";
            break;

        case ESP_INFO_SPS_NOT_RECOMMENDED:
            full_msg ="%s is not recommended for this array type.\n";
            break;

        case ESP_INFO_SPS_CONFIG_CHANGE:
            full_msg ="%s Configuration Change (SPS Test needed to verify cabling).\n";
            break;

        case ESP_INFO_PS_INSERTED:
            full_msg ="%s Inserted.\n";
            break;

        case ESP_INFO_FUP_QUEUED:
            full_msg ="%s FUP queued, rev:%s, pid:%s\n";
            break;

        case ESP_INFO_FUP_STARTED:
            full_msg ="%s FUP started, rev:%s, pid:%s\n";
            break;

        case ESP_INFO_FUP_UP_TO_REV:
            full_msg ="%s firmware revision %s is up to date, firmware upgrade is not needed.\n";
            break;

        case ESP_INFO_FUP_SUCCEEDED:
            full_msg ="%s firmware upgrade succeeded, current revision %s, old revision %s.\n";
            break;

        case ESP_INFO_FUP_ABORTED:
            full_msg ="%s firmware upgrade aborted, current revision %s, old revision %s.\n";
            break;

        case ESP_INFO_FUP_TERMINATED:
            full_msg ="%s firmware upgrade terminated, current revision %s, old revision %s.\n";
            break;

        case ESP_INFO_FUP_WAIT_PEER_PERMISSION:
            full_msg ="%s firmware upgrade waiting for peer SP permission, current revision %s.\n";
            break;

        case ESP_INFO_FUP_ACTIVATION_DEFERRED:
            full_msg ="%s firmware upgrade activation deferred, current revision %s, product id %s.\n";
            break;

        case ESP_INFO_IO_MODULE_ENABLED:
            full_msg = "%s is enabled and %s.\n";
            break;
    
        case ESP_INFO_IO_MODULE_REMOVED:
            full_msg = "%s is removed and %s.";
            break;

        case ESP_INFO_IO_PORT_ENABLED:
            full_msg = "%s port %d is enabled and %s.\n";
            break;

        case ESP_INFO_DIEH_LOAD_SUCCESS:
            full_msg = "DIEH successfully loaded.\n";
            break;

        case ESP_INFO_DRIVE_ONLINE:
            full_msg = "%s came online. SN:%s TLA:%s Rev:%s.\n";
            break; 

        case ESP_INFO_SERIAL_NUM:
            full_msg = "%s SN: %s\n";
            break;
    
        case ESP_INFO_RESUME_PROM_READ_SUCCESS:
            full_msg = "%s Resume Prom Read Completed successfully.\n";
            break;
        
        case ESP_INFO_PEER_SP_INSERTED:
            full_msg = "Peer %s has been Inserted.\n";
            break;
             
        case ESP_INFO_PEER_SP_BOOT_SUCCESS:
            full_msg = "PBL: %s status: %s(0x%x).\n";
            break;
             
        case ESP_INFO_PEER_SP_BOOT_OTHER:
            full_msg = "PBL: %s status: %s(0x%x).\n";
            break;
             
        case ESP_INFO_MGMT_PORT_RESTORE_COMPLETED:
            full_msg = "%s management  port configuration request failed. Restoring last known working configuration completed. \
                         Please retry after making sure the requested configuration is supported.\n";
            break;

        case ESP_INFO_MGMT_PORT_CONFIG_COMPLETED:
            full_msg = "%s management port speed configuration request completed successfully.\n";
            break;
             
        case ESP_INFO_MGMT_PORT_CONFIG_FAILED:
            full_msg = "%s management port speed configuration request failed. This requested configuration is not supported.\n";
            break;

        case ESP_INFO_MGMT_PORT_RESTORE_FAILED:
            full_msg = "%s management port speed configuration request failed. Restoring last known working configuration also failed.\
                         Please gather diagnostic materials and contact your service provider.\n";
            break;

        case ESP_INFO_MGMT_FRU_INSERTED:
            full_msg = "%s is inserted.\n";
            break;

        case ESP_INFO_MGMT_FRU_FAULT_CLEARED:
            full_msg = "%s fault cleared.\n";
            break;

        case ESP_INFO_MGMT_FRU_LINK_UP:
            full_msg = "%s network connection is restored.\n";
            break;

        case ESP_INFO_MGMT_FRU_NO_NETWORK_CONNECTION:
            full_msg = "%s network connection not detected.\n";
            break;

        case ESP_INFO_MGMT_FRU_SERVICE_PORT_LINK_UP:
            full_msg = "The link on the service port on the %s is up.\n";
            break;

        case ESP_INFO_MGMT_FRU_SERVICE_PORT_NO_LINK:
            full_msg = "The link on the service port on the %s is down.\n";
            break;   
             
        case ESP_INFO_SFP_INSERTED:
            full_msg = "SFP located in %s in slot %d port %d is inserted.\n";
            break;

        case ESP_INFO_SFP_REMOVED:
            full_msg = "SFP located in %s in slot %d port %d is removed.\n";
            break;
        
        case ESP_INFO_CONNECTOR_INSERTED:
            full_msg = "%s is inserted.\n";
            break;
        
        case ESP_INFO_CONNECTOR_REMOVED:
            full_msg = "%s is removed.\n";
            break;
        
        case ESP_INFO_CONNECTOR_STATUS_CHANGE:
            full_msg = "%s cable status changed from %s to %s.\n";
            break;

        case ESP_INFO_ENCL_SHUTDOWN_CLEARED:
            full_msg = "%s shutdown condition has been cleared.\n";
            break;

        case ESP_INFO_FAN_INSERTED:
            full_msg = "%s Inserted\n";
            break;

        case ESP_INFO_LINK_STATUS_CHANGE:
             full_msg = "%s %d Port %d %s %s link status changed from %s to %s.\n";
             break;

        case ESP_INFO_LCC_INSERTED:
             full_msg = "%s inserted\n";
             break; 

        case ESP_INFO_LCC_FAULT_CLEARED:
            full_msg = "%s fault cleared\n";
            break; 

        case ESP_INFO_MGMT_PORT_CONFIG_COMMAND_RECEIVED:
            full_msg = "User requested %s management port speed change to auto neg: %s, speed: %s, duplex mode: %s.\n";
            break;

        case ESP_INFO_ENV_ABORT_UPGRADE_COMMAND_RECEIVED:
            full_msg = "Aborting %s firmware upgrade.\n";
            break;

        case ESP_INFO_ENV_RESUME_UPGRADE_COMMAND_RECEIVED:
            full_msg = "Resuming %s firmware upgrade.\n";
            break;

        case ESP_INFO_SUITCASE_FAULT_CLEARED:
            full_msg = "%s Fault cleared (%s).\n";
            break;

        case ESP_INFO_PS_OVER_TEMP_CLEARED:
             full_msg = "%s over temperature condition has been cleared.\n";
             break;

        case ESP_INFO_PS_FAULT_CLEARED:
             full_msg = "%s fault condition has been cleared.\n";
             break;

        case ESP_INFO_ENCL_OVER_TEMP_WARNING:
             full_msg = "%s is reporting OverTemp warning.\n";
             break;

        case ESP_INFO_ENCL_OVER_TEMP_CLEARED:
             full_msg = "%s over temperature %s condition has been cleared.\n";
             break;

        case ESP_INFO_ENCL_ADDED:
             full_msg = "%s added.\n";
             break;

        case ESP_INFO_SPS_CABLING_VALID:
            full_msg = "%s Cabling has been validated.\n";
            break;

        case ESP_INFO_ENCL_SHUTDOWN_DELAY_INPROGRESS:
            full_msg = "%s shutdown delay is in progress.\n";
            break;

        case ESP_INFO_RESET_ENCL_SHUTDOWN_TIMER:
            full_msg = "%s shutdown timer reseted.\n";
            break;

        case ESP_INFO_EXT_PEER_BOOT_STATUS_CHANGE:
            full_msg = "The core software is loading on %s. %s - %s(0x%x).\n";
            break;

        case ESP_INFO_USER_MODIFIED_WWN_SEED:
            full_msg = "A user command is received to modify the array wwn seed to 0x%x.\n";
            break;

        case ESP_INFO_USER_CHANGE_SYS_ID_INFO:
            full_msg = "A user command is received to modify the system id. SN %s, ChangeSN %d, PN %s, ChangePN %d.\n";
            break;

        case ESP_INFO_FAN_FAULT_CLEARED:
            full_msg ="%s fault has been cleared (%s).\n";
            break;

        case ESP_INFO_BMC_SHUTDOWN_WARNING_CLEARED:
            full_msg = "%s shutdown condition has been cleared.\n";
            break;

        case ESP_INFO_BMC_BIST_FAULT_CLEARED:
            full_msg = "%s BIST(Build In Self Test) Fault cleared.\n";
            break;

        case ESP_INFO_DRIVE_POWERED_ON:
            full_msg = "%s powered on.\n";
            break;

        case ESP_INFO_DRIVE_POWERED_OFF:
            full_msg = "%s powered off.\n";
            break;

        case ESP_INFO_CACHE_STATUS_CHANGED:
            full_msg ="%s CacheStatus changes from: %s to %s.\n";
            break;

        case ESP_INFO_LOW_BATTERY_FAULT_CLEARED:
            full_msg = "%s CPU low battery fault cleared.\n";
            break;
            
        case ESP_INFO_BM_ECB_FAULT_CLEARED:
            full_msg = "%s fault has been cleared. SP drive power distribution has returned to normal.\n";
            break;

        case ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED:
            full_msg = "%s environmental interface failure cleared.\n";
            break;

        case ESP_INFO_IO_PORT_CONFIGURED:
             full_msg = "%s port %d configured as %s.\n";
             break;

        case ESP_INFO_CACHE_CARD_CONFIGURATION_ERROR_CLEARED:
            full_msg = "%s configuration becomes correct.\n";
            break;

        case ESP_INFO_CACHE_CARD_FAULT_CLEARED:
            full_msg = "%s fault cleared.\n";
            break;

        case ESP_INFO_CACHE_CARD_INSERTED:
            full_msg = "%s is inserted.\n";
            break;

        case ESP_INFO_PEERSP_CLEAR:
             full_msg = "PBL: %s has BIOS/POST fault during last boot. The fault has been cleared.\n";
             break;
            
        case ESP_INFO_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION: 
             full_msg = "%s Predicted to exceed drive specified write endurance in %d days.\n";
             break;
            
        case ESP_INFO_DRIVE_BMS_FOR_WORST_HEAD:
             full_msg = "%s SN:%s BMS found %d entries for head: %d. Total entries: %d.\n";
             break;
             
        case ESP_INFO_DRIVE_BMS_LBB:
             full_msg = "%s SN:%s Lambda_BB for BMS is %d.\n";
             break;
             
         case ESP_INFO_SSC_INSERTED:
             full_msg = "%s Inserted.\n";
             break;
              
          case ESP_INFO_SSC_FAULT_CLEARED:
             full_msg = "%s fault has cleared\n";
             break;

        case ESP_INFO_DIMM_FAULT_CLEARED:
            full_msg = "%s fault cleared.\n";
            break;

        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK; 
}
/****************************************************
*   end of fbe_event_log_esp_get_info_generic_msg()  
****************************************************/
/*!***************************************************************
 * @fn fbe_event_log_esp_get_warn_generic_msg(fbe_u32_t msg_id, 
 *                                            fbe_u8_t *msg,
 *                                            fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for ESP of
 *  ESP_WARN_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Message length for buffer
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_event_log_esp_get_warn_generic_msg(fbe_u32_t msg_id, 
                                       fbe_u8_t *msg,
                                       fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;
    
    /* Add new switch case statement for every new ESP_WARN_GENERIC message 
     * that added in EspMessages.mc file, along with the appropriate format 
     * specifier.
     */
    switch(msg_id)
    {
        case ESP_WARN_FAN_DERGADED:
             full_msg = "%2 is Degraded. Please gather diagnostic materials and contact your service provider.\n";
            break;

        case ESP_WARN_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION:
             full_msg = "%s Predicted to exceed drive specified write endurance in %d days. \n";
            break;

        case ESP_WARN_IO_PORT_CONFIGURATION_DESTROYED:
            full_msg = "IO Port Configuration has been removed at the request of the user the SPs will reboot.\n";
            break;

        case ESP_WARN_IO_PORT_CONFIGURATION_CHANGED:
            full_msg = "IO Port configuration has changed the SP will reboot.\n";
            break;

        case ESP_WARN_DRIVE_BMS_OVER_WARN_THRESHOLD:
            full_msg = "%s SN:%s BMS found %d entries for head %d. Total entries: %d. Drive recommended for Proactive Spare.\n";
            break;
    
        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/****************************************************
*   end of fbe_event_log_esp_get_warn_generic_msg()  
****************************************************/
/*!***************************************************************
 * @fn fbe_event_log_esp_get_error_generic_msg(fbe_u32_t msg_id, 
 *                                             fbe_u8_t *msg,
 *                                             fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for ESP of
 *  ESP_ERROR_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Message length for buffer
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_event_log_esp_get_error_generic_msg(fbe_u32_t msg_id, 
                                        fbe_u8_t *msg,
                                        fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new ESP_ERROR_GENERIC message 
     * that added in EspMessages.mc file, along with the appropriate format 
     * specifier.
     */
    switch(msg_id)
    {
    case ESP_ERROR_SPS_REMOVED:
            full_msg ="%s has been Removed.\n";
            break;

    case ESP_ERROR_SPS_FAULTED:
            full_msg ="%s is Faulted (%s to %s).\n";
            break;

    case ESP_ERROR_SPS_CABLING_FAULT:
            full_msg ="%s is cabled incorrectly (%s).\n";
            break;

    case ESP_ERROR_SPS_NOT_SUPPORTED:
            full_msg ="%s is not supported.\n";
            break;

    case ESP_ERROR_PS_REMOVED:
            full_msg ="%s has been Removed.\n";
            break;

    case ESP_ERROR_PS_FAULTED:
            full_msg ="%s is Faulted (%s).\n";
            break;

    case ESP_ERROR_PS_OVER_TEMP_DETECTED:
            full_msg ="%s is reporting OverTemp. The Power Supply itself is probably not the source of the problem.\n";
            break;

    case ESP_ERROR_FAN_REMOVED:
            full_msg ="%s has been Removed.\n";
            break;

    case ESP_ERROR_FAN_FAULTED:
            full_msg ="%s is Faulted (%s).\n";
            break;

    case ESP_ERROR_FUP_FAILED:
            full_msg ="%s FUP failed, current rev:%s, %s\n";
            break;

    case ESP_ERROR_IO_MODULE_FAULTED:
            full_msg = "%s is faulted because %s.\n";
            break;

    case ESP_ERROR_IO_MODULE_INCORRECT:
            full_msg = "%s is incorrect found %s expected %s.\n";
            break;

    case ESP_ERROR_IO_PORT_FAULTED:
            full_msg = "%s port %d is faulted because %s.\n";
            break;

    case ESP_ERROR_MGMT_FRU_FAULTED:
         full_msg = "%s is faulted.\n";
        break;

    case ESP_ERROR_MGMT_FRU_REMOVED:
         full_msg = "%s is removed. \n";
        break;

    case ESP_ERROR_DIEH_LOAD_FAILED:
            full_msg = "DIEH failed to load, status %d.\n";
            break;                       

    case ESP_ERROR_DRIVE_CAPACITY_FAILED:
            full_msg = "%s failed capacity check. SN:%s TLA:%s Rev:%s.\n";
            break;

    case ESP_ERROR_DRIVE_OFFLINE:
            full_msg = "%s taken offline. SN:%s TLA:%s Rev:%s (%s) Reason:%s Action:%s.\n";
            break;

    case ESP_ERROR_DRIVE_OFFLINE_ACTION_REINSERT:
            full_msg = "%s taken offline. Reinsert the drive. SN:%s TLA:%s Rev:%s (%s) Reason:%s.\n";
            break;

    case ESP_ERROR_DRIVE_OFFLINE_ACTION_REPLACE:
            full_msg = "%s taken offline. Replace the drive. SN:%s TLA:%s Rev:%s (%s) Reason:%s.\n";
            break;

    case ESP_ERROR_DRIVE_OFFLINE_ACTION_ESCALATE:
            full_msg = "%s taken offline. Escalate to support. SN:%s TLA:%s Rev:%s (%s) Reason:%s.\n";
            break;

    case ESP_ERROR_DRIVE_OFFLINE_ACTION_UPGRD_FW:
            full_msg = "%s taken offline. Upgrade drive Firmware. SN:%s TLA:%s Rev:%s (%s) Reason:%s.\n";
            break;

    case ESP_ERROR_DRIVE_OFFLINE_ACTION_RELOCATE:
            full_msg = "%s taken offline. Replace vault drive with compatible drive. SN:%s TLA:%s Rev:%s (%s) Reason:%s.\n";
            break;

    case ESP_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED:
            full_msg = "%s Exceeded platform limit of %d slots. SN:%s TLA:%s Rev:%s. %s";
            break;

    case ESP_ERROR_MCU_HOST_RESET:
            full_msg = "%s has been reset. Reason: %s. Reset status code[%d]: 0x%x. The %s should be replaced. Please gather diagnostic materials and contact your service provider.\n";
            break;
            
    case ESP_ERROR_MCU_HOST_RESET_WITH_FRU_PART_NUM:
            full_msg = "%s has been reset. Reason: %s. Reset status code[%d]: 0x%x. The %s (Part Number: %s) should be replaced. Please gather diagnostic materials and contact your service provider.\n";
            break;

    case ESP_ERROR_SFP_FAULTED:
        full_msg = "SFP located in %s in slot %d port %d is faulted and should be replaced.\n";
        break;

    case ESP_ERROR_ENCL_SHUTDOWN_DETECTED:
        full_msg = "%s shutdown condition (%s) has been detected.\n";
        break;
        
    case ESP_ERROR_LCC_REMOVED:
        full_msg = "%s has been removed!\n";
        break;
        
    case ESP_ERROR_LCC_FAULTED:
        full_msg = "%s is faulted, SN: %s, PN: %s.\n";
        break;
        
    case ESP_ERROR_LCC_COMPONENT_FAULTED:
        full_msg = "%s is Faulted (LCC, Drive, or Connector)\n";
        break;

    case ESP_ERROR_SUITCASE_FAULT_DETECTED:
        full_msg = "%s Fault detected (%s).\n";
        break;

    case ESP_ERROR_ENCL_OVER_TEMP_DETECTED:
        full_msg = "%s is reporting over temperature failure.\n";
        break;

    case ESP_ERROR_ENCL_STATE_CHANGED:
         full_msg = "%s state changed from %s to %s.\n";
         break;

    case ESP_ERROR_DRIVE_PRICE_CLASS_FAILED:
         full_msg = "%s failed price class check. SN:%s TLA:%s Rev:%s.\n";
         break;

    case ESP_ERROR_ENCL_CROSS_CABLED:
         full_msg = "%s LCC Cables, between 2 enclosures or between the SP and the first enclosure, are crossed between the A and B sides. Please check the enclosure loop connection.\n";
         break;

    case ESP_ERROR_ENCL_BE_LOOP_MISCABLED:
         full_msg = "%s LCC Cables, between 2 enclosures or between the SP and the first enclosure, are not connected to the same loop. Please check the enclosure loop connection.\n";
         break;

    case ESP_ERROR_ENCL_LCC_INVALID_UID:
         full_msg = "%s invalid serial number detected, not able to validate the cabling.\n";
         break;

    case ESP_ERROR_ENCL_EXCEEDED_MAX:
         full_msg = "%s exceeds the limit for the drives and/or expanders on the bus.\n";
         break;

     case ESP_ERROR_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION:
         full_msg = "%s Predicted to exceed drive specified write endurance in %d days. Drive replacement recommended.\n";
        break;

    case ESP_ERROR_BMC_SHUTDOWN_WARNING_DETECTED:
        full_msg = "%s shutdown condition(%s) has been detected.\n";
        break;

    case ESP_ERROR_BMC_BIST_FAULT_DETECTED:
        full_msg = "%s BIST(Built In Self Test) Fault detected. Reason:%s, Action:%s.\n";
        break;

    case ESP_ERROR_PEER_SP_DOWN:
         full_msg = "Peer %s is Down.\n";
         break;
        
    case ESP_ERROR_PEER_SP_REMOVED:
         full_msg = "Peer %s has been Removed.\n";
         break;

    case ESP_ERROR_LOW_BATTERY_FAULT_DETECTED:
        full_msg = "%s CPU low battery fault detected.\n";
        break;

    case ESP_ERROR_DRIVE_ENHANCED_QUEUING_FAILED:
        full_msg = "%s failed enhanced queuing check. SN:%s TLA:%s Rev:%s.\n";
        break;

    case ESP_ERROR_BM_ECB_FAULT_DETECTED:
        full_msg = "%s power distribution control is not engaged. Only peer SP power is being supplied to the drives. Reseat the base module. If the fault persists, replace the base module.\n";
        break;

    case ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED:
        full_msg = "%s environmental interface failure detected. Reason: %s. Please gather diagnostic materials and contact your service provider.\n";
        break;


    case ESP_ERROR_ENCL_TYPE_NOT_SUPPORTED:
        full_msg = "%s Enclosure Type %s is not supported by this Platform.\n";
        break;
    
    case ESP_ERROR_DRIVE_BMS_OVER_MAX_ENTRIES:
        full_msg = "%s SN:%s BMS found max %d entries. Drive recommended for Proactive Spare.\n";
        break;
        
    case ESP_ERROR_DRIVE_BMS_LBB_OVER_THRESHOLD:
        full_msg = "%s SN:%s Lambda_BB for BMS is %d.  Drive recommended for Proactive Spare.\n";
        break;
    
    case ESP_ERROR_DRIVE_BMS_FOR_WORST_HEAD_OVER_WARN_THRESHOLD:
        full_msg = "%s SN:%s BMS found %d entries for worst head %d. Total entries: %d. Drive recommended for Proactive Spare.\n";
        break;

    case ESP_ERROR_RESUME_PROM_READ_FAILED:
        full_msg = "%s Resume Prom Read Failed. Status: %s.\n";
        break;
                        
    case ESP_ERROR_CACHE_CARD_CONFIGURATION_ERROR:
        full_msg = "%s configuration error (%s).\n";
        break;

    case ESP_ERROR_CACHE_CARD_FAULTED:
        full_msg = "%s is Faulted (%s).\n";
        break;
        
    case ESP_ERROR_CACHE_CARD_REMOVED:
        full_msg = "%s has been Removed.\n";
        break;

    case ESP_ERROR_DIMM_FAULTED:
        full_msg = "%s is Faulted.\n";
        break;

    case ESP_ERROR_DRIVE_SED_NOT_SUPPORTED_FAILED:
	full_msg = "%s SED drives not supported. SN:%s TLA:%s Rev:%s.\n";
	break;

    case ESP_ERROR_SSC_FAULTED:
        full_msg = "%s is faulted. Please replace the System Status Card.\n";
        break;

    case ESP_ERROR_SSC_REMOVED:
        full_msg = "%s has been Removed. Please insert the System Status Card.\n";
        break;
        
    case ESP_ERROR_IO_MODULE_UNSUPPORT:
        full_msg = "%s is not supported for this platform. Please remove the slic and replace it with a supported slic.\n";
        break;

    default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/****************************************************
*   end of fbe_event_log_esp_get_error_generic_msg()  
****************************************************/
/*!***************************************************************
 * @fn fbe_event_log_esp_get_crit_generic_msg(fbe_u32_t msg_id, 
 *                                            fbe_u8_t *msg,
 *                                            fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for ESP of
 *  ESP_CRIT_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Message length for buffer
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_event_log_esp_get_crit_generic_msg(fbe_u32_t msg_id, 
                                       fbe_u8_t *msg,
                                       fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new ESP_CRIT_GENERIC message 
     * that added in EspMessages.mc file, along with the appropriate format 
     * specifier.
     */
    switch(msg_id)
    {
        case ESP_CRIT_PEER_SP_POST_FAIL:
            full_msg = "PBL: %s is faulted. The fault cannot be isolated. status: %s(0x%x). Please call your service provider.\n";
            break;

        case ESP_CRIT_PEER_SP_POST_HUNG:
            full_msg = "PBL: %s is in a hung state. Last status: %s(0x%x).\n";
            break;

        case ESP_CRIT_PEER_SP_POST_FAIL_BLADE:
            full_msg = "PBL: %s is faulted. Fault Code: 0x%x. The SP and one or all of its components (CPU, I/O Module, I/O Carrier if applicable) has failed and should be replaced. Please contact your service provider.\n";
            break;

        case ESP_CRIT_PEER_SP_POST_FAIL_FRU_PART_NUM:
            full_msg = "PBL: %s is faulted. Fault Code: 0x%x. The Faulted FRU: %s - Part Number: %s should be replaced. Please contact your service provider.\n";
            break;
            
        case ESP_CRIT_PEERSP_POST_FAIL_FRU:
            full_msg = "PBL: %s is faulted. Recommend replacement of %s. Please contact your service provider.\n";
            break;
            
        case ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_NUM:
            full_msg = "PBL: %s is faulted. Recommend replacement of %s %d. Please contact your service provider.\n";
            break;
            
        case ESP_CRIT_PEERSP_POST_FAIL_FRU_PART_NUM:
            full_msg = "PBL: %s is faulted. Recommend replacement of %s - Part Number: %s. Please contact your service provider.\n";
            break;
            
        case ESP_CRIT_PEERSP_POST_FAIL_FRU_SLOT_PART_NUM:
            full_msg = "PBL: %s is faulted. Recommend replacement of %s %d - Part Number: %s. Please contact your service provider.\n";
            break;
            
        case ESP_CRIT_PEERSP_POST_FAIL:
            full_msg = "PBL: %s is faulted. %s. Please contact your service provider.\n";
            break;
                
        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/****************************************************
*   end of fbe_event_log_esp_get_crit_generic_msg()  
****************************************************/
