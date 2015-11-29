/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file forrest_gump_test.c
 ***************************************************************************
 *
 * @brief
 *  Verify Enclosure Fault LED & Drive LED behavior.
 * 
 * @version
 *   01/31/2011 - Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "esp_tests.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_ps_info.h"
#include "esp_tests.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "pp_utils.h"  
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe_base_environment_debug.h"
#include "EspMessages.h"

#include "fp_test.h"
#include "sep_tests.h"
#include "fbe_test_esp_utils.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * forrest_gump_short_desc = "Verify Enclosure Fault LED & Drive LED behavior";
char * forrest_gump_long_desc ="\
\n\
Forrest Gump\n\
        -lead character in movie of the same name\n\
        -famous quote 'Life is like a box of chocolates'\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
        - Start the foll. service in user space:\n\
        - 1. Memory Service\n\
        - 2. Scheduler Service\n\
        - 3. Eventlog service\n\
        - 4. FBE API \n\
        - Verify that the foll. classes are loaded and initialized\n\
        - 1. ERP\n\
        - 2. Enclosure Firmware Download\n\
        - 3. PS Management\n\
        - 4. Flexports\n\
        - 5. EIR\n\
STEP 2: Verify ps_mgmt control Enclosure Fault LED correctly\n\
        - fault a DAE PS (Enclosure Fault LED On)\n\
        - remove DAE PS fault (Enclosure Fault LED Off)\n\
        - remove DAE PS (Enclosure Fault LED On)\n\
        - insert DAE PS (Enclosure Fault LED Off)\n\
STEP 3: Verify drive_mgmt control Enclosure Fault LED correctly\n\
        - fault a DAE drive (Enclosure Fault LED On)\n\
        - remove DAE drive fault (Enclosure Fault LED Off)\n\
";

#define FORREST_GUMP_LIFECYCLE_NOTIFICATION_TIMEOUT 15000  // 15 sec


static fbe_object_id_t	expected_object_id = FBE_OBJECT_ID_INVALID;

/* EXTERNS */
extern void sleeping_beauty_verify_event_log_drive_offline(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_physical_drive_information_t *pdo_drive_info_p, fbe_u32_t death_reason);

static void forrest_gump_commmand_response_callback_function (update_object_msg_t * update_object_msg, 
                                                               void *context)
{
	fbe_semaphore_t 	*sem = (fbe_semaphore_t *)context;

	mut_printf(MUT_LOG_LOW, "=== callback, objId 0x%x, exp 0x%x ===\n",
        update_object_msg->object_id, expected_object_id);

	if (update_object_msg->object_id == expected_object_id) 
	{
	    if (update_object_msg->notification_info.notification_data.data_change_info.device_mask == FBE_DEVICE_TYPE_PS) 
	    {
    	    fbe_semaphore_release(sem, 0, 1 ,FALSE);
        }
	}
}

void forrest_gump_setClearPePsFault(fbe_u32_t psIndex, fbe_bool_t psFault)
{
    fbe_status_t                        status;
	fbe_semaphore_t 					sem;
    DWORD   							dwWaitResult;
    fbe_esp_ps_mgmt_ps_info_t           psInfo;
    SPECL_SFI_MASK_DATA                 sfi_mask_data;

    mut_printf(MUT_LOG_LOW, "  BaseBoard PS Info, setting PS %d to fault %d\n", 
               psIndex,
               psFault);

    fbe_semaphore_init(&sem, 0, 1);

    sfi_mask_data.structNumber = SPECL_SFI_PS_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.psStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.psStatus.psSummary[psIndex].psStatus->generalFault = psFault;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    // Verify that ESP ps_mgmt detects PS Info change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    fbe_api_sleep (5000);           // delay so PS_MGMT can see the same event
    psInfo.location.bus = 0;
    psInfo.location.enclosure = 0;
    psInfo.location.slot = psIndex;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "  BaseBoard PS Info, ps %d, psInserted %d, fault %d\n", 
               psIndex,
               psInfo.psInfo.bInserted,
               psInfo.psInfo.generalFault);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
    MUT_ASSERT_TRUE(psInfo.psInfo.generalFault == psFault);

    fbe_semaphore_destroy(&sem);

}

