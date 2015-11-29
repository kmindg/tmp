/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file pink_panther_test.c
 ***************************************************************************
 *
 * @brief
 *  This file detects the insertion of a drive and tests for a valid price class.
 *
 * @version
 *   05/14/2010 - Created (Sleeping Beauty Test).  Wayne Garrett
 *   05/17/2012 - Adapted from Sleeping Beauty Test. 
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

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   MACRO DEFINITIONS
 *************************/
#define PINK_PANTHER_BUS  0
#define PINK_PANTHER_ENCL 0
#define PINK_PANTHER_SLOT 5
#define PINK_PANTHER_LIFECYCLE_NOTIFICATION_TIMEOUT 15000  // 15 sec

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

sb_drive_capacity_t clar_drive_info[] =
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

sb_drive_capacity_t neo_drive_info[] =
{
    {(TEXT *) "xxxxxxxx  NEO300", MAX_LBA_300GB_CLASS+1},
    {(TEXT *) "xxxxxxxx  NEO320", MAX_LBA_320GB_CLASS+1},
    {(TEXT *) "xxxxxxxx  NEO400", MAX_LBA_400GB_CLASS+1},
};

#define SB_DRIVE_TABLE_ENTRIES (sizeof(clar_drive_info)/sizeof(clar_drive_info[0]))

#define NEO_DRIVE_TABLE_ENTRIES (sizeof(neo_drive_info)/sizeof(clar_drive_info[0]))


/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*******************************************************************************************/ 
static fbe_status_t wait_for_state(fbe_lifecycle_state_t state)
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

        for (i=0; i<15000; i+= 100)    // 15 sec timeout
        {
            fbe_u32_t object_handle_p;
    
            /* Check for the physical drives on the enclosure.
             */            
            status = fbe_api_get_physical_drive_object_id_by_location(PINK_PANTHER_BUS, PINK_PANTHER_ENCL, PINK_PANTHER_SLOT, &object_handle_p);
    
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
            fbe_api_sleep(100);
    
        }

        MUT_ASSERT_INT_EQUAL(is_destroyed, FBE_TRUE);

    }
    else   // all other states
    {    
        status = fbe_test_pp_util_verify_pdo_state(PINK_PANTHER_BUS, 
                                                   PINK_PANTHER_ENCL, 
                                                   PINK_PANTHER_SLOT, 
                                                   state, 
                                                   PINK_PANTHER_LIFECYCLE_NOTIFICATION_TIMEOUT);
                                                   
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status)
    
        if ( state != FBE_LIFECYCLE_STATE_FAIL )
        {
            /* Do not wait for LDO when going to FAIL since PDO can go to READY then FAIL 
               before LDO becomes READY, leaving it stuck in SPECIALIZED.  */
            
            mut_printf(MUT_LOG_LOW, "Waiting for LDO state %d\n", state); 
    
            status = fbe_test_pp_util_verify_ldo_state(PINK_PANTHER_BUS, 
                                                       PINK_PANTHER_ENCL, 
                                                       PINK_PANTHER_SLOT, 
                                                       state, 
                                                       PINK_PANTHER_LIFECYCLE_NOTIFICATION_TIMEOUT); 
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status)
        }
    }
        
    return status;    
}

/*******************************************************************************************/
static fbe_status_t pink_panther_insert_drive(const TEXT* product_id, fbe_lba_t capacity)
{
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_terminator_sas_drive_info_t     sas_drive;   
    fbe_status_t                        status;

        
    drive_info.location.bus =       PINK_PANTHER_BUS;
    drive_info.location.enclosure = PINK_PANTHER_ENCL;
    drive_info.location.slot =      PINK_PANTHER_SLOT;    
    
    status = fbe_api_terminator_get_enclosure_handle(PINK_PANTHER_BUS,PINK_PANTHER_ENCL,&encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    sas_drive.backend_number = PINK_PANTHER_BUS;
    sas_drive.encl_number = PINK_PANTHER_ENCL;
    sas_drive.capacity = capacity;
    sas_drive.block_size = 520;
    sas_drive.sas_address = 0x5000097A78000000 + ((fbe_u64_t)(sas_drive.encl_number + sas_drive.backend_number) << 16) + PINK_PANTHER_SLOT;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;    
    fbe_copy_memory(sas_drive.product_id,product_id,FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1); //16 bytes and NULL termination
    
    
    mut_printf(MUT_LOG_LOW, "Inserting drive %d_%d_%d\n", sas_drive.backend_number, sas_drive.encl_number,
               PINK_PANTHER_SLOT);    
    status = fbe_api_terminator_insert_sas_drive (encl_handle, PINK_PANTHER_SLOT, &sas_drive, &drive_handle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
/*
    status = fbe_terminator_api_set_drive_product_id(drive_handle, product_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
*/    
    return status;
}
 
/*******************************************************************************************/
static fbe_status_t pink_panther_remove_drive(fbe_u32_t port,
                                                 fbe_u32_t enclosure,
                                                 fbe_u32_t slot)
{
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_status_t                        status;               
    fbe_physical_drive_information_t    physical_drive_info;
    fbe_object_id_t                     object_id;
    fbe_bool_t                          is_msg_present = FBE_FALSE;
    char                                deviceStr[FBE_DEVICE_STRING_LENGTH];
    char                                reasonStr[FBE_DEVICE_STRING_LENGTH];

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
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    wait_for_state(FBE_LIFECYCLE_STATE_DESTROY);

    /* Check for event log message
     */ 
    /* event log is not always there when we check, since logging is handled in a separate thread.  
     * So wait for maximum 30 seconds to see whether the event log message is present
     * before giving up
     */ 
                  
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE, &drive_info.location, &deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK, "Failed to creat device string.");

    csx_p_snprintf(reasonStr, sizeof(reasonStr), "0x%x", DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED);
    status = fbe_api_wait_for_event_log_msg(30000,
                                     FBE_PACKAGE_ID_ESP, 
                                     &is_msg_present, 
                                     ESP_ERROR_DRIVE_OFFLINE,
                                     &deviceStr[0],
                                     physical_drive_info.product_serial_num, physical_drive_info.tla_part_number, physical_drive_info.product_rev,
                                     reasonStr, "Removed" , "Reinsert");    
  
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_TRUE, is_msg_present, "Event log msg is not present!");
   
    return status;
}

