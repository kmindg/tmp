#ifndef FBE_API_ESP_ENCL_MGMT_INTERFACE_H
#define FBE_API_ESP_ENCL_MGMT_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_encl_mgmt_interface.h
 ***************************************************************************
 *
 * @brief
 * 	This header file defines the FBE API for the ESP ENCL Mgmt object.
 *
 * @ingroup fbe_api_esp_interface_class_files
 *
 * @version
 *   03/16/2010:  Created. Joe Perry
 ****************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
>>>
>>> Include files go here
>>>

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs
 *  @brief
 *    This is the set of definitions for FBE ESP APIs.
 *
 *  @ingroup fbe_api_esp_interface
 */
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API LUN Interface for the Usurper Interface.
// This is where all the data structures defined.
//----------------------------------------------------------------
/*! @defgroup fbe_api_lun_interface_usurper_interface FBE API LUN Interface Usurper Interface
 *  @brief
 *    This is the set of definitions that comprise the FBE API LUN Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */
//----------------------------------------------------------------
>>>>>
>>>>> These are where all the "NAMED CONSTANTS (#defines):", "ENUMERATIONS:", "STRUCT", or "TYPEDEF"
>>>>> defined.
>>>>>
>>>>> Please put the comments (/*!< Enclosure type. */) next to the define variables
>>>>>
>>>>> Example of STRUCT definition:
>>>>> /*!*********************************************************************
>>>>>  * @struct fbe_esp_encl_mgmt_encl_info_t
>>>>>  *
>>>>>  * @brief
>>>>>  *   Contains the enclosure info.
>>>>>  *
>>>>>  * @ingroup fbe_api_esp_encl_mgmt_interface
>>>>>  * @ingroup fbe_esp_encl_mgmt_encl_info
>>>>>  **********************************************************************/
>>>>>  typedef struct fbe_esp_encl_mgmt_encl_info_s
>>>>>  {
>>>>>      fbe_device_physical_location_t location; /*!< IN - Set by the caller for the device we want the info about */
>>>>>
>>>>>      fbe_enclosure_types_t encl_type; /*!< Enclosure type. */
>>>>>      fbe_u8_t serial_number[RESUME_PROM_EMC_SERIAL_NUM_SIZE + 1]; /*!< Serial number. */
>>>>>  } fbe_esp_encl_mgmt_encl_info_t;
//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):
//
>>>>> Example of the #define (/*! @def <definition> */)
>>>>> /*!*********************************************************************
>>>>>  * @def IO_COMPLETION_IDLE_TIMER
>>>>>  *
>>>>>  * @brief
>>>>>  *   This represents io completion thread delay in msec
>>>>>  *
>>>>>  * @ingroup fbe_api_common_sim_interface
>>>>>  **********************************************************************/
>>>>>  #define IO_COMPLETION_IDLE_TIMER 2 /* io completion thread delay in msec.*/

//-----------------------------------------------------------------------------
//  ENUMERATIONS:
//
>>>>> Document the "enum":  /*! @enum <name> */
>>>>> Example:
>>>>> /*!*********************************************************************
>>>>>  * @enum thread_flag_t
>>>>>  *
>>>>>  * @brief
>>>>>  *   This contains the enum data info for Thread flag.
>>>>>  *
>>>>>  * @ingroup fbe_api_common_user_interface
>>>>>  * @ingroup thread_flag
>>>>>  **********************************************************************/
>>>>>  typedef enum thread_flag_e{
>>>>>      THREAD_RUN,  /*!< Run Thread */
>>>>>      THREAD_STOP, /*!< Stop Thread */
>>>>>      THREAD_DONE  /*!< Done Thread */
>>>>>  }thread_flag_t;

//-----------------------------------------------------------------------------
//  TYPEDEFS:
//
>>>>> - Document the "typedef":  /*! @typedef <typedef declaration> */
>>>>> Example:
>>>>> /*!*********************************************************************
>>>>>  * @typedef void (* event_notification_function_t)(void)
>>>>>  *
>>>>>  * @ingroup fbe_api_esp_sps_mgmt_interface_usurper_interface
>>>>>  **********************************************************************/
>>>>>  typedef void (* event_notification_function_t)(void);
...
...
>>>  When done with all the data structures, typedef, #define, etc, the below line
>>>  need to be added, "/*! @} */
>>>
/*! @} */ /* end of group fbe_api_esp_sps_mgmt_interface_usurper_interface */


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES:
//

//----------------------------------------------------------------
// Define the group for the FBE API ESP SPS Mgmt Interface.
// This is where all the function prototypes for the FBE API ESP SPS Mgmt.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_sps_mgmt_interface FBE API ESP SPS Mgmt Interface
 *  @brief
 *    This is the set of FBE API ESP SPS Mgmt Interface.
 *
 *  @details
 *    In order to access this library, please include fbe_api_esp_sps_mgmt_interface.h.
 *
 *  @ingroup fbe_api_esp_interface_class
 *  @{
 */
//----------------------------------------------------------------
>>>>
>>>> Add all function prototypes here
>>>> Example:
>>>>
>>>> fbe_status_t FBE_API_CALL
>>>> fbe_api_esp_sps_mgmt_getSpsStatus(fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo);
>>>>
>>>> Need to add this, "/* @} */ at the last function prototype.
>>>>
/*! @} */ /* end of group fbe_api_esp_sps_mgmt_interface */

//----------------------------------------------------------------
// Define the group for all FBE ESP Package APIs Interface class files.
// This is where all the class files that belong to the FBE API ESP
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class_files FBE ESP APIs Interface Class Files
 *  @brief
 *    This is the set of files for the FBE ESP Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------
