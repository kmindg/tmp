#ifndef NEIT_UTILS_H
#define NEIT_UTILS_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_protocol_error_injection_service.h"

fbe_status_t fbe_test_neit_utils_init_error_injection_record(fbe_protocol_error_injection_error_record_t * error_injection_record);

fbe_status_t fbe_test_neit_utils_convert_to_physical_drive_range(fbe_object_id_t pdo_object_id,
                                                                 fbe_lba_t start_lba,
                                                                 fbe_lba_t end_lba,
                                                                 fbe_lba_t *physical_start_lba_p,
                                                                 fbe_lba_t *physical_end_lba_p);

fbe_status_t fbe_test_neit_utils_populate_protocol_error_injection_record(fbe_u32_t disk_location_bus,
                                                                          fbe_u32_t disk_location_enclosure,
                                                                          fbe_u32_t disk_location_slot,
                                                              fbe_protocol_error_injection_error_record_t *error_record_p,
                                                              fbe_protocol_error_injection_record_handle_t *record_handle_p,
                                                              fbe_lba_t user_space_start_lba,
                                                              fbe_block_count_t blocks,
                                                              fbe_protocol_error_injection_error_type_t protocol_error_injection_error_type,
                                                              fbe_u8_t scsi_command,
                                                              fbe_scsi_sense_key_t scsi_sense_key,
                                                              fbe_scsi_additional_sense_code_t scsi_additional_sense_code,
                                                              fbe_scsi_additional_sense_code_qualifier_t scsi_additional_sense_code_qualifier,
                                                              fbe_port_request_status_t port_status, 
                                                              fbe_u32_t num_of_times_to_insert);


#endif /* NEIT_UTILS_H */
