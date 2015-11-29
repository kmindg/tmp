/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_event_log_physical_package_msgs.c.c
 ***************************************************************************
 *
 * @brief
 *  This file contain event log message string for Physical Package.
 * 
 * @version
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_event_log_utils.h"


/****************************************************************************/
/*                         loacal functions                                 */
/****************************************************************************/
static fbe_status_t fbe_event_log_physical_get_info_generic_msg(fbe_u32_t msg_id, 
                                                                fbe_u8_t *msg,
                                                                fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_physical_get_warn_generic_msg(fbe_u32_t msg_id, 
                                                                fbe_u8_t *msg,
                                                                fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_physical_get_error_generic_msg(fbe_u32_t msg_id, 
                                                                 fbe_u8_t *msg,
                                                                 fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_physical_get_crit_generic_msg(fbe_u32_t msg_id, 
                                                                fbe_u8_t *msg,
                                                                fbe_u32_t msg_len);

/*!*******************************************************************
 * @fn fbe_event_log_get_physical_msg(fbe_u32_t msg_id, 
 *                                    fbe_u8_t *msg)
 *********************************************************************
 * @brief
 *  This function directs the appropriate function depend 
 *  upon Physical Package message severity type.
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
 ********************************************************************/
fbe_status_t 
fbe_event_log_get_physical_msg(fbe_u32_t msg_id, fbe_u8_t *msg, fbe_u32_t msg_len)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t mask_bit = 0xFFFFF000;

    if((msg_id & mask_bit) == PHYSICAL_PACKAGE_INFO_GENERIC)
    {
        status = fbe_event_log_physical_get_info_generic_msg (msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == PHYSICAL_PACKAGE_WARN_GENERIC)
    {
        status = fbe_event_log_physical_get_warn_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == PHYSICAL_PACKAGE_ERROR_GENERIC)
    {
        status = fbe_event_log_physical_get_error_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == PHYSICAL_PACKAGE_CRIT_GENERIC)
    {
        status = fbe_event_log_physical_get_crit_generic_msg(msg_id, msg, msg_len);
    }
    return status; 
}
/********************************************
*   end of fbe_event_log_get_physical_msg()  
*********************************************/

/*!*******************************************************************
 * @fn fbe_event_log_physical_get_info_generic_msg(fbe_u32_t msg_id, 
 *                                                 fbe_u8_t *msg,
 *                                                 fbe_u32_t msg_len)
 *********************************************************************
 * @brief
 *  This function return event log message string for Physical Package
 * of PHYSICAL_PACKAGE_INFO_GENERIC type.
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 *********************************************************************/
static fbe_status_t
fbe_event_log_physical_get_info_generic_msg(fbe_u32_t msg_id, 
                                            fbe_u8_t *msg,
                                            fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new PHYSICAL_PACKAGE_INFO_GENERIC
     * message that added in PhysicalPackageMessages.mc.mc file, along with the 
     * appropriate format specifier.
     */
     switch(msg_id)
    {
        case PHYSICAL_PACKAGE_INFO_LOAD_VERSION:
            full_msg ="Driver compiled on %s at %s.\n";
        break;

        case PHYSICAL_PACKAGE_INFO_SAS_SENSE:
            full_msg = "Bus %d Enclosure %d Disk %d CDB Check Condition. SN %s %s.\n";
        break; 

        case PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_STARTED:
            full_msg = "Drive firmware download started, TLA %s, new revision %s.\n";
        break; 

        case PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_COMPLETED:
            full_msg = "Drive firmware download completed, TLA %s, new revision %s.\n";
        break; 

        case PHYSICAL_PACKAGE_INFO_DRIVE_FW_DOWNLOAD_ABORTED:
            full_msg = "Drive firmware download aborted by user.\n";
        break;
        
        case PHYSICAL_PACKAGE_INFO_PDO_SENSE_DATA:
            full_msg = "%s %s %s.\n";
        break;
        
        case PHYSICAL_PACKAGE_INFO_PDO_SET_EOL:
            full_msg = "%s PDO has set EOL flag. SN:%s\n";
        break;
        
        case PHYSICAL_PACKAGE_INFO_PDO_SET_LINK_FAULT:
            full_msg = "%s PDO has set Link Fault flag. SN:%s\n";
        break;
        
        case PHYSICAL_PACKAGE_INFO_PDO_SET_DRIVE_FAULT:
            full_msg = "%s PDO has set Drive Fault flag. SN:%s\n";
        break;

        case PHYSICAL_PACKAGE_INFO_USER_INITIATED_DRIVE_POWER_CTRL_CMD:
            full_msg = "User initiated %s on %s.";
        break;

        case PHYSICAL_PACKAGE_INFO_HEALTH_CHECK_STATUS:
           full_msg = "%s Health Check Status: %s\n.";
        break;

       default:
         return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/*********************************************************
*   end of fbe_event_log_physical_get_info_generic_msg()  
**********************************************************/
/*!********************************************************************
 * @fn fbe_event_log_physical_get_warn_generic_msg(fbe_u32_t msg_id, 
                                                   fbe_u8_t *msg,
                                                   fbe_u32_t msg_len)
 **********************************************************************
 * @brief
 *  This function return event log message string for Physical Package
 * of PHYSICAL_PACKAGE_WARN_GENERIC type.
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 **********************************************************************/
static fbe_status_t
fbe_event_log_physical_get_warn_generic_msg(fbe_u32_t msg_id, 
                                            fbe_u8_t *msg,
                                            fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new PHYSICAL_PACKAGE_WARN_GENERIC
     * message that added in PhysicalPackageMessages.mc.mc file, along with the 
     * appropriate format specifier.
     */
    switch(msg_id)
    {
        
        case PHYSICAL_PACKAGE_WARN_SOFT_SCSI_BUS_ERROR:
            full_msg = "%s Soft SCSI bus error. DrvErrExtStat:0x%x %s %s.\n";
        break;

        case PHYSICAL_PACKAGE_WARN_SOFT_MEDIA_ERROR:
            full_msg = "%s Soft media error. DrvErrExtStat:0x%x %s %s.\n";
        break;

        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/*********************************************************
*   end of fbe_event_log_physical_get_warn_generic_msg()  
**********************************************************/
/*!***********************************************************************
 * @fn fbe_event_log_physical_get_error_generic_msg(fbe_u32_t msg_id, 
                                                    fbe_u8_t *msg,
                                                    fbe_u32_t msg_len)
 *************************************************************************
 * @brief
 *  This function return event log message string for Physical Package
 * of PHYSICAL_PACKAGE_ERROR_GENERIC type.
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 *************************************************************************/
static fbe_status_t
fbe_event_log_physical_get_error_generic_msg(fbe_u32_t msg_id, 
                                             fbe_u8_t *msg,
                                             fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new PHYSICAL_PACKAGE_ERROR_GENERIC
     * message that added in PhysicalPackageMessages.mc.mc file, along with the 
     * appropriate format specifier.
     */
    switch(msg_id)
    {
    
        /* This message string is added to test only*/
        case SAMPLE_TEST_MESSAGE:
            full_msg = SAMPLE_TEST_MESSAGE_STRING;
        break;

        case PHYSICAL_PACKAGE_ERROR_DRIVE_FW_DOWNLOAD_FAILED:
            full_msg = "Drive firmware download failed, TLA %s, new revision %s, reason %d.\n";
        break; 

        case PHYSICAL_PACKAGE_ERROR_RECOMMEND_DISK_REPLACEMENT:
            full_msg = "%s Recommend disk replacement. DrvErrExtStat:0x%x %s %s.\n";
        break;

        case PHYSICAL_PACKAGE_ERROR_PDO_SET_EOL:
            full_msg = "%s PDO has set EOL flag. SN:%s\n";
        break;

        case PHYSICAL_PACKAGE_ERROR_PDO_SET_DRIVE_KILL:
            full_msg = "%s PDO has set Drive Kill flag. SN:%s\n";
        break;

        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/*********************************************************
*   end of fbe_event_log_physical_get_error_generic_msg()  
**********************************************************/
/*!***********************************************************************
 * @fn fbe_event_log_physical_get_crit_generic_msg(fbe_u32_t msg_id, 
                                                   fbe_u8_t *msg,
                                                   fbe_u32_t msg_len)
 *************************************************************************
 * @brief
 *  This function return event log message string for Physical Package
 * of PHYSICAL_PACKAGE_CRIT_GENERIC type.
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 *************************************************************************/
static fbe_status_t
fbe_event_log_physical_get_crit_generic_msg(fbe_u32_t msg_id, 
                                            fbe_u8_t *msg, 
                                            fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new PHYSICAL_PACKAGE_CRIT_GENERIC
     * message that added in PhysicalPackageMessages.mc.mc file, along with the 
     * appropriate format specifier.
     */
    switch(msg_id)
    {
        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/*********************************************************
*   end of fbe_event_log_physical_get_error_generic_msg()  
**********************************************************/
