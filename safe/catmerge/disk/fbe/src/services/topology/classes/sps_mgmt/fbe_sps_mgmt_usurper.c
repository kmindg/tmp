/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sps_mgmt_usurper.c
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
 *   23-Feb-2010:  Created. Joe Perry
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_sps_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_esp_sps_mgmt.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe_base_environment_debug.h"


static fbe_status_t fbe_sps_mgmt_getSpsCount(fbe_sps_mgmt_t *spsMgmtPtr, 
                                             fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getSpsStatus(fbe_sps_mgmt_t *spsMgmtPtr, 
                                              fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getBobCount(fbe_sps_mgmt_t *spsMgmtPtr, 
                                             fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getBobStatus(fbe_sps_mgmt_t *spsMgmtPtr, 
                                              fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getObjectData(fbe_sps_mgmt_t *spsMgmtPtr, 
                                               fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getSpsTestTime(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_setSpsTestTime(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getSpsInputPower(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                  fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getSpsInputPowerTotal(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                       fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getSpsManufInfo(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                 fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getCacheStatus(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_powerdownSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_verifyPowerdownSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                    fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getSimulateSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_setSimulateSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getSpsInputPowerSample(fbe_sps_mgmt_t *spsMgmtPtr,
                                                  fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_getLocalBatteryTime(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                     fbe_packet_t * packet);
static fbe_status_t fbe_sps_mgmt_initiateSelfTest(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                  fbe_packet_t * packet);
void fbe_sps_mgmt_format_rev_for_admin(fbe_sps_mgmt_t * pSpsMgmt,fbe_u8_t *pRevToFormat, fbe_u8_t sizeLimit);

/*!***************************************************************
 * fbe_sps_mgmt_control_entry()
 ****************************************************************
 * @brief
 *  This function is entry point for control operation for this
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_sps_mgmt_t                          *spsMgmtPtr = NULL;
    
    spsMgmtPtr = (fbe_sps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, control_code 0x%x\n", 
                          __FUNCTION__, control_code);
    switch(control_code) 
    {
    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_COUNT:
        status = fbe_sps_mgmt_getSpsCount(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_STATUS:
        status = fbe_sps_mgmt_getSpsStatus(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_TEST_TIME:
        status = fbe_sps_mgmt_getSpsTestTime(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_TEST_TIME:
        status = fbe_sps_mgmt_setSpsTestTime(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER:
        status = fbe_sps_mgmt_getSpsInputPower(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER_TOTAL:
        status = fbe_sps_mgmt_getSpsInputPowerTotal(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_MANUF_INFO:
        status = fbe_sps_mgmt_getSpsManufInfo(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_CACHE_STATUS:
        status = fbe_sps_mgmt_getCacheStatus(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_SPS_POWERDOWN:
        status = fbe_sps_mgmt_powerdownSps(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_SPS_VERIFY_POWERDOWN:
        status = fbe_sps_mgmt_verifyPowerdownSps(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SIMULATE_SPS:
        status = fbe_sps_mgmt_getSimulateSps(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SIMULATE_SPS:
        status = fbe_sps_mgmt_setSimulateSps(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_BOB_COUNT:
        status = fbe_sps_mgmt_getBobCount(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_BOB_STATUS:
        status = fbe_sps_mgmt_getBobStatus(spsMgmtPtr, packet);
        break;

   case FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_INPUT_POWER_SAMPLE:
        status = fbe_sps_mgmt_getSpsInputPowerSample(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_LOCAL_BATTERY_TIME:
         status = fbe_sps_mgmt_getLocalBatteryTime(spsMgmtPtr, packet);
         break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_INITIATE_SELF_TEST:
        status = fbe_sps_mgmt_initiateSelfTest(spsMgmtPtr, packet);
        break;

    case FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_OBJECT_DATA:
        status = fbe_sps_mgmt_getObjectData(spsMgmtPtr, packet);
        break;

    default:
        status =  fbe_base_environment_control_entry (object_handle, packet);
        break;
    }

    return status;

}
/******************************************
 * end fbe_sps_mgmt_control_entry()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_getSpsCount()
 ****************************************************************
 * @brief
 *  This function processes the GetSpsCount Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Feb-2012 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSpsCount(fbe_sps_mgmt_t *spsMgmtPtr, 
                                             fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_spsCounts_t        *spsCountInfo = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsCountInfo);
    if (spsCountInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_spsCounts_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (spsMgmtPtr->total_sps_count == 0)
    {
        spsCountInfo->totalNumberOfSps = 
            spsCountInfo->enclosuresWithSps = 
            spsCountInfo->spsPerEnclosure = 0;
    }
    else
    {
        spsCountInfo->totalNumberOfSps = spsMgmtPtr->total_sps_count;
        spsCountInfo->enclosuresWithSps = spsMgmtPtr->encls_with_sps;
        spsCountInfo->spsPerEnclosure = spsMgmtPtr->total_sps_count / spsMgmtPtr->encls_with_sps;
    }

    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, totalNum %d, spsEncls %d, spsPerEncl %d\n", 
                          __FUNCTION__,
                          spsCountInfo->totalNumberOfSps,
                          spsCountInfo->enclosuresWithSps,
                          spsCountInfo->spsPerEnclosure);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getSpsCount()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_getSpsStatus()
 ****************************************************************
 * @brief
 *  This function processes the GetSpsStatus Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSpsStatus(fbe_sps_mgmt_t *spsMgmtPtr, 
                                              fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_spsStatus_t        *spsStatusInfoPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_sps_info_t                      *requestSpsInfoPtr;
    fbe_u32_t                           spsEnclIndex, spsIndex;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsStatusInfoPtr);
    if (spsStatusInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_spsStatus_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // validate & convert SPS location info
    status = fbe_sps_mgmt_convertSpsLocation(spsMgmtPtr, &spsStatusInfoPtr->spsLocation, &spsEnclIndex, &spsIndex);
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, bus 0x%x, encl 0x%x, slot %d, spsEnclIndex %d, spsIndex %d\n",
                              __FUNCTION__,
                              spsStatusInfoPtr->spsLocation.bus,
                              spsStatusInfoPtr->spsLocation.enclosure,
                              spsStatusInfoPtr->spsLocation.slot,
                              spsEnclIndex, spsIndex);
        requestSpsInfoPtr = 
            &spsMgmtPtr->arraySpsInfo->sps_info[spsEnclIndex][spsIndex];
        spsStatusInfoPtr->spsModuleInserted = requestSpsInfoPtr->spsModulePresent;
        spsStatusInfoPtr->spsBatteryInserted = requestSpsInfoPtr->spsBatteryPresent;
        spsStatusInfoPtr->spsStatus = requestSpsInfoPtr->spsStatus;
        spsStatusInfoPtr->spsFaultInfo = requestSpsInfoPtr->spsFaultInfo;
        spsStatusInfoPtr->spsModuleFfid = requestSpsInfoPtr->spsModuleFfid;
        spsStatusInfoPtr->spsBatteryFfid = requestSpsInfoPtr->spsBatteryFfid;
        spsStatusInfoPtr->spsModel = requestSpsInfoPtr->spsModel;
        spsStatusInfoPtr->dualComponentSps = requestSpsInfoPtr->dualComponentSps;
        spsStatusInfoPtr->spsCablingStatus = requestSpsInfoPtr->spsCablingStatus;

        fbe_copy_memory(&spsStatusInfoPtr->primaryFirmwareRev[0],
                        &requestSpsInfoPtr->primaryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        fbe_copy_memory(&spsStatusInfoPtr->secondaryFirmwareRev[0],
                        &requestSpsInfoPtr->secondaryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        fbe_copy_memory(&spsStatusInfoPtr->batteryFirmwareRev[0],
                        &requestSpsInfoPtr->batteryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, %s spsStatus %s, faults %s, cabling %s\n", 
                               __FUNCTION__, 
                               spsMgmtPtr->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsNameString,
                               fbe_sps_mgmt_getSpsStatusString(spsStatusInfoPtr->spsStatus), 
                               fbe_sps_mgmt_getSpsFaultString(&spsStatusInfoPtr->spsFaultInfo), 
                               fbe_sps_mgmt_getSpsCablingStatusString(spsStatusInfoPtr->spsCablingStatus));
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Invalid location, bus 0x%x, encl 0x%x, slot %d\n",
                              __FUNCTION__,
                              spsStatusInfoPtr->spsLocation.bus,
                              spsStatusInfoPtr->spsLocation.enclosure,
                              spsStatusInfoPtr->spsLocation.slot);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#if FALSE
    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                          "%s, spsStatusInfoPtr->spsIndex %d\n",
                          __FUNCTION__, spsEnclIndex);
    requestSpsInfoPtr = 
        &spsMgmtPtr->arraySpsInfo->sps_info[spsEnclIndex][spsStatusInfoPtr->spsLocation.slot];
    spsStatusInfoPtr->spsModuleInserted = requestSpsInfoPtr->spsModulePresent;
    spsStatusInfoPtr->spsBatteryInserted = requestSpsInfoPtr->spsBatteryPresent;
    spsStatusInfoPtr->spsStatus = requestSpsInfoPtr->spsStatus;
    spsStatusInfoPtr->spsFaultInfo = requestSpsInfoPtr->spsFaultInfo;
    spsStatusInfoPtr->spsCablingStatus = requestSpsInfoPtr->spsCablingStatus;
    spsStatusInfoPtr->spsModuleFfid = requestSpsInfoPtr->spsModuleFfid;
    spsStatusInfoPtr->spsBatteryFfid = requestSpsInfoPtr->spsBatteryFfid;
    spsStatusInfoPtr->spsModel = requestSpsInfoPtr->spsModel;
     fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                           "%s, %s spsStatus %s, faults %s, cabling %s\n", 
                           __FUNCTION__, 
                           spsMgmtPtr->arraySpsInfo->sps_info[spsEnclIndex][spsStatusInfoPtr->spsLocation.slot].spsNameString,
                           fbe_sps_mgmt_getSpsStatusString(spsStatusInfoPtr->spsStatus), 
                           fbe_sps_mgmt_getSpsFaultString(&spsStatusInfoPtr->spsFaultInfo), 
                           fbe_sps_mgmt_getSpsCablingStatusString(spsStatusInfoPtr->spsCablingStatus));
#endif

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getSpsStatus()
 ******************************************/


/*!***************************************************************
 * fbe_sps_mgmt_getBobCount()
 ****************************************************************
 * @brief
 *  This function processes the GetBobCount Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  20-Apr-2012 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getBobCount(fbe_sps_mgmt_t *spsMgmtPtr, 
                                             fbe_packet_t * packet)
{
    fbe_u32_t                           *bobCountPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &bobCountPtr);
    if (bobCountPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *bobCountPtr = spsMgmtPtr->total_bob_count;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getBobCount()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_getBobStatus()
 ****************************************************************
 * @brief
 *  This function processes the GetBobStatus Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  20-Apr-2012 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getBobStatus(fbe_sps_mgmt_t *spsMgmtPtr, 
                                              fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_bobStatus_t        *bobStatusInfoPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &bobStatusInfoPtr);
    if (bobStatusInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_bobStatus_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid length %d\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get current BoB status based on index
    if (bobStatusInfoPtr->bobIndex < spsMgmtPtr->total_bob_count)
    {
        bobStatusInfoPtr->bobInfo = spsMgmtPtr->arrayBobInfo->bob_info[bobStatusInfoPtr->bobIndex];
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                               "%s, index %d, bbuInserted %d, state %d\n", 
                               __FUNCTION__, 
                              bobStatusInfoPtr->bobIndex,
                              bobStatusInfoPtr->bobInfo.batteryInserted,
                              bobStatusInfoPtr->bobInfo.batteryState);
        // set environmental status
        if (spsMgmtPtr->arrayBobInfo->bob_info[bobStatusInfoPtr->bobIndex].envInterfaceStatus ==  
            FBE_ENV_INTERFACE_STATUS_DATA_STALE)
        {
            bobStatusInfoPtr->dataStale = TRUE;
            bobStatusInfoPtr->environmentalInterfaceFault = FALSE;
        }
        else if (spsMgmtPtr->arrayBobInfo->bob_info[bobStatusInfoPtr->bobIndex].envInterfaceStatus == 
                 FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            bobStatusInfoPtr->dataStale = FALSE;
            bobStatusInfoPtr->environmentalInterfaceFault = TRUE;
        }
        else
        {
            bobStatusInfoPtr->dataStale = FALSE;
            bobStatusInfoPtr->environmentalInterfaceFault = FALSE;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, bbuIndex %d invalid\n", 
                              __FUNCTION__, bobStatusInfoPtr->bobIndex);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getBobStatus()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_getObjectData()
 ****************************************************************
 * @brief
 *  This function processes the GetObjectData Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  19-Apr-2013 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getObjectData(fbe_sps_mgmt_t *spsMgmtPtr, 
                                               fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_objectData_t       *objectDataPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t                           spsEnclIndex, bbuIndex;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &objectDataPtr);
    if (objectDataPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_objectData_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid length %d\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    objectDataPtr->testingState = spsMgmtPtr->testingState;
    objectDataPtr->testStartTime = spsMgmtPtr->testStartTime;
    // get private object data based on whether there are SPS's or BBU's
    if (spsMgmtPtr->total_sps_count > 0)
    {
        objectDataPtr->privateObjectData.privateSpsData.needToTest =
            spsMgmtPtr->arraySpsInfo->needToTest;
        objectDataPtr->privateObjectData.privateSpsData.testingCompleted =
            spsMgmtPtr->arraySpsInfo->testingCompleted;
        objectDataPtr->privateObjectData.privateSpsData.spsTestType =
            spsMgmtPtr->arraySpsInfo->spsTestType;
        objectDataPtr->privateObjectData.privateSpsData.spsTestStartTime =
            spsMgmtPtr->arraySpsInfo->spsTestStartTime;
        objectDataPtr->privateObjectData.privateSpsData.spsTestTimeDetected =
            spsMgmtPtr->arraySpsInfo->spsTestTimeDetected;
        objectDataPtr->privateObjectData.privateSpsData.simulateSpsInfo =
            spsMgmtPtr->arraySpsInfo->simulateSpsInfo;
        objectDataPtr->privateObjectData.privateSpsData.spsBatteryOnlineCount =
            spsMgmtPtr->arraySpsInfo->spsBatteryOnlineCount;
        for (spsEnclIndex = 0; spsEnclIndex < spsMgmtPtr->encls_with_sps; spsEnclIndex++)
        {
            objectDataPtr->privateObjectData.privateSpsData.mfgInfoNeeded[spsEnclIndex] =
                spsMgmtPtr->arraySpsInfo->mfgInfoNeeded[spsEnclIndex];
            objectDataPtr->privateObjectData.privateSpsData.debouncedSpsStatus[spsEnclIndex] =
                spsMgmtPtr->arraySpsInfo->debouncedSpsStatus[spsEnclIndex];
            objectDataPtr->privateObjectData.privateSpsData.debouncedSpsStatusCount[spsEnclIndex] =
                spsMgmtPtr->arraySpsInfo->debouncedSpsStatusCount[spsEnclIndex];
            objectDataPtr->privateObjectData.privateSpsData.debouncedSpsFaultInfo[spsEnclIndex] =
                spsMgmtPtr->arraySpsInfo->debouncedSpsFaultInfo[spsEnclIndex];
            objectDataPtr->privateObjectData.privateSpsData.debouncedSpsFaultsCount[spsEnclIndex] =
                spsMgmtPtr->arraySpsInfo->debouncedSpsFaultsCount[spsEnclIndex];
        }
    }
    else if (spsMgmtPtr->total_bob_count > 0)
    {
        objectDataPtr->privateObjectData.privateBbuData.needToTest =
            spsMgmtPtr->arrayBobInfo->needToTest;
        objectDataPtr->privateObjectData.privateBbuData.testingCompleted =
            spsMgmtPtr->arrayBobInfo->testingCompleted;
        objectDataPtr->privateObjectData.privateBbuData.simulateBobInfo =
            spsMgmtPtr->arrayBobInfo->simulateBobInfo;
        for (bbuIndex = 0; bbuIndex < spsMgmtPtr->total_bob_count; bbuIndex++)
        {
            objectDataPtr->privateObjectData.privateBbuData.needToTestPerBbu[bbuIndex] =
                spsMgmtPtr->arrayBobInfo->bobNeedsTesting[bbuIndex];
            objectDataPtr->privateObjectData.privateBbuData.notBatteryReadyTimeStart[bbuIndex] =
                spsMgmtPtr->arrayBobInfo->notBatteryReadyTimeStart[bbuIndex];
            objectDataPtr->privateObjectData.privateBbuData.debouncedBobOnBatteryState[bbuIndex] =
                spsMgmtPtr->arrayBobInfo->debouncedBobOnBatteryState[bbuIndex];
            objectDataPtr->privateObjectData.privateBbuData.debouncedBobOnBatteryStateCount[bbuIndex] =
                spsMgmtPtr->arrayBobInfo->debouncedBobOnBatteryStateCount[bbuIndex];
        }
    }
    else
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getObjectData()
 ******************************************/


/*!***************************************************************
 * fbe_sps_mgmt_getSpsTestTime()
 ****************************************************************
 * @brief
 *  This function processes the GetSpsTestTime Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  18-Jun-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSpsTestTime(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_spsTestTime_t        *spsTestTimePtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_sps_bbu_test_info_t             *persistentTestTime;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsTestTimePtr);
    if (spsTestTimePtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_spsTestTime_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get current SPS status
    persistentTestTime = (fbe_sps_bbu_test_info_t *)spsMgmtPtr->base_environment.my_persistent_data;
    if (persistentTestTime == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Persistent Data pointer not initialized\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    spsTestTimePtr->spsTestTime = persistentTestTime->testStartTime;
    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,get SPS Test Time: weekday %d, hour %d, min %d\n",
                              __FUNCTION__, 
                              persistentTestTime->testStartTime.weekday, 
                              persistentTestTime->testStartTime.hour, 
                              persistentTestTime->testStartTime.minute);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getSpsTestTime()
 ******************************************/


/*!***************************************************************
 * fbe_sps_mgmt_setSpsTestTime()
 ****************************************************************
 * @brief
 *  This function processes the SetSpsTestTime Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  18-Jun-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_setSpsTestTime(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_spsTestTime_t        *spsTestTimePtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_sps_bbu_test_info_t     *persistent_data_p;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsTestTimePtr);
    if (spsTestTimePtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_spsTestTime_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // set SPS Test Time & reschedule timer
    persistent_data_p = (fbe_sps_bbu_test_info_t *) ((fbe_base_environment_t *)spsMgmtPtr)->my_persistent_data;
    if (persistent_data_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Persistent Data pointer not initialized\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    persistent_data_p->testStartTime = spsTestTimePtr->spsTestTime;

    status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)spsMgmtPtr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s persistent write error, status: 0x%X",
                              __FUNCTION__, status);
    }
    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,updated SPS Test Time: weekday %d, hour %d, min %d\n",
                              __FUNCTION__, 
                              persistent_data_p->testStartTime.weekday, 
                              persistent_data_p->testStartTime.hour, 
                              persistent_data_p->testStartTime.minute);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;

}
/******************************************
 * end fbe_sps_mgmt_setSpsTestTime()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_getSpsInputPower()
 ****************************************************************
 * @brief
 *  This function processes the GetSpsInputPower Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  18-Jun-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSpsInputPower(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                  fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_spsInputPower_t    *spsInputPowerPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t                           spsEnclIndex, spsIndex;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsInputPowerPtr);
    if (spsInputPowerPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_spsInputPower_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get SPS Input Power info
//    fbe_set_memory(spsInputPowerPtr, 0, sizeof(fbe_esp_sps_mgmt_spsInputPower_t));
    // validate & convert SPS location info
    status = fbe_sps_mgmt_convertSpsLocation(spsMgmtPtr, &spsInputPowerPtr->spsLocation, &spsEnclIndex, &spsIndex);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, Invalid location, bus 0x%x, encl 0x%x, slot %d\n",
                              __FUNCTION__,
                              spsInputPowerPtr->spsLocation.bus,
                              spsInputPowerPtr->spsLocation.enclosure,
                              spsInputPowerPtr->spsLocation.slot);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                           "%s, spsEnclIndex %d, spsIndex %d, sps_mgmtInputPower: curr %d, max %d, ave %d\n", 
                           __FUNCTION__, spsEnclIndex, spsIndex,
                          spsMgmtPtr->spsEirData->spsCurrentInputPower[spsEnclIndex][spsIndex].inputPower,
                          spsMgmtPtr->spsEirData->spsAverageInputPower[spsEnclIndex][spsIndex].inputPower,
                          spsMgmtPtr->spsEirData->spsMaxInputPower[spsEnclIndex][spsIndex].inputPower);
    if ((spsMgmtPtr->spsEirData->spsCurrentInputPower[spsEnclIndex][spsIndex].inputPower != 0) &&
        (spsMgmtPtr->spsEirData->spsCurrentInputPower[spsEnclIndex][spsIndex].status == FBE_ENERGY_STAT_VALID))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, spsEnclIndex %d, spsIndex %d has valid EIR data\n", 
                              __FUNCTION__, spsEnclIndex, spsIndex);
    }

    spsInputPowerPtr->spsCurrentInputPower = 
        spsMgmtPtr->spsEirData->spsCurrentInputPower[spsEnclIndex][spsIndex];
    spsInputPowerPtr->spsAverageInputPower = 
        spsMgmtPtr->spsEirData->spsAverageInputPower[spsEnclIndex][spsIndex];
    spsInputPowerPtr->spsMaxInputPower = 
        spsMgmtPtr->spsEirData->spsMaxInputPower[spsEnclIndex][spsIndex];

    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, spsEnclIndex %d, spsIndex %d, InputPower: curr %d, max %d, ave %d\n", 
                           __FUNCTION__, spsEnclIndex, spsIndex,
                           spsInputPowerPtr->spsCurrentInputPower.inputPower,
                           spsInputPowerPtr->spsMaxInputPower.inputPower,
                           spsInputPowerPtr->spsAverageInputPower.inputPower);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getSpsInputPower()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_getSpsInputPowerTotal()
 ****************************************************************
 * @brief
 *  This function processes the GetSpsInputPower Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  18-Jun-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSpsInputPowerTotal(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                       fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_spsInputPowerTotal_t    *spsInputPowerPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_u32_t                           spsIndex, enclIndex;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsInputPowerPtr);
    if (spsInputPowerPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_sps_mgmt_spsInputPowerTotal_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get SPS Input Power info
    spsInputPowerPtr->spsCount = spsMgmtPtr->total_sps_count;
    spsInputPowerPtr->inputPowerAll.inputPower = 0;
    spsInputPowerPtr->inputPowerAll.status = 0;
    if (spsMgmtPtr->total_sps_count > 0)
    {
        for (enclIndex = 0; enclIndex < spsMgmtPtr->encls_with_sps; enclIndex++)
        {
            for (spsIndex = 0; spsIndex < FBE_SPS_MAX_COUNT; spsIndex++)
            {
                spsInputPowerPtr->inputPowerAll.inputPower +=
                    spsMgmtPtr->spsEirData->spsCurrentInputPower[enclIndex][spsIndex].inputPower;
                spsInputPowerPtr->inputPowerAll.status |=
                    spsMgmtPtr->spsEirData->spsCurrentInputPower[enclIndex][spsIndex].status;
            }
        }
    }

    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, spsCount, %d, spsTotalInputPower %d, status 0x%x\n", 
                           __FUNCTION__, 
                           spsInputPowerPtr->spsCount,
                           spsInputPowerPtr->inputPowerAll.inputPower,
                           spsInputPowerPtr->inputPowerAll.status);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getSpsInputPowerTotal()
 ******************************************/

/*!***************************************************************
 * @fn fbe_sps_mgmt_getSpsManufInfo()
 ****************************************************************
 * @brief
 *  This function processes the GetManufInfo Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  04-Aug-2010 - Created. Vishnu Sharma
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSpsManufInfo(fbe_sps_mgmt_t *spsMgmtPtr, 
                                              fbe_packet_t * packet)
{
    fbe_esp_sps_mgmt_spsManufInfo_t         *spsManufInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_u32_t                               spsEnclIndex, spsIndex;
    fbe_status_t                            status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &spsManufInfoPtr);
    if (spsManufInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length); 
    if (length != sizeof(fbe_esp_sps_mgmt_spsManufInfo_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get  SPS manufacturer info  based on index
    // validate & convert SPS location info
    status = fbe_sps_mgmt_convertSpsLocation(spsMgmtPtr, &spsManufInfoPtr->spsLocation, &spsEnclIndex, &spsIndex);
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                               "%s, spsEnclIndex %d, spsIndex %d\n",
                               __FUNCTION__, spsEnclIndex, spsIndex);
        spsManufInfoPtr->spsManufInfo = 
            spsMgmtPtr->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsManufInfo;
         fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                               "%s, %s spsSerialNumber %s, spsPartNumber %s, spsModelString %s,spsVendor %s\n", 
                               __FUNCTION__, 
                               spsMgmtPtr->arraySpsInfo->sps_info[spsEnclIndex][spsIndex].spsNameString,
                               spsManufInfoPtr->spsManufInfo.spsModuleManufInfo.spsSerialNumber,
                               spsManufInfoPtr->spsManufInfo.spsModuleManufInfo.spsPartNumber,
                               spsManufInfoPtr->spsManufInfo.spsModuleManufInfo.spsModelString,
                               spsManufInfoPtr->spsManufInfo.spsModuleManufInfo.spsVendor);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, bus 0x%x, encl 0x%x, slot %d invalid\n", 
                              __FUNCTION__, 
                              spsManufInfoPtr->spsLocation.bus,
                              spsManufInfoPtr->spsLocation.enclosure,
                              spsManufInfoPtr->spsLocation.slot);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sps_mgmt_format_rev_for_admin(spsMgmtPtr,&spsManufInfoPtr->spsManufInfo.spsModuleManufInfo.spsFirmwareRevision[0],FBE_SPS_FW_REVISION_SIZE );
    fbe_sps_mgmt_format_rev_for_admin(spsMgmtPtr,&spsManufInfoPtr->spsManufInfo.spsModuleManufInfo.spsSecondaryFirmwareRevision[0],FBE_SPS_FW_REVISION_SIZE );
    fbe_sps_mgmt_format_rev_for_admin(spsMgmtPtr,&spsManufInfoPtr->spsManufInfo.spsBatteryManufInfo.spsFirmwareRevision[0],FBE_SPS_FW_REVISION_SIZE );

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getSpsManufInfo()
 ******************************************/

/*!***************************************************************
 * @fn fbe_sps_mgmt_getCacheStatus()
 ****************************************************************
 * @brief
 *  This function determines & returns the SPS Caching status.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Mar-2011 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getCacheStatus(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet)
{
    fbe_common_cache_status_t               *spsCacheStatusPtr = NULL;
    fbe_common_cache_status_t               spsPeerCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &spsCacheStatusPtr);
    if (spsCacheStatusPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length); 
    if (length != sizeof(fbe_common_cache_status_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // determine SPS Caching status (need at least one good SPS)
    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *) spsMgmtPtr,
                                                      &spsPeerCacheStatus);

    *spsCacheStatusPtr = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *) spsMgmtPtr,
                                                                  spsMgmtPtr->arraySpsInfo->spsCacheStatus,
                                                                  spsPeerCacheStatus,
                                                                  FBE_CLASS_ID_SPS_MGMT);

    fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, spsCacheStatusPtr %d\n",
                          __FUNCTION__, *spsCacheStatusPtr);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getCacheStatus()
 ******************************************/


/*!***************************************************************
 * @fn fbe_sps_mgmt_powerdownSps()
 ****************************************************************
 * @brief
 *  This function determines whether the SPS needs to be shutdown.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  07-Apr-2011 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_powerdownSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_status_t                            status;
    fbe_base_board_mgmt_battery_command_t   batteryCommand;
    fbe_u32_t                               bobIndex;
    fbe_device_physical_location_t          location;
    fbe_u8_t                                deviceStr[FBE_DEVICE_STRING_LENGTH];

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer_length(control_operation, &length); 
    if (length != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* 
     * determine if SPS needs to be shutdown
     */
    if (((spsMgmtPtr->encls_with_sps == 1) && 
            (spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsStatus == SPS_STATE_ON_BATTERY)) ||
        ((spsMgmtPtr->encls_with_sps == 2) &&
            (spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsStatus == SPS_STATE_ON_BATTERY) &&
            (spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_DAE0][FBE_LOCAL_SPS].spsStatus == SPS_STATE_ON_BATTERY)) ||
        ((spsMgmtPtr->total_bob_count == 2) && (spsMgmtPtr->arrayBobInfo->bob_info[spsMgmtPtr->base_environment.spid].batteryChargeState == BATTERY_DISCHARGING)) &&
        (spsMgmtPtr->arraySpsInfo->spsCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN))
    {
        // send message to Phy Pkg to shut down SPS
        if (spsMgmtPtr->total_sps_count > 0)
        {
            status = fbe_sps_mgmt_sendSpsCommand(spsMgmtPtr, FBE_SPS_ACTION_SHUTDOWN, FBE_SPS_MGMT_ALL);
            if (status == FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, shutdown sent to local SPS\n",
                                      __FUNCTION__);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_TRANSACTION_FAILED,
                                      "%s, SPS shutdown failed, status %d\n",
                                      __FUNCTION__, status);
                fbe_transport_set_status(packet, FBE_STATUS_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_FAILED;
            }
        }

        // init location for use below
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));

        // send message to Phy Pkg to shut down BOB
        if (spsMgmtPtr->total_bob_count> 0)
        {
             // shutdown peer BOB first, then local BOB
            for (bobIndex=0; bobIndex<spsMgmtPtr->total_bob_count; bobIndex++)
            {
                batteryCommand.batteryAction = FBE_BATTERY_ACTION_SHUTDOWN;
                if (spsMgmtPtr->arrayBobInfo->bob_info[bobIndex].associatedSp != spsMgmtPtr->base_environment.spid)
                {

                    // set location to the peer SP and the peer slot
                    batteryCommand.device_location.sp = spsMgmtPtr->arrayBobInfo->bob_info[bobIndex].associatedSp;
                    batteryCommand.device_location.slot= spsMgmtPtr->arrayBobInfo->bob_info[bobIndex].slotNumOnSpBlade;
                    // set location for device string
                    location.sp = batteryCommand.device_location.sp;
                    location.slot = batteryCommand.device_location.slot;
                    fbe_zero_memory(&deviceStr, FBE_DEVICE_STRING_LENGTH);
                    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);

                    status = fbe_api_board_send_battery_command(spsMgmtPtr->arrayBobInfo->bobObjectId, &batteryCommand);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s Error shutting down %s, status: 0x%X\n",
                                              __FUNCTION__, 
                                              &deviceStr[0],
                                              status);
                    }
                    else
                    {
                        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, Successfully sent shutdown command to %s.\n",
                                              __FUNCTION__,
                                              &deviceStr[0]);
                    }
                }
            }

            // delay disabling local BBU so peer BBU can finish disabling.
            fbe_api_sleep(500);

            for (bobIndex=0; bobIndex<spsMgmtPtr->total_bob_count; bobIndex++)
            {
                batteryCommand.batteryAction = FBE_BATTERY_ACTION_SHUTDOWN;
                if (spsMgmtPtr->arrayBobInfo->bob_info[bobIndex].associatedSp == spsMgmtPtr->base_environment.spid)
                {
                    batteryCommand.device_location.sp = spsMgmtPtr->arrayBobInfo->bob_info[bobIndex].associatedSp;
                    batteryCommand.device_location.slot= spsMgmtPtr->arrayBobInfo->bob_info[bobIndex].slotNumOnSpBlade;
                    // set location for device string
                    location.sp = batteryCommand.device_location.sp;
                    location.slot = batteryCommand.device_location.slot;
                    fbe_zero_memory(&deviceStr, FBE_DEVICE_STRING_LENGTH);
                    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               &location, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);

                    status = fbe_api_board_send_battery_command(spsMgmtPtr->arrayBobInfo->bobObjectId, &batteryCommand);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s Error shutting down %s, status: 0x%X\n",
                                              __FUNCTION__, 
                                              &deviceStr[0],
                                              status);
                        fbe_transport_set_status(packet, FBE_STATUS_FAILED, 0);
                        fbe_transport_complete_packet(packet);
                        return FBE_STATUS_FAILED;
                    }
                    else
                    {
                        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, Successfully sent shutdown command to %s.\n",
                                              __FUNCTION__,
                                              &deviceStr[0]);
                    }
                }
            }
        }
    }
    else
    {
        if (spsMgmtPtr->encls_with_sps == 1)
        {
            fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, No Shutdown, SPS PE %s\n",
                                  __FUNCTION__,
                                  fbe_sps_mgmt_getSpsStatusString(
                                      spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsStatus));
        }
        else if (spsMgmtPtr->encls_with_sps == 2)
        {
            fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, No Shutdown, SPS PE %s, DAE0 %s\n",
                                  __FUNCTION__,
                                  fbe_sps_mgmt_getSpsStatusString(
                                      spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsStatus),
                                  fbe_sps_mgmt_getSpsStatusString(
                                      spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_DAE0][FBE_LOCAL_SPS].spsStatus));
        }
        else if (spsMgmtPtr->total_bob_count == 2)
        {
#if 1
            fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, No Shutdown, BBU[0]:%s, BBU[1]:%s\n",
                                  __FUNCTION__,
                                  fbe_sps_mgmt_getBobStateString(spsMgmtPtr->arrayBobInfo->bob_info[0].batteryState),
                                  fbe_sps_mgmt_getBobStateString(spsMgmtPtr->arrayBobInfo->bob_info[1].batteryState));
