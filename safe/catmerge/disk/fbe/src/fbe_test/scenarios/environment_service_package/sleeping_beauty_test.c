/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file sleeping_beauty_test.c
 ***************************************************************************
 *
 * @brief
 *  This file detects the insertion of a drive and tests for a valid capacity.
 *
 * @version
 *   05/14/2010 - Created.  Wayne Garrett
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "mut.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "esp_tests.h"  
#include "pp_utils.h"  
#include "fbe/fbe_api_event_log_interface.h"
#include "EspMessages.h"
#include "physical_package_tests.h"
#include "fbe_test_esp_utils.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_api_environment_limit_interface.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   MACRO DEFINITIONS
 *************************/
#define SLEEPING_BEAUTY_BUS  0
#define SLEEPING_BEAUTY_ENCL 0
#define SLEEPING_BEAUTY_SLOT 5

/* copied exactly from dd_init to verify new code is correct */
#define DH_FORCE_SMALL_DISKS 0    //ignore small_disks
#define MAX_LBA_1GB_CLASS  (DH_FORCE_SMALL_DISKS ? 0x20000  : 0x1ED065)
#define MAX_LBA_9GB_CLASS  (DH_FORCE_SMALL_DISKS ? 0x1A57B6 : 0x1076D23)
#define MAX_LBA_18GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x353519 : 0x2141300)
#define MAX_LBA_36GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x69AD0B : 0x420C277)
#define MAX_LBA_72GB_CLASS (DH_FORCE_SMALL_DISKS ? 0xD53E75 : 0x8547099)
#define MAX_LBA_181GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x20CBC60 : 0x147F5BC3)
#define MAX_LBA_146GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x1ABC71E : 0x10B5C72F)
#define MAX_LBA_250GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x2E087D3 : 0x1CC54E3F)
#define MAX_LBA_300GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x35AE4AE : 0x218CEECD)
#define MAX_LBA_320GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x3B68773 : 0x25214A7F)
#define MAX_LBA_400GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x495C0BF : 0x2DD9877F)
#define MAX_LBA_450GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x50856FF : 0x325365FF)
#define MAX_LBA_500GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x5BB81F9 : 0x395313BF)
#define MAX_LBA_750GB_CLASS (DH_FORCE_SMALL_DISKS ? 0x8993EE6 : 0x55FC74FF)
#define MAX_LBA_1000GB_CLASS (DH_FORCE_SMALL_DISKS ? 0xB76FBD3 : 0x72A5D63F)

typedef struct sb_drive_capacity_s
{
    TEXT *product_id;   /* string to compare to inquiry data */
    fbe_lba_t capacity;
}
sb_drive_capacity_t;

sb_drive_capacity_t sb_drive_info[] =
{
    {(TEXT *) "xxxxxxxx CLAR01 ", MAX_LBA_1GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR09 ", MAX_LBA_9GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR18 ", MAX_LBA_18GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR36 ", MAX_LBA_36GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR72 ", MAX_LBA_72GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR181", MAX_LBA_181GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR146", MAX_LBA_146GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR250", MAX_LBA_250GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR300", MAX_LBA_300GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR320", MAX_LBA_320GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR400", MAX_LBA_400GB_CLASS+1},
    {(TEXT *) "xxxxxxxx CLAR500", 0x395313DF},   // from Joe
    {(TEXT *) "xxxxxxxx CLAR750", 0x55FC751A},   // from Joe
    {(TEXT *) "xxxxxxxxCLAR1000", 0x72A5D655},   // from Joe
    {(TEXT *) "xxxxxxxxCLAR2000", 0xE54B5B42}    // from Joe
};
#define SB_DRIVE_TABLE_ENTRIES (sizeof(sb_drive_info)/sizeof(sb_drive_info[0]))


#define SB_MAX_REASON_STR 8

typedef struct sb_taken_offline_event_log_reason_s {
    dmo_event_log_drive_offline_reason_t offline_reason_id;
    fbe_u8_t reason_str[SB_MAX_REASON_STR+1];  /* must NULL terminate*/
    fbe_u8_t action_str[SB_MAX_REASON_STR+1];  /* must NULL terminate*/
} sb_taken_offline_event_log_reason_t;




