#ifndef FBE_ESP_COOLING_MGMT_H
#define FBE_ESP_COOLING_MGMT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_esp_cooling_mgmt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the ESP COOLING Mgmt object.
 * 
 * @ingroup esp_cooling_mgmt_class_files
 * 
 * @revision
 *   17-Jan-2011: PHE - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup esp_cooling_mgmt_class ESP COOLING MGMT Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup esp_cooling_mgmt_usurper_interface ESP COOLING MGMT Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */

/*!********************************************************************* 
 * @enum fbe_esp_cooling_mgmt_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP COOLING Mgmt specific control codes.
 * These are control codes specific to the ESP COOLING Mgmt object, which only
 * the COOLING Mgmt object will accept.
 *         
 **********************************************************************/
typedef enum
{
	FBE_ESP_COOLING_MGMT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_COOLING_MGMT),

    FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_FAN_INFO,

    FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_CACHE_STATUS,

    /* Insert new control codes here. */

	FBE_ESP_COOLING_MGMT_CONTROL_CODE_LAST
}
fbe_esp_cooling_mgmt_control_code_t;


// FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_FAN_INFO
typedef struct fbe_esp_cooling_mgmt_get_fan_info_s
{
    fbe_device_physical_location_t        location; 
    fbe_esp_cooling_mgmt_fan_info_t       fanInfo;  
} fbe_esp_cooling_mgmt_get_fan_info_t;

// FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_FAN_COUNT
typedef struct fbe_esp_cooling_mgmt_get_fan_count_s
{
    fbe_device_physical_location_t  location; 
    fbe_u32_t                       fanCount;
} fbe_esp_cooling_mgmt_get_fan_count_t;

#endif /* FBE_ESP_COOLING_MGMT_H */

/*************************
 * end file fbe_esp_cooling_mgmt.h
 *************************/