#else
            if (spsMgmtPtr->arrayBobInfo->bob_info[bobIndex].associatedSp == SP_A)
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, No Shutdown, SPA BBU0 %s, SPB BBU0 %s.\n",
                                  __FUNCTION__,
                                  fbe_sps_mgmt_getBobStateString(spsMgmtPtr->arrayBobInfo->bob_info[0].batteryState),
                                  fbe_sps_mgmt_getBobStateString(spsMgmtPtr->arrayBobInfo->bob_info[1].batteryState));
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, No Shutdown, SPA BBU0 %s, SPB BBU0 %s.\n",
                                  __FUNCTION__,
                                  fbe_sps_mgmt_getBobStateString(spsMgmtPtr->arrayBobInfo->bob_info[1].batteryState),
                                  fbe_sps_mgmt_getBobStateString(spsMgmtPtr->arrayBobInfo->bob_info[0].batteryState));
            }
#endif
        }
        fbe_transport_set_status(packet, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_FAILED;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_powerdownSps()
 ******************************************/

/*!***************************************************************
 * @fn fbe_sps_mgmt_verifyPowerdownSps()
 ****************************************************************
 * @brief
 *  This function verifies that the SPS has been shutdown.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  27-Aug-2012 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_verifyPowerdownSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                    fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_status_t                            status;
    fbe_u32_t                               spsEnclIndex;
    fbe_u32_t                               bbuIndex;
    fbe_bool_t                              spsNotShutdown = FALSE;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer_length(control_operation, &length); 
    if (length != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* 
     * check if SPS's/BBU's are shutdown
     */
    // check if all local SPS's are shutdown (not present)
    if (spsMgmtPtr->total_sps_count > 0)
    {
    
        for (spsEnclIndex = 0; spsEnclIndex < spsMgmtPtr->encls_with_sps; spsEnclIndex++)
        {
    
            if (spsMgmtPtr->arraySpsInfo->sps_info[spsEnclIndex][FBE_LOCAL_SPS].spsModulePresent)
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                      "%s, spsEnclIndex %d is not shutdown\n", 
                                      __FUNCTION__, spsEnclIndex);
                spsNotShutdown = TRUE;
                break;
            }
        }
    }
    // check if all BBU's are shutdown
    else if (spsMgmtPtr->total_bob_count > 0)
    {
        for (bbuIndex = 0; bbuIndex < spsMgmtPtr->total_bob_count; bbuIndex++)
        {
            fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, bbuIndex %d, ins %d, enabled %d, state %s\n", 
                                  __FUNCTION__, bbuIndex,
                                  spsMgmtPtr->arrayBobInfo->bob_info[bbuIndex].batteryInserted,
                                  spsMgmtPtr->arrayBobInfo->bob_info[bbuIndex].batteryEnabled,
                                  fbe_sps_mgmt_getBobStateString(spsMgmtPtr->arrayBobInfo->bob_info[bbuIndex].batteryState));
            if (spsMgmtPtr->arrayBobInfo->bob_info[bbuIndex].batteryEnabled ||
                (spsMgmtPtr->arrayBobInfo->bob_info[bbuIndex].batteryState != FBE_BATTERY_STATUS_REMOVED))
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                      "%s, bbuIndex %d is not shutdown\n", 
                                      __FUNCTION__, bbuIndex);
                spsNotShutdown = TRUE;
            }
        }
    }

    if (spsNotShutdown)
    {
        fbe_transport_set_status(packet, FBE_STATUS_FAILED, 0);
        status = FBE_STATUS_FAILED;
    }
    else
    {
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        status = FBE_STATUS_OK;
    }

    fbe_transport_complete_packet(packet);
    return status;

}
/******************************************
 * end fbe_sps_mgmt_verifyPowerdownSps()
 ******************************************/


