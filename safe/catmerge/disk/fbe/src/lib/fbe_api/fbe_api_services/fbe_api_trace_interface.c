/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_lun_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_lun interface for the storage extent package.
 *
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_trace_interface
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_utils.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!***************************************************************
   @fn fbe_api_trace_reset_error_counters(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function resets the error counters in the trace service
 *
 * @param package_id      - in which package to reset the counters
 *
 * @return
 *  fbe_status_t
 *
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_trace_reset_error_counters(fbe_package_id_t package_id)
 {
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_RESET_ERROR_COUNTERS,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

		return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

 }

 /*!***************************************************************
 * fbe_api_trace_err_set_notify_level()
 *****************************************************************
 * @brief
 *   Trace set notify level 
 *
 * @param trace_level - trace level 
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    8/22/11: vera wang   created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_err_set_notify_level(fbe_trace_level_t trace_level, fbe_package_id_t package_id)
{
    fbe_trace_err_set_notify_level_t notify_level;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;
    
    notify_level.level = trace_level;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_TRACE_ERR_SET_NOTIFY_LEVEL,
                                                           &notify_level,
                                                           sizeof(fbe_trace_err_set_notify_level_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_trace_set_notify_level()
 **************************************/

/*!***************************************************************
 * fbe_api_trace_enable_backtrace()
 ****************************************************************
 * @brief
 *  This function resets the error counters in the trace service
 *
 * @param package_id      - in which package to reset the counters
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_trace_enable_backtrace(fbe_package_id_t package_id)
 {
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_ENABLE_BACKTRACE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

		return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

 }
 /**************************************
  * end fbe_api_trace_enable_backtrace
  **************************************/

/*!***************************************************************
 * fbe_api_trace_disable_backtrace()
 ****************************************************************
 * @brief
 *  This function resets the error counters in the trace service
 *
 * @param package_id      - in which package to reset the counters
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
 fbe_status_t FBE_API_CALL fbe_api_trace_disable_backtrace(fbe_package_id_t package_id)
 {
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;
    
    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_DISABLE_BACKTRACE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                      status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

		return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

 }
 /**************************************
  * end fbe_api_trace_disable_backtrace
  **************************************/

 /*!*******************************************************************
 * @var fbe_api_command_to_ktrace_buff
 *********************************************************************
 * @brief Function to send fbe_cli commands into ktrace start buffer.
 *
 *  @param    cmd - Command
 *  @param    length - size of the command.
 *
 * @return - none.  
 *
 * @author
 *  2/28/2013 - Created. Preethi Poddatur
 *********************************************************************/

fbe_status_t FBE_API_CALL fbe_api_command_to_ktrace_buff(fbe_u8_t *cmd, fbe_u32_t length) 
{
   fbe_status_t status=FBE_STATUS_OK;
   fbe_trace_command_to_ktrace_buff_t cmd_ktracebuf;
   fbe_api_control_operation_status_info_t status_info;

   cmd_ktracebuf.length = length;

   fbe_zero_memory(cmd_ktracebuf.cmd, MAX_KTRACE_BUFF);
   if (length < MAX_KTRACE_BUFF) {
      fbe_copy_memory(cmd_ktracebuf.cmd, cmd, length);
   }
   else {
      //Last element in the array is definitely '\0' since we only copy upto MAX_KTRACE_BUFF-1
      fbe_copy_memory(cmd_ktracebuf.cmd, cmd, MAX_KTRACE_BUFF-1);
      fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Partial copy of command name since command length is greater than MAX_KTRACE_BUFF. \n", __FUNCTION__); 
   }

   fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_COMMAND_TO_KTRACE_BUFF,
                                                           &cmd_ktracebuf,
                                                           sizeof(fbe_trace_command_to_ktrace_buff_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0
                                                 );

   if (status != FBE_STATUS_OK) {
       fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: fail to send control packet to database service status:%d",
                           __FUNCTION__, status);
       return status;
   }

   if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
   {
       fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
       return FBE_STATUS_GENERIC_FAILURE;
   }


   return status;
}


/******************************************
 * end fbe_api_command_to_ktrace_buff()
 ******************************************/

