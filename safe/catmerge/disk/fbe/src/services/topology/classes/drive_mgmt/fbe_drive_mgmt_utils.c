#include <stdlib.h>    /* todo:  can we include standard c libs, or use CSX?*/
#include "fbe/fbe_types.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_esp.h"
#include "fbe_drive_mgmt_private.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_event_log_api.h"
#include "EspMessages.h"
#include "fbe/fbe_api_enclosure_interface.h"

/********* Local Structures **********/


typedef struct dmo_drive_death_action_tuple_s
{
    fbe_u32_t                death_reason;
    dmo_drive_death_action_t action;

} dmo_drive_death_action_tuple_t;


dmo_drive_death_action_tuple_t dmo_drive_death_action_map[] =
{
    {FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED,               DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED,        DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED,          DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED,     DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED,         DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED,      DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_THRESHOLD_EXCEEDED,   DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_DATA_THRESHOLD_EXCEEDED,          DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR,                      DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ACTIVATE_TIMER_EXPIRED,           DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED,          DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY,                 DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN,              DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_PRICE_CLASS_MISMATCH,             DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED,   DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PYHSICAL_DRIVE_DEATH_REASON_EXCEEDS_PLATFORM_LIMITS,          DMO_DRIVE_DEATH_ACTION_PLATFORM_LIMIT_EXCEEDED}, 
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION,      DMO_DRIVE_DEATH_ACTION_RELOCATE},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_ID,                       DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},

    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS,                DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED,               DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO,            DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_MODE_SENSE,      DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_READ_CAPACITY,   DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN,                DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DRIVE_NOT_SPINNING,                DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN,               DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT,                     DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED,               DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},

    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE,                DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DC_FLUSH_FAILED,                   DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_FAILED,                DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_FAILED,                 DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},

    {FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_INVALID_INSCRIPTION,              DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_PIO_MODE_UNSUPPORTED,             DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_UDMA_MODE_UNSUPPORTED,            DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO,           DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},

    {DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED,                                DMO_DRIVE_DEATH_ACTION_REINSERT_DRIVE},
    {DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE,                              DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},  
    {DMO_DRIVE_OFFLINE_REASON_PERSISTED_EOL,                                DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE}, 
    {DMO_DRIVE_OFFLINE_REASON_PERSISTED_DF,                                 DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER}, 

    {FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED,               DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_DISCOVERED_DEATH_REASON_HARDWARE_NOT_READY,                   DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},

    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_MULTI_BIT,                    DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_SINGLE_BIT,                   DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_OTHER,                        DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER}, 
    {DMO_DRIVE_OFFLINE_REASON_NON_EQ,                                       DMO_DRIVE_DEATH_ACTION_UPGRADE_FW},
    {DMO_DRIVE_OFFLINE_REASON_INVALID_ID,                                   DMO_DRIVE_DEATH_ACTION_UPGRADE_FW},
    {DMO_DRIVE_OFFLINE_REASON_SSD_LE,                                       DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {DMO_DRIVE_OFFLINE_REASON_SSD_RI,                                       DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {DMO_DRIVE_OFFLINE_REASON_HDD_520,                                      DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SED_DRIVE_NOT_SUPPORTED,	    DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER}, 

    /*add new stuff here*/

    {-1, -1},    // must be last
};



/**** Local/Private Function Prototypes ****/

static char* dmo_drive_death_action_to_string(const fbe_drive_mgmt_t *drive_mgmt, dmo_drive_death_action_t action);




//TODO: Make this more flexible
/**************************************************************************
 *      fbe_drive_mgmt_ntmirror_mirror_drive()
 **************************************************************************
 *
 *  DESCRIPTION:
 *    fbe_drive_mgmt_mirror_drive - Returns mirror drive
 *
 *  PARAMETERS:
 *    slot:  Location of an NtMirror drive
 *
 *  RETURN VALUES/ERRORS:
 *    slot
 *
 *  NOTES:
 *    Currently NtMirror drives only exist on Bus 0, Encl 0, Slot 0-3.  Only
 *    slot is passed in at this time.  Some day this may change.
 * 
 *  HISTORY:
 *      18-Aug-2010    Wayne Garrett  Created.
 *
 **************************************************************************/
fbe_u8_t   fbe_drive_mgmt_ntmirror_mirror_drive(fbe_u8_t slot)
{
    const fbe_u8_t mirror[4] = {2,3,0,1};   //hardcode for mirror drives 0,2 and 1,3

    return mirror[slot];
}


fbe_u32_t    
fbe_drive_mgmt_bes_to_fru_num(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot)
{
    //TODO: cleanup when bes2fru tested.

    const int max_drives_per_encl = 15;  /*TODO: are there const already defined for these?  
                                          They will change with newer enclosures */
    const int max_encl_per_bus = 8;
    const int max_drives_per_bus = max_drives_per_encl * max_encl_per_bus;

    return  bus*max_drives_per_bus + enclosure*max_drives_per_encl + slot;
}

/*!**************************************************************
 * fbe_drive_mgmt_send_cmi_message()
 ****************************************************************
 * @brief
 *  This function sends the specified Drive Mgmt message to the
 *  peer SP.
 *
 * @param TBD
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_base_environment_send_cmi_message()
 *
 * @notes Since CMI always returns FBE_STATUS_OK, this function
 *  will do the same if a valid CMI msg is passed in.  When there is an 
 *  error, the cmi callback will have the event code that indicates 
 *  the status.  If an invalid CMI msg is passed in, then 
 *  FBE_STATUS_INVALID will be returned. This should be considered 
 *  a software bug.      
 *
 * @author
 *  13-Sep-2010:  Created. Wayne Garrett
 *
 *
 ****************************************************************/
fbe_status_t
fbe_drive_mgmt_send_cmi_message(const fbe_drive_mgmt_t *drive_mgmt, 
                                fbe_u8_t bus, fbe_u8_t enclosure, fbe_u8_t slot,
                                fbe_drive_mgmt_cmi_msg_type_t msg_type)
{
    fbe_status_t            cmi_status = FBE_STATUS_INVALID;
    fbe_u32_t               msg_length = 0;
    fbe_drive_mgmt_cmi_msg_t dmoMsg = {0};

    //TODO: Make sure callers are not holding locks.  cmi uses other drivers, so we cannot
    // guarantee response time.  This will involve changing the locking strategy to be more fine grained.


    switch(msg_type)
    {

    /* Add CMI messages here */
    case FBE_DRIVE_MGMT_CMI_MSG_DEBUG_REMOVE:
    case FBE_DRIVE_MGMT_CMI_MSG_DEBUG_INSERT:
        dmoMsg.type      = msg_type;
        dmoMsg.info.bus  = bus;
        dmoMsg.info.encl = enclosure;
        dmoMsg.info.slot = slot;
        cmi_status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)drive_mgmt,
                                                       sizeof(fbe_drive_mgmt_cmi_msg_t),
                                                       &dmoMsg);
        msg_length = sizeof(fbe_drive_mgmt_cmi_msg_t);
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s NOT IMPLEMENTED message %d\n",
                              __FUNCTION__, msg_type);
        msg_length = 0;  // indicates failure
        break;
    }


    if (msg_length > 0)   //safety checking
    {
        if (cmi_status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: %s sent message %d to peer\n",
                                  __FUNCTION__, msg_type);
        }        
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "DMO: %s Error sending message %d to peer, status: 0x%X\n",
                                  __FUNCTION__, msg_type, cmi_status);
        }       
    }

    return cmi_status;
}
/******************************************************
 * end fbe_drive_mgmt_send_cmi_messge() 
 ******************************************************/


