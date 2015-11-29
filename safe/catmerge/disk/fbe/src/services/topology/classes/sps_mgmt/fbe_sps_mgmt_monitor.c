/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sps_mgmt_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the SPS Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_sps_mgmt_monitor_entry "sps_mgmt monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup sps_mgmt_class_files
 * 
 * @version
 *   09-March-2010:  Created. Joe Perry
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_sps_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_power_supply_interface.h"
#include "fbe/fbe_api_sps_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_lifecycle.h"
#include "EspMessages.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_eir_info.h"
#include "fbe_base_environment_debug.h"
#include "HardwareAttributesLib.h"
#include "generic_utils_lib.h"
#include "fbe_base_environment_private.h"


static fbe_sps_mgmt_cmi_msg_t               SPS_MGMT_spsCmiMsg;

// SPS Input Power Data Table
fbe_u32_t fbe_sps_mgmt_inputPowerTable[SPS_TYPE_MAX][7] =
// Columns are the SPS States (Unknonwn, Available, Charging, OnBattery, Faulted, Shutdown, Testing)
{   {   0,   0,   0,   0,  0,  0,  0   },              // Unknown SPS
    {   0,   6,   40,  0,  0,  0,  0   },              // 1.0 KW SPS
    {   0,   6,   40,  0,  0,  0,  0   },              // 1.2 KW SPS
    {   0,  20,  150,  0,  0,  0,  0   },              // 2.2 KW SPS
    {   0,  12,  270,  0,  0,  0,  0   }   };          // Li-ion 2.2 KW SPS

// Array & SPS Model Supported Table
#define FBE_SPS_PLATFORM_COUNT  (sizeof(fbe_sps_mgmt_supportedTable)/sizeof(fbe_sps_platform_support_t))
fbe_sps_platform_support_t fbe_sps_mgmt_supportedTable[] = 
{
    //  Platform                       1.2KW                   2.2KW   
    //-----------                   ---------------         -----------------
    {SPID_PROMETHEUS_M1_HW_TYPE,    FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_YES},
    {SPID_PROMETHEUS_S1_HW_TYPE,    FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_YES},
    {SPID_DEFIANT_M1_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_M2_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_M3_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_M4_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_M5_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_S1_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_S4_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_K2_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_DEFIANT_K3_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_NOVA1_HW_TYPE,            FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_NOVA3_HW_TYPE,            FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_NOVA3_XM_HW_TYPE,         FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_NOVA_S1_HW_TYPE,          FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_OBERON_1_HW_TYPE,         FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_OBERON_2_HW_TYPE,         FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_OBERON_3_HW_TYPE,         FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_OBERON_4_HW_TYPE,         FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_OBERON_S1_HW_TYPE,        FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_HYPERION_1_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_HYPERION_2_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_HYPERION_3_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_TRITON_1_HW_TYPE,         FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_ENTERPRISE_HW_TYPE,       FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_YES},
    {SPID_MERIDIAN_ESX_HW_TYPE,     FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_TUNGSTEN_HW_TYPE,         FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
    {SPID_BEARCAT_HW_TYPE,          FBE_SPS_SUPP_NO,        FBE_SPS_SUPP_NO},
};

typedef enum
{
    SPS_MGMT_SPS_EVENT_MODULE_INSERT = 1,
    SPS_MGMT_SPS_EVENT_BATTERY_INSERT,
    SPS_MGMT_SPS_EVENT_MODULE_BATTERY_INSERT,
    SPS_MGMT_SPS_EVENT_MODULE_REMOVE,
    SPS_MGMT_SPS_EVENT_BATTERY_REMOVE,
} sps_mgmt_sps_module_battery_event_t;

#define FBE_SPS_BATTERY_ONLINE_DEBOUNCE_COUNT   3       // Do not report for at least 15 seconds
#define FBE_SPS_STATUS_DEBOUNCE_COUNT           3       // Do not report for at least 15 seconds
#define FBE_SPS_FAULT_DEBOUNCE_COUNT            2       // Do not report for consecutive polls at lower levels
#define FBE_SPS_STATUS_IGNORE_AFTER_LCC_FUP_COUNT   6   // Do not report for 30 seconds after LCC FUP completed

static fbe_status_t fbe_sps_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context);

static fbe_status_t fbe_sps_mgmt_handle_generic_info_change(fbe_sps_mgmt_t * sps_mgmt, 
                                                          update_object_msg_t * update_object_msg);

static fbe_status_t fbe_sps_mgmt_handle_object_data_change(fbe_sps_mgmt_t * pSpsMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg);

static fbe_status_t fbe_sps_mgmt_fup_handle_firmware_status_change(fbe_sps_mgmt_t * pSpsMgmt,
                                                               fbe_u64_t deviceType, 
                                                               fbe_device_physical_location_t * pLocation);

static void fbe_sps_mgmt_processPsStatusChange(fbe_sps_mgmt_t *sps_mgmt, 
                                               fbe_object_id_t object_id,
                                               fbe_device_physical_location_t *location);
static fbe_lifecycle_status_t fbe_sps_mgmt_init_test_time_cond_function(fbe_base_object_t * base_object, 
                                                                        fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_check_test_time_cond_function(fbe_base_object_t * base_object, 
                                                                         fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_sps_test_check_needed_cond_function(fbe_base_object_t * base_object, 
                                                                               fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_check_stop_test_cond_function(fbe_base_object_t * base_object, 
                                                                         fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_sps_mfg_info_needed_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_debounce_timer_cond_function(fbe_base_object_t * base_object, 
                                                                        fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_hung_sps_test_timer_cond_function(fbe_base_object_t * base_object, 
                                                                             fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_test_needed_cond_function(fbe_base_object_t * base_object, 
                                                                     fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_sendCmiMessage(fbe_sps_mgmt_t *sps_mgmt, fbe_u32_t MsgType);
static fbe_status_t fbe_sps_mgmt_initiateTest(fbe_sps_mgmt_t *sps_mgmt);
static fbe_status_t fbe_sps_mgmt_initiateSpsTest(fbe_sps_mgmt_t *sps_mgmt);
static fbe_status_t fbe_sps_mgmt_initiateBbuTest(fbe_sps_mgmt_t *sps_mgmt);
static fbe_bool_t fbe_sps_mgmt_hasTestArrived(fbe_system_time_t *spsTestTime);
static fbe_status_t fbe_sps_mgmt_processNotPresentStatus(fbe_sps_mgmt_t *sps_mgmt,
                                                         fbe_sps_ps_encl_num_t spsLocation,
                                                         sps_mgmt_sps_module_battery_event_t spsEvent);
static fbe_status_t fbe_sps_mgmt_processBatteryOkStatus(fbe_sps_mgmt_t *sps_mgmt,
                                                        fbe_sps_ps_encl_num_t spsLocation,
                                                        fbe_sps_info_t *previousSpsInfo);
static fbe_status_t fbe_sps_mgmt_processRechargingStatus(fbe_sps_mgmt_t *sps_mgmt,
                                                         fbe_sps_ps_encl_num_t spsLocation,
                                                         fbe_sps_info_t *previousSpsInfo);
static fbe_status_t fbe_sps_mgmt_processTestingStatus(fbe_sps_mgmt_t *sps_mgmt,
                                                      fbe_sps_info_t *previousSpsInfo);
static fbe_status_t fbe_sps_mgmt_processBatteryOnlineStatus(fbe_sps_mgmt_t *sps_mgmt,
                                                            fbe_sps_ps_encl_num_t spsLocation,
                                                            fbe_sps_info_t *previousSpsInfo);
static fbe_status_t fbe_sps_mgmt_processFaultedStatus(fbe_sps_mgmt_t *sps_mgmt,
                                                      fbe_sps_ps_encl_num_t spsLocation,
                                                      fbe_sps_info_t *previousSpsInfo);
static fbe_status_t fbe_sps_mgmt_processInsertedSps(fbe_sps_mgmt_t *sps_mgmt,
                                                    fbe_sps_ps_encl_num_t spsLocation,
                                                    sps_mgmt_sps_module_battery_event_t spsEvent);
static fbe_status_t fbe_sps_mgmt_processInsertedBbu(fbe_sps_mgmt_t *sps_mgmt, fbe_device_physical_location_t *location);
static fbe_status_t fbe_sps_mgmt_processSpsInputPower(fbe_sps_mgmt_t *sps_mgmt);
fbe_sps_cabling_status_t fbe_sps_mgmt_checkArrayCablingStatus(fbe_sps_mgmt_t *sps_mgmt,
                                                              fbe_bool_t testComplete);
const char *fbe_sps_mgmt_getSpsMsgTypeString (fbe_u32_t MsgType);
const char *fbe_sps_mgmt_getCmiEventTypeString (fbe_cmi_event_t cmiEventType);
void fbe_sps_mgmt_processPeerSpsInfo(fbe_sps_mgmt_t *sps_mgmt, 
                                     fbe_bool_t peerSpGone,
                                     fbe_sps_mgmt_cmi_msg_t *cmiMsgPtr);
const char *fbe_sps_mgmt_getSpsTestStateString (fbe_sps_testing_state_t spsTestState);
void fbe_sps_mgmt_determineSpsInputPower(fbe_bool_t spsInserted,
                                         SPS_STATE spsStatus,
                                         SPS_TYPE spsModel,
                                         fbe_eir_input_power_sample_t *inputPowerSample);
void fbe_sps_mgmt_checkSpsCacheStatus(fbe_sps_mgmt_t *sps_mgmt);

void fbe_sps_mgmt_determineAcDcArrayType(fbe_sps_mgmt_t *sps_mgmt);
void fbe_sps_mgmt_determineExpectedPsAcFails(fbe_sps_mgmt_t *sps_mgmt, fbe_sps_ps_encl_num_t spsToTest);
fbe_common_cache_status_t fbe_sps_mgmt_spsCheckSpsCacheStatus(fbe_sps_mgmt_t *sps_mgmt);
fbe_common_cache_status_t fbe_sps_mgmt_bobCheckSpsCacheStatus(fbe_sps_mgmt_t *sps_mgmt);
static fbe_status_t fbe_sps_mgmt_spsDiscovery(fbe_sps_mgmt_t *sps_mgmt);
static fbe_status_t fbe_sps_mgmt_bobDiscovery(fbe_sps_mgmt_t *sps_mgmt);
static void fbe_sps_mgmt_analyzeSpsFaultInfo(fbe_sps_fault_info_t *spsFaultInfo);
static void fbe_sps_mgmt_analyzeSpsModel(fbe_sps_mgmt_t *sps_mgmt,
                                         fbe_sps_info_t *spsStatus);
static fbe_bool_t fbe_sps_mgmt_okToStartSpsTest(fbe_sps_mgmt_t *sps_mgmt, fbe_sps_ps_encl_num_t *spsEnclToTest);
static fbe_bool_t fbe_sps_mgmt_okToStartBbuTest(fbe_sps_mgmt_t *sps_mgmt, fbe_device_physical_location_t * pLocation);
static fbe_lifecycle_status_t fbe_sps_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, 
                                                                                       fbe_packet_t * packet);
static fbe_bool_t 
fbe_sps_mgmt_get_battery_logical_fault(fbe_base_battery_info_t *bob_info);
fbe_status_t fbe_sps_mgmt_update_encl_fault_led(fbe_sps_mgmt_t *pSpsMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason);
static fbe_lifecycle_status_t fbe_sps_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_process_peer_cache_status_update(fbe_sps_mgmt_t *sps_mgmt, 
                                                                  fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr);
static fbe_status_t fbe_sps_mgmt_process_peer_bbu_status(fbe_sps_mgmt_t *sps_mgmt, 
                                                         fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr);
static fbe_status_t fbe_sps_mgmt_process_peer_disk_battery_backed_set_info(fbe_sps_mgmt_t *sps_mgmt, 
                                                         fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr);
static fbe_bool_t fbe_sps_mgmt_anySpsFaults(fbe_sps_mgmt_t *sps_mgmt, fbe_bool_t processorEncl);
static fbe_bool_t fbe_sps_mgmt_anyBbuFaults(fbe_sps_mgmt_t *sps_mgmt);
static fbe_status_t fbe_sps_mgmt_get_bm_lcc_info(fbe_sps_mgmt_t * pSpsMgmt, SP_ID sp, fbe_lcc_info_t * pLccInfo);
static fbe_bool_t fbe_sps_mgmt_okToShutdownSps(fbe_sps_mgmt_t *sps_mgmt, fbe_sps_ps_encl_num_t spsEnclIndex);
static void sps_mgmt_restart_env_failed_fups(fbe_sps_mgmt_t *pSpsMgmt, fbe_u32_t spsEnclIndex, fbe_u32_t spsSideIndex);
static fbe_status_t fbe_sps_mgmt_sendSpsMfgInfoRequest(fbe_sps_mgmt_t *sps_mgmt,
                                                       fbe_sps_ps_encl_num_t spsLocation,
                                                       fbe_sps_get_sps_manuf_info_t *spsManufInfo);


static fbe_status_t fbe_sps_mgmt_processNewEnclosureSps(fbe_sps_mgmt_t *sps_mgmt, fbe_object_id_t objectId);
static fbe_status_t fbe_sps_mgmt_processRemovedEnclosureSps(fbe_sps_mgmt_t *sps_mgmt, fbe_object_id_t objectId);

static fbe_status_t fbe_sps_mgmt_get_current_sps_info(fbe_sps_mgmt_t * sps_mgmt,
                                       fbe_device_physical_location_t * pLocation,
                                       fbe_sps_info_t * pCurrentSpsInfo);

static fbe_status_t fbe_sps_mgmt_sps_compare_firmware_rev(fbe_sps_mgmt_t * sps_mgmt,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_sps_info_t * pCurrentSpsInfo,
                                                   fbe_sps_info_t * pPreviousSpsInfo,
                                                   fbe_bool_t * pRevChanged);

static fbe_status_t fbe_sps_mgmt_sps_update_debounced_status(fbe_sps_mgmt_t * sps_mgmt,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_sps_info_t * pCurrentSpsInfo);

static fbe_status_t fbe_sps_mgmt_sps_check_state_change(fbe_sps_mgmt_t * sps_mgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_sps_info_t * pCurrentSpsInfo,
                                                 fbe_sps_info_t * pPreviousSpsInfo,
                                                 fbe_bool_t * pSpsStateChanged);
static fbe_status_t 
fbe_sps_mgmt_calculateBatteryStateInfo(fbe_sps_mgmt_t *sps_mgmt,
                                       fbe_device_physical_location_t *pLocation,
                                       fbe_u32_t bbuSlotIndex);
static fbe_status_t fbe_sps_mgmt_verifyRideThroughTimeout(fbe_sps_mgmt_t *sps_mgmt);

/*!***************************************************************
 * fbe_sps_mgmt_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the SPS Management monitor.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_sps_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_sps_mgmt_t * sps_mgmt = NULL;

	sps_mgmt = (fbe_sps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_topology_class_trace(FBE_CLASS_ID_SPS_MGMT,
			     FBE_TRACE_LEVEL_DEBUG_HIGH, 
			     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
			     "%s entry\n", __FUNCTION__);
                 
	return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt),
	                                  (fbe_base_object_t*)sps_mgmt, packet);
}
/******************************************
 * end fbe_sps_mgmt_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_sps_mgmt_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the SPS management.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt));
}
/******************************************
 * end fbe_sps_mgmt_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_sps_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_discover_sps_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_process_sps_state_change_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_sps_mgmt_discover_ps_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context);

static fbe_status_t fbe_sps_mgmt_cmi_message_handler(fbe_base_object_t *pBaseObject, 
                                                     fbe_base_environment_cmi_message_info_t *pCmiMsg);
static fbe_status_t fbe_sps_mgmt_processReceivedCmiMessage(fbe_sps_mgmt_t * pSpsMgmt, 
                                                           fbe_sps_mgmt_cmi_packet_t *pSpsCmiPkt);
static fbe_status_t fbe_sps_mgmt_processPeerNotPresent(fbe_sps_mgmt_t * pSpsMgmt, 
                                                       fbe_sps_mgmt_cmi_packet_t *pSpsCmiPkt);
static fbe_status_t fbe_sps_mgmt_processContactLost(fbe_sps_mgmt_t * pSpsMgmt);
static fbe_status_t fbe_sps_mgmt_processPeerBusy(fbe_sps_mgmt_t * pSpsMgmt, 
                                                 fbe_sps_mgmt_cmi_packet_t * pSpsCmiPkt);
static fbe_status_t fbe_sps_mgmt_processFatalError(fbe_sps_mgmt_t * pSpsMgmt, 
                                                   fbe_sps_mgmt_cmi_packet_t * pSpsCmiPkt);
/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sps_mgmt);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sps_mgmt);

/*  sps_mgmt_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sps_mgmt) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		sps_mgmt,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};


/*--- constant derived condition entries -----------------------------------------------*/
/* FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE condition
 *  - The purpose of this derived condition is to handle the completion
 *    of reading the data from persistent storage.  
 */
static fbe_lifecycle_const_cond_t fbe_sps_mgmt_read_persistent_data_complete_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE,
		fbe_sps_mgmt_read_persistent_data_complete_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the FBE API. The leaf class needs to
 *  implement the actual condition handler function providing
 *  the callback that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_sps_mgmt_register_state_change_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
		fbe_sps_mgmt_register_state_change_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_sps_mgmt_register_cmi_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        fbe_sps_mgmt_register_cmi_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK condition
 * - The purpose of this derived condition is to register
 * call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_sps_mgmt_register_resume_prom_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
        fbe_sps_mgmt_register_resume_prom_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_register_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
        fbe_sps_mgmt_fup_register_callback_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_release_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE,
        fbe_base_env_fup_release_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_get_download_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
        fbe_base_env_fup_get_download_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_get_activate_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
        fbe_base_env_fup_get_activate_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_abort_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
        fbe_base_env_fup_abort_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_wait_before_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
        fbe_base_env_fup_wait_before_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_wait_for_inter_device_delay_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
        fbe_base_env_fup_wait_for_inter_device_delay_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_read_image_header_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
        fbe_base_env_fup_read_image_header_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_check_rev_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
        fbe_base_env_fup_check_rev_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_read_entire_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
        fbe_base_env_fup_read_entire_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_download_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
        fbe_base_env_fup_download_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_get_peer_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
        fbe_base_env_fup_get_peer_permission_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_check_env_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
        fbe_base_env_fup_check_env_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_activate_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
        fbe_base_env_fup_activate_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_check_result_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
        fbe_base_env_fup_check_result_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_refresh_device_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
        fbe_base_env_fup_refresh_device_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_sps_mgmt_fup_end_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
        fbe_base_env_fup_end_upgrade_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

/* FBE_SPS_MGMT_DISCOVER_SPS condition -
 *   The purpose of this condition is start the discovery process
 *   of a new SPS that was added
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_sps_mgmt_discover_sps_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_sps_mgmt_discover_sps_cond",
        FBE_SPS_MGMT_DISCOVER_SPS,
        fbe_sps_mgmt_discover_sps_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_SPS_MGMT_DISCOVER_PS condition -
 *   The purpose of this condition is start the discovery process
 *   of PS's that are needed for SPS testing
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_sps_mgmt_discover_ps_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_sps_mgmt_discover_ps_cond",
        FBE_SPS_MGMT_DISCOVER_PS,
        fbe_sps_mgmt_discover_ps_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_SPS_MGMT_INIT_TEST_TIME_COND condition 
 *   The purpose of this condition is to set the timer to check for 
 *   the weekly SPS Test time.
 */
static fbe_lifecycle_const_base_cond_t fbe_sps_mgmt_init_test_time_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_sps_mgmt_init_test_time_cond",
        FBE_SPS_MGMT_INIT_TEST_TIME_COND,
        fbe_sps_mgmt_init_test_time_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_SPS_MGMT_CHECK_TEST_TIME_COND condition 
 *   The purpose of this condition is to check if we have arrived at
 *   the weekly SPS Test time.
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_sps_mgmt_check_test_time_cond = {
	{
		FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
			"fbe_sps_mgmt_check_test_time_cond",
			FBE_SPS_MGMT_CHECK_TEST_TIME_COND,
			fbe_sps_mgmt_check_test_time_cond_function),
		FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
			FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
			FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
			FBE_LIFECYCLE_STATE_READY,          /* READY */
			FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
			FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
			FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
			FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
	},
	FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
	3000 /* fires every 30 seconds */
};

/* FBE_SPS_MGMT_TEST_NEEDED_COND condition 
 *   The purpose of this condition is to determine if we can start testing the SPS
 */
static fbe_lifecycle_const_base_cond_t fbe_sps_mgmt_test_needed_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
            "fbe_sps_mgmt_test_needed_cond",
            FBE_SPS_MGMT_TEST_NEEDED_COND,
            fbe_sps_mgmt_test_needed_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,       /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,          /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,            /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_SPS_MGMT_SPS_TEST_CHECK_NEEDED_COND condition 
 *   The purpose of this condition is to start a timer condition that will
 *   periocially check the status of the SPS Test
 */
static fbe_lifecycle_const_base_cond_t fbe_sps_mgmt_sps_test_check_needed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_sps_mgmt_sps_test_check_needed_cond",
        FBE_SPS_MGMT_SPS_TEST_CHECK_NEEDED_COND,
        fbe_sps_mgmt_sps_test_check_needed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,       /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_SPS_MGMT_CHECK_STOP_TEST_COND condition 
 *   The purpose of this condition is to check if we can stop the SPS Test
 *   that is in progress.
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_sps_mgmt_check_stop_test_cond = {
	{
		FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
			"fbe_sps_mgmt_check_stop_test_cond",
			FBE_SPS_MGMT_CHECK_STOP_TEST_COND,
			fbe_sps_mgmt_check_stop_test_cond_function),
		FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
			FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
			FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
			FBE_LIFECYCLE_STATE_READY,          /* READY */
			FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
			FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
			FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
			FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
	},
	FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
	1000 /* fires every 10 seconds */
};


/* FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND condition 
 *   The purpose of this condition is to request SPS Mfg Info from Phy Pkg
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_sps_mgmt_sps_mfg_info_needed_timer_cond = {
	{
		FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
			"fbe_sps_mgmt_sps_mfg_info_needed_timer_cond",
			FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND,
			fbe_sps_mgmt_sps_mfg_info_needed_timer_cond_function),
		FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
			FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
			FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
			FBE_LIFECYCLE_STATE_READY,          /* READY */
			FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
			FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
			FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
			FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
	},
	FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
	100 /* fires every second */
};

/* FBE_SPS_MGMT_PROCESS_SPS_STATUS_CHANGE_COND condition 
 *   The purpose of this condition is to process SPS Status change
 *   (Usurper command may have caused a change)
 */
static fbe_lifecycle_const_base_cond_t fbe_sps_mgmt_process_sps_status_change_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_sps_mgmt_process_sps_state_change_cond",
        FBE_SPS_MGMT_PROCESS_SPS_STATUS_CHANGE_COND,
        fbe_sps_mgmt_process_sps_state_change_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_SPS_MGMT_DEBOUNCE_TIMER_COND condition 
 *   The purpose of this condition is verify a status has existed for a
 *   certain amount of time
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_sps_mgmt_debounce_timer_cond = {
	{
		FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
			"fbe_sps_mgmt_debounce_timer_cond",
			FBE_SPS_MGMT_DEBOUNCE_TIMER_COND,
			fbe_sps_mgmt_debounce_timer_cond_function),
		FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
			FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
			FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
			FBE_LIFECYCLE_STATE_READY,          /* READY */
			FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
			FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
			FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
			FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
	},
	FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
	500 /* fires every 5 seconds */
};

/* FBE_SPS_MGMT_CHECK_FOR_HUNG_SPS_TEST_TIMER_COND condition 
 *   The purpose of this condition is check if the SPS's are stuck in a state
 *   were they need to perform an SPS Test, but have not started.
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_sps_mgmt_hung_sps_test_timer_cond = {
	{
		FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
			"fbe_sps_mgmt_hung_sps_test_timer_cond",
			FBE_SPS_MGMT_HUNG_SPS_TEST_TIMER_COND,
			fbe_sps_mgmt_hung_sps_test_timer_cond_function),
		FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
			FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
			FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
			FBE_LIFECYCLE_STATE_READY,          /* READY */
			FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
			FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
			FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
			FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
	},
	FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
	3000 /* fires every 30 seconds */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sps_mgmt)[] = {
    &fbe_sps_mgmt_discover_sps_cond,
    &fbe_sps_mgmt_discover_ps_cond,
    &fbe_sps_mgmt_init_test_time_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_sps_mgmt_check_test_time_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_sps_mgmt_sps_test_check_needed_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_sps_mgmt_check_stop_test_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_sps_mgmt_debounce_timer_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_sps_mgmt_hung_sps_test_timer_cond,
    &fbe_sps_mgmt_test_needed_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_sps_mgmt_sps_mfg_info_needed_timer_cond,
    &fbe_sps_mgmt_process_sps_status_change_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sps_mgmt);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t fbe_sps_mgmt_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_register_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_register_resume_prom_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_discover_sps_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_discover_ps_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_sps_mgmt_activate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t fbe_sps_mgmt_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_wait_before_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* Moved up fbe_sps_mgmt_sps_mfg_info_needed_timer_cond to get the mfg info earlier.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_sps_mfg_info_needed_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_release_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Moved the following two conditions up. (AR651486)
     * so that work item can be removed from the queue immediately after the device gets removed.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_refresh_device_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_end_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_wait_for_inter_device_delay_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_read_image_header_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_check_rev_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_read_entire_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_get_peer_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_check_env_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_download_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_get_download_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_activate_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_get_activate_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_check_result_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_init_test_time_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_check_test_time_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_sps_test_check_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_check_stop_test_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_debounce_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_hung_sps_test_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_test_needed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_process_sps_status_change_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Need to put the abort condition in the end of all the fup conditions. 
     * We want to complete the execution of other condition which was already set before running the abort upgrade 
     * condition. Otherwise, when resuming the upgrade, the condition which was set before would get to run and 
     * it would make the upgrade out of sequence.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_sps_mgmt_fup_abort_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), 
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sps_mgmt)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, fbe_sps_mgmt_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, fbe_sps_mgmt_activate_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, fbe_sps_mgmt_ready_rotary)
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sps_mgmt);

/*--- global sps mgmt lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sps_mgmt) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		sps_mgmt,
		FBE_CLASS_ID_SPS_MGMT,                  /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_environment))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_sps_mgmt_register_state_change_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on SPS object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the SPS management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_sps_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, 
                                                          fbe_packet_t * packet)
{
	fbe_status_t    status;
    fbe_sps_mgmt_t  *sps_mgmt = (fbe_sps_mgmt_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

	status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}
    /* Register with the base environment for the conditions we care
     * about.  We only care about Enclosure events if we have SPS's connected to DAE0.
     */
    if (sps_mgmt->encls_with_sps > 1)
    {
        status = fbe_base_environment_register_event_notification((fbe_base_environment_t *) base_object,
                                                                  (fbe_api_notification_callback_function_t)fbe_sps_mgmt_state_change_handler,
                                                                  (FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY|FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL|FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY|FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED),
                                                                  (FBE_DEVICE_TYPE_SPS | FBE_DEVICE_TYPE_PS | FBE_DEVICE_TYPE_BATTERY | FBE_DEVICE_TYPE_BACK_END_MODULE | FBE_DEVICE_TYPE_ENCLOSURE),
                                                                  base_object,
                                                                  (FBE_TOPOLOGY_OBJECT_TYPE_BOARD | FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE),
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL);
    }
    else
    {
        status = fbe_base_environment_register_event_notification((fbe_base_environment_t *) base_object,
                                                                  (fbe_api_notification_callback_function_t)fbe_sps_mgmt_state_change_handler,
                                                                  FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  (FBE_DEVICE_TYPE_SPS | FBE_DEVICE_TYPE_PS | FBE_DEVICE_TYPE_BATTERY | FBE_DEVICE_TYPE_BACK_END_MODULE | FBE_DEVICE_TYPE_MISC| FBE_DEVICE_TYPE_BMC),
                                                                  base_object,
                                                                  (FBE_TOPOLOGY_OBJECT_TYPE_BOARD | FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE),
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL);
    }
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s API callback init failed, status: 0x%X",
								__FUNCTION__, status);
	}
	return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_sps_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_register_cmi_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on SPS object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the SPS management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_sps_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
	fbe_status_t status;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

	status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}
    /* Register with the CMI for the conditions we care about.
     */
	status = fbe_base_environment_register_cmi_notification((fbe_base_environment_t *)base_object,
															FBE_CMI_CLIENT_ID_SPS_MGMT,
															fbe_sps_mgmt_cmi_message_handler);

	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s API callback init failed, status: 0x%X",
								__FUNCTION__, status);
	}
	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_sps_mgmt_register_resume_prom_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the cooling_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Encl management's constant
 *                        lifecycle data.
 *
 * @author
 *  08-Oct-2012: Rui Chang - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_sps_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);

    fbe_base_env_initiate_resume_prom_read_callback(pBaseEnv, &fbe_sps_mgmt_initiate_resume_prom_read);
    fbe_base_env_get_resume_prom_info_ptr_callback(pBaseEnv, (fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t)&fbe_sps_mgmt_get_resume_prom_info_ptr);
    fbe_base_env_resume_prom_update_encl_fault_led_callback(pBaseEnv, (fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t) &fbe_sps_mgmt_resume_prom_update_encl_fault_led);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "RP: %s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
    }
   
    return FBE_LIFECYCLE_STATUS_DONE;
} // fbe_sps_mgmt_register_resume_prom_callback_cond_function


/*******************************************************************
 * end 
 * fbe_sps_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_fup_register_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the PS management's constant
 *                        lifecycle data.
 *
 * @author
 *  02-July-2010: PHE - Created. 
 ****************************************************************/
static fbe_lifecycle_status_t fbe_sps_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, 
                                                                               fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    fbe_base_env_set_fup_get_full_image_path_callback(pBaseEnv, &fbe_sps_mgmt_fup_get_full_image_path);
    fbe_base_env_set_fup_check_env_status_callback(pBaseEnv, &fbe_sps_mgmt_fup_check_env_status);
    fbe_base_env_set_fup_info_ptr_callback(pBaseEnv, &fbe_sps_mgmt_get_component_fup_info_ptr);
    fbe_base_env_set_fup_info_callback(pBaseEnv, &fbe_sps_mgmt_get_fup_info);
    fbe_base_env_set_fup_get_firmware_rev_callback(pBaseEnv, &fbe_sps_mgmt_fup_get_firmware_rev);
    fbe_base_env_set_fup_refresh_device_status_callback(pBaseEnv, &fbe_sps_mgmt_fup_refresh_device_status);
    fbe_base_env_set_fup_initiate_upgrade_callback(pBaseEnv, &fbe_sps_mgmt_fup_initiate_upgrade);
    fbe_base_env_set_fup_resume_upgrade_callback(pBaseEnv, &fbe_sps_mgmt_fup_resume_upgrade);
    fbe_base_env_set_fup_priority_check_callback(pBaseEnv, (void *)NULL);
    fbe_base_env_set_fup_copy_fw_rev_to_resume_prom(pBaseEnv, (void *)NULL);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "FUP: %s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
   
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_state_change_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for state change notification.
 *  
 * @param update_object_msg - 
 * @param context - This is the object handle, or in our case the sps_mgmt.
 *
 * @return fbe_status_t -  
 * 
 * @author
 *  05-Oct-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context)
{
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_sps_mgmt_t          *sps_mgmt;
    fbe_base_object_t       *base_object;
    fbe_status_t            status = FBE_STATUS_OK;

    sps_mgmt = (fbe_sps_mgmt_t *)context;
    base_object = (fbe_base_object_t *)sps_mgmt;
    
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, notifType 0x%llx, objType 0x%llx, devMsk 0x%llx\n",
                          __FUNCTION__,
			  (unsigned long long)notification_info->notification_type, 
                          (unsigned long long)notification_info->object_type, 
                          notification_info->notification_data.data_change_info.device_mask);

    /*
     * Process the notification
     */

    switch (notification_info->notification_type)
    { 
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:
            status = fbe_sps_mgmt_handle_object_data_change(sps_mgmt, update_object_msg);
            break;

        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY:
            // only care about Enclosure events
            if (notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE)
            {
                // discover new SPS
                fbe_sps_mgmt_processNewEnclosureSps(sps_mgmt, update_object_msg->object_id);
            }
            break;

        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY:
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL:
            if (notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE)
            {
                // process removed SPS
                fbe_sps_mgmt_processRemovedEnclosureSps(sps_mgmt, update_object_msg->object_id);
            }
            break;

        default:
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unhandled noticiation type:%d\n",
                                  __FUNCTION__, (int)notification_info->notification_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/*******************************************************************
 * end fbe_sps_mgmt_state_change_handler() 
 *******************************************************************/

/*!**************************************************************
 * @fn fbe_sps_mgmt_handle_object_data_change()
 ****************************************************************
 * @brief
 *  This function is to handle the object data change.
 *
 * @param pSpsMgmt - This is the object handle, or in our case the sps_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  05-Oct-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_handle_object_data_change(fbe_sps_mgmt_t * pSpsMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t * pDataChangedInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    
    fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, dataType 0x%x.\n",
                          __FUNCTION__, pDataChangedInfo->data_type);
    
    switch(pDataChangedInfo->data_type)
    {
        case FBE_DEVICE_DATA_TYPE_GENERIC_INFO:
            status = fbe_sps_mgmt_handle_generic_info_change(pSpsMgmt, pUpdateObjectMsg);
            break;

        case FBE_DEVICE_DATA_TYPE_FUP_INFO:
            if(pDataChangedInfo->device_mask == FBE_DEVICE_TYPE_SPS)
            {
                status = fbe_sps_mgmt_fup_handle_firmware_status_change(pSpsMgmt,
                                                                    pDataChangedInfo->device_mask,
                                                                    &(pDataChangedInfo->phys_location));
            }
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Object data changed, unhandled data type %d.\n",
                                  __FUNCTION__, 
                                  pDataChangedInfo->data_type);
            break;
    }

    return status;
}// End of function fbe_sps_mgmt_handle_object_data_change

/*!**************************************************************
 * @fn fbe_sps_mgmt_handle_generic_info_change()
 ****************************************************************
 * @brief
 *  This function is to handle the generic info change.
 *
 * @param sps_mgmt - This is the object handle.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *  05-Oct-2012:  PHE - Renamed the function from fbe_sps_mgmt_state_change_handler.
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_handle_generic_info_change(fbe_sps_mgmt_t * sps_mgmt, 
                                                          update_object_msg_t * update_object_msg)
{
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_base_object_t       *base_object;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_device_physical_location_t  location;

    base_object = (fbe_base_object_t *)sps_mgmt;

    /*
     * Process the notification
     */
    switch (notification_info->notification_type)
    {
    case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:

        switch (notification_info->object_type)
        {
        /*
         * Process Data Changes to Board Object
         */
        case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s BoardObject DataChanged, deviceMask 0x%llx\n",
                                  __FUNCTION__,
                                  notification_info->notification_data.data_change_info.device_mask);
            switch (notification_info->notification_data.data_change_info.device_mask)
            {
            case FBE_DEVICE_TYPE_SPS:
                // SPS State change reported by Board Object
                status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, 
                                                       FBE_SPS_MGMT_PE, 
                                                       notification_info->notification_data.data_change_info.phys_location.slot);
                if (status != FBE_STATUS_OK) 
                {
        	    	fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting SPS Status, status: 0x%X\n",
                                          __FUNCTION__, status);
            	}
                break;

            case FBE_DEVICE_TYPE_BATTERY:
                // SPS State change reported by Board Object
                status = fbe_sps_mgmt_processBobStatus(sps_mgmt,
                                                       &(notification_info->notification_data.data_change_info.phys_location)); 
                if (status != FBE_STATUS_OK) 
                {
        	    	fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting Battery Status, status: 0x%X\n",
                                          __FUNCTION__, status);
            	}
                break;

            case FBE_DEVICE_TYPE_PS:
               fbe_sps_mgmt_processPsStatusChange(sps_mgmt, 
                                                  update_object_msg->object_id,
                                                  &(notification_info->notification_data.data_change_info.phys_location)); 
                break;

            case FBE_DEVICE_TYPE_MISC:
                // check if Cache status changed
                fbe_sps_mgmt_checkSpsCacheStatus(sps_mgmt);
                // update Enclosure Fault LED for DPE
                fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
                status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                            &location, 
                                                            FBE_ENCL_FAULT_LED_BATTERY_FLT);
                break;

            case FBE_DEVICE_TYPE_BMC:
                // check if Ride Through Timeout changed (may need to reset)
                fbe_sps_mgmt_verifyRideThroughTimeout(sps_mgmt);
                break;

            case FBE_DEVICE_TYPE_BACK_END_MODULE:
            default:
                break;
            }
            break;

        /*
         * Process Data Changes to Enclosure Object
         */
        case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
            switch (notification_info->notification_data.data_change_info.device_mask)
            {
            case FBE_DEVICE_TYPE_SPS:
                // SPS State change reported by Enclosure Object (check if this platform uses SPS)
                if (sps_mgmt->total_sps_count > 2)
                {
                    status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, 
                                                           FBE_SPS_MGMT_DAE0,
                                                           notification_info->notification_data.data_change_info.phys_location.slot);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s Error getting SPS Status, status: 0x%X\n",
                                              __FUNCTION__, status);
                    }
                }
                break;
            case FBE_DEVICE_TYPE_PS:
                // only care about PS state change (AC_FAIL) to DAE0 while Testing SPS
                if ((notification_info->notification_data.data_change_info.phys_location.bus == 0) &&
                    (notification_info->notification_data.data_change_info.phys_location.enclosure == 0))
                {
                   fbe_sps_mgmt_processPsStatusChange(sps_mgmt, 
                                                      update_object_msg->object_id,
                                                      &(notification_info->notification_data.data_change_info.phys_location)); 
                }
                break;

            case FBE_DEVICE_TYPE_BACK_END_MODULE:
                // check if Cache status changed
                fbe_sps_mgmt_checkSpsCacheStatus(sps_mgmt);
                // update Enclosure Fault LED for DPE
                fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
                status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                            &location, 
                                                            FBE_ENCL_FAULT_LED_BATTERY_FLT);

                break;

            default:
                break;
            }

        default:

            break;
        }
        break;

    default:
        break;
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_handle_firmware_status_change()
 ****************************************************************
 * @brief
 *  This function is to handle the generic info change.
 *
 * @param pSpsMgmt - This is the object handle.
 * @param deviceType -
 * @param pLocation - Pointer to the location.
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *  05-Oct-2012:  PHE - Renamed the function from fbe_sps_mgmt_state_change_handler.
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_fup_handle_firmware_status_change(fbe_sps_mgmt_t * pSpsMgmt, 
                                                               fbe_u64_t deviceType,
                                                               fbe_device_physical_location_t * pLocation)
{
    fbe_base_env_fup_work_item_t   * pWorkItem = NULL;
    fbe_status_t                     status = FBE_STATUS_OK;

    // We should have done this in the physical package. But fbe_sps_mgmt_processSpsStatus expects
    // the slot as 0. So I can not change it now. - PINGHE
    pLocation->slot = (pSpsMgmt->base_environment.spid == SP_A)? 0 : 1;

    fbe_base_env_fup_get_first_work_item((fbe_base_environment_t *)pSpsMgmt, &pWorkItem);

    // Set the componentId
    if(pWorkItem != NULL)
    {
        pLocation->componentId = pWorkItem->location.componentId;
    }
                                                 
    // process the fup change
    status = fbe_base_env_fup_handle_firmware_status_change((fbe_base_environment_t *)pSpsMgmt,
                                                            deviceType,
                                                            pLocation);

    return status;
}

