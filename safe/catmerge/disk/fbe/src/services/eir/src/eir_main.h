#ifndef EIR_MAIN_H
#define EIR_MAIN_H
/***************************************************************************
 * Copyright (C) EMC Corporation  2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation Unpublished 
 * - all rights reserved under the copyright laws 
 * of the United States
 ***********************************************************************************/

/************************************************************************************
 *                          eir_main.h
 ************************************************************************************
 *
 * DESCRIPTION:
 *   Include file for the eir service main file.
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 * 
 *  HISTORY:
 *      26-Apr-2010    Dan McFarland  Created.
 *
 ***********************************************************************************/
#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_esp_eir_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_service.h"
#include "fbe/fbe_sps_info.h"


typedef struct fbe_eir_service_data_s 
{
    fbe_eir_input_power_sample_t        totalEnclInputPower;
    fbe_eir_input_power_sample_t        totalSpsInputPower;

    fbe_eir_input_power_sample_t        arrayCurrentInputPower;
    fbe_eir_input_power_sample_t        arrayAverageInputPower;
    fbe_eir_input_power_sample_t        arrayMaxInputPower;

    fbe_u32_t                           sampleCount;
    fbe_u32_t                           sampleIndex;
    fbe_eir_input_power_sample_t        inputPowerSamples[FBE_SAMPLE_DATA_SIZE];

} fbe_eir_service_data_t;


static void eir_thread_func(void * context);
void fbe_eir_trace(fbe_trace_level_t trace_level, fbe_trace_message_id_t message_id, const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));
static fbe_status_t fbe_eir_init(fbe_packet_t * packet);
static fbe_status_t eir_destroy(fbe_packet_t * packet);
fbe_status_t fbe_eir_entry(fbe_packet_t * packet);
static fbe_status_t fbe_eir_clear_data(fbe_eir_service_data_t *eir_data);
fbe_status_t fbe_esp_eir_getData(fbe_packet_t * packet);
fbe_status_t fbe_esp_eir_getSample(fbe_packet_t * packet);

#endif /* EIR_MAIN_H */

