#ifndef FBE_ESP_ENCL_MGMT_H
#define FBE_ESP_ENCL_MGMT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_esp_encl_mgmt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the ESP Enclosure Mgmt
 *  object.
 * 
 * @ingroup esp_encl_mgmt_class_files
 * 
 * @revision
 *   03/24/2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_enclosure_interface.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup esp_encl_mgmt_class ESP Encl MGMT Class
 *  @brief This is the set of definitions for the enclosure
 *  management class.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup esp_encl_mgmt_usurper_interface ESP Encl MGMT 
 *  Usurper Interface 
 *  @brief This is the set of definitions that comprise the
 *  enclosure management class usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */

/*!********************************************************************* 
 * @enum fbe_esp_encl_mgmt_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP Encl Mgmt specific control
 * codes which the Enclosure Mgmt object will accept.
 *         
 **********************************************************************/
typedef enum
{
    FBE_ESP_ENCL_MGMT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_ENCL_MGMT),

    // retrieve Enclosure Info from ESP
    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_INFO,
    // retrieve LCC Info from ESP
    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_INFO,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INFO,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_TOTAL_ENCL_COUNT,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_LOCATION,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_COUNT_ON_BUS,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_COUNT,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_PS_COUNT,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_FAN_COUNT,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_DRIVE_SLOT_COUNT,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_CACHE_STATUS,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_CONNECTOR_COUNT,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_CONNECTOR_INFO,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_FLASH_ENCL_LEDS,

    FBE_ESP_CONTROL_CODE_GET_BUS_INFO,

    FBE_ESP_CONTROL_CODE_GET_MAX_BE_BUS_COUNT,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INPUT_POWER_SAMPLE,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_TEMP_SAMPLE,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ARRAY_MIDPLANE_RP_WWN_SEED,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_SET_ARRAY_MIDPLANE_RP_WWN_SEED,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_USER_MODIFIED_WWN_SEED_FLAG,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_CLEAR_USER_MODIFIED_WWN_SEED_FLAG,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_USER_MODIFY_ARRAY_MIDPLANE_WWN_SEED,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_MODIFY_SYSTEM_ID_INFO,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SYSTEM_ID_INFO,

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_PRESENCE,
    
    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SSC_COUNT,
    
    FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SSC_INFO,
    /* Insert new control codes here. */

    FBE_ESP_ENCL_MGMT_CONTROL_CODE_LAST
}
fbe_esp_encl_mgmt_control_code_t;

// FBE_ESP_CONTROL_CODE_GET_BUS_INFO
typedef struct fbe_esp_get_bus_info_s
{
    fbe_u32_t           bus;
    fbe_esp_bus_info_t  busInfo;
} fbe_esp_get_bus_info_t;

// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_COUNT_ON_BUS
typedef struct fbe_esp_encl_mgmt_get_encl_count_on_bus_s
{
    fbe_device_physical_location_t location; 
    fbe_u32_t                      enclCountOnBus; 
} fbe_esp_encl_mgmt_get_encl_count_on_bus_t;



// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_INFO
typedef struct fbe_esp_encl_mgmt_get_encl_info_s
{
    fbe_device_physical_location_t location; 
    fbe_esp_encl_mgmt_encl_info_t  enclInfo;
} fbe_esp_encl_mgmt_get_encl_info_t;

// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_PRESENCE
typedef struct fbe_esp_encl_mgmt_get_encl_presence_s
{
    fbe_device_physical_location_t location;   // Input
    fbe_bool_t                     enclPresent;
} fbe_esp_encl_mgmt_get_encl_presence_t;

// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_INFO
typedef struct fbe_esp_encl_mgmt_get_lcc_info_s
{
    fbe_device_physical_location_t location; 
    fbe_esp_encl_mgmt_lcc_info_t   lccInfo;
} fbe_esp_encl_mgmt_get_lcc_info_t;

// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_CONNECTOR_INFO
typedef struct fbe_esp_encl_mgmt_get_connector_info_s
{
    fbe_device_physical_location_t location; 
    fbe_esp_encl_mgmt_connector_info_t   connectorInfo;
} fbe_esp_encl_mgmt_get_connector_info_t;

//FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SSC_INFO
typedef struct fbe_esp_encl_mgmt_get_ssc_info_s
{
    fbe_device_physical_location_t location;
    fbe_esp_encl_mgmt_ssc_info_t   sscInfo;
} fbe_esp_encl_mgmt_get_ssc_info_t;

// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_COUNT
// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_PS_COUNT
// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_FAN_COUNT
// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_DRIVE_SLOT_COUNT
typedef struct fbe_esp_encl_mgmt_get_device_count_s
{
    fbe_device_physical_location_t location; 
    fbe_u32_t                      deviceCount;
} fbe_esp_encl_mgmt_get_device_count_t;

#endif /* FBE_ESP_ENCL_MGMT_H */

#define FBE_ENCL_MGMT_USER_MODIFIED_WWN_SEED_INFO_KEY "ESPEnclMgmtUserModifiedWwnSeedInfo"

/*******************************
 * end file fbe_esp_encl_mgmt.h
 *******************************/