/*!**************************************************************
 * fbe_sps_mgmt_cmi_message_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for CMI message notification.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 * 
 * 
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *  12-Oct-2012: PHE - created the functions for each CMI event.
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_cmi_message_handler(fbe_base_object_t *pBaseObject, 
                                fbe_base_environment_cmi_message_info_t *pCmiMsg)
{
    fbe_sps_mgmt_t                      * pSpsMgmt = (fbe_sps_mgmt_t *)pBaseObject;
    fbe_sps_mgmt_cmi_packet_t           * pSpsCmiPkt = NULL;
    fbe_base_environment_cmi_message_t * pBaseCmiPkt = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    // client message (if it exists)
    pSpsCmiPkt = (fbe_sps_mgmt_cmi_packet_t *)pCmiMsg->user_message;
    pBaseCmiPkt = (fbe_base_environment_cmi_message_t *)pSpsCmiPkt;

    if(pBaseCmiPkt == NULL) 
    {
        fbe_base_object_trace(pBaseObject, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Event %s(0x%x), no msgType.\n",
                              __FUNCTION__,
                              fbe_sps_mgmt_getCmiEventTypeString(pCmiMsg->event),
                              pCmiMsg->event);
    }
    else
    {
        fbe_base_object_trace(pBaseObject, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Event %s(0x%x), msgType %s(0x%x).\n",
                              __FUNCTION__,
                              fbe_sps_mgmt_getCmiEventTypeString(pCmiMsg->event),
                              pCmiMsg->event, 
                              fbe_sps_mgmt_getSpsMsgTypeString(pBaseCmiPkt->opcode),
                              pBaseCmiPkt->opcode);
    }

    /*
     * process the message based on type
     */
    switch (pCmiMsg->event)
    {
    case FBE_CMI_EVENT_MESSAGE_RECEIVED:
        status = fbe_sps_mgmt_processReceivedCmiMessage(pSpsMgmt, pSpsCmiPkt);
        break;

    case FBE_CMI_EVENT_PEER_NOT_PRESENT:
        status =  fbe_sps_mgmt_processPeerNotPresent(pSpsMgmt, pSpsCmiPkt);
        break;

    case FBE_CMI_EVENT_SP_CONTACT_LOST:         // jap - should this be treated like PEER_NOT_PRESENT?
                                                // brion - not entirely, there is no message associated with the event
        status = fbe_sps_mgmt_processContactLost(pSpsMgmt);
        break;

    case FBE_CMI_EVENT_PEER_BUSY:
        status = fbe_sps_mgmt_processPeerBusy(pSpsMgmt, pSpsCmiPkt);
        break;

    case FBE_CMI_EVENT_FATAL_ERROR:
        status = fbe_sps_mgmt_processFatalError(pSpsMgmt, pSpsCmiPkt);
        break;

    default:
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        break;
    }

    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        // handle the message in base env
        status = fbe_base_environment_cmi_message_handler(pBaseObject, pCmiMsg);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(pBaseObject, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to handle CMI msg.\n",
                               __FUNCTION__);
        }
    }

	return status;
}

/*******************************************************************
 * end fbe_sps_mgmt_cmi_message_handler() 
 *******************************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_processReceivedCmiMessage()
 ****************************************************************
 * @brief
 *  This function will handle SPS related CMI messsage received
 *  from peer SP.
 *
 * @param pSpsMgmt - pointer to SPS Mgmt info.
 * @param pSpsCmiPkt - pointer to user info of CMI message.
 *
 * @return fbe_status_t - The status of whether message was
 *                        successfully processed.
 * 
 * @author
 *  11-June-2010:  Created. Joe Perry
 *  12-Oct-2012: PHE - Added the SPS firmware upgrade related opcode.
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processReceivedCmiMessage(fbe_sps_mgmt_t * sps_mgmt, 
                                      fbe_sps_mgmt_cmi_packet_t *pSpsCmiPkt)
{
    fbe_base_environment_t             * pBaseEnv = (fbe_base_environment_t *)sps_mgmt;
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pSpsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pSpsCmiPkt;
    fbe_sps_mgmt_cmi_msg_t             * cmiMsgPtr = (fbe_sps_mgmt_cmi_msg_t *)pSpsCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_u32_t                           spsIndex;
    fbe_u32_t                           bbuIndex;

    switch (pBaseCmiMsg->opcode)
    {
    case FBE_BASE_ENV_CMI_MSG_REQ_SPS_INFO:
        // peer is requesting our local SPS info, so send it
        status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error sending SPS Info to peer, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        break;

    case FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO:
        if (sps_mgmt->total_sps_count > 0)
        {
            fbe_sps_mgmt_processPeerSpsInfo(sps_mgmt, FALSE, cmiMsgPtr);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, no SPS's & received SendSpsInfo msg\n",
                                  __FUNCTION__);
        }
        break;

    case FBE_BASE_ENV_CMI_MSG_TEST_REQ_PERMISSION:
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s (ReqPerm), spsTestState %d, spid %d, testBothSps %d\n",
                              __FUNCTION__, 
                              sps_mgmt->testingState,
                              sps_mgmt->base_environment.spid, cmiMsgPtr->testBothSps);
        // peer is asking for permission to start an SPS Test
        if ((sps_mgmt->testingState == FBE_SPS_NO_TESTING_IN_PROGRESS) ||
            (sps_mgmt->testingState == FBE_SPS_PEER_SPS_TESTING) ||
            ((sps_mgmt->testingState == FBE_SPS_WAIT_FOR_PEER_PERMISSION) &&
             (sps_mgmt->base_environment.spid == SP_B)))
        {
            status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_TEST_REQ_ACK);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error sending SPS Test Req Ack to peer, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s SpsTestState change, %s to %s\n",
                                      __FUNCTION__, 
                                      fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                                      fbe_sps_mgmt_getSpsTestStateString(FBE_SPS_PEER_SPS_TESTING));
                sps_mgmt->testingState = FBE_SPS_PEER_SPS_TESTING;
            }

            // check if this SPS needs testing too (scheduled SPS Test)
            if (cmiMsgPtr->testBothSps)
            {
                if (sps_mgmt->total_sps_count > 0 && !sps_mgmt->arraySpsInfo->testingCompleted)
                {
                    sps_mgmt->arraySpsInfo->spsTestTimeDetected = TRUE;
                    for (spsIndex = 0; spsIndex < sps_mgmt->encls_with_sps; spsIndex++)
                    {
                        sps_mgmt->arraySpsInfo->sps_info[spsIndex][FBE_LOCAL_SPS].needToTest = TRUE;
                    }
                    sps_mgmt->arraySpsInfo->spsTestType = FBE_SPS_BATTERY_TEST_TYPE;
                    sps_mgmt->arraySpsInfo->needToTest = TRUE;
                    sps_mgmt->arraySpsInfo->testingCompleted = FALSE;
                    fbe_sps_mgmt_determineExpectedPsAcFails(sps_mgmt, FBE_SPS_MGMT_PE);      // recalculate test data
                }
                
                if (sps_mgmt->total_bob_count > 0 && !sps_mgmt->arrayBobInfo->testingCompleted)
                {
                    sps_mgmt->arraySpsInfo->spsTestTimeDetected = TRUE;
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, BBU Self Test not supported, ffid 0x%x\n",
                                          __FUNCTION__, 
                                          sps_mgmt->arrayBobInfo->bob_info[sps_mgmt->base_environment.spid].batteryFfid);
                    for (bbuIndex=0;bbuIndex<sps_mgmt->total_bob_count;bbuIndex++) 
                    {
                        if ((sps_mgmt->arrayBobInfo->bob_info[bbuIndex].associatedSp == sps_mgmt->base_environment.spid) &&
                            HwAttrLib_isBbuSelfTestSupported(sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFfid))
                        {
                            sps_mgmt->arrayBobInfo->needToTest = TRUE;
                            sps_mgmt->arrayBobInfo->bobNeedsTesting[bbuIndex] = TRUE;
                            sps_mgmt->arrayBobInfo->testingCompleted = FALSE;

                        }
                        else
                        {
                            sps_mgmt->arrayBobInfo->bobNeedsTesting[bbuIndex] = FALSE;
                            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                                  FBE_TRACE_LEVEL_WARNING,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s, BBU Self Test not supported, ffid 0x%x\n",
                                                  __FUNCTION__, 
                                                  sps_mgmt->arrayBobInfo->bob_info[sps_mgmt->base_environment.spid].batteryFfid);
                        }
                    }
                }
            }
        }
        else
        {
            status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_TEST_REQ_NACK);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error sending SPS Test Req Nack to peer, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
        break;

    case FBE_BASE_ENV_CMI_MSG_TEST_REQ_ACK:
        // peer has granted us permission to start testing our SPS
        fbe_sps_mgmt_initiateTest(sps_mgmt);
        break;

    case FBE_BASE_ENV_CMI_MSG_TEST_REQ_NACK:
        // peer has rejected us permission to start testing our SPS, check again later
        break;

    case FBE_BASE_ENV_CMI_MSG_TEST_COMPLETED:
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s SpsTestState change, %s to %s\n",
                              __FUNCTION__, 
                              fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                              fbe_sps_mgmt_getSpsTestStateString(FBE_SPS_NO_TESTING_IN_PROGRESS));
        sps_mgmt->testingState = FBE_SPS_NO_TESTING_IN_PROGRESS;
        // peer has completed tested his SPS, request status of the test from peer
        status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_REQ_SPS_INFO);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error requesting SPS (Testing) Info to peer, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        // set condition to signify that we need to test the SPS
        if (((sps_mgmt->total_sps_count > 0) && sps_mgmt->arraySpsInfo->needToTest) || 
            ((sps_mgmt->total_bob_count > 0) && sps_mgmt->arrayBobInfo->needToTest))
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, spsCnt %d, needToTest %d, bobCnt %d, needToTest %d\n",
                                  __FUNCTION__,
                                  sps_mgmt->total_sps_count, sps_mgmt->arraySpsInfo->needToTest,
                                  sps_mgmt->total_bob_count, sps_mgmt->arrayBobInfo->needToTest);
            status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                            (fbe_base_object_t *)sps_mgmt,
                                            FBE_SPS_MGMT_TEST_NEEDED_COND);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't set TestNeeded condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, %s, set TestNeeded condition\n",
                                      __FUNCTION__,
                                      fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState));
            }
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s, no further Testing needed\n",
                                  __FUNCTION__,
                                  fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState));
            sps_mgmt->arraySpsInfo->spsTestTimeDetected = FALSE;
        }
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        // peer has granted permission to fup
        status = fbe_base_env_fup_processGotPeerPerm(pBaseEnv, 
                                                     pFupCmiPkt->pRequestorWorkItem);
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
        // peer has denied permission to sart fup
        status = fbe_base_env_fup_processDeniedPeerPerm(pBaseEnv,
                                                        pFupCmiPkt->pRequestorWorkItem);
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
        // peer requests permission to fup
        status = fbe_base_env_fup_processRequestForPeerPerm(pBaseEnv, pFupCmiPkt);
        break;

    case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
        status = fbe_sps_mgmt_process_peer_cache_status_update(sps_mgmt, 
                                                               pSpsCmiPkt);
        break;

    case FBE_BASE_ENV_CMI_MSG_PEER_BBU_TEST_STATUS:
        status = fbe_sps_mgmt_process_peer_bbu_status(sps_mgmt, pSpsCmiPkt);
        break;

    case FBE_BASE_ENV_CMI_MSG_PEER_DISK_BATTERY_BACKED_SET:
        status = fbe_sps_mgmt_process_peer_disk_battery_backed_set_info(sps_mgmt, pSpsCmiPkt);
        break;

    case FBE_BASE_ENV_CMI_MSG_REQ_PEER_DISK_BATTERY_BACKED_SET:
        status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_PEER_DISK_BATTERY_BACKED_SET);
        break;

    case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
        // check if our earlier perm request failed
        status = fbe_sps_mgmt_fup_new_contact_init_upgrade(sps_mgmt);
        break;

    default:
        status = fbe_base_environment_cmi_process_received_message((fbe_base_environment_t *)sps_mgmt, 
                                                                   (fbe_base_environment_cmi_message_t *) cmiMsgPtr);
        break;
    }

	return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_sps_mgmt_processReceivedCmiMessage() 
 *******************************************************************/


/*!**************************************************************
 *  @fn fbe_sps_mgmt_processPeerNotPresent(fbe_sps_mgmt_t * pSpsMgmt, 
 *                                         fbe_sps_mgmt_cmi_packet_t *pSpsCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
 *
 * @param pSpsMgmt - 
 * @param pSpsCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  12-Oct-2012: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processPeerNotPresent(fbe_sps_mgmt_t * pSpsMgmt, 
                                   fbe_sps_mgmt_cmi_packet_t *pSpsCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pSpsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pSpsCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_TEST_REQ_PERMISSION:
            // if trying to start SPS test, go ahead if peer SP not present
            fbe_sps_mgmt_initiateTest(pSpsMgmt);
            status = FBE_STATUS_OK;
            break;
        
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            // Peer SP is not present when sending the peermission grant or deny.
            status = FBE_STATUS_OK;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_processGotPeerPerm((fbe_base_environment_t *)pSpsMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
     
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_processContactLost(fbe_sps_mgmt_t * pSpsMgmt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_SP_CONTACT_LOST.
 *
 * @param pSpsMgmt - 
 *
 * @return fbe_status_t - always return
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 * 
 * @author
 *  12-Oct-2012: PHE - Created.
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_processContactLost(fbe_sps_mgmt_t * pSpsMgmt)
{
    if (pSpsMgmt->total_sps_count > 0)
    {
        fbe_sps_mgmt_processPeerSpsInfo(pSpsMgmt, TRUE, NULL);
    }

    /* No need to handle the fup work items here. If the contact is lost, we will also receive
     * Peer Not Present message and we will handle the fup work item there. 
     */

    /* Return FBE_STATUS_MORE_PROCESSING_REQUIRED so that it will do further processing in base_environment.*/
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_processPeerBusy(fbe_sps_mgmt_t * pSpsMgmt, 
 *                           fbe_sps_mgmt_cmi_packet_t * pSpsCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_BUSY.
 *
 * @param pSpsMgmt - 
 * @param pSpsCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  12-Oct-2012: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processPeerBusy(fbe_sps_mgmt_t * pSpsMgmt, 
                            fbe_sps_mgmt_cmi_packet_t * pSpsCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pSpsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pSpsCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, should never happen, opcode 0x%x\n",
                          __FUNCTION__,
                          pBaseCmiMsg->opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_peerPermRetry((fbe_base_environment_t *)pSpsMgmt, 
                                                    pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }
    
    return(status);
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_processFatalError(fbe_ps_mgmt_t * pPsMgmt, 
 *                                   fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_FATAL_ERROR.
 *
 * @param pSpsMgmt - 
 * @param pSpsCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  12-Oct-2012: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processFatalError(fbe_sps_mgmt_t * pSpsMgmt, 
                              fbe_sps_mgmt_cmi_packet_t * pSpsCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pSpsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pSpsCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            /* No need to retry.
             * The peer did not receive the response in certain period time,
             * it will do whatever it needs to do. 
             */ 
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pSpsMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}


/*!**************************************************************
 * fbe_sps_mgmt_discover_sps_cond_function()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with SPS)
 *  and adds them to the queue to be processed later
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *

 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_discover_sps_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_object_id_t                             object_id;
    fbe_u32_t                                   index;
    fbe_u32_t                                   bbuCount;
    fbe_status_t                                status;
    fbe_base_board_get_base_board_info_t        *baseBoardInfoPtr = NULL;
    fbe_sps_mgmt_t                              *sps_mgmt = (fbe_sps_mgmt_t *) base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) 
    {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

    /*
     * Get initial SPS Status
     */
    // Get the Board Object ID 
    status = fbe_api_get_board_object_id(&object_id);
    if (status != FBE_STATUS_OK) 
    {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Error in getting the Board Object ID, status: 0x%X\n",
								__FUNCTION__, status);
        sps_mgmt->object_id = 0;
        // mark SPS's or BoB's not present (only local SPS's)
        sps_mgmt->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsModulePresent = FALSE;
        sps_mgmt->arraySpsInfo->sps_info[FBE_SPS_MGMT_DAE0][FBE_LOCAL_SPS].spsBatteryPresent = FALSE;
        bbuCount = (sps_mgmt->total_bob_count == FBE_SPS_MGMT_BBU_COUNT_TBD) ? FBE_BOB_MAX_COUNT : sps_mgmt->total_bob_count;
        for (index = 0; index < bbuCount; index++)
        {
            sps_mgmt->arrayBobInfo->bob_info[index].batteryInserted = FALSE;
        }
	}
    else
    {
        // Get initial MfgMode setting
        baseBoardInfoPtr = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)sps_mgmt, sizeof(fbe_base_board_get_base_board_info_t));
        if(baseBoardInfoPtr == NULL)
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to allocate for board object information.\n",
                                  __FUNCTION__);
        }
        else
        {
            fbe_zero_memory(baseBoardInfoPtr, sizeof(fbe_base_board_get_base_board_info_t));
            fbe_api_board_get_base_board_info(object_id, baseBoardInfoPtr);
            if (baseBoardInfoPtr->mfgMode)
            {
                sps_mgmt->arraySpsInfo->simulateSpsInfo = TRUE;
                sps_mgmt->arrayBobInfo->simulateBobInfo = TRUE;
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, MfgMode set (simulate SPS)\n",
                                      __FUNCTION__);
            }
            fbe_base_env_memory_ex_release((fbe_base_environment_t *)sps_mgmt, baseBoardInfoPtr);
        }

        // Determine if this array uses SPS's or BoB's
        if (sps_mgmt->total_sps_count > 0)
        {
            sps_mgmt->arraySpsInfo->spsObjectId[FBE_SPS_MGMT_PE] = object_id;
            if (sps_mgmt->encls_with_sps > 1)
            {
                status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_id);
                if (status == FBE_STATUS_OK)
                {
                    sps_mgmt->arraySpsInfo->spsObjectId[FBE_SPS_MGMT_DAE0] = object_id;
                }
            }
            fbe_sps_mgmt_spsDiscovery(sps_mgmt);
        }
        else if (sps_mgmt->total_bob_count > 0)
        {
            sps_mgmt->arrayBobInfo->bobObjectId = object_id;
            fbe_sps_mgmt_bobDiscovery(sps_mgmt);
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_sps_mgmt_discover_sps_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_spsDiscovery()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with SPS)
 *  and adds them to the queue to be processed later
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *

 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_spsDiscovery(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_base_object_t                           *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_status_t                                status;
    fbe_u32_t                                   spsIndex, spsEnclIndex;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
    {
        for (spsIndex = 0; spsIndex < sps_mgmt->sps_per_side; spsIndex++)
        {
            // Get initial SPS status
            status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, spsEnclIndex, spsIndex);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Error getting SPS %d %d , status: 0x%X\n",
                                        __FUNCTION__, spsEnclIndex, spsIndex, status);
                return status;
            }
            else
            {
                // setup so SPS is tested
                sps_mgmt->arraySpsInfo->needToTest = TRUE;
                sps_mgmt->arraySpsInfo->testingCompleted = FALSE;
                sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].needToTest = TRUE;

                // Get Manufacturing Info from new SPS (set flag & start timer)
                sps_mgmt->arraySpsInfo->mfgInfoNeeded[spsEnclIndex] = TRUE;
                status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                        base_object, 
                                                        FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, %s, can't clear SPS Mfg Info timer condition, status: 0x%X\n",
                                          __FUNCTION__, 
                                          sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                                          status);
                }
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, SPS %d %d: ModPres %d, BattPres %d, status 0x%x, count %d\n",
                                      __FUNCTION__,
                                      spsEnclIndex,
                                      spsIndex,
                                      sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsModulePresent,
                                      sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsBatteryPresent,
                                      sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsStatus,
                                      sps_mgmt->total_sps_count);
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, SPS %d %d: Manuf, SN %s, PN %s\n",
                                      __FUNCTION__,
                                      spsEnclIndex,
                                      spsIndex,
                                      sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsManufInfo.spsModuleManufInfo.spsSerialNumber,
                                      sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsManufInfo.spsModuleManufInfo.spsPartNumber);
            }
        }
    }

    /*
     * Get intial peer SPS Status
     */
    status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_REQ_SPS_INFO);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting peer SPS Info, status: 0x%X\n",
                              __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_OK;

}
/******************************************************
 * end fbe_sps_mgmt_spsDiscovery() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_bobDiscovery()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with SPS)
 *  and adds them to the queue to be processed later
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *

 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_bobDiscovery(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_u32_t                                   realBobCount = 0;
    fbe_u32_t                                   bobIndex;
    fbe_status_t                                status;
    fbe_device_physical_location_t              location;
    fbe_board_mgmt_platform_info_t              platformInfo;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    // need to determine if single SP array
    status = fbe_api_board_get_platform_info(sps_mgmt->object_id, &platformInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get platform info, status: 0x%X",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get initial BoB status
    status = fbe_api_enclosure_get_battery_count(sps_mgmt->arrayBobInfo->bobObjectId, &realBobCount);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Error getting BoB count, status: 0x%X\n",
                                __FUNCTION__, status);
        return status;
    }

    // check if single SP array (need to adjust count to avoid missing BBU from causing a fault)
    if (platformInfo.is_single_sp_system)
    {
        sps_mgmt->total_bob_count = realBobCount / 2;
    }
    else
    {
        sps_mgmt->total_bob_count = realBobCount;
    }
    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, BBU count %d, isSingleSp %d\n",
                            __FUNCTION__, sps_mgmt->total_bob_count, platformInfo.is_single_sp_system);
    sps_mgmt->arrayBobInfo->needToTest = TRUE;
    sps_mgmt->arrayBobInfo->testingCompleted = FALSE;

    //send CMI command to get peer's diskBatteryBackedSet status.
    status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_REQ_PEER_DISK_BATTERY_BACKED_SET);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending FBE_BASE_ENV_CMI_MSG_REQ_PEER_DISK_BATTERY_BACKED_SET info to peer, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
    for (bobIndex = 0; bobIndex < sps_mgmt->total_bob_count; bobIndex++)
    {
        if (platformInfo.is_single_sp_system)
        {
            location.bus = 0;
            location.enclosure = 0;
            location.sp = SP_A;
            location.slot = bobIndex; 
        }
        else
        {
    // ENCL_CLEANUP - need to handle xPE for Triton
            location.bus = 0;
            location.enclosure = 0;
            if (bobIndex < (sps_mgmt->total_bob_count/2))
            {
                location.sp = SP_A;
                location.slot = bobIndex; 
            }
            else
            {
                location.sp = SP_B;
                location.slot = bobIndex - (sps_mgmt->total_bob_count/2); 
            }
        }
        status = fbe_sps_mgmt_processBobStatus(sps_mgmt, &location);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Error getting BoB Status, status: 0x%X\n",
                                    __FUNCTION__, status);
            return status;
        }
        // mark local BBU's for testing if they support it
        if ((sps_mgmt->arrayBobInfo->bob_info[bobIndex].associatedSp == sps_mgmt->base_environment.spid) &&
            HwAttrLib_isBbuSelfTestSupported(sps_mgmt->arrayBobInfo->bob_info[bobIndex].batteryFfid))
        {
            sps_mgmt->arrayBobInfo->bobNeedsTesting[bobIndex] = TRUE; 
            sps_mgmt->arrayBobInfo->needToTest = TRUE;
        }

    }

    // update Enclosure Fault LED based off initial BBU status
    status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                &location, 
                                                FBE_ENCL_FAULT_LED_BATTERY_FLT);

    return FBE_STATUS_OK;

}
/******************************************************
 * end fbe_sps_mgmt_bobDiscovery() 
 ******************************************************/
   
/*!**************************************************************
 * fbe_sps_mgmt_discover_ps_cond_function()
 ****************************************************************
 * @brief
 *  This function gets initial PS status from enclosures that
 *  are relevant to SPS Testing.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *

 * @author
 *  22-Mar-2011:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_discover_ps_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_object_id_t                             object_id;
    fbe_u32_t                                   psIndex, psCount;
    fbe_status_t                                status;
    fbe_device_physical_location_t              location;
    fbe_sps_mgmt_t                              *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_enclosure_types_t                       enclType = FBE_ENCL_TYPE_INVALID;

    memset(&location, 0, sizeof(location)); // SAFEBUG - uninitialized fields seen to be used
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) 
    {
		fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
	}

    /*
     * Get initial PE PS's Status
     */

    if (sps_mgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE)
    {
        location.bus = 0;
        location.enclosure = 0;
    }
    else
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    // Get the Board Object ID 
    status = fbe_api_get_board_object_id(&object_id);
    if (status != FBE_STATUS_OK) 
    {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Error in getting PE Board Object ID, status: 0x%X\n",
								__FUNCTION__, status);
	}
    else
    {
        status = fbe_api_power_supply_get_ps_count(object_id, &psCount);
        if (status == FBE_STATUS_OK)
        {
            sps_mgmt->arraySpsInfo->psEnclCounts[FBE_SPS_MGMT_PE] = psCount;
            for (psIndex = 0; psIndex < psCount; psIndex++)
            {
                location.slot = psIndex;
                fbe_sps_mgmt_processPsStatusChange(sps_mgmt, object_id, &location);
            }
        }
    }

    /*
     * Get initial DAE0 PS's Status if needed
     */
    if (sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE)
    {
        // initialize PS's to not inserted
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            sps_mgmt->arraySpsInfo->ps_info[FBE_SPS_MGMT_DAE0][psIndex].psInserted = FALSE;
        }

        location.bus = 0;
        location.enclosure = 0;
        // Get the Enclosure Object ID 
        status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_id);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Error in getting DAE0 Encl Object ID, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
        else
        {
            // determine the enclosure type of DAE0 (some have different cabling)
            status = fbe_api_enclosure_get_encl_type(object_id, &enclType);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Getting enclosure type failed for objectId %d, status 0x%x.\n",
                                      __FUNCTION__, object_id, status);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else
            {
                if (enclType == FBE_ENCL_SAS_VOYAGER_ICM)
                {
                    sps_mgmt->arraySpsInfo->spsTestConfig = FBE_SPS_TEST_CONFIG_XPE_VOYAGER;
                }
                else if (enclType == FBE_ENCL_SAS_VIKING_IOSXP)
                {
                    sps_mgmt->arraySpsInfo->spsTestConfig = FBE_SPS_TEST_CONFIG_XPE_VIKING;
                }
            }

            status = fbe_api_power_supply_get_ps_count(object_id, &psCount);
            if (status == FBE_STATUS_OK)
            {
                sps_mgmt->arraySpsInfo->psEnclCounts[FBE_SPS_MGMT_DAE0] = psCount;
                for (psIndex = 0; psIndex < psCount; psIndex++)
                {
                    location.slot = psIndex;
                    fbe_sps_mgmt_processPsStatusChange(sps_mgmt, object_id, &location);
                }
            }
        }
    }

    /*
     * Determine if this is an AC or DC powered array
     */
    fbe_sps_mgmt_determineAcDcArrayType(sps_mgmt);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_sps_mgmt_discover_ps_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_processSpsStatus()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with SPS)
 *  and adds them to the queue to be processed later
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *  14-May-2013:  PHE - Broke down the function to make it easier to maintain. 
 *
 ****************************************************************/
fbe_status_t 
fbe_sps_mgmt_processSpsStatus(fbe_sps_mgmt_t *sps_mgmt, 
                              fbe_sps_ps_encl_num_t spsEnclIndex,
                              fbe_u8_t spsIndex)
{
    fbe_base_object_t                     * base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sps_info_t                          previousSpsInfo = {0};
    fbe_sps_info_t                        * pPreviousSpsInfo = NULL;
    fbe_sps_info_t                        * pCurrentSpsInfo = NULL;
    fbe_bool_t                              spsStateChange = FALSE;
    fbe_bool_t                              revChange = FALSE;
    fbe_device_physical_location_t          location = {0};
    fbe_device_physical_location_t          xpeLocation = {0};
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};

    // verify valid SPS indexes
    if (sps_mgmt->total_sps_count == 0)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, no SPS's, no processing to do\n", 
                              __FUNCTION__); 
        return FBE_STATUS_OK;
    }
    else if ((spsEnclIndex >= sps_mgmt->encls_with_sps) || (spsIndex >= FBE_SPS_MAX_COUNT))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Invalid parameters, spsEnclIndex %d, spsIndex %d\n", 
                              __FUNCTION__, spsEnclIndex, spsIndex); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if(spsIndex == FBE_PEER_SPS)
    {
        /* Processor Enclosure's peer SPS status comes from CMI messaging, exit. */
        return FBE_STATUS_OK;
    }
    /*
     * Generate device strings
     */
    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));

    status = fbe_sps_mgmt_convertSpsIndex(sps_mgmt, spsEnclIndex, spsIndex, &location);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,failed to covertSpsIndex, spsEnclIndex %d, spsIndex %d.\n",
                              __FUNCTION__, spsEnclIndex, spsIndex);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               &location, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
        return status;
    }

    // save old SPS info
    previousSpsInfo = sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex];
    pPreviousSpsInfo = &previousSpsInfo;

    // get the current SPS info
    pCurrentSpsInfo = &(sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex]);
    status = fbe_sps_mgmt_get_current_sps_info(sps_mgmt, &location, pCurrentSpsInfo);
    
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s get_current_sps_info failed,status 0x%X.\n",
                                  &spsDeviceStr[0],
                                  __FUNCTION__, 
                                  status);
    }

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %s, spsModPres %d, spsBatPres %d, SpsStatus %s, SpsFaults %s\n",
                          &spsDeviceStr[0],
                          __FUNCTION__, 
                          pCurrentSpsInfo->spsModulePresent,
                          pCurrentSpsInfo->spsBatteryPresent,
                          fbe_sps_mgmt_getSpsStatusString(pCurrentSpsInfo->spsStatus),
                          fbe_sps_mgmt_getSpsFaultString(&pCurrentSpsInfo->spsFaultInfo));

    fbe_sps_mgmt_analyzeSpsModel(sps_mgmt, pCurrentSpsInfo);

    status = fbe_sps_mgmt_sps_update_debounced_status(sps_mgmt,
                                                      &location,
                                                      pCurrentSpsInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s update_debounced_status failed,status 0x%X.\n",
                                  &spsDeviceStr[0],
                                  __FUNCTION__, 
                                  status);
    }
    
    status = fbe_sps_mgmt_sps_compare_firmware_rev(sps_mgmt, 
                                                   &location, 
                                                   pCurrentSpsInfo, 
                                                   pPreviousSpsInfo, 
                                                   &revChange);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s compare_firmware_rev failed,status 0x%X.\n",
                                  &spsDeviceStr[0],
                                  __FUNCTION__, 
                                  status);
    }

    status = fbe_sps_mgmt_sps_check_state_change(sps_mgmt,
                                                 &location,
                                                 pCurrentSpsInfo,
                                                 pPreviousSpsInfo,
                                                 &spsStateChange);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s check_state_change failed,status 0x%X.\n",
                                  &spsDeviceStr[0],
                                  __FUNCTION__, 
                                  status);
    }

    status = fbe_sps_mgmt_fup_handle_sps_status_change(sps_mgmt,
                                                       &location,
                                                       pCurrentSpsInfo,
                                                       pPreviousSpsInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s fup_handle_sps_status_change failed, status 0x%X.\n",
                                  &spsDeviceStr[0],
                                  __FUNCTION__, status);
    }

    // check if SPS Cache status changed
    fbe_sps_mgmt_checkSpsCacheStatus(sps_mgmt);

    // update Enclosure Fault LED
    status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                &location, 
                                                FBE_ENCL_FAULT_LED_SPS_FLT);
    // if DAE0 SPS status changed, the Cache Status may have changed which affects xPE Flt LED
    if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
        (location.bus == 0) && (location.enclosure == 0))
    {
        fbe_zero_memory(&xpeLocation, sizeof(fbe_device_physical_location_t));
        xpeLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        xpeLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        xpeLocation.slot = location.slot;
        status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                    &xpeLocation, 
                                                    FBE_ENCL_FAULT_LED_SPS_FLT);
    }

    if (revChange)
    {
        // get Manufacturing Info if SPS FW revisions changed 
        sps_mgmt->arraySpsInfo->mfgInfoNeeded[spsEnclIndex] = TRUE;
        status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                base_object, 
                                                FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s, can't clear SPS Mfg Info timer condition, status: 0x%X\n",
                                  &spsDeviceStr[0],
                                  __FUNCTION__, 
                                  status);
        }
    }

    /*
     * Notify peer SP of SPS change and no message already in progress
     * and send notification for sps_mgmt data change
     */
    if ((spsStateChange) || (revChange))
    {
        status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s,%s Error sending SPS Info to peer, status 0x%X.\n",
                                  &spsDeviceStr[0],
                                  __FUNCTION__, 
                                  status);
        }

        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, SPS data change notify, StateChange:%s RevChange:%s.\n",
                              &spsDeviceStr[0],
                              __FUNCTION__, 
                              (spsStateChange ? "TRUE" : "FALSE"),
                              (revChange ? "TRUE" : "FALSE"));

        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)sps_mgmt,
                                                           FBE_CLASS_ID_SPS_MGMT, 
                                                           FBE_DEVICE_TYPE_SPS,
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &location);
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_sps_mgmt_processSpsStatus() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_processPsStatusChange()
 ****************************************************************
 * @brief
 *  This function processes a PS State Change by:
 *      -request PS Status from Phy Pkg
 *      -check for AC_FAIL events
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  27-Jul-2010:  Created. Joe Perry
 *
 ****************************************************************/
