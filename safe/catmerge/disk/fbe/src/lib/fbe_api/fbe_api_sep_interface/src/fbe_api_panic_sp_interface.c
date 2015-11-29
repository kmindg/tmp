/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_panic_sp_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the fbe_panic_sp interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_panic_sp_interface
 *
 * @version
 *  
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
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_transport.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/


/*!***************************************************************
 * @fn fbe_api_panic_sp()

 ****************************************************************
 * @brief
 *  This is api function to panic a SP
 *
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_panic_sp()
{
    fbe_api_control_operation_status_info_t control_status;
    fbe_status_t          status = FBE_STATUS_OK;

    status = fbe_api_common_send_control_packet_to_service (FBE_BASE_SERVICE_CONTROL_CODE_USER_INDUCED_PANIC,
                                                            NULL, 
                                                            0,
                                                            FBE_SERVICE_ID_SERVICE_MANAGER,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &control_status,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || control_status.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, control_status.packet_qualifier, control_status.control_operation_status, control_status.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}



/*!***************************************************************
 * @fn fbe_api_panic_sp_async()

 ****************************************************************
 * @brief
 *  This is api function to panic a SP
 *
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_panic_sp_async(void)
{
    fbe_status_t          status = FBE_STATUS_OK;
    fbe_packet_t *packet = NULL;

    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_to_service_asynch(packet,
                                                                  FBE_BASE_SERVICE_CONTROL_CODE_USER_INDUCED_PANIC,
                                                                  NULL, 
                                                                  0,
                                                                  FBE_SERVICE_ID_SERVICE_MANAGER,
                                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                                  NULL, NULL,
                                                                  FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:error:%d\n", __FUNCTION__, status );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}
/*************************
 * end file fbe_api_panic_sp_interface.c
 *************************/
