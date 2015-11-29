/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_encl_mgmt_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure Management object usurper
 *  control code handling function.
 * 
 * @ingroup encl_mgmt_class_files
 * 
 * @version
 *   23-Mar-2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_encl_mgmt_private.h"
#include "fbe_encl_mgmt_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_esp_encl_mgmt.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_environment_limit.h"
#include "speeds.h"
#include "fbe/fbe_event_log_api.h"
#include "EspMessages.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t fbe_get_bus_info(fbe_encl_mgmt_t * encl_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_get_max_be_bus_count(fbe_encl_mgmt_t * encl_mgmt, fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_get_total_encl_count(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_encl_location(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_encl_count_on_bus(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_encl_info(fbe_encl_mgmt_t *encl_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_encl_presence(fbe_encl_mgmt_t *encl_mgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_lcc_info(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_eir_info(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_device_count(fbe_encl_mgmt_t * pEnclMgmt, 
                                                   fbe_packet_t * packet,
                                                   fbe_u64_t deviceType);
static fbe_status_t fbe_encl_mgmt_getCacheStatus(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_connector_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                               fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_ssc_info(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_set_led_flash(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_send_led_flash_cmd(fbe_encl_mgmt_t *pEnclMgmt, 
                                                     fbe_esp_encl_mgmt_set_led_flash_t *pFlashCommand);
fbe_u32_t fbe_encl_mgmt_roundValueU32Tenths(UINT_32 valueInTenths);
fbe_u32_t fbe_encl_mgmt_roundValueU32HundredTimes(UINT_32 valueInHundredTimes);
static fbe_status_t fbe_encl_mgmt_get_eir_power_input_sample(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_eir_temp_sample(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_get_array_midplane_rp_wwn_seed(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_get_user_modified_wwn_seed_flag(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_set_array_midplane_rp_wwn_seed(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_user_modify_array_midplane_rp_wwn_seed(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_clear_user_modified_wwn_seed_flag(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_modify_system_id_info(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_get_system_id_info(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet);
fbe_bool_t fbe_encl_mgmt_encl_has_no_physical_lcc(fbe_encl_mgmt_t * pEnclMgmt, 
                                                   fbe_encl_info_t * pEnclInfo);

static fbe_status_t fbe_encl_mgmt_notify_peer_to_set_user_modified_system_id_flag(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_bool_t user_modified_sys_id_flag);

/*!***************************************************************
 * fbe_encl_mgmt_control_entry()
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
 *  25-Feb-2010 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_encl_mgmt_t *encl_mgmt;
    
    encl_mgmt = (fbe_encl_mgmt_t *)fbe_base_handle_to_pointer(object_handle);
    
    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_ESP_CONTROL_CODE_GET_BUS_INFO:
            status = fbe_get_bus_info(encl_mgmt, packet);
            break;

        case FBE_ESP_CONTROL_CODE_GET_MAX_BE_BUS_COUNT:
            status = fbe_get_max_be_bus_count(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_TOTAL_ENCL_COUNT:
            status = fbe_encl_mgmt_get_total_encl_count(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_LOCATION:
            status = fbe_encl_mgmt_get_encl_location(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_COUNT_ON_BUS:
            status = fbe_encl_mgmt_get_encl_count_on_bus(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_INFO:
            status = fbe_encl_mgmt_get_encl_info(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_PRESENCE:
            status = fbe_encl_mgmt_get_encl_presence(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_INFO:
            status = fbe_encl_mgmt_get_lcc_info(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INFO:
            status = fbe_encl_mgmt_get_eir_info(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_COUNT:
            status = fbe_encl_mgmt_get_device_count(encl_mgmt, packet, FBE_DEVICE_TYPE_LCC);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_PS_COUNT:
            status = fbe_encl_mgmt_get_device_count(encl_mgmt, packet, FBE_DEVICE_TYPE_PS);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_FAN_COUNT:
            status = fbe_encl_mgmt_get_device_count(encl_mgmt, packet, FBE_DEVICE_TYPE_FAN);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_DRIVE_SLOT_COUNT:
            status = fbe_encl_mgmt_get_device_count(encl_mgmt, packet, FBE_DEVICE_TYPE_DRIVE);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_CONNECTOR_COUNT:
            status = fbe_encl_mgmt_get_device_count(encl_mgmt, packet, FBE_DEVICE_TYPE_CONNECTOR);
            break;
            
        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SSC_COUNT:
            status = fbe_encl_mgmt_get_device_count(encl_mgmt, packet, FBE_DEVICE_TYPE_SSC);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SSC_INFO:
            status = fbe_encl_mgmt_get_ssc_info(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_CACHE_STATUS:
            status = fbe_encl_mgmt_getCacheStatus(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_CONNECTOR_INFO:
            status = fbe_encl_mgmt_get_connector_info(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_FLASH_ENCL_LEDS:
            status = fbe_encl_mgmt_set_led_flash(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INPUT_POWER_SAMPLE:
            status = fbe_encl_mgmt_get_eir_power_input_sample(encl_mgmt, packet);
            break;
        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_TEMP_SAMPLE:
            status = fbe_encl_mgmt_get_eir_temp_sample(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ARRAY_MIDPLANE_RP_WWN_SEED:
            status = fbe_encl_mgmt_get_array_midplane_rp_wwn_seed(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_SET_ARRAY_MIDPLANE_RP_WWN_SEED:
            status = fbe_encl_mgmt_set_array_midplane_rp_wwn_seed(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_USER_MODIFIED_WWN_SEED_FLAG:
            status = fbe_encl_mgmt_get_user_modified_wwn_seed_flag(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_CLEAR_USER_MODIFIED_WWN_SEED_FLAG:
            status = fbe_encl_mgmt_clear_user_modified_wwn_seed_flag(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_USER_MODIFY_ARRAY_MIDPLANE_WWN_SEED:
            status = fbe_encl_mgmt_user_modify_array_midplane_rp_wwn_seed(encl_mgmt, packet);
            break;

        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_MODIFY_SYSTEM_ID_INFO:
            status = fbe_encl_mgmt_modify_system_id_info(encl_mgmt, packet);
            break;
        case FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SYSTEM_ID_INFO:
            status = fbe_encl_mgmt_get_system_id_info(encl_mgmt, packet);
            break;

        default:
            status =  fbe_base_environment_control_entry (object_handle, packet);
            break;
    }
    return status;
}
/******************************************
 * end fbe_encl_mgmt_control_entry()
 ******************************************/

/*!***************************************************************
 * fbe_get_bus_info()
 ****************************************************************
 * @brief
 *  This function processes the FBE_ESP_CONTROL_CODE_GET_BUS_INFO Control Code.
 *
 * @param encl_mgmt -
 * @param packet -
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @author
 *  20-Feb-2012 Eric Zhou - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_get_bus_info(fbe_encl_mgmt_t * encl_mgmt,
                                     fbe_packet_t * packet)
{
    fbe_payload_ex_t                        * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_payload_control_buffer_length_t       control_buffer_length = 0;
    fbe_esp_get_bus_info_t                  * pGetBusInfo = NULL;
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_u32_t                                 enclIndex = 0;
    fbe_u32_t                                 enclCountOnBus = 0;
    fbe_status_t                              status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetBusInfo);
    if( (pGetBusInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetBusInfo: %64p.\n",
                               __FUNCTION__, status, pGetBusInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) ||
       (control_buffer_length != sizeof(fbe_esp_get_bus_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %d, status: 0x%x.\n",
                                __FUNCTION__,
                                control_buffer_length,
                                (int)sizeof(fbe_esp_get_bus_info_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pGetBusInfo->busInfo.currentBusSpeed = SPEED_SIX_GIGABIT;
    pGetBusInfo->busInfo.capableBusSpeed = SPEED_SIX_GIGABIT;

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex ++)
    {
        pEnclInfo = encl_mgmt->encl_info[enclIndex];

        if((pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)&&
           (pEnclInfo->location.bus == pGetBusInfo->bus))
        {
            enclCountOnBus ++;
        }
    }

    pGetBusInfo->busInfo.numOfEnclOnBus = enclCountOnBus;
    pGetBusInfo->busInfo.maxNumOfEnclOnBus = FBE_API_ENCLOSURES_PER_BUS;
    pGetBusInfo->busInfo.maxNumOfEnclOnBusExceeded = (enclCountOnBus > FBE_API_ENCLOSURES_PER_BUS) ? TRUE : FALSE;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_get_bus_info()
 ******************************************/

/*!***************************************************************
 * fbe_get_max_be_bus_count()
 ****************************************************************
 * @brief
 *  This function processes the FBE_ESP_CONTROL_CODE_GET_MAX_BE_BUS_COUNT Control Code.
 *
 * @param encl_mgmt -
 * @param packet -
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @author
 *  20-Feb-2012 Eric Zhou - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_get_max_be_bus_count(fbe_encl_mgmt_t * encl_mgmt,
                                         fbe_packet_t * packet)
{
    fbe_payload_ex_t                    * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_payload_control_buffer_length_t   control_buffer_length = 0;
    fbe_u32_t                           * pBeBusCount = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pBeBusCount);
    if( (pBeBusCount == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pBeBusCount: %64p.\n",
                               __FUNCTION__, status, pBeBusCount);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) ||
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %d, status: 0x%x.\n",
                                __FUNCTION__,
                                control_buffer_length,
                                (int)sizeof(fbe_u32_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pBeBusCount = fbe_environment_limit_get_platform_max_be_count();

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_get_max_be_bus_count()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_get_total_encl_count()
 ****************************************************************
 * @brief
 *  This function gets the total enclosure count on the system.
 *  It includes the XPE if this is a XPE system.
 *
 * @param pEnclMgmt - 
 * @param packet - 
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_total_encl_count(fbe_encl_mgmt_t * pEnclMgmt, 
                                                       fbe_packet_t * packet)
{ 
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_payload_control_buffer_length_t   control_buffer_length = 0;
    fbe_u32_t                           * pTotalEnclCount = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pTotalEnclCount);
    if( (pTotalEnclCount == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pTotalEnclCount: %64p.\n",
                               __FUNCTION__, status, pTotalEnclCount);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    *pTotalEnclCount = pEnclMgmt->total_encl_count;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_get_total_encl_count()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_get_encl_location()
 ****************************************************************
 * @brief
 *  This function gets the total enclosure count on the system.
 *  It includes the XPE if this is a XPE system.
 *
 * @param pEnclMgmt - 
 * @param packet - 
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_encl_location(fbe_encl_mgmt_t * pEnclMgmt, 
                                                    fbe_packet_t * packet)
{ 
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_payload_control_buffer_length_t   control_buffer_length = 0;
    fbe_esp_encl_mgmt_get_encl_loc_t    * pEnclLocation = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_u32_t                             enclFoundCount, enclIndex;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pEnclLocation);
    if( (pEnclLocation == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pEnclLocation: %64p.\n",
                               __FUNCTION__, status, pEnclLocation);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_encl_mgmt_get_encl_loc_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_u32_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /*
     * Loop through enclosure until specified one is found
     */
    enclFoundCount = 0;
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, index %d, enclCount %d\n", 
                           __FUNCTION__, pEnclLocation->enclIndex, pEnclMgmt->total_encl_count);

    for (enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        if (pEnclMgmt->encl_info[enclIndex]->object_id != FBE_OBJECT_ID_INVALID)
        {
            if (pEnclLocation->enclIndex == enclFoundCount)
            {
                pEnclLocation->location = pEnclMgmt->encl_info[enclIndex]->location;

                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                       FBE_TRACE_LEVEL_DEBUG_LOW,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, index %d Found, bus %d encl %d.\n", 
                                       __FUNCTION__, 
                                      pEnclLocation->enclIndex,
                                      pEnclLocation->location.bus,
                                      pEnclLocation->location.enclosure);
                
                status = FBE_STATUS_OK;
                fbe_transport_set_status(packet, status, 0);
                fbe_transport_complete_packet(packet);
                return status;
            }
            enclFoundCount++;
        }
    }

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, index %d NOT Found.\n", 
                           __FUNCTION__,
                           pEnclLocation->enclIndex);

    status = FBE_STATUS_NO_DEVICE;
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/******************************************
 * end fbe_encl_mgmt_get_encl_location()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_get_encl_count_on_bus()
 ****************************************************************
 * @brief
 *  This function gets the total enclosure count on the system.
 *  It includes the XPE if this is a XPE system.
 *
 * @param pEnclMgmt - 
 * @param packet - 
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_get_encl_count_on_bus(fbe_encl_mgmt_t * pEnclMgmt, 
                                    fbe_packet_t * packet)
{
    fbe_payload_ex_t                               * payload = NULL;
    fbe_payload_control_operation_t             * control_operation = NULL;
    fbe_payload_control_buffer_length_t           control_buffer_length = 0;
    fbe_esp_encl_mgmt_get_encl_count_on_bus_t   * pGetEnclCountOnBus = NULL;
    fbe_encl_info_t                             * pEnclInfo = NULL;
    fbe_u32_t                                     enclIndex = 0;
    fbe_u32_t                                     enclCountOnBus = 0;
    fbe_status_t                                  status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetEnclCountOnBus);
    if( (pGetEnclCountOnBus == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetEnclCountOnBus: %64p.\n",
                               __FUNCTION__, status, pGetEnclCountOnBus);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_encl_mgmt_get_encl_count_on_bus_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_encl_count_on_bus_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex ++) 
    {
        pEnclInfo = pEnclMgmt->encl_info[enclIndex];

        if((pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)&&
           (pEnclInfo->location.bus == pGetEnclCountOnBus->location.bus))
        {
            enclCountOnBus ++;
        }
    }

    pGetEnclCountOnBus->enclCountOnBus = enclCountOnBus;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_get_encl_count_on_bus()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_get_device_count()
 ****************************************************************
 * @brief
 *  This function processes the Control Code to get the device count.
 *  
 * @param pEnclMgmt 
 * @param packet 
 * @param deviceType
 *
 * @return fbe_status_t
 *
 * @author
 *  28-Mar-2011 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_device_count(fbe_encl_mgmt_t * pEnclMgmt, 
                                                   fbe_packet_t * packet,
                                                   fbe_u64_t deviceType)
{
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_payload_control_buffer_length_t       control_buffer_length = 0;
    fbe_esp_encl_mgmt_get_device_count_t    * pGetDeviceCountCmd = NULL;
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetDeviceCountCmd);
    if( (pGetDeviceCountCmd == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status 0x%x, pGetDeviceCountCmd 0x%p.\n",
                               __FUNCTION__, status, pGetDeviceCountCmd);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_encl_mgmt_get_device_count_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_device_count_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 


    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt,
                                            &pGetDeviceCountCmd->location,
                                            &pEnclInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%x\n", 
                              __FUNCTION__, status);
        
        pGetDeviceCountCmd->deviceCount = 0;
    }
    else
    {
        switch(deviceType)
        {
            case FBE_DEVICE_TYPE_LCC:
                pGetDeviceCountCmd->deviceCount = pEnclInfo->lccCount; 
                break;

            case FBE_DEVICE_TYPE_PS:
                pGetDeviceCountCmd->deviceCount = pEnclInfo->psCount; 
                break;
    
            case FBE_DEVICE_TYPE_FAN:
                pGetDeviceCountCmd->deviceCount = pEnclInfo->fanCount; 
                break;
    
            case FBE_DEVICE_TYPE_DRIVE:
                pGetDeviceCountCmd->deviceCount = pEnclInfo->driveSlotCount; 
                break;

            case FBE_DEVICE_TYPE_CONNECTOR:
                pGetDeviceCountCmd->deviceCount = pEnclInfo->connectorCount; 
                break;

            case FBE_DEVICE_TYPE_SSC:
                pGetDeviceCountCmd->deviceCount = pEnclInfo->sscCount;
    
            default:
                pGetDeviceCountCmd->deviceCount = 0;
                break;
        }
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_encl_mgmt_get_device_count()
 ******************************************/


/*!***************************************************************
 * fbe_encl_mgmt_get_encl_info()
 ****************************************************************
 * @brief
 *  This function processes the get enclosure info Control Code.
 *  
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  02-Mar-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_encl_info(fbe_encl_mgmt_t *encl_mgmt, 
                                                fbe_packet_t * packet)
{
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_payload_control_buffer_length_t       control_buffer_length = 0;
    fbe_esp_encl_mgmt_get_encl_info_t       * pGetEnclInfo = NULL;
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_u32_t                                 i=0;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetEnclInfo);
    if( (pGetEnclInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetEnclInfo: %64p.\n",
                               __FUNCTION__, status, pGetEnclInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_encl_mgmt_get_encl_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_encl_info_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_encl_mgmt_get_encl_info_by_location(encl_mgmt, &pGetEnclInfo->location, &pEnclInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%x.\n", 
                              __FUNCTION__, status);

        pGetEnclInfo->enclInfo.enclPresent = FALSE;
        pGetEnclInfo->enclInfo.enclState = FBE_ESP_ENCL_STATE_MISSING;
        pGetEnclInfo->enclInfo.enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_NO_FAULT;
        pGetEnclInfo->enclInfo.encl_type = FBE_ESP_ENCL_TYPE_UNKNOWN;
        pGetEnclInfo->enclInfo.lccCount = 0;
        pGetEnclInfo->enclInfo.psCount = 0;
        pGetEnclInfo->enclInfo.fanCount = 0;
        pGetEnclInfo->enclInfo.driveSlotCount = 0;
        pGetEnclInfo->enclInfo.sscCount = 0;
        pGetEnclInfo->enclInfo.dimmCount = 0;
        pGetEnclInfo->enclInfo.processorEncl = 0;
        pGetEnclInfo->enclInfo.shutdownReason = FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED;
        pGetEnclInfo->enclInfo.overTempWarning = FALSE;
        pGetEnclInfo->enclInfo.overTempFailure = FALSE;
        pGetEnclInfo->enclInfo.ignoreUnsupportedEnclosures = FALSE;
    }
    else
    {
        // Copy the enclosure information
        pGetEnclInfo->enclInfo.enclPresent = TRUE;
        pGetEnclInfo->enclInfo.enclState = pEnclInfo->enclState;
        pGetEnclInfo->enclInfo.enclFaultSymptom = pEnclInfo->enclFaultSymptom;
        pGetEnclInfo->enclInfo.encl_type = pEnclInfo->enclType;
        pGetEnclInfo->enclInfo.lccCount = pEnclInfo->lccCount;
        pGetEnclInfo->enclInfo.psCount = pEnclInfo->psCount;
        pGetEnclInfo->enclInfo.fanCount = pEnclInfo->fanCount;
        pGetEnclInfo->enclInfo.driveSlotCount = pEnclInfo->driveSlotCount;
        pGetEnclInfo->enclInfo.driveSlotCountPerBank = pEnclInfo->driveSlotCountPerBank;
        pGetEnclInfo->enclInfo.bbuCount = pEnclInfo->bbuCount;
        pGetEnclInfo->enclInfo.spsCount = pEnclInfo->spsCount;
        pGetEnclInfo->enclInfo.spCount = pEnclInfo->spCount;
        pGetEnclInfo->enclInfo.sscCount = pEnclInfo->sscCount;
        pGetEnclInfo->enclInfo.dimmCount = pEnclInfo->dimmCount;
        pGetEnclInfo->enclInfo.ssdCount = pEnclInfo->ssdCount;
        pGetEnclInfo->enclInfo.subEnclCount = pEnclInfo->subEnclCount;
        pGetEnclInfo->enclInfo.currSubEnclCount = pEnclInfo->currSubEnclCount;
        pGetEnclInfo->enclInfo.processorEncl = pEnclInfo->processorEncl;
        pGetEnclInfo->enclInfo.shutdownReason = pEnclInfo->shutdownReason;
        pGetEnclInfo->enclInfo.overTempWarning = pEnclInfo->overTempWarning;
        pGetEnclInfo->enclInfo.overTempFailure = pEnclInfo->overTempFailure;
        pGetEnclInfo->enclInfo.maxEnclSpeed = pEnclInfo->maxEnclSpeed;
        pGetEnclInfo->enclInfo.currentEnclSpeed = pEnclInfo->currentEnclSpeed;
        pGetEnclInfo->enclInfo.enclFaultLedStatus = pEnclInfo->enclFaultLedStatus;
        pGetEnclInfo->enclInfo.enclFaultLedReason = pEnclInfo->enclFaultLedReason;
        pGetEnclInfo->enclInfo.crossCable = pEnclInfo->crossCable;
        pGetEnclInfo->enclInfo.driveMidplaneCount = pEnclInfo->driveMidplaneCount;
        pGetEnclInfo->enclInfo.ignoreUnsupportedEnclosures = encl_mgmt->ignoreUnsupportedEnclosures;
        fbe_copy_memory(&pGetEnclInfo->enclInfo.display, &pEnclInfo->display, sizeof(fbe_encl_display_info_t));
        fbe_copy_memory(&pGetEnclInfo->enclInfo.enclDriveFaultLeds, 
                        &pEnclInfo->enclDriveFaultLeds, 
                        (sizeof(fbe_led_status_t) * FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS));
        for (i=0; i<pEnclInfo->subEnclCount;i++)
        {
            fbe_copy_memory(&pGetEnclInfo->enclInfo.subEnclInfo[i].location, &pEnclInfo->subEnclInfo[i].location, sizeof(fbe_device_physical_location_t));
        }
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_encl_mgmt_get_encl_info()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_get_encl_presence()
 ****************************************************************
 * @brief
 *  This function checks whether the enclosuer on the specified location 
 *  is present or not.  
 *
 * @param encl_mgmt - 
 * @param packet - 
 *
 * @return fbe_status_t
 *
 * @author
 *  13-Mar-2013 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_encl_presence(fbe_encl_mgmt_t *encl_mgmt, 
                                                fbe_packet_t * packet)
{
    fbe_payload_ex_t                           * payload = NULL;
    fbe_payload_control_operation_t         * control_operation = NULL;
    fbe_payload_control_buffer_length_t       control_buffer_length = 0;
    fbe_esp_encl_mgmt_get_encl_presence_t   * pGetEnclPresence = NULL;
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetEnclPresence);
    if( (pGetEnclPresence == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetEnclPresence: %64p.\n",
                               __FUNCTION__, status, pGetEnclPresence);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_encl_mgmt_get_encl_presence_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_encl_presence_t), status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_encl_mgmt_get_encl_info_by_location(encl_mgmt, &pGetEnclPresence->location, &pEnclInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%x.\n", 
                              __FUNCTION__, status);

        pGetEnclPresence->enclPresent = FBE_FALSE;
    }
    else
    {
        pGetEnclPresence->enclPresent = TRUE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_encl_mgmt_get_encl_info()
 ******************************************/

/*!***************************************************************
 * fbe_encl_mgmt_get_lcc_info()
 ****************************************************************
 * @brief
 *  This function processes the get lcc info Control Code.
 *  
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  29-Sept-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_lcc_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                               fbe_packet_t * packet)
{
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_payload_control_buffer_length_t   control_buffer_length = 0;
    fbe_esp_encl_mgmt_get_lcc_info_t    * pGetLccInfo = NULL;
    fbe_lcc_info_t                      * pLccInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetLccInfo);
    if( (pGetLccInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetLccInfo: %64p.\n",
                               __FUNCTION__, status, pGetLccInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_encl_mgmt_get_lcc_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_lcc_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 


    status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt,
                                        &pGetLccInfo->location,
                                        &pLccInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_encl_mgmt_get_lcc_info_ptr failed, status 0x%x\n",
                              __FUNCTION__, status);

        pGetLccInfo->lccInfo.inserted = FBE_FALSE;
        pGetLccInfo->lccInfo.state = FBE_LCC_STATE_REMOVED;
        pGetLccInfo->lccInfo.subState = FBE_LCC_SUBSTATE_NO_FAULT;
    }
    else
    {
        pGetLccInfo->lccInfo.inserted = pLccInfo->inserted;
        // check type of LCC failure
        if (pLccInfo->faulted && (pLccInfo->additionalStatus == SES_STAT_CODE_CRITICAL))
        {
            pGetLccInfo->lccInfo.faulted = TRUE; 
            pGetLccInfo->lccInfo.lccComponentFaulted = FALSE;
            // only using state/subState for LCC Component faults for now
            pGetLccInfo->lccInfo.state = FBE_LCC_STATE_FAULTED;
            pGetLccInfo->lccInfo.subState = FBE_LCC_SUBSTATE_LCC_FAULT;
        }
        else if (pLccInfo->faulted && (pLccInfo->additionalStatus != SES_STAT_CODE_CRITICAL))
        {
// ESP_STAND_ALONE_ALERT - remove when CP support available
            pGetLccInfo->lccInfo.faulted = TRUE; 
            pGetLccInfo->lccInfo.lccComponentFaulted = TRUE;
            pGetLccInfo->lccInfo.state = FBE_LCC_STATE_FAULTED;
            pGetLccInfo->lccInfo.subState = FBE_LCC_SUBSTATE_COMPONENT_FAULT;
        }
        else if (pLccInfo->fupFailure && pLccInfo->inserted)
        {
            pGetLccInfo->lccInfo.state = FBE_LCC_STATE_DEGRADED;
            pGetLccInfo->lccInfo.subState = FBE_LCC_SUBSTATE_FUP_FAILURE;
        } 
        else if (pLccInfo->resumePromReadFailed && pLccInfo->inserted)
        {
            pGetLccInfo->lccInfo.state = FBE_LCC_STATE_DEGRADED;
            pGetLccInfo->lccInfo.subState = FBE_LCC_SUBSTATE_RP_READ_FAILURE;
        } 
        else 
        {
            pGetLccInfo->lccInfo.faulted = FALSE; 
            pGetLccInfo->lccInfo.lccComponentFaulted = FALSE;
            pGetLccInfo->lccInfo.state = FBE_LCC_STATE_OK;
            pGetLccInfo->lccInfo.subState = FBE_LCC_SUBSTATE_NO_FAULT;
        }
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, state %s, subState %s\n",
                              __FUNCTION__, 
                              fbe_encl_mgmt_decode_lcc_state(pGetLccInfo->lccInfo.state),
                              fbe_encl_mgmt_decode_lcc_subState(pGetLccInfo->lccInfo.subState));

        pGetLccInfo->lccInfo.isLocal = pLccInfo->isLocal;
        pGetLccInfo->lccInfo.esesRevision = pLccInfo->eses_version_desc;
        fbe_copy_memory(&pGetLccInfo->lccInfo.firmwareRev[0],
                        &pLccInfo->firmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetLccInfo->lccInfo.initStrFirmwareRev[0],
                        &pLccInfo->initStrFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetLccInfo->lccInfo.fpgaFirmwareRev[0],
                        &pLccInfo->fpgaFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        pGetLccInfo->lccInfo.shunted = pLccInfo->shunted;
        pGetLccInfo->lccInfo.expansionPortOpen = pLccInfo->expansionPortOpen;
        pGetLccInfo->lccInfo.lccType = pLccInfo->lccType;
        pGetLccInfo->lccInfo.currentSpeed = pLccInfo->currentSpeed;
        pGetLccInfo->lccInfo.maxSpeed = pLccInfo->maxSpeed;
        pGetLccInfo->lccInfo.hasResumeProm = pLccInfo->hasResumeProm;
        pGetLccInfo->lccInfo.resumePromReadFailed = pLccInfo->resumePromReadFailed;
        pGetLccInfo->lccInfo.fupFailure = pLccInfo->fupFailure;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_encl_mgmt_get_ssc_info()
 ****************************************************************
 * @brief
 *  Get the SSC info from the enclosure and copy it to the 
 *  buffer passed fro mthe caller.
 *  
 * @param pEnclMgmt - None.
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  19-Dec-2013 GB - Created.
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_ssc_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                               fbe_packet_t * packet)
{
    fbe_payload_ex_t                        * payload = NULL;
    fbe_payload_control_operation_t         * pControlOperation = NULL;
    fbe_payload_control_buffer_length_t     controlBufferLength = 0;
    fbe_esp_encl_mgmt_get_ssc_info_t        * pGetSscInfo = NULL;
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pControlOperation = fbe_payload_ex_get_control_operation(payload);
    if(pControlOperation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(pControlOperation, &pGetSscInfo);
    if( (pGetSscInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetSscInfo: %64p.\n",
                               __FUNCTION__, status, pGetSscInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(pControlOperation, &controlBufferLength);
    if((status != FBE_STATUS_OK) || 
       (controlBufferLength != sizeof(fbe_esp_encl_mgmt_get_ssc_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                controlBufferLength, 
                                (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_ssc_info_t),
                                status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt,
                                                    &pGetSscInfo->location,
                                                    &pEnclInfo);

    if (status == FBE_STATUS_OK)
    {
        // copy to the caller's buffer
        pGetSscInfo->sscInfo.inserted = pEnclInfo->sscInfo.inserted;
        pGetSscInfo->sscInfo.faulted = pEnclInfo->sscInfo.faulted;
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s, SSC Insert %d, Fault %d\n", 
                              __FUNCTION__, 
                              pEnclInfo->sscInfo.inserted,
                              pEnclInfo->sscInfo.faulted);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%x\n", 
                              __FUNCTION__, status);

        pGetSscInfo->sscInfo.inserted = FBE_FALSE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
} //fbe_encl_mgmt_get_ssc_info

/*!***************************************************************
 * fbe_encl_mgmt_get_connector_info()
 ****************************************************************
 * @brief
 *  This function processes the get Connector info Control Code.
 *  
 * @param pEnclMgmt - None.
 * @param packet -
 *
 * @return fbe_status_t
 *
 * @author
 *  24-Aug-2011 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_connector_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                               fbe_packet_t * packet)
{
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_payload_control_buffer_length_t   control_buffer_length = 0;
    fbe_esp_encl_mgmt_get_connector_info_t    * pGetConnectorInfo = NULL;
    fbe_encl_info_t                     * pEnclInfo = NULL;
    fbe_connector_info_t                * pConnectorInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetConnectorInfo);
    if( (pGetConnectorInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetConnectorInfo: %64p.\n",
                               __FUNCTION__, status, pGetConnectorInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_encl_mgmt_get_connector_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_connector_info_t),
                                status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt,
                                            &pGetConnectorInfo->location,
                                            &pEnclInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%x\n", 
                              __FUNCTION__, status);

        pGetConnectorInfo->connectorInfo.inserted = FBE_FALSE;
    }
    else
    {
        pConnectorInfo = &pEnclInfo->connectorInfo[pGetConnectorInfo->location.slot];
        
        pGetConnectorInfo->connectorInfo.isLocalFru = pConnectorInfo->isLocalFru; 
        pGetConnectorInfo->connectorInfo.inserted = pConnectorInfo->inserted; 
        pGetConnectorInfo->connectorInfo.cableStatus = pConnectorInfo->cableStatus;
        pGetConnectorInfo->connectorInfo.connectorType = pConnectorInfo->connectorType;
        pGetConnectorInfo->connectorInfo.connectorId = pConnectorInfo->connectorId;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/******************************************
 * end fbe_encl_mgmt_get_lcc_info()
 ******************************************/


/*!***************************************************************
 * fbe_encl_mgmt_get_eir_info()
 ****************************************************************
 * @brief
 *  This function processes the get EIR info Control Code.
 *  
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Dec-2010 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_eir_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                               fbe_packet_t * packet)
{
    fbe_esp_encl_mgmt_get_eir_info_t    * pGetEirInfo = NULL;
    fbe_payload_control_buffer_length_t   len = 0;
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_encl_info_t                     * pEnclInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_u32_t                             enclIndex = 0, validEnclCount = 0;
    fbe_eir_input_power_sample_t        inputPowerTotal;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGetEirInfo);
    if (pGetEirInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_encl_mgmt_get_eir_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %llu.\n", 
                               __FUNCTION__, len, (unsigned long long)sizeof(fbe_esp_encl_mgmt_get_eir_info_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (pGetEirInfo->eirInfoType == FBE_ENCL_MGMT_EIR_IP_TOTAL)
    {
        fbe_encl_mgmt_clearInputPowerSample(&inputPowerTotal);
        for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex ++) 
        {
            pEnclInfo = pEnclMgmt->encl_info[enclIndex];
      
            if (pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)
            {
                inputPowerTotal.status |= pEnclInfo->eir_data.enclCurrentInputPower.status;
                if (inputPowerTotal.status == FBE_ENERGY_STAT_VALID)
                {
                    inputPowerTotal.inputPower +=
                            pEnclInfo->eir_data.enclCurrentInputPower.inputPower;
                }
                ++validEnclCount;
            }
            
            if (validEnclCount >= pEnclMgmt->total_encl_count) 
            {
                break;
            }
        }
        
        pGetEirInfo->eirEnclInfo.eirEnclInputPowerTotal.enclCount = pEnclMgmt->total_encl_count;
        pGetEirInfo->eirEnclInfo.eirEnclInputPowerTotal.inputPowerAll.inputPower =
                fbe_encl_mgmt_roundValueU32Tenths(inputPowerTotal.inputPower);
        pGetEirInfo->eirEnclInfo.eirEnclInputPowerTotal.inputPowerAll.status = inputPowerTotal.status;

        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, EIR_IP_TOTAL, enclCnt %d, inputPower %d, status 0x%x\n",
                              __FUNCTION__, 
                              pGetEirInfo->eirEnclInfo.eirEnclInputPowerTotal.enclCount,
                              pGetEirInfo->eirEnclInfo.eirEnclInputPowerTotal.inputPowerAll.inputPower,
                              pGetEirInfo->eirEnclInfo.eirEnclInputPowerTotal.inputPowerAll.status);
    }
    else if (pGetEirInfo->eirInfoType == FBE_ENCL_MGMT_EIR_INDIVIDUAL)
    {
        /*
         * Find enclosure (should already exist)
         */
        status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, 
                                            &(pGetEirInfo->eirEnclInfo.eirEnclIndividual.location), 
                                                         &pEnclInfo);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                                  __FUNCTION__, 
                                  pGetEirInfo->eirEnclInfo.eirEnclIndividual.location.bus, 
                                  pGetEirInfo->eirEnclInfo.eirEnclIndividual.location.enclosure, status);

            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;
        }

        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.inputPower =
                fbe_encl_mgmt_roundValueU32Tenths(pEnclInfo->eir_data.enclCurrentInputPower.inputPower);
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.inputPower =
                fbe_encl_mgmt_roundValueU32Tenths(pEnclInfo->eir_data.enclMaxInputPower.inputPower);
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.inputPower =
                fbe_encl_mgmt_roundValueU32Tenths(pEnclInfo->eir_data.enclAverageInputPower.inputPower);

        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentInputPower.status =
            pEnclInfo->eir_data.enclCurrentInputPower.status;
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxInputPower.status =
            pEnclInfo->eir_data.enclMaxInputPower.status;
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageInputPower.status =
            pEnclInfo->eir_data.enclAverageInputPower.status;

        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentAirInletTemp.airInletTemp =
                fbe_encl_mgmt_roundValueU32HundredTimes(pEnclInfo->eir_data.currentAirInletTemp.airInletTemp);
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxAirInletTemp.airInletTemp =
                fbe_encl_mgmt_roundValueU32HundredTimes(pEnclInfo->eir_data.maxAirInletTemp.airInletTemp);
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageAirInletTemp.airInletTemp =
                fbe_encl_mgmt_roundValueU32HundredTimes(pEnclInfo->eir_data.averageAirInletTemp.airInletTemp);

        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclCurrentAirInletTemp.status =
                pEnclInfo->eir_data.currentAirInletTemp.status;
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclMaxAirInletTemp.status =
                pEnclInfo->eir_data.maxAirInletTemp.status;
        pGetEirInfo->eirEnclInfo.eirEnclIndividual.eirInfo.enclAverageAirInletTemp.status =
                pEnclInfo->eir_data.averageAirInletTemp.status;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
}// end of fbe_encl_mgmt_get_eir_info

/**************************************************************************
*  fbe_encl_mgmt_roundValueU32Tenths()
**************************************************************************
*
*  DESCRIPTION:
*      These functions round a value in tenths of units to the nearest
*      whole unit.
*
*  PARAMETERS:
*
*
*  RETURN VALUES/ERRORS:
*
*
*  NOTES:
*
*
*  HISTORY:
*      09-May-2012:  Created by Dongz
*
**************************************************************************/
fbe_u32_t fbe_encl_mgmt_roundValueU32Tenths(UINT_32 valueInTenths)
{
    fbe_u32_t     valueInWholeUnits;

    valueInWholeUnits = (valueInTenths / 10);
    if ((valueInTenths % 10) >= 5)
    {
        valueInWholeUnits++;
    }
    return valueInWholeUnits;

}   // end of fbe_encl_mgmt_roundValueU32Tenths

/**************************************************************************
*  fbe_encl_mgmt_roundValueU32HundredTimes()
**************************************************************************
*
*  DESCRIPTION:
*      These functions round a value in 100 times of units to the nearest
*      whole unit.
*
*  PARAMETERS:
*
*
*  RETURN VALUES/ERRORS:
*
*
*  NOTES:
*
*
*  HISTORY:
*      09-May-2012:  Created by Dongz
*
**************************************************************************/
fbe_u32_t fbe_encl_mgmt_roundValueU32HundredTimes(UINT_32 valueInHundredTimes)
{
    fbe_u32_t     valueInWholeUnits;

    valueInWholeUnits = (valueInHundredTimes / 100);
    if ((valueInHundredTimes % 100) >= 50)
    {
        valueInWholeUnits++;
    }
    return valueInWholeUnits;

}   // end of fbe_encl_mgmt_roundValueU32HundredTimes

/*!***************************************************************
 * fbe_encl_mgmt_getCacheStatus()
 ****************************************************************
 * @brief
 *  This function processes the get Cache Status Control Code.
 *  
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Mar-2011 - Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_getCacheStatus(fbe_encl_mgmt_t * pEnclMgmt, 
                                                 fbe_packet_t * packet)
{
    fbe_common_cache_status_t           * pGetCacheStatus = NULL;
    fbe_common_cache_status_t           enclPeerCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_payload_control_buffer_length_t   len = 0;
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGetCacheStatus);
    if (pGetCacheStatus == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_common_cache_status_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %llu.\n", 
                               __FUNCTION__, len, (unsigned long long)sizeof(fbe_common_cache_status_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *) pEnclMgmt,
                                                      &enclPeerCacheStatus);

    *pGetCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *) pEnclMgmt,
                                                                 pEnclMgmt->enclCacheStatus,
                                                                 enclPeerCacheStatus,
                                                                 FBE_CLASS_ID_ENCL_MGMT);

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, pGetCacheStatus %d\n",
                          __FUNCTION__, *pGetCacheStatus);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;

}   // end of fbe_encl_mgmt_getCacheStatus

/*!***************************************************************
 * fbe_encl_mgmt_set_led_flash()
 ****************************************************************
 * @brief
 *  This function processes the get set LED flash Control Code.
 *  
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  24-Jan-2012 - Created. bphilbin
 *  
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_set_led_flash(fbe_encl_mgmt_t * pEnclMgmt, fbe_packet_t * packet)
{
    fbe_esp_encl_mgmt_set_led_flash_t   *pFlashCommand = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pFlashCommand);
    if (pFlashCommand == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n", 
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_encl_mgmt_set_led_flash_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %llu.\n", 
                               __FUNCTION__, len,
                               (unsigned long long)sizeof(fbe_esp_encl_mgmt_set_led_flash_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_send_led_flash_cmd(pEnclMgmt, pFlashCommand);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}



/*!***************************************************************
 * fbe_encl_mgmt_set_led_flash()
 ****************************************************************
 * @brief
 *  This function sends the request to physical package to flash
 *  the LEDs for an enclosure.
 *  
 *
 * @param  - pFlashCommand - pointer to flash LED structure.
 *
 * @return fbe_status_t
 *
 * @author
 *  24-Jan-2012 - Created. bphilbin
 *  
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_send_led_flash_cmd(fbe_encl_mgmt_t *pEnclMgmt, 
                                                     fbe_esp_encl_mgmt_set_led_flash_t *pFlashCommand)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_base_object_mgmt_set_enclosure_led_t    enclosureLedInfo;
    fbe_encl_info_t                             *pEnclInfo = NULL;
    fbe_base_enclosure_led_status_control_t     ledCommandInfo;
    fbe_u32_t                                   slot;

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &pFlashCommand->location, &pEnclInfo);

    

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                              __FUNCTION__, 
                              pFlashCommand->location.bus, 
                              pFlashCommand->location.enclosure, status);
        
        return status;
    }

    if(pFlashCommand->ledControlType == FBE_ENCLOSURE_FLASH_ALL_DRIVE_SLOTS)
    {
        fbe_zero_memory(&enclosureLedInfo, sizeof(fbe_base_object_mgmt_set_enclosure_led_t));
        if(pFlashCommand->enclosure_led_flash_on == FBE_TRUE)
        {
            enclosureLedInfo.flashLedsActive = FBE_TRUE;
            enclosureLedInfo.flashLedsOn = FBE_TRUE;
        }
        else
        {
            enclosureLedInfo.flashLedsActive = FBE_TRUE;
            enclosureLedInfo.flashLedsOn = FBE_FALSE;

        }
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, encl %d_%d, sending flash command for all drive slots.\n",
                              __FUNCTION__, 
                              pFlashCommand->location.bus, 
                              pFlashCommand->location.enclosure);
        status = fbe_api_enclosure_setLeds(pEnclInfo->object_id, &enclosureLedInfo);
    }
    else if(pFlashCommand->ledControlType == FBE_ENCLOSURE_FLASH_ENCL_FLT_LED)
    {
        fbe_zero_memory(&ledCommandInfo, sizeof(fbe_base_enclosure_led_status_control_t));
        ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
        if(pFlashCommand->enclosure_led_flash_on == FBE_TRUE)
        {
            ledCommandInfo.ledInfo.enclFaultLed = FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON;
        }
        else
        {
            ledCommandInfo.ledInfo.enclFaultLed = FBE_ENCLOSURE_LED_BEHAVIOR_MARK_OFF;
        }
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, encl %d_%d, sending flash command for enclosure fault LED.\n",
                              __FUNCTION__, 
                              pFlashCommand->location.bus, 
                              pFlashCommand->location.enclosure);
        status = fbe_api_enclosure_send_set_led(pEnclInfo->object_id, &ledCommandInfo);
    }
    else if(pFlashCommand->ledControlType == FBE_ENCLOSURE_FLASH_DRIVE_SLOT_LIST)
    {
        fbe_zero_memory(&ledCommandInfo, sizeof(fbe_base_enclosure_led_status_control_t));
        ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
        for(slot=0;slot<FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS;slot++)
        {
            if(pFlashCommand->slot_led_flash_on[slot] == FBE_TRUE)
            {
                ledCommandInfo.ledInfo.enclDriveFaultLeds[slot] = FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON;
            }
            else
            {
                ledCommandInfo.ledInfo.enclDriveFaultLeds[slot] = FBE_ENCLOSURE_LED_BEHAVIOR_MARK_OFF;
            }
        }
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, encl %d_%d, sending flash command for drive slot list.\n",
                              __FUNCTION__, 
                              pFlashCommand->location.bus, 
                              pFlashCommand->location.enclosure);
        status = fbe_api_enclosure_send_set_led(pEnclInfo->object_id, &ledCommandInfo);
    }

    
    return status;
}

/*!***************************************************************
 * fbe_encl_mgmt_get_eir_power_input_sample()
 ****************************************************************
 * @brief
 *  This function processes the get EIR input power sample info
 *  Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  09-May-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_eir_power_input_sample(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_esp_encl_mgmt_eir_input_power_sample_t * pGetPowerSampleInfo = NULL;
    fbe_payload_control_buffer_length_t   len = 0;
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_encl_info_t                     * pEnclInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_u32_t                             sampleIndex;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pGetPowerSampleInfo);
    if (pGetPowerSampleInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_encl_mgmt_eir_input_power_sample_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_esp_encl_mgmt_get_eir_info_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Find enclosure (should already exist)
     */
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt,
                                        &(pGetPowerSampleInfo->location),
                                                     &pEnclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                              __FUNCTION__,
                              pGetPowerSampleInfo->location.bus,
                              pGetPowerSampleInfo->location.enclosure, status);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    for(sampleIndex = 0; sampleIndex < pEnclMgmt->eirSampleCount; sampleIndex++)
    {
        pGetPowerSampleInfo->inputSample[sampleIndex].status =
                pEnclInfo->eirSampleData->inputPowerSamples[sampleIndex].status;
        pGetPowerSampleInfo->inputSample[sampleIndex].inputPower =
                fbe_encl_mgmt_roundValueU32Tenths(
                        pEnclInfo->eirSampleData->inputPowerSamples[sampleIndex].inputPower);
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}// end of fbe_encl_mgmt_get_eir_power_input_sample

/*!***************************************************************
 * fbe_encl_mgmt_get_eir_temp_sample()
 ****************************************************************
 * @brief
 *  This function processes the get EIR airinlet temp info Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  09-May-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_eir_temp_sample(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_esp_encl_mgmt_eir_temp_sample_t        * pGetTempSampleInfo = NULL;
    fbe_payload_control_buffer_length_t   len = 0;
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_encl_info_t                     * pEnclInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_u32_t                             sampleIndex;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pGetTempSampleInfo);
    if (pGetTempSampleInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_encl_mgmt_eir_input_power_sample_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_esp_encl_mgmt_get_eir_info_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Find enclosure (should already exist)
     */
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt,
                                        &(pGetTempSampleInfo->location),
                                                     &pEnclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                              __FUNCTION__,
                              pGetTempSampleInfo->location.bus,
                              pGetTempSampleInfo->location.enclosure, status);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    for(sampleIndex = 0; sampleIndex < pEnclMgmt->eirSampleCount; sampleIndex++)
    {
        pGetTempSampleInfo->airInletTempSamples[sampleIndex].status =
                pEnclInfo->eirSampleData->airInletTempSamples[sampleIndex].status;
        pGetTempSampleInfo->airInletTempSamples[sampleIndex].airInletTemp =
                fbe_encl_mgmt_roundValueU32HundredTimes(pEnclInfo->eirSampleData->airInletTempSamples[sampleIndex].airInletTemp);
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}// end of fbe_encl_mgmt_get_eir_temp_sample

/*!***************************************************************
 * fbe_encl_mgmt_get_rp_wwn_seed()
 ****************************************************************
 * @brief
 *  This function processes the get resume prom wwn seed Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  16-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_array_midplane_rp_wwn_seed(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_u32_t                             * wwn_seed = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_control_operation_t       * control_operation = NULL;
    fbe_encl_info_t                       * pEnclInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &wwn_seed);
    if (wwn_seed == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_u32_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    //get wwn seed
    if(pEnclInfo->enclResumeInfo.op_status != FBE_RESUME_PROM_STATUS_READ_SUCCESS)
    {
        *wwn_seed = INVALID_WWN_SEED;
    }
    else
    {
        *wwn_seed = pEnclInfo->enclResumeInfo.resume_prom_data.wwn_seed;
    }

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, get wwn seed: %d, status %d.\n",
                          __FUNCTION__, *wwn_seed, pEnclInfo->enclResumeInfo.op_status);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_encl_mgmt_get_user_modified_wwn_seed_flag()
 ****************************************************************
 * @brief
 *  This function processes the get user modified wwn seed flag Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  16-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_user_modified_wwn_seed_flag(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_bool_t                             * user_modifyed_wwn_seed_flag = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_control_operation_t       * control_operation = NULL;
    fbe_encl_info_t                       * pEnclInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &user_modifyed_wwn_seed_flag);
    if (user_modifyed_wwn_seed_flag == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_bool_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_bool_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    //get wwn seed
    status = fbe_encl_mgmt_reg_get_user_modified_wwn_seed_info(pEnclMgmt, user_modifyed_wwn_seed_flag);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, failed to get wwn seed, status: %d.\n",
                              __FUNCTION__, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_encl_mgmt_set_rp_wwn_seed()
 ****************************************************************
 * @brief
 *  This function processes the set wwn seed Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  16-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_set_array_midplane_rp_wwn_seed(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_u32_t                             * wwn_seed = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_control_operation_t       * control_operation = NULL;
    fbe_encl_info_t                       * pEnclInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &wwn_seed);
    if (wwn_seed == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_u32_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Get Processor Enclosure Info
     */

    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, write wwn seed: 0x%x.\n",
                          __FUNCTION__, *wwn_seed);

    status = fbe_encl_mgmt_resume_prom_write(pEnclMgmt,
            pEnclInfo->location,
            (fbe_u8_t *)wwn_seed,
            sizeof(fbe_u32_t),
            RESUME_PROM_WWN_SEED,
            FBE_DEVICE_TYPE_ENCLOSURE,
            TRUE);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, resume prom write fail.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_encl_mgmt_user_modify_wwn_seed()
 ****************************************************************
 * @brief
 *  This function processes the user modify wwn seed Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  16-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_user_modify_array_midplane_rp_wwn_seed(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_u32_t                             * wwn_seed = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_control_operation_t       * control_operation = NULL;
    fbe_encl_info_t                       * pEnclInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_t cmi_packet = {0};

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &wwn_seed);
    if (wwn_seed == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_u32_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, write wwn seed: %d.\n",
                          __FUNCTION__, *wwn_seed);

    status = fbe_encl_mgmt_resume_prom_write(pEnclMgmt,
                                             pEnclInfo->location,
                                            (fbe_u8_t *)wwn_seed,
                                             sizeof(fbe_u32_t),
                                             RESUME_PROM_WWN_SEED,
                                             FBE_DEVICE_TYPE_ENCLOSURE,
                                             TRUE);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, resume prom write fail.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_event_log_write(ESP_INFO_USER_MODIFIED_WWN_SEED,
                                 NULL,
                                 0,
                                 "%x",
                                 *wwn_seed);

    //set user modified wwn seed flag
    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, set user modified wwn seed flag.\n",
                          __FUNCTION__);
    status = fbe_encl_mgmt_reg_set_user_modified_wwn_seed_info(pEnclMgmt, TRUE);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fail to write user modified wwn seed flag.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    cmi_packet.baseCmiMsg.opcode = FBE_BASE_ENV_CMI_MSG_USER_MODIFY_WWN_SEED_FLAG_CHANGE;
    cmi_packet.user_modified_wwn_seed_flag = TRUE;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, send cmi message to set peer user modified wwn seed flag.\n",
                          __FUNCTION__);

    //send message to peer
    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)pEnclMgmt,
                                           sizeof(fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_t),
                                          (fbe_cmi_message_t)&cmi_packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fail to send cmi message.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_encl_mgmt_clear_user_modified_wwn_seed_flag()
 ****************************************************************
 * @brief
 *  This function processes the clear user modified wwn seed flag Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  16-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_clear_user_modified_wwn_seed_flag(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_encl_info_t                       * pEnclInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_t cmi_packet = {0};

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    //set user modified wwn seed flag to false
    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, clear user modified wwn seed flag.\n",
                          __FUNCTION__);
    status = fbe_encl_mgmt_reg_set_user_modified_wwn_seed_info(pEnclMgmt, FALSE);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fail to write user modified wwn seed flag.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
    cmi_packet.baseCmiMsg.opcode = FBE_BASE_ENV_CMI_MSG_USER_MODIFY_WWN_SEED_FLAG_CHANGE;
    cmi_packet.user_modified_wwn_seed_flag = FALSE;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, send cmi message to clear peer user modified wwn seed flag.\n",
                          __FUNCTION__);

    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)pEnclMgmt,
                                           sizeof(fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_t),
                                           (fbe_cmi_message_t) &cmi_packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fail to send cmi message.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_encl_mgmt_modify_system_id_info()
 ****************************************************************
 * @brief
 *  This function processes the get set system id info Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  09-May-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_modify_system_id_info(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_esp_encl_mgmt_modify_system_id_info_t        * pModifySystemIdInfo = NULL;
    fbe_encl_mgmt_modify_system_id_info_t              modifySystemIdCmd = {0};
    fbe_payload_control_buffer_length_t   len = 0;
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_encl_info_t                     * pEnclInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pModifySystemIdInfo);
    if (pModifySystemIdInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_encl_mgmt_modify_system_id_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_esp_encl_mgmt_modify_system_id_info_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_event_log_write(ESP_INFO_USER_CHANGE_SYS_ID_INFO,
                NULL, 0,
                "%s, %d, %s, %d",
                pModifySystemIdInfo->serialNumber,
                pModifySystemIdInfo->changeSerialNumber,
                pModifySystemIdInfo->partNumber,
                pModifySystemIdInfo->changePartNumber);

    //move data into internal structure

    modifySystemIdCmd.changePartNumber = pModifySystemIdInfo->changePartNumber;
    modifySystemIdCmd.changeSerialNumber = pModifySystemIdInfo->changeSerialNumber;
    fbe_copy_memory(modifySystemIdCmd.partNumber,
            pModifySystemIdInfo->partNumber,
            RESUME_PROM_PRODUCT_PN_SIZE);
    fbe_copy_memory(modifySystemIdCmd.serialNumber,
            pModifySystemIdInfo->serialNumber,
            RESUME_PROM_PRODUCT_SN_SIZE);

    status = fbe_encl_mgmt_set_resume_prom_system_id_info(pEnclMgmt, pEnclInfo->location, &modifySystemIdCmd);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, set resume prom system id info fail!\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    //set user modified system id flag in the registery of the local SP.
    status = fbe_encl_mgmt_reg_set_user_modified_system_id_flag(pEnclMgmt, TRUE);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, set user modified system id flag fail!\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    //Notify peer to set the userModifiedSystemId flag.
    status = fbe_encl_mgmt_notify_peer_to_set_user_modified_system_id_flag(pEnclMgmt, TRUE);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, notify_peer_to_set_user_modified_system_id_flag failed!\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}// end of fbe_encl_mgmt_modify_system_id_info