void forrest_gump_setClearPeCoolingFault(fbe_u32_t coolingIndex, fbe_bool_t coolingFault)
{
    fbe_status_t                        status;
	fbe_semaphore_t 					sem;
    DWORD   							dwWaitResult;
    fbe_esp_cooling_mgmt_fan_info_t     coolingInfo;
    SPECL_SFI_MASK_DATA                 sfi_mask_data;
    fbe_device_physical_location_t      location;
    fbe_semaphore_init(&sem, 0, 1);

    sfi_mask_data.structNumber = SPECL_SFI_FAN_STRUCT_NUMBER;
    sfi_mask_data.sfiSummaryUnions.fanStatus.sfiMaskStatus = TRUE;
    sfi_mask_data.maskStatus = SPECL_SFI_GET_CACHE_DATA;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    sfi_mask_data.maskStatus = SPECL_SFI_SET_CACHE_DATA;
    sfi_mask_data.sfiSummaryUnions.fanStatus.fanSummary[0].multFanFault = coolingFault;
    fbe_api_terminator_send_specl_sfi_mask_data(&sfi_mask_data);

    // Verify that ESP ps_mgmt detects PS Info change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;
    status = fbe_api_esp_cooling_mgmt_get_fan_info(&location, &coolingInfo);
    mut_printf(MUT_LOG_LOW, "  BaseBoard Cooling Info, cooling %d, inserted %d, fault %d\n", 
               0,
               coolingInfo.inserted,
               coolingInfo.fanFaulted);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(coolingInfo.inserted);
    MUT_ASSERT_TRUE(coolingInfo.fanFaulted == coolingFault);

    fbe_semaphore_destroy(&sem);

}