/****************************************** 
 *    PROTOTYPES
 *****************************************/
fbe_status_t  sb_construct_event_log_msg(fbe_event_log_check_msg_t *event_log_msg,
                                         fbe_u32_t msg_id,                                                    
                                         ...);


/*!*******************************************************************
 * @var sb_taken_offline_event_log_reason_table
 *********************************************************************
 * @brief This is table of taken offline event log strings
 *
 *********************************************************************/
sb_taken_offline_event_log_reason_t sb_taken_offline_event_log_reason_table[] =
{
    {DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED,                              "Removed\0","Reinsert\0"},  /* must NULL terminate strings*/
    {DMO_DRIVE_OFFLINE_REASON_NON_EQ,                                     "Non EQ\0", "Upgrd FW\0"},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED, "Non-EQ\0", "Escalate\0"},
    {FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION,    "MisMatch\0", "Relocate\0"},
    {FBE_BASE_PYHSICAL_DRIVE_DEATH_REASON_EXCEEDS_PLATFORM_LIMITS,        "By ESP", "\0"},

/*  As we add more test cases for different death reasons, fill out details below ..

    {DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE,,},  
    {DMO_DRIVE_OFFLINE_REASON_PERSISTED_EOL,,},
    {DMO_DRIVE_OFFLINE_REASON_PERSISTED_DF,,},
    etc...
*/

    {0, NULL, NULL}
};



/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * sleeping_beauty_get_taken_offline_reason_str()
 ****************************************************************
 * @brief
 *  Return taken offline reason string.  NULL returned if reason ID
 *  not found.
 *  
 * @param reason_id - ID for take offline reason
 *
 * @author
 *  12/23/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_u8_t* sleeping_beauty_get_taken_offline_reason_str(dmo_event_log_drive_offline_reason_t reason_id)
{
    sb_taken_offline_event_log_reason_t * table = sb_taken_offline_event_log_reason_table;

    while (table->offline_reason_id != 0)
    {
        if (reason_id == table->offline_reason_id )
        {
            MUT_ASSERT_TRUE(strlen(table->reason_str) <= SB_MAX_REASON_STR);
            return table->reason_str;
        }
        table++;
    }
    MUT_FAIL_MSG("reason_id not found in reason_table.   add it!");
    return NULL;
}

/*!**************************************************************
 * sleeping_beauty_get_taken_offline_action_str()
 ****************************************************************
 * @brief
 *  Return taken offline action string.  NULL returned if reason ID
 *  not found.
 *  
 * @param reason_id - ID for take offline action
 *
 * @author
 *  12/23/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
fbe_u8_t* sleeping_beauty_get_taken_offline_action_str(dmo_event_log_drive_offline_reason_t reason_id)
{
    sb_taken_offline_event_log_reason_t * table = sb_taken_offline_event_log_reason_table;

    while (table->offline_reason_id != 0)
    {
        if (reason_id == table->offline_reason_id)
        {
            MUT_ASSERT_TRUE(strlen(table->action_str) <= SB_MAX_REASON_STR);
            return table->action_str;
        }
        table++;
    }
    MUT_FAIL_MSG("reason_id not found in reason_table.   add it!");
    return NULL;
}

/*!**************************************************************
 * sleeping_beauty_verify_event_log_drive_online()
 ****************************************************************
 * @brief
 *  Verify event log for Drive Online.
 *  
 * @param bus, encl, slot - drive location
 *
 * @author
 *  12/23/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
void sleeping_beauty_verify_event_log_drive_online(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot)
{
    fbe_device_physical_location_t  location = {0};   
    fbe_bool_t is_msg_present = FBE_FALSE;        
    char deviceStr[FBE_DEVICE_STRING_LENGTH];    
    fbe_physical_drive_information_t physical_drive_info = {0};
    fbe_object_id_t pdo;
    fbe_status_t status;

    location.bus = bus;
    location.enclosure = encl;
    location.slot = slot;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE, &location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to create device string.");

    status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(pdo, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_wait_for_event_log_msg(10000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present, 
                                         ESP_INFO_DRIVE_ONLINE,
                                         &deviceStr[0],                                         
                                         physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_TRUE(is_msg_present);
}

/*!**************************************************************
 * sleeping_beauty_verify_event_log_drive_offline()
 ****************************************************************
 * @brief
 *  Verify event log for Drive Offline.
 *  
 * @param bus, encl, slot - drive location
 * @param death_reason - ID for death reason
 *
 * @author
 *  12/23/2013 : Wayne Garrett - Created.
 *
 ****************************************************************/