/*!***************************************************************
 * @fn fbe_sps_mgmt_getSimulateSps()
 ****************************************************************
 * @brief
 *  This function returns the SimulatedSps status.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Nov-2011 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSimulateSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet)
{
    fbe_bool_t                              *simulateSpsPtr = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &simulateSpsPtr);
    if (simulateSpsPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length); 
    if (length != sizeof(fbe_bool_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (spsMgmtPtr->total_sps_count > 0)
    {
        *simulateSpsPtr = spsMgmtPtr->arraySpsInfo->simulateSpsInfo;
    }
    else if (spsMgmtPtr->total_bob_count > 0)
    {
        *simulateSpsPtr = spsMgmtPtr->arrayBobInfo->simulateBobInfo;
    }
    else
    {
        *simulateSpsPtr = FALSE;
    }


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_sps_mgmt_getSimulateSps()
 ******************************************/


/*!***************************************************************
 * @fn fbe_sps_mgmt_setSimulateSps()
 ****************************************************************
 * @brief
 *  This function returns the SimulatedSps status.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Nov-2011 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_setSimulateSps(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                fbe_packet_t * packet)
{
    fbe_bool_t                              *simulateSpsPtr = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_device_physical_location_t          location;
    fbe_u32_t                               bobIndex;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &simulateSpsPtr);
    if (simulateSpsPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length); 
    if (length != sizeof(fbe_bool_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((spsMgmtPtr->total_sps_count > 0) &&
        (spsMgmtPtr->arraySpsInfo->simulateSpsInfo != *simulateSpsPtr))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, simulateSps changed from %d to %d\n", 
                              __FUNCTION__, spsMgmtPtr->arraySpsInfo->simulateSpsInfo, *simulateSpsPtr);
        spsMgmtPtr->arraySpsInfo->simulateSpsInfo = *simulateSpsPtr;
        // need to force SPS status change
        status = fbe_sps_mgmt_processSpsStatus(spsMgmtPtr, FBE_SPS_MGMT_PE, FBE_LOCAL_SPS);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, processSpsStatus failed, status 0x%x \n", 
                                  __FUNCTION__, status);
        }
        // check if second SPS configured & need to force SPS status change
        if (spsMgmtPtr->total_sps_count > 2)
        {
            status = fbe_sps_mgmt_processSpsStatus(spsMgmtPtr, FBE_SPS_MGMT_DAE0, FBE_LOCAL_SPS);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, processSpsStatus failed, status 0x%x \n", 
                                      __FUNCTION__, status);
            }
        }
    }
// may want a separate command for BoB's at some point
    if ((spsMgmtPtr->total_bob_count > 0) &&
        (spsMgmtPtr->arrayBobInfo->simulateBobInfo != *simulateSpsPtr))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, simulateBob changed from %d to %d\n", 
                              __FUNCTION__, spsMgmtPtr->arrayBobInfo->simulateBobInfo, *simulateSpsPtr);
        spsMgmtPtr->arrayBobInfo->simulateBobInfo = *simulateSpsPtr;
        // need to force BoB status change (all BoB's)
        for (bobIndex = 0; bobIndex < spsMgmtPtr->total_bob_count; bobIndex++)
        {
            fbe_set_memory(&location, 0, sizeof(fbe_device_physical_location_t));
            location.slot = bobIndex;
            status = fbe_sps_mgmt_processBobStatus(spsMgmtPtr, &location);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, processBobStatus failed, slot %d, status 0x%x \n", 
                                      __FUNCTION__, bobIndex, status);
            }
        }
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}
/******************************************
 * end fbe_sps_mgmt_setSimulateSps()
 ******************************************/