void forrest_gump_setClearDaePsFault(fbe_device_physical_location_t *pLocation, fbe_bool_t psFault)
{
    fbe_status_t                        status;
	fbe_semaphore_t 					sem;
    DWORD   							dwWaitResult;
    fbe_api_terminator_device_handle_t  encl_device_id;
    ses_stat_elem_ps_struct             ps_struct;
    fbe_esp_ps_mgmt_ps_info_t           psInfo;

    fbe_semaphore_init(&sem, 0, 1);

    status = fbe_api_terminator_get_enclosure_handle(pLocation->bus, 
                                                     pLocation->enclosure, 
                                                     &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_ps_eses_status(encl_device_id, 
                                                   pLocation->slot, 
                                                   &ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    // set PS fault
    if (psFault)
    {
        ps_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_CRITICAL;
    }
    else
    {
        ps_struct.cmn_stat.elem_stat_code = SES_STAT_CODE_OK;
    }
    status = fbe_api_terminator_set_ps_eses_status(encl_device_id, 
                                                   pLocation->slot, 
                                                   ps_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    // Verify that ESP ps_mgmt detects PS Info change
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event
    psInfo.location = *pLocation;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "  EnclObj %d_%d, PS Info, ps %d, psInserted %d, fault %d\n", 
               pLocation->bus, pLocation->enclosure, pLocation->slot,
               psInfo.psInfo.bInserted,
               psInfo.psInfo.generalFault);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.psInfo.bInserted);
    MUT_ASSERT_TRUE(psInfo.psInfo.generalFault == psFault);

    fbe_semaphore_destroy(&sem);

}

void forrest_gump_sendCmdToTerminator(fbe_device_physical_location_t *pLocation,
                                      fbe_u8_t coolingIndex, 
                                      fbe_u8_t coolingStatus)
{
    fbe_status_t                        status;
    fbe_api_terminator_device_handle_t  encl_device_id;
    ses_stat_elem_cooling_struct        cooling_struct;

    status = fbe_api_terminator_get_enclosure_handle(pLocation->bus, 
                                                     pLocation->enclosure, 
                                                     &encl_device_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_terminator_get_cooling_eses_status(encl_device_id, 
                                                        coolingIndex,
                                                        &cooling_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    cooling_struct.cmn_stat.elem_stat_code = coolingStatus;
    mut_printf(MUT_LOG_LOW, "  Send to Terminator, encl %d_%d, cooling %d, elemStat %d\n", 
               pLocation->bus,
               pLocation->enclosure,
               coolingIndex,
               cooling_struct.cmn_stat.elem_stat_code);
    status = fbe_api_terminator_set_cooling_eses_status(encl_device_id, 
                                                        coolingIndex,
                                                        cooling_struct);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

}

void forrest_gump_setClearDaePsInternalFanFault(fbe_device_physical_location_t *pLocation, 
                                          fbe_bool_t coolingFault)
{
    fbe_status_t                        status;
	fbe_semaphore_t 					sem;
    DWORD   							dwWaitResult;
    fbe_esp_ps_mgmt_ps_info_t           psInfo;
    fbe_u8_t                            coolingStatus;


    fbe_semaphore_init(&sem, 0, 1);


    if (coolingFault)
    {
        coolingStatus = SES_STAT_CODE_CRITICAL;
    }
    else
    {
        coolingStatus = SES_STAT_CODE_OK;
    }

    forrest_gump_sendCmdToTerminator(pLocation, COOLING_0, coolingStatus);
    forrest_gump_sendCmdToTerminator(pLocation, COOLING_1, coolingStatus);
    forrest_gump_sendCmdToTerminator(pLocation, COOLING_2, coolingStatus);
    forrest_gump_sendCmdToTerminator(pLocation, COOLING_3, coolingStatus);

    // Verify that ESP ps_mgmt detects the change of PS internal fan.
	dwWaitResult = fbe_semaphore_wait_ms(&sem, 10000);
	MUT_ASSERT_TRUE(dwWaitResult != EMCPAL_STATUS_TIMEOUT);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event
    fbe_zero_memory(&psInfo, sizeof(fbe_esp_ps_mgmt_ps_info_t));
    psInfo.location = *pLocation;
    status = fbe_api_esp_ps_mgmt_getPsInfo(&psInfo);
    mut_printf(MUT_LOG_LOW, "  EnclObj PS Info, psInternalFanFault %d\n", 
               psInfo.psInfo.internalFanFault);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(psInfo.psInfo.internalFanFault == coolingFault);

    fbe_semaphore_destroy(&sem);

}

void forrest_gump_verifyPeEnclFaultLedInfo(LED_BLINK_RATE enclFaultLed,
                                           fbe_u64_t enclFaultLedReason)
{
    fbe_status_t                        status;
    fbe_object_id_t                     objectId;
    fbe_board_mgmt_misc_info_t          miscInfo = {0};

    // Get Board Object ID
    status = fbe_api_get_board_object_id(&objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != FBE_OBJECT_ID_INVALID);

    // verify that Enclosure Fault LED is OFF
    status = fbe_api_board_get_misc_info(objectId, &miscInfo);
    mut_printf(MUT_LOG_LOW, "  BaseBoard EnclLedInfo, LED %d, Reason 0x%llX, Expected 0x%llX\n", 
              miscInfo.EnclosureFaultLED,
              miscInfo.enclFaultLedReason,
              enclFaultLedReason);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(miscInfo.EnclosureFaultLED == enclFaultLed);
    MUT_ASSERT_TRUE(miscInfo.enclFaultLedReason == enclFaultLedReason);
}

void forrest_gump_verifyDaeEnclFaultLedInfo(fbe_device_physical_location_t *pLocation,
                                            fbe_bool_t enclFaultLedOn,
                                            fbe_u64_t enclFaultLedReason)
{
    fbe_status_t                        status;
    fbe_object_id_t                     objectId;
    fbe_base_enclosure_led_status_control_t     ledStatusControlInfo;

    // verify the state of the Enclosure Fault LED & Reason
    status = fbe_api_get_enclosure_object_id_by_location(pLocation->bus, 
                                                         pLocation->enclosure, 
                                                         &objectId);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(objectId != NULL);
    ledStatusControlInfo.ledAction = FBE_ENCLOSURE_LED_STATUS;
    status = fbe_api_enclosure_send_set_led(objectId,
                                            &ledStatusControlInfo);
    mut_printf(MUT_LOG_LOW, "  EnclObj %d_%d, EnclLedInfo, LED behavior %d, LED status %d, Reason 0x%llx\n", 
               pLocation->bus, pLocation->enclosure,
              ledStatusControlInfo.ledInfo.enclFaultLed,
              ledStatusControlInfo.ledInfo.enclFaultLedStatus,
              ledStatusControlInfo.ledInfo.enclFaultReason);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(ledStatusControlInfo.ledInfo.enclFaultLedStatus == enclFaultLedOn);
    MUT_ASSERT_TRUE(ledStatusControlInfo.ledInfo.enclFaultReason == enclFaultLedReason);
}

/*******************************************************************************************/ 
static fbe_status_t wait_for_state(fbe_u32_t port,
                                   fbe_u32_t enclosure,
                                   fbe_u32_t slot,
                                   fbe_lifecycle_state_t state)
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
            status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_handle_p);
    
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* If status was bad, then the drive does not exist.
             */
            if (object_handle_p ==  FBE_OBJECT_ID_INVALID)
            {
                mut_printf(MUT_LOG_LOW, "PDO destroyed.  obj_id=%d", object_handle_p); 
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
        status = fbe_test_pp_util_verify_pdo_state(port, 
                                                   enclosure, 
                                                   slot, 
                                                   state, 
                                                   FORREST_GUMP_LIFECYCLE_NOTIFICATION_TIMEOUT);
                                                   
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status)
    
    }
        
    return status;    
}

