#ifndef FBE_API_ESP_EIR_INTERFACE_H
#define FBE_API_ESP_EIR_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_eir_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the ESP Enclosure Information Reporting service.
 * 
 * @ingroup fbe_api_esp_interface_class_files
 * 
 * @version
 *   04/27/2010:  Created. Dan McFarland
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_board.h"

#include "fbe/fbe_esp_ps_mgmt.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"

FBE_API_CPP_EXPORT_START

typedef enum fbe_eir_control_code_e 
{
	FBE_EVENT_EIR_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_EIR),
	FBE_EVENT_EIR_CONTROL_CODE_GET_DATA,
	FBE_EVENT_EIR_CONTROL_CODE_GET_STATUS,
        FBE_EVENT_EIR_CONTROL_CODE_GET_SAMPLE,
	FBE_EVENT_EIR_CONTROL_CODE_LAST
} fbe_eir_control_code_t;

typedef struct fbe_eir_data_s 
{
    fbe_eir_input_power_sample_t    arrayInputPower;
    fbe_eir_input_power_sample_t    arrayMaxInputPower;
    fbe_eir_input_power_sample_t    arrayAvgInputPower;
    fbe_esp_encl_eir_data_t         encl_data[FBE_EIR_MAX_ENCL_COUNT];
    fbe_esp_indiv_sps_eir_data_t    sps_data[FBE_SPS_MAX_COUNT];

    fbe_u32_t           eir_loop_count;
    fbe_u32_t           some_data;
    // Array eir data goes here.
} fbe_eir_data_t;

fbe_status_t FBE_API_CALL fbe_api_esp_eir_getData(fbe_eir_data_t *fbe_eir_data);
fbe_status_t FBE_API_CALL fbe_api_esp_eir_getSample(fbe_eir_input_power_sample_t *sample_data_ptr);
//fbe_status_t FBE_API_CALL fbe_api_esp_eir_getStatus(fbe_eir_data_t *fbe_eir_data);

FBE_API_CPP_EXPORT_END

#endif /* FBE_API_ESP_EIR_INTERFACE_H */