void sleeping_beauty_verify_event_log_drive_offline(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_physical_drive_information_t *pdo_drive_info_p, fbe_u32_t death_reason)
{
    fbe_device_physical_location_t  location = {0};   
    fbe_physical_drive_information_t physical_drive_info = {0};
    fbe_bool_t is_msg_present = FBE_FALSE;        
    fbe_object_id_t pdo = FBE_OBJECT_ID_INVALID;
    fbe_status_t status;
    char deviceStr[FBE_DEVICE_STRING_LENGTH];    
    char reasonStr[FBE_DEVICE_STRING_LENGTH]; 
    fbe_u32_t msg_id;    
    fbe_event_log_check_msg_t event_log_msg = {0};

    mut_printf(MUT_LOG_LOW, "%s death_reason=0x%x", __FUNCTION__, death_reason); 


    location.bus = bus;
    location.enclosure = encl;
    location.slot = slot;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE, &location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to create device string.");

    if (pdo_drive_info_p == NULL)
    {
        status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &pdo);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        status = fbe_api_physical_drive_get_drive_information(pdo, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB); 
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        physical_drive_info = *pdo_drive_info_p;
    }


    csx_p_snprintf(reasonStr,sizeof(reasonStr),"0x%x", death_reason);    


    switch(death_reason)
    {
    case DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED:
        msg_id = ESP_ERROR_DRIVE_OFFLINE_ACTION_REINSERT;
        status = fbe_api_wait_for_event_log_msg(10000,
                     FBE_PACKAGE_ID_ESP, 
                     &is_msg_present, 
                     msg_id,
                     &deviceStr[0],
                     physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                     reasonStr, "Removed");   
        if (status != FBE_STATUS_OK || !is_msg_present)
        {
            status = sb_construct_event_log_msg(&event_log_msg,
                                                msg_id,
                                                &deviceStr[0],
                                                physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                                                reasonStr, "Removed");
             mut_printf(MUT_LOG_LOW, "%s wait_for_event failed. event:%s", __FUNCTION__, event_log_msg.msg); 
        }
        break;
    case DMO_DRIVE_OFFLINE_REASON_NON_EQ:
        msg_id = ESP_ERROR_DRIVE_OFFLINE_ACTION_UPGRD_FW;
        status = fbe_api_wait_for_event_log_msg(10000,
                     FBE_PACKAGE_ID_ESP, 
                     &is_msg_present, 
                     msg_id,
                     &deviceStr[0],
                     physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                     reasonStr, "Non EQ");
        if (status != FBE_STATUS_OK || !is_msg_present)
        {
            status = sb_construct_event_log_msg(&event_log_msg,
                                                msg_id,
                                                &deviceStr[0],
                                                physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                                                reasonStr, "Non EQ");
             mut_printf(MUT_LOG_LOW, "%s wait_for_event failed. event:%s", __FUNCTION__, event_log_msg.msg); 
        }
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED:
        msg_id = ESP_ERROR_DRIVE_OFFLINE_ACTION_ESCALATE;
        status = fbe_api_wait_for_event_log_msg(10000,
                     FBE_PACKAGE_ID_ESP, 
                     &is_msg_present, 
                     msg_id,
                     &deviceStr[0],
                     physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                     reasonStr, "Non-EQ");
        if (status != FBE_STATUS_OK || !is_msg_present)
        {
            status = sb_construct_event_log_msg(&event_log_msg,
                                                msg_id,
                                                &deviceStr[0],
                                                physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                                                reasonStr, "Non-EQ");
             mut_printf(MUT_LOG_LOW, "%s wait_for_event failed. event:%s", __FUNCTION__, event_log_msg.msg); 
        }
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION:
        msg_id = ESP_ERROR_DRIVE_OFFLINE_ACTION_RELOCATE;
        status = fbe_api_wait_for_event_log_msg(10000,
                     FBE_PACKAGE_ID_ESP, 
                     &is_msg_present, 
                     msg_id,
                     &deviceStr[0],
                     physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                     reasonStr, "MisMatch");
        if (status != FBE_STATUS_OK || !is_msg_present)
        {
            status = sb_construct_event_log_msg(&event_log_msg,
                                                msg_id,
                                                &deviceStr[0],
                                                physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                                                reasonStr, "MisMatch");
             mut_printf(MUT_LOG_LOW, "%s wait_for_event failed. event:%s", __FUNCTION__, event_log_msg.msg); 
        }
        break;
    case FBE_BASE_PYHSICAL_DRIVE_DEATH_REASON_EXCEEDS_PLATFORM_LIMITS:
        {
            fbe_environment_limits_t env_limits_esp, env_limits_sep;
            status = fbe_api_get_environment_limits(&env_limits_esp, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            status = fbe_api_get_environment_limits(&env_limits_sep, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
            MUT_ASSERT_TRUE(env_limits_esp.platform_fru_count == env_limits_sep.platform_fru_count);

            msg_id = ESP_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED;
            status = fbe_api_wait_for_event_log_msg(10000,
                         FBE_PACKAGE_ID_ESP, 
                         &is_msg_present, 
                         msg_id,
                         &deviceStr[0], env_limits_sep.platform_fru_count,
                         physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                         ""); 
            if (status != FBE_STATUS_OK || !is_msg_present)
            {
                status = sb_construct_event_log_msg(&event_log_msg,
                                                    msg_id,
                                                    &deviceStr[0], env_limits_sep.platform_fru_count,
                                                    physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                                                    ""); 
                 mut_printf(MUT_LOG_LOW, "%s wait_for_event failed.\n   msg_id:0x%x   msg_string:%s", __FUNCTION__, event_log_msg.msg_id, event_log_msg.msg); 
            }                   
        }
        break;
    default:
        MUT_FAIL_MSG("death reason validation not implemented");
    }
    

    if (status != FBE_STATUS_OK || !is_msg_present)
    {
        fbe_event_log_get_event_log_info_t events_array[FBE_EVENT_LOG_MESSAGE_COUNT];
        fbe_u32_t total_events_copied;
        fbe_u32_t event_log_index;
        fbe_status_t status2;

        mut_printf(MUT_LOG_LOW, "%s failed to find exact match.  dumping matching IDs", __FUNCTION__); 

        status2 = fbe_api_get_all_events_logs(events_array, &total_events_copied, FBE_EVENT_LOG_MESSAGE_COUNT, FBE_PACKAGE_ID_ESP);
        MUT_ASSERT_INT_EQUAL(status2, FBE_STATUS_OK);
        
        /* dump any event match msg_id and last 20 events.*/
        for (event_log_index = 0; event_log_index < total_events_copied; event_log_index++)
        {
            if (msg_id == events_array[event_log_index].event_log_info.msg_id ||
                event_log_index+20 > total_events_copied)
            {            
                mut_printf(MUT_LOG_LOW, "   time:%02d:%02d:%02d  msg_id:0x%x  msg_string:%s",
                           events_array[event_log_index].event_log_info.time_stamp.hour,
                           events_array[event_log_index].event_log_info.time_stamp.minute,
                           events_array[event_log_index].event_log_info.time_stamp.second,
                           events_array[event_log_index].event_log_info.msg_id,
                           events_array[event_log_index].event_log_info.msg_string);
            }
        }
        mut_printf(MUT_LOG_LOW, "%s end of matching IDs\n", __FUNCTION__);
    }
    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(is_msg_present);
}

/*******************************************************************************************/ 
fbe_status_t sleeping_beauty_wait_for_state(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_lifecycle_state_t state, fbe_u32_t timeout_ms)
{
    fbe_status_t status = FBE_STATUS_INVALID; 
    fbe_u32_t i;
          
    /*ns_context.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE;
    ns_context.expected_lifecycle_state = state;
    fbe_test_pp_util_wait_lifecycle_state_notification (&ns_context);
    */
     
    mut_printf(MUT_LOG_LOW, "Waiting for PDO state %d\n", state); 
    
    if ( state == FBE_LIFECYCLE_STATE_DESTROY)   // special case since object id is destroyed.  can't do normal wait.
    {
        fbe_bool_t is_destroyed = FBE_FALSE;

        for (i=0; i<timeout_ms; i+= 1000)    
        {
            fbe_u32_t object_handle_p;
    
            /* Check for the physical drives on the enclosure.
             */            
            status = fbe_api_get_physical_drive_object_id_by_location(bus, encl, slot, &object_handle_p);
    
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* If status was bad, then the drive does not exist.
             */
            if (object_handle_p ==  FBE_OBJECT_ID_INVALID)
            {
                is_destroyed = FBE_TRUE;
                status = FBE_STATUS_OK;
                break;   
            }
            mut_printf(MUT_LOG_LOW, "PDO not destroyed.  obj_id=%d", object_handle_p); 
            fbe_api_sleep(1000);
    
        }

        MUT_ASSERT_INT_EQUAL(is_destroyed, FBE_TRUE);

    }
    else   // all other states
    {    
        status = fbe_test_pp_util_verify_pdo_state(bus, 
                                                   encl, 
                                                   slot, 
                                                   state, 
                                                   timeout_ms);
                                                   
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status)
    
    }
        
    return status;    
}