static void fbe_sps_mgmt_processPsStatusChange(fbe_sps_mgmt_t *sps_mgmt, 
                                               fbe_object_id_t object_id,
                                               fbe_device_physical_location_t *location)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_status_t                            status;
    fbe_power_supply_info_t                 psStatus;
    fbe_bool_t                              processorEnclosure = FALSE;
    fbe_bool_t                              needToTestSps = FALSE;
    fbe_bool_t                              logSpsConfigChange = FALSE;
    fbe_device_physical_location_t          sps_location = {0};
    fbe_u32_t                               enclIndex, spsIndex;
    fbe_u32_t                               psSlot;
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_bool_t                              psRemovedFaulted = FALSE;
    fbe_bool_t                              psInsertedUnfaulted = FALSE;
    fbe_u32_t                               psIndex;
    fbe_u32_t                               localPsCount = 0;
    fbe_u32_t                               localGoodPsCount = 0;
    fbe_u32_t                               localBadPsCount = 0;

    /*
     * If no SPS's, we don't care about PS changes (only used for SPS Testing)
     */
    if (sps_mgmt->total_sps_count == 0)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, no SPS's,ignore PS change\n",
                              __FUNCTION__);
        return;
    }

    /*
     * Request PS Status
     */
    status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)sps_mgmt, 
                                                       object_id, 
                                                       location);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get and adjust location for objectId %d failed, status: 0x%x.\n",
                              __FUNCTION__, object_id, status);
        return;
    }
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, obj 0x%x, bus %d, encl %d, slot %d\n",
                          __FUNCTION__, object_id, 
                          location->bus, location->enclosure, location->slot);

    if (((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
         (location->bus == FBE_XPE_PSEUDO_BUS_NUM) && (location->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
        ((sps_mgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) &&
             (location->bus == 0) && (location->enclosure == 0)))
    {
        processorEnclosure = TRUE;
        enclIndex = FBE_SPS_MGMT_PE;
    }
    else
    {
        enclIndex = FBE_SPS_MGMT_DAE0;
    }

    // get PS status
    fbe_set_memory(&psStatus, 0, sizeof(fbe_power_supply_info_t));
    psStatus.slotNumOnEncl = location->slot;
    status = fbe_api_power_supply_get_power_supply_info(object_id, &psStatus);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting PS Status, slot %d, status: 0x%X\n",
                              __FUNCTION__, psStatus.slotNumOnEncl, status);
        return;
    }

    spsIndex = psStatus.associatedSps;

    /*
     * Check & record AC_FAIL events if SPS Testing in progress
     */
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s (SPS_TEST), Old spsStatus %d, PE %d, slot %d, ins %d, flt %d, acfl %d\n",
                          __FUNCTION__, 
                          sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsStatus,
                          processorEnclosure,
                          location->slot,
                          sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psInserted,
                          sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psFault,
                          sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psAcFail);
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s (SPS_TEST), New spsStatus %d, PE %d, slot %d, ins %d, flt %d, acfl %d\n",
                          __FUNCTION__, 
                          sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsStatus,
                          processorEnclosure,
                          location->slot,
                          psStatus.bInserted,
                          psStatus.generalFault,
                          psStatus.ACFail);

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, encl %d, slot %d, , SpsTesting %d, AcFail %d, Flt %d\n",
                          __FUNCTION__, 
                          enclIndex, location->slot,
                          sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsStatus,
                          psStatus.ACFail,
                          psStatus.generalFault);

    if (sps_mgmt->testingState == FBE_SPS_LOCAL_SPS_TESTING)
    {
        if (psStatus.generalFault)
        {
            //we need to stop cabling test since PS fault.
            fbe_base_object_trace(base_object, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s, encl %d, slot %d, AcFail %d, Flt %d. Will stop sps cabling test. \n",
                      __FUNCTION__, 
                      enclIndex, 
                      location->slot,
                      psStatus.ACFail,
                      psStatus.generalFault);
        }
        else if (psStatus.ACFail)
        {
            // Voyager DAE0 has PS's with two AC_FAIL's
            if ((enclIndex == FBE_SPS_MGMT_DAE0) &&
                (sps_mgmt->arraySpsInfo->spsTestConfig == FBE_SPS_TEST_CONFIG_XPE_VOYAGER))
            {
                if (location->slot == 0)
                {
                    psSlot = 0;
                }
                else
                {
                    psSlot = 2;
                }
                sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psSlot].acFailDetected = 
                    psStatus.ACFailDetails[0];
                sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psSlot+1].acFailDetected = 
                    psStatus.ACFailDetails[1];
            }
            else
            {
                sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][location->slot].acFailDetected = TRUE;
            }

            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s (SPS_TEST), %s PS %d, AC_FAIL detected\n",
                                  __FUNCTION__, 
                                  (processorEnclosure ? "PE" : "DAE0"),
                                   location->slot);
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d, slot %d, AcFailDetected %d %d %d %d\n",
                                  __FUNCTION__, 
                                  enclIndex, location->slot,
                                  sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][0].acFailDetected,
                                  sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][1].acFailDetected,
                                  sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][2].acFailDetected,
                                  sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][3].acFailDetected);

        }
    }

    /*
     * Check for Power Supply configuration changes (only for PS related to this side's SPS)
     */
    if (((psStatus.associatedSps == FBE_SPS_A) && (sps_mgmt->base_environment.spid == SP_A)) ||
        ((psStatus.associatedSps == FBE_SPS_B) && (sps_mgmt->base_environment.spid == SP_B)))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Old spsStatus %d, PE %d, slot %d, assocSps %d, ins %d, flt %d\n",
                              __FUNCTION__, 
                              sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsStatus,
                              processorEnclosure,
                              location->slot,
                              sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].associatedSps,
                              sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psInserted,
                              sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psFault);
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, New spsStatus %d, PE %d, slot %d, assocSps %d, ins %d, flt %d\n",
                              __FUNCTION__, 
                              sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsStatus,
                              processorEnclosure,
                              location->slot,
                              psStatus.associatedSps,
                              psStatus.bInserted,
                              psStatus.generalFault);

        // PS became Inserted
        if (psStatus.bInserted && 
            !(sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psInserted))
        {
            psInsertedUnfaulted = TRUE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s PS %d Inserted, psInsertedUnfaulted set\n",
                                  __FUNCTION__, 
                                  (processorEnclosure ? "PE" : "DAE0"),
                                  location->slot);
        }
        // PS became Removed
        else if (!psStatus.bInserted && 
                  sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psInserted)
        {
            psRemovedFaulted = TRUE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s,  %s PS %d Removed, psRemovedFaulted set\n",
                                  __FUNCTION__, 
                                  (processorEnclosure ? "PE" : "DAE0"),
                                  location->slot);
        }
        // PS Fault detected
        if (psStatus.generalFault && 
            !(sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psFault))
        {
            psRemovedFaulted = TRUE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s PS %d Fault Detected, psRemovedFaulted set\n",
                                  __FUNCTION__, 
                                  (processorEnclosure ? "PE" : "DAE0"),
                                  location->slot);
        }
        // PS Fault cleared
        else if (!psStatus.generalFault && 
                 sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psFault)
        {
            psInsertedUnfaulted = TRUE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s PS %d Fault Cleared, psInsertedUnfaulted set\n",
                                  __FUNCTION__, 
                                  (processorEnclosure ? "PE" : "DAE0"),
                                  location->slot);
        }

        // save new values
        sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psAcFail =
            psStatus.ACFail;
        sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psFault =
            psStatus.generalFault;
        sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psInserted =
            psStatus.bInserted;

        // do some additional checks for enclosures based on how many PS's in enclosure
        if (sps_mgmt->arraySpsInfo->psEnclCounts[enclIndex] == 2)
        {
            if (psRemovedFaulted)
            {
                logSpsConfigChange = TRUE;
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsCablingStatus = 
                    FBE_SPS_CABLING_UNKNOWN;
            }
            else if (psInsertedUnfaulted)
            {
                needToTestSps = TRUE;
            }
        }
        else if (sps_mgmt->arraySpsInfo->psEnclCounts[enclIndex] > 2)
        {
            for (psIndex = 0; psIndex < sps_mgmt->arraySpsInfo->psEnclCounts[enclIndex]; psIndex++)
            {
                if (((sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].associatedSps == FBE_SPS_A) && (sps_mgmt->base_environment.spid == SP_A)) ||
                        ((sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].associatedSps == FBE_SPS_B) && (sps_mgmt->base_environment.spid == SP_B)))
                {
                    localPsCount++;
                    if (sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psInserted &&
                        !sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psFault)
                    {
                        localGoodPsCount++;
                    }
                    if (!sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psInserted ||
                        sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psFault)
                    {
                        localBadPsCount++;
                    }
                }
            }
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, localGoodPsCount %d, localBadPsCount %d\n",
                                  __FUNCTION__, localGoodPsCount, localBadPsCount);
            // if a PS became Removed or Faulted and there are no PS's related to local SPS, 
            // process config change
            if (psRemovedFaulted && (localGoodPsCount == 0))
            {
                logSpsConfigChange = TRUE;
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsCablingStatus = 
                    FBE_SPS_CABLING_UNKNOWN;
            }
            // if a PS became Inserted or Fault was cleared and there are no more faulted PS's related
            // to local SPS, mark SPS for testing
            if (psInsertedUnfaulted && (localBadPsCount == 0))
            {
                needToTestSps = TRUE;
            }
        }

        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, logSpsConfigChange %d, needToTestSps %d\n",
                              __FUNCTION__, logSpsConfigChange, needToTestSps);

        // set condition if SPS Testing needed
        if (needToTestSps)
        {
            sps_mgmt->arraySpsInfo->needToTest = TRUE;
            sps_mgmt->arraySpsInfo->testingCompleted = FALSE;
            sps_mgmt->arraySpsInfo->spsTestType = FBE_SPS_CABLING_TEST_TYPE;
            // check the number of SPS's to determine which one needs testing
            if (sps_mgmt->encls_with_sps == 1)
            {
                sps_mgmt->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].needToTest = TRUE;
            }
            else
            {
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].needToTest = TRUE;
            }
            status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                            (fbe_base_object_t *)sps_mgmt,
                                            FBE_SPS_MGMT_TEST_NEEDED_COND);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't set TestNeeded condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s set TestNeeded condition, needToTest encl %d, sps %d\n",
                                      __FUNCTION__, enclIndex, spsIndex);
            }
        }

        /*
         * Generate device strings
         */
        sps_location = *location;
        sps_location.slot = sps_mgmt->base_environment.spid;
        sps_location.sp = sps_mgmt->base_environment.spid;
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                                   location, 
                                                   &spsDeviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s, Failed to create device string.\n", 
                                  __FUNCTION__); 
            return;
        }

        // log event for SPS Configuration change if there are SPS's present
        if ((logSpsConfigChange) && (sps_mgmt->total_sps_count > 0))
        {
            fbe_event_log_write(ESP_INFO_SPS_CONFIG_CHANGE,
                                NULL, 0,
                                "%s", 
                                &spsDeviceStr[0]);
            // send change to peer SP
            status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error sending SPS Info to peer, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
    }
    else
    {
        // still save new values for PS's not associated with this sides SPS
        sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psAcFail =
            psStatus.ACFail;
        sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psFault =
            psStatus.generalFault;
        sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].psInserted =
            psStatus.bInserted;
    }

    sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].associatedSp =
        psStatus.associatedSp;
    sps_mgmt->arraySpsInfo->ps_info[enclIndex][location->slot].associatedSps =
        psStatus.associatedSps;

    /*
     * Send the notification 
     */
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)sps_mgmt,
                                                       FBE_CLASS_ID_SPS_MGMT,
                                                       FBE_DEVICE_TYPE_SPS,
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       &sps_location);

}
/******************************************************
 * end fbe_sps_mgmt_processPsStatusChange() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_processBobStatus()
 ****************************************************************
 * @brief
 *  This function processes a BoB State Change by:
 *      -request Battery Status from Phy Pkg
 *      -check for changes events
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  24-Apr-2012:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_processBobStatus(fbe_sps_mgmt_t *sps_mgmt, 
                                           fbe_device_physical_location_t *pLocation)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_status_t                            status;
    fbe_base_board_mgmt_get_battery_status_t    bobStatus;
    fbe_base_battery_info_t                 previousBobInfo;
    fbe_base_battery_info_t                 *currentBobInfo;
    fbe_bool_t                              bobStateChange = FALSE;
    fbe_bool_t                              scheduleDebounceTimer = FALSE;
    fbe_base_board_mgmt_battery_command_t   batteryCommand;
    fbe_bool_t                              currentBobFaultStatus, previousBobFaultStatus;
    fbe_u8_t                                bobDeviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_bool_t                              setEnergyRequirementNeeded=FALSE;
    fbe_bool_t                              bbuToTestFound;
    fbe_u32_t                               bbuIndex;
    fbe_u32_t                               bbuSlotIndex;

    // ESP stores BBU data index 0 to BbuMaxCount-1.  pLocation has slot index per SP.
    if (pLocation->sp == SP_A)
    {
        bbuSlotIndex = pLocation->slot;
    }
    else
    {
        bbuSlotIndex = pLocation->slot + (sps_mgmt->total_bob_count/2);
    }

    // save old SPS status
    previousBobInfo = sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex];
    currentBobInfo = &(sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex]);
    previousBobFaultStatus = fbe_sps_mgmt_anyBbuFaults(sps_mgmt);
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               pLocation, 
                                               &bobDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    
        return status;
    }

    /*
     * Checkif we are simulating BoB's
     */
    if (sps_mgmt->arrayBobInfo->simulateBobInfo)
    {
        currentBobInfo->batteryInserted = TRUE;
        currentBobInfo->batteryEnabled = TRUE;
        currentBobInfo->batteryReady = TRUE;
        currentBobInfo->batteryState = FBE_BATTERY_STATUS_BATTERY_READY;
        currentBobInfo->batteryChargeState = BATTERY_FULLY_CHARGED;
        currentBobInfo->batteryFaults = FBE_BATTERY_FAULT_NO_FAULT;
        currentBobInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_COMPLETED;
        currentBobInfo->hasResumeProm = FBE_TRUE;
        currentBobInfo->isFaultRegFail = FBE_FALSE;
        currentBobInfo->associatedSp = pLocation->sp;
        currentBobInfo->slotNumOnSpBlade = pLocation->slot;
    }
    else
    {
        // Get BoB status
        fbe_set_memory(&bobStatus, 0, sizeof(fbe_base_board_mgmt_get_battery_status_t));
        bobStatus.device_location = *pLocation;
        status = fbe_api_board_get_battery_status(sps_mgmt->arrayBobInfo->bobObjectId, &bobStatus);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting initial BBU Status, status: 0x%X\n",
                                  __FUNCTION__, status);
            currentBobInfo->batteryInserted = FALSE;
            sps_mgmt->total_bob_count = 0;
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s bbuInserted %d, bbuState %d\n",
                                  __FUNCTION__, 
                                  &bobDeviceStr[0], 
                                  bobStatus.batteryInfo.batteryInserted,
                                  bobStatus.batteryInfo.batteryState);

            // Debounce some BoB status (BatteryOnline)
            if (bobStatus.batteryInfo.batteryState == FBE_BATTERY_STATUS_ON_BATTERY)
            {
                if (sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bbuSlotIndex] == 0)
                {
                    // start debouncing
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, %s start OnBattery debounce, prev %s\n",
                                          __FUNCTION__, 
                                          &bobDeviceStr[0], 
                                          fbe_sps_mgmt_getBobStateString(previousBobInfo.batteryState));
                    sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bbuSlotIndex] = 1;
                    bobStatus.batteryInfo.batteryState = previousBobInfo.batteryState;
                    scheduleDebounceTimer = TRUE;
                }
                else
                {
                    // already debouncing
                    if (sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bbuSlotIndex] > FBE_SPS_STATUS_DEBOUNCE_COUNT)
                    {
                        // debouncing complete
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, %s OnBattery debounce complete\n",
                                              __FUNCTION__, 
                                              &bobDeviceStr[0]); 
                    }
                    else
                    {
                        // continue debouncing
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, %s continue OnBattery debounce, cnt %d, prev %s\n",
                                              __FUNCTION__, 
                                              &bobDeviceStr[0], 
                                              sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bbuSlotIndex],
                                              fbe_sps_mgmt_getBobStateString(previousBobInfo.batteryState));
                        bobStatus.batteryInfo.batteryState = previousBobInfo.batteryState;
                    }
                }
            }
            else if (sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bbuSlotIndex] > 0)
            {
                // stop debouncing (no longer OnBattery)
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, %s stop OnBattery debounce\n",
                                      __FUNCTION__, 
                                      &bobDeviceStr[0]); 
                sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bbuSlotIndex] = 0;
            }

            // update BoB status
            currentBobInfo->envInterfaceStatus = bobStatus.batteryInfo.envInterfaceStatus;
            currentBobInfo->batteryInserted = bobStatus.batteryInfo.batteryInserted;
            currentBobInfo->batteryEnabled = bobStatus.batteryInfo.batteryEnabled;
            currentBobInfo->batteryState = bobStatus.batteryInfo.batteryState;
            currentBobInfo->batteryReady = bobStatus.batteryInfo.batteryReady;
            currentBobInfo->batteryChargeState = bobStatus.batteryInfo.batteryChargeState;
            currentBobInfo->batteryFfid = bobStatus.batteryInfo.batteryFfid;
            currentBobInfo->associatedSp = bobStatus.batteryInfo.associatedSp;
            currentBobInfo->slotNumOnSpBlade = bobStatus.batteryInfo.slotNumOnSpBlade;
            currentBobInfo->batteryFaults = bobStatus.batteryInfo.batteryFaults;
            currentBobInfo->batteryDeliverableCapacity = bobStatus.batteryInfo.batteryDeliverableCapacity;
            currentBobInfo->batteryStorageCapacity = bobStatus.batteryInfo.batteryStorageCapacity;
            currentBobInfo->batteryTestStatus = bobStatus.batteryInfo.batteryTestStatus;
            currentBobInfo->hasResumeProm = bobStatus.batteryInfo.hasResumeProm;
            currentBobInfo->firmwareRevMajor = bobStatus.batteryInfo.firmwareRevMajor;
            currentBobInfo->firmwareRevMinor = bobStatus.batteryInfo.firmwareRevMinor;
            currentBobInfo->isFaultRegFail = bobStatus.batteryInfo.isFaultRegFail;
            currentBobInfo->energyRequirement = bobStatus.batteryInfo.energyRequirement;
        }
    }


    if (currentBobInfo->envInterfaceStatus != previousBobInfo.envInterfaceStatus)
    {
        bobStateChange = TRUE;
        if ((currentBobInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL) &&
            (currentBobInfo->transactionStatus != EMCPAL_STATUS_SUCCESS))
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &bobDeviceStr[0],
                                SmbTranslateErrorString(currentBobInfo->transactionStatus));
        }
        else if (currentBobInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &bobDeviceStr[0],
                                fbe_base_env_decode_env_status(currentBobInfo->envInterfaceStatus));
        }
        else
        {
            fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                NULL, 0,
                                "%s", 
                                &bobDeviceStr[0]);
        }
    }

    if ((previousBobInfo.batteryInserted != currentBobInfo->batteryInserted) ||
        (previousBobInfo.batteryState != currentBobInfo->batteryState) ||
        (previousBobInfo.batteryReady != currentBobInfo->batteryReady) ||
        (previousBobInfo.batteryFaults != currentBobInfo->batteryFaults) ||
        (previousBobInfo.batteryTestStatus != currentBobInfo->batteryTestStatus) ||
        (previousBobInfo.isFaultRegFail != currentBobInfo->isFaultRegFail))
    {
        bobStateChange = TRUE;
        if (previousBobInfo.batteryInserted != currentBobInfo->batteryInserted)
        {
            if (currentBobInfo->batteryInserted)
            {
                status = fbe_sps_mgmt_processInsertedBbu(sps_mgmt, pLocation);
            }
            else
            {
                // BBU removed, so clear fault & test status
                currentBobInfo->batteryFaults = FBE_BATTERY_FAULT_NO_FAULT;
                currentBobInfo->batteryTestStatus = FBE_BATTERY_TEST_STATUS_NONE;
// AR 577579 - BMC bug causing spurious BBU Removals.
//    Workaround - only log Info event for removal
//    Remove workaround (enable Error event) once AR 577211 is resolved
                fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                    NULL, 0,
                                    "%s %s %s", 
                                    &bobDeviceStr[0],
                                    "Inserted",
                                    "Removed");
/*
                fbe_event_log_write(ESP_ERROR_SPS_REMOVED,
                                    NULL, 0,
                                    "%s", 
                                    &bobDeviceStr[0]);
                                    sps_mgmt->arrayBobInfo->bobNameString[pLocation->slot]);
*/
            }
        }
        if (previousBobInfo.batteryState != currentBobInfo->batteryState)
        {
            if ((sps_mgmt->base_environment.spid == pLocation->sp) &&
                (sps_mgmt->testingState == FBE_SPS_LOCAL_SPS_TESTING) &&
                ((previousBobInfo.batteryState == FBE_BATTERY_STATUS_CHARGING) ||
                    (previousBobInfo.batteryState == FBE_BATTERY_STATUS_TESTING)) &&
                (currentBobInfo->batteryState == FBE_BATTERY_STATUS_BATTERY_READY))
            {
                sps_mgmt->arrayBobInfo->bobNeedsTesting[bbuSlotIndex] = FALSE;
                // need to clear BBU needToTest if no local BBU's need testing
                bbuToTestFound = FALSE;
                for (bbuIndex = 0; bbuIndex < sps_mgmt->total_bob_count; bbuIndex++)
                {
                    if (sps_mgmt->arrayBobInfo->bobNeedsTesting[bbuIndex])
                    {
                        // found another BBU to test
                        bbuToTestFound = TRUE;
                        break;
                    }
                }

                if (bbuToTestFound)
                {
                    // start testing next BBU (already have permission)
                    fbe_sps_mgmt_initiateTest(sps_mgmt);
                }
                else
                {
                    // all BBU's tested, complete testing
                    sps_mgmt->arrayBobInfo->needToTest = FALSE;
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s SpsTestState change, %s to %s\n",
                                          __FUNCTION__, 
                                          fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                                          fbe_sps_mgmt_getSpsTestStateString(FBE_SPS_NO_TESTING_IN_PROGRESS));
                    sps_mgmt->testingState = FBE_SPS_NO_TESTING_IN_PROGRESS;
                    status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_TEST_COMPLETED);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s Error sending SPS Test Completed to peer, status: 0x%X\n",
                                              __FUNCTION__, status);
                    }
                }
            }
/* Disable logging of batteryState - has caused some confusion & is an internally generated state
            if (currentBobInfo->batteryState == FBE_BATTERY_STATUS_UNKNOWN)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, ignore SpsTestState of Unknown\n",
                                      __FUNCTION__);
                currentBobInfo->batteryState = previousBobInfo.batteryState;
            }
            else
            {
                fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                    NULL, 0,
                                    "%s %s %s", 
                                    &bobDeviceStr[0],
                                    fbe_sps_mgmt_getBobStateString(previousBobInfo.batteryState),
                                    fbe_sps_mgmt_getBobStateString(currentBobInfo->batteryState));
            }
*/
        }
        
        if (previousBobInfo.batteryReady != currentBobInfo->batteryReady)
        {
            if (!currentBobInfo->batteryReady)
            {
                fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                    NULL, 0,
                                    "%s %s %s",
                                    &bobDeviceStr[0],
                                    "BatteryReady", "NotBatteryReady");
                // save time when BBU's BatteryReady got cleared
                sps_mgmt->arrayBobInfo->notBatteryReadyTimeStart[bbuSlotIndex] = fbe_get_time();
            }
            else
            {
                fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                    NULL, 0,
                                    "%s %s %s",
                                    &bobDeviceStr[0],
                                    "NotBatteryReady", "BatteryReady");
                if (currentBobInfo->batteryFaults == FBE_BATTERY_FAULT_CANNOT_CHARGE)
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, %s clear CannotCharge fault\n",
                                          __FUNCTION__,
                                          &bobDeviceStr[0]);
                    currentBobInfo->batteryFaults = FBE_BATTERY_FAULT_NO_FAULT;
                }
            }

            // check if Ride Through Timeout changed (may need to reset)
            fbe_sps_mgmt_verifyRideThroughTimeout(sps_mgmt);
        }

        if (previousBobInfo.batteryFaults != currentBobInfo->batteryFaults)
        {
            if (currentBobInfo->batteryFaults != FBE_BATTERY_FAULT_NO_FAULT)
            {
                // We are not recognizing these faults for now (BBU is still OK to use)
                if ((currentBobInfo->batteryFaults == FBE_BATTERY_FAULT_COMM_FAILURE) ||
                    (currentBobInfo->batteryFaults == FBE_BATTERY_FAULT_INTERNAL_FAILURE) ||
                    (currentBobInfo->batteryFaults == FBE_BATTERY_FAULT_LOW_CHARGE))
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, BBU has detected %s, ignore it\n",
                                          __FUNCTION__,
                                          fbe_sps_mgmt_getBobFaultString(currentBobInfo->batteryFaults));
                    currentBobInfo->batteryFaults = previousBobInfo.batteryFaults;
                }
                else
                {
                    fbe_event_log_write(ESP_ERROR_SPS_FAULTED,
                                        NULL, 0,
                                        "%s %s %s",
                                        &bobDeviceStr[0],
                                        fbe_sps_mgmt_getBobFaultString(previousBobInfo.batteryFaults),
                                        fbe_sps_mgmt_getBobFaultString(currentBobInfo->batteryFaults));
                }
            }
            else
            {
                fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                    NULL, 0,
                                    "%s %s %s", 
                                    &bobDeviceStr[0],
                                    fbe_sps_mgmt_getBobFaultString(previousBobInfo.batteryFaults),
                                    fbe_sps_mgmt_getBobFaultString(currentBobInfo->batteryFaults));
            }
        }
                
        if (previousBobInfo.batteryTestStatus != currentBobInfo->batteryTestStatus)
        {
            fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                NULL, 0,
                                "%s %s %s", 
                                &bobDeviceStr[0],
                                fbe_sps_mgmt_getBobTestStatusString(previousBobInfo.batteryTestStatus),
                                fbe_sps_mgmt_getBobTestStatusString(currentBobInfo->batteryTestStatus));