/*!**************************************************************
 * fbe_drive_mgmt_process_cmi_message()
 ****************************************************************
 * @brief
 *  This function processes a cmi message sent from the 
 *  peer SP.
 *
 * @param TBD
 *
 * @return void
 *
 * @notes   This routine runs in the context of ESP.  It's safe
 *          to send a CMI response from this context.
 *
 * @author
 *  13-Sep-2010:  Created. Wayne Garrett
 *
 ****************************************************************/
void fbe_drive_mgmt_process_cmi_message(fbe_drive_mgmt_t * drive_mgmt, fbe_drive_mgmt_cmi_msg_t * base_msg)
{    
    fbe_drive_info_t                    *pDriveInfo = NULL;
    fbe_u32_t                           i;
    fbe_base_object_mgmt_drv_dbg_action_t action = FBE_DRIVE_ACTION_NONE;
    fbe_base_object_mgmt_drv_dbg_ctrl_t debugControl;
    fbe_object_id_t                     parentObjId = FBE_OBJECT_ID_INVALID;
    fbe_status_t                        status;

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %s received message %d from peer\n",
                          __FUNCTION__, base_msg->type);

    switch(base_msg->type)
    {
        /* Add cmi msgs here */
    case FBE_DRIVE_MGMT_CMI_MSG_DEBUG_REMOVE:
    case FBE_DRIVE_MGMT_CMI_MSG_DEBUG_INSERT:
        action = (base_msg->type == FBE_DRIVE_MGMT_CMI_MSG_DEBUG_REMOVE)?FBE_DRIVE_ACTION_REMOVE:FBE_DRIVE_ACTION_INSERT;

        dmo_drive_array_lock(drive_mgmt,__FUNCTION__);
        pDriveInfo = drive_mgmt->drive_info_array;
        for (i = 0; i < drive_mgmt->max_supported_drives; i++, pDriveInfo++)
        {
            if(pDriveInfo->parent_object_id != FBE_OBJECT_ID_INVALID)
            {
                if((pDriveInfo->location.bus == base_msg->info.bus) &&
                    (pDriveInfo->location.enclosure == base_msg->info.encl) &&
                    (pDriveInfo->location.slot == base_msg->info.slot))
                {
                    parentObjId = pDriveInfo->parent_object_id;
                    pDriveInfo->dbg_remove = (action == FBE_DRIVE_ACTION_REMOVE)?TRUE:FALSE;
                    break;
                }
            }
        }
        dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
        if (parentObjId != FBE_OBJECT_ID_INVALID)
        {
            fbe_zero_memory(&debugControl, sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t));
            debugControl.driveDebugAction[base_msg->info.slot] = action;
            debugControl.driveCount = drive_mgmt->max_supported_drives;
            status = fbe_api_enclosure_send_drive_debug_control(parentObjId, &debugControl);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)drive_mgmt,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "peer request: Drive:%d_%d_%d: simulate physical %s, sending cmd to PhyPkg failed, status 0x%X.\n",
                                        base_msg->info.bus, base_msg->info.encl, base_msg->info.slot,
                                        (action == FBE_DRIVE_ACTION_REMOVE) ? "REMOVAL":"INSERTION",
                                        status);
            }
        }
        break;
        default:
            fbe_base_environment_cmi_process_received_message((fbe_base_environment_t *)drive_mgmt, 
                                                                (fbe_base_environment_cmi_message_t *) base_msg);
        break;
    }

    return;
}
/******************************************************
 * end fbe_drive_mgmt_process_cmi_message() 
 ******************************************************/