/*******************************************************************************************/
static fbe_status_t sleeping_beauty_insert_drive(const TEXT* product_id, fbe_lba_t capacity)
{
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_terminator_sas_drive_info_t     sas_drive;
    fbe_status_t                        status;

        
    drive_info.location.bus =       SLEEPING_BEAUTY_BUS;
    drive_info.location.enclosure = SLEEPING_BEAUTY_ENCL;
    drive_info.location.slot =      SLEEPING_BEAUTY_SLOT;    
    
    status = fbe_api_terminator_get_enclosure_handle(SLEEPING_BEAUTY_BUS,SLEEPING_BEAUTY_ENCL,&encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    sas_drive.backend_number = SLEEPING_BEAUTY_BUS;
    sas_drive.encl_number = SLEEPING_BEAUTY_ENCL;
    sas_drive.capacity = capacity;
    sas_drive.block_size = 520;
    sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number + sas_drive.backend_number) << 16) + SLEEPING_BEAUTY_SLOT;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;

    fbe_sprintf(sas_drive.drive_serial_number, sizeof(sas_drive.drive_serial_number), "%llX", (unsigned long long)sas_drive.sas_address);
    fbe_copy_memory(sas_drive.product_id, product_id, sizeof(sas_drive.product_id));

    //TODO: inject product id when terminator provides capability and remove hardcode workaround in drive_mgmt_monitor.
    
    mut_printf(MUT_LOG_LOW, "Inserting drive %d_%d_%d\n", sas_drive.backend_number, sas_drive.encl_number,
               SLEEPING_BEAUTY_SLOT);    
    status = fbe_api_terminator_insert_sas_drive (encl_handle, SLEEPING_BEAUTY_SLOT, &sas_drive, &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    return status;
}
 