/*******************************************************************************************/
static fbe_status_t forrest_gump_insert_drive(fbe_u32_t port,
                                              fbe_u32_t enclosure,
                                              fbe_u32_t slot,
                                              const TEXT* product_id, 
                                              fbe_lba_t capacity)
{
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
    fbe_api_terminator_device_handle_t  drive_handle;
    fbe_api_terminator_device_handle_t  encl_handle;
    fbe_terminator_sas_drive_info_t     sas_drive;   
    fbe_status_t                        status;

        
    drive_info.location.bus =       port;
    drive_info.location.enclosure = enclosure;
    drive_info.location.slot =      slot;    
    
    status = fbe_api_terminator_get_enclosure_handle(port,enclosure,&encl_handle);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    sas_drive.backend_number = port;
    sas_drive.encl_number = enclosure;
    sas_drive.capacity = capacity;
    sas_drive.block_size = 520;
    sas_drive.sas_address = 0x5000097A78000000 + 
        ((fbe_u64_t)(sas_drive.encl_number + 
                     sas_drive.backend_number) << 16) + slot;
    sas_drive.drive_type = FBE_SAS_DRIVE_CHEETA_15K;    
    //TODO: inject product id when terminator provides capability and remove hardcode workaround in drive_mgmt_monitor.
    
    mut_printf(MUT_LOG_LOW, "Inserting drive %d_%d_%d\n", sas_drive.backend_number, sas_drive.encl_number, slot);    
    status = fbe_api_terminator_insert_sas_drive (encl_handle, slot, &sas_drive, &drive_handle); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    return status;
}
 
/*******************************************************************************************/
static fbe_status_t forrest_gump_remove_drive(fbe_u32_t port,
                                              fbe_u32_t enclosure,
                                              fbe_u32_t slot)
{
    fbe_esp_drive_mgmt_drive_info_t     drive_info;
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
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    wait_for_state(port, enclosure, slot, FBE_LIFECYCLE_STATE_DESTROY);

    /* Check for event log message
     */ 
    sleeping_beauty_verify_event_log_drive_offline(port, enclosure, slot, &physical_drive_info, DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED);   
    return status;
}

/*!**************************************************************
 * forrest_gump_test() 
 ****************************************************************
 * @brief
 *  Main entry point to the test 
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   02/01/2011 - Created. Joe Perry
 *
 ****************************************************************/
void forrest_gump_test(void)
{
    fbe_device_physical_location_t      location = {0};
#if FALSE
    fbe_status_t                        status;
    fbe_object_id_t                     object_id;
#endif
    fbe_api_sleep (9000);           // delay so ps_mgmt can process the same event

    // verify Enclosure Fault LED turned OFF
    forrest_gump_verifyPeEnclFaultLedInfo(LED_BLINK_OFF, FBE_ENCL_FAULT_LED_NO_FLT);

    /*
     * Verify Enclosure Fault LED for Processor Enclosure PS events
     */
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Processor Enclosure Fault LED processing (Power Supplies)\n");
    // fault enclosure 0_1 PSB
    forrest_gump_setClearPePsFault(PS_1, TRUE);

    fbe_api_sleep (9000);           // delay so ps_mgmt can process the same event

    // verify Enclosure Fault LED turned ON
    forrest_gump_verifyPeEnclFaultLedInfo(LED_BLINK_ON, FBE_ENCL_FAULT_LED_PS_FLT);
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Processor Enclosure Fault LED processing - Verified\n");


    // clear fault enclosure 0_1 PSB
    forrest_gump_setClearPePsFault(PS_1, FALSE);

    fbe_api_sleep (9000);           // delay so ps_mgmt can process the same event

    // verify Enclosure Fault LED turned OFF
    forrest_gump_verifyPeEnclFaultLedInfo(LED_BLINK_OFF, FBE_ENCL_FAULT_LED_NO_FLT);


    /*
     * Verify Enclosure Fault LED for DAE PS events
     */
    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    // verify Enclosure Fault LED turned OFF
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing (Power Supplies)\n");
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, FALSE, FBE_ENCL_FAULT_LED_NO_FLT);

    // fault enclosure 0_1 PSA
    forrest_gump_setClearDaePsFault(&location, TRUE);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event

    // verify Enclosure Fault LED turned ON
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, TRUE, FBE_ENCL_FAULT_LED_PS_FLT);
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing - Verified\n");
    
    // clear fault enclosure 0_1 PSA
    forrest_gump_setClearDaePsFault(&location, FALSE);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event

    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    // verify Enclosure Fault LED turned OFF
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, FALSE, FBE_ENCL_FAULT_LED_NO_FLT);
        