/*!**************************************************************
 * fbe_drive_mgmt_process_cmi_peer_not_present()
 ****************************************************************
 * @brief
 *  This function processes a cmi event indicating the peer
 *  is not present
 *
 * @param TBD
 *
 * @return void
 *
 * @author
 *  21-Sep-2010:  Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t                 
fbe_drive_mgmt_process_cmi_peer_not_present(fbe_drive_mgmt_t * drive_mgmt, fbe_drive_mgmt_cmi_msg_t * base_msg)
{
    fbe_status_t status = FBE_STATUS_MORE_PROCESSING_REQUIRED;    
    switch (base_msg->type)
    {
        /* add new cmi messages here */
    default:
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        break;
    }
    return status;
}
/******************************************************
 * end fbe_drive_mgmt_process_cmi_peer_not_present() 
 ******************************************************/


/**************************************************************************
 *      fbe_drive_mgmt_death_reason_desc()
 **************************************************************************
 *
 *  @brief
 *    This function return string description for fru death reason.
 *
 *  @param
 *    death_reason - death reason value set by set_death_reason.
 * 
 *  @return
 *     char * - Retrun string for death reason.
 *
 *  @author
 *     10/29/2010  Created.  Wayne Garrett
 *
 **************************************************************************/
char* 
fbe_drive_mgmt_death_reason_desc(fbe_u32_t death_reason)
{
    char *death_reason_desc;

    /*Due to string size limit for eventlog, specially for "TAKEN OFFLINE" event,
      death reason string should not go more than 8 char.(AR#593320)*/

    if (death_reason == FBE_DEATH_REASON_INVALID) 
    { 
        death_reason_desc = "Not set";
    }
    else if ( death_reason == DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED)
    {
        death_reason_desc = "Removed";
    }
    else if ( death_reason == FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY)
    {
        death_reason_desc = "Capacity";
    }
    else if (death_reason == FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_PRICE_CLASS_MISMATCH)
    {
        death_reason_desc = "PriceCls";
    }
    else if (death_reason == FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED)
    {
        death_reason_desc = "Non-EQ";
    }
    else if (death_reason == FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_ID)
    {
        death_reason_desc = "WrongID";
    } 
    else if (death_reason == FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED) 
    {
        death_reason_desc = "Upgrd FW";
    }
    else if (death_reason == FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION)
    {
        death_reason_desc = "MisMatch";
    }
    else if (death_reason == FBE_BASE_PYHSICAL_DRIVE_DEATH_REASON_EXCEEDS_PLATFORM_LIMITS)
    {
        death_reason_desc = "Limits";
    }
    else if ( (death_reason > FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID &&        // fbe_base_physical_drive_death_reason_t
               death_reason < FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LAST)         ||
              (death_reason > FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_INVALID &&         // fbe_physical_drive_death_reason_t
               death_reason < FBE_PHYSICAL_DRIVE_DEATH_REASON_LAST))
    {
        death_reason_desc = "By PDO";
    }
    else if ( death_reason == DMO_DRIVE_OFFLINE_REASON_PERSISTED_EOL)
    {
        death_reason_desc = "EOL set";
    }
    else if ( death_reason == DMO_DRIVE_OFFLINE_REASON_PERSISTED_DF)
    {
        death_reason_desc = "DF set";
    }
    else if ( death_reason == DMO_DRIVE_OFFLINE_REASON_NON_EQ)
    {
        death_reason_desc = "Non EQ";
    }
    else if (death_reason == DMO_DRIVE_OFFLINE_REASON_INVALID_ID)
    {
        death_reason_desc = "WrongID";
    } 
    else if (death_reason == DMO_DRIVE_OFFLINE_REASON_SSD_LE)
    {
        death_reason_desc = "SSD LE";
    } 
    else if (death_reason == DMO_DRIVE_OFFLINE_REASON_SSD_RI)
    {
        death_reason_desc = "SSD RI";
    } 
    else if (death_reason == DMO_DRIVE_OFFLINE_REASON_HDD_520)
    {
        death_reason_desc = "HDD 520";
    } 
    else if (death_reason == DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE) 
    {
        death_reason_desc = "LO";
    }
    else if ( death_reason == FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED)
    {
        death_reason_desc = "Expired";
    }
    else if ( death_reason == FBE_BASE_DISCOVERED_DEATH_REASON_HARDWARE_NOT_READY)
    {
        death_reason_desc = "NotReady";
    }
	else if (death_reason == FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SED_DRIVE_NOT_SUPPORTED)
	{
		death_reason_desc = "SED drive";
	}
    else
    {
        death_reason_desc = "Unknown";
    }

    return death_reason_desc;
}
/******************************************************
 * end fbe_drive_mgmt_death_reason_desc() 
 ******************************************************/