/*******************************************************************************************/
fbe_status_t sleeping_beauty_remove_drive(fbe_u32_t port,
                                          fbe_u32_t enclosure,
                                          fbe_u32_t slot)
{
    fbe_esp_drive_mgmt_drive_info_t     drive_info = {0};
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_status_t                        status;               
    fbe_physical_drive_information_t    physical_drive_info;
    fbe_object_id_t                     object_id;    

    /* get drive info so we can test that an event log entry was added after
       drive is destroyed
    */
    status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_id);
    status = fbe_api_physical_drive_get_drive_information(object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);    

    drive_info.location.bus =       port;
    drive_info.location.enclosure = enclosure;
    drive_info.location.slot =      slot;

    status = fbe_api_terminator_get_drive_handle(drive_info.location.bus,
                                                 drive_info.location.enclosure,
                                                 drive_info.location.slot,
                                                 &drive_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
                                                 
    mut_printf(MUT_LOG_LOW, "Removing drive %d_%d_%d\n", drive_info.location.bus, drive_info.location.enclosure,
               drive_info.location.slot);   
   
    status = fbe_api_terminator_remove_sas_drive (drive_handle);   

#if 0
    // TODO: investigate using this function over remove_sas_drive
    /* Logout the drive using terminator by changing insert bit
     * (but leave the data intact)
     */
    status = fbe_api_terminator_pull_drive(drive_handle);
#endif

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    sleeping_beauty_wait_for_state(port, enclosure, slot, FBE_LIFECYCLE_STATE_DESTROY, 15000);

    sleeping_beauty_verify_event_log_drive_offline(port, enclosure, slot, &physical_drive_info, DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED);

    return status;
}

