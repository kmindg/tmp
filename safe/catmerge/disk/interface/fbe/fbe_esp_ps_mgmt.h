#ifndef FBE_ESP_PS_MGMT_H
#define FBE_ESP_PS_MGMT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_esp_ps_mgmt.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the ESP PS Mgmt object.
 * 
 * @ingroup esp_ps_mgmt_class_files
 * 
 * @revision
 *   04/15/2010:  Created. Joe Perry
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

 /*! @defgroup esp_ps_mgmt_class ESP PS MGMT Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup esp_ps_mgmt_usurper_interface ESP PS MGMT Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */

/*!********************************************************************* 
 * @enum fbe_esp_ps_mgmt_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP PS Mgmt specific control codes.
 * These are control codes specific to the ESP PS Mgmt object, which only
 * the PS Mgmt object will accept.
 *         
 **********************************************************************/
typedef enum
{
	FBE_ESP_PS_MGMT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_PS_MGMT),

    // retrieve PS Status from ESP
    FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_COUNT,
    FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_STATUS,
    FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_CACHE_STATUS,
    FBE_ESP_PS_MGMT_CONTROL_CODE_PS_POWERDOWN,
    FBE_ESP_PS_MGMT_CONTROL_CODE_SET_EXPECTED_PS_TYPE,

    /* Insert new control codes here. */

	FBE_ESP_PS_MGMT_CONTROL_CODE_LAST
}
fbe_esp_ps_mgmt_control_code_t;




#endif /* FBE_ESP_PS_MGMT_H */

/*************************
 * end file fbe_esp_ps_mgmt.h
 *************************/
