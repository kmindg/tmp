#ifndef FBE_API_PHYSICAL_DRIVE_INTERFACE_H
#define FBE_API_PHYSICAL_DRIVE_INTERFACE_H

/*!**************************************************************************
 * @file fbe_api_physical_drive_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the Physical Drive APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * 
 * @version
 *   04/08/00 sharel    Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_api_perfstats_interface.h"


//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief 
 *    This is the set of definitions for FBE Physical Package APIs.
 *
 *  @ingroup fbe_api_physical_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Physical Drive Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_physical_drive_usurper_interface FBE API Physical Drive Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Physical Drive class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*!********************************************************************* 
 * @typedef fbe_physical_drive_mgmt_get_error_counts_t fbe_api_drive_error_count_t
 *
 * @ingroup fbe_api_physical_drive_usurper_interface
 **********************************************************************/
typedef fbe_physical_drive_mgmt_get_error_counts_t fbe_api_drive_error_count_t;

typedef fbe_physical_drive_dieh_stats_t fbe_api_dieh_stats_t;

/*!********************************************************************* 
 * @def DISABLE_WRITE_CACHE
 *  
 * @brief 
 *  This set default for DISABLE write cache.
 *
 * @ingroup fbe_api_physical_drive_usurper_interface
 **********************************************************************/
#define DISABLE_WRITE_CACHE 0

/*!********************************************************************* 
 * @def ENABLE_WRITE_CACHE
 *  
 * @brief 
 *  This set default for ENABLE write cache.
 *
 * @ingroup fbe_api_physical_drive_usurper_interface
 **********************************************************************/
#define ENABLE_WRITE_CACHE 1
#define DC_TRANSFER_SIZE 520