/**************************************************************************
 *      fbe_drive_mgmt_drive_failed_action()
 **************************************************************************
 *
 *  @brief
 *    This function return action field for ESP_ERROR_DRIVE_OFFLINE
 *    error message.
 *
 *  @param
 *    death_reason - death reason value from DRIVE_DEAD_REASON enum.
 * 
 *  @return
 *     char * - Retrun string for death reason.
 *
 *  @author
 *     10/29/2010 - Created.  Wayne Garrett
 *
 **************************************************************************/
dmo_drive_death_action_t
fbe_drive_mgmt_drive_failed_action(const fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t death_reason)
{
    dmo_drive_death_action_tuple_t* map_index = dmo_drive_death_action_map;
    dmo_drive_death_action_t action = DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER;
    fbe_bool_t death_reason_found = FBE_FALSE;

    while (map_index->death_reason != (fbe_u32_t)-1)
    {
        if (map_index->death_reason == death_reason)
        {
            action = map_index->action;
            death_reason_found = FBE_TRUE;
            break;
        }

        map_index++;
    }
    
    if (!death_reason_found)
    {
        action = DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER;

        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                      "drive_mgmt_drive_fail_action Death reason 0x%X not found in death action map.\n",
                      death_reason);
    }
    
    return action;
}
/******************************************************
 * end fbe_drive_mgmt_drive_failed_action() 
 ******************************************************/