/*!***************************************************************
 * fbe_encl_mgmt_notify_peer_to_set_user_modified_system_id_flag()
 ****************************************************************
 * @brief
 *  This function sends the CMI message to the peer SP to notify
 *  the userModifiedSystemId Flag needs to be set.
 *
 * @param  pEnclMgmt - 
 * @param  user_modified_sys_id_flag - 
 * 
 * @return fbe_status_t
 *
 * @author
 *  18-Apr-2013 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_notify_peer_to_set_user_modified_system_id_flag(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_bool_t user_modified_sys_id_flag)
{
    fbe_encl_mgmt_user_modified_sys_id_flag_cmi_packet_t cmi_packet = {0};
    fbe_status_t                                         status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "Sending cmi message to set peer user modified system id flag to %d.\n",
                          user_modified_sys_id_flag);

    cmi_packet.baseCmiMsg.opcode = FBE_BASE_ENV_CMI_MSG_USER_MODIFIED_SYS_ID_FLAG;
    cmi_packet.user_modified_sys_id_flag = user_modified_sys_id_flag;

    //send message to peer
    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)pEnclMgmt,
                                           sizeof(fbe_encl_mgmt_user_modified_sys_id_flag_cmi_packet_t),
                                          (fbe_cmi_message_t)&cmi_packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fail to send cmi message.\n",
                              __FUNCTION__);
        
    }

    return status;
}

/*!***************************************************************
 * fbe_encl_mgmt_get_system_id_info()
 ****************************************************************
 * @brief
 *  This function processes the get system id info Control Code.
 *
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_system_id_info(fbe_encl_mgmt_t * pEnclMgmt,
                                               fbe_packet_t * packet)
{
    fbe_esp_encl_mgmt_system_id_info_t        * pSystemIdInfo = NULL;
    fbe_payload_control_buffer_length_t   len = 0;
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;
    fbe_encl_info_t                     * pEnclInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pSystemIdInfo);
    if (pSystemIdInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_esp_encl_mgmt_system_id_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, invalid len %d, expected %d.\n",
                               __FUNCTION__, len, (int)sizeof(fbe_esp_encl_mgmt_system_id_info_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_NO_DEVICE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    //get system info
    fbe_copy_memory(pSystemIdInfo->partNumber,
            pEnclMgmt->system_id_info.product_part_number,
            RESUME_PROM_PRODUCT_PN_SIZE);

    fbe_copy_memory(pSystemIdInfo->serialNumber,
            pEnclMgmt->system_id_info.product_serial_number,
            RESUME_PROM_PRODUCT_PN_SIZE);

    fbe_copy_memory(pSystemIdInfo->revision,
            pEnclMgmt->system_id_info.product_revision,
            RESUME_PROM_PRODUCT_REV_SIZE);

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, system id: %.16s, %.16s, %.3s.\n",
                          __FUNCTION__,
                          pSystemIdInfo->partNumber, pSystemIdInfo->serialNumber, pSystemIdInfo->revision);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}// end of fbe_encl_mgmt_get_system_id_info


/*!***************************************************************
 * fbe_encl_mgmt_updateLccFailStatus()
 ****************************************************************
 * @brief
 *  This function will set the appropriate LCC failed status.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  24-Apr-2014 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_encl_mgmt_updateLccFailStatus(fbe_u8_t *mgmtPtr,
                                  fbe_device_physical_location_t *pLocation,
                                  fbe_base_environment_failure_type_t failureType,
                                  fbe_bool_t newValue)
{
    fbe_status_t        status;
    fbe_encl_mgmt_t     *pEnclMgmt;
    fbe_lcc_info_t      *pLccInfo = NULL;
    fbe_u8_t            deviceStr[FBE_DEVICE_STRING_LENGTH]={0};

    pEnclMgmt = (fbe_encl_mgmt_t *)mgmtPtr;
    status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt,
                                            pLocation,
                                            &pLccInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_encl_mgmt_get_lcc_info_ptr failed, status 0x%x\n",
                              __FUNCTION__, status);
        return FBE_STATUS_COMPONENT_NOT_FOUND;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_LCC, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    switch (failureType)
    {
    case FBE_BASE_ENV_RP_READ_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, resPromReadFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              pLccInfo->resumePromReadFailed,
                              newValue);
        pLccInfo->resumePromReadFailed = newValue;
        break;
    case FBE_BASE_ENV_FUP_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, fupFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              pLccInfo->fupFailure,
                              newValue);
        pLccInfo->fupFailure = newValue;
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Unsupported failureType %d.\n", 
                              __FUNCTION__, failureType); 
        return FBE_STATUS_COMPONENT_NOT_FOUND;
        break;
    }

    return FBE_STATUS_OK;

}   // end of fbe_encl_mgmt_updateLccFailStatus