/*!***************************************************************
 * @fn fbe_sps_mgmt_getLocalBatteryTime()
 ****************************************************************
 * @brief
 *  This function returns the Local Battery Time.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  11-Jul-2012 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getLocalBatteryTime(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                     fbe_packet_t * packet)
{
    fbe_u32_t                               *batteryTimePtr = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &batteryTimePtr);
    if (batteryTimePtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length); 
    if (length != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %X len invalid\n", 
                              __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (spsMgmtPtr->total_sps_count > 0)
    {
// JAP - batteryTime from SPECL is zero, derive from SPS state for now
#if TRUE
        if (spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsStatus == SPS_STATE_AVAILABLE)
        {
            *batteryTimePtr = 100;
        }
        else
        {
            *batteryTimePtr = 0;
        }

#else
        *batteryTimePtr = spsMgmtPtr->arraySpsInfo->sps_info[FBE_SPS_MGMT_PE][FBE_LOCAL_SPS].spsBatteryTime;
#endif
    }
    else if (spsMgmtPtr->total_bob_count > 0)
    {
        if (spsMgmtPtr->arrayBobInfo->bob_info[spsMgmtPtr->base_environment.spid].batteryChargeState == 
            BATTERY_FULLY_CHARGED)
        {
            *batteryTimePtr = 100;
        }
        else
        {
            *batteryTimePtr = 0;
        }
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}
/******************************************
 * end fbe_sps_mgmt_getLocalBatteryTime()
 ******************************************/