// JAP - problems forcing Cooling Faults in Board Object            
#if FALSE
    /*
     * Verify Enclosure Fault LED for Processor Enclosure Cooling events
     */

    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Processor Enclosure Fault LED processing (Cooling)\n");

    // fault enclosure 0_1 Cooling
    forrest_gump_setClearPeCoolingFault(0, TRUE);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event

    // verify Enclosure Fault LED turned ON
    forrest_gump_verifyPeEnclFaultLedInfo(LED_BLINK_ON, FBE_ENCL_FAULT_LED_FAN_FLT);
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Processor Enclosure Fault LED processing - Verified\n");


    // clear fault enclosure 0_1 Cooling
    forrest_gump_setClearPeCoolingFault(0, FALSE);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event

    // verify Enclosure Fault LED turned OFF
    forrest_gump_verifyPeEnclFaultLedInfo(LED_BLINK_OFF, FBE_ENCL_FAULT_LED_NO_FLT);
#endif

    /*
     * Verify Enclosure Fault LED for DAE Cooling events
     */
    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    // verify Enclosure Fault LED turned OFF
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing (PS internal Fan)\n");
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, FALSE, FBE_ENCL_FAULT_LED_NO_FLT);

    // fault enclosure 0_1 PSA
    forrest_gump_setClearDaePsInternalFanFault(&location, TRUE);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event

    // verify Enclosure Fault LED turned ON
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, TRUE, FBE_ENCL_FAULT_LED_PS_FLT);
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing - Verified\n");
    
    // clear fault enclosure 0_1 PSA
    forrest_gump_setClearDaePsInternalFanFault(&location, FALSE);

    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event

    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    location.bus = 0;
    location.enclosure = 1;
    location.slot = 0;
    // verify Enclosure Fault LED turned OFF
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, FALSE, FBE_ENCL_FAULT_LED_NO_FLT);
            
    /*
     * Verify Enclosure Fault LED for DAE Drive Faults
     */
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing (Drives)\n");
    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    location.bus = 0;
    location.port = 0;
    location.enclosure = 1;
    location.slot = 0;

// jap - temporary
#if FALSE
    /*
     * Verify Enclosure Fault LED for DAE Drive Faults
     */
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing (Drives)\n");
    fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
    location.bus = 0;
    location.port = 0;
    location.enclosure = 1;
    location.slot = 0;

    status = fbe_api_get_physical_drive_object_id_by_location(location.port, 
                                                              location.enclosure,
                                                              location.slot, 
                                                              &object_id);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_physical_drive_fail_drive(object_id, 
                                               FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    
    fbe_api_sleep (5000);           // delay so drive_mgmt can process the same event
        
    // verify Enclosure Fault LED turned ON
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, TRUE, FBE_ENCL_FAULT_LED_DRIVE_FLT);
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing - Verified\n");
// jap - temporary
//    status = forrest_gump_remove_drive(location.port, location.enclosure, location.slot);
    
    fbe_api_sleep (5000);           // delay so ps_mgmt can process the same event
        
    // verify Enclosure Fault LED turned ON
    forrest_gump_verifyDaeEnclFaultLedInfo(&location, TRUE, FBE_ENCL_FAULT_LED_DRIVE_FLT);
    mut_printf(MUT_LOG_TEST_STATUS, "FORREST_GUMP:Verify Enclosure (DAE) 0_1 Fault LED processing - Verified\n");
#endif
            
    return;
}
/******************************************
 * end forrest_gump_test()
 ******************************************/
/*!**************************************************************
 * forrest_gump_setup()
 ****************************************************************
 * @brief
 *  Setup for filling the interface structure
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/
void forrest_gump_setup(void)
{
    fbe_test_startEspAndSep_with_common_config_dualSp(FBE_ESP_TEST_SIMPLE_VOYAGER_CONFIG,
                                        SPID_OBERON_1_HW_TYPE,
                                        NULL,
                                        NULL);

    
}
/**************************************
 * end forrest_gump_setup()
 **************************************/

/*!**************************************************************
 * forrest_gump_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the forrest_gump test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *   03/09/2010 - Created. Joe Perry
 *
 ****************************************************************/

void forrest_gump_destroy(void)
{
    fbe_test_esp_delete_registry_file();
    fbe_test_esp_delete_esp_lun();
    fbe_test_sep_util_destroy_neit_sep_physical();
    return;
}
/******************************************
 * end forrest_gump_cleanup()
 ******************************************/
/*************************
 * end file forrest_gump_test.c
 *************************/


