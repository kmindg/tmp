/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_event_log.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the code to log event log messages for the provision drive 
 *   object.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   11/18/2010:  Created. Amit Dhaduk
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_provision_drive_private.h"
#include "fbe/fbe_event_log_api.h"                 /*  for fbe_event_log_write */
#include "fbe/fbe_event_log_utils.h"               /*  for message codes */
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/
static fbe_status_t fbe_provision_drive_event_log_get_disk_info_for_position(
                                                    fbe_provision_drive_t * provision_drive_p,
                                                    fbe_u32_t*              bus_num_p,
                                                    fbe_u32_t*              encl_num_p,
                                                    fbe_u32_t*              slot_num_p);
                                        

static void fbe_provision_drive_notify_upper_layers_of_zero_start_or_complete(fbe_provision_drive_t *   provision_drive_p, fbe_object_id_t objectid, fbe_u32_t message_code);

/***************
 * FUNCTIONS
 ***************/

/*!****************************************************************************
 * fbe_provision_drive_event_log_disk_zero_start_or_complete 
 ******************************************************************************
 * @brief
 *   This function will log a message that a disk zeroing operation has started or 
 *   completed. 
 * 
 * @param provision_drive_p      - pointer to the provision drive
 * @param is_disk_zero_start     - TRUE indicates disk zerostart; FALSE indicates
 *                                 disk zero finished
 * @param packet_p               - pointer to a packet
 *
 * @return  fbe_status_t           
 *
 * @author
 *   11/18/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_event_log_disk_zero_start_or_complete(
                                        fbe_provision_drive_t *   provision_drive_p,
                                        fbe_bool_t                is_disk_zero_start,
                                        fbe_packet_t*             packet_p)
{


    fbe_u32_t                           bus;                         
    fbe_u32_t                           enclosure;                   
    fbe_u32_t                           slot;                        
    fbe_object_id_t                     object_id;                   
    fbe_u32_t                           message_code;                

    /*  Get the provision drive object id 
     */
    fbe_base_object_get_object_id((fbe_base_object_t*) provision_drive_p, &object_id);


    /*  Get the b-e-d of the drive that is being zeroed */
    fbe_provision_drive_event_log_get_disk_info_for_position(provision_drive_p,&bus, &enclosure, &slot);

    /*  Determine whether the message to log is a Disk Zero started or Disk Zero completed */
    if (is_disk_zero_start == FBE_TRUE)
    {
        message_code = SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED;
    }
    else
    {
        message_code = SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED;
    }    

    if(is_disk_zero_start == FBE_TRUE) {
	    provision_drive_p->bg_zeroing_log_status = FBE_TRUE;
    	    /*  Write the message to the event log  */
	    fbe_event_log_write(message_code, NULL, 0, "%d %d %d %x",bus, enclosure, slot, object_id);
    }
    else {	
	provision_drive_p->bg_zeroing_log_status = FBE_FALSE;
        /*  Write the message to the event log  */
	fbe_event_log_write(message_code, NULL, 0, "%d %d %d %x",bus, enclosure, slot, object_id);	

    }

	/*we also use the occasion to keep Admin in check since it's not polling and it has no update the status of the disk*/
	fbe_provision_drive_notify_upper_layers_of_zero_start_or_complete(provision_drive_p, object_id, message_code);

    /*  Return success  */
    return FBE_STATUS_OK;

} 
/******************************************************************************
 * end fbe_provision_drive_event_log_disk_zero_start_or_complete()
 ******************************************************************************/


/*!****************************************************************************
 *  fbe_provision_drive_event_log_get_disk_info_for_position  
 ******************************************************************************
 *
 * @brief
 *   This function is used to get the port, enclosure, and slot information 
 *   for a specific drive position 
 *
 * @param provision_drive_p     - pointer to the provision drive
 * 
 * @param bus_num_p         - pointer to data that gets populated with the bus 
 *                                number
 * @param encl_num_p        - pointer to data that gets populated with the 
 *                                enclosure number
 * @param slot_num_p        - pointer to data that gets populated with the slot 
 *                                number
 * 
 * @return  fbe_status_t 
 *
 * @author
 *   11/18/2010 - Created. Amit Dhaduk
 *   09/16/2011 - Modified. Vishnu Sharma
 *****************************************************************************/
static fbe_status_t fbe_provision_drive_event_log_get_disk_info_for_position(
                                                    fbe_provision_drive_t * provision_drive_p,
                                                    fbe_u32_t*              bus_num_p,
                                                    fbe_u32_t*              encl_num_p,
                                                    fbe_u32_t*              slot_num_p)

{
    fbe_provision_drive_nonpaged_metadata_drive_info_t  nonpaged_drive_info;
    fbe_status_t                                        status;
    /*  Initialize the output parameters */
    *bus_num_p      = FBE_PORT_NUMBER_INVALID;
    *encl_num_p     = FBE_ENCLOSURE_NUMBER_INVALID;
    *slot_num_p     = FBE_SLOT_NUMBER_INVALID;

    /* Get the nonpaged metadata drive information */
    status = fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &nonpaged_drive_info);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* copy nonpaged metadata drive information. */
    *bus_num_p  = nonpaged_drive_info.port_number;
    *encl_num_p = nonpaged_drive_info.enclosure_number;
    *slot_num_p = nonpaged_drive_info.slot_number;

    /*  Return success  */
    return FBE_STATUS_OK;

} 
/******************************************************************************
 * end fbe_provision_drive_event_log_get_disk_info_for_position()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_notify_upper_layers_of_zero_start_or_complete  
 ******************************************************************************
 *
 * @brief
 *   Let users outside of MCR know that zero started or completed
 * @return  fbe_status_t 
 *
 *****************************************************************************/