/* JAP - disable, not needed with SPECL changes for BBU Test results
            // send new Test status to peer (only local can read it because it gets cleared on read)
            status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_PEER_BBU_TEST_STATUS);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error sending BBU Test Status to peer, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
*/
        }
                
        if (previousBobInfo.isFaultRegFail != currentBobInfo->isFaultRegFail)
        {
            if (currentBobInfo->isFaultRegFail)
            {
                fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                    NULL, 0,
                                    "%s %s %s", 
                                    &bobDeviceStr[0],
                                    "OK",
                                    "FaultReg Fault");
                if (previousBobInfo.batteryFaults == FBE_BATTERY_FAULT_NO_FAULT)
                {
                    currentBobInfo->batteryFaults = FBE_BATTERY_FAULT_FLT_STATUS_REG;
                }
            }
            else
            {
                fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                    NULL, 0,
                                    "%s %s %s", 
                                    &bobDeviceStr[0], 
                                    "FaultReg Fault",
                                    "OK");
                if (previousBobInfo.batteryFaults == FBE_BATTERY_FAULT_FLT_STATUS_REG)
                {
                    currentBobInfo->batteryFaults = FBE_BATTERY_FAULT_NO_FAULT;
                }
            }
        }
    }

    fbe_sps_mgmt_calculateBatteryStateInfo(sps_mgmt,pLocation, bbuSlotIndex);

    // check if BBU FFID became avaliable (need to verify if it supports Self Test)
    if ((previousBobInfo.batteryFfid == NO_MODULE) &&
        (currentBobInfo->batteryFfid != NO_MODULE))
    {
        if ((currentBobInfo->associatedSp == sps_mgmt->base_environment.spid) &&
            HwAttrLib_isBbuSelfTestSupported(currentBobInfo->batteryFfid))
        {
            sps_mgmt->arrayBobInfo->bobNeedsTesting[bbuSlotIndex] = TRUE; 
            sps_mgmt->arrayBobInfo->needToTest = TRUE;
            // set condition to signify that we need to test the SPS
            status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                            base_object,  
                                            FBE_SPS_MGMT_TEST_NEEDED_COND);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't set TestNeeded condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, BBU %d FFID avaliable, TestNeeded condition scheduled\n",
                                      __FUNCTION__, bbuSlotIndex);
            }
        }
    }

    // enable BOB.
    // enable local BOB if not enabled
    if ((currentBobInfo->associatedSp == sps_mgmt->base_environment.spid) &&
        (!currentBobInfo->batteryEnabled) && currentBobInfo->batteryInserted)
    {
        batteryCommand.batteryAction = FBE_BATTERY_ACTION_ENABLE;
        batteryCommand.device_location = *pLocation;
        status = fbe_api_board_send_battery_command(sps_mgmt->arrayBobInfo->bobObjectId, &batteryCommand);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error enabling %s BBU %d, status: 0x%X\n",
                                  __FUNCTION__, 
                                  currentBobInfo->associatedSp?"SPB":"SPA", 
                                  currentBobInfo->slotNumOnSpBlade, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Successfully sent enable command to %s BBU %d\n",
                                  __FUNCTION__,
                                  currentBobInfo->associatedSp?"SPB":"SPA", 
                                  currentBobInfo->slotNumOnSpBlade);
        }
    }

    // set local battery energy requirements if it is not set yet.
    if ((currentBobInfo->associatedSp == sps_mgmt->base_environment.spid) && 
        (currentBobInfo->batteryInserted) &&
        (currentBobInfo->setEnergyRequirementSupported == TRUE))
    {
        //check if energy requirement is already set
        //if not, we will set it
        if (currentBobInfo->defaultEnergyRequirement.format == BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER)
        {
            if(currentBobInfo->energyRequirement.energy != currentBobInfo->defaultEnergyRequirement.data.energyAndPower.energy ||
               currentBobInfo->energyRequirement.maxLoad != currentBobInfo->defaultEnergyRequirement.data.energyAndPower.power)
            {
                setEnergyRequirementNeeded = TRUE;
                batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement = currentBobInfo->defaultEnergyRequirement;
            }
        }
        else if (currentBobInfo->defaultEnergyRequirement.format == BATTERY_ENERGY_REQUIREMENTS_FORMAT_POWER_AND_TIME)
        {
            if(currentBobInfo->energyRequirement.power != currentBobInfo->defaultEnergyRequirement.data.powerAndTime.power ||
               currentBobInfo->energyRequirement.time != currentBobInfo->defaultEnergyRequirement.data.powerAndTime.time)
            {
                setEnergyRequirementNeeded = TRUE;
                batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement = currentBobInfo->defaultEnergyRequirement;
            }
        }  

        if (setEnergyRequirementNeeded == TRUE)
        {
            batteryCommand.batteryAction = FBE_BATTERY_ACTION_POWER_REQ_INIT;
            batteryCommand.device_location = *pLocation;
            status = fbe_api_board_send_battery_command(sps_mgmt->arrayBobInfo->bobObjectId, &batteryCommand);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error setting power reqs %s, status: 0x%X\n",
                                      __FUNCTION__, 
                                      &bobDeviceStr[0], 
                                      status);
            }
            else
            {
                if (batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.format == BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER)
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, Successfully sent power reqs, %s, energy %d, maxLoad %d.\n",
                                          __FUNCTION__,
                                          &bobDeviceStr[0], 
                                          batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.energyAndPower.energy,
                                          batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.energyAndPower.power);
                }
                else if (batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.format == BATTERY_ENERGY_REQUIREMENTS_FORMAT_POWER_AND_TIME)
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, Successfully sent power reqs, %s, power %d, time %d.\n",
                                          __FUNCTION__,
                                          &bobDeviceStr[0], 
                                          batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.powerAndTime.power,
                                          batteryCommand.batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.powerAndTime.time);
                }
            }
        }
    }

    // check if SPS Cache status changed
    fbe_sps_mgmt_checkSpsCacheStatus(sps_mgmt);

    pLocation->sp = currentBobInfo->associatedSp;
    status = fbe_sps_mgmt_resume_prom_handle_bob_status_change(sps_mgmt,
                                                               pLocation,
                                                               currentBobInfo,
                                                               &previousBobInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: BBU(%d_%d_%d) handle_state_change failed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);
    }

    // update Enclosure Fault LED if possible change detected
    currentBobFaultStatus = fbe_sps_mgmt_anyBbuFaults(sps_mgmt);
    if (previousBobFaultStatus != currentBobFaultStatus)
    {
        status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                    pLocation, 
                                                    FBE_ENCL_FAULT_LED_BATTERY_FLT);
    }

    /*
     * Notify peer SP of SPS change and no message already in progress
     * and send notification for sps_mgmt data change
     */
    if (bobStateChange)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, sending BBU event notification, BBU %d\n",
                              __FUNCTION__, pLocation->slot);
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)sps_mgmt,
                                                           FBE_CLASS_ID_SPS_MGMT, 
                                                           FBE_DEVICE_TYPE_BATTERY,
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           pLocation);
    }

    /*
     * Start Debounce Timer if needed. We start timer base on local BOB status
     */
    if ((scheduleDebounceTimer) && (pLocation->sp == sps_mgmt->base_environment.spid))
    {
        // clear condition to start the timer
        status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                base_object, 
                                                FBE_SPS_MGMT_DEBOUNCE_TIMER_COND);
    
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't clear debounce timer condition, status: 0x%X\n",
                                      __FUNCTION__, status);
                
                return status;
            }
    }

    return FBE_STATUS_OK;

}
/******************************************************
 * end fbe_sps_mgmt_processBobStatus() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_processNotPresentStatus()
 ****************************************************************
 * @brief
 *  This function process the SPS Status when it transitions
 *  to Not Present.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processNotPresentStatus(fbe_sps_mgmt_t *sps_mgmt,
                                     fbe_sps_ps_encl_num_t spsLocation,
                                     sps_mgmt_sps_module_battery_event_t spsEvent)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_sps_info_t                          *currentSpsInfo;
    fbe_status_t                            status = FBE_STATUS_OK, status1;
    fbe_device_physical_location_t          location;
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                spsBatteryDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};

    /*
     * Generate device strings
     */
    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
    if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
        (spsLocation == FBE_SPS_MGMT_PE))
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    location.slot = sps_mgmt->base_environment.spid; 
    location.sp = sps_mgmt->base_environment.spid;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               &location, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    status1 = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS_BATTERY, 
                                               &location, 
                                               &spsBatteryDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if ((status != FBE_STATUS_OK) || (status1 != FBE_STATUS_OK))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
        return status;
    }

    currentSpsInfo = &(sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS]);

    // status changed to Not Present, so log SPS Removed
    if (spsEvent == SPS_MGMT_SPS_EVENT_MODULE_REMOVE)
    {
        currentSpsInfo->spsModulePresent = FALSE;
        fbe_zero_memory(&currentSpsInfo->spsFaultInfo, sizeof(fbe_sps_fault_info_t));
        fbe_zero_memory(&currentSpsInfo->spsManufInfo.spsModuleManufInfo, sizeof(fbe_sps_manuf_record_t));
        fbe_zero_memory(&currentSpsInfo->primaryFirmwareRev, (FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1));
        fbe_zero_memory(&currentSpsInfo->secondaryFirmwareRev, (FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1));
        fbe_event_log_write(ESP_ERROR_SPS_REMOVED,
                            NULL, 0,
                            "%s",
                            &spsDeviceStr[0]);
    }

    if ((spsEvent == SPS_MGMT_SPS_EVENT_MODULE_REMOVE) || (spsEvent == SPS_MGMT_SPS_EVENT_BATTERY_REMOVE))
    {
        currentSpsInfo->spsBatteryPresent = FALSE;
        currentSpsInfo->spsFaultInfo.spsBatteryFault = FALSE;
        currentSpsInfo->spsFaultInfo.spsBatteryEOL = FALSE;
        currentSpsInfo->spsFaultInfo.spsBatteryNotEngagedFault = FALSE;
        currentSpsInfo->testInProgress = FALSE;
        fbe_zero_memory(&currentSpsInfo->spsManufInfo.spsBatteryManufInfo, sizeof(fbe_sps_manuf_record_t));
        fbe_zero_memory(&currentSpsInfo->batteryFirmwareRev, (FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1));
        fbe_event_log_write(ESP_ERROR_SPS_REMOVED,
                            NULL, 0,
                            "%s",
                            &spsBatteryDeviceStr[0]);
    }

    // if SPS Battery removed, mark spsStatus as faulted
    if (spsEvent == SPS_MGMT_SPS_EVENT_BATTERY_REMOVE)
    {
        sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsStatus = SPS_STATE_FAULTED;
    }

    sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsCablingStatus = FBE_SPS_CABLING_UNKNOWN;
    currentSpsInfo->spsModel = SPS_TYPE_UNKNOWN;

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processNotPresentStatus() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_sps_mgmt_processBatteryOkStatus()
 ****************************************************************
 * @brief
 *  This function process the SPS Status when it transitions
 *  to Battery Ok.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processBatteryOkStatus(fbe_sps_mgmt_t *sps_mgmt,
                                    fbe_sps_ps_encl_num_t spsLocation,
                                    fbe_sps_info_t *previousSpsInfo)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_sps_info_t                          *currentSpsInfo;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sps_cabling_status_t                newSpsCablingStatus, previousSpsCablingStatus;
    fbe_sps_ps_encl_num_t                   spsOtherEnclLocation;
    fbe_device_physical_location_t          location;
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_bool_t                              moreTestingNeeded;
    fbe_u32_t                               enclIndex;

    /*
     * Generate device strings
     */
    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
    if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
        (spsLocation == FBE_SPS_MGMT_PE))
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    location.slot = sps_mgmt->base_environment.spid; 
    location.sp = sps_mgmt->base_environment.spid;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               &location, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
        return status;
    }

    if (spsLocation == FBE_SPS_MGMT_PE)
    {
        spsOtherEnclLocation = FBE_SPS_MGMT_DAE0;
    }
    else
    {
        spsOtherEnclLocation = FBE_SPS_MGMT_PE;
    }

    currentSpsInfo = &(sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS]);

    /*
     * If SPS Testing just completed, notify the peer
     */
    if (currentSpsInfo->testInProgress &&
        ((previousSpsInfo->spsStatus == SPS_STATE_CHARGING) ||
            (previousSpsInfo->spsStatus == SPS_STATE_TESTING)) &&
        ((sps_mgmt->encls_with_sps == 1) ||
        ((sps_mgmt->encls_with_sps == 2) && 
            (sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsStatus == SPS_STATE_AVAILABLE))) &&
        (sps_mgmt->testingState == FBE_SPS_LOCAL_SPS_TESTING))
    {
        currentSpsInfo->testInProgress = FALSE;
        currentSpsInfo->needToTest = FALSE;
        // update Cabling status & log any changes
        if (sps_mgmt->arraySpsInfo->spsTestType == FBE_SPS_CABLING_TEST_TYPE)
        {
            previousSpsCablingStatus = 
                sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsCablingStatus;
            newSpsCablingStatus = fbe_sps_mgmt_checkArrayCablingStatus(sps_mgmt, TRUE);
            if (previousSpsCablingStatus != newSpsCablingStatus)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, %s Cabling Status change, %s to %s\n",
                                      __FUNCTION__, 
                                     currentSpsInfo->spsNameString,
                                     fbe_sps_mgmt_getSpsCablingStatusString(previousSpsCablingStatus),
                                     fbe_sps_mgmt_getSpsCablingStatusString(newSpsCablingStatus));

                if (newSpsCablingStatus == FBE_SPS_CABLING_VALID)
                {
                    fbe_event_log_write(ESP_INFO_SPS_CABLING_VALID,
                                        NULL, 0,
                                        "%s",
                                        currentSpsInfo->spsNameString);
                }
                else
                {
                    fbe_event_log_write(ESP_ERROR_SPS_CABLING_FAULT,
                                        NULL, 0,
                                        "%s %s",
                                        currentSpsInfo->spsNameString,
                                        fbe_sps_mgmt_getSpsCablingStatusString(newSpsCablingStatus));
                }
                sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsCablingStatus
                    = newSpsCablingStatus;
            }
        }

        // check if any other testing is needed
        moreTestingNeeded = FALSE;
        sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].needToTest = FALSE;
        for (enclIndex = 0; enclIndex < sps_mgmt->encls_with_sps; enclIndex++)
        {
            if (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsModulePresent &&
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsBatteryPresent &&
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].needToTest)
            {
                moreTestingNeeded = TRUE;
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, need to test other SPS %d\n",
                                      __FUNCTION__, enclIndex);
            }
        }
        if (moreTestingNeeded)
        {
            fbe_sps_mgmt_initiateSpsTest(sps_mgmt);
        }
        else
        {
            // mark testing as over
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s SpsTestState change, %s to %s\n",
                                  __FUNCTION__, 
                                  fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                                  fbe_sps_mgmt_getSpsTestStateString(FBE_SPS_NO_TESTING_IN_PROGRESS));
            sps_mgmt->testingState = FBE_SPS_NO_TESTING_IN_PROGRESS;
            // set other flags & send TestComplete to peer
            sps_mgmt->arraySpsInfo->needToTest = FALSE;
            sps_mgmt->arraySpsInfo->testingCompleted = TRUE;

            status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_TEST_COMPLETED);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error sending SPS Test Completed to peer, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processBatteryOkStatus() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_sps_mgmt_processRechargingStatus()
 ****************************************************************
 * @brief
 *  This function process the SPS Status when it transitions
 *  to Recharging.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processRechargingStatus(fbe_sps_mgmt_t *sps_mgmt,
                                     fbe_sps_ps_encl_num_t spsLocation,
                                     fbe_sps_info_t *previousSpsInfo)
{
    fbe_sps_info_t                          *currentSpsInfo;
    fbe_status_t                            status = FBE_STATUS_OK;

    currentSpsInfo = &(sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS]);

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processRechargingStatus() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_sps_mgmt_processTestingStatus()
 ****************************************************************
 * @brief
 *  This function process the SPS Status when it transitions
 *  to Testing.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processTestingStatus(fbe_sps_mgmt_t *sps_mgmt,
                                     fbe_sps_info_t *previousSpsInfo)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               enclIndex, psIndex;

    /*
     * SPS is testing, but PS events for AC_FAIL may have already occurred,
     * so re-check PS status.
     */
    for (enclIndex = 0; enclIndex < sps_mgmt->encls_with_sps; enclIndex++)
    {
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            if (sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psAcFail && 
                !sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psFault)
            {
                sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailDetected = TRUE;
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s (SPS_TEST), encl %d, PS %d, AC_FAIL detected\n",
                                      __FUNCTION__, 
                                      enclIndex,
                                      psIndex);
            }
        }
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processTestingStatus() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_sps_mgmt_processBatteryOnlineStatus()
 ****************************************************************
 * @brief
 *  This function process the SPS Status when it transitions
 *  to Battery Online.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processBatteryOnlineStatus(fbe_sps_mgmt_t *sps_mgmt,
                                        fbe_sps_ps_encl_num_t spsLocation,
                                        fbe_sps_info_t *previousSpsInfo)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_sps_info_t                          *currentSpsInfo;
    fbe_status_t                            status = FBE_STATUS_OK;

    currentSpsInfo = &(sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS]);
    sps_mgmt->arraySpsInfo->spsBatteryOnlineCount = 0;

    /*
     * Determine whether BatteryOnline debounce starting or completed
     */
    if (sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsLocation] == 0)
    {
        sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsLocation]++;
        sps_mgmt->arraySpsInfo->debouncedSpsStatus[spsLocation] = SPS_STATE_ON_BATTERY;
        // restore the previous SPS Status
        currentSpsInfo->spsStatus = previousSpsInfo->spsStatus;
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, spsLocation %d, previous SpsStatus %s, start debounce\n",
                              __FUNCTION__, 
                              spsLocation,
                              fbe_sps_mgmt_getSpsStatusString(previousSpsInfo->spsStatus));
        // clear condition to start the debounce timer
        status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                base_object, 
                                                FBE_SPS_MGMT_DEBOUNCE_TIMER_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear debounce timer condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        if (sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsLocation] > FBE_SPS_STATUS_DEBOUNCE_COUNT)
        {
            sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsLocation] = 0;
            sps_mgmt->arraySpsInfo->debouncedSpsStatus[spsLocation] = SPS_STATE_UNKNOWN;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, spsLocation %d, debounce count exceeded, SpsStatus %s, end debounce\n",
                                  __FUNCTION__, 
                                  spsLocation,
                                  fbe_sps_mgmt_getSpsStatusString(sps_mgmt->arraySpsInfo->debouncedSpsStatus[spsLocation]));
        }
        else
        {
            // restore the previous SPS Status
            currentSpsInfo->spsStatus = previousSpsInfo->spsStatus;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, spsLocation %d, debounce count %d, spsStatus %s\n",
                                  __FUNCTION__, 
                                  spsLocation,
                                  sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsLocation],
                                  fbe_sps_mgmt_getSpsStatusString(currentSpsInfo->spsStatus));
        }
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processBatteryOnlineStatus() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_sps_mgmt_processFaultedStatus()
 ****************************************************************
 * @brief
 *  This function process the SPS Status when it transitions
 *  to Faulted.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processFaultedStatus(fbe_sps_mgmt_t *sps_mgmt,
                                  fbe_sps_ps_encl_num_t spsLocation,
                                  fbe_sps_info_t *previousSpsInfo)
{
    fbe_base_object_t         *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_sps_info_t            *currentSpsInfo;
    fbe_status_t              status = FBE_STATUS_OK, status1;
    fbe_device_physical_location_t          location;
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                spsBatteryDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};

    /*
     * Generate device strings
     */
    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
    if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
        (spsLocation == FBE_SPS_MGMT_PE))
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    location.slot = sps_mgmt->base_environment.spid; 
    location.sp = sps_mgmt->base_environment.spid;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               &location, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    status1 = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS_BATTERY, 
                                               &location, 
                                               &spsBatteryDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if ((status != FBE_STATUS_OK) || (status1 != FBE_STATUS_OK))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
        return status;
    }

    currentSpsInfo = &(sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS]);

    /*
     * Determine whether SPS Fault debounce starting or completed
     */
    if (sps_mgmt->arraySpsInfo->debouncedSpsFaultsCount[spsLocation] == 0)
    {
        sps_mgmt->arraySpsInfo->debouncedSpsFaultsCount[spsLocation]++;
        sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsLocation] = currentSpsInfo->spsFaultInfo;
        // restore the previous SPS Status
        currentSpsInfo->spsStatus = previousSpsInfo->spsStatus;
        currentSpsInfo->spsFaultInfo = previousSpsInfo->spsFaultInfo;
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, spsLocation %d, new SpsFaults %s, start debounce\n",
                              __FUNCTION__, 
                              spsLocation,
                              fbe_sps_mgmt_getSpsFaultString(&sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsLocation]));
        // clear condition to start the debounce timer
        status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                base_object, 
                                                FBE_SPS_MGMT_DEBOUNCE_TIMER_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear debounce timer condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
    else if (sps_mgmt->arraySpsInfo->debouncedSpsFaultsCount[spsLocation] > FBE_SPS_STATUS_DEBOUNCE_COUNT)
    {
        sps_mgmt->arraySpsInfo->debouncedSpsFaultsCount[spsLocation] = 0;
        fbe_set_memory(&sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsLocation], 0, sizeof(fbe_sps_fault_info_t));
        sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsLocation].spsBatteryFault = FALSE;
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, spsLocation %d, debounce count exceeded, SpsFaults %s, end debounce\n",
                              __FUNCTION__, 
                              spsLocation,
                              fbe_sps_mgmt_getSpsFaultString(&sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsLocation]));
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processFaultedStatus() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_sps_mgmt_processInsertedSps()
 ****************************************************************
 * @brief
 *  This function process a newly inserted SPS.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processInsertedSps(fbe_sps_mgmt_t *sps_mgmt, 
                                fbe_sps_ps_encl_num_t spsLocation,
                                sps_mgmt_sps_module_battery_event_t spsEvent)
{
    fbe_base_object_t                           *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_status_t                                status = FBE_STATUS_OK, status1;
    fbe_sps_info_t                              *currentSpsInfo;
    fbe_board_mgmt_platform_info_t              boardPlatformInfo;
    fbe_u8_t                                    i;
    fbe_device_physical_location_t              location;
    fbe_u8_t                                    spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                    spsBatteryDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};

    /*
     * Generate device strings
     */
    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
    if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
        (spsLocation == FBE_SPS_MGMT_PE))
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    location.slot = sps_mgmt->base_environment.spid; 
    location.sp = sps_mgmt->base_environment.spid;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               &location, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    status1 = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS_BATTERY, 
                                               &location, 
                                               &spsBatteryDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if ((status != FBE_STATUS_OK) || (status1 != FBE_STATUS_OK))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
        return status;
    }

    currentSpsInfo = &(sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS]);

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s, %s, spsEvent %d\n", 
                          __FUNCTION__,
                          &spsDeviceStr[0],
                          spsEvent); 

    // get Manufacturing Info in either SPS Module or Battery inserted
    sps_mgmt->arraySpsInfo->mfgInfoNeeded[spsLocation] = TRUE;
    status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                            base_object, 
                                            FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, %s, can't clear SPS Mfg Info timer condition, status: 0x%X\n",
                              __FUNCTION__, 
                              sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsNameString, 
                              status);
    }

    // log SPS event based on type
    if ((spsEvent == SPS_MGMT_SPS_EVENT_MODULE_INSERT) || (spsEvent == SPS_MGMT_SPS_EVENT_MODULE_BATTERY_INSERT))
    {
        currentSpsInfo->spsModulePresent = TRUE;
        fbe_event_log_write(ESP_INFO_SPS_INSERTED,
                            NULL, 0,
                            "%s",
                            &spsDeviceStr[0]);
    }
    if ((spsEvent == SPS_MGMT_SPS_EVENT_BATTERY_INSERT) || (spsEvent == SPS_MGMT_SPS_EVENT_MODULE_BATTERY_INSERT))
    {
        currentSpsInfo->spsBatteryPresent = TRUE;
        fbe_event_log_write(ESP_INFO_SPS_INSERTED,
                            NULL, 0,
                            "%s",
                            &spsBatteryDeviceStr[0]);
    }

    // if simulated SPS, no need to test it
    if (((sps_mgmt->total_sps_count > 0) && sps_mgmt->arraySpsInfo->simulateSpsInfo) ||
        ((sps_mgmt->total_bob_count > 0) && sps_mgmt->arrayBobInfo->simulateBobInfo))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, simulated SPS, do not test\n",
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    currentSpsInfo->spsModulePresent = TRUE;
    sps_mgmt->arraySpsInfo->needToTest = TRUE;
    sps_mgmt->arraySpsInfo->testingCompleted = FALSE;
    sps_mgmt->arraySpsInfo->spsTestType = FBE_SPS_CABLING_TEST_TYPE;
    sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsCablingStatus = FBE_SPS_CABLING_UNKNOWN;
    sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].needToTest = TRUE;

    // verify that this is the correct SPS type for this platform
    currentSpsInfo->spsSupportedStatus = FBE_SPS_SUPP_UNKNOWN;
    status = fbe_api_board_get_platform_info(sps_mgmt->arraySpsInfo->spsObjectId[FBE_SPS_MGMT_PE], 
                                             &boardPlatformInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get Board Platform Info, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {
        switch (boardPlatformInfo.hw_platform_info.platformType)
        {
        case SPID_PROMETHEUS_M1_HW_TYPE:
        case SPID_PROMETHEUS_S1_HW_TYPE:            // JAP??
        case SPID_ENTERPRISE_HW_TYPE:
            if (currentSpsInfo->spsModel == SPS_TYPE_LI_ION_2_2_KW)
            {
                currentSpsInfo->spsSupportedStatus = TRUE;
                // Need to set SPS to FastSwitchover mode (xPE only for now)
                if (spsLocation == FBE_SPS_MGMT_PE)
                {
                    status = fbe_sps_mgmt_sendSpsCommand(sps_mgmt, FBE_SPS_ACTION_ENABLE_SPS_FAST_SWITCHOVER, FBE_SPS_MGMT_PE);
                    if (status == FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, PE SPS FastSwitchover set\n",
                                              __FUNCTION__);
                    }
                    else
                    {
                        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, PE SPS FastSwitchover set failed, status %d\n",
                                              __FUNCTION__, 
                                              status);
                    }
                }
            }
            else
            {
                currentSpsInfo->spsSupportedStatus = FALSE;
            }
            break;

        case SPID_MUSTANG_HW_TYPE:
        case SPID_MUSTANG_XM_HW_TYPE:
        case SPID_SPITFIRE_HW_TYPE:
        case SPID_LIGHTNING_HW_TYPE:
        case SPID_HELLCAT_HW_TYPE:
        case SPID_HELLCAT_LITE_HW_TYPE:
            for (i = 0; i < FBE_SPS_PLATFORM_COUNT; i++)
            {
                if (fbe_sps_mgmt_supportedTable[i].platformType == boardPlatformInfo.hw_platform_info.platformType)
                {
                    currentSpsInfo->spsSupportedStatus = 
                        fbe_sps_mgmt_supportedTable[i].spsSupportedStatus[currentSpsInfo->spsModel];
                    break;
                }
            }
            break;
        default:
            currentSpsInfo->spsSupportedStatus = FALSE;
            break;
        }
    }

    // set condition to signify that we need to test the SPS
    status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                    base_object,  
                                    FBE_SPS_MGMT_TEST_NEEDED_COND);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't set TestNeeded condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s set TestNeeded condition\n",
                              __FUNCTION__);
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processInsertedSps() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_processInsertedBbu()
 ****************************************************************
 * @brief
 *  This function process a newly inserted BBU.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_processInsertedBbu(fbe_sps_mgmt_t *sps_mgmt,
                                fbe_device_physical_location_t *location)
{
    fbe_base_object_t                      *base_object = (fbe_base_object_t *)sps_mgmt;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u8_t                                bobDeviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_device_physical_location_t          tempLocation = {0};

    tempLocation = *location;
    if (sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE)
    {
        tempLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        tempLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               &tempLocation, 
                                               &bobDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    
        return status;
    }

    // log SPS event based on type
    fbe_event_log_write(ESP_INFO_SPS_INSERTED,
                        NULL, 0,
                        "%s",
                        &bobDeviceStr[0]);

    // if simulated SPS, no need to test it
    if (sps_mgmt->arrayBobInfo->simulateBobInfo)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, simulated SPS, do not set PowerReqs or Test\n",
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Reset notBatteryReady start time to now. If the new BBU is batteryReady,
     * this timeStamp will be reset when it becomes not batteryReady, otherwise
     * this is the notBatteryReady start time.
     */
    sps_mgmt->arrayBobInfo->notBatteryReadyTimeStart[location->slot] = fbe_get_time();

    // if local BBU, we need to run self test
    if ((sps_mgmt->arrayBobInfo->bob_info[location->slot].associatedSp == sps_mgmt->base_environment.spid) &&
        HwAttrLib_isBbuSelfTestSupported(sps_mgmt->arrayBobInfo->bob_info[location->slot].batteryFfid))
    {
        sps_mgmt->arrayBobInfo->bobNeedsTesting[location->slot] = TRUE; 
        sps_mgmt->arrayBobInfo->needToTest = TRUE;
        sps_mgmt->arrayBobInfo->testingCompleted = FALSE;
        // set condition to signify that we need to test the SPS
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                        base_object,  
                                        FBE_SPS_MGMT_TEST_NEEDED_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set TestNeeded condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s set TestNeeded condition\n",
                                  __FUNCTION__);
        }
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processInsertedBbu() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_init_test_time_cond_function()
 ****************************************************************
 * @brief
 *  This function checks the Weekly SPS Test time has arrived.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  07-Apr-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_init_test_time_cond_function(fbe_base_object_t * base_object, 
                                           fbe_packet_t * packet)
{
    fbe_status_t                                status;
 
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);


    /* clear condition to start the timer */
    status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                            base_object, 
                                            FBE_SPS_MGMT_CHECK_TEST_TIME_COND);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear timer condition, status: 0x%X\n",
                              __FUNCTION__, status);
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);

    }

    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_init_test_time_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_process_sps_state_change_cond_function()
 ****************************************************************
 * @brief
 *  This function processes SPS Status changes (this condition
 *  is only set from Usurper context).
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  07-Dec-2011:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_process_sps_state_change_cond_function(fbe_base_object_t * base_object, 
                                                    fbe_packet_t * packet)
{
    fbe_status_t                                status;
    fbe_sps_mgmt_t                              *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
 
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    // process SPS Status
    if (sps_mgmt->total_sps_count > 0)
    {
        status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, FBE_SPS_MGMT_PE, FBE_LOCAL_SPS);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, PE processSpsStatus failed, status %d\n", 
                                  __FUNCTION__, status);
        }
    }
    if (sps_mgmt->total_sps_count > FBE_SPS_MGMT_TWO_SPS_COUNT)
    {
        status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, FBE_SPS_MGMT_DAE0, FBE_LOCAL_SPS);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, DAE0 processSpsStatus failed, status %d\n", 
                                  __FUNCTION__, status);
        }
    }

    // process SPS Status
    if (sps_mgmt->total_bob_count > 0)
    {
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);

    }

    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_process_sps_state_change_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_check_test_time_cond_function()
 ****************************************************************
 * @brief
 *  This function checks the Weekly SPS Test time has arrived.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  07-Apr-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_check_test_time_cond_function(fbe_base_object_t * base_object, 
                                           fbe_packet_t * packet)
{
    fbe_status_t                                status;
    fbe_sps_mgmt_t                              *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_system_time_t                           currentTime;
    fbe_sps_bbu_test_info_t                     *persistentTestTime;
    fbe_u32_t                                   spsIndex;
    fbe_u32_t                                   bbuIndex, notBatteryReadyTime;
    fbe_device_physical_location_t              location = {0};
    fbe_u8_t                                    deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_bool_t                                  checkTestTime = TRUE;
    fbe_bool_t                                  testingNeeded = FALSE;
     
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, spsTestTimeDetected %d\n",
                          __FUNCTION__, sps_mgmt->arraySpsInfo->spsTestTimeDetected);

    /*
     * Get Test Time & compare with current time
     */
    
    // get current time
    status = fbe_get_system_time(&currentTime);
    if (status != FBE_STATUS_OK) 
    {
        checkTestTime = FALSE;
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get current system time, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    persistentTestTime = (fbe_sps_bbu_test_info_t *)sps_mgmt->base_environment.my_persistent_data;
    if (persistentTestTime == NULL)
    {
        checkTestTime = FALSE;
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Persistent Data pointer not initialized\n",
                              __FUNCTION__);
    }

    if (checkTestTime)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, SpsTestTime weekday %d, hour %d, min %d\n",
                              __FUNCTION__, 
                              persistentTestTime->testStartTime.weekday, 
                              persistentTestTime->testStartTime.hour, 
                              persistentTestTime->testStartTime.minute);
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, CurrentTime, weekday %d, hour %d, min %d\n",
                              __FUNCTION__, currentTime.weekday, currentTime.hour, currentTime.minute);
       
        // test for SPS Test time
        if (!sps_mgmt->arraySpsInfo->spsTestTimeDetected &&
            (fbe_sps_mgmt_hasTestArrived(&(persistentTestTime->testStartTime))))
        {
            // set flags based on whether SPS's or BBU's present
            if (sps_mgmt->total_sps_count > 0)
            {
                for (spsIndex = 0; spsIndex < sps_mgmt->encls_with_sps; spsIndex++)
                {
                    sps_mgmt->arraySpsInfo->sps_info[spsIndex][FBE_LOCAL_SPS].needToTest = TRUE;
                }
                testingNeeded = TRUE;
                sps_mgmt->arraySpsInfo->needToTest = TRUE;
                sps_mgmt->arraySpsInfo->testingCompleted = FALSE;
                fbe_sps_mgmt_determineExpectedPsAcFails(sps_mgmt, FBE_SPS_MGMT_PE);      // recalculate test data
            }
            else if (sps_mgmt->total_bob_count > 0)
            {
                for (bbuIndex = 0; bbuIndex < sps_mgmt->total_bob_count; bbuIndex++)
                {
                    if ((sps_mgmt->arrayBobInfo->bob_info[bbuIndex].associatedSp == sps_mgmt->base_environment.spid) &&
                        HwAttrLib_isBbuSelfTestSupported(sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFfid))
                    {
                        testingNeeded = TRUE;
                        sps_mgmt->arrayBobInfo->needToTest = TRUE;
                        sps_mgmt->arrayBobInfo->bobNeedsTesting[bbuIndex] = TRUE; 
                    }
                }
            }
            fbe_event_log_write(ESP_INFO_SPS_TEST_TIME,
                                NULL, 0, NULL);
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Sps Test Time Detected, weekday %d, hour %d, min %d\n",
                                  __FUNCTION__, currentTime.weekday, currentTime.hour, currentTime.minute);

            if (testingNeeded)
            {
                sps_mgmt->arraySpsInfo->spsTestType = FBE_SPS_BATTERY_TEST_TYPE;
                sps_mgmt->arraySpsInfo->spsTestTimeDetected = TRUE;
                // set condition to signify that we need to test the SPS
                status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                base_object,  
                                                FBE_SPS_MGMT_TEST_NEEDED_COND);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s can't set TestNeeded condition, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
                else
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s set TestNeeded condition\n",
                                          __FUNCTION__);
                }
            }
            else
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, No BBU/SPS testing needed\n",
                                      __FUNCTION__);
            }
        }
    }

    /*
     * Check if BBU stuck in Charging state
     */
    if (sps_mgmt->total_bob_count > 0)
    {
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
        for (bbuIndex = 0; bbuIndex < sps_mgmt->total_bob_count; bbuIndex++)
        {
            // determine the location slot and sp from the bbu index
            if (bbuIndex < (sps_mgmt->total_bob_count/2))
            {
                location.sp = sps_mgmt-> base_environment.spid;
                location.slot = bbuIndex;
            }
            else
            {
                location.sp = ((sps_mgmt-> base_environment.spid == SP_A) ? SP_B : SP_A);
                location.slot = bbuIndex - (sps_mgmt->total_bob_count/2);
            }

            fbe_zero_memory(&deviceStr, FBE_DEVICE_STRING_LENGTH);
            status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);

            if (!sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryReady &&
                (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFaults == FBE_BATTERY_FAULT_NO_FAULT))
            {
                notBatteryReadyTime = fbe_get_elapsed_seconds(sps_mgmt->arrayBobInfo->notBatteryReadyTimeStart[bbuIndex]);
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, BBU %d, notBatteryReadyTime %d, MaxTime %d\n",
                                      __FUNCTION__, bbuIndex, notBatteryReadyTime, FBE_SPS_MGMT_MAX_BBU_CHARGE_TIME);
                if (notBatteryReadyTime > FBE_SPS_MGMT_MAX_BBU_CHARGE_TIME)
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, BBU %d has exceeded notBatteryReady time, %d seconds\n",
                                          __FUNCTION__, bbuIndex, notBatteryReadyTime);

                    if (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryState == FBE_BATTERY_STATUS_CHARGING)
                    {
                        fbe_event_log_write(ESP_ERROR_SPS_FAULTED,
                                            NULL, 0,
                                            "%s %s %s",
                                            &deviceStr[0], 
                                            fbe_sps_mgmt_getBobFaultString(sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFaults),
                                            fbe_sps_mgmt_getBobFaultString(FBE_BATTERY_FAULT_CANNOT_CHARGE));
                        sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFaults = FBE_BATTERY_FAULT_CANNOT_CHARGE;
                    }
                    else
                    {
                        fbe_event_log_write(ESP_ERROR_SPS_FAULTED,
                                            NULL, 0,
                                            "%s %s %s",
                                            &deviceStr[0], 
                                            fbe_sps_mgmt_getBobFaultString(sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFaults),
                                            fbe_sps_mgmt_getBobFaultString(FBE_BATTERY_FAULT_NOT_READY));
                        sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFaults = FBE_BATTERY_FAULT_NOT_READY;
                    }

                    status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt,
                                                                &location,
                                                                FBE_ENCL_FAULT_LED_BATTERY_FLT);

                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, sending BBU event notification, BBU %d\n",
                                          __FUNCTION__, location.slot);
                    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)sps_mgmt,
                                                                       FBE_CLASS_ID_SPS_MGMT,
                                                                       FBE_DEVICE_TYPE_BATTERY,
                                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                                       &location);
                }
            }
        }
    }

    /*
     * Get the current SPS InputPower
     */
    status = fbe_sps_mgmt_processSpsInputPower(sps_mgmt);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s processSpsInputPower failed, status: 0x%X",
                              __FUNCTION__, status);
    }

    /*
     * Clear this condition (will reset timer)
     */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_check_test_time_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_sps_test_check_needed_cond_function()
 ****************************************************************
 * @brief
 *  This function will start the periodic timer to check on
 *  SPS Test status.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  04-Aug-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_sps_test_check_needed_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
    fbe_status_t                    status;
     
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    /* clear condition to start the timer */
    status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                            base_object, 
                                            FBE_SPS_MGMT_CHECK_STOP_TEST_COND);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear timer condition, status: 0x%X\n",
                              __FUNCTION__, status);
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);

    }

    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_sps_test_check_needed_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_check_stop_test_cond_function()
 ****************************************************************
 * @brief
 *  This function checks if we should stop the SPS Test that
 *	is in progress, based off of type of test.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  04-Aug-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_check_stop_test_cond_function(fbe_base_object_t * base_object, 
                                           fbe_packet_t * packet)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_sps_mgmt_t                  *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_bool_t                      rescheduleCond = FALSE;
    fbe_bool_t                      stopTest = FALSE;
    fbe_u32_t                       spsTestTimeDuration;
    fbe_u32_t                       enclIndex;
    fbe_u32_t                       psIndex;
    fbe_bool_t                      associatedPsFaulted = FALSE;
     
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, testingState %s, spsTestType %d\n",
                          __FUNCTION__, 
                          fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                          sps_mgmt->arraySpsInfo->spsTestType);

    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            if ((sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].associatedSps == FBE_LOCAL_SPS)
                && (sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psFault == TRUE))
            {
                associatedPsFaulted = TRUE;
                break;
            }
        }
    }

    // this SPS is not testing, so no need to check
    if (sps_mgmt->testingState != FBE_SPS_LOCAL_SPS_TESTING)
    {
        // nothing to do
    }
    /*
     * Processing based on SPS Test Type
     */
    else if ((sps_mgmt->arraySpsInfo->spsTestType == FBE_SPS_CABLING_TEST_TYPE) &&
        (fbe_sps_mgmt_checkArrayCablingStatus(sps_mgmt, FALSE) == FBE_SPS_CABLING_VALID))
    {
            // if good cabling found, stop test
            stopTest = TRUE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (SPS_TEST), stop SPS Test, Cabling Valid\n",
                                  __FUNCTION__);
    }
    else if ((sps_mgmt->arraySpsInfo->spsTestType == FBE_SPS_CABLING_TEST_TYPE) &&
        (associatedPsFaulted == TRUE))
    {
        stopTest = TRUE;
        fbe_base_object_trace(base_object, 
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s (SPS_TEST), stop SPS Test due to PS faulted\n",
                      __FUNCTION__);
    }
    else 
    {
        spsTestTimeDuration = fbe_get_elapsed_seconds(sps_mgmt->testStartTime);
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (SPS_TEST), duration %d\n",
                              __FUNCTION__, spsTestTimeDuration);
        if (spsTestTimeDuration >= 90)
        {
            // check if test has been running long enough
            stopTest = TRUE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (SPS_TEST), stop SPS Test, duration %d\n",
                                  __FUNCTION__, spsTestTimeDuration);
        }
        else
        {
            rescheduleCond = TRUE;
        }
    }

    if (stopTest)
    {
        // send message to Phy Pkg
        // Cabling test, send stop to SPS under test
        for (enclIndex = 0; enclIndex < sps_mgmt->encls_with_sps; enclIndex++)
        {
            if (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsStatus == SPS_STATE_TESTING)
            {
                status = fbe_sps_mgmt_sendSpsCommand(sps_mgmt, FBE_SPS_ACTION_STOP_TEST, enclIndex);
                break;
            }
        }
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s (SPS_TEST), stop SPS Test failed, status: 0x%X\n",
                                  __FUNCTION__, status);
            rescheduleCond = TRUE;

            //if SPS testing stoped due to PS fault, treat it as nothing happened.
            if (associatedPsFaulted == TRUE)
            {
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].testInProgress = FALSE;
            }
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (SPS_TEST), SPS Test stopped\n",
                                  __FUNCTION__);
            rescheduleCond = FALSE;
        }
    }

    if (rescheduleCond)
    {
        // Clear this condition (will reset timer)
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s (SPS_TEST), can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (SPS_TEST), reset timer condition\n",
                                  __FUNCTION__);
        }
    }
    else
    {
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt),
                                          (fbe_base_object_t*)sps_mgmt,
                                          FBE_SPS_MGMT_CHECK_STOP_TEST_COND);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (SPS_TEST), timer stopped\n",
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s (SPS_TEST), error stopping timer, status 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_check_stop_test_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_sps_mfg_info_needed_timer_cond_function()
 ****************************************************************
 * @brief
 *  This function will request any needed SPS Mfg Info.  The 
 *  condition will be rescheduled if any Mfg info failed.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  24-Apr-2013:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_sps_mfg_info_needed_timer_cond_function(fbe_base_object_t * base_object, 
                                                    fbe_packet_t * packet)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_sps_mgmt_t                  *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_bool_t                      retryNeeded = FALSE;
    fbe_bool_t                      sendInfoToPeer = FALSE;
    fbe_sps_get_sps_manuf_info_t    spsManufInfo;
    fbe_u32_t                       spsEnclIndex;
    fbe_device_physical_location_t  location;
    fbe_u8_t                        spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                        spsBatteryDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
     
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    // if simulated SPS, no need to test it
    if (sps_mgmt->arraySpsInfo->simulateSpsInfo)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Simulated SPS, do not need to get SPS mfg info.\n");
    }
    else
    {
        for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
        {
            if (sps_mgmt->arraySpsInfo->mfgInfoNeeded[spsEnclIndex])
            {
                // Get Manufacturing Info from new SPS
                status = fbe_sps_mgmt_sendSpsMfgInfoRequest(sps_mgmt, spsEnclIndex, &spsManufInfo);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, %s Error getting Manuf Info, status: 0x%X\n",
                                          __FUNCTION__, 
                                          sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                                          status);
                    retryNeeded = TRUE;
                }
                else
                {
                    sendInfoToPeer = TRUE;
                    sps_mgmt->arraySpsInfo->mfgInfoNeeded[spsEnclIndex] = FALSE;
                    sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsManufInfo = 
                        spsManufInfo.spsManufInfo;
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, %s MfgInfo, SpsFW %s, BattFw %s\n",
                                          __FUNCTION__, 
                                          sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                                          sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsManufInfo.spsModuleManufInfo.spsFirmwareRevision,
                                          sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsManufInfo.spsBatteryManufInfo.spsFirmwareRevision);
                    /*
                     * Generate device strings
                     */
                    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
                    if ((spsEnclIndex == FBE_SPS_MGMT_PE) && (sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE))
                    {
                        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
                        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                    }
                    location.slot = sps_mgmt->base_environment.spid; 
                    location.sp = sps_mgmt->base_environment.spid;
                    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                                               &location, 
                                                               &spsDeviceStr[0],
                                                               FBE_DEVICE_STRING_LENGTH);
                    if (status == FBE_STATUS_OK)
                    {
                        fbe_event_log_write(ESP_INFO_SERIAL_NUM, 
                                        NULL, 0, 
                                        "%s %s", 
                                        &spsDeviceStr[0], 
                                        sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsManufInfo.spsModuleManufInfo.spsSerialNumber);
                    }
                    else
                    {
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                              "%s, Failed to create SPS Module device string.\n", 
                                              __FUNCTION__); 
                    }
                    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS_BATTERY, 
                                                               &location, 
                                                               &spsBatteryDeviceStr[0],
                                                               FBE_DEVICE_STRING_LENGTH);
                    if (status == FBE_STATUS_OK)
                    {
                        fbe_event_log_write(ESP_INFO_SERIAL_NUM, 
                                        NULL, 0, 
                                        "%s %s", 
                                        &spsBatteryDeviceStr[0], 
                                        sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsManufInfo.spsBatteryManufInfo.spsSerialNumber);
                    }
                    else
                    {
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                              "%s, Failed to create SPS Battery device string.\n", 
                                              __FUNCTION__); 
                    }
                }
            }
        }
    

        if (sendInfoToPeer)
        {
            status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, %s Error sending SPS Info to peer, status: 0x%X\n",
                                      __FUNCTION__, 
                                      sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                                      status);
            }
        }
    }

    // Determine if more retries are needed
    if (retryNeeded)
    {
        //retries needed, clear condition to reschedule timer
        status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                base_object, 
                                                FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't reschdule SPS Mfg Info timer condition, status: %d\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        // no retries needed, stop timer
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt),
                                          base_object,
                                          FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: %d\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s clear current condition\n",
                                  __FUNCTION__);
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_sps_mfg_info_needed_timer_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_debounce_timer_cond_function()
 ****************************************************************
 * @brief
 *  This timer function performs Debounce logic for SPS
 *  BatteryOnline status & SPS Faults (we want to ignore short
 *  glitches).
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  06-Dec-2011:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_debounce_timer_cond_function(fbe_base_object_t * base_object, 
                                          fbe_packet_t * packet)
{
    fbe_status_t                    status;
    fbe_sps_mgmt_t                  *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_bool_t                      rescheduleCondSpsStatus = FALSE;
    fbe_bool_t                      rescheduleCondSpsFault = FALSE;
    fbe_bool_t                      rescheduleCondBbuStatus = FALSE;
    fbe_bool_t                      spsStatusFaultChange = FALSE;
    fbe_u32_t                       spsEnclIndex;
    fbe_u32_t                       bobIndex;
    fbe_device_physical_location_t  location;
    fbe_u8_t                        deviceStr[FBE_DEVICE_STRING_LENGTH];


    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, spsCount %d, enclsWithSps %d\n",
                          __FUNCTION__, sps_mgmt->total_sps_count, sps_mgmt->encls_with_sps);

    /*
     * Perform Debouncing of SPS Status & Faults
     */
    if (sps_mgmt->total_sps_count > 0)
    {
        for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
        {
            spsStatusFaultChange = FALSE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, spsEnclIndex %d, debouncedSpsStatus %d, count %d\n",
                                  __FUNCTION__, 
                                  spsEnclIndex,
                                  sps_mgmt->arraySpsInfo->debouncedSpsStatus[spsEnclIndex], 
                                  sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsEnclIndex]);
            if (sps_mgmt->arraySpsInfo->debouncedSpsStatus[spsEnclIndex] == SPS_STATE_ON_BATTERY)
            {
                if (++sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsEnclIndex] > FBE_SPS_STATUS_DEBOUNCE_COUNT)
                {
                    spsStatusFaultChange = TRUE;
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, spsEnclIndex %d, SPS Status debounce count exceeded\n",
                                          __FUNCTION__, spsEnclIndex);
                }
                else
                {
                    rescheduleCondSpsStatus = TRUE;
                }
            }

            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, spsEnclIndex %d, debouncedSpsFaults %s, count %d\n",
                                  __FUNCTION__, 
                                  spsEnclIndex,
                                  fbe_sps_mgmt_getSpsFaultString(&sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsEnclIndex]), 
                                  sps_mgmt->arraySpsInfo->debouncedSpsFaultsCount[spsEnclIndex]);
            if (sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsEnclIndex].spsModuleFault ||
                sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsEnclIndex].spsBatteryFault)
            {
                if (++sps_mgmt->arraySpsInfo->debouncedSpsFaultsCount[spsEnclIndex] > FBE_SPS_FAULT_DEBOUNCE_COUNT)
                {
                    spsStatusFaultChange = TRUE;
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, spsEnclIndex %d, SPS Fault debounce count exceeded\n",
                                          __FUNCTION__, spsEnclIndex);
                }
                else
                {
                    rescheduleCondSpsFault = TRUE;
                }
            }

            // if any debouncing ended, process the new status
            if (spsStatusFaultChange)
            {
                status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, spsEnclIndex, FBE_LOCAL_SPS);
            }

        }
    
        /*
         * Check if any SPS's should be shut down (SPS OnBattery & other side has power)
         */
        for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, spsEnclIndex %d, spsStatus local %d\n",
                                  __FUNCTION__,
                                  sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsStatus,
                                  sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS].spsStatus);

            if (sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsStatus == SPS_STATE_ON_BATTERY)
            {
                /*
                 * Check if SPS should be shutdown
                 */
                if (fbe_sps_mgmt_okToShutdownSps(sps_mgmt, spsEnclIndex))
                {
                    // send message to Phy Pkg
                    status = fbe_sps_mgmt_sendSpsCommand(sps_mgmt, FBE_SPS_ACTION_SHUTDOWN, spsEnclIndex);
                    if (status == FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s, Local side PowerFail, shutdown SPS\n",
                                              __FUNCTION__);
                    }
                    else
                    {
                        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s, Local side PowerFail, SPS shutdown failed, status 0x%x\n",
                                              __FUNCTION__, status);
                    }
                }
            }
        }

        /*
            ** check if it's long enough to process SPS staus from DAE0 LCC FUP activation.
            */
        if (sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFup == FBE_TRUE)
        {
            if (++sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFupCount >= FBE_SPS_STATUS_IGNORE_AFTER_LCC_FUP_COUNT)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, SPS status delay count exceeded, count %d\n",
                                      __FUNCTION__,
                                      sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFupCount);
                status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, FBE_SPS_MGMT_DAE0, FBE_LOCAL_SPS);
            }
            else
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, SPS status delay count %d\n",
                                      __FUNCTION__,
                                      sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFupCount);
                rescheduleCondSpsStatus = TRUE;
            }
        }

    }

    /*
     * Perform Debouncing of BoB (BBU) Status
     */
    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
    for (bobIndex=0; bobIndex<sps_mgmt->total_bob_count; bobIndex++)
    {
        // determine the location slot and sp from the bbu index
        if (bobIndex < (sps_mgmt->total_bob_count/2))
        {
            location.sp = sps_mgmt-> base_environment.spid;
            location.slot = bobIndex;
        }
        else
        {
            location.sp = ((sps_mgmt-> base_environment.spid == SP_A) ? SP_B : SP_A);
            location.slot = bobIndex - (sps_mgmt->total_bob_count/2);
        }

        fbe_zero_memory(&deviceStr, FBE_DEVICE_STRING_LENGTH);
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                                   &location, 
                                                   &deviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);

        if (sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bobIndex] > 0)
        {

            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s debouncedOnBattStat %d, debouncedOnBattStatCnt %d\n",
                                  __FUNCTION__, &deviceStr[0], 
                                  sps_mgmt->arrayBobInfo->debouncedBobOnBatteryState[bobIndex], 
                                  sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bobIndex]);


            if (++sps_mgmt->arrayBobInfo->debouncedBobOnBatteryStateCount[bobIndex] > FBE_SPS_STATUS_DEBOUNCE_COUNT)
            {
                status = fbe_sps_mgmt_processBobStatus(sps_mgmt, &location);
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, %s Status debounce count exceeded\n",
                                      __FUNCTION__,
                                      &deviceStr[0]); 
            }
            else
            {
                rescheduleCondBbuStatus = TRUE;
            }
        }
    }

    if (rescheduleCondSpsStatus || rescheduleCondSpsFault || rescheduleCondBbuStatus)
    {
        // Clear this condition (will reset timer)
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, reset timer condition\n",
                                  __FUNCTION__);
        }
    }
    else
    {
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt),
                                          (fbe_base_object_t*)sps_mgmt,
                                          FBE_SPS_MGMT_DEBOUNCE_TIMER_COND);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (SPS_TEST), timer stopped\n",
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s (SPS_TEST), error stopping timer, status 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_debounce_timer_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_hung_sps_test_timer_cond_function()
 ****************************************************************
 * @brief
 *  This timer function checks to see if SPS Test needs to be
 *  started, but has not.  It will start the test if possible.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  06-Jul-2012:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_hung_sps_test_timer_cond_function(fbe_base_object_t * base_object, 
                                               fbe_packet_t * packet)
{
    fbe_status_t                    status;
    fbe_sps_mgmt_t                  *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_bool_t                      rescheduleCond = FALSE;
     
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    // waiting for peer SP response, resend it
    if (sps_mgmt->testingState == FBE_SPS_WAIT_FOR_PEER_PERMISSION)
    {
        status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_TEST_REQ_PERMISSION);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error sending ReqPermission to peer, status: 0x%X\n",
                                   __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Resending ReqPermission to peer\n",
                                  __FUNCTION__);
        }
        rescheduleCond = TRUE;
    }

    if (rescheduleCond)
    {
        // Clear this condition (will reset timer)
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, reset timer condition\n",
                                  __FUNCTION__);
        }
    }
    else
    {
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt),
                                          (fbe_base_object_t*)sps_mgmt,
                                          FBE_SPS_MGMT_HUNG_SPS_TEST_TIMER_COND);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (SPS_TEST), timer stopped\n",
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s (SPS_TEST), error stopping timer, status 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_hung_sps_test_timer_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_read_persistent_data_complete_cond_function()
 ****************************************************************
 * @brief
 *  This function handles the persistent storage read completion.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 * 
 * 
 * @author
 *  28-Nov-2012 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, 
                                                         fbe_packet_t * packet)
{
    fbe_sps_mgmt_t              *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_sps_bbu_test_info_t     *persistent_data_p;
    fbe_status_t                status;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry\n",
                          __FUNCTION__);

    status = fbe_lifecycle_clear_current_cond(base_object);

    persistent_data_p = (fbe_sps_bbu_test_info_t *) ((fbe_base_environment_t *)sps_mgmt)->my_persistent_data;

    // if data not initialized, set to defaults
    if ((persistent_data_p->revision != FBE_SPS_MGMT_PERSISTENT_STORAGE_REVISION) ||
         (persistent_data_p->testTimeValid != FBE_SPS_BBU_TEST_TIME_CANARY))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, data uninitialized, set default, rev %d, Valid 0x%x\n",
                              __FUNCTION__,
                              persistent_data_p->revision,
                              persistent_data_p->testTimeValid);
        persistent_data_p->revision = FBE_SPS_MGMT_PERSISTENT_STORAGE_REVISION;
        persistent_data_p->testTimeValid = FBE_SPS_BBU_TEST_TIME_CANARY;
        persistent_data_p->testStartTime.weekday = FBE_SPS_DEFAULT_TEST_TIME_DAY;
        persistent_data_p->testStartTime.hour = FBE_SPS_DEFAULT_TEST_TIME_HOUR;
        persistent_data_p->testStartTime.minute = FBE_SPS_DEFAULT_TEST_TIME_MIN;

        status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)sps_mgmt);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s persistent write error, status: 0x%X",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        // data good, so copy to our local copy
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, TestTimeInfo, day %d, hour %d, min %d\n",
                              __FUNCTION__,
                              persistent_data_p->testStartTime.weekday,
                              persistent_data_p->testStartTime.hour,
                              persistent_data_p->testStartTime.minute);
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;

}   // end of fbe_sps_mgmt_read_persistent_data_complete_cond_function


