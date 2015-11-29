#ifndef FBE_BASE_ENVIRONMENT_DEBUG_H
#define FBE_BASE_ENVIRONMENT_DEBUG_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_environment_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the base environment debug library.
 *
 * @author
 *   18-October-2010:  Created. Vishnu Sharma
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_class.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_api_common_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * fbe_base_env_decode_device_type(fbe_u64_t deviceType);
char * fbe_base_env_decode_fup_completion_status(fbe_base_env_fup_completion_status_t completionStatus);
char * fbe_base_env_decode_fup_work_state(fbe_base_env_fup_work_state_t workState);
char * fbe_base_env_decode_firmware_op_code(fbe_enclosure_firmware_opcode_t firmwareOpCode);
char * fbe_base_env_decode_env_status(fbe_env_inferface_status_t  envStatus);
char * fbe_base_env_decode_status(fbe_mgmt_status_t  status);
fbe_status_t fbe_base_env_map_devicetype_to_classid(fbe_u64_t deviceType,fbe_class_id_t *classId);
fbe_status_t fbe_base_env_create_device_string(fbe_u64_t deviceType, 
                                         fbe_device_physical_location_t * pLocation,
                                         char * deviceStr,
                                         fbe_u32_t bufferLen);
fbe_status_t fbe_base_env_create_simple_device_string(fbe_u64_t deviceType, 
                                         fbe_device_physical_location_t * pLocation,
                                         char * simpleDeviceStr,
                                         fbe_u32_t bufferLen);
char * fbe_base_env_decode_resume_prom_status(fbe_resume_prom_status_t resumeStatus);
char * fbe_base_env_decode_resume_prom_work_state(fbe_base_env_resume_prom_work_state_t workState);
char * fbe_base_env_decode_connector_cable_status(fbe_cable_status_t cableStatus);
char * fbe_base_env_decode_connector_type(fbe_connector_type_t connectorType);
char * fbe_base_env_decode_led_status(fbe_led_status_t ledStatus);
char * fbe_base_env_decode_encl_fault_led_reason(fbe_encl_fault_led_reason_t enclFaultLedReason);
char * fbe_base_env_decode_class_id(fbe_class_id_t classId);
char * fbe_base_env_decode_cache_status(fbe_common_cache_status_t status);
fbe_u32_t fbe_base_env_resume_prom_getFieldOffset(RESUME_PROM_FIELD resume_field);
char * fbe_base_env_decode_firmware_target(fbe_enclosure_fw_target_t firmwareTarget);
#endif /* FBE_BASE_ENVIRONMENT_DEBUG_H */