static void fbe_provision_drive_notify_upper_layers_of_zero_start_or_complete(fbe_provision_drive_t *   provision_drive_p, fbe_object_id_t objectid, fbe_u32_t message_code)
{
	fbe_notification_info_t					notification_info;
	fbe_provision_drive_clustered_flags_t	flag_to_set, flag_to_clear;

	/*set up the notificaiton*/
	notification_info.notification_type = FBE_NOTIFICATION_TYPE_ZEROING;
	notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
	notification_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;

	if (message_code == SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED) {
		notification_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_STARTED;
		flag_to_set = FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_STARTED;
		flag_to_clear = FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_ENDED;
		if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_ENDED)) {
			fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_ENDED);/*if we were the passive one and became active, we need to clear this too*/
		}
	}else if (message_code == SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED){
		notification_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_ENDED;
		flag_to_set = FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_ENDED;
        if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED)) {
            notification_info.notification_data.object_zero_notification.scrub_complete_to_be_set = FBE_TRUE;
            flag_to_set |= FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SCRUB_ENDED;
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Scrubbing is complete.  Generate notification.\n");
        }
        else {
            notification_info.notification_data.object_zero_notification.scrub_complete_to_be_set = FBE_FALSE;
        }
		
		flag_to_clear = FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_STARTED;
		if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED)) {
			fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED);/*if we were the passive one and became active, we need to clear this too*/
		}
	}

	/*and pop the notification*/
	fbe_notification_send(objectid, notification_info);

	/*since we get here only on the active side, we need to let the peer know it needs to send notification*/
	fbe_provision_drive_lock(provision_drive_p);
	fbe_provision_drive_clear_clustered_flag(provision_drive_p, flag_to_clear);
	fbe_provision_drive_set_clustered_flag(provision_drive_p, flag_to_set);
    fbe_provision_drive_unlock(provision_drive_p);

}
/******************************************************************************
 * end fbe_provision_drive_notify_upper_layers_of_zero_start_or_complete()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_write_event_log 
 ******************************************************************************
 * @brief
 *   This function will log a message for provision drive. 
 * 
 * @param provision_drive_p      - pointer to the provision drive
 * @param event_code             - event code
 *
 * @return  fbe_status_t           
 *
 * @author
 *   10/03/2010 - Created. Lili Chen
 *   10/31/2012 - Updated by adding EOL/DRIVE FAULT event log. Wenxuan Yin
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_write_event_log(fbe_provision_drive_t *   provision_drive_p,
                                                 fbe_u32_t event_code)
{
    fbe_u32_t                           bus;                         
    fbe_u32_t                           enclosure;                   
    fbe_u32_t                           slot;                        
    fbe_object_id_t                     object_id;                   
    fbe_u8_t                            *serial_num = NULL;

    /*  Get the provision drive object id */
    fbe_base_object_get_object_id((fbe_base_object_t*) provision_drive_p, &object_id);

    /*  Get the b-e-d of the drive */
    fbe_provision_drive_event_log_get_disk_info_for_position(provision_drive_p,&bus, &enclosure, &slot);

    /*  Get serial number */
    fbe_provision_drive_get_serial_num(provision_drive_p, &serial_num);

    /*  Write the message to the event log  */
    switch(event_code)
    {
        case SEP_INFO_SYSTEM_DISK_END_OF_LIFE_CLEAR:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %s",
                                slot, serial_num);
            break;

        case SEP_INFO_USER_DISK_END_OF_LIFE_CLEAR:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %d %d %s",
                                bus, enclosure, slot, serial_num);
            break;	

        case SEP_INFO_SYSTEM_DISK_DRIVE_FAULT_CLEAR:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %s",
                                slot, serial_num);
            break;

        case SEP_INFO_USER_DISK_DRIVE_FAULT_CLEAR:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %d %d %s",
                                bus, enclosure, slot, serial_num);
            break;
			
        case SEP_WARN_SYSTEM_DISK_NEED_REPLACEMENT:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %s",
                                slot, serial_num);
            break;

        case SEP_WARN_USER_DISK_END_OF_LIFE:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %d %d %s",
                                bus, enclosure, slot, serial_num);
            break;

        case SEP_WARN_SYSTEM_DISK_DRIVE_FAULT:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %s",
                                slot, serial_num);
            break;

        case SEP_WARN_USER_DISK_DRIVE_FAULT:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %d %d %s",
                                bus, enclosure, slot, serial_num);
            break;

        case SEP_ERROR_CODE_PROVISION_DRIVE_INCORRECT_KEY:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %d %d %x %s", 
                                bus, enclosure, slot, object_id, serial_num);
            break;

        case SEP_ERROR_CODE_PROVISION_DRIVE_WEAR_LEVELING_THRESHOLD:
            fbe_event_log_write(event_code, NULL, 0, 
                                "%d %d %d %x %s", 
                                bus, enclosure, slot, object_id, serial_num);
            break;

        default:
            /* Unsupported event code */
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: write event: 0x%08X unsupported. \n",
                                  event_code);
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /*  Return success  */
    return FBE_STATUS_OK;
} 
/******************************************************************************
 * end fbe_provision_drive_write_event_log()
 ******************************************************************************/

/******************************************
 * end fbe_provision_drive_event_log.c
 *****************************************/