/**************************************************************************
 *      dmo_drive_death_action_to_string()
 **************************************************************************
 *
 *  @brief
 *    This function return a string description for the drive death action.
 *
 *  @param
 *    action - Action to be performed for death a drive.
 * 
 *  @return
 *     char * - Retrun string for action.
 *
 *  @author
 *     10/29/2010 - Created.  Wayne Garrett
 *
 **************************************************************************/
static char* 
dmo_drive_death_action_to_string(const fbe_drive_mgmt_t *drive_mgmt, dmo_drive_death_action_t action)
{
    char* action_str;

    /*Due to string size limit for eventlog, specially for "TAKEN OFFLINE" event,
      action string should not go more than 8 char.(AR#593320)*/

    switch(action)
    {
        case DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE:
            action_str = "Replace";
            break;
        case DMO_DRIVE_DEATH_ACTION_REINSERT_DRIVE:
            action_str = "Reinsert";
            break;
        case DMO_DRIVE_DEATH_ACTION_UPGRADE_FW:
            action_str = "Upgrd FW";
            break;
        case DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER:
            action_str = "Escalate";
            break;
        case DMO_DRIVE_DEATH_ACTION_RELOCATE:
            action_str = "Relocate";
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %s Action %d not found. Please implement. Defaulting to Escalate\n",
                          __FUNCTION__, action);
            action_str = "Escalate";
            break;
    }
    return action_str;
}
/******************************************************
 * end dmo_drive_death_action_to_string() 
 ******************************************************/
/**************************************************************************
 *      fbe_drive_mgmt_log_drive_offline_event()
 **************************************************************************
 *
 *  @brief
 *    This function logs offline event based on action.
 *
 *  @param
 *    reason - death reason.
 *    action - action depends upon death reason.
 *    drive_info - pointer to drive info
 *    phys_location - pointer to phy location structure
 * 
 *  @return
 *     None
 *
 *  @author
 *     02/20/2014 - Created.  Dipak Patel
 *
 **************************************************************************/
