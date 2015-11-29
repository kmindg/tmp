#ifndef FBE_ESP_MODULE_MGMT_H
#define FBE_ESP_MODULE_MGMT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_esp_module_mgmt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the ESP Module Mgmt object.
 * 
 * @ingroup esp_module_mgmt_class_files
 * 
 * @revision
 *   03/16/2010:  Created. bphilbin
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

 /*! @defgroup esp_module_mgmt_class ESP MODULE MGMT Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup esp_module_mgmt_usurper_interface ESP MODULE MGMT Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */

/*!********************************************************************* 
 * @enum fbe_esp_module_mgmt_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP MODULE Mgmt specific control codes.
 * These are control codes specific to the ESP MODULE Mgmt object, which only
 * the MODULE Mgmt object will accept.
 *         
 **********************************************************************/
typedef enum
{
	FBE_ESP_MODULE_MGMT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_MODULE_MGMT),

    // retrieve MODULE Status from ESP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS,
    // retrieve the PORT Status from ESP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_PORT_STATUS,
    // retrieve the IO module info from ESP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_MODULE_INFO,
    // retrieve the Mezzanine info from ESP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MEZZANINE_INFO,
    // retrieve the io port info from ESP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_PORT_INFO,
    // retrieve the SFP info from ESP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_SFP_INFO,
    // retrieve the limits information from ESP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_LIMITS_INFO,
    // set port command
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_PORT_CONFIG,
    // set mgmt port speed
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_MGMT_PORT_CONFIG,
    // get mgmt comp info
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MGMT_COMP_INFO,
    // get config mgmt port info
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_REQUESTED_MGMT_PORT_CONFIG,
    // mark IO Module
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_MARK_IOM,
    // mark IO Port
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_MARK_IOPORT,
    // get port interrupt affinity settings
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_PORT_AFFINITY,
    // get the general module mgmt status for this SP
    FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_GENERAL_STATUS,
    

    /* Insert new control codes here. */

	FBE_ESP_MODULE_MGMT_CONTROL_CODE_LAST
}
fbe_esp_module_mgmt_control_code_t;




#endif /* FBE_ESP_MODULE_MGMT_H */

/*************************
 * end file fbe_esp_module_mgmt.h
 *************************/