/*!**************************************************************
 * fbe_sps_mgmt_hasTestArrived()
 ****************************************************************
 * @brief
 *  This function is called determine if the SPS Test Time has
 *  arrive (compare the current time against supplied time).
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  16-Jun-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_bool_t fbe_sps_mgmt_hasTestArrived(fbe_system_time_t *spsTestTime)
{
    fbe_bool_t                  timeArrived = FALSE;
    fbe_status_t                status;
    fbe_system_time_t           currentTime;

    // get the current time
    status = fbe_get_system_time(&currentTime);
    if (status == FBE_STATUS_OK) 
    {
        // check against SPS Test Time
        if (currentTime.weekday == spsTestTime->weekday)
        {
            if (currentTime.hour == spsTestTime->hour)
            {
                if (currentTime.minute == spsTestTime->minute)
                {
                    timeArrived = TRUE;
                }
            }
        }
    }

    return timeArrived;

}
/******************************************************
 * end fbe_sps_mgmt_hasTestArrived() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_test_needed_cond_function()
 ****************************************************************
 * @brief
 *  This function is called when the SPS needs to be tested.
 *  There will be checks to see if the array is OK to perform
 *  an SPS test (no PS faults, other SPS not faulted, cache is
 *  stable) & if it is OK, permission will be requested from 
 *  the peer SP.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the SPS management's
 *                        constant lifecycle data.
 *
 * @author
 *  16-Jun-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_sps_mgmt_test_needed_cond_function(fbe_base_object_t * base_object, 
                                       fbe_packet_t * packet)
{
    fbe_status_t                                status;
    fbe_sps_mgmt_t                              *sps_mgmt = (fbe_sps_mgmt_t *) base_object;
    fbe_bool_t                  clearCondition  = FALSE;
    fbe_sps_ps_encl_num_t       spsEnclToTest;
    fbe_bool_t                  testInProgress = FALSE;
    fbe_u32_t                   enclIndex;
    fbe_device_physical_location_t              bbuToTestLocation;
     
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, needToTest %d\n",
                          __FUNCTION__, sps_mgmt->arrayBobInfo->needToTest);

    /*
     * Determine if it is OK to run an SPS test
     */
    // if simulated SPS, no need to test it
    if (sps_mgmt->arraySpsInfo->simulateSpsInfo)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, simulated SPS, do not test\n",
                              __FUNCTION__);
        clearCondition = TRUE;
    }
    else if (((sps_mgmt->total_sps_count > 0) && !sps_mgmt->arraySpsInfo->needToTest) ||
             ((sps_mgmt->total_bob_count > 0) && !sps_mgmt->arrayBobInfo->needToTest))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, no testing needed, clear condition\n",
                              __FUNCTION__);
        clearCondition = TRUE;
    }
    else if ((sps_mgmt->total_sps_count > 0) && !fbe_sps_mgmt_okToStartSpsTest(sps_mgmt, &spsEnclToTest))
    {
        // cannot start test with faulted SPS
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, SPS not OK to test\n",
                              __FUNCTION__);
    }
    else if((sps_mgmt->total_bob_count > 0) && !fbe_sps_mgmt_okToStartBbuTest(sps_mgmt, &bbuToTestLocation))
    {
        // cannot start test BBU Self Test, determine if supported or if BBU's not ready
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, BBU is not OK to do test\n",
                              __FUNCTION__);
    }

/*
    else if ()
    {
        // cannot start test with faulted PS('s)
    }
    else if ()
    {
        // cannot start test with unstable Cache
    }
*/
    else
    {
        // check if any SPS Test in progress
        for (enclIndex = 0; enclIndex < sps_mgmt->encls_with_sps; enclIndex++)
        {
            if (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsModulePresent &&
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsBatteryPresent &&
                sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].testInProgress)
            {
                testInProgress = TRUE;
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, encl %d, localSps testInProgress\n",
                                      __FUNCTION__, enclIndex);
                break;
            }
        }

        if ((sps_mgmt->testingState == FBE_SPS_LOCAL_SPS_TESTING) && testInProgress)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, testingState %s, wait to start another SPS test\n",
                                  __FUNCTION__,
                                  fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState));
        }
        else
        {
    
            // OK to test, ask permission from peer SP
            status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_TEST_REQ_PERMISSION);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error sending ReqPermission to peer, status: 0x%X\n",
                                       __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s SpsTestState change, %s to %s\n",
                                      __FUNCTION__, 
                                      fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                                      fbe_sps_mgmt_getSpsTestStateString(FBE_SPS_WAIT_FOR_PEER_PERMISSION));
                sps_mgmt->testingState = FBE_SPS_WAIT_FOR_PEER_PERMISSION;
                clearCondition = TRUE;
                // clear condition to start the timer
                status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                        base_object, 
                                                        FBE_SPS_MGMT_HUNG_SPS_TEST_TIMER_COND);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s can't clear Hung SPS Test timer condition, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
            }
        }
    }

    // clear condition
    if (clearCondition)
    {
            status = fbe_lifecycle_clear_current_cond(base_object);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't clear current condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
            else
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_DEBUG_LOW,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s clear current condition\n",
                                      __FUNCTION__);
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_sps_mgmt_test_needed_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_sendCmiMessage()
 ****************************************************************
 * @brief
 *  This function sends the specified SPS Mgmt message to the
 *  peer SP.
 *
 * @param object_handle - This is the object handle, or in our
 * case the sps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_base_environment_send_cmi_message()
 *
 * @author
 *  11-June-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t
fbe_sps_mgmt_sendCmiMessage(fbe_sps_mgmt_t *sps_mgmt, 
                            fbe_u32_t MsgType)
{
    fbe_status_t                status;
    fbe_u32_t                   spsEnclIndex;

    // format the message
    fbe_set_memory(&SPS_MGMT_spsCmiMsg, 0, sizeof(fbe_sps_mgmt_cmi_msg_t));
    SPS_MGMT_spsCmiMsg.msgType.opcode = MsgType;
    if (MsgType == FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO)
    {
        SPS_MGMT_spsCmiMsg.spsCount = sps_mgmt->encls_with_sps;
        for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
        {
            SPS_MGMT_spsCmiMsg.sendSpsInfo[spsEnclIndex] = 
                sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS];
        }
    }
    else if (MsgType == FBE_BASE_ENV_CMI_MSG_TEST_REQ_PERMISSION)
    {
        // if Weekly Test (Battery), need to test both SPS's (only do this first request for permission)
        if ((sps_mgmt->arraySpsInfo->spsTestType == FBE_SPS_BATTERY_TEST_TYPE) &&
            (sps_mgmt->testingState == FBE_SPS_NO_TESTING_IN_PROGRESS))
        {
            SPS_MGMT_spsCmiMsg.testBothSps = TRUE;
        }
        else
        {
            SPS_MGMT_spsCmiMsg.testBothSps = FALSE;
        }
    }
    else if (MsgType == FBE_BASE_ENV_CMI_MSG_PEER_BBU_TEST_STATUS)
    {
        if (sps_mgmt->base_environment.spid == SP_A)
        {
            SPS_MGMT_spsCmiMsg.bbuTestStatus = sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].batteryTestStatus;
        }
        else
        {
            SPS_MGMT_spsCmiMsg.bbuTestStatus = sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].batteryTestStatus;
        }
    }
    else if (MsgType == FBE_BASE_ENV_CMI_MSG_TEST_COMPLETED)
    {
        sps_mgmt->arraySpsInfo->spsTestTimeDetected = FALSE;
    }
    else if (MsgType == FBE_BASE_ENV_CMI_MSG_PEER_DISK_BATTERY_BACKED_SET)
    {
        SPS_MGMT_spsCmiMsg.diskBatteryBackedSet = sps_mgmt->arrayBobInfo->localDiskBatteryBackedSet;
    }

    // send the message to peer SP
    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)sps_mgmt,
                                                   sizeof(fbe_sps_mgmt_cmi_msg_t),
                                                   &SPS_MGMT_spsCmiMsg);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending %s to peer, status: 0x%X\n",
                              __FUNCTION__, fbe_sps_mgmt_getSpsMsgTypeString(MsgType), status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s sent %s to peer, testBothSps %d\n",
                              __FUNCTION__, 
                              fbe_sps_mgmt_getSpsMsgTypeString(MsgType),
                              SPS_MGMT_spsCmiMsg.testBothSps);
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_sendCmiMessage() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_initiateTest()
 ****************************************************************
 * @brief
 *  This function determines if this array has SPS's or BBU's
 *  and calls the appropriate function to start testing.
 *
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of whether SPS/BBU test started.
 *
 * @author
 *  26-Nov-2012:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t
fbe_sps_mgmt_initiateTest(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_status_t        status = FBE_STATUS_OK;

    if (sps_mgmt->total_sps_count > 0)
    {
        status = fbe_sps_mgmt_initiateSpsTest(sps_mgmt);
    }
    else if (sps_mgmt->total_bob_count > 0)
    {
        status = fbe_sps_mgmt_initiateBbuTest(sps_mgmt);
    }

    return status;

}   // end of fbe_sps_mgmt_initiateTest

/*!**************************************************************
 * fbe_sps_mgmt_initiateSpsTest()
 ****************************************************************
 * @brief
 *  This function determines if this array can start SPS testing.
 *
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of whether SPS/BBU test started.
 *
 * @author
 *  26-Nov-2012:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t
fbe_sps_mgmt_initiateSpsTest(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u8_t                enclIndex, psIndex;
    fbe_sps_ps_encl_num_t   spsEnclToTest;

    // if SPS not ready for testing, check it in a little while
    if (!fbe_sps_mgmt_okToStartSpsTest(sps_mgmt, &spsEnclToTest))
    {
        sps_mgmt->arraySpsInfo->needToTest = TRUE;
        sps_mgmt->arraySpsInfo->testingCompleted = FALSE;
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (SPS_TEST), SPS's not ready for testing\n",
                              __FUNCTION__);
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                        (fbe_base_object_t *)sps_mgmt,
                                        FBE_SPS_MGMT_TEST_NEEDED_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set TestNeeded condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        return status;
    }

    // clear Test data
    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailDetected = FALSE;
        }
    }
    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, encl %d, AcFailDetected %d %d\n",
                          __FUNCTION__, 0, 
                          sps_mgmt->arraySpsInfo->spsAcFailInfo[0][0].acFailDetected,
                          sps_mgmt->arraySpsInfo->spsAcFailInfo[0][1].acFailDetected);

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, encl %d, AcFailDetected %d %d\n",
                          __FUNCTION__, 1, 
                          sps_mgmt->arraySpsInfo->spsAcFailInfo[1][0].acFailDetected,
                          sps_mgmt->arraySpsInfo->spsAcFailInfo[1][1].acFailDetected);

    // send message to Phy Pkg
    fbe_sps_mgmt_determineExpectedPsAcFails(sps_mgmt, spsEnclToTest);      // recalculate test data
    status = fbe_sps_mgmt_sendSpsCommand(sps_mgmt, FBE_SPS_ACTION_START_TEST, spsEnclToTest);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (SPS_TEST), start SPS Test failed, status: %d\n",
                              __FUNCTION__, status);
            status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                            (fbe_base_object_t *)sps_mgmt,
                                            FBE_SPS_MGMT_TEST_NEEDED_COND);
            if (status == FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, can't start SPS Test, so reschedule TestNeeded condition.\n",
                                      __FUNCTION__);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't set TestNeeded condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
    }
    else
    {
        sps_mgmt->arraySpsInfo->sps_info[spsEnclToTest][FBE_LOCAL_SPS].testInProgress = TRUE;
        sps_mgmt->testStartTime = fbe_get_time();
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (SPS_TEST), Sps Test intitiated\n",
                              __FUNCTION__);
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (SPS_TEST), SpsTestState change, %s to %s\n",
                              __FUNCTION__, 
                              fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                                  fbe_sps_mgmt_getSpsTestStateString(FBE_SPS_LOCAL_SPS_TESTING));
        sps_mgmt->testingState = FBE_SPS_LOCAL_SPS_TESTING;
        // Clear this condition (will reset timer)
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                        (fbe_base_object_t *)sps_mgmt,
                                        FBE_SPS_MGMT_SPS_TEST_CHECK_NEEDED_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s (SPS_TEST), can't set SpsTestCheckNeeded condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_initiateSpsTest() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_initiateBbuTest()
 ****************************************************************
 * @brief
 *  This function determines if this array can start BBU testing.
 *
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of whether SPS/BBU test started.
 *
 * @author
 *  26-Nov-2012:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t
fbe_sps_mgmt_initiateBbuTest(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_status_t                            status;
    fbe_base_board_mgmt_battery_command_t   batteryCommand;
    fbe_device_physical_location_t          bbuToTestLocation;

    // if BBU not ready for testing, check it in a little while
    fbe_zero_memory(&bbuToTestLocation, sizeof(fbe_device_physical_location_t));
    if (!fbe_sps_mgmt_okToStartBbuTest(sps_mgmt, &bbuToTestLocation))
    {
        sps_mgmt->arrayBobInfo->needToTest = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (BBU_TEST), BBU not ready for testing\n",
                              __FUNCTION__);
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                        (fbe_base_object_t *)sps_mgmt,
                                        FBE_SPS_MGMT_TEST_NEEDED_COND);
        if (status != FBE_STATUS_OK) 
        {

            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set TestNeeded condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        return status;
    }

/*
    if (!HwAttrLib_isBbuSelfTestSupported(
        sps_mgmt->arrayBobInfo->bob_info[sps_mgmt->base_environment.spid].batteryFfid))
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, BBU %d does not support Self Test, ffid 0x%X\n",
                              __FUNCTION__,
                              sps_mgmt->base_environment.spid,
                              sps_mgmt->arrayBobInfo->bob_info[sps_mgmt->base_environment.spid].batteryFfid);
        sps_mgmt->arrayBobInfo->needToTest = FALSE;
        return FBE_STATUS_CANCELED;
    }
*/

    // send message to Phy Pkg
    fbe_zero_memory(&batteryCommand, sizeof(fbe_base_board_mgmt_battery_command_t));
    batteryCommand.batteryAction = FBE_BATTERY_ACTION_START_TEST;
    batteryCommand.device_location = bbuToTestLocation;
    status = fbe_api_board_send_battery_command(sps_mgmt->arrayBobInfo->bobObjectId, &batteryCommand);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (BBU_TEST), start BBU Test failed, sp %d, slot %d, status: 0x%x\n",
                              __FUNCTION__, 
                              batteryCommand.device_location.sp,
                              batteryCommand.device_location.slot,
                              status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s (BBU_TEST), BBU Test intitiated, sp %d, slot %d\n",
                              __FUNCTION__,
                              batteryCommand.device_location.sp,
                              batteryCommand.device_location.slot);
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s SpsTestState change, %s to %s\n",
                              __FUNCTION__, 
                              fbe_sps_mgmt_getSpsTestStateString(sps_mgmt->testingState),
                              fbe_sps_mgmt_getSpsTestStateString(FBE_SPS_LOCAL_SPS_TESTING));
        sps_mgmt->testingState = FBE_SPS_LOCAL_SPS_TESTING;
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_initiateBbuTest() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_processSpsInputPower()
 ****************************************************************
 * @brief
 *  This function determines the current InputPower values for
 *  the SPS's and performs the necessary calculations.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_base_environment_send_cmi_message()
 *
 * @author
 *  28-June-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t
fbe_sps_mgmt_processSpsInputPower(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_eir_input_power_sample_t    currentInputPower;
    fbe_eir_input_power_sample_t    *sampleInputPowerPtr;
    fbe_eir_input_power_sample_t    *currentInputPowerPtr, *maxInputPowerPtr, *averageInputPowerPtr;
    fbe_u8_t                        spsIndex, enclIndex;
    fbe_u8_t                        sampleIndex;
    fbe_u8_t                        validSampleCount;
    fbe_u32_t                       inputPowerSum;

    // update count
    if (sps_mgmt->spsEirData->spsSampleCount < FBE_SAMPLE_DATA_SIZE)
    {
        sps_mgmt->spsEirData->spsSampleCount++;
    }

    for (enclIndex = 0; enclIndex < (sps_mgmt->total_sps_count/FBE_SPS_MGMT_SPS_PER_ENCL); enclIndex++)
    {
        for (spsIndex = 0; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
        {
            fbe_sps_mgmt_determineSpsInputPower(sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsBatteryPresent,
                                                sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsStatus,
                                                sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsModel,
                                                &currentInputPower);
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s(EIR), Sps %d InputPower %d, status 0x%x\n",
                                  __FUNCTION__, spsIndex, currentInputPower.inputPower, currentInputPower.status);
    
            /*
             * Perform EIR calculations
             */
            // save current Input Power & sample
            currentInputPowerPtr = 
                &(sps_mgmt->spsEirData->spsCurrentInputPower[enclIndex][spsIndex]);
            *currentInputPowerPtr = currentInputPower;
            sampleInputPowerPtr = 
                &(sps_mgmt->spsEirData->spsInputPowerSamples[enclIndex][spsIndex][sps_mgmt->spsEirData->spsSampleIndex]);
            *sampleInputPowerPtr = currentInputPower;
            // clear Max Input Power (will re-calculate over samples)
            maxInputPowerPtr = &(sps_mgmt->spsEirData->spsMaxInputPower[enclIndex][spsIndex]);
            maxInputPowerPtr->inputPower = 0;
            maxInputPowerPtr->status = FBE_ENERGY_STAT_UNINITIALIZED;
            // clear Average Input Power (will re-calculate over samples)
            averageInputPowerPtr = &(sps_mgmt->spsEirData->spsAverageInputPower[enclIndex][spsIndex]);
            averageInputPowerPtr->inputPower = 0;
            averageInputPowerPtr->status = FBE_ENERGY_STAT_UNINITIALIZED;
    
            // go through samples & calculate stats
            validSampleCount = 0;
            inputPowerSum = 0;
            for (sampleIndex = 0; sampleIndex < sps_mgmt->spsEirData->spsSampleCount; sampleIndex++)
            {
                // get next sample
                sampleInputPowerPtr = &(sps_mgmt->spsEirData->spsInputPowerSamples[enclIndex][spsIndex][sampleIndex]);
    
                if (sampleInputPowerPtr->status == FBE_ENERGY_STAT_VALID)
                {
                    validSampleCount++;
                    // determine if we have a new Max value
                    if (sampleInputPowerPtr->inputPower > maxInputPowerPtr->inputPower)
                    {
                        *maxInputPowerPtr = *sampleInputPowerPtr;
                    }
    
                    // accumulate for average
                    inputPowerSum += sampleInputPowerPtr->inputPower;
    
                }
                else
                {
                    // update status for average
                    averageInputPowerPtr->status |= sampleInputPowerPtr->status;
                }
    
            }   // end of for sampleIndex
    
            if (validSampleCount > 0)
            {
                averageInputPowerPtr->inputPower = inputPowerSum / validSampleCount;
                averageInputPowerPtr->status = FBE_ENERGY_STAT_VALID;
                //if sample too small, set the bit
                if (validSampleCount < FBE_SAMPLE_DATA_SIZE)
                {
                    averageInputPowerPtr->status |= FBE_ENERGY_STAT_SAMPLE_TOO_SMALL;
                }
            }
            else if (validSampleCount == 0)
            {
                averageInputPowerPtr->status = FBE_ENERGY_STAT_UNINITIALIZED;
            }
    
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s(EIR), Sps %d InputPower: Current %d, status 0x%x\n",
                                  __FUNCTION__, spsIndex, currentInputPowerPtr->inputPower, currentInputPowerPtr->status);
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s(EIR), Sps %d InputPower: Max %d, status 0x%x\n",
                                  __FUNCTION__, spsIndex, maxInputPowerPtr->inputPower, maxInputPowerPtr->status);
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s(EIR), Sps %d InputPower: Average %d, status 0x%x, sum %d, cnt %d\n",
                                  __FUNCTION__, spsIndex, averageInputPowerPtr->inputPower, averageInputPowerPtr->status,
                                  inputPowerSum, validSampleCount);
    
        }   // end of for spsIndex
    }

    // update indexes
    if (++sps_mgmt->spsEirData->spsSampleIndex >= FBE_SAMPLE_DATA_SIZE)
    {
        sps_mgmt->spsEirData->spsSampleIndex = 0;
    }

    return status;

}
/******************************************************
 * end fbe_sps_mgmt_processSpsInputPower() 
 ******************************************************/


/*!**************************************************************
 * fbe_sps_mgmt_processPeerSpsInfo()
 ****************************************************************
 * @brief
 *  This function processes peer SPS info via CMI messaging.
 *  There is also special processing for when the peer SP is
 *  gone (create fake CMI Message data for removed SPS).
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 * @param peerSpGone - boolean indicating whether peer SP is gone.
 * @param cmiMsgPtr - pointer to CMI message.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_base_environment_send_cmi_message()
 *
 * @author
 *  28-June-2010:  Created. Joe Perry
 *
 ****************************************************************/
void fbe_sps_mgmt_processPeerSpsInfo(fbe_sps_mgmt_t *sps_mgmt, 
                                     fbe_bool_t peerSpGone,
                                     fbe_sps_mgmt_cmi_msg_t *cmiMsgPtr)
{
    fbe_sps_info_t      *spsInfoPtr;
    fbe_device_physical_location_t          location = {0};
    fbe_device_physical_location_t          xpeLocation = {0};
    fbe_u32_t           spsEnclIndex;
    fbe_bool_t          stateChange;
    fbe_sps_mgmt_cmi_msg_t tempCmiMsg;
    fbe_u8_t                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                spsBatteryDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                peerSpId;
    fbe_status_t            status, status1;
    fbe_bool_t              fupCheckNeeded = FALSE;

    if (sps_mgmt->total_sps_count == 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, no SPS's so don't do anything\n",
                              __FUNCTION__);
        return;
    }

    if (sps_mgmt->base_environment.spid == SP_A)
    {
        peerSpId = SP_B;
    }
    else
    {
        peerSpId = SP_A;
    }

    // if peer SP gone, create CMI data for removed SPS's
    if (peerSpGone)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, peerSpGone TRUE, create CMI data\n",
                              __FUNCTION__);
        fbe_set_memory(&tempCmiMsg, 0, sizeof(fbe_sps_mgmt_cmi_msg_t));
        for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
        {
            tempCmiMsg.sendSpsInfo[spsEnclIndex].spsModulePresent = FALSE;
            tempCmiMsg.sendSpsInfo[spsEnclIndex].spsBatteryPresent = FALSE;
            tempCmiMsg.sendSpsInfo[spsEnclIndex].spsStatus = SPS_STATE_UNKNOWN;
            tempCmiMsg.sendSpsInfo[spsEnclIndex].spsCablingStatus = FBE_SPS_CABLING_UNKNOWN;
            tempCmiMsg.sendSpsInfo[spsEnclIndex].spsModel = SPS_TYPE_UNKNOWN;
        }
        tempCmiMsg.spsCount = sps_mgmt->encls_with_sps;
        cmiMsgPtr = &tempCmiMsg;
    }

    for (spsEnclIndex = 0; spsEnclIndex < cmiMsgPtr->spsCount; spsEnclIndex++)
    {
        /*
         * Generate device strings
         */
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
        if ((spsEnclIndex == FBE_SPS_MGMT_PE) && (sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE))
        {
            location.bus = FBE_XPE_PSEUDO_BUS_NUM;
            location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        location.slot = peerSpId; 
        location.sp = sps_mgmt->base_environment.spid;
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                                   &location, 
                                                   &spsDeviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        status1 = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS_BATTERY, 
                                                   &location, 
                                                   &spsBatteryDeviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        if ((status != FBE_STATUS_OK) || (status1 != FBE_STATUS_OK))
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s, Failed to create device string.\n", 
                                  __FUNCTION__); 
            return;
        }
        
        spsInfoPtr = &(sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS]);
        stateChange = FALSE;
    
        
        // check SPS Status 
        if (spsInfoPtr->spsStatus != cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsStatus)
        {
            stateChange = TRUE;
            if(SPS_STATE_AVAILABLE == cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsStatus)
            {
                fupCheckNeeded = TRUE;
            }
            fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                NULL, 0,
                                "%s %s %s", 
                                &spsDeviceStr[0],
                                fbe_sps_mgmt_getSpsStatusString(spsInfoPtr->spsStatus),
                                fbe_sps_mgmt_getSpsStatusString(cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsStatus));
        }
    
        if (!spsInfoPtr->spsModulePresent && cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsModulePresent)
        {
            fupCheckNeeded = TRUE;
            fbe_event_log_write(ESP_INFO_SPS_INSERTED,
                                NULL, 0,
                                "%s",
                                &spsDeviceStr[0]);
        }
        else if (spsInfoPtr->spsModulePresent && !cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsModulePresent)
        {
            fbe_event_log_write(ESP_ERROR_SPS_REMOVED,
                                NULL, 0,
                                "%s",
                                &spsDeviceStr[0]);
        }

        if (!spsInfoPtr->spsBatteryPresent && cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsBatteryPresent)
        {
            fupCheckNeeded = TRUE;
            fbe_event_log_write(ESP_INFO_SPS_INSERTED,
                                NULL, 0,
                                "%s",
                                &spsBatteryDeviceStr[0]);
        }
        else if (spsInfoPtr->spsBatteryPresent && !cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsBatteryPresent)
        {
            fbe_event_log_write(ESP_ERROR_SPS_REMOVED,
                                NULL, 0,
                                "%s",
                                &spsBatteryDeviceStr[0]);
        }
    
        // check for Serial Numbers available
        if (cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsModulePresent &&
            (strncmp(spsInfoPtr->spsManufInfo.spsModuleManufInfo.spsSerialNumber,
                     cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsManufInfo.spsModuleManufInfo.spsSerialNumber,
                     FBE_SPS_SERIAL_NUM_REVSION_SIZE) != 0))
        {
            fbe_event_log_write(ESP_INFO_SERIAL_NUM,
                                NULL, 0,
                                "%s %s", 
                                &spsDeviceStr[0],
                                cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsManufInfo.spsModuleManufInfo.spsSerialNumber);
        }
        if (cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsBatteryPresent &&
            (strncmp(spsInfoPtr->spsManufInfo.spsBatteryManufInfo.spsSerialNumber,
                     cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsManufInfo.spsBatteryManufInfo.spsSerialNumber,
                     FBE_SPS_SERIAL_NUM_REVSION_SIZE) != 0))
        {
            fbe_event_log_write(ESP_INFO_SERIAL_NUM,
                                NULL, 0,
                                "%s %s", 
                                &spsBatteryDeviceStr[0],
                                cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsManufInfo.spsBatteryManufInfo.spsSerialNumber);
        }

        // check SPS Cabling Status
        if (sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS].spsCablingStatus != 
            cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsCablingStatus)
        {
            stateChange = TRUE;
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s Cabling Status change, %s to %s\n",
                                  __FUNCTION__, 
                                  spsInfoPtr->spsNameString,
                                  fbe_sps_mgmt_getSpsCablingStatusString(sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS].spsCablingStatus),
                                  fbe_sps_mgmt_getSpsCablingStatusString(cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsCablingStatus));
    
            if (cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsCablingStatus == FBE_SPS_CABLING_VALID)
            {
                fbe_event_log_write(ESP_INFO_SPS_CABLING_VALID,
                                    NULL, 0,
                                    "%s",
                                    &spsDeviceStr[0]);
            }
            else if (cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsModulePresent &&
                     cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsBatteryPresent)
            {
                fbe_event_log_write(ESP_ERROR_SPS_CABLING_FAULT,
                                    NULL, 0,
                                    "%s %s",
                                    &spsDeviceStr[0],
                                    fbe_sps_mgmt_getSpsCablingStatusString(cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsCablingStatus));
            }
        }

        // check SPS Faults
        if ((cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsModulePresent &&
                cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsFaultInfo.spsModuleFault &&
                !spsInfoPtr->spsFaultInfo.spsModuleFault) ||
            (cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsBatteryPresent && 
                cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsFaultInfo.spsBatteryFault &&
                !spsInfoPtr->spsFaultInfo.spsBatteryFault))
        {
            stateChange = TRUE;
            fbe_event_log_write(ESP_ERROR_SPS_FAULTED,
                                NULL, 0,
                                "%s %s %s",
                                &spsDeviceStr[0],
                                fbe_sps_mgmt_getSpsFaultString(&spsInfoPtr->spsFaultInfo),
                                fbe_sps_mgmt_getSpsFaultString(&cmiMsgPtr->sendSpsInfo[spsEnclIndex].spsFaultInfo));
        }

        // peer has sent us his SPS info, so save it
        sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS] = cmiMsgPtr->sendSpsInfo[spsEnclIndex];

        if (stateChange)
        {
            location.slot = (sps_mgmt->base_environment.spid == SP_A) ? SP_B : SP_A; // PEER SP 

            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, sending SPS event notification, sps %d\n",
                                  __FUNCTION__, location.slot);
            fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)sps_mgmt,
                                                               FBE_CLASS_ID_SPS_MGMT, 
                                                               FBE_DEVICE_TYPE_SPS,
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               &location);
        }
        fbe_sps_mgmt_analyzeSpsModel(sps_mgmt, &sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS]);
        
        // when peer SPS/BAT change to inserted or change to AVAILABLE, need upgrade checks
        if(fupCheckNeeded)
        {
            // check if we failed earlier due to bad peer env status
            sps_mgmt_restart_env_failed_fups(sps_mgmt, spsEnclIndex, FBE_LOCAL_SPS);
        }
    }


    // check if SPS Cache status changed
    fbe_sps_mgmt_checkSpsCacheStatus(sps_mgmt);

    // update Enclosure Fault LED
    status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                &location, 
                                                FBE_ENCL_FAULT_LED_SPS_FLT);
    // if DAE0 SPS status changed, the Cache Status may have changed which affects xPE Flt LED
    if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
        (location.bus == 0) && (location.enclosure == 0))
    {
        fbe_zero_memory(&xpeLocation, sizeof(fbe_device_physical_location_t));
        xpeLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        xpeLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        xpeLocation.slot = location.slot;
        status = fbe_sps_mgmt_update_encl_fault_led(sps_mgmt, 
                                                    &xpeLocation, 
                                                    FBE_ENCL_FAULT_LED_SPS_FLT);
    }

}
/******************************************************
 * end fbe_sps_mgmt_processPeerSpsInfo() 
 ******************************************************/

/*!**************************************************************
 * fbe_sps_mgmt_checkArrayCablingStatus()
 ****************************************************************
 * @brief
 * There has been a change to the peer sps status so retry the 
 * local side if needed. If the sps component upgrade failed 
 * with bad env status, initiate a new upgrade for it. 
 *
 * @param pSpsMgmt - sps_mgmt pointer
 *        spsEnclIndex -  index to the enclosure
 *        spsSideIndex -  index to the side (local or peer)
 *
 * @return fbe_sps_cabling_status_t - SPS Cabling status
 *
 * @author
 *  29-July-2010:  Created. Joe Perry
 *
 ****************************************************************/