void
fbe_drive_mgmt_log_drive_offline_event(const fbe_drive_mgmt_t *drive_mgmt, 
                                       dmo_drive_death_action_t action,
                                       fbe_physical_drive_information_t * drive_info,
                                       fbe_u32_t reason,
                                       fbe_device_physical_location_t * phys_location)
{

    char deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    char reasonStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_status_t status = FBE_STATUS_OK;

    csx_p_snprintf(reasonStr, sizeof(reasonStr), "0x%x", reason);

    /* generate default the device prefix string (used for tracing & events) */
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE,        
                                               phys_location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: log_drive_offline_event: failed to create_device_string for drive:%d_%d_%d,status: 0x%X\n",
                              phys_location->bus,phys_location->enclosure,phys_location->slot,status);
    }

    switch(action)
    {
        case DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE:
          fbe_event_log_write(ESP_ERROR_DRIVE_OFFLINE_ACTION_REPLACE, NULL, 0, "%s %s %s %s %s %s",
                      &deviceStr[0], drive_info->product_serial_num, drive_info->tla_part_number, drive_info->product_rev, 
                      reasonStr, fbe_drive_mgmt_death_reason_desc(reason));
        break;

        case DMO_DRIVE_DEATH_ACTION_REINSERT_DRIVE:
          fbe_event_log_write(ESP_ERROR_DRIVE_OFFLINE_ACTION_REINSERT, NULL, 0, "%s %s %s %s %s %s",
                      &deviceStr[0], drive_info->product_serial_num, drive_info->tla_part_number, drive_info->product_rev, 
                      reasonStr, fbe_drive_mgmt_death_reason_desc(reason));
        break;

        case DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER:
          fbe_event_log_write(ESP_ERROR_DRIVE_OFFLINE_ACTION_ESCALATE, NULL, 0, "%s %s %s %s %s %s",
                      &deviceStr[0], drive_info->product_serial_num, drive_info->tla_part_number, drive_info->product_rev, 
                      reasonStr, fbe_drive_mgmt_death_reason_desc(reason));
        break;

        case DMO_DRIVE_DEATH_ACTION_UPGRADE_FW:
          fbe_event_log_write(ESP_ERROR_DRIVE_OFFLINE_ACTION_UPGRD_FW, NULL, 0, "%s %s %s %s %s %s",
                      &deviceStr[0], drive_info->product_serial_num, drive_info->tla_part_number, drive_info->product_rev, 
                      reasonStr, fbe_drive_mgmt_death_reason_desc(reason));
        break;

        case DMO_DRIVE_DEATH_ACTION_RELOCATE:
          fbe_event_log_write(ESP_ERROR_DRIVE_OFFLINE_ACTION_RELOCATE, NULL, 0, "%s %s %s %s %s %s",
                      &deviceStr[0], drive_info->product_serial_num, drive_info->tla_part_number, drive_info->product_rev, 
                      reasonStr, fbe_drive_mgmt_death_reason_desc(reason));
        break;

        case DMO_DRIVE_DEATH_ACTION_PLATFORM_LIMIT_EXCEEDED:
          fbe_event_log_write(ESP_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED, NULL, 0, "%s %d %s %s %s %s",
                      &deviceStr[0], drive_mgmt->max_supported_drives, drive_info->product_serial_num, drive_info->tla_part_number, drive_info->product_rev, 
                      "");
        break;

        default:
          fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "DMO: %s Action %d not found. Please implement\n",__FUNCTION__, action);
        break;
  }
  return;
}
/******************************************************
 * end fbe_drive_mgmt_log_drive_offline_event() 
 ******************************************************/


/*TODO:  We should import this from libutil library, or somewhere else */
/*************************************************************************
 *                              atoh ()                                  *
 *************************************************************************
 *
 *  @brief
 *    This function converts an ASCII string into a hex number
 *
 *  @param
 *     string (input) - pointer to ASCII string
 * 
 *  @return
 *  RETURN VALUES:
 *     Result of conversion (-1 = illegal string)
 * 
 *  @note
 *  NOTES:
 *     This function only allows up to 8 digit strings
 *
 *  @author
 *    3/20/2012  Wayne Garrett  -- copied from libutil/string.c/atoh()
 *
 *************************************************************************/

fbe_u32_t dmo_atoh(char * s_p)
{

/*********************************
 **    VARIABLE DECLARATIONS    **
 *********************************/

    fbe_u32_t fvar;
    size_t length;
    fbe_u32_t result;
    fbe_u32_t temp;

/*****************
 **    BEGIN    **
 *****************/

    /*
     * First, check the length.  If it's greater than 8, return the error code.
     */
    if (((length = strlen(s_p)) > 8) || (length == 0))
    {
        return DMO_MAX_UINT_32;
    }

    /* Now, convert the string */
    result = 0;

    for (fvar = 0; fvar < length; fvar++)
    {
        /*  Shift the result to the next power of 16.  */
        result = result << 4;

        temp = s_p[fvar];

        if ((temp >= '0') && (temp <= '9'))
        {
            result += (temp - '0');
        }
        else if ((temp >= 'a') && (temp <= 'f'))
        {
            result += (0xA + (temp - 'a'));
        }
        else if ((temp >= 'A') && (temp <= 'F'))
        {
            result += (0xA + (temp - 'A'));
        }
        else
        {
            result = DMO_MAX_UINT_32;
            break;
        }
    }                           /*  End of "for..." loop.  */

    /* That's it.  Return the result. */
    return (result);
}


