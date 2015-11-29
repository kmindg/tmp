/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_api_extent_pool_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_extent_pool interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_extent_pool_interface
 *
 * @version
 *  6/13/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_extent_pool.h"
#include "fbe/fbe_api_extent_pool_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_cmi_interface.h"


/*************************
 *   FORWARD DECLARATIONS
 *************************/

/*******************************************
                Local functions
********************************************/

/*!***************************************************************
 *  fbe_api_extent_pool_get_info()
 ****************************************************************
 * @brief
 *  This function get info with the given extent pool.
 *
 * @param object_id - object id
 * @param extent_pool_info_p - pointer to info
 * @param attribute - attribute
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  6/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_extent_pool_get_info(fbe_object_id_t object_id, 
                             fbe_extent_pool_get_info_t * extent_pool_info_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    if (extent_pool_info_p == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: NULL input buffer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(extent_pool_info_p, sizeof(fbe_extent_pool_get_info_t));

    status = fbe_api_common_send_control_packet(FBE_EXTENT_POOL_CONTROL_CODE_GET_INFO,
                                                 extent_pool_info_p,
                                                 sizeof(fbe_extent_pool_get_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_SEP_0);
    if ((status != FBE_STATUS_OK) || 
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        if (status == FBE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }
    return status;
}
/**************************************
 * end fbe_api_extent_pool_get_info()
 **************************************/

fbe_status_t FBE_API_CALL
fbe_api_extent_pool_class_init_options(fbe_extent_pool_class_set_options_t * set_options_p)
{
    fbe_zero_memory(set_options_p, sizeof(fbe_extent_pool_class_set_options_t));
    return FBE_STATUS_OK;
}
/*!***************************************************************
 *  fbe_api_extent_pool_class_set_options()
 ****************************************************************
 * @brief
 *  This function determines information about a raid gorup
 *  given the input values.
 *
 * @param get_info_p - pointer to get RG class info
 * @param class_id - class ID
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  6/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_extent_pool_class_set_options(fbe_extent_pool_class_set_options_t * set_options_p,
                                      fbe_class_id_t class_id)
{
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_class(FBE_EXTENT_POOL_CONTROL_CODE_CLASS_SET_OPTIONS,
                                                         set_options_p,
                                                         sizeof(fbe_extent_pool_class_set_options_t),
                                                         class_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*********************************************
 * end fbe_api_extent_pool_class_set_options()
 ********************************************/

/******************************************
 * end file fbe_api_extent_pool_interface.c
 ******************************************/