static void sps_mgmt_restart_env_failed_fups(fbe_sps_mgmt_t *pSpsMgmt, 
                                             fbe_u32_t spsEnclIndex, 
                                             fbe_u32_t spsSideIndex)
{
    fbe_sps_cid_enum_t      cid;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_sps_fup_info_t      *pSpsFupInfo = NULL;
    fbe_device_physical_location_t fupLocation = {0};

    // check all local components- primary, secondary, battery
    // if they failed for env status we will restart the upgrade
    pSpsFupInfo = &(pSpsMgmt->arraySpsFupInfo->spsFupInfo[spsEnclIndex][spsSideIndex]);
    for(cid = FBE_SPS_COMPONENT_ID_PRIMARY; 
         cid < pSpsMgmt->arraySpsInfo->sps_info[spsEnclIndex][spsSideIndex].programmableCount; 
         cid ++)
    {
        if(cid >= FBE_SPS_COMPONENT_ID_MAX) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s ProgCountLimitExceeded spsindex:%d\n",
                                  __FUNCTION__,
                                  spsEnclIndex);
            /* Added the check to avoid the panic when the data got corrupted.*/
            break;
        }
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP: %s,[%d][%d].%d\n",
                              __FUNCTION__, spsEnclIndex, spsSideIndex, cid);

        if(pSpsFupInfo->fupPersistentInfo[cid].completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS)
        {
            status = fbe_sps_mgmt_convertSpsIndex(pSpsMgmt, spsEnclIndex, spsSideIndex, &fupLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "FUP: %s,failed to covertSpsIndex, spsEnclIndex %d, spsIndex %d.\n",
                                      __FUNCTION__, spsEnclIndex, spsSideIndex);
                continue;
            }
            fupLocation.componentId = cid;
            status = fbe_sps_mgmt_fup_initiate_upgrade(pSpsMgmt, 
                                                       FBE_DEVICE_TYPE_SPS, 
                                                       &fupLocation,
                                                       pSpsFupInfo->fupPersistentInfo[cid].forceFlags,
                                                       0);  //upgradeRetryCount
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "FUP:SPS(%d_%d_%d.%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                      fupLocation.bus,
                                      fupLocation.enclosure,
                                      fupLocation.slot,
                                      fupLocation.componentId,
                                      __FUNCTION__,
                                      status);
            }
        }
    }
    return;
}

/*!**************************************************************
 * fbe_sps_mgmt_checkArrayCablingStatus()
 ****************************************************************
 * @brief
 *  This function determines whether the SPS cabling is correct
 *  or not for the array
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return fbe_sps_cabling_status_t - SPS Cabling status
 *
 * @author
 *  29-July-2010:  Created. Joe Perry
 *
 ****************************************************************/
fbe_sps_cabling_status_t 
fbe_sps_mgmt_checkArrayCablingStatus(fbe_sps_mgmt_t *sps_mgmt, fbe_bool_t testComplete)
{
    fbe_sps_cabling_status_t        newCablingStatus = FBE_SPS_CABLING_UNKNOWN;
    fbe_u8_t                        localSpsIndex, peerSpsIndex;
    fbe_u32_t                       enclIndex, psIndex;
    fbe_u32_t                       acFailMatchCount = 0;
    fbe_u32_t                       enclAcFailExpectedMatchCount = 0;
    fbe_u32_t                       acFailExpectedMatchCount = 0;
    fbe_u32_t                       acFailOppositeMatchCount = 0;
    fbe_u32_t                       acFailMismatchCount = 0;
    fbe_u32_t                       peCablingProblem = FALSE;
    fbe_u32_t                       dae0CablingProblem = FALSE;
    fbe_u32_t                       enclGoodPsCount = 0;

    /*
     * Verify different PS's based on which SP
     */
    if (sps_mgmt->base_environment.spid == SP_A)
    {
        localSpsIndex = SP_A;
        peerSpsIndex = SP_B;
    }
    else
    {
        localSpsIndex = SP_B;
        peerSpsIndex = SP_A;
    }

    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        enclAcFailExpectedMatchCount = enclGoodPsCount = 0;
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "checkCabling, %d %d, exp %d, det %d, opp %d\n",
                                  enclIndex, psIndex,
                                  sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected,
                                  sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailDetected,
                                  sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailOpposite);
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "checkCabling, %d %d, spid %d, assocSp %d, assocSps %d, psIns %d\n",
                                  enclIndex, psIndex,
                                  sps_mgmt->base_environment.spid,
                                  sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].associatedSp,
                                  sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].associatedSps,
                                  sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psInserted);
            if (sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected)
            {
                acFailExpectedMatchCount++;
                enclAcFailExpectedMatchCount++;
            }
            if (sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailDetected && 
                sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailOpposite)
            {
                acFailOppositeMatchCount++;
            }
            if (sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected &&
                sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailDetected)
            {
                acFailMatchCount++;
                // special processing based on enclosure type:
                //   -xPE has multiple PS's & we must tolerate missing PS for SPS testing
                //   -DAE0 only has one PS per side, so cannot tolerate missing PS for SPS testing
                if ((enclIndex == FBE_SPS_MGMT_DAE0) ||
                    ((enclIndex == FBE_SPS_MGMT_PE) && 
                     (sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].associatedSp == sps_mgmt->base_environment.spid)))
                {
                    enclGoodPsCount++;
                }
            }
            else if (sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected &&
                testComplete && !sps_mgmt->arraySpsInfo->ps_info[enclIndex][psIndex].psInserted)
            {
                acFailMatchCount++;
            }
            else if ((!sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailDetected &&
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected) ||
                (sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailDetected &&
                    !sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected))
            {
                acFailMismatchCount++;
                if (enclIndex == FBE_SPS_MGMT_PE)
                {
                    peCablingProblem = TRUE;
                }
                else
                {
                    dae0CablingProblem = TRUE;
                }
            }
        }   // end of psIndex

        // we can ignore a single missing PS if enclosure has multiple PS's, but it must have one good PS present
        if ((enclAcFailExpectedMatchCount > 0) && (enclGoodPsCount == 0))
        {
            acFailMatchCount--;
            if (enclIndex == FBE_SPS_MGMT_PE)
            {
                peCablingProblem = TRUE;
            }
            else
            {
                dae0CablingProblem = TRUE;
            }
        }

    }   // end of enclIndex

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s(SPS_TEST), ExpCnt %d, MatchCnt %d, OppCnt %d, MismatchCnt %d\n",
                          __FUNCTION__,
                          acFailExpectedMatchCount,
                          acFailMatchCount,
                          acFailOppositeMatchCount,
                          acFailMismatchCount);

    if (acFailMatchCount == acFailExpectedMatchCount)
    {
        newCablingStatus = FBE_SPS_CABLING_VALID;
    }
    else if (acFailOppositeMatchCount == acFailExpectedMatchCount)
    {
        // SPS's do not have a fixed slot like other components (all depends on which SPS or LCC they are cabled to),
        // so we cannot determine for sure if the Serial or Power cables are crossed.  Mark so user will check all cables.
        newCablingStatus = FBE_SPS_CABLING_INVALID_MULTI;
    }
    else if (peCablingProblem && !dae0CablingProblem)
    {
        newCablingStatus = FBE_SPS_CABLING_INVALID_PE;
    }
    else if (!peCablingProblem && dae0CablingProblem)
    {
        newCablingStatus = FBE_SPS_CABLING_INVALID_DAE0;
    }
    else
    {
        newCablingStatus = FBE_SPS_CABLING_INVALID_MULTI;
    }

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s(SPS_TEST), newCablingStatus %s\n",
                          __FUNCTION__,
                          fbe_sps_mgmt_getSpsCablingStatusString(newCablingStatus));

    return newCablingStatus;

}
/******************************************************
 * end fbe_sps_mgmt_checkArrayCablingStatus() 
 ******************************************************/


const char *fbe_sps_mgmt_getSpsMsgTypeString (fbe_u32_t MsgType)
{
    const char *spsMsgString = "Undefined";

    switch (MsgType)
    {
    case FBE_BASE_ENV_CMI_MSG_REQ_SPS_INFO:
        spsMsgString = "ReqSpsInfo" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO:
        spsMsgString = "SendSpsInfo" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_TEST_REQ_PERMISSION:
        spsMsgString = "ReqPermission" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_TEST_REQ_ACK:
        spsMsgString = "ReqAck" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_TEST_REQ_NACK:
        spsMsgString = "ReqNack" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_TEST_COMPLETED:
        spsMsgString = "TestCompleted" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:    // request permission for fup
        spsMsgString = "FupPeerPermissionRequest" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:      // grant permission to fup
        spsMsgString = "FupPeerPermissionGrant" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
        spsMsgString = "FupPeerPermissionDeny" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_PEER_BBU_TEST_STATUS:
        spsMsgString = "PeerBbuTestStatus" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_REQUEST:
        spsMsgString = "PersistentReadReq" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_COMPLETE:
        spsMsgString = "PersistentReadComplete" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
        spsMsgString = "CacheStatusUpdate" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_PEER_DISK_BATTERY_BACKED_SET:
        spsMsgString = "PeerDiskBatteryBackedSet" ;
        break;
    case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
        spsMsgString = "PeerSpAlive" ;
        break;
    default:
        break;
    }

    return spsMsgString;

}   // end of fbe_sps_mgmt_getSpsMsgTypeString

const char *fbe_sps_mgmt_getCmiEventTypeString (fbe_cmi_event_t cmiEventType)
{
    const char *cmiEventTypeString = "Undefined";

    switch (cmiEventType)
    {
    case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
        cmiEventTypeString = "MsgTransmitted" ;
        break;
    case FBE_CMI_EVENT_MESSAGE_RECEIVED:
        cmiEventTypeString = "MsgReceived" ;
        break;
    case FBE_CMI_EVENT_CLOSE_COMPLETED:
        cmiEventTypeString = "CloseCompleted" ;
        break;
    case FBE_CMI_EVENT_SP_CONTACT_LOST:
        cmiEventTypeString = "SpContactLost" ;
        break;
    case FBE_CMI_EVENT_DMA_ADDRESSES_NEEDED:
        cmiEventTypeString = "DmaAddressNeeded" ;
        break;
    case FBE_CMI_EVENT_PEER_NOT_PRESENT:
        cmiEventTypeString = "PeerNotPresent" ;
        break;
    case FBE_CMI_EVENT_FATAL_ERROR:
        cmiEventTypeString = "FatalError" ;
        break;
    case FBE_CMI_EVENT_INVALID:
        cmiEventTypeString = "Invalid" ;
        break;
	case FBE_CMI_EVENT_PEER_BUSY:
		cmiEventTypeString = "PeerBusy" ;
        break;
    default:
        break;
    }

    return cmiEventTypeString;

}   // end of fbe_sps_mgmt_getCmiEventTypeString

const char *fbe_sps_mgmt_getSpsTestStateString (fbe_sps_testing_state_t spsTestState)
{
    const char *spsTestStateString = "Undefined";

    switch (spsTestState)
    {
    case FBE_SPS_NO_TESTING_IN_PROGRESS:
        spsTestStateString = "NoTesting" ;
        break;
    case FBE_SPS_LOCAL_SPS_TESTING:
        spsTestStateString = "LocalSpsTesting" ;
        break;
    case FBE_SPS_PEER_SPS_TESTING:
        spsTestStateString = "PeerSpsTesting" ;
        break;
    case FBE_SPS_WAIT_FOR_PEER_PERMISSION:
        spsTestStateString = "WaitPeerPerm" ;
        break;
    case FBE_SPS_WAIT_FOR_PEER_TEST:
        spsTestStateString = "WaitPeerTest" ;
        break;
    case FBE_SPS_WAIT_FOR_STABLE_ARRAY:
        spsTestStateString = "WaitStableArray" ;
        break;
    default:
        break;
    }

    return spsTestStateString;
}

/*!**************************************************************
 * fbe_sps_mgmt_determineSpsInputPower()
 ****************************************************************
 * @brief
 *  This function takes the SPS model & status and determines
 *  the current InputPower value & status.
 *
 * @param spsStatus - status of SPS
 * @param spsModel - model of SPS
 * @param inputPowerSample - pointer to an InputPower sample
 *
 * @return none
 *
 * @author
 *  08-Dec-2010:  Created. Joe Perry
 *
 ****************************************************************/
void fbe_sps_mgmt_determineSpsInputPower(fbe_bool_t spsInserted,
                                         SPS_STATE spsStatus,
                                         SPS_TYPE spsModel,
                                         fbe_eir_input_power_sample_t *inputPowerSample)
{

    if (!spsInserted)
    {
        inputPowerSample->inputPower = 0;
        inputPowerSample->status = FBE_ENERGY_STAT_NOT_PRESENT;
    }
    else
    {
        switch (spsStatus)
        {
        case SPS_STATE_AVAILABLE:
        case SPS_STATE_CHARGING:
        case SPS_STATE_ON_BATTERY:
        case SPS_STATE_TESTING:
            // get SPS Model & get InputPower value based on that
            if ((spsModel == SPS_TYPE_1_2_KW) || (spsModel == SPS_TYPE_2_2_KW) || 
                (spsModel == SPS_TYPE_LI_ION_2_2_KW))
            {
                inputPowerSample->inputPower = 
                    fbe_sps_mgmt_inputPowerTable[spsModel][spsStatus];
                inputPowerSample->status = FBE_ENERGY_STAT_VALID;
            }
            else
            {
                inputPowerSample->inputPower = 0;
                inputPowerSample->status = FBE_ENERGY_STAT_INVALID;
            }
            break;
    
        case SPS_STATE_FAULTED:
            // sps is faulted, set status appropriately
            inputPowerSample->inputPower = 0;
            inputPowerSample->status = FBE_ENERGY_STAT_FAILED;
            break;
    
        default:
            inputPowerSample->inputPower = 0;
            inputPowerSample->status = FBE_ENERGY_STAT_INVALID;
            break;
        }
    }

}   // end of fbe_sps_mgmt_determineSpsInputPower


/*!**************************************************************
 * fbe_sps_mgmt_checkSpsCacheStatus()
 ****************************************************************
 * @brief
 *  This function checks the SPS's status to determine if it is
 *  OK to allow caching (send notification if changes).
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return none
 *
 * @author
 *  23-Mar-2011:  Created. Joe Perry
 *
 ****************************************************************/
void fbe_sps_mgmt_checkSpsCacheStatus(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_common_cache_status_t       spsCacheStatus = FBE_CACHE_STATUS_UNINITIALIZE;
    fbe_device_physical_location_t  location;
    fbe_common_cache_status_t       peerSpsCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_status_t status = FBE_STATUS_OK;

    if (sps_mgmt->total_sps_count > 0)
    {
        spsCacheStatus = fbe_sps_mgmt_spsCheckSpsCacheStatus(sps_mgmt);
    }
    else if (sps_mgmt->total_bob_count > 0)
    {
        spsCacheStatus = fbe_sps_mgmt_bobCheckSpsCacheStatus(sps_mgmt);
    }

    /*
     * Check for status change & send notification if needed
     */
    if (spsCacheStatus != sps_mgmt->arraySpsInfo->spsCacheStatus)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, spsCacheStatus changes from %d, to %d\n",
                              __FUNCTION__,
                              sps_mgmt->arraySpsInfo->spsCacheStatus,
                              spsCacheStatus);

        /* Log the event for cache status change */
        fbe_event_log_write(ESP_INFO_CACHE_STATUS_CHANGED,
                            NULL, 0,
                            "%s %s %s",
                            "SPS_MGMT",
                            fbe_base_env_decode_cache_status(sps_mgmt->arraySpsInfo->spsCacheStatus),
                            fbe_base_env_decode_cache_status(spsCacheStatus));

        status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)sps_mgmt,
                                                             &peerSpsCacheStatus);
        /* Send CMI to peer */
        fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)sps_mgmt, 
                                                              spsCacheStatus,
                                                             ((peerSpsCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE)? FALSE : TRUE));

        sps_mgmt->arraySpsInfo->spsCacheStatus = spsCacheStatus;
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));     // location not relevant
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)sps_mgmt, 
                                                           FBE_CLASS_ID_SPS_MGMT, 
                                                           FBE_DEVICE_TYPE_SP_ENVIRON_STATE, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &location);    
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE sent\n",
                              __FUNCTION__);
    }

}   // end of fbe_sps_mgmt_checkSpsCacheStatus

/*!**************************************************************
 * fbe_sps_mgmt_spsCheckSpsCacheStatus()
 ****************************************************************
 * @brief
 *  This function checks the SPS's status to determine if it is
 *  OK to allow caching (send notification if changes).
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return none
 *
 * @author
 *  24-Apr-2012:  Created. Joe Perry
 *
 ****************************************************************/
fbe_common_cache_status_t fbe_sps_mgmt_spsCheckSpsCacheStatus(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_common_cache_status_t       spsCacheStatus;
    fbe_common_cache_status_t       previousSpsCacheStatus;
    fbe_status_t                    status;
    fbe_u32_t                       enclIndex, spsIndex;
    fbe_u32_t                       spsCount = 0, spsGoodCount = 0, spsBatteryOnlineCount = 0;
    fbe_bool_t                      enclHasGoodSps, enclSpsOnBattery;
    fbe_bool_t                      spsPresentNeeded = FALSE;
    fbe_sps_action_type_t           spsPresentAction;
    fbe_u32_t                       spsPerEnclosure;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, spsCacheStatus %d, spsCabling PE %d, DAE0 %d\n",
                          __FUNCTION__,
                          sps_mgmt->arraySpsInfo->spsCacheStatus,
                          sps_mgmt->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsCablingStatus,
                          sps_mgmt->arraySpsInfo->sps_info[FBE_SPS_MGMT_DAE0][FBE_LOCAL_SPS].spsCablingStatus);
    if (sps_mgmt->total_sps_count > FBE_SPS_MGMT_TWO_SPS_COUNT)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, spsStatus PE %d, DAE0 %d\n",
                              __FUNCTION__,
                              sps_mgmt->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsStatus,
                              sps_mgmt->arraySpsInfo->sps_info[FBE_SPS_MGMT_DAE0][FBE_LOCAL_SPS].spsStatus);
    }
 
    /*
     * Calculate current status (must check both on same side)
     */
    spsPerEnclosure = sps_mgmt->total_sps_count / sps_mgmt->encls_with_sps;
    for (enclIndex = 0; enclIndex < sps_mgmt->encls_with_sps; enclIndex++)
    {
        for (spsIndex = 0; spsIndex < spsPerEnclosure; spsIndex++)
        {
            spsCount++;
            if ((sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsStatus == 
                  SPS_STATE_AVAILABLE) &&
                (sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsCablingStatus == 
                 FBE_SPS_CABLING_VALID))
            {
                spsGoodCount++;
            }
            else if (sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsStatus == SPS_STATE_ON_BATTERY)
            {
                spsBatteryOnlineCount++;
            }
        }
    }

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, spsCnt %d, spsGoodCnt %d, spsBattOnlCnt %d\n",
                          __FUNCTION__,
                          spsCount, spsGoodCount, spsBatteryOnlineCount);

    // check if all SPS's are good
    if (spsGoodCount == spsCount)
    {
        spsCacheStatus = FBE_CACHE_STATUS_OK;
    }
    // check if all SPS's are BatteryOnline
    else if ((spsBatteryOnlineCount > 0) && 
             (spsBatteryOnlineCount == spsCount))
    {
        spsCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
    }
    // check if no SPS's are available
    else if (spsGoodCount == 0)
    {
        if (spsBatteryOnlineCount == 0)
        {
            spsCacheStatus = FBE_CACHE_STATUS_FAILED;
        }
        else
        {
            spsCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
        }
    }
    // check for other, more complex conditions
    else
    {
        spsCacheStatus = FBE_CACHE_STATUS_DEGRADED;     // set to Degraded for now
        // we need at least one good SPS per enclosure to keep caching
        for (enclIndex = 0; enclIndex < sps_mgmt->encls_with_sps; enclIndex++)
        {
            enclHasGoodSps = FALSE;
            enclSpsOnBattery = FALSE;
            for (spsIndex = 0; spsIndex < spsPerEnclosure; spsIndex++)
            {
                if ((sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsStatus == 
                      SPS_STATE_AVAILABLE) &&
                    (sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsCablingStatus == 
                     FBE_SPS_CABLING_VALID))
                {
                    enclHasGoodSps = TRUE;
                }
                if (sps_mgmt->arraySpsInfo->sps_info[enclIndex][spsIndex].spsStatus == 
                      SPS_STATE_ON_BATTERY)
                {
                    enclSpsOnBattery = TRUE;
                }
            }
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d, enclHasGoodSps %d, enclSpsOnBattery %d\n",
                                  __FUNCTION__, enclIndex, enclHasGoodSps, enclSpsOnBattery);
            // if enclosure does not have good SPS's, determine state based off of BatteryOnline
            if (!enclHasGoodSps)
            {
                if (enclSpsOnBattery)
                {
                    spsCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
                }
                else
                {
                    spsCacheStatus = FBE_CACHE_STATUS_FAILED;
                }
                break;
            }
        } // end of for enclIndex
    }

    /*
     * Notify BMC if any changes in SPS available for backup status
     */
    previousSpsCacheStatus = sps_mgmt->arraySpsInfo->spsCacheStatus;
    if ((spsCacheStatus == FBE_CACHE_STATUS_FAILED) &&
        (previousSpsCacheStatus != FBE_CACHE_STATUS_FAILED))
    {
        // lost SPS backup, clear SPS Present
        spsPresentNeeded = TRUE;
        spsPresentAction = FBE_SPS_ACTION_CLEAR_SPS_PRESENT;
    }
    if ((spsCacheStatus != FBE_CACHE_STATUS_FAILED) &&
        (previousSpsCacheStatus == FBE_CACHE_STATUS_FAILED))
    {
        // gained SPS backup, set SPS Present
        spsPresentNeeded = TRUE;
        spsPresentAction = FBE_SPS_ACTION_SET_SPS_PRESENT;
    }
    // Do not send SpsPresent comamnd if simulationg SPS
    if (spsPresentNeeded && !sps_mgmt->arraySpsInfo->simulateSpsInfo)
    {
        status = fbe_sps_mgmt_sendSpsCommand(sps_mgmt, spsPresentAction, FBE_SPS_MGMT_PE);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, SPS Present %s\n",
                                  __FUNCTION__,
                                  ((spsPresentAction == FBE_SPS_ACTION_SET_SPS_PRESENT) ? "set" : "clear"));
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, SPS Present %s failed, status %d\n",
                                  __FUNCTION__, 
                                  ((spsPresentAction == FBE_SPS_ACTION_SET_SPS_PRESENT) ? "set" : "clear"),
                                  status);
        }
    }

    return spsCacheStatus;

}   // end of fbe_sps_mgmt_spsCheckSpsCacheStatus

/*!**************************************************************
 * fbe_sps_mgmt_bobCheckSpsCacheStatus()
 ****************************************************************
 * @brief
 *  This function checks the BoB's status to determine if it is
 *  OK to allow caching (send notification if changes).
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return none
 *
 * @author
 *  24-Apr-2012:  Created. Joe Perry
 *
 ****************************************************************/
fbe_common_cache_status_t fbe_sps_mgmt_bobCheckSpsCacheStatus(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_common_cache_status_t       spsCacheStatus;
    fbe_bool_t                      spaBobGood = FALSE, spbBobGood = FALSE;
    fbe_bool_t                      spaBbuOnBattery = FALSE, spbBbuOnBattery = FALSE;
    fbe_bool_t                      diskBatteryBackedSet = FALSE, localDiskBatteryBackedSet = FALSE;
    fbe_status_t                    status;
    fbe_topology_control_get_enclosure_by_location_t  enclosure_object_ids={0};
    fbe_lcc_info_t                  spaBmLccInfo ={0};
    fbe_lcc_info_t                  spbBmLccInfo ={0};
    fbe_board_mgmt_misc_info_t      miscInfo;
    fbe_bool_t                      powerEcbFaultSpa, powerEcbFaultSpb;
    fbe_bool_t                      spaBbuOk, spbBbuOk;
    fbe_u32_t                       bbuIndex;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, bbuA Pres %d, State %s, bbuB Pres %d, State %s\n",
                          __FUNCTION__,
                          sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].batteryInserted,
                          fbe_sps_mgmt_getBobStateString(sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].batteryState),
                          sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].batteryInserted,
                          fbe_sps_mgmt_getBobStateString(sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].batteryState));

    status = fbe_api_get_enclosure_object_ids_array_by_location(0,0, &enclosure_object_ids);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Fail to get encl 0_0 object id. Return status %d\n",
                                  __FUNCTION__, status);
        return FBE_CACHE_STATUS_FAILED;
    }

    // get status of Drives that are Battery Backed
    status = fbe_api_enclosure_get_disk_battery_backed_status(enclosure_object_ids.enclosure_object_id, &localDiskBatteryBackedSet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Fail to get disk battery backed status. Return status %d\n",
                                  __FUNCTION__, status);
        return FBE_CACHE_STATUS_FAILED;
    }

    /* 
       *Fix AR563856. 
       *  we determine diskBatteryBackedSet attribute by both side, it's ok to enable cache if batteryBacked bit is set in either side.
       */
    sps_mgmt->arrayBobInfo->localDiskBatteryBackedSet = localDiskBatteryBackedSet;
    status = fbe_sps_mgmt_sendCmiMessage(sps_mgmt, FBE_BASE_ENV_CMI_MSG_PEER_DISK_BATTERY_BACKED_SET);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending diskBatteryBackedSet info to peer, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    diskBatteryBackedSet = sps_mgmt->arrayBobInfo->localDiskBatteryBackedSet || (fbe_cmi_is_peer_alive() && (sps_mgmt->arrayBobInfo->peerDiskBatteryBackedSet));

    // get status of Drive ECB Faults (BBU can't provide power to drives)
    status = fbe_sps_mgmt_get_bm_lcc_info(sps_mgmt, SP_A, &spaBmLccInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Fail to get SPA BM lcc info, status 0x%X.\n",
                                  __FUNCTION__, status);

        return FBE_CACHE_STATUS_FAILED;
    }
    status = fbe_sps_mgmt_get_bm_lcc_info(sps_mgmt, SP_B, &spbBmLccInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Fail to get SPB BM lcc info, status 0x%X.\n",
                                  __FUNCTION__, status);

        return FBE_CACHE_STATUS_FAILED;
    }

    // get status of Power ECB Fault.  Only the Local is relevant (BBU can't provide power to local SP)
    status = fbe_api_board_get_misc_info(sps_mgmt->object_id, &miscInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Fail to get misc info, status 0x%X.\n",
                                  __FUNCTION__, status);

        return FBE_CACHE_STATUS_FAILED;
    }
    else
    {
        if (sps_mgmt->base_environment.spid == SP_A)
        {
            powerEcbFaultSpa = miscInfo.localPowerECBFault;
            powerEcbFaultSpb = FALSE;
        }
        else
        {
            powerEcbFaultSpb = miscInfo.localPowerECBFault;
            powerEcbFaultSpa = FALSE;
        }
    }

    /*
     * check all the BBU's per side (all need to be in good state to hold up an SP)
     */
    spaBbuOk = spbBbuOk = TRUE;
    spaBbuOnBattery = spbBbuOnBattery = FALSE;
    for (bbuIndex = 0; bbuIndex < sps_mgmt->total_bob_count; bbuIndex++)
    {
        // check for BBU fault conditions
        if (!sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryInserted ||
            !sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryReady ||
            !sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryEnabled ||
            (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFaults != FBE_BATTERY_FAULT_NO_FAULT) ||
            (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD))
        {
            if (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].associatedSp == SP_A)
            {
                spaBbuOk = FALSE;
            }
            else
            {
                spbBbuOk = FALSE;
            }
        }
        // check if BBU is OnBattery
        if (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryInserted &&
            (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryState == FBE_BATTERY_STATUS_ON_BATTERY))
        {
            if (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].associatedSp == SP_A)
            {
                spaBbuOnBattery = TRUE;
            }
            else
            {
                spbBbuOnBattery = TRUE;
            }
        }
    }

    /*
     * Calculate current status (must check both on same side)
     * Added the check of ecbFault. 
     * ecbFault is set which means the drives get power only from the peer side PS.
     */
    // check SPA BOB
    if (spaBbuOk && !spaBmLccInfo.ecbFault && !powerEcbFaultSpa)
    {
        spaBobGood = TRUE;
    }
    // check SPB BOB
    if (spbBbuOk && !spbBmLccInfo.ecbFault && !powerEcbFaultSpb)
    {
        spbBobGood = TRUE;
    }

    // if peer SP is not available, mark that BoB as faulted
    if (!fbe_cmi_is_peer_alive())
    {
        if (sps_mgmt->base_environment.spid == SP_A)
        {
            spbBobGood = FALSE;
            if (spaBbuOnBattery)
            {
                spbBbuOnBattery = TRUE;
            }
        }
        else
        {
            spaBobGood = FALSE;
            if (spbBbuOnBattery)
            {
                spaBbuOnBattery = TRUE;
            }
        }
    }

    // BOB's OnBattery or only good BoB OnBattery
    if ((spaBbuOnBattery && spbBbuOnBattery) ||
        (spaBbuOnBattery && !spbBobGood) ||
        (spbBbuOnBattery && !spaBobGood))
    {
        spsCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
    }
    else if ((spaBobGood && spbBobGood && diskBatteryBackedSet) ||
            (sps_mgmt->base_environment.isSingleSpSystem == TRUE && spaBobGood && diskBatteryBackedSet))
    {
        // all BOBs are available
        spsCacheStatus = FBE_CACHE_STATUS_OK;
    }
    else if (!diskBatteryBackedSet)
    {
        spsCacheStatus = FBE_CACHE_STATUS_FAILED;
    }
    else if (!spaBobGood && !spbBobGood)
    {
        // no BOBs are available, check if due to Environmental Interface faliures
        if ((sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD) &&
            (sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD))
        {
            spsCacheStatus = FBE_CACHE_STATUS_FAILED_ENV_FLT;
        }
        else
        {
            spsCacheStatus = FBE_CACHE_STATUS_FAILED;
        }
    }
    else
    {
        // some BOBs are available
        spsCacheStatus = FBE_CACHE_STATUS_DEGRADED;
    }

    if (spsCacheStatus != sps_mgmt->arraySpsInfo->spsCacheStatus)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, spsCacheStatus old %d, new %d\n", 
                              __FUNCTION__,
                              sps_mgmt->arraySpsInfo->spsCacheStatus,
                              spsCacheStatus);
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, peerAlive %d, diskBatBkd %d.\n",
                              __FUNCTION__, 
                              fbe_cmi_is_peer_alive(),
                              diskBatteryBackedSet);
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, DrEcbFlt spa %d, spb %d, PwrEcbFlt spa %d, spb %d.\n",
                              __FUNCTION__, 
                              spaBmLccInfo.ecbFault,
                              spbBmLccInfo.ecbFault,
                              powerEcbFaultSpa,
                              powerEcbFaultSpb);
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, BBUA enable %d, state %s, flt %s, envIntStatus %d.\n",
                              __FUNCTION__, 
                              sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].batteryEnabled, 
                              fbe_sps_mgmt_getBobStateString(sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].batteryState),
                              fbe_sps_mgmt_getBobFaultString(sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].batteryFaults),
                              sps_mgmt->arrayBobInfo->bob_info[FBE_SPA_BOB].envInterfaceStatus);
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, BBUB enable %d, state %s, flt %s, envIntStatus %d.\n",
                              __FUNCTION__, 
                              sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].batteryEnabled, 
                              fbe_sps_mgmt_getBobStateString(sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].batteryState),
                              fbe_sps_mgmt_getBobFaultString(sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].batteryFaults),
                              sps_mgmt->arrayBobInfo->bob_info[FBE_SPB_BOB].envInterfaceStatus);
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "  spaBobGood %d, spbBobGood %d, spaBbuOnBattery %d, spbBbuOnBattery %d\n",
                              spaBobGood, spbBobGood, spaBbuOnBattery, spbBbuOnBattery);
    }

    return spsCacheStatus;

}   // end of fbe_sps_mgmt_bobCheckSpsCacheStatus


/*!**************************************************************
 * fbe_sps_mgmt_get_local_bm_lcc_info()
 ****************************************************************
 * @brief
 *  This function get lcc info for bem from physical package.
 *  Jetfire BEM is also treated as lcc in physical package enclosure object.
 *
 * @param pSpsMgmt - object handle
 * @para sp - the BM side (SPA or SPB)
 * @param pLccInfo - pointer to save Lcc info
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  09-Jan-2013 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_get_bm_lcc_info(fbe_sps_mgmt_t * pSpsMgmt, 
                                   SP_ID sp,
                                   fbe_lcc_info_t * pLccInfo)
{
    fbe_device_physical_location_t          location = {0};
    fbe_status_t                            status = FBE_STATUS_OK;

    location.bus = 0;
    location.enclosure = 0;
    location.slot = (sp == SP_A) ? 0 : 1;

    status = fbe_api_enclosure_get_lcc_info(&location, pLccInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting BM LCC info for %s, status: 0x%X.\n",
                              __FUNCTION__,
                              (sp == SP_A) ? "SPA" : "SPB",
                              status);
        return status;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_sps_mgmt_checkSpsCacheStatus()
 ****************************************************************
 * @brief
 *  This function checks the SPS's status to determine if it is
 *  OK to allow caching (send notification if changes).
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return none
 *
 * @author
 *  23-Mar-2011:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_sendSpsCommand(fbe_sps_mgmt_t *sps_mgmt,
                                         fbe_sps_action_type_t spsCommand,
                                         fbe_sps_ps_encl_num_t spsToSendTo)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_s32_t           spsEnclIndex;
    fbe_u32_t           retryIndex, retryCount = 1;

    // only retry SHUTDOWN command (other commands already have retry conditions)
    if (spsCommand == FBE_SPS_ACTION_SHUTDOWN)
    {
        retryCount = 3;
    }

    // Send commands to DAE0 first (if shutting down, want the xPE to go down last)
    for (spsEnclIndex = (sps_mgmt->encls_with_sps-1); spsEnclIndex >= 0; spsEnclIndex--)
    {
        if ((spsToSendTo == FBE_SPS_MGMT_ALL) || (spsToSendTo == spsEnclIndex))
        {
            for (retryIndex = 0; retryIndex < retryCount; retryIndex++)
            {
                // send message to Phy Pkg
                status = fbe_api_sps_send_sps_command(sps_mgmt->arraySpsInfo->spsObjectId[spsEnclIndex], 
                                                      spsCommand);
                if (status == FBE_STATUS_OK) 
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, %s cmd %s sent, retryIndex %d\n",
                                          __FUNCTION__, 
                                          sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString,
                                          fbe_sps_mgmt_getSpsActionTypeString(spsCommand),
                                          retryIndex);
                    break;
                }
                else
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, %s cmd %s failed, retryIndex %d, status %d\n",
                                          __FUNCTION__, 
                                          sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString,
                                          fbe_sps_mgmt_getSpsActionTypeString(spsCommand),
                                          retryIndex,
                                          status);
                    if ((retryCount > 1) && (retryIndex < (retryCount-1)))
                    {
                        fbe_api_sleep(250);
                    }
                }
            }   // for retries
        }
   
    }
    return status;

}   // end of fbe_sps_mgmt_sendSpsCommand


/*!**************************************************************
 * fbe_sps_mgmt_sendSpsMfgInfoRequest()
 ****************************************************************
 * @brief
 *  This function sends a request for SPS Mfg Info to Phy Pkg.
 *  It will also issue retries if BUSY status returned.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return none
 *
 * @author
 *  28-Mar-2013:  Created. Joe Perry
 *
 ****************************************************************/
static
fbe_status_t fbe_sps_mgmt_sendSpsMfgInfoRequest(fbe_sps_mgmt_t *sps_mgmt,
                                                fbe_sps_ps_encl_num_t spsLocation,
                                                fbe_sps_get_sps_manuf_info_t *spsManufInfo)
{
    fbe_status_t        status = FBE_STATUS_OK;

    // get Manufacturing Info in either SPS Module or Battery inserted
    fbe_zero_memory(spsManufInfo, sizeof(fbe_sps_manuf_info_t));
    status = fbe_api_sps_get_sps_manuf_info(sps_mgmt->arraySpsInfo->spsObjectId[spsLocation], 
                                            spsManufInfo);
    if (status != FBE_STATUS_OK) 
    {
        sps_mgmt->arraySpsInfo->mfgInfoNeeded[spsLocation] = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, %s, Manuf Info failed, status: 0x%X\n",
                              __FUNCTION__, 
                              sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsNameString, 
                              status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, Manuf Info successfully read\n",
                              __FUNCTION__, 
                              sps_mgmt->arraySpsInfo->sps_info[spsLocation][FBE_LOCAL_SPS].spsNameString);
    }
  
    return status;

}   // end of fbe_sps_mgmt_sendSpsMfgInfoRequest

/*!**************************************************************
 * fbe_sps_mgmt_determineAcDcArrayType()
 ****************************************************************
 * @brief
 *  This function checks the PE PS's to determine if this array
 *  is an AC or DC powered array.
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return none
 *
 * @author
 *  10-Feb-2012:  Created. Joe Perry
 *
 ****************************************************************/
