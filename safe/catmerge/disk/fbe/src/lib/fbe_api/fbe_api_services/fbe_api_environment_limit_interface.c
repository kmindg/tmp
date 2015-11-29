/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_environment_limit_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions which use to access the
 *  environment limit service.
 *  
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_environment_limit_service_interface
 *
 * @version
 *   25-August - 2010 - Vishnu Sharma Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_environment_limit_interface.h"

/*!***************************************************************
 * @fn fbe_api_get_environment_limits(fbe_environment_limits_t *env_limits,
 *                                     fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This fbe api used to get environment_limit.
 *
 * @param env_limits    pointer to fbe_environment_limits_t structure
 * @param package_id    Package ID
 * 
 * @return fbe_status_t
 *
 * @author
 *  25-August - 2010: Vishnu Sharma Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_get_environment_limits(fbe_environment_limits_t *env_limits,
                                                          fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    if(package_id <= FBE_PACKAGE_ID_INVALID || 
       package_id >= FBE_PACKAGE_ID_LAST) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet_to_service(FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_GET_LIMITS,
                                                           env_limits,
                                                           sizeof(fbe_environment_limits_t),
                                                           FBE_SERVICE_ID_ENVIRONMENT_LIMIT,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    return status;
}
/********************************************
    end fbe_api_get_environment_limits()     
*********************************************/

/*!*****************************************************************************
 * @fn fbe_api_set_environment_limits(fbe_environment_limits_t *env_limits
 *                                     fbe_package_id_t package_id,
 *                                  ...)
 *******************************************************************************
 * @brief
 *  This fbe api used to set environment limits.
 *
 * @param env_limits    pointer to fbe_environment_limits_t structure
 *
 * @param package_id     package ID
 * 
 * @return fbe_status_t 
 *
 * @author
 *  25-August - 2010: Vishnu Sharma Created.
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_set_environment_limits(fbe_environment_limits_t *env_limits,
                                                         fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    if(package_id <= FBE_PACKAGE_ID_INVALID || 
       package_id >= FBE_PACKAGE_ID_LAST) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    status = fbe_api_common_send_control_packet_to_service(FBE_ENVIRONMENT_LIMIT_CONTROL_CODE_SET_LIMITS,
                                                           env_limits,
                                                           sizeof(fbe_environment_limits_t),
                                                           FBE_SERVICE_ID_ENVIRONMENT_LIMIT,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id); 
    return status;
}
/********************************************
    end fbe_api_set_environment_limits()     
*********************************************/