/*******************************************************************************************/
static void test_valid_cap_known_b_class_drive(TEXT* product_id, fbe_lba_t capacity)
{
    fbe_status_t                        status;
    fbe_bool_t                          is_msg_present = FBE_FALSE;
    fbe_physical_drive_information_t    physical_drive_info;
    fbe_object_id_t                     object_id;
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
    char                                deviceStr[FBE_DEVICE_STRING_LENGTH];
    
    mut_printf(MUT_LOG_LOW, "!!! Testing drive insert with B class drive. Expect drive pass\n");
    
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_ESP);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, 
                        "Failed to clear ESP event log!");

    status = pink_panther_remove_drive(PINK_PANTHER_BUS,PINK_PANTHER_ENCL,PINK_PANTHER_SLOT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    mut_printf(MUT_LOG_LOW, "Insertion B class drive (id=%s cap=%X)\n", product_id+8,(unsigned int)capacity);
    status = pink_panther_insert_drive(product_id, capacity);   // expect pass
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    wait_for_state(FBE_LIFECYCLE_STATE_READY);

    /* Test that event log contains msg for drive coming online
     */
    status = fbe_api_get_physical_drive_object_id_by_location(PINK_PANTHER_BUS, PINK_PANTHER_ENCL, PINK_PANTHER_SLOT, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_physical_drive_get_drive_information(object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);        
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    drive_info.location.bus =       PINK_PANTHER_BUS;
    drive_info.location.enclosure = PINK_PANTHER_ENCL;
    drive_info.location.slot =      PINK_PANTHER_SLOT;  

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
static void test_valid_cap_known_j_class_drive(TEXT* product_id, fbe_lba_t capacity)
{
    fbe_status_t status;
    
    mut_printf(MUT_LOG_LOW, "!!! Testing drive insert with J class. Expect drive fail\n");
    
    status = pink_panther_remove_drive(PINK_PANTHER_BUS,PINK_PANTHER_ENCL,PINK_PANTHER_SLOT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);    
    mut_printf(MUT_LOG_LOW, "Insertion J class drive (id=%s cap=%X)\n",product_id+8,(unsigned int)capacity);
    status = pink_panther_insert_drive(product_id, capacity);  // expect fail
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    wait_for_state(FBE_LIFECYCLE_STATE_FAIL);  
    fbe_api_sleep(100);   // give time for DMO to process FAIL event before going onto next test.    
}

/*******************************************************************************************/
static void test_valid_cap_known_j_class_drive_policy_off(TEXT* product_id, fbe_lba_t capacity)
{
    fbe_status_t status;
    
    mut_printf(MUT_LOG_LOW, "!!! Testing drive insert with J class with policy off. Expect drive pass\n");
    
    status = pink_panther_remove_drive(PINK_PANTHER_BUS,PINK_PANTHER_ENCL,PINK_PANTHER_SLOT);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
    mut_printf(MUT_LOG_LOW, "Insertion J class drive (id=%s cap=%X)\n",product_id+8,(unsigned int)capacity);
    status = pink_panther_insert_drive(product_id, capacity);  // expect pass
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK); 
    wait_for_state(FBE_LIFECYCLE_STATE_READY);      
}


/*******************************************************************************************/
char * pink_panther_short_desc = "Drive price class test";
char * pink_panther_long_desc ="\
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
STEP 2: Test table entry addition, lookup and deletion.\n\
STEP 3: Validate the insertion of a B class drive. Drive will pass.\n\
STEP 4: Validate the insertion of a J class drive. Drive will fail.\n\
STEP 5: Validate the insertion of a J class with policy off. Drive will pass.\n";

/*!**************************************************************
 * pink_panther_test() 
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
 *  5/17/2012 - Adapted from Sleeping Beauty Test.
 ****************************************************************/
void pink_panther_test(void)
{
    fbe_drive_mgmt_policy_t policy;
    fbe_u32_t i;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t   drive_supported = FBE_FALSE;
    
   /* Wait util there is no firmware upgrade in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 60 seconds for upgrade to complete ===");
    status = fbe_test_esp_wait_for_no_upgrade_in_progress(60000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for upgrade to complete failed!");

     /* Wait util there is no resume prom read in progress. */
    mut_printf(MUT_LOG_LOW, "=== Wait max 30 seconds for resume prom read to complete ===");
    status = fbe_test_esp_wait_for_no_resume_prom_read_in_progress(30000);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for resume prom read to complete failed!");

    mut_printf(MUT_LOG_LOW, "%s Start\n",__FUNCTION__);

    /* Test table entry addition. */
    status = fbe_api_esp_drive_mgmt_add_drive_price_class_support_entry(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M4_HW_TYPE);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Adding price class - platform entry failed!");

    status = fbe_api_esp_drive_mgmt_add_drive_price_class_support_entry(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Adding price class - platform entry failed!");

    drive_supported = FBE_FALSE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M4_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_TRUE, "Checking price class - platform supported. Wrong result!");

    drive_supported = FBE_FALSE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_TRUE, "Checking price class - platform supported. Wrong result!");

    /* FBE_DRIVE_PRICE_CLASS_J should not be supported on SPID_PROMETHEUS_M1_HW_TYPE. */
    drive_supported = FBE_TRUE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_J, SPID_PROMETHEUS_M1_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_FALSE, "Checking price class - platform supported. Wrong result!");

    drive_supported = FBE_FALSE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_B, SPID_DEFIANT_M5_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_TRUE, "Checking price class - platform supported. Wrong result!");
    
    drive_supported = FBE_FALSE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_B, SPID_PROMETHEUS_M1_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_TRUE, "Checking price class - platform supported. Wrong result!");

    /* Test table duplicate entry addition. */
    /* API will return OK status since the entry is already present. */
    status = fbe_api_esp_drive_mgmt_add_drive_price_class_support_entry(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Adding price class - platform entry failed!");

    drive_supported = FBE_FALSE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_TRUE, "Checking price class - platform supported. Wrong result!");

    /* Test table entry removal. */
    status = fbe_api_esp_drive_mgmt_remove_drive_price_class_support_entry(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Adding price class - platform entry failed!");

    /* Should return no supported since only a single entry should exist for 
     * FBE_DRIVE_PRICE_CLASS_J - SPID_DEFIANT_M5_HW_TYPE and we just removed it.
     */
    drive_supported = FBE_TRUE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_FALSE, "Checking price class - platform supported. Wrong result!");

    /* Add entry back to table and check for supported.*/
    status = fbe_api_esp_drive_mgmt_add_drive_price_class_support_entry(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Adding price class - platform entry failed!");

    drive_supported = FBE_FALSE;
    status = fbe_api_esp_drive_mgmt_check_drive_price_class_supported(FBE_DRIVE_PRICE_CLASS_J, SPID_DEFIANT_M5_HW_TYPE, &drive_supported);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Checking price class - platform supported failed!");
    MUT_ASSERT_TRUE_MSG(drive_supported == FBE_TRUE, "Checking price class - platform supported. Wrong result!");

    /* Enable the drive price class verification policy 
    */
    mut_printf(MUT_LOG_LOW, "Enabling Drive Mgmt Verify-Price Class policy\n");
    policy.id =     FBE_DRIVE_MGMT_POLICY_VERIFY_DRIVE_CLASS;
    policy.value =  FBE_DRIVE_MGMT_POLICY_ON;
    fbe_api_esp_drive_mgmt_change_policy(&policy);
     
    /* Run test cases 
    */
    for(i=0; i<SB_DRIVE_TABLE_ENTRIES; i++)
    {
        test_valid_cap_known_b_class_drive(clar_drive_info[i].product_id, clar_drive_info[i].capacity);
    }

    for(i=0; i<NEO_DRIVE_TABLE_ENTRIES; i++)
    {
        test_valid_cap_known_j_class_drive(neo_drive_info[i].product_id, neo_drive_info[i].capacity);
    }

    policy.id =     FBE_DRIVE_MGMT_POLICY_VERIFY_DRIVE_CLASS;
    policy.value =  FBE_DRIVE_MGMT_POLICY_OFF;
    fbe_api_esp_drive_mgmt_change_policy(&policy);

    for(i=0; i<NEO_DRIVE_TABLE_ENTRIES; i++)
    {
        test_valid_cap_known_j_class_drive_policy_off(neo_drive_info[i].product_id, neo_drive_info[i].capacity);
    }

    
    mut_printf(MUT_LOG_LOW, "Pink Panther test passed...\n");
    return;
}

/******************************************
 * end pink_panther_test()
 ******************************************/

void pink_panther_setup(void)
{
    fbe_test_startEspAndSep_with_common_config(FBE_SIM_SP_A,
                                    FBE_ESP_TEST_CHAUTAUQUA_VIPER_CONFIG,
                                    SPID_PROMETHEUS_M1_HW_TYPE,
                                    NULL,
                                    NULL);
}

void pink_panther_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/*************************
 * end file pink_panther_test.c
 *************************/