/*TODO:  is this already implemented somewhere else?*/
fbe_u32_t dmo_str2int(const char *src)
{
    char * src_str = (char *)src;    /*todo: hack to remove const.  make a copy*/
    char * temp_str_ptr;
    char * saveptr;
    fbe_u32_t intval, src_len;    

    if(src_str == NULL)
    {
        return DMO_MAX_UINT_32;
    }

    if(strstr(src_str, "0x") == NULL)
    {
        /* Make sure there are no alpha characters. 
         */
        src_len = (UINT_32)strlen(src_str);
        while(src_len)
        {
            src_len--;
            /* 48 is ASCII notation for 0 and 57 is for ASCII notation for
             * 9.
             */
            if( (src_str[src_len] < 48 ) || (src_str[src_len] > 57))
            {
                return DMO_MAX_UINT_32;
            }
        }
        /* This is a decimal value.
         */
        intval = atoi(src_str);
    }
    else
    {
        /* This is a hex value.
         */
        csx_p_strtok_r(src_str, "x", &saveptr);
        temp_str_ptr = csx_p_strtok_r(CSX_NULL, "x", &saveptr);
        if(temp_str_ptr == NULL)
        {
            return DMO_MAX_UINT_32;
        }
        else
        {
            intval = dmo_atoh(temp_str_ptr);
        }
        intval = dmo_atoh(temp_str_ptr);
    }
    return intval;

}


/*!**************************************************************
 * @fn fbe_drive_mgmt_get_logical_error_action
 ****************************************************************
 * @brief
 *  Gets the default logical crc error actions.
 * 
 * @param drive_mgmt - pointer to DMO object
 * @param actions - returns logical error actions
 * 
 * @return none
 *
 * @version
 *   03/15/2012:  Wayne Garrett - Created.
 *
 ****************************************************************/
void fbe_drive_mgmt_get_logical_error_action(fbe_drive_mgmt_t * drive_mgmt, fbe_pdo_logical_error_action_t * actions)
{
    fbe_status_t status;
    fbe_u32_t defaultKey = FBE_LOGICAL_ERROR_ACTION_DEFAULT;
    fbe_u32_t key;

    status = fbe_registry_read(NULL,
                               fbe_get_registry_path(),
                               FBE_LOGICAL_ERROR_ACTION_REG_KEY, 
                               &key,
                               sizeof(key),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaultKey,   // default 
                               0,                                     
                               FALSE);

    *actions = key;

    if (FBE_STATUS_OK != status)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: fbe_registry_read %s failed, status=%d, defaulting action to 0x%x\n", 
                              FBE_LOGICAL_ERROR_ACTION_REG_KEY, status, defaultKey);

        *actions = defaultKey;
    }
}






/*!**************************************************************
 * dmo_physical_drive_information_init()
 ****************************************************************
 * @brief
 *  Initialize the fbe_physical_drive_information_t struct.
 *  
 *
 * @param p_drive_info - Struct to be inialized.
 *
 * @return none
 * 
 * @author
 *  07/09/2012:  Wayne Garrett - created.
 *
 ****************************************************************/