void fbe_sps_mgmt_determineAcDcArrayType(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_object_id_t                             object_id;
    fbe_u32_t                                   psIndex, psCount;
    fbe_u32_t                                   psPresentCount = 0;
    fbe_u32_t                                   psDcCount = 0;
    fbe_status_t                                status;
    fbe_device_physical_location_t              location;
    fbe_power_supply_info_t                     psStatus;

    if (sps_mgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE)
    {
        location.bus = 0;
        location.enclosure = 0;
    }
    else
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    // Get the Board Object ID 
    status = fbe_api_get_board_object_id(&object_id);
    if (status != FBE_STATUS_OK) 
    {
		fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting PE Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);
	}
    else
    {
        status = fbe_api_power_supply_get_ps_count(object_id, &psCount);
        if (status == FBE_STATUS_OK)
        {
            for (psIndex = 0; psIndex < psCount; psIndex++)
            {
                fbe_set_memory(&psStatus, 0, sizeof(fbe_power_supply_info_t));
                psStatus.slotNumOnEncl = psIndex;
                status = fbe_api_power_supply_get_power_supply_info(object_id, &psStatus);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting PS Status, psIndex %d, status: 0x%X\n",
                                          __FUNCTION__, psIndex, status);
                    return;
                }

                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, ps %d, inserted %d, acDc %d\n",
                                      __FUNCTION__, 
                                      psIndex,
                                      psStatus.bInserted,
                                      psStatus.ACDCInput);

                if (psStatus.bInserted)
                {
                    psPresentCount++;
                    if (psStatus.ACDCInput == DC_INPUT)
                    {
                        psDcCount++;
                    }
                }
    
            }
        }

        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, psPresentCount %d, psDcCount %d\n",
                              __FUNCTION__, psPresentCount, psDcCount);

        if ((psPresentCount > 0) && (psPresentCount == psDcCount))
        {
            // this is a DC array, so simulate SPS
            sps_mgmt->arraySpsInfo->simulateSpsInfo = TRUE;
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, DC Array detected (simulate SPS)\n",
                                  __FUNCTION__);
        }
    }

}   // end of fbe_sps_mgmt_determineAcDcArrayType


/*!**************************************************************
 * fbe_sps_mgmt_determineExpectedPsAcFails()
 ****************************************************************
 * @brief
 *  This function checks the enclosure types & sets which PS's
 *  expect to see AC_FAIL's during SPS testing (it also sets the
 *  PS's that are the opposite of expected for case where SPS
 *  Serial cables are backwards).  Here are the
 *  basic configurations:
 *      -DPE: has two PS's
 *      -xPE with Viper/Derringer: xPE has 4 PS's, DAE0 has 2 PS's
 *      -xPE with Voyager: xPE has 4 PS's, DAE0 has 4 PS's
 *
 * @param sps_mgmt - pointer to SPS Mgmt info.
 *
 * @return none
 *
 * @author
 *  14-Mar-2012:  Created. Joe Perry
 *
 ****************************************************************/
void fbe_sps_mgmt_determineExpectedPsAcFails(fbe_sps_mgmt_t *sps_mgmt,
                                             fbe_sps_ps_encl_num_t spsToTest)
{
    fbe_status_t                    status;
    fbe_board_mgmt_platform_info_t  platformInfo;
    fbe_u32_t                       enclIndex, psIndex;
    SP_ID                           peerSpid;

    // need to know platform info (CPU Moduled)
    status = fbe_api_board_get_platform_info(sps_mgmt->object_id, &platformInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get platform info, status: 0x%X",
                              __FUNCTION__, status);
        return;
    }
    peerSpid = (sps_mgmt->base_environment.spid == SP_A) ? SP_B : SP_A;

    // clear Expected AC_FAIL's
    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected = FALSE;
        }
    }

    // set Expected AC_FAIL's based on Platform & DAE0 types
    switch (platformInfo.hw_platform_info.enclosureType)
    {
    case DPE_ENCLOSURE_TYPE:
        switch (platformInfo.hw_platform_info.platformType)
        {
        case SPID_LIGHTNING_HW_TYPE:
        case SPID_HELLCAT_HW_TYPE:
        case SPID_HELLCAT_LITE_HW_TYPE:
            sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][sps_mgmt->base_environment.spid].acFailExpected = TRUE;
            sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][peerSpid].acFailOpposite = TRUE;
            break;
        default:
            break;
        }
        break;
    case XPE_ENCLOSURE_TYPE:
        switch (platformInfo.hw_platform_info.platformType)
        {
        case SPID_MUSTANG_HW_TYPE:
        case SPID_SPITFIRE_HW_TYPE:
        case SPID_PROMETHEUS_M1_HW_TYPE:
        case SPID_ENTERPRISE_HW_TYPE:
            // set xPE Expected AC_FAIL's based on SP side
            if ((spsToTest == FBE_SPS_MGMT_PE) || (spsToTest == FBE_SPS_MGMT_ALL))
            {
                if (sps_mgmt->base_environment.spid == SP_A)
                {
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][0].acFailExpected = TRUE;
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][2].acFailExpected = TRUE;
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][1].acFailOpposite = TRUE;
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][3].acFailOpposite = TRUE;
                }
                else
                {
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][1].acFailExpected = TRUE;
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][3].acFailExpected = TRUE;
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][0].acFailOpposite = TRUE;
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_PE][2].acFailOpposite = TRUE;
                }
            }
            // set DAE0 Expected AC_FAIL's based on DAE0 type
            if ((platformInfo.hw_platform_info.platformType == SPID_MUSTANG_HW_TYPE)||
                (platformInfo.hw_platform_info.platformType == SPID_SPITFIRE_HW_TYPE) ||
                (spsToTest == FBE_SPS_MGMT_DAE0) || (spsToTest == FBE_SPS_MGMT_ALL))
            {
                switch (sps_mgmt->arraySpsInfo->spsTestConfig)
                {
                case FBE_SPS_TEST_CONFIG_XPE:
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][sps_mgmt->base_environment.spid].acFailExpected = TRUE;
                    sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][peerSpid].acFailOpposite = TRUE;
                    break;
                case FBE_SPS_TEST_CONFIG_XPE_VOYAGER:
                case FBE_SPS_TEST_CONFIG_XPE_VIKING:
                    if (sps_mgmt->base_environment.spid == SP_A)
                    {
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][0].acFailExpected = TRUE;
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][1].acFailExpected = TRUE;
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][2].acFailOpposite = TRUE;
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][3].acFailOpposite = TRUE;
                    }
                    else
                    {
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][0].acFailOpposite = TRUE;
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][1].acFailOpposite = TRUE;
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][2].acFailExpected = TRUE;
                        sps_mgmt->arraySpsInfo->spsAcFailInfo[FBE_SPS_MGMT_DAE0][3].acFailExpected = TRUE;
                    }
                    break;
                }
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    // debug
    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, spid %d, enclType %d, spsTestConfig %d, spsToTest %d\n",                          __FUNCTION__, 
                          sps_mgmt->base_environment.spid,
                          platformInfo.hw_platform_info.enclosureType,
                          sps_mgmt->arraySpsInfo->spsTestConfig,
                          spsToTest);
    for (enclIndex = 0; enclIndex < FBE_SPS_MGMT_ENCL_MAX; enclIndex++)
    {
        for (psIndex = 0; psIndex < FBE_ESP_PS_MAX_COUNT_PER_ENCL; psIndex++)
        {
            if (sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailExpected)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Expected, encl %d, ps %d, AC_FAIL required\n",
                                      __FUNCTION__, enclIndex, psIndex);
            }
            if (sps_mgmt->arraySpsInfo->spsAcFailInfo[enclIndex][psIndex].acFailOpposite)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Opposite, encl %d, ps %d, AC_FAIL required\n",
                                      __FUNCTION__, enclIndex, psIndex);
            }
        }
    }

}   // end of fbe_sps_mgmt_determineExpectedPsAcFails


/*!**************************************************************
 * fbe_sps_mgmt_analyzeSpsFaultInfo()
 ****************************************************************
 * @brief
 *  This function determines which component of the SPS is faulted.
 *
 * @param spsFaultInfo - pointer to SPS Fault Info
 *
 * @return none
 *
 * @author
 *  18-Jun-2012:  Created. Joe Perry
 *
 ****************************************************************/
static void fbe_sps_mgmt_analyzeSpsFaultInfo(fbe_sps_fault_info_t *spsFaultInfo)
{
    // different faults indict different SPS component
    if (spsFaultInfo->spsBatteryEOL || spsFaultInfo->spsBatteryNotEngagedFault)
    {
        spsFaultInfo->spsBatteryFault = TRUE;
    }
    if ((spsFaultInfo->spsInternalFault || spsFaultInfo->spsChargerFailure) &&
        (!spsFaultInfo->spsBatteryEOL && !spsFaultInfo->spsBatteryNotEngagedFault))
    {
        spsFaultInfo->spsModuleFault = TRUE;
    }

}   // end of fbe_sps_mgmt_analyzeSpsFaultInfo

/*!**************************************************************
 * fbe_sps_mgmt_analyzeSpsModel()
 ****************************************************************
 * @brief
 *  This function sets SPS fields based on the SPS Model & the
 *  platform.
 *
 * @param spsFaultInfo - pointer to SPS Fault Info
 *
 * @return none
 *
 * @author
 *  01-Aug-2012:  Created. Joe Perry
 *
 ****************************************************************/
static void fbe_sps_mgmt_analyzeSpsModel(fbe_sps_mgmt_t *sps_mgmt,
                                         fbe_sps_info_t *spsStatus)
{

    switch (spsStatus->spsModel)
    {
    case SPS_TYPE_1_0_KW:
    case SPS_TYPE_1_2_KW:
    case SPS_TYPE_2_2_KW:
        spsStatus->dualComponentSps = FALSE;
        break;
    case SPS_TYPE_LI_ION_2_2_KW:
        spsStatus->dualComponentSps = TRUE;
        break;
    default:
        // default to type of SPS that should be used on Trasnformers
        spsStatus->dualComponentSps = TRUE;
        break;
    }

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, %s, spsModel %d, dualComponentSps %d\n",
                          __FUNCTION__, 
                          spsStatus->spsNameString,
                          spsStatus->spsModel,
                          spsStatus->dualComponentSps);

}   // end of fbe_sps_mgmt_analyzeSpsFaultInfo


/*!**************************************************************
 * fbe_sps_mgmt_okToStartSpsTest()
 ****************************************************************
 * @brief
 *  This function determines if OK to start SPS testing.  SPS's
 *  must be fully charged with no faults.
 *
 * @param spsFaultInfo - pointer to SPS Fault Info
 *
 * @return none
 *
 * @author
 *  26-Sep-2012:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_bool_t fbe_sps_mgmt_okToStartSpsTest(fbe_sps_mgmt_t *sps_mgmt,
                                                fbe_sps_ps_encl_num_t *spsEnclToTest)
{
    fbe_u32_t           spsEnclIndex;
    fbe_u32_t           psIndex;
    fbe_bool_t          okToStartSpsTest = FALSE;
    fbe_sps_info_t      *localSpsStatus;
    fbe_ps_info_t       *psStatus;
    fbe_status_t        status;
    fbe_device_physical_location_t  location;
    fbe_base_env_fup_work_state_t   upgradeWorkStateCheck = FBE_BASE_ENV_FUP_WORK_STATE_CHECK_RESULT_DONE;
    fbe_bool_t                      spsFwUpgradeInProgress = FALSE;

    // check that SPS Module/Battery are present & in correct state
    for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
    {
        // check for SPS FW update which is higher priority than SPS testing
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
        if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
            (spsEnclIndex == FBE_SPS_MGMT_PE))
        {
            location.bus = FBE_XPE_PSEUDO_BUS_NUM;
            location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
            upgradeWorkStateCheck = FBE_BASE_ENV_FUP_WORK_STATE_READ_ENTIRE_IMAGE_DONE;
        }
        else
        {
            upgradeWorkStateCheck = FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT;
        }
        location.slot = sps_mgmt->base_environment.spid;        // checking local side
        status = fbe_base_env_get_specific_upgrade_in_progress(&sps_mgmt->base_environment,
                                                               FBE_DEVICE_TYPE_SPS,
                                                               &location,
                                                               upgradeWorkStateCheck,
                                                               &spsFwUpgradeInProgress);
        
        if ((status != FBE_STATUS_OK) || spsFwUpgradeInProgress)
        {
            continue;
        }

        // check for an SPS ready for testing & additional processing based on SPS Test type:
        //   Battery Test - need all SPS's to be ready
        //   Cabling Test - just need to find one SPS ready
        localSpsStatus = &sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS];
        if (localSpsStatus->spsModulePresent &&
                !localSpsStatus->spsFaultInfo.spsModuleFault &&
                (localSpsStatus->spsStatus == SPS_STATE_AVAILABLE) &&
                localSpsStatus->needToTest &&
            (!localSpsStatus->dualComponentSps ||
                (localSpsStatus->dualComponentSps &&
                 localSpsStatus->spsBatteryPresent && !localSpsStatus->spsFaultInfo.spsBatteryFault)))
        {
            // must also check for PS Faults (need peer PS available in case SPS fails during testing
            for (psIndex = 0; psIndex < sps_mgmt->arraySpsInfo->psEnclCounts[spsEnclIndex]; psIndex++)
            {
                psStatus = &sps_mgmt->arraySpsInfo->ps_info[spsEnclIndex][psIndex];
                if ((psStatus->associatedSp != sps_mgmt->base_environment.spid) &&
                    psStatus->psInserted && !psStatus->psFault)
                {
                    // if we find one good SPS with peer PS, we are done
                    okToStartSpsTest = TRUE;
                    *spsEnclToTest = spsEnclIndex;
                    break;
                }
            }

        }
    }

    return okToStartSpsTest;

}   // end of fbe_sps_mgmt_okToStartSpsTest

/*!**************************************************************
 * fbe_sps_mgmt_okToStartBbuTest()
 ****************************************************************
 * @brief
 *  This function determines if OK to start BBU testing.  BBU's
 *  must be fully charged with no faults.
 *
 * @param sps_mgmt - pointer to sps_mgmt data
 *
 * @return none
 *
 * @author
 *  26-Sep-2012:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_bool_t fbe_sps_mgmt_okToStartBbuTest(fbe_sps_mgmt_t *sps_mgmt,
                                                fbe_device_physical_location_t * pLocation)
{
    fbe_bool_t              okToStartBbuTest = TRUE;
    fbe_base_battery_info_t *localBbuStatus;
    fbe_lcc_info_t          peerSpBmLccInfo ={0};
    SP_ID                   peerSp;
    fbe_power_supply_info_t psStatus;
    fbe_u32_t               psIndex, psCount;
    fbe_u32_t               bbuIndex;
    fbe_status_t            status;
    fbe_bool_t              eirSupported;
    fbe_bool_t              psSupported = FALSE;
    fbe_u32_t               bbuStartIndex, bbuEndIndex;
    fbe_board_mgmt_platform_info_t          platform_info;
    fbe_bool_t              bbuFaultFound = FALSE;

    fbe_zero_memory(pLocation, sizeof(fbe_device_physical_location_t));

    // do not test if Bootflash (includes Service Mode)
    if (sps_mgmt->base_environment.isBootFlash)
    {
        okToStartBbuTest = FALSE;
    }

    // do not test if peer SP not present & Caching OK (local BoB can only hold up local SP)
    if (!fbe_cmi_is_peer_alive() &&
        ((sps_mgmt->arraySpsInfo->spsCacheStatus == FBE_CACHE_STATUS_OK) ||
        (sps_mgmt->arraySpsInfo->spsCacheStatus == FBE_CACHE_STATUS_DEGRADED)))
    {
        okToStartBbuTest = FALSE;
    }

    if (sps_mgmt->base_environment.spid == SP_A)
    {
        peerSp = SP_B;
        bbuStartIndex = 0;
        bbuEndIndex = (sps_mgmt->total_bob_count/2);
    }
    else
    {
        peerSp = SP_A;
        bbuStartIndex = sps_mgmt->total_bob_count/2;
        bbuEndIndex = sps_mgmt->total_bob_count;
    }

    // do not test if there are ECB faults
    status = fbe_sps_mgmt_get_bm_lcc_info(sps_mgmt, peerSp, &peerSpBmLccInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Fail to get SP %d BM lcc info, status 0x%X.\n",
                                  __FUNCTION__, peerSp, status);
        okToStartBbuTest = FALSE;
    }
    else if (peerSpBmLccInfo.ecbFault)
    {
        okToStartBbuTest = FALSE;
    }

    /*
     * Check if Local BBU's are good for Testing
     */
    if (okToStartBbuTest)
    {
        // loop through & verify BBU's (support testing & are ready to test)
        for (bbuIndex = bbuStartIndex; bbuIndex < bbuEndIndex; bbuIndex++)
        {
            localBbuStatus = &sps_mgmt->arrayBobInfo->bob_info[bbuIndex]; 
            if (localBbuStatus->associatedSp != sps_mgmt->base_environment.spid)
            {
                continue;
            }

            if (!HwAttrLib_isBbuSelfTestSupported(localBbuStatus->batteryFfid))
            {
                bbuFaultFound = TRUE;
                break;
            }
            // do not test if BBU missing, disabled, faulted or not ready
            else if (!localBbuStatus->batteryInserted ||
                     !localBbuStatus->batteryEnabled ||
                     (localBbuStatus->batteryFaults != FBE_BATTERY_FAULT_NO_FAULT) ||
                     (localBbuStatus->batteryState != FBE_BATTERY_STATUS_BATTERY_READY))
            {
                bbuFaultFound = TRUE;
                break;
            }
        }
        if (bbuFaultFound)
        {
            okToStartBbuTest = FALSE;
        }
    }

    /*
     * Verify that there are no Peer PS problems
     */
    if (okToStartBbuTest)
    {
        // have to verify peer SP can power drives
        // have to verify peer SP's power
        status = fbe_api_power_supply_get_ps_count(sps_mgmt->arrayBobInfo->bobObjectId, &psCount);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Fail to get psCount, status 0x%X.\n",
                                      __FUNCTION__, status);
            okToStartBbuTest = FALSE;
        }
        else
        {
            // need Platform info for determining if PS supported
            fbe_zero_memory(&platform_info, sizeof(fbe_board_mgmt_platform_info_t));
            status = fbe_api_board_get_platform_info(sps_mgmt->arrayBobInfo->bobObjectId, &platform_info);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error getting platform info, status: 0x%X.\n",
                                      __FUNCTION__, status);
                okToStartBbuTest = FALSE;
            }
            else
            {
        
                for (psIndex = 0; psIndex < psCount; psIndex++)
                {
                    fbe_set_memory(&psStatus, 0, sizeof(fbe_power_supply_info_t));
                    psStatus.slotNumOnEncl = psIndex;
                    status = fbe_api_power_supply_get_power_supply_info(sps_mgmt->arrayBobInfo->bobObjectId, &psStatus);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                                  FBE_TRACE_LEVEL_WARNING,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s, Fail to get psStatus, ps %d, status 0x%X.\n",
                                                  __FUNCTION__, psIndex, status);
                        okToStartBbuTest = FALSE;
                        break;
                    }
        
                    // to determine if PS is supported, need FFID & Platform info
                    fbe_base_environment_isPsSupportedByEncl((fbe_base_object_t *)sps_mgmt, 
                                                             pLocation, 
                                                             psStatus.uniqueId,
                                                             (psStatus.ACDCInput == DC_INPUT) ? TRUE : FALSE,
                                                             &psSupported,
                                                             &eirSupported);
                    if ((psStatus.associatedSp == peerSp) && 
                        (!psStatus.bInserted || psStatus.generalFault || !psSupported))
                    {
                        okToStartBbuTest = FALSE;
                        break;
                    }
                }
            }
        }
    }

    if (okToStartBbuTest)
    {
        pLocation->sp = sps_mgmt->base_environment.spid;
        pLocation->bus = pLocation->enclosure = 0;
        for (bbuIndex = bbuStartIndex; bbuIndex < bbuEndIndex; bbuIndex++)
        {
            if (sps_mgmt->arrayBobInfo->bobNeedsTesting[bbuIndex])
            {
                if (pLocation->sp == SP_A)
                {
                    pLocation->slot = bbuIndex; 
                }
                else
                {
                    pLocation->slot = bbuIndex - (sps_mgmt->total_bob_count/2); 
                }
            }
        }
    }
    return okToStartBbuTest;

}   // end of fbe_sps_mgmt_okToStartBbuTest


/*!**************************************************************
 * @fn fbe_sps_mgmt_get_sps_info_ptr(void * pContext,
 *                         fbe_u64_t deviceType,
 *                         fbe_device_physical_location_t * pLocation,
 *                         fbe_sps_info_t ** pSpsInfoPtr)
 ****************************************************************
 * @brief
 *  Find the pointer which points to the SPS info.
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pSpsInfoPtr - 
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Aug-2012: GB - Created.
 *  05-Oct-2012: PHE - Updated to use fbe_sps_mgmt_convertSpsLocation.
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_get_sps_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_sps_info_t ** pSpsInfoPtr)
{
    fbe_sps_mgmt_t                          *pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_u32_t                                spsEnclIndex;
    fbe_u32_t                                spsIndex;

    if(deviceType != FBE_DEVICE_TYPE_SPS) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sps_mgmt_convertSpsLocation(pSpsMgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "SPS(%d_%d_%d) %s, failed to convert location.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pSpsInfoPtr = &(pSpsMgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex]);

    return status;
} //fbe_sps_mgmt_get_sps_info_ptr


/*!**************************************************************
 * @fn fbe_sps_mgmt_get_battery_logical_fault(fbe_base_battery_info_t *bob_info)
 ****************************************************************
 * @brief
 *  This function considers all the fields for the BBU
 *  and decides whether the BBU is logically faulted or not.
 *
 * @param pFanInfo -The pointer to fbe_cooling_mgmt_fan_info_t.
 *
 * @return fbe_bool_t - The BBU is faulted or not logically.
 *
 * @author
 *  12-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
static fbe_bool_t 
fbe_sps_mgmt_get_battery_logical_fault(fbe_base_battery_info_t *bob_info)
{
    fbe_bool_t logicalFault = FBE_FALSE;

    if (!bob_info->batteryInserted ||
        bob_info->batteryFaults != FBE_BATTERY_FAULT_NO_FAULT)
    {
        logicalFault = FBE_TRUE;
    }
    else
    {
        logicalFault = FBE_FALSE;
    }

    return logicalFault;
}


/*!**************************************************************
 * @fn fbe_sps_mgmt_update_encl_fault_led(fbe_cooling_mgmt_t *pCoolingMgmt,
 *                                  fbe_device_physical_location_t *pLocation,
 *                                  fbe_encl_fault_led_reason_t enclFaultLedReason)
 ****************************************************************
 * @brief
 *  This function checks the status to determine
 *  if the Enclosure Fault LED needs to be updated.
 *
 * @param pEnclMgmt -
 * @param pLocation - The location of the enclosure.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  12-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_update_encl_fault_led(fbe_sps_mgmt_t *pSpsMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_base_env_resume_prom_info_t     *pBobResumeInfo = NULL;
    fbe_bool_t                          enclFaultLedOn = FBE_FALSE;
    fbe_object_id_t                     objectId = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                          processorEncl = FALSE;

    fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, (%d_%d), enclFaultLedReason 0x%llX.\n",
                          __FUNCTION__,
                          pLocation->bus,
                          pLocation->enclosure,
                          enclFaultLedReason);

    switch(enclFaultLedReason) 
    {
    case FBE_ENCL_FAULT_LED_SPS_FLT:
        if (((pSpsMgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
             (pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) && (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
            ((pSpsMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) &&
                 (pLocation->bus == 0) && (pLocation->enclosure == 0)))
        {
            processorEncl = TRUE;
            objectId = pSpsMgmt->arraySpsInfo->spsObjectId[FBE_SPS_MGMT_PE];
        }
        else
        {
            objectId = pSpsMgmt->arraySpsInfo->spsObjectId[FBE_SPS_MGMT_DAE0];
        }
        enclFaultLedOn = fbe_sps_mgmt_anySpsFaults(pSpsMgmt, processorEncl);
        break;

    case FBE_ENCL_FAULT_LED_BATTERY_FLT:
        objectId = pSpsMgmt->arrayBobInfo->bobObjectId;
        enclFaultLedOn = fbe_sps_mgmt_anyBbuFaults(pSpsMgmt);
        break;

    case FBE_ENCL_FAULT_LED_BATTERY_RESUME_PROM_FLT:
        objectId = pSpsMgmt->arrayBobInfo->bobObjectId;
        pBobResumeInfo = &pSpsMgmt->arrayBobInfo->bobResumeInfo[pLocation->slot];
        enclFaultLedOn = fbe_base_env_get_resume_prom_fault(pBobResumeInfo->op_status);
        break;
   
    default:
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ENCL(%d_%d), unsupported enclFaultLedReason 0x%llX.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              enclFaultLedReason);

        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    status = fbe_base_environment_updateEnclFaultLed((fbe_base_object_t *)pSpsMgmt,
                                                     objectId,
                                                     enclFaultLedOn,
                                                     enclFaultLedReason);
   
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ENCL(%d_%d), %s enclFaultLedReason %s.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "ENCL(%d_%d), error in %s enclFaultLedReason %s.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));
        
    }
       
    return status;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_process_peer_cache_status_update(fbe_sps_mgmt_t *sps_mgmt, 
 *                                      fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr,
 *                                      fbe_class_id_t classId)
 ****************************************************************
 * @brief
 *  This function will handle the message we received from peer 
 *  to update peerCacheStatus.
 *
 * @param sps_mgmt -
 * @param cmiMsgPtr - Pointer to sps_mgmt cmi packet
 * @param classId - sps_mgmt class id.
 *
 * @return fbe_status_t 
 *
 * @author
 *  20-Nov-2012:  Dipak Patel - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_process_peer_cache_status_update(fbe_sps_mgmt_t *sps_mgmt, 
                                                                  fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_cacheStatus_cmi_packet_t *baseCmiPtr = (fbe_base_env_cacheStatus_cmi_packet_t *)cmiMsgPtr;
    fbe_common_cache_status_t prevCombineCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_common_cache_status_t peerCacheStatus = FBE_CACHE_STATUS_OK;
    
    
    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s entry.\n",
                           __FUNCTION__); 


    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)sps_mgmt,
                                                       &peerCacheStatus); 

    prevCombineCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *)sps_mgmt,
                                                                       sps_mgmt->arraySpsInfo->spsCacheStatus,
                                                                       peerCacheStatus,
                                                                       FBE_CLASS_ID_SPS_MGMT);

    /* Update Peer cache status for this side */
    status = fbe_base_environment_set_peerCacheStatus((fbe_base_environment_t *)sps_mgmt,
                                                       baseCmiPtr->CacheStatus);

    /* Compare it with local cache status and send notification to cache if require.*/ 
    status = fbe_base_environment_compare_cacheStatus((fbe_base_environment_t *)sps_mgmt,
                                                      prevCombineCacheStatus,
                                                      baseCmiPtr->CacheStatus,
                                                      FBE_CLASS_ID_SPS_MGMT);

    /* Check if peer cache status is not initialized at peer side.
       If so, we need to send CMI message to peer to update it's
       peer cache status */
    if (baseCmiPtr->isPeerCacheStatusInitilized == FALSE)
    {
        /* Send CMI to peer.
           Over here, peerCacheStatus for this SP will intilized as we have update it above.
           So we are returning it with TRUE */

        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, peerCachestatus is uninitialized at peer\n",
                           __FUNCTION__);

        status = fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)sps_mgmt, 
                                                                       sps_mgmt->arraySpsInfo->spsCacheStatus,
                                                                       TRUE);
    }

    return status;

}


/*!**************************************************************
 * @fn fbe_sps_mgmt_process_peer_bbu_status(fbe_sps_mgmt_t *sps_mgmt, 
 *                                      fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr,
 *                                      fbe_class_id_t classId)
 ****************************************************************
 * @brief
 *  This function will handle the message we received from peer 
 *  with BBU Self Test changes.
 *
 * @param sps_mgmt -
 * @param cmiMsgPtr - Pointer to sps_mgmt cmi packet
 *
 * @return fbe_status_t 
 *
 * @author
 *  01-Feb-2013:  Joe Perry - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_process_peer_bbu_status(fbe_sps_mgmt_t *sps_mgmt, 
                                                         fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr)
{
    fbe_sps_mgmt_cmi_msg_t  *baseCmiPtr = (fbe_sps_mgmt_cmi_msg_t *)cmiMsgPtr;
    fbe_u32_t               peerBbuIndex;
    fbe_device_physical_location_t  location;
    fbe_u8_t                deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_status_t            status;


    fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));
    fbe_zero_memory(&deviceStr[0], FBE_DEVICE_STRING_LENGTH);
    // save peer BBU Test status & generated events
    // generate peer bbu location, this is inadequate for more than 1 bbu per side
    if (sps_mgmt->base_environment.spid == SP_A)
    {
        peerBbuIndex = FBE_SPB_BOB;
        location.slot = peerBbuIndex;
        location.sp = SP_B;
    }
    else
    {
        peerBbuIndex = FBE_SPA_BOB;
        location.slot = peerBbuIndex;
        location.sp = SP_A;
    }
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    }

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, peerBbuIndex %d, batteryTestStatus, old %s, new %s\n",
                           __FUNCTION__,
                          peerBbuIndex,
                          fbe_sps_mgmt_getBobTestStatusString(sps_mgmt->arrayBobInfo->bob_info[peerBbuIndex].batteryTestStatus),
                          fbe_sps_mgmt_getBobTestStatusString(baseCmiPtr->bbuTestStatus));

    if (sps_mgmt->arrayBobInfo->bob_info[peerBbuIndex].batteryTestStatus != baseCmiPtr->bbuTestStatus)
    {
        fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                            NULL, 0,
                            "%s %s %s", 
                            &deviceStr[0], 
                            fbe_sps_mgmt_getBobTestStatusString(sps_mgmt->arrayBobInfo->bob_info[peerBbuIndex].batteryTestStatus),
                            fbe_sps_mgmt_getBobTestStatusString(baseCmiPtr->bbuTestStatus));
    }
    sps_mgmt->arrayBobInfo->bob_info[peerBbuIndex].batteryTestStatus = baseCmiPtr->bbuTestStatus;

    return FBE_STATUS_OK;

}   // end of fbe_sps_mgmt_process_peer_bbu_status

/*!**************************************************************
 * @fn fbe_sps_mgmt_process_peer_disk_battery_backed_set_info(fbe_sps_mgmt_t *sps_mgmt, 
 *                                      fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr)
 ****************************************************************
 * @brief
 *  This function will handle the message we received from peer 
 *  with diskBatteryBackedSet info.
 *
 * @param sps_mgmt -
 * @param cmiMsgPtr - Pointer to sps_mgmt cmi packet
 *
 * @return fbe_status_t 
 *
 * @author
 * 2/25/2014    zhoue1 
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_process_peer_disk_battery_backed_set_info(fbe_sps_mgmt_t *sps_mgmt, 
                                                         fbe_sps_mgmt_cmi_packet_t * cmiMsgPtr)
{
    fbe_sps_mgmt_cmi_msg_t  *baseCmiPtr = (fbe_sps_mgmt_cmi_msg_t *)cmiMsgPtr;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, peerDiskBatteryBackedSet old %s, new %s\n",
                           __FUNCTION__, 
                           sps_mgmt->arrayBobInfo->peerDiskBatteryBackedSet ? "TRUE" : "FALSE", 
                           baseCmiPtr->diskBatteryBackedSet ? "TRUE" : "FALSE");

    sps_mgmt->arrayBobInfo->peerDiskBatteryBackedSet = baseCmiPtr->diskBatteryBackedSet;

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_anySpsFaults(fbe_sps_mgmt_t *sps_mgmt, 
 ****************************************************************
 * @brief
 *  This function will return whether there are any SPS faults.
 *
 * @param sps_mgmt -
 *
 * @return fbe_bool_t 
 *
 * @author
 *  09-Jan-2013:  Joe Perry - Created. 
 *
 ****************************************************************/
static fbe_bool_t fbe_sps_mgmt_anySpsFaults(fbe_sps_mgmt_t *sps_mgmt,
                                            fbe_bool_t processorEncl)
{
    fbe_bool_t      spsFaults = FALSE;
    fbe_u32_t       enclIndex;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, processorEncl %d, spsCacheStatus %d\n", 
                          __FUNCTION__, processorEncl, sps_mgmt->arraySpsInfo->spsCacheStatus);

    if (processorEncl)
    {
        enclIndex = FBE_SPS_MGMT_PE;
    }
    else
    {
        enclIndex = FBE_SPS_MGMT_DAE0;
    }

    // check for faulted SPS
    if (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsFaultInfo.spsModuleFault ||
        sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsFaultInfo.spsBatteryFault)
    {
        spsFaults = TRUE;
    }

    // check for miscabled SPS
    else if ((sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsCablingStatus == FBE_SPS_CABLING_INVALID_PE) ||
        (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsCablingStatus == FBE_SPS_CABLING_INVALID_DAE0) ||
        (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsCablingStatus == FBE_SPS_CABLING_INVALID_SERIAL) ||
        (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsCablingStatus == FBE_SPS_CABLING_INVALID_MULTI))
    {
        spsFaults = TRUE;
    }

    // check if there are no SPS's present (handle different types of SPS's)
    else if (sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].dualComponentSps && 
             (!sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsModulePresent ||
                !sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsBatteryPresent) &&
             (!sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_PEER_SPS].spsModulePresent ||
                !sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_PEER_SPS].spsBatteryPresent))
    {
        spsFaults = TRUE;
    }
    else if (!sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].dualComponentSps && 
             !sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_LOCAL_SPS].spsModulePresent &&
             !sps_mgmt->arraySpsInfo->sps_info[enclIndex][FBE_PEER_SPS].spsModulePresent)
    {
        spsFaults = TRUE;
    }

    // check if SPS cannot support Caching (Processor Enclosure only)
    if (processorEncl &&
        ((sps_mgmt->arraySpsInfo->spsCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) ||
        (sps_mgmt->arraySpsInfo->spsCacheStatus == FBE_CACHE_STATUS_FAILED)))
    {
        spsFaults = TRUE;
    }

    return spsFaults;

}   // end of fbe_sps_mgmt_anySpsFaults


/*!**************************************************************
 * @fn fbe_sps_mgmt_anyBbuFaults(fbe_sps_mgmt_t *sps_mgmt, 
 ****************************************************************
 * @brief
 *  This function will return whether there are any BBU faults.
 *
 * @param sps_mgmt -
 *
 * @return fbe_bool_t 
 *
 * @author
 *  09-Jan-2013:  Joe Perry - Created. 
 *
 ****************************************************************/
static fbe_bool_t fbe_sps_mgmt_anyBbuFaults(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_bool_t      bbuFaults = FALSE;
    fbe_u32_t       bbuIndex;

    for (bbuIndex = 0; bbuIndex < sps_mgmt->total_bob_count; bbuIndex++)
    {
        // check for faulted BBU
        if (!sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryInserted ||
            (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].batteryFaults != FBE_BATTERY_FAULT_NO_FAULT) ||
            (sps_mgmt->arrayBobInfo->bob_info[bbuIndex].envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD))
        {
            bbuFaults = TRUE;
            break;
        }
    }

    // check if SPS cannot support Caching
    if ((sps_mgmt->arraySpsInfo->spsCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) ||
        (sps_mgmt->arraySpsInfo->spsCacheStatus == FBE_CACHE_STATUS_FAILED))
    {
        bbuFaults = TRUE;
    }

    return bbuFaults;

}   // end of fbe_sps_mgmt_anyBbuFaults


/*!**************************************************************
 * @fn fbe_sps_mgmt_okToShutdownSps(fbe_sps_mgmt_t *sps_mgmt, 
 ****************************************************************
 * @brief
 *  This function will return whether it is OK to shutdown the
 *  SPS.  This depends on whether there is a power failure
 *  on the peer SP or if peer SPS is available.
 *
 * @param sps_mgmt -
 * @param spsEnclIndex - specifies which enclosure to check
 *
 * @return fbe_bool_t 
 *
 * @author
 *  19-Feb-2013:  Joe Perry - Created. 
 *
 ****************************************************************/
static fbe_bool_t fbe_sps_mgmt_okToShutdownSps(fbe_sps_mgmt_t *sps_mgmt,
                                               fbe_sps_ps_encl_num_t spsEnclIndex)
{
    fbe_object_id_t                 objectId;
    fbe_status_t                    status;
    fbe_u32_t                       psIndex;
    fbe_power_supply_info_t         psStatus;
    SP_ID                           peerSp;
    fbe_bool_t                      okToShutdownSps = FALSE;
    fbe_bool_t                      peerPowerAvailable = FALSE;
    fbe_bool_t                      peerSpsAvailable = FALSE;

    objectId = sps_mgmt->arraySpsInfo->spsObjectId[spsEnclIndex];

    if (sps_mgmt->base_environment.spid == SP_A)
    {
        peerSp = SP_B;
    }
    else
    {
        peerSp = SP_A;
    }

    /*
     * Check the peer PS('s) to see if they are running on battery (AC_FAIL set)
     */
    for (psIndex = 0; psIndex < sps_mgmt->arraySpsInfo->psEnclCounts[spsEnclIndex]; psIndex++)
    {
        // get PS status
        fbe_set_memory(&psStatus, 0, sizeof(fbe_power_supply_info_t));
        psStatus.slotNumOnEncl = psIndex;
        status = fbe_api_power_supply_get_power_supply_info(objectId, &psStatus);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting PS Status, slot %d, status: 0x%X\n",
                                  __FUNCTION__, psIndex, status);
            continue;
        }

        // if PS associated with peer SP, check the AC_FAIL status
        if ((psStatus.associatedSps == FBE_SPS_A && peerSp == SP_A) || (psStatus.associatedSps == FBE_SPS_B && peerSp == SP_B))
        {
            if (!psStatus.generalFault && !psStatus.ACFail)
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, encl %d, ps %d, good Power detected\n",
                                      __FUNCTION__, spsEnclIndex, psIndex);
                peerPowerAvailable = TRUE;
                break;
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, encl %d, ps %d, genFlt %d, acFail %d\n",
                                      __FUNCTION__, spsEnclIndex, psIndex,
                                      psStatus.generalFault, psStatus.ACFail);
                continue;
            }
        }
    }

    // check if the peer SPS is available 
    if (sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS].spsModulePresent &&
        sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS].spsBatteryPresent &&
        (sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_PEER_SPS].spsStatus == SPS_STATE_AVAILABLE))
    {
        peerSpsAvailable = TRUE;
    }

    if (peerPowerAvailable && peerSpsAvailable)
    {
        okToShutdownSps = TRUE;
    }

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, okToShutdownSps %d, peerPowerAvailable %d, peerSpsAvailable %d\n",
                          __FUNCTION__, okToShutdownSps, peerPowerAvailable, peerSpsAvailable);
    return okToShutdownSps;

}   // end of fbe_sps_mgmt_okToShutdownSps

