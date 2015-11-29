#ifndef FBE_ESP_BOARD_MGMT_H
#define FBE_ESP_BOARD_MGMT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_esp_board_mgmt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains declarations for the ESP Board Mgmt object.
 * 
 * @ingroup esp_board_mgmt_class_files
 * 
 * @revision
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/
 /*! @defgroup esp_board_mgmt_class ESP Board MGMT Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup esp_board_mgmt_usurper_interface ESP Board MGMT Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */
/*!********************************************************************* 
 * @enum fbe_esp_board_mgmt_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP Board Mgmt specific control
 * codes which the Board Mgmt object will accept.
 *  
 **********************************************************************/
typedef enum
{
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BOARD_MGMT),

    // retrieve Board Info from ESP
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_BOARD_INFO,
    // reboot the SP and hold in POST or Reset
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_REBOOT_SP,
    // get the Board's Cache Status
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_STATUS,
    
    // Retrieve peer boot state from ESP
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_BOOT_INFO,

    // Retrieve peer CPU status from ESP
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_CPU_STATUS,

    // Retrieve peer DIMM status from ESP
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_DIMM_STATUS,

    // To get Suitcase info
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SUITCASE_INFO,

    // To get BMC info
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_BMC_INFO,

    // To get Cache Card count
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_CARD_COUNT,

    // To get Cache Card info
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_CARD_STATUS,

    // To flush system data
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_FLUSH_SYSTEM,

    // To get DIMM count
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_DIMM_COUNT,

    // To get DIMM info
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_DIMM_INFO,

    // To get SSD count
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SSD_COUNT,

    // To get SSD info
    FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SSD_INFO,

    /* Insert new control codes here. */

    FBE_ESP_BOARD_MGMT_CONTROL_CODE_LAST
}
fbe_esp_board_mgmt_control_code_t;

// FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_DIMM_INFO
typedef struct fbe_esp_board_mgmt_get_dimm_info_s
{
    fbe_device_physical_location_t location; 
    fbe_esp_board_mgmt_dimm_info_t dimmInfo;
} fbe_esp_board_mgmt_get_dimm_info_t;

// FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SSD_INFO
typedef struct fbe_esp_board_mgmt_get_ssd_info_s
{
    fbe_device_physical_location_t location; 
    fbe_esp_board_mgmt_ssd_info_t ssdInfo;
} fbe_esp_board_mgmt_get_ssd_info_t;

#endif /* FBE_ESP_BOARD_MGMT_H*/