/*!***************************************************************
 * @fn fbe_sps_mgmt_initiateSelfTest()
 ****************************************************************
 * @brief
 *  This function will initiate a Self Test on Battery Backup
 *  (SPS or BBU).
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  20-Dec-2012 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_initiateSelfTest(fbe_sps_mgmt_t *spsMgmtPtr, 
                                                  fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;

    // only support for BBU currently
    if (spsMgmtPtr->total_bob_count > 0)
    {
        spsMgmtPtr->arrayBobInfo->needToTest = TRUE;
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(sps_mgmt), 
                                        (fbe_base_object_t *)spsMgmtPtr,
                                        FBE_SPS_MGMT_TEST_NEEDED_COND);
        if (status == FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, schedule testNeeded condition\n",
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set TestNeeded condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}
/******************************************
 * end fbe_sps_mgmt_initiateSelfTest()
 ******************************************/

/*!***************************************************************
 * fbe_sps_mgmt_getSpsInputPowerSample()
 ****************************************************************
 * @brief
 *  This function processes the
 *  FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_INPUT_POWER_SAMPLE
 *  Control Code.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  09-May-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_getSpsInputPowerSample(fbe_sps_mgmt_t *spsMgmtPtr,
                                                  fbe_packet_t * packet)
{
    fbe_status_t                           status;
    fbe_esp_sps_mgmt_spsInputPowerSample_t *spsSamplePtr = NULL;
    fbe_payload_control_buffer_length_t    len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_u32_t                              spsEnclIndex, spsIndex;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &spsSamplePtr);
    if (spsSamplePtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_sps_mgmt_spsInputPowerSample_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sps_mgmt_convertSpsLocation(spsMgmtPtr, &spsSamplePtr->spsLocation, &spsEnclIndex, &spsIndex);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, bus 0x%x, encl 0x%x, slot %d invalid\n",
                              __FUNCTION__,
                              spsSamplePtr->spsLocation.bus,
                              spsSamplePtr->spsLocation.enclosure,
                              spsSamplePtr->spsLocation.slot);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    // get SPS Input Power info
//    fbe_set_memory(spsInputPowerPtr, 0, sizeof(fbe_esp_sps_mgmt_spsInputPower_t));
    if (spsIndex >= FBE_SPS_MAX_COUNT)
    {
        fbe_base_object_trace((fbe_base_object_t*)spsMgmtPtr,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, spsIndex %d invalid\n",
                              __FUNCTION__, spsIndex);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(
            spsSamplePtr->spsInputPowerSamples,
            spsMgmtPtr->spsEirData->spsInputPowerSamples[spsEnclIndex][spsIndex],
            sizeof(fbe_eir_input_power_sample_t) * spsMgmtPtr->spsEirData->spsSampleCount);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}


/*!***************************************************************
 * fbe_sps_mgmt_convertSpsLocation()
 ****************************************************************
 * @brief
 *  This function validates the SPS location info & returns
 *  indexes to be used to retrieve the appropriate data.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  10-Jun-2012 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_convertSpsLocation(fbe_sps_mgmt_t *spsMgmtPtr,
                                             fbe_device_physical_location_t *location,
                                             fbe_u32_t *spsEnclIndex,
                                             fbe_u32_t *spsIndex)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t      validEnclosure = FALSE;

    // validate SPS location info
    if (((spsMgmtPtr->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) &&
            (location->bus == 0) && (location->enclosure == 0)) ||
        ((spsMgmtPtr->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
            (location->bus == FBE_XPE_PSEUDO_BUS_NUM) && 
            (location->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
        ((spsMgmtPtr->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE) &&
            (location->bus == FBE_XPE_PSEUDO_BUS_NUM) && 
            (location->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)))
    {
        // processor enclosure SPS
        validEnclosure = TRUE;
        *spsEnclIndex = FBE_SPS_MGMT_PE;
    }
    else if (((spsMgmtPtr->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) &&
                 (location->bus == 0) && (location->enclosure == 0)) ||
             ((spsMgmtPtr->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE) &&
                 (location->bus == 0) && (location->enclosure == 0)) )
    {
        // processor enclosure SPS
        validEnclosure = TRUE;
        *spsEnclIndex = FBE_SPS_MGMT_DAE0;
    }

    if (validEnclosure)
    {
        if ((location->slot >= FBE_LOCAL_SPS) && (location->slot <= FBE_PEER_SPS))
        {
            status = FBE_STATUS_OK;
            if (spsMgmtPtr->base_environment.spid == location->slot)
            {
                *spsIndex = FBE_LOCAL_SPS;
            }
            else 
            {
                *spsIndex = FBE_PEER_SPS;
            }
        }
    }

    return status;

}   // end of fbe_sps_mgmt_convertSpsLocation


/*!***************************************************************
 * @fn fbe_sps_mgmt_convertSpsIndex()
 ****************************************************************
 * @brief
 *  This function converts the SPS index to the SPS physical location.
 *
 * @param  pSpsMgmt - 
 * @param  spsEnclIndex - 
 * @param  spsIndex - 
 * @param  pLocation - 
 * 
 * @return fbe_status_t
 *
 * @author
 *  11-Oct-2012  PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_convertSpsIndex(fbe_sps_mgmt_t * pSpsMgmt,
                                          fbe_u32_t spsEnclIndex,
                                          fbe_u32_t spsIndex,
                                          fbe_device_physical_location_t * pLocation)
{
    HW_ENCLOSURE_TYPE   processorEnclType;
    SP_ID               localSp;
    fbe_status_t        status = FBE_STATUS_OK;

    status = fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)pSpsMgmt, 
                                                  &processorEnclType);
    
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,  
                       FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Error in getting peType, status: 0x%x.\n",
                       __FUNCTION__, status);
        return status;
    }

    fbe_base_env_get_local_sp_id((fbe_base_environment_t *) pSpsMgmt, &localSp);

    if(spsEnclIndex == FBE_SPS_MGMT_PE)
    {
        if(processorEnclType == XPE_ENCLOSURE_TYPE) 
        {
            pLocation->bus = FBE_XPE_PSEUDO_BUS_NUM;
            pLocation->enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
        }
        else if(processorEnclType == DPE_ENCLOSURE_TYPE) 
        {
            pLocation->bus = 0;
            pLocation->enclosure = 0;
        }
        else
        { 
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else if(spsEnclIndex == FBE_SPS_MGMT_DAE0) 
    {
        pLocation->bus = 0;
        pLocation->enclosure = 0;
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(spsIndex == FBE_LOCAL_SPS)
    {
        pLocation->slot = (localSp == SP_A) ? 0 : 1; 
    }
    else if(spsIndex == FBE_PEER_SPS) 
    {
        pLocation->slot = (localSp == SP_A) ? 1 : 0; 
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return FBE_STATUS_OK;
}
       
/*!***************************************************************
 * @fn fbe_sps_mgmt_format_rev_for_admin()
 ****************************************************************
 * @brief
 *  Copy only the numeric revision, no letters.
 *
 * @param fbe_u8_t *pRevToFormat - pointer to the revision to reformat
 *        fbe_u8_t sizeLimit - revision size in bytes
 *
 * @return none
 *
 * @author
 *  12-Mar-2012 GB
 *
 ****************************************************************/
