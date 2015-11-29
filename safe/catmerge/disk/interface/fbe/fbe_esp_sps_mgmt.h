#ifndef FBE_ESP_SPS_MGMT_H
#define FBE_ESP_SPS_MGMT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_esp_sps_mgmt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the ESP SPS Mgmt object.
 * 
 * @ingroup esp_sps_mgmt_class_files
 * 
 * @revision
 *   03/16/2010:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup esp_sps_mgmt_class ESP SPS MGMT Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup esp_sps_mgmt_usurper_interface ESP SPS MGMT Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */

/*!********************************************************************* 
 * @enum fbe_esp_sps_mgmt_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP SPS Mgmt specific control codes.
 * These are control codes specific to the ESP SPS Mgmt object, which only
 * the SPS Mgmt object will accept.
 *         
 **********************************************************************/
typedef enum
{
	FBE_ESP_SPS_MGMT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_SPS_MGMT),

    // retrieve SPS Status from ESP
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_STATUS,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_TEST_TIME,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_TEST_TIME,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER_TOTAL,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_MANUF_INFO,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_CACHE_STATUS,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_SPS_POWERDOWN,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_SPS_VERIFY_POWERDOWN,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SIMULATE_SPS,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SIMULATE_SPS,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_COUNT,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_BOB_COUNT,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_BOB_STATUS,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_INPUT_POWER_SAMPLE,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_LOCAL_BATTERY_TIME,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_INITIATE_SELF_TEST,
    FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_OBJECT_DATA,
    /* Insert new control codes here. */

	FBE_ESP_SPS_MGMT_CONTROL_CODE_LAST
}
fbe_esp_sps_mgmt_control_code_t;




#endif /* FBE_ESP_SPS_MGMT_H */

/*************************
 * end file fbe_esp_sps_mgmt.h
 *************************/
