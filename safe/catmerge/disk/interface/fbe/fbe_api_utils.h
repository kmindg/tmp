#ifndef FBE_API_UTIL_H
#define FBE_API_UTIL_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe_pe_types.h"
#include "fbe/fbe_enclosure.h"

FBE_API_CPP_EXPORT_START

fbe_status_t fbe_api_wait_for_discovery_edge_attr(fbe_object_id_t edge_handle_p, fbe_u32_t expected_attr, fbe_u32_t timeout_ms);
fbe_status_t fbe_api_wait_for_protocol_edge_attr(fbe_object_id_t edge_handle_p, fbe_u32_t expected_attr, fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_object_lifecycle_state (fbe_object_id_t object_id,
                                                      fbe_lifecycle_state_t lifecycle_state,
                                                      fbe_u32_t timeout_ms,
                                                      fbe_package_id_t package_id);

fbe_status_t fbe_api_wait_for_object_lifecycle_state_warn_trace (fbe_object_id_t object_id,
                                                      fbe_lifecycle_state_t lifecycle_state,
                                                      fbe_u32_t timeout_ms,
                                                      fbe_package_id_t package_id);

fbe_status_t fbe_wait_for_object_to_transiton_from_lifecycle_state(fbe_object_id_t object_id,
                                                                   fbe_lifecycle_state_t lifecycle_state,
                                                                   fbe_u32_t timeout_ms,
                                                                   fbe_package_id_t package_id);

fbe_status_t fbe_api_wait_for_number_of_objects (fbe_u32_t expected_number, fbe_u32_t timeout_ms, fbe_package_id_t package_id);

fbe_status_t fbe_api_wait_for_number_of_class(fbe_class_id_t class_id, 
                                              fbe_u32_t expected_number_of_objects,
                                              fbe_u32_t timeout_ms, 
                                              fbe_package_id_t package_id);

fbe_status_t fbe_api_wait_for_fault_attr(fbe_object_id_t object_handle, 
                                         fbe_base_enclosure_attributes_t expected_attr,
                                         fbe_bool_t expected_value,
                                         fbe_enclosure_component_types_t component_type,
                                         fbe_u8_t component_index,
                                         fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_encl_attr(fbe_object_id_t object_handle, fbe_base_enclosure_attributes_t expected_attr,
                                         fbe_u32_t expected_value, fbe_u32_t component, fbe_u32_t index, fbe_u32_t timeout_ms);

fbe_status_t fbe_api_get_encl_attr(fbe_object_id_t object_handle, fbe_base_enclosure_attributes_t expected_attr,
                                         fbe_u32_t component, fbe_u32_t index, fbe_u32_t *expected_value);


fbe_status_t fbe_api_wait_for_block_edge_path_state(fbe_object_id_t object_id, 
                                                    fbe_u32_t edge_index,
                                                    fbe_u32_t expected_path_state, 
                                                    fbe_u32_t package_id,
                                                    fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_block_edge_path_attribute(fbe_object_id_t object_id, 
                                                        fbe_u32_t edge_index,
                                                        fbe_u32_t expected_path_attribute, 
                                                        fbe_u32_t package_id,
                                                        fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_block_edge_path_attribute_cleared(fbe_object_id_t object_id, 
                                                                fbe_u32_t edge_index,
                                                                fbe_u32_t cleared_path_attribute, 
                                                                fbe_u32_t package_id,
                                                                fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_virtual_drive_configuration_mode(fbe_object_id_t vd_object_id, 
                                                               fbe_virtual_drive_configuration_mode_t expected_configuration_mode, 
                                                               fbe_virtual_drive_configuration_mode_t * current_configuration_mode_p,
                                                               fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_lun_peer_state_ready(fbe_object_id_t lun_object_id, 
                                                   fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_rg_peer_state_ready(fbe_object_id_t rg_object_id, 
                                                   fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_for_encl_firmware_download_status(fbe_object_id_t object_id, 
                                  fbe_enclosure_fw_target_t target, 
                                  fbe_enclosure_firmware_status_t expected_download_status,
                                  fbe_enclosure_firmware_ext_status_t expected_extended_download_status,
                                  fbe_u32_t timeout_ms);

fbe_status_t fbe_api_wait_till_object_is_destroyed(fbe_object_id_t obj_id,fbe_package_id_t package_id);

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_UTIL_H*/