/*! @} */ /* end of group fbe_api_physical_drive_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Physical Drive Interface.  
// This is where all the function prototypes for the Physical Drive API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_physical_drive_interface FBE API Physical Drive Interface
 *  @brief 
 *    This is the set of FBE API Physical Drive.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_physical_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_physical_drive_reset(fbe_object_id_t object_id, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_mode_select(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_dieh_info(fbe_object_id_t  object_id, fbe_physical_drive_dieh_stats_t * dieh_stats, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_error_counts(fbe_object_id_t  object_id, fbe_physical_drive_mgmt_get_error_counts_t * error_counts, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_clear_error_counts(fbe_object_id_t  object_id, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_object_queue_depth(fbe_object_id_t  object_id, fbe_u32_t depth, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_queue_depth(fbe_object_id_t  object_id, fbe_u32_t *depth, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_object_queue_depth(fbe_object_id_t  object_id, fbe_u32_t *depth, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_is_wr_cache_enabled(fbe_object_id_t  object_id, fbe_bool_t *enabled, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_information(fbe_object_id_t  object_id, fbe_physical_drive_information_t *get_drive_information, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_default_io_timeout(fbe_object_id_t  object_id, fbe_time_t *timeout, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_default_io_timeout(fbe_object_id_t  object_id, fbe_time_t timeout, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_get_physical_drive_object_id_by_location(fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_power_cycle(fbe_object_id_t object_id, fbe_u32_t duration);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_write_uncorrectable(fbe_object_id_t  object_id, fbe_lba_t lba, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_receive_diag_info(fbe_object_id_t object_id, fbe_physical_drive_mgmt_receive_diag_page_t * diag_info);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_cached_drive_info(fbe_object_id_t  object_id, fbe_physical_drive_information_t *get_drive_information, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_information_asynch(fbe_object_id_t  object_id, fbe_physical_drive_information_asynch_t * get_drive_information);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_fail_drive(fbe_object_id_t  object_id, fbe_base_physical_drive_death_reason_t reason); 
fbe_status_t FBE_API_CALL fbe_api_physical_drive_enter_glitch_drive(fbe_object_id_t  object_id, fbe_time_t glitch_time); 
fbe_status_t FBE_API_CALL fbe_api_physical_drive_fw_download_start_asynch(fbe_physical_drive_fwdl_start_asynch_t *start_info);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_fw_download_asynch(fbe_object_id_t object_id, const fbe_sg_element_t *sg_list, fbe_u32_t sg_blocks, fbe_physical_drive_fw_info_asynch_t *fw_info);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_fw_download_stop_asynch(fbe_physical_drive_fwdl_stop_asynch_t *stop_info);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_reset_slot(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_drive_death_reason(fbe_object_id_t  object_id, FBE_DRIVE_DEAD_REASON death_reason, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_disk_error_log(fbe_object_id_t object_id, fbe_lba_t lba, fbe_u8_t * data_buffer);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_fuel_gauge_asynch(fbe_object_id_t object_id, fbe_physical_drive_fuel_gauge_asynch_t * fuel_gauge_op);                                           
fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_pass_thru(fbe_object_id_t object_id, fbe_physical_drive_send_pass_thru_t * pass_thru_op);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_pass_thru_asynch(fbe_object_id_t object_id, fbe_physical_drive_send_pass_thru_asynch_t * pass_thru_op); 
fbe_status_t FBE_API_CALL fbe_api_physical_drive_dc_rcv_diag_enabled(fbe_object_id_t object_id, fbe_u8_t flag);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_health_check(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_enable_disable_perf_stats(fbe_object_id_t object_id, fbe_bool_t b_set_enable);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_perf_stats(fbe_object_id_t object_id,
                                                                fbe_pdo_performance_counters_t *pdo_counters);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_drivegetlog_asynch(fbe_object_id_t object_id, fbe_physical_drive_get_smart_dump_asynch_t * smart_dump_op);                                          
fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_download_asynch(fbe_object_id_t object_id, fbe_physical_drive_fw_download_asynch_t * fw_info);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_abort_download(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_service_time(fbe_object_id_t  object_id, fbe_time_t service_time_ms);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_service_time(fbe_object_id_t  object_id,
                                                                  fbe_physical_drive_control_get_service_time_t *get_service_time_p);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_outstanding_io_count(fbe_object_id_t  object_id, fbe_u32_t * outstanding_io_count);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_clear_eol(fbe_object_id_t  object_id, fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_crc_action(fbe_object_id_t  object_id, fbe_pdo_logical_error_action_t action);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_send_logical_error(fbe_object_id_t object_id, fbe_block_transport_error_type_t error);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_update_link_fault(fbe_object_id_t object_id, fbe_bool_t b_set_clear_link_fault);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_update_drive_fault(fbe_object_id_t object_id, fbe_bool_t b_set_clear_drive_fault);

fbe_status_t FBE_API_CALL fbe_api_physical_drive_health_check_test(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_logical_drive_state(fbe_object_id_t object_id, fbe_block_transport_logical_state_t *logical_state);

fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_object_io_throttle_MB(fbe_object_id_t  object_id, fbe_u32_t io_throttle_MB);
fbe_status_t fbe_api_create_read_uncorrectable(fbe_object_id_t  object_id, fbe_lba_t lba, fbe_packet_attr_t attribute);

fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_attributes(fbe_object_id_t object_id, 
                                                               fbe_physical_drive_attributes_t * const attributes_p);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_phy_info(fbe_object_id_t object_id, 
                                                fbe_drive_phy_info_t * drive_phy_info_p);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_drive_block_size(const fbe_object_id_t object_id,
                                                         fbe_block_transport_negotiate_t *const negotiate_p,
                                                         fbe_payload_block_operation_status_t *const block_status_p,
                                                         fbe_payload_block_operation_qualifier_t *const block_qualifier);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_id_by_serial_number(const fbe_u8_t *sn, 
                                                                         fbe_u32_t sn_array_size, 
                                                                         fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_pvd_eol(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_interface_read_ica_stamp(fbe_object_id_t object_id, fbe_imaging_flags_t *ica_stamp);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_interface_generate_ica_stamp(fbe_object_id_t object_id);

fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_enhanced_queuing_timer(fbe_object_id_t object_id, fbe_u32_t lpq_timer, fbe_u32_t hpq_timer);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_rla_abort_required(fbe_object_id_t object_id, fbe_bool_t *b_is_required_p);

fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_bms_op_asynch(fbe_object_id_t object_id, fbe_physical_drive_bms_op_asynch_t * bms_op_p);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_sanitize(fbe_object_id_t object_id, fbe_scsi_sanitize_pattern_t pattern);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_get_sanitize_status(fbe_object_id_t object_id, fbe_physical_drive_sanitize_info_t *sanitize_status);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_format_block_size(fbe_object_id_t object_id, fbe_u32_t block_size);
fbe_status_t FBE_API_CALL fbe_api_physical_drive_set_dieh_media_threshold(fbe_object_id_t object_id, fbe_physical_drive_set_dieh_media_threshold_t *threshold_p);

/*! @} */ /* end of group fbe_api_port_interface */


FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE Physical Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Physical 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_physical_package_interface_class_files FBE Physical Package APIs Interface Class Files 
 *  @brief This is the set of files for the FBE Physical Package Interface.
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_PHYSICAL_DRIVE_INTERFACE_H*/