void dmo_physical_drive_information_init(fbe_physical_drive_information_t *p_drive_info)
{
    fbe_zero_memory(p_drive_info, sizeof(fbe_physical_drive_information_t));
    fbe_copy_memory(p_drive_info->product_rev, "unknown", FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
    fbe_copy_memory(p_drive_info->product_serial_num, "unknown", FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);   
    fbe_copy_memory(p_drive_info->tla_part_number, "unknown", FBE_SCSI_INQUIRY_TLA_SIZE);
}


/*!**************************************************************
 * @fn fbe_drive_mgmt_get_fuel_gauge_poll_interval_from_registry
 ****************************************************************
 * @brief
 *  Gets the fuel gauge and SMART data minimum poll interval from registry.
 * 
 * @param drive_mgmt - pointer to DMO object
 * @param actions - returns minimum poll interval
 * 
 * @return none
 *
 * @version
 *   09/14/2012:  Christina Chiang - Created.
 *
 ****************************************************************/
void fbe_drive_mgmt_get_fuel_gauge_poll_interval_from_registry(fbe_drive_mgmt_t * drive_mgmt, fbe_u32_t *fuel_gauge_poll_interval)
{
    fbe_status_t status;
    fbe_u32_t defaultKey = DMO_FUEL_GAUGE_MIN_POLL_INTERVAL;
    fbe_u32_t key;
    
    status = fbe_registry_read(NULL,
                               fbe_get_registry_path(),
                               FBE_FUEL_GAUGE_POLL_INTERVAL_REG_KEY, 
                               &key,
                               sizeof(key),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaultKey,   // default 
                               0,                                     
                               FALSE);

 
    *fuel_gauge_poll_interval = key;
    
    if (FBE_STATUS_OK != status)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: fbe_registry_read %s failed, status=%d, minimum poll interval 0x%x\n", 
                              FBE_FUEL_GAUGE_POLL_INTERVAL_REG_KEY, status, *fuel_gauge_poll_interval);

        *fuel_gauge_poll_interval = defaultKey;
    }
    
    fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %s fuel_gauge_poll_interval is set to %d \n",
                          __FUNCTION__, *fuel_gauge_poll_interval);
    return;
}

/*!****************************************************************************
 * @fn      fbe_drive_mgmt_cvt_value_to_ull()
 *
 * @brief
 *  This function converts an attribute field into a long long. This
 * assumes the data is in little endian format and so it swaps the bytes around.
 *
 * @param num       - pointer to 1st of the any bytes to be converted.
 * @param swap      - Swap from the little endian to big endian (FBE_TRUE or FBE_FALSE).
 * @param numbytes  - size of bytes are required to be converted. 
 *
 * @return
 *  resulting number as a unsigned long long.
 *
 * HISTORY
 *  01/20/12 :  Gerry Fredette -- Created.
 *  09/14/12 :  Christina Chiang -- Updated.
 *
 *****************************************************************************/
fbe_u64_t fbe_drive_mgmt_cvt_value_to_ull(fbe_u8_t *num, fbe_bool_t swap, fbe_s32_t numbytes)
{
    fbe_u64_t retvalue=0;
    fbe_s32_t shiftvalue = 8 * (numbytes - 1);
    fbe_s32_t i;
    
    if (swap)
    {
        for (i = numbytes-1; i >= 0; i--, shiftvalue -= 8)
        {
            retvalue |= (((fbe_u64_t)num[i]) << shiftvalue); 
        }
    }
    else
    {
        for (i = 0; i < numbytes; i++, shiftvalue -= 8)
        {
            retvalue |= (((fbe_u64_t)num[i]) << shiftvalue);
        }
    }
    return retvalue;
}

/*!****************************************************************************
 * @fn      fbe_drive_mgmt_find_duplicate_bms_lba()
 *
 * @brief
 *       This function find the duplicate lba bms 03/xx or 01/xx entry.
 *
 * @param failed_lba        - pointer to 1st of the failed lba.
 * @param current_fail_lba  - current failed lba.
 * @param total_errors      - total failed lba 01/xx or 03/xx BMS entries. 
 *
 * @return
 *  resulting increment number. increment = 0 if find a duplicate.
 *                              increment = 1 if don't find a duplicate.  
 *
 * HISTORY
 *  04/19/13 :  Christina Chiang -- Created.
 *
 *****************************************************************************/
fbe_u16_t fbe_drive_mgmt_find_duplicate_bms_lba(fbe_lba_t *failed_lba, fbe_lba_t current_failed_lba, fbe_u16_t total_errors)
{
    fbe_u16_t i = 0;
    fbe_u16_t increment = 1;
    
    /* Find the unique Lba for 01/xx/xx or 03/xx/xx BMS entries. */
    for( i = 0; i < total_errors; i++ )
    {
        if (current_failed_lba == failed_lba[i])
        {
            increment = 0;
            break;
        }
    }
    
    return increment;
}