/*!**************************************************************
 * @fn fbe_sps_mgmt_processNewEnclosureSps
 ****************************************************************
 * @brief
 *  This function will process a new enclosure with SPS's.
 *
 * @param sps_mgmt -
 * @param locationPtr - location of the enclosure
 *
 * @return fbe_bool_t 
 *
 * @author
 *  04-Apr-2013:  Joe Perry - Created. 
 *
 ****************************************************************/
static
fbe_status_t fbe_sps_mgmt_processNewEnclosureSps(fbe_sps_mgmt_t *sps_mgmt,
                                                 fbe_object_id_t objectId)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_device_physical_location_t  location;
    fbe_sps_ps_encl_num_t           spsEnclIndex;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry\n",
                          __FUNCTION__);

    // only care about Enclosure events for 0_0 (no processing needed for others)
    status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)sps_mgmt, 
                                                       objectId, 
                                                       &location);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get and adjust location for objectId %d failed, status: 0x%x.\n",
                              __FUNCTION__, objectId, status);
        return FBE_STATUS_OK;
    }

    // only process enclosure 0_0
    if ((location.bus == 0) && (location.enclosure == 0))
    {
        spsEnclIndex = FBE_SPS_MGMT_DAE0;
    }
    else
    {
        // we don't care about other enclosures, so return OK
        return FBE_STATUS_OK;
    }

    // New enclosure, get initial SPS status
    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, enclosure %d_%d ready, get SPS info\n",
                          __FUNCTION__, location.bus, location.enclosure);
    sps_mgmt->arraySpsInfo->spsObjectId[spsEnclIndex] = objectId;
    status = fbe_sps_mgmt_processSpsStatus(sps_mgmt, spsEnclIndex, FBE_LOCAL_SPS);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting %s , status: %d\n",
                              __FUNCTION__, 
                              sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                              status);
        return status;
    }
    // only do this processing if SPS present
    else if (sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsModulePresent)
    {
        // setup so SPS is tested
        sps_mgmt->arraySpsInfo->needToTest = TRUE;
        sps_mgmt->arraySpsInfo->testingCompleted = FALSE;
        sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].needToTest = TRUE;

        // get Manufacturing Info if SPS FW revisions changed 
        sps_mgmt->arraySpsInfo->mfgInfoNeeded[spsEnclIndex] = TRUE;
        status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                (fbe_base_object_t *)sps_mgmt, 
                                                FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s, can't clear SPS Mfg Info timer condition, status: 0x%X\n",
                                  __FUNCTION__, 
                                  sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                                  status);
        }
    }

    return status;

}   // end of fbe_sps_mgmt_processNewEnclosureSps

/*!**************************************************************
 * @fn fbe_sps_mgmt_processRemovedEnclosureSps
 ****************************************************************
 * @brief
 *  This function will process a removed enclosure with SPS's.
 *
 * @param sps_mgmt -
 * @param locationPtr - location of the enclosure
 *
 * @return fbe_bool_t 
 *
 * @author
 *  04-Apr-2013:  Joe Perry - Created. 
 *
 ****************************************************************/
static
fbe_status_t fbe_sps_mgmt_processRemovedEnclosureSps(fbe_sps_mgmt_t *sps_mgmt,
                                                     fbe_object_id_t objectId)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_sps_ps_encl_num_t   spsEnclIndex;

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry, objectId 0x%x\n",
                          __FUNCTION__, objectId);

    // check if any object ID's match the one being destroyed
    for (spsEnclIndex = 0; spsEnclIndex < sps_mgmt->encls_with_sps; spsEnclIndex++)
    {
        if (sps_mgmt->arraySpsInfo->spsObjectId[spsEnclIndex] == objectId)
        {
            // clear object ID
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, objId 0x%x removed\n",
                                  __FUNCTION__,
                                  sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                                  objectId);
            sps_mgmt->arraySpsInfo->spsObjectId[spsEnclIndex] = FBE_OBJECT_ID_INVALID;

            status = fbe_sps_mgmt_processNotPresentStatus(sps_mgmt, 
                                                          spsEnclIndex, 
                                                          SPS_MGMT_SPS_EVENT_MODULE_REMOVE);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                       FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s, error removing %s, status %d\n",
                                       __FUNCTION__,
                                      sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsNameString, 
                                      status);
                status = FBE_STATUS_OK;
            }
            break;
        }
    }

    return status;

}   // end of fbe_sps_mgmt_processRemovedEnclosureSps



/*!**************************************************************
 * @fn fbe_sps_mgmt_get_current_sps_info(fbe_sps_mgmt_t * sps_mgmt,
 *                                          fbe_device_physical_location_t * pLocation,
 *                                          fbe_sps_info_t * pCurrentSpsInfo,
 *                                          fbe_sps_info_t * pPreviousSpsInfo,
 *                                          fbe_bool_t * pRevChanged)
 ****************************************************************
 * @brief
 *  This function gets the current SPS info.
 *
 * @param sps_mgmt - The pointer to sps_mgmt.
 * @param pLocation - The pointer to the physical location of the SPS.
 * @param pCurrentSpsInfo (OUTPUT) - The pointer to the current sps info to be queried.
 * 
 * @return fbe_status_t - 
 *
 * @version
 *  07-May-2013: PHE - Copied over from the original fbe_sps_mgmt_processSpsStatus.
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_get_current_sps_info(fbe_sps_mgmt_t * sps_mgmt,
                                       fbe_device_physical_location_t * pLocation,
                                       fbe_sps_info_t * pCurrentSpsInfo)
{
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u32_t                               spsEnclIndex = 0;
    fbe_u32_t                               spsIndex = 0;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sps_get_sps_status_t                spsStatus = {0};
    fbe_bool_t                              spsFwUpgradeInProgress = FBE_FALSE;
    fbe_base_env_fup_work_state_t           upgradeWorkStateCheck = FBE_BASE_ENV_FUP_WORK_STATE_CHECK_RESULT_DONE;

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               pLocation, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    }

    status = fbe_sps_mgmt_convertSpsLocation(sps_mgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to convert location.\n",
                              &spsDeviceStr[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Check if we are simulating SPS
     */
    if (sps_mgmt->arraySpsInfo->simulateSpsInfo)
    {
        pCurrentSpsInfo->spsModulePresent = TRUE;
        pCurrentSpsInfo->spsBatteryPresent = TRUE;
        pCurrentSpsInfo->spsStatus = SPS_STATE_AVAILABLE;
        fbe_set_memory(&(pCurrentSpsInfo->spsFaultInfo), 0, sizeof(fbe_sps_fault_info_t));
        pCurrentSpsInfo->spsSupportedStatus = TRUE;
        pCurrentSpsInfo->spsInputPower.status = FBE_ENERGY_STAT_INVALID;
        pCurrentSpsInfo->spsModel = SPS_TYPE_LI_ION_2_2_KW;      // default to SPS used on Transformers
        pCurrentSpsInfo->spsBatteryTime = 300;
        pCurrentSpsInfo->spsCablingStatus = FBE_SPS_CABLING_VALID;
        sps_mgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsCablingStatus = FBE_SPS_CABLING_VALID;
        
    }
    else
    {
        // Get SPS status
        fbe_set_memory(&spsStatus, 0, sizeof(fbe_sps_get_sps_status_t));
        spsStatus.spsIndex = spsIndex;
        status = fbe_api_sps_get_sps_info(sps_mgmt->arraySpsInfo->spsObjectId[spsEnclIndex], 
                                          &spsStatus);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t*)sps_mgmt,  
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting SPS Status, status 0x%X.\n",
                                  &spsDeviceStr[0], 
                                  status);

            pCurrentSpsInfo->envInterfaceStatus = FBE_ENV_INTERFACE_STATUS_NA;
            pCurrentSpsInfo->spsModulePresent = FALSE;
            pCurrentSpsInfo->spsBatteryPresent = FALSE;
            pCurrentSpsInfo->spsStatus = SPS_STATE_UNKNOWN;
            pCurrentSpsInfo->spsModel = SPS_TYPE_UNKNOWN;
            pCurrentSpsInfo->spsBatteryTime = 0;
            pCurrentSpsInfo->spsModuleFfid = 0;
            pCurrentSpsInfo->spsBatteryFfid = 0;
        }
        else
        {
            // check for SPS FW update and ignore changes in insertness & faults
            if (pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM)
            {
                upgradeWorkStateCheck = FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT;
            }
            else
            {
                upgradeWorkStateCheck = FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT;
            }
            status = fbe_base_env_get_specific_upgrade_in_progress(&sps_mgmt->base_environment,
                                                                   FBE_DEVICE_TYPE_SPS,
                                                                   pLocation,
                                                                   upgradeWorkStateCheck,
                                                                   &spsFwUpgradeInProgress);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, getting spsFwUpgradeInProgress failed, status 0x%X.\n",
                                      &spsDeviceStr[0],
                                      status);

                spsFwUpgradeInProgress = FALSE;
            }

            // check for LCC FW update (DAE 0_0 only) and ignore the SPS being removed
            if ((sps_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
                (pLocation->bus == 0) && (pLocation->enclosure == 0))
            {
                //DAE0 SPS would detected "removed" for about 30 seconds after LCC FUP activation completed, so keep this flag a bit long to ignore SPS status change.
                if ((pCurrentSpsInfo->lccFwActivationInProgress) && (!spsStatus.spsInfo.lccFwActivationInProgress) 
                    && (spsStatus.spsInfo.spsModulePresent == FBE_FALSE)
                    && (sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFupCount < FBE_SPS_STATUS_IGNORE_AFTER_LCC_FUP_COUNT))
                {
                    fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, LCC Fup just finished, ignore SPS status change for a period. Current delay count: %d\n",
                                          &spsDeviceStr[0],
                                          sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFupCount);

                    sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFup = FBE_TRUE;
                    // clear condition to start the debounce timer
                    status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                                            (fbe_base_object_t*)sps_mgmt, 
                                                            FBE_SPS_MGMT_DEBOUNCE_TIMER_COND);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s can't clear debounce timer condition, status: 0x%X\n",
                                              __FUNCTION__, status);
                    }
                }
                else
                {
                    pCurrentSpsInfo->lccFwActivationInProgress = spsStatus.spsInfo.lccFwActivationInProgress;
                    sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFup = FBE_FALSE;
                    sps_mgmt->arraySpsInfo->spsStatusDelayAfterLccFupCount = 0;
                }
            }
            
            fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, spsFwUpgradeInProgress %d, lccFwActivationInProgress %d\n",
                                      &spsDeviceStr[0],
                                      spsFwUpgradeInProgress,
                                      pCurrentSpsInfo->lccFwActivationInProgress);

            if (pCurrentSpsInfo->lccFwActivationInProgress == FBE_TRUE)
            {
                // Ignore SPS status change during DAE0 LCC Fup.
                fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Ignore SPSInsert:%d BatteryInsert:%d Status(%s) Fault(%s) changes due to DAE0 LCC FUP\n",
                                      &spsDeviceStr[0],
                                      spsStatus.spsInfo.spsModulePresent,
                                      spsStatus.spsInfo.spsBatteryPresent,
                                      fbe_sps_mgmt_getSpsStatusString(spsStatus.spsInfo.spsStatus),
                                      fbe_sps_mgmt_getSpsFaultString(&spsStatus.spsInfo.spsFaultInfo));
            }
            else
            {
                if (spsFwUpgradeInProgress == FBE_FALSE)
                {
                    // fup not in progress, copy over the new spsinfo
                    pCurrentSpsInfo->spsStatus = spsStatus.spsInfo.spsStatus;
                    fbe_sps_mgmt_analyzeSpsFaultInfo(&spsStatus.spsInfo.spsFaultInfo);
                    pCurrentSpsInfo->spsFaultInfo = spsStatus.spsInfo.spsFaultInfo;
                    pCurrentSpsInfo->spsModulePresent = spsStatus.spsInfo.spsModulePresent;
                    pCurrentSpsInfo->spsBatteryPresent = spsStatus.spsInfo.spsBatteryPresent;
                    /* We might not be able to get spsModuleFfid and spsBatteryFfid for DAE0 SPS
                     * because they were populated after the SPS manufacturing info was read in the physical package.
                     * The SPS manufacturing info was read after the client calling 
                     * fbe_api_sps_get_sps_manuf_info
                     */
                    pCurrentSpsInfo->spsModuleFfid = spsStatus.spsInfo.spsModuleFfid; 
                    pCurrentSpsInfo->spsBatteryFfid = spsStatus.spsInfo.spsBatteryFfid;
                    pCurrentSpsInfo->bSpsModuleDownloadable = spsStatus.spsInfo.bSpsModuleDownloadable;
                    pCurrentSpsInfo->bSpsBatteryDownloadable = spsStatus.spsInfo.bSpsBatteryDownloadable;
                    pCurrentSpsInfo->programmableCount = spsStatus.spsInfo.programmableCount;
                }
                else if (pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM)
                {
                    // When SPE SPS transitions through firmware activation we will see SPS status change
                    // but not the other fields. So do not copy new spsStatus during fup activation.
                    pCurrentSpsInfo->spsModulePresent = spsStatus.spsInfo.spsModulePresent;
                    pCurrentSpsInfo->spsBatteryPresent = spsStatus.spsInfo.spsBatteryPresent;
                    pCurrentSpsInfo->spsModuleFfid = spsStatus.spsInfo.spsModuleFfid; 
                    pCurrentSpsInfo->spsBatteryFfid = spsStatus.spsInfo.spsBatteryFfid;
                    pCurrentSpsInfo->bSpsModuleDownloadable = spsStatus.spsInfo.bSpsModuleDownloadable;
                    pCurrentSpsInfo->bSpsBatteryDownloadable = spsStatus.spsInfo.bSpsBatteryDownloadable;
                    pCurrentSpsInfo->programmableCount = spsStatus.spsInfo.programmableCount;
                    fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Ignore Status(%s) Fault(%s) changes due to firmware upgrade\n",
                                      &spsDeviceStr[0],
                                      fbe_sps_mgmt_getSpsStatusString(spsStatus.spsInfo.spsStatus),
                                      fbe_sps_mgmt_getSpsFaultString(&spsStatus.spsInfo.spsFaultInfo));
                }
                else
                {
                    // When SPS DAE transitions through image activation the SPS will get removed.
                    // Ignore new spsInfo during activation so we ride through and don't cause cache disable.
                    fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, Ignore SPSInsert:%d BatteryInsert:%d Status(%s) Fault(%s) changes due to FUP\n",
                                          &spsDeviceStr[0],
                                          spsStatus.spsInfo.spsModulePresent,
                                          spsStatus.spsInfo.spsBatteryPresent,
                                          fbe_sps_mgmt_getSpsStatusString(spsStatus.spsInfo.spsStatus),
                                          fbe_sps_mgmt_getSpsFaultString(&spsStatus.spsInfo.spsFaultInfo));
                }

                pCurrentSpsInfo->envInterfaceStatus = spsStatus.spsInfo.envInterfaceStatus;
                pCurrentSpsInfo->transactionStatus = spsStatus.spsInfo.transactionStatus;
                pCurrentSpsInfo->spsModel = spsStatus.spsInfo.spsModel;
                pCurrentSpsInfo->spsBatteryTime = spsStatus.spsInfo.spsBatteryTime;

                fbe_copy_memory(&pCurrentSpsInfo->primaryFirmwareRev[0], 
                                &spsStatus.spsInfo.primaryFirmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
                fbe_copy_memory(&pCurrentSpsInfo->secondaryFirmwareRev[0], 
                                &spsStatus.spsInfo.secondaryFirmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
                fbe_copy_memory(&pCurrentSpsInfo->batteryFirmwareRev[0], 
                                &spsStatus.spsInfo.batteryFirmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
            }
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_sps_update_debounced_status(fbe_sps_mgmt_t * sps_mgmt,
 *                                             fbe_device_physical_location_t * pLocation,
 *                                             fbe_sps_info_t * pCurrentSpsInfo)
 ****************************************************************
 * @brief
 *  This function updates the debounced status as needed.
 *  
 * @param sps_mgmt - The pointer to sps_mgmt.
 * @param pLocation - The pointer to the physical location of the SPS.
 * @param pCurrentSpsInfo - The pointer to the current sps info.
 * 
 * @return fbe_status_t - 
 *
 * @version
 *  07-May-2013: PHE - Copied over from the original fbe_sps_mgmt_processSpsStatus.
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_sps_update_debounced_status(fbe_sps_mgmt_t * sps_mgmt,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_sps_info_t * pCurrentSpsInfo)
 
{  
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               spsEnclIndex;
    fbe_u32_t                               spsIndex;

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               pLocation, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    }

    status = fbe_sps_mgmt_convertSpsLocation(sps_mgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to convert location.\n",
                              &spsDeviceStr[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    // check if any debouncing should be terminated
    if ((sps_mgmt->arraySpsInfo->debouncedSpsStatus[spsEnclIndex] == SPS_STATE_ON_BATTERY) &&
        (pCurrentSpsInfo->spsStatus != SPS_STATE_ON_BATTERY))
    {
        sps_mgmt->arraySpsInfo->debouncedSpsStatus[spsEnclIndex] = SPS_STATE_UNKNOWN;
        sps_mgmt->arraySpsInfo->debouncedSpsStatusCount[spsEnclIndex] = 0;
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, terminate SpsStatus debounce\n",
                              &spsDeviceStr[0],
                              __FUNCTION__);
    }
    if ((sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsEnclIndex].spsModuleFault &&
         !pCurrentSpsInfo->spsFaultInfo.spsModuleFault) ||
        (sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsEnclIndex].spsBatteryFault &&
             !pCurrentSpsInfo->spsFaultInfo.spsBatteryFault))
    {
        fbe_zero_memory(&sps_mgmt->arraySpsInfo->debouncedSpsFaultInfo[spsEnclIndex], 
                        sizeof(fbe_sps_fault_info_t));
        sps_mgmt->arraySpsInfo->debouncedSpsFaultsCount[spsEnclIndex] = 0;
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, terminate SpsFaults debounce\n",
                              &spsDeviceStr[0],
                              __FUNCTION__);
    } 

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_sps_compare_firmware_rev(fbe_sps_mgmt_t * sps_mgmt,
 *                                          fbe_device_physical_location_t * pLocation,
 *                                          fbe_sps_info_t * pCurrentSpsInfo,
 *                                          fbe_sps_info_t * pPreviousSpsInfo,
 *                                          fbe_bool_t * pRevChanged)
 ****************************************************************
 * @brief
 *  This function compares the firmware revision for the SPS programmables.
 *
 * @param sps_mgmt - The pointer to sps_mgmt.
 * @param pLocation - The pointer to the physical location of the SPS.
 * @param pCurrentSpsInfo - The pointer to the current sps info.
 * @param pPreviousSpsInfo - The pointer of the previous sps info.
 * @param pRevChanged (OUTPUT) - The poiter to value which indicates whether there is any rev change.
 * 
 * @return fbe_status_t - 
 *
 * @version
 *  07-May-2013: PHE - Copied over from the original fbe_sps_mgmt_processSpsStatus.
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_sps_compare_firmware_rev(fbe_sps_mgmt_t * sps_mgmt,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_sps_info_t * pCurrentSpsInfo,
                                                   fbe_sps_info_t * pPreviousSpsInfo,
                                                   fbe_bool_t * pRevChanged)
{
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_status_t                            status = FBE_STATUS_OK;


    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               pLocation, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    }

    *pRevChanged = FBE_FALSE;

    // check for rev change, need to notify peer with new rev
    if(pCurrentSpsInfo->spsModulePresent &&
       (strncmp(&pCurrentSpsInfo->primaryFirmwareRev[0], 
               &pPreviousSpsInfo->primaryFirmwareRev[0], 
               FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) != 0))
    {
        *pRevChanged = FBE_TRUE;
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %s SPSPrimary Curr:%s Prev:%s\n",
                          &spsDeviceStr[0],
                          __FUNCTION__, 
                          &pCurrentSpsInfo->primaryFirmwareRev[0],
                          &pPreviousSpsInfo->primaryFirmwareRev[0]);
    }
    if(pCurrentSpsInfo->spsModulePresent &&
       (strncmp(&pCurrentSpsInfo->secondaryFirmwareRev[0], 
               &pPreviousSpsInfo->secondaryFirmwareRev[0], 
               FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) != 0))
    {
        *pRevChanged = FBE_TRUE;
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %s SPSSecondary Curr:%s Prev:%s\n",
                          &spsDeviceStr[0],
                          __FUNCTION__, 
                          &pCurrentSpsInfo->secondaryFirmwareRev[0],
                          &pPreviousSpsInfo->secondaryFirmwareRev[0]);
    }
    if(pCurrentSpsInfo->spsBatteryPresent &&
       (strncmp(&pCurrentSpsInfo->batteryFirmwareRev[0], 
               &pPreviousSpsInfo->batteryFirmwareRev[0], 
               FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE) != 0))
    {
        *pRevChanged = FBE_TRUE;
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt,  
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %s SPSBattery Curr:%s Prev:%s\n",
                          &spsDeviceStr[0],
                          __FUNCTION__, 
                          &pCurrentSpsInfo->batteryFirmwareRev[0],
                          &pPreviousSpsInfo->batteryFirmwareRev[0]);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_sps_check_state_change(fbe_sps_mgmt_t * sps_mgmt,
 *                                          fbe_device_physical_location_t * pLocation,
 *                                          fbe_sps_info_t * pCurrentSpsInfo,
 *                                          fbe_sps_info_t * pPreviousSpsInfo,
 *                                          fbe_bool_t * pSpsStateChanged)
 ****************************************************************
 * @brief
 *  This function checks whether there is any state change for the specified SPS.
 *
 * @param sps_mgmt - The pointer to sps_mgmt.
 * @param pLocation - The pointer to the physical location of the SPS.
 * @param pCurrentSpsInfo - The pointer to the current sps info.
 * @param pPreviousSpsInfo - The pointer of the previous sps info.
 * @param pSpsStateChanged (OUTPUT) - The poiter to value which indicates whether spsState changed.
 * 
 * @return fbe_status_t - 
 *
 * @version
 *  07-May-2013: PHE - Copied over from the original fbe_sps_mgmt_processSpsStatus.
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_sps_check_state_change(fbe_sps_mgmt_t * sps_mgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_sps_info_t * pCurrentSpsInfo,
                                                 fbe_sps_info_t * pPreviousSpsInfo,
                                                 fbe_bool_t * pSpsStateChanged)
{
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                spsBatteryDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               spsEnclIndex;
    fbe_u32_t                               spsIndex;

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS, 
                                               pLocation, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SPS_BATTERY, 
                                               pLocation, 
                                               &spsBatteryDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string sps battery.\n", 
                              __FUNCTION__); 
    }

    status = fbe_sps_mgmt_convertSpsLocation(sps_mgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to convert location.\n",
                              &spsDeviceStr[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pSpsStateChanged = FBE_FALSE;

    if (pCurrentSpsInfo->envInterfaceStatus != pPreviousSpsInfo->envInterfaceStatus)
    {
        *pSpsStateChanged = FBE_TRUE;
        if (pCurrentSpsInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &spsDeviceStr[0],
                                SmbTranslateErrorString(pCurrentSpsInfo->transactionStatus));
        }
        else if (pCurrentSpsInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &spsDeviceStr[0],
                                fbe_base_env_decode_env_status(pCurrentSpsInfo->envInterfaceStatus));
        }
        else
        {
            fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                NULL, 0,
                                "%s", 
                                &spsDeviceStr[0]);
        }
    }

    if ((pPreviousSpsInfo->spsStatus != pCurrentSpsInfo->spsStatus) ||
        (pPreviousSpsInfo->spsModulePresent != pCurrentSpsInfo->spsModulePresent)||
        (pPreviousSpsInfo->spsBatteryPresent != pCurrentSpsInfo->spsBatteryPresent)||
        (pPreviousSpsInfo->spsFaultInfo.spsModuleFault != pCurrentSpsInfo->spsFaultInfo.spsModuleFault) ||
        (pPreviousSpsInfo->spsFaultInfo.spsBatteryFault != pCurrentSpsInfo->spsFaultInfo.spsBatteryFault))
    {
        *pSpsStateChanged = FBE_TRUE;
        // check for change in Module & Battery insert/remove
        if ((pPreviousSpsInfo->spsModulePresent != pCurrentSpsInfo->spsModulePresent) ||
            (pPreviousSpsInfo->spsBatteryPresent != pCurrentSpsInfo->spsBatteryPresent))
        {
            // SPS Module & Battery inserted
            if (!pPreviousSpsInfo->spsModulePresent && pCurrentSpsInfo->spsModulePresent &&
                !pPreviousSpsInfo->spsBatteryPresent && pCurrentSpsInfo->spsBatteryPresent)
            {
                fbe_sps_mgmt_processInsertedSps(sps_mgmt, spsEnclIndex, SPS_MGMT_SPS_EVENT_MODULE_BATTERY_INSERT);
            }
            // just SPS Module inserted
            else if (!pPreviousSpsInfo->spsModulePresent && pCurrentSpsInfo->spsModulePresent &&
                (pPreviousSpsInfo->spsBatteryPresent == pCurrentSpsInfo->spsBatteryPresent))
            {
                fbe_sps_mgmt_processInsertedSps(sps_mgmt, spsEnclIndex, SPS_MGMT_SPS_EVENT_MODULE_INSERT);
            }
            // SPS Battery inserted
            else if ((pPreviousSpsInfo->spsModulePresent == pCurrentSpsInfo->spsModulePresent) &&
                !pPreviousSpsInfo->spsBatteryPresent && pCurrentSpsInfo->spsBatteryPresent)
            {
                fbe_sps_mgmt_processInsertedSps(sps_mgmt, spsEnclIndex, SPS_MGMT_SPS_EVENT_BATTERY_INSERT);
            }
            // SPS Module & Battery removed
            else if (pPreviousSpsInfo->spsModulePresent && !pCurrentSpsInfo->spsModulePresent)
            {
                fbe_sps_mgmt_processNotPresentStatus(sps_mgmt, spsEnclIndex, SPS_MGMT_SPS_EVENT_MODULE_REMOVE);
            }
            // just SPS Battery removed
            else if ((pPreviousSpsInfo->spsModulePresent == pCurrentSpsInfo->spsModulePresent) &&
                 pPreviousSpsInfo->spsBatteryPresent && !pCurrentSpsInfo->spsBatteryPresent)
            {
                fbe_sps_mgmt_processNotPresentStatus(sps_mgmt, spsEnclIndex, SPS_MGMT_SPS_EVENT_BATTERY_REMOVE);
            }
        }

        if (pCurrentSpsInfo->spsModulePresent)
        {
            switch (pCurrentSpsInfo->spsStatus)
            {
            case SPS_STATE_AVAILABLE:
                fbe_sps_mgmt_processBatteryOkStatus(sps_mgmt, spsEnclIndex, pPreviousSpsInfo);
                break;
            case SPS_STATE_CHARGING:
                fbe_sps_mgmt_processRechargingStatus(sps_mgmt, spsEnclIndex, pPreviousSpsInfo);
                break;
            case SPS_STATE_TESTING:
                fbe_sps_mgmt_processTestingStatus(sps_mgmt, pPreviousSpsInfo);
                break;
            case SPS_STATE_ON_BATTERY:
                fbe_sps_mgmt_processBatteryOnlineStatus(sps_mgmt, spsEnclIndex, pPreviousSpsInfo);
                break;
            case SPS_STATE_FAULTED:
                fbe_sps_mgmt_processFaultedStatus(sps_mgmt, spsEnclIndex, pPreviousSpsInfo);
                break;
            default:
                break;
            }

            // status changed to Faulted, so log SPS faulted
            if (pCurrentSpsInfo->spsFaultInfo.spsModuleFault && !pPreviousSpsInfo->spsFaultInfo.spsModuleFault)
            {
                fbe_event_log_write(ESP_ERROR_SPS_FAULTED,
                                    NULL, 0,
                                    "%s %s %s",
                                    &spsDeviceStr[0],
                                    fbe_sps_mgmt_getSpsFaultString(&pPreviousSpsInfo->spsFaultInfo),
                                    fbe_sps_mgmt_getSpsFaultString(&pCurrentSpsInfo->spsFaultInfo));
            }
            if (pCurrentSpsInfo->spsFaultInfo.spsBatteryFault && !pPreviousSpsInfo->spsFaultInfo.spsBatteryFault)
            {
                fbe_event_log_write(ESP_ERROR_SPS_FAULTED,
                                    NULL, 0,
                                    "%s %s %s",
                                    &spsBatteryDeviceStr[0],
                                    fbe_sps_mgmt_getSpsFaultString(&pPreviousSpsInfo->spsFaultInfo),
                                    fbe_sps_mgmt_getSpsFaultString(&pCurrentSpsInfo->spsFaultInfo));
            }
        }

        if (pPreviousSpsInfo->spsStatus != pCurrentSpsInfo->spsStatus)
        {
            fbe_event_log_write(ESP_INFO_SPS_STATE_CHANGE,
                                NULL, 0,
                                "%s %s %s", 
                                &spsDeviceStr[0],
                                fbe_sps_mgmt_getSpsStatusString(pPreviousSpsInfo->spsStatus),
                                fbe_sps_mgmt_getSpsStatusString(pCurrentSpsInfo->spsStatus));
        }
    }

    if ((pCurrentSpsInfo->spsModel != pPreviousSpsInfo->spsModel)
        || (pCurrentSpsInfo->spsModuleFfid != pPreviousSpsInfo->spsModuleFfid)
        || (pCurrentSpsInfo->spsBatteryFfid != pPreviousSpsInfo->spsBatteryFfid))
    {
        *pSpsStateChanged = FBE_TRUE;
    }
    
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_sps_mgmt_calculateBatteryStateInfo()
 ****************************************************************
 * @brief
 *  This function will calculate & update the specified
 *  Battery's State & SubState attributes.
 *
 * @param sps_mgmt - Handle to sps_mgmt.
 * @param pLocation - pointer to Battery location info
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Apr-2015: Created  Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_sps_mgmt_calculateBatteryStateInfo(fbe_sps_mgmt_t *sps_mgmt,
                                       fbe_device_physical_location_t *pLocation,
                                       fbe_u32_t bbuSlotIndex)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u8_t                                spsDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               pLocation, 
                                               &spsDeviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
        return status;
    }

    if (sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].isFaultRegFail)
    {
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].state = FBE_BATTERY_STATE_FAULTED;
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].subState = FBE_BATTERY_SUBSTATE_FLT_STATUS_REG_FAULT;
    }
    else if (sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].resumePromReadFailed)
    {
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].state = FBE_BATTERY_STATE_DEGRADED;
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].subState = FBE_BATTERY_SUBSTATE_RP_READ_FAILURE;
    } 
    else if ((sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL) ||
             (sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE))
    {
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].state = FBE_BATTERY_STATE_DEGRADED;
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].subState = FBE_BATTERY_SUBSTATE_ENV_INTF_FAILURE;
    } 
    else 
    {
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].state = FBE_BATTERY_STATE_OK;
        sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].subState = FBE_BATTERY_SUBSTATE_NO_FAULT;
    }

    fbe_base_object_trace((fbe_base_object_t*)sps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %s, state %s, substate %s\n",
                          __FUNCTION__,
                          &spsDeviceStr[0],
                          fbe_sps_mgmt_decode_bbu_state(sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].state),
                          fbe_sps_mgmt_decode_bbu_subState(sps_mgmt->arrayBobInfo->bob_info[bbuSlotIndex].subState));
    return FBE_STATUS_OK;

}   // end of fbe_sps_mgmt_calculateBatteryStateInfo

/*!***************************************************************
 * fbe_sps_mgmt_verifyRideThroughTimeout()
 ****************************************************************
 * @brief
 *  This function will verify the RideThroughTimeout & update it
 *  if necessary.
 *
 * @param sps_mgmt - Handle to sps_mgmt.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Jul-2015: Created  Joe Perry
 *
 ****************************************************************/
static
fbe_status_t fbe_sps_mgmt_verifyRideThroughTimeout(fbe_sps_mgmt_t *sps_mgmt)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_bmc_info_t               getBmcInfo;
    fbe_bool_t                              updateRideThroughTime = FALSE;
    fbe_base_board_mgmt_battery_command_t   batteryCommand;
//    fbe_u32_t                               bobIndex;

    fbe_zero_memory(&batteryCommand, sizeof(fbe_base_board_mgmt_battery_command_t));

    // get the current setting of the Ride Through timer
    fbe_zero_memory(&getBmcInfo, sizeof(fbe_board_mgmt_bmc_info_t));
    getBmcInfo.associatedSp = sps_mgmt->base_environment.spid;
    status = fbe_api_board_get_bmc_info(sps_mgmt->object_id, &getBmcInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Bmc info, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, isBootFlash %d, rideThroughTime %d, bobCnt %d\n",
                          __FUNCTION__, 
                          sps_mgmt->base_environment.isBootFlash,
                          getBmcInfo.rideThroughTime,
                          sps_mgmt->total_bob_count);

    // if running in Bootflash, Ride Through timer should be disabled
    if (sps_mgmt->base_environment.isBootFlash)
    {
        if (getBmcInfo.rideThroughTime != FBE_BMC_RIDE_THROUGH_TIMER_DISABLE_VALUE)
        {
            // Disable Ride Through timer
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, disable rideThroughTime (BootFlash & time 0)\n",
                                  __FUNCTION__);
            updateRideThroughTime = TRUE;
            batteryCommand.batteryAction = FBE_BATTERY_ACTION_DISABLE_RIDE_THROUGH;
        }
    }
    else
    {
// Disable the Ride Through Timer all the time
#if TRUE
        if (getBmcInfo.rideThroughTime != FBE_BMC_RIDE_THROUGH_TIMER_DISABLE_VALUE)
        {
            // Disable Ride Through timer
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, disable rideThroughTime\n",
                                  __FUNCTION__);
            updateRideThroughTime = TRUE;
            batteryCommand.batteryAction = FBE_BATTERY_ACTION_DISABLE_RIDE_THROUGH;
        }
#else
        // set Ride Through timer based on local BBU BatteryReady status
        for (bobIndex = 0; bobIndex < sps_mgmt->total_bob_count; bobIndex++)
        {
            if (sps_mgmt->arrayBobInfo->bob_info[bobIndex].associatedSp == sps_mgmt->base_environment.spid)
            {
                if (sps_mgmt->arrayBobInfo->bob_info[bobIndex].batteryReady && 
                    (getBmcInfo.rideThroughTime != FBE_BMC_RIDE_THROUGH_TIMER_ENABLE_VALUE))
                {
                    // Enable Ride Through timer
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, enable rideThroughTime, index %d batRdy %d\n",
                                          __FUNCTION__,
                                          bobIndex,
                                          sps_mgmt->arrayBobInfo->bob_info[bobIndex].batteryReady);
                    updateRideThroughTime = TRUE;
                    batteryCommand.batteryAction = FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH;
                }
                if (!sps_mgmt->arrayBobInfo->bob_info[bobIndex].batteryReady && 
                    (getBmcInfo.rideThroughTime != FBE_BMC_RIDE_THROUGH_TIMER_DISABLE_VALUE))
                {
                    // Disable Ride Through timer
                    fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, disable rideThroughTime, index %d batRdy %d\n",
                                          __FUNCTION__,
                                          bobIndex,
                                          sps_mgmt->arrayBobInfo->bob_info[bobIndex].batteryReady);
                    updateRideThroughTime = TRUE;
                    batteryCommand.batteryAction = FBE_BATTERY_ACTION_DISABLE_RIDE_THROUGH;
                }
            }
        }
#endif
    }

    if (updateRideThroughTime)
    {
        // send message to Phy Pkg
        batteryCommand.device_location.sp = sps_mgmt->base_environment.spid;
        status = fbe_api_board_send_battery_command(sps_mgmt->arrayBobInfo->bobObjectId, &batteryCommand); 
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s updateRideThru failed, sp %d, status: 0x%x\n",
                                  __FUNCTION__, 
                                  ((batteryCommand.batteryAction == FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH) ? "Enable" : "Disable"),
                                  batteryCommand.device_location.sp,
                                  status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)sps_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s updateRideThru successful, sp %d\n",
                                  __FUNCTION__, 
                                  ((batteryCommand.batteryAction == FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH) ? "Enable" : "Disable"),
                                  batteryCommand.device_location.sp);
        }
    }

    return status;

}   // end of fbe_sps_mgmt_verifyRideThroughTimeout


// End of file fbe_sps_mgmt_monitor.c