/*******************************************************************************************/
static void test_valid_cap_known_drive(TEXT* product_id, fbe_lba_t capacity)
{
    fbe_status_t                        status;
    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_physical_drive_information_t    physical_drive_info;
    fbe_object_id_t                     object_id;
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
    char                                deviceStr[FBE_DEVICE_STRING_LENGTH];
    
    mut_printf(MUT_LOG_LOW, "!!! Testing drive insert with valid capacity and known drive id. Expect drive pass\n");
    
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to clear ESP event log!");

    status = sleeping_beauty_remove_drive(SLEEPING_BEAUTY_BUS, SLEEPING_BEAUTY_ENCL, SLEEPING_BEAUTY_SLOT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    mut_printf(MUT_LOG_LOW, "Insertion with valid capacity (id=%s cap=%X)\n", product_id+8,(unsigned int)capacity);
    status = sleeping_beauty_insert_drive(product_id, capacity);   // expect pass
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    sleeping_beauty_wait_for_state(SLEEPING_BEAUTY_BUS, SLEEPING_BEAUTY_ENCL, SLEEPING_BEAUTY_SLOT, FBE_LIFECYCLE_STATE_READY, 15000);

    /* Test that event log contains msg for drive coming online
     */
    status = fbe_api_get_physical_drive_object_id_by_location(SLEEPING_BEAUTY_BUS, SLEEPING_BEAUTY_ENCL, SLEEPING_BEAUTY_SLOT, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);        
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    drive_info.location.bus =       SLEEPING_BEAUTY_BUS;
    drive_info.location.enclosure = SLEEPING_BEAUTY_ENCL;
    drive_info.location.slot =      SLEEPING_BEAUTY_SLOT;  

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE, &drive_info.location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to creat device string.");

    status = fbe_api_wait_for_event_log_msg(30000,
                                         FBE_PACKAGE_ID_ESP, 
                                         &is_msg_present, 
                                         ESP_INFO_DRIVE_ONLINE,
                                         &deviceStr[0],                                         
                                         physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev);    
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");

}

/*******************************************************************************************/
static void test_invalid_cap_known_drive(TEXT* product_id, fbe_lba_t capacity)
{
    fbe_status_t status;
    
    mut_printf(MUT_LOG_LOW, "!!! Testing drive insert with INVALID capacity and known drive id. Expect drive fail\n");
    
    status = sleeping_beauty_remove_drive(SLEEPING_BEAUTY_BUS, SLEEPING_BEAUTY_ENCL, SLEEPING_BEAUTY_SLOT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    mut_printf(MUT_LOG_LOW, "Insertion with invalid capacity (id=%s cap=%X)\n",product_id+8,(unsigned int)capacity);
    status = sleeping_beauty_insert_drive(product_id, capacity);  // expect fail
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    sleeping_beauty_wait_for_state(SLEEPING_BEAUTY_BUS, SLEEPING_BEAUTY_ENCL, SLEEPING_BEAUTY_SLOT, FBE_LIFECYCLE_STATE_FAIL, 15000);  
    fbe_api_sleep(100);   // give time for DMO to process FAIL event before going onto next test.    
}

/*******************************************************************************************/
static void test_valid_cap_unkown_drive(TEXT* product_id, fbe_lba_t capacity)
{
    fbe_status_t status;
        
    mut_printf(MUT_LOG_LOW, "!!! Testing drive insert with valid capacity and UNKOWN drive id. Expect drive pass\n");
    
    status = sleeping_beauty_remove_drive(SLEEPING_BEAUTY_BUS, SLEEPING_BEAUTY_ENCL, SLEEPING_BEAUTY_SLOT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    mut_printf(MUT_LOG_LOW, "Insertion with unknown drive capacity (id=%s cap=%X)\n",product_id+8,(unsigned int)capacity);
    status = sleeping_beauty_insert_drive(product_id, capacity);   // expect pass
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    sleeping_beauty_wait_for_state(SLEEPING_BEAUTY_BUS, SLEEPING_BEAUTY_ENCL, SLEEPING_BEAUTY_SLOT, FBE_LIFECYCLE_STATE_READY, 15000);
}


/*******************************************************************************************/
char * sleeping_beauty_short_desc = "Drive insertion test";
char * sleeping_beauty_long_desc ="\
Tests the maximum capacity of inserted drvies\n\
\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper drive\n\
        [PP] 3 SAS drives/drive\n\
        [PP] 3 logical drives/drive\n\
STEP 1: Bring up the initial topology.\n\
STEP 2: Validate the insertion of a known drive with valid capacity. Drive will pass.\n\
STEP 3: Validate the insertion of a known drive with an invalid capacity. Drive will fail.\n\
STEP 4: Validate the insertion of an unkown drive with any capacity. Drive will pass.\n";

/*!**************************************************************
 * sleeping_beauty_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test ESP framework
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/14/2010 - Created.  Wayne Garrett
 ****************************************************************/
void sleeping_beauty_test(void)
{
    fbe_drive_mgmt_policy_t policy;
    fbe_u32_t i;
    fbe_status_t status = FBE_STATUS_OK;

   /* Wait util there is no firmware upgrade in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

     /* Wait util there is no resume prom read in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 30 seconds for resume prom read to complete ===");
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    mut_printf(MUT_LOG_LOW, "%s Start\n",__FUNCTION__);
                    
    /* Enable the drive capacity verification policy 
    */
    mut_printf(MUT_LOG_LOW, "Enabling Drive Mgmt Verify-Capacity policy\n");
    policy.id =     FBE_DRIVE_MGMT_POLICY_VERIFY_CAPACITY;
    policy.value =  FBE_DRIVE_MGMT_POLICY_ON;
    fbe_api_esp_drive_mgmt_change_policy(&policy);
     
    /* Run test cases 
    */
    for(i=0; i<SB_DRIVE_TABLE_ENTRIES; i++)
    {
        test_valid_cap_known_drive(sb_drive_info[i].product_id, sb_drive_info[i].capacity);
    }
    
    for(i=0; i<SB_DRIVE_TABLE_ENTRIES; i++)
    {
        test_invalid_cap_known_drive(sb_drive_info[i].product_id, sb_drive_info[i].capacity+1);
    }
    
    test_valid_cap_unkown_drive("xxxxxxxxCLAR1\0\0\0", 0x11111111);


    mut_printf(MUT_LOG_LOW, "Sleeping Beauty test passed...\n");

    fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    return;
}

/******************************************
 * end sleeping_beauty_test()
 ******************************************/

void sleeping_beauty_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                    FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
                                    SPID_PROMETHEUS_M1_HW_TYPE,
                                    NULL,
                                    NULL);
}

void sleeping_beauty_destroy(void)
{
    fbe_test_sep_util_destroy_neit_sep_physical();
    fbe_test_esp_delete_registry_file();
    return;
}

/*!*****************************************************************************
 * @fn sb_construct_event_log_msg()
 *******************************************************************************
 * @brief
 *  This returns the construct the event log message, which can be useful
 *  when debugging. Only fills in event_log_msg.msg field.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  03/14/2014  Created.  Wayne Garrett
 *
 ******************************************************************************/
fbe_status_t  sb_construct_event_log_msg(fbe_event_log_check_msg_t *event_log_msg,
                                         fbe_u32_t msg_id,                                                    
                                         ...)
{
    fbe_status_t status;
    va_list args;

    /* Initialized the fbe_event_log_check_msg_t struct*/
    event_log_msg->msg_id = msg_id;
    
    va_start(args, msg_id);
    status = fbe_event_log_get_full_msg(event_log_msg->msg,
                                        FBE_EVENT_LOG_MESSAGE_STRING_LENGTH, 
                                        msg_id, 
                                        args);
    va_end(args);

    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in retrieving the message string, Status:%d\n", __FUNCTION__, status);
        return status;
    }

    return status;
}


/*************************
 * end file sleeping_beauty_test.c
 *************************/