void fbe_sps_mgmt_format_rev_for_admin(fbe_sps_mgmt_t * pSpsMgmt,
                                       fbe_u8_t *pRevToFormat, 
                                       fbe_u8_t sizeLimit)
{
    fbe_u8_t srcIndex = 0;
    fbe_u8_t dstIndex = 0;
    fbe_u8_t    afterFormatting[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1] = {0};
    
    if (sizeLimit > FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,  
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s RevSizeExceeded ActLimit:%d ExpLimit:%d\n",
                              __FUNCTION__, 
                              sizeLimit,
                              FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
        sizeLimit = FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE;
    }

    // only accept numbers, no letters
    for(srcIndex = 0; srcIndex < sizeLimit; srcIndex++)
    {
        if((pRevToFormat[srcIndex] >= '0') && 
           (pRevToFormat[srcIndex] <= '9'))
        {
            afterFormatting[dstIndex] = pRevToFormat[srcIndex];
            dstIndex ++;
        }
    }
    fbe_copy_memory(pRevToFormat,
                    &afterFormatting[0],
                    sizeLimit);
    return;

} //fbe_sps_mgmt_format_rev_for_admin   



/*!***************************************************************
 * fbe_sps_mgmt_updateBbuFailStatus()
 ****************************************************************
 * @brief
 *  This function will set the appropriate PS failed status.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  10-Mar-2014 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_sps_mgmt_updateBbuFailStatus(fbe_u8_t *mgmtPtr,
                                 fbe_device_physical_location_t *pLocation,
                                 fbe_base_environment_failure_type_t failureType,
                                 fbe_bool_t newValue)
{
    fbe_status_t                status;
    fbe_sps_mgmt_t              *spsMgmtPtr;
    fbe_u8_t                    deviceStr[FBE_DEVICE_STRING_LENGTH]={0};

    spsMgmtPtr = (fbe_sps_mgmt_t *)mgmtPtr;

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BATTERY, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    switch (failureType)
    {
    case FBE_BASE_ENV_RP_READ_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, resPromReadFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              spsMgmtPtr->arrayBobInfo->bob_info[pLocation->slot].resumePromReadFailed,
                              newValue);
        spsMgmtPtr->arrayBobInfo->bob_info[pLocation->slot].resumePromReadFailed = newValue;
        break;
    case FBE_BASE_ENV_FUP_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, fupFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              spsMgmtPtr->arrayBobInfo->bob_info[pLocation->slot].fupFailure,
                              newValue);
        spsMgmtPtr->arrayBobInfo->bob_info[pLocation->slot].fupFailure = newValue;
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t *)spsMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Unsupported failureType %d.\n", 
                              __FUNCTION__, failureType); 
        return FBE_STATUS_COMPONENT_NOT_FOUND;
        break;
    }

    return FBE_STATUS_OK;

}   // end of fbe_sps_mgmt_updateBbuFailStatus


