/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_eir_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the eir API for the Environmental
 *  Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_eir_interface
 *
 * @version
 *   5/13/2010 Created Dan McFarland
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
#include "fbe/fbe_api_esp_eir_interface.h"

/************************************************************************************
 *                          fbe_api_esp_eir_getData
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This is the handler function for returning data from eir.
 *
 *  PARAMETERS:
 *      packet - Pointer to the packet received by the event log
 *      service
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      21-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/

fbe_status_t FBE_API_CALL fbe_api_esp_eir_getData(fbe_eir_data_t *fbe_eir_data)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_EIR_CONTROL_CODE_GET_DATA,
                                                  fbe_eir_data,
                                                  sizeof(fbe_eir_data_t),
                                                  FBE_SERVICE_ID_EIR,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_ESP);

    if ((status != FBE_STATUS_OK) ||
        (status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                   "%s get status failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/************************************************************************************
 *                          fbe_api_esp_eir_getSample
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This is the handler function for returning sample data from eir.
 *
 *  PARAMETERS:
 *      packet - Pointer to the packet received by the event log
 *      service
 *
 *  RETURN VALUES/ERRORS:
 *       fbe_status_t
 *
 *  NOTES:
 *
 * HISTORY:
 *      9-May-2012    Dongz  Created.
 *
 ***********************************************************************************/

fbe_status_t FBE_API_CALL fbe_api_esp_eir_getSample(fbe_eir_input_power_sample_t *sample_data_ptr)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_EVENT_EIR_CONTROL_CODE_GET_SAMPLE,
                                                  sample_data_ptr,
                                                  sizeof(fbe_eir_input_power_sample_t)* FBE_SAMPLE_DATA_SIZE,
                                                  FBE_SERVICE_ID_EIR,
                                                  FBE_PACKET_FLAG_NO_ATTRIB,
                                                  &status_info,
                                                  FBE_PACKAGE_ID_ESP);

    if ((status != FBE_STATUS_OK) ||
        (status_info.packet_status != FBE_STATUS_OK) ||
        (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                   "%s get status failed packet_status: 0x%x qual: 0x%x "
                   "control payload status: 0x%x control qualifier: 0x%x\n",
                   __FUNCTION__, status_info.packet_status, status_info.packet_qualifier,
                   status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
