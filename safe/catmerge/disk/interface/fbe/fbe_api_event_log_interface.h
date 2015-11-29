#ifndef FBE_API_EVENT_LOG_INTERFACE_H
#define FBE_API_EVENT_LOG_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_event_log_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains declaration of API use to interface with event log service.
 *  
 * @ingroup fbe_api_system_package_interface_class_files
 *
 * @version
 *   3 May - 2010 - Vaibhav Gaonkar Created
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"
//------------------------------------------------------------
//  Define the top level group for the FBE System APIs.
//------------------------------------------------------------
/*! @defgroup fbe_system_package_class FBE System APIs
 *  @brief   
 *    This is the set of definations for FBE System APIs.
 *  
 *  @ingroup fbe_api_system_package_interface
 */
//---------------------------------------------------------------- 
//----------------------------------------------------------------
// Define the FBE API System Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_interface_usurper_interface FBE API System Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API System class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------
/*!********************************************************************* 
 * @struct fbe_event_log_statistics_t 
 *  
 * @brief 
 *    Contains the event log statistics for package.
 *
 * @ingroup fbe_api_event_log_service_interface
 * @ingroup fbe_event_log_statistics
 *
 **********************************************************************/
typedef struct fbe_event_log_statistics_s {
    fbe_u32_t total_msgs_logged;    /*!< Number of Message that has been logged since the event log service was started */
    fbe_u32_t total_msgs_queued;    /*!<  Total number of Message that got queued because of error returned */
    fbe_u32_t total_msgs_dropped;   /*!< Total number of messages that were dropped because windows could not handle it */
    fbe_u32_t current_msgs_logged;  /*!< Current number of messages in the log ring buffer. */
} fbe_event_log_statistics_t;

/*!********************************************************************* 
 * @struct fbe_event_log_msg_count_t 
 *  
 * @brief 
 *   This is message ID structure defined to find out number of 
 *   particular message in event log.
 *
 * @ingroup fbe_api_event_log_service_interface
 * @ingroup fbe_event_log_msg_count
 *
 **********************************************************************/
typedef struct fbe_event_log_msg_count_s {
    fbe_u32_t msg_id;       /*!<IN*/
    fbe_u32_t msg_count;    /*!<OUT*/
}fbe_event_log_msg_count_t;

/*!********************************************************************* 
 * @struct fbe_event_log_check_msg_s 
 *  
 * @brief 
 *   This is structure defined to find out particular message 
 *   present in event log.
 *
 * @ingroup fbe_api_event_log_service_interface
 * @ingroup fbe_event_log_check_msg
 *
 **********************************************************************/
typedef struct fbe_event_log_check_msg_s{
    fbe_u32_t msg_id;                                       /*!<IN*/
    fbe_u8_t msg[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];      /*!<IN*/
    fbe_bool_t is_msg_present;                              /*!<OUT*/
}fbe_event_log_check_msg_t;

/*!********************************************************************* 
 * @struct fbe_event_log_get_event_log_info_s 
 *  
 * @brief 
 *   This is structure defined to find out event log info 
 *   present in event log.
 *
 * @ingroup fbe_api_event_log_service_interface
 * @ingroup fbe_event_log_get_event_log_info_s
 *
 **********************************************************************/
typedef struct fbe_event_log_get_event_log_info_s{
    fbe_u32_t event_log_index;                                       /*!<IN*/
    fbe_event_log_info_t event_log_info;                             /*!<OUT*/   
}fbe_event_log_get_event_log_info_t;
/*! @} */ /* end of group fbe_api_system_interface_usurper_interface */

/*!********************************************************************* 
 * @struct fbe_event_log_get_all_events_log_info_s 
 *  
 * @brief 
 *   This is structure defined to find out event log info 
 *   present in event log.
 *
 * @ingroup fbe_api_event_log_service_interface
 * @ingroup fbe_event_log_get_all_events_log_info_s
 *
 **********************************************************************/
typedef struct fbe_event_log_get_all_events_log_info_s{
   fbe_u32_t total_events;         /*!<IN */
    fbe_u32_t total_events_copied;            /*!<OUT*/
}fbe_event_log_get_all_events_log_info_t;
/*! @} */ /* end of group fbe_event_log_get_all_events_log_info_s */

/*use this macro and the closing one: FBE_API_CPP_EXPORT_END to export your functions to C++ applications*/
FBE_API_CPP_EXPORT_START
//------------------------------------------------------------
// Define the group for the event log service interface APIs.
//------------------------------------------------------------
/*! @defgroup fbe_api_event_log_service_interface FBE API Event log Service Interface
 *  @brief   
 *    This is the set of FBE API for event log services.
 *  
 *  @details
 *    In order to access this library, please include fbe_api_event_log_interface.h.  
 *  
 *  @ingroup fbe_system_package_class
 *  @{  
 */
//---------------------------------------------------------------- 
fbe_status_t FBE_API_CALL fbe_api_get_event_log_msg_count(fbe_event_log_msg_count_t *event_log_msg, 
                                                          fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_clear_event_log(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_check_event_log_msg(fbe_package_id_t package_id,
                                                      fbe_bool_t  *is_msg_present,
                                                      fbe_u32_t msg_id, ...);
fbe_status_t FBE_API_CALL fbe_api_wait_for_event_log_msg(fbe_u32_t timeoutMs,
                                                      fbe_package_id_t package_id,
                                                      fbe_bool_t *is_msg_present,
                                                      fbe_u32_t msg_id, ...);
fbe_status_t FBE_API_CALL fbe_api_get_event_log(fbe_event_log_get_event_log_info_t *event_log_info, 
                                                fbe_package_id_t package_id);
/* fbe_status_t FBE_API_CALL fbe_api_get_all_events_logs(fbe_event_log_info_t* events_array, 
                                                    fbe_u32_t *actual_events,  fbe_u32_t total_events,
                                                    fbe_package_id_t package_id);*/
fbe_status_t FBE_API_CALL fbe_api_get_all_events_logs(fbe_event_log_get_event_log_info_t *event_log_info, fbe_u32_t *actual_events,  fbe_u32_t total_events,
                                                          fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_get_event_log_statistics(fbe_event_log_statistics_t *event_log_statistics, 
                                                           fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_clear_event_log_statistics(fbe_package_id_t package_id);

fbe_status_t FBE_API_CALL fbe_api_event_log_disable(fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_event_log_enable(fbe_package_id_t package_id);

/*! @} */ /* end of group fbe_api_event_log_service_interface */
FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for the FBE API System Interface class files. 
// This is where all the class files that belong to the FBE API Physical 
// Package define. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_package_interface_class_files FBE System APIs Interface Class Files 
 *  @brief 
 *     This is the set of files for the FBE System Package Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif  /*FBE_API_EVENT_LOG_INTERFACE_H*/
