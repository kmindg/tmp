/***************************************************************************
 *  terminator_service_sim.c
 ***************************************************************************
 *
 *  Description
 *      user space implementation for ioctl api
 *      
 *
 *  History:
 *      06/16/08    sharel  Created
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_base_service.h"
#include "fbe/fbe_transport.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_terminator_service.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_terminator.h"
#include "fbe_terminator_persistent_memory.h"

static fbe_status_t fbe_terminator_service_terminator_init (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_terminator_destroy (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_force_login_device (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_force_logout_device (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_find_device_class (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_set_simulated_drive_type (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_simulated_drive_server_name (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_simulated_drive_server_port (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_simulated_drive_debug_flags (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_device_attribute (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_sas_enclosure_drive_slot_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_temp_sensor_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_cooling_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_ps_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_sas_conn_eses_status(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_lcc_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_buf (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_buf_by_buf_id (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_resume_prom_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_io_mode (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_set_io_completion_irql(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_io_global_completion_delay (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_drive_product_id (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_enclosure_getEmcEnclStatus (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_enclosure_setEmcEnclStatus (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_enclosure_getEmcPsInfoStatus (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_enclosure_setEmcPsInfoStatus (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_get_device_attribute (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_port_handle (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_enclosure_handle (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_drive_handle (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_temp_sensor_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_cooling_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_ps_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_lcc_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_sas_conn_eses_status (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_sas_drive_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_sata_drive_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_drive_error_count (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_device_cpd_device_id (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_drive_product_id (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_create_device_class_instance (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_insert_board (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_device (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_sas_port (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_fc_port (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_iscsi_port (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_fcoe_port (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_sas_enclosure (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_sas_drive (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_insert_sata_drive (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_reinsert_drive (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_reinsert_enclosure (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_force_create_sas_drive (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_force_create_sata_drive (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_remove_device (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_remove_sas_enclosure (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_remove_sas_drive (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_remove_sata_drive (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_pull_drive (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_pull_enclosure (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_activate_device (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_start_port_reset (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_complete_port_reset (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_get_hardware_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_hardware_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_sfp_media_interface_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_sfp_media_interface_info (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_enclosure_bypass_drive_slot (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_enclosure_unbypass_drive_slot (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_eses_increment_config_page_gen_code (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_eses_set_download_microcode_stat_page_stat_desc (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_eses_get_ver_desc (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_eses_set_ver_desc (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_eses_set_unit_attention (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_mark_eses_page_unsupported (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_error_injection_init (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_error_injection_init_error (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_error_injection_add_error (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_error_injection_start (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_error_injection_stop (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_error_injection_destroy (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_reserve_miniport_sas_device_table_index (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_miniport_sas_device_table_force_add (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_miniport_sas_device_table_force_remove (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation);
static fbe_status_t fbe_terminator_service_set_sp_id(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_sp_id(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_send_specl_sfi_mask_data(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_set_state (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_get_default_page_info(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_set_default_page_info(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_drive_set_default_field(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_need_update_enclosure_resume_prom_checksum (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_need_update_enclosure_resume_prom_checksum (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_need_update_enclosure_firmware_rev (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_need_update_enclosure_firmware_rev (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_enclosure_firmware_activate_time_interval (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_enclosure_firmware_activate_time_interval (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_enclosure_firmware_reset_time_interval (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_enclosure_firmware_reset_time_interval (fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_get_port_link_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_port_link_info (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_encryption_key (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_port_address (fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_devices_count_by_type_name(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_connector_id_list_for_enclosure(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_drive_slot_parent(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_terminator_device_count(fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_set_persistence_request(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_persistence_request(fbe_packet_t *packet);

static fbe_status_t fbe_terminator_service_set_persistence_status(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_persistence_status(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_zero_persistent_memory(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_set_log_page_31(fbe_packet_t *packet);
static fbe_status_t fbe_terminator_service_get_log_page_31(fbe_packet_t *packet);

fbe_status_t fbe_terminator_service_send_packet(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_operation_opcode_t  control_code;

    fbe_terminator_service_get_control_operation(packet, &control_operation);
    fbe_payload_control_get_opcode(control_operation, &control_code);

    switch (control_code)
    {
    case FBE_TERMINATOR_CONTROL_CODE_INIT:
        status = fbe_terminator_service_terminator_init(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DESTROY:
        status = fbe_terminator_service_terminator_destroy(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_FORCE_LOGIN_DEVICE:
        status = fbe_terminator_service_force_login_device(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_FORCE_LOGOUT_DEVICE:
        status = fbe_terminator_service_force_logout_device(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_FIND_DEVICE_CLASS:
        status = fbe_terminator_service_find_device_class(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_CREATE_DEVICE_CLASS_INSTANCE:
        status = fbe_terminator_service_create_device_class_instance(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_DEVICE:
        status = fbe_terminator_service_insert_device(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_BOARD:
        status = fbe_terminator_service_insert_board(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_FC_PORT:
        status = fbe_terminator_service_insert_fc_port(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_ISCSI_PORT:
        status = fbe_terminator_service_insert_iscsi_port(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_FCOE_PORT:
        status = fbe_terminator_service_insert_fcoe_port(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_PORT:
        status = fbe_terminator_service_insert_sas_port(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_ENCLOSURE:
        status = fbe_terminator_service_insert_sas_enclosure(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_REINSERT_DRIVE:
        status = fbe_terminator_service_reinsert_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_REINSERT_ENCLOSURE:
        status = fbe_terminator_service_reinsert_enclosure(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_REMOVE_DEVICE:
        status = fbe_terminator_service_remove_device(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_REMOVE_SAS_ENCLOSURE:
        status = fbe_terminator_service_remove_sas_enclosure(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_REMOVE_SAS_DRIVE:
        status = fbe_terminator_service_remove_sas_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_REMOVE_SATA_DRIVE:
        status = fbe_terminator_service_remove_sata_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_PULL_DRIVE:
        status = fbe_terminator_service_pull_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_PULL_ENCLOSURE:
        status = fbe_terminator_service_pull_enclosure(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ACTIVATE_DEVICE:
        status = fbe_terminator_service_activate_device(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_START_PORT_RESET:
        status = fbe_terminator_service_start_port_reset(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_COMPLETE_PORT_RESET:
        status = fbe_terminator_service_complete_port_reset(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_HARDWARE_INFO:
        status = fbe_terminator_service_get_hardware_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_HARDWARE_INFO:
        status = fbe_terminator_service_set_hardware_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_SFP_MEDIA_INTERFACE_INFO:
        status = fbe_terminator_service_get_sfp_media_interface_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SFP_MEDIA_INTERFACE_INFO:
        status = fbe_terminator_service_set_sfp_media_interface_info(packet);
        break;

    case FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_BYPASS_DRIVE_SLOT:
        status = fbe_terminator_service_enclosure_bypass_drive_slot(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_UNBYPASS_DRIVE_SLOT:
        status = fbe_terminator_service_enclosure_unbypass_drive_slot(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_GET_EMC_ENCL_STATUS:
        status = fbe_terminator_service_enclosure_getEmcEnclStatus(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_SET_EMC_ENCL_STATUS:
        status = fbe_terminator_service_enclosure_setEmcEnclStatus(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_GET_EMC_PS_INFO_STATUS:
        status = fbe_terminator_service_enclosure_getEmcPsInfoStatus(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ENCLOSURE_SET_EMC_PS_INFO_STATUS:
        status = fbe_terminator_service_enclosure_setEmcPsInfoStatus(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ESES_INCREMENT_CONFIG_PAGE_GEN_CODE:
        status = fbe_terminator_service_eses_increment_config_page_gen_code(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ESES_SET_DOWNLOAD_MICROCODE_STAT_PAGE_STAT_DESC:
        status = fbe_terminator_service_eses_set_download_microcode_stat_page_stat_desc(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ESES_GET_VER_DESC:
        status = fbe_terminator_service_eses_get_ver_desc(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ESES_SET_VER_DESC:
        status = fbe_terminator_service_eses_set_ver_desc(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ESES_SET_UNIT_ATTENTION:
        status = fbe_terminator_service_eses_set_unit_attention(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_MARK_ESES_PAGE_UNSUPPORTED:
        status = fbe_terminator_service_mark_eses_page_unsupported(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_INIT:
        status = fbe_terminator_service_drive_error_injection_init(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_INIT_ERROR:
        status = fbe_terminator_service_drive_error_injection_init_error(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_ADD_ERROR:
        status = fbe_terminator_service_drive_error_injection_add_error(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_START:
        status = fbe_terminator_service_drive_error_injection_start(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_STOP:
        status = fbe_terminator_service_drive_error_injection_stop(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_ERROR_INJECTION_DESTROY:
        status = fbe_terminator_service_drive_error_injection_destroy(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_TYPE:
        status = fbe_terminator_service_set_simulated_drive_type(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_SERVER_NAME:
        status = fbe_terminator_service_set_simulated_drive_server_name(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_SERVER_PORT:
        status = fbe_terminator_service_set_simulated_drive_server_port(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SIMULATED_DRIVE_DEBUG_FLAGS:
        status = fbe_terminator_service_set_simulated_drive_debug_flags(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_DEVICE_ATTRIBUTE:
        status = fbe_terminator_service_set_device_attribute(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SAS_ENCLOSURE_DRIVE_SLOT_ESES_STATUS:
        status = fbe_terminator_service_set_sas_enclosure_drive_slot_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_TEMP_SENSOR_ESES_STATUS:
        status = fbe_terminator_service_set_temp_sensor_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_COOLING_ESES_STATUS:
        status = fbe_terminator_service_set_cooling_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_PS_ESES_STATUS:
        status = fbe_terminator_service_set_ps_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_LCC_ESES_STATUS:
        status = fbe_terminator_service_set_lcc_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SAS_CONN_ESES_STATUS:
        status = fbe_terminator_service_set_sas_conn_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_BUF:
        status = fbe_terminator_service_set_buf(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_BUF_BY_BUF_ID:
        status = fbe_terminator_service_set_buf_by_buf_id(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_RESUME_PROM_INFO:
        status = fbe_terminator_service_set_resume_prom_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_IO_MODE:
        status = fbe_terminator_service_set_io_mode(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_IO_COMPLETION_IRQL:
        status = fbe_terminator_set_io_completion_irql(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_IO_GLOBAL_COMPLETION_DELAY:
        status = fbe_terminator_service_set_io_global_completion_delay(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_DRIVE_PRODUCT_ID:
        status = fbe_terminator_service_set_drive_product_id(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_NEED_UPDATE_ENCLOSURE_FIRMWARE_REV:
        status = fbe_terminator_service_set_need_update_enclosure_firmware_rev(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_NEED_UPDATE_ENCLOSURE_FIRMWARE_REV:
            status = fbe_terminator_service_get_need_update_enclosure_firmware_rev(packet);
            break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_NEED_UPDATE_ENCLOSURE_RESUME_PROM_CHECKSUM:
        status = fbe_terminator_service_set_need_update_enclosure_resume_prom_checksum(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_NEED_UPDATE_ENCLOSURE_RESUME_PROM_CHECKSUM:
        status = fbe_terminator_service_get_need_update_enclosure_resume_prom_checksum(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_DEVICE_ATTRIBUTE:
        status = fbe_terminator_service_get_device_attribute(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_PORT_HANDLE:
        status = fbe_terminator_service_get_port_handle(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_HANDLE:
        status = fbe_terminator_service_get_enclosure_handle(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_HANDLE:
        status = fbe_terminator_service_get_drive_handle(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_PRODUCT_ID:
        status = fbe_terminator_service_get_drive_product_id(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_TEMP_SENSOR_ESES_STATUS:
        status = fbe_terminator_service_get_temp_sensor_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_COOLING_ESES_STATUS:
        status = fbe_terminator_service_get_cooling_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_PS_ESES_STATUS:
        status = fbe_terminator_service_get_ps_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_LCC_ESES_STATUS:
        status = fbe_terminator_service_get_lcc_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_SAS_CONN_ESES_STATUS:
        status = fbe_terminator_service_get_sas_conn_eses_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_SAS_DRIVE_INFO:
        status = fbe_terminator_service_get_sas_drive_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_SATA_DRIVE_INFO:
        status = fbe_terminator_service_get_sata_drive_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_ERROR_COUNT:
        status = fbe_terminator_service_get_drive_error_count(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_DEVICE_CPD_DEVICE_ID:
        status = fbe_terminator_service_get_device_cpd_device_id(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_SAS_DRIVE:
        status = fbe_terminator_service_insert_sas_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_INSERT_SATA_DRIVE:
        status = fbe_terminator_service_insert_sata_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_FORCE_CREATE_SAS_DRIVE:
        status = fbe_terminator_service_force_create_sas_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_FORCE_CREATE_SATA_DRIVE:
        status = fbe_terminator_service_force_create_sata_drive(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_RESERVE_MINIPORT_SAS_DEVICE_TABLE_INDEX:
        status = fbe_terminator_service_reserve_miniport_sas_device_table_index(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_MINIPORT_SAS_DEVICE_TABLE_FORCE_ADD:
        status = fbe_terminator_service_miniport_sas_device_table_force_add(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_MINIPORT_SAS_DEVICE_TABLE_FORCE_REMOVE:
        status = fbe_terminator_service_miniport_sas_device_table_force_remove(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_SP_ID:
        status = fbe_terminator_service_set_sp_id(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_SP_ID:
        status = fbe_terminator_service_get_sp_id(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_STATE:
        status = fbe_terminator_service_drive_set_state(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_GET_DEFAULT_PAGE_INFO:
        status = fbe_terminator_service_drive_get_default_page_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_DEFAULT_PAGE_INFO:
        status = fbe_terminator_service_drive_set_default_page_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_DRIVE_SET_DEFAULT_FIELD:
        status = fbe_terminator_service_drive_set_default_field(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_DEVICES_COUNT_BY_TYPE_NAME:
        status = fbe_terminator_service_get_devices_count_by_type_name(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SEND_SPECL_SFI_MASK_DATA:
        status = fbe_terminator_service_send_specl_sfi_mask_data(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL:
        status = fbe_terminator_service_set_enclosure_firmware_activate_time_interval(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_FIRMWARE_ACTIVATE_TIME_INTERVAL:
        status = fbe_terminator_service_get_enclosure_firmware_activate_time_interval(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL:
        status = fbe_terminator_service_set_enclosure_firmware_reset_time_interval(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_ENCLOSURE_FIRMWARE_RESET_TIME_INTERVAL:
        status = fbe_terminator_service_get_enclosure_firmware_reset_time_interval(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_PORT_LINK_INFO:
        status = fbe_terminator_service_get_port_link_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_PORT_LINK_INFO:
        status = fbe_terminator_service_set_port_link_info(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_ENCRYPTION_KEY:
        status = fbe_terminator_service_get_encryption_key(packet);
        break;

    case FBE_TERMINATOR_CONTROL_CODE_PORT_ADDRESS:
        status = fbe_terminator_service_get_port_address(packet);
        break;

    case FBE_TERMINATOR_CONTROL_CODE_GET_CONNECTOR_ID_LIST_FOR_ENCLOSURE:
        status = fbe_terminator_service_get_connector_id_list_for_enclosure(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_DRIVE_SLOT_PARENT:
        status = fbe_terminator_service_get_drive_slot_parent(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_TERMINATOR_DEVICE_COUNT:
        status = fbe_terminator_service_get_terminator_device_count(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_PERSISTENCE_REQUEST:
        status = fbe_terminator_service_set_persistence_request(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_PERSISTENCE_REQUEST:
        status = fbe_terminator_service_get_persistence_request(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_PERSISTENCE_STATUS:
        status = fbe_terminator_service_set_persistence_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_PERSISTENCE_STATUS:
        status = fbe_terminator_service_get_persistence_status(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_ZERO_PERSISTENT_MEMORY:
        status = fbe_terminator_service_zero_persistent_memory(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_SET_LOG_PAGE_31:
        status = fbe_terminator_service_set_log_page_31(packet);
        break;
    case FBE_TERMINATOR_CONTROL_CODE_GET_LOG_PAGE_31:
        status = fbe_terminator_service_get_log_page_31(packet);
        break;
    default:
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "Control code not supported!\n");
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        break;
    }
    return status;
}

static fbe_status_t fbe_terminator_service_terminator_init (fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    status  = fbe_terminator_api_init();

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_terminator_destroy (fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    status  = fbe_terminator_api_destroy();

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_force_login_device (fbe_packet_t *packet)
{
    fbe_terminator_force_login_device_ioctl_t * fbe_terminator_force_login_device_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_force_login_device_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_force_login_device_ioctl); 
    if (fbe_terminator_force_login_device_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_force_login_device (fbe_terminator_force_login_device_ioctl->device_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_force_logout_device (fbe_packet_t *packet)
{
    fbe_terminator_force_logout_device_ioctl_t * fbe_terminator_force_logout_device_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_force_logout_device_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_force_logout_device_ioctl); 
    if (fbe_terminator_force_logout_device_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_force_logout_device (fbe_terminator_force_logout_device_ioctl->device_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_find_device_class (fbe_packet_t *packet)
{
    fbe_terminator_find_device_class_ioctl_t * fbe_terminator_find_device_class_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_find_device_class_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_find_device_class_ioctl); 
    if (fbe_terminator_find_device_class_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_find_device_class (fbe_terminator_find_device_class_ioctl->device_class_name,
                                                    &fbe_terminator_find_device_class_ioctl->device_class_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_create_device_class_instance(fbe_packet_t *packet)
{
    fbe_terminator_create_device_class_instance_ioctl_t * fbe_terminator_create_device_class_instance_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;   

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_create_device_class_instance_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_create_device_class_instance_ioctl); 
    if (fbe_terminator_create_device_class_instance_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_create_device_class_instance (fbe_terminator_create_device_class_instance_ioctl->device_class_handle,
                                                               fbe_terminator_create_device_class_instance_ioctl->device_type,
                                                               &fbe_terminator_create_device_class_instance_ioctl->device_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_insert_board (fbe_packet_t *packet)
{
    fbe_terminator_insert_board_ioctl_t *   fbe_terminator_insert_board_ioctl = NULL;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t *       control_operation = NULL;   

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_board_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_board_ioctl); 
    if (fbe_terminator_insert_board_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_insert_board (&fbe_terminator_insert_board_ioctl->board_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_insert_device (fbe_packet_t *packet)
{
    fbe_terminator_insert_device_ioctl_t * fbe_terminator_insert_device_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_device_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_device_ioctl); 
    if (fbe_terminator_insert_device_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_insert_device (fbe_terminator_insert_device_ioctl->parent_device,
                                                fbe_terminator_insert_device_ioctl->child_device);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_insert_sas_port (fbe_packet_t *packet)
{
    fbe_terminator_insert_sas_port_ioctl_t *        fbe_terminator_insert_sas_port_ioctl = NULL;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_payload_control_operation_t *           control_operation = NULL;
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_sas_port_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_sas_port_ioctl); 
    if (fbe_terminator_insert_sas_port_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_insert_sas_port (&fbe_terminator_insert_sas_port_ioctl->port_info, &fbe_terminator_insert_sas_port_ioctl->port_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_insert_fc_port (fbe_packet_t *packet)
{
    fbe_terminator_insert_fc_port_ioctl_t *        fbe_terminator_insert_fc_port_ioctl = NULL;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_payload_control_operation_t *           control_operation = NULL;
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_fc_port_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_fc_port_ioctl); 
    if (fbe_terminator_insert_fc_port_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_insert_fc_port (&fbe_terminator_insert_fc_port_ioctl->port_info, &fbe_terminator_insert_fc_port_ioctl->port_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_insert_iscsi_port (fbe_packet_t *packet)
{
    fbe_terminator_insert_iscsi_port_ioctl_t *        fbe_terminator_insert_iscsi_port_ioctl = NULL;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_payload_control_operation_t *           control_operation = NULL;
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_iscsi_port_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_iscsi_port_ioctl); 
    if (fbe_terminator_insert_iscsi_port_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_insert_iscsi_port (&fbe_terminator_insert_iscsi_port_ioctl->port_info, &fbe_terminator_insert_iscsi_port_ioctl->port_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_insert_fcoe_port (fbe_packet_t *packet)
{
    fbe_terminator_insert_fcoe_port_ioctl_t *        fbe_terminator_insert_fcoe_port_ioctl = NULL;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t         len = 0;
    fbe_payload_control_operation_t *           control_operation = NULL;
    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_fcoe_port_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_fcoe_port_ioctl); 
    if (fbe_terminator_insert_fcoe_port_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_insert_fcoe_port (&fbe_terminator_insert_fcoe_port_ioctl->port_info, &fbe_terminator_insert_fcoe_port_ioctl->port_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_insert_sas_enclosure (fbe_packet_t *packet)
{
    fbe_terminator_insert_sas_encl_ioctl_t * fbe_terminator_insert_sas_encl_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_sas_encl_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_sas_encl_ioctl); 
    if (fbe_terminator_insert_sas_encl_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_insert_sas_enclosure (fbe_terminator_insert_sas_encl_ioctl->port_handle,
                                                       &fbe_terminator_insert_sas_encl_ioctl->encl_info,
                                                       &fbe_terminator_insert_sas_encl_ioctl->enclosure_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_reinsert_drive (fbe_packet_t *packet)
{
    fbe_terminator_reinsert_drive_ioctl_t * fbe_terminator_reinsert_drive_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_reinsert_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_reinsert_drive_ioctl);
    if (fbe_terminator_reinsert_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_reinsert_drive(fbe_terminator_reinsert_drive_ioctl->enclosure_handle,
                                            fbe_terminator_reinsert_drive_ioctl->slot_number,
                                            fbe_terminator_reinsert_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_reinsert_enclosure (fbe_packet_t *packet)
{
    fbe_terminator_reinsert_enclosure_ioctl_t * fbe_terminator_reinsert_enclosure_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_reinsert_enclosure_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_reinsert_enclosure_ioctl);
    if (fbe_terminator_reinsert_enclosure_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_reinsert_sas_enclosure(fbe_terminator_reinsert_enclosure_ioctl->port_handle,
                                                            fbe_terminator_reinsert_enclosure_ioctl->enclosure_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_simulated_drive_type (fbe_packet_t *packet)
{
    fbe_terminator_set_simulated_drive_type_ioctl_t * fbe_terminator_set_simulated_drive_type_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_simulated_drive_type_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_simulated_drive_type_ioctl); 
    if (fbe_terminator_set_simulated_drive_type_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_simulated_drive_type(fbe_terminator_set_simulated_drive_type_ioctl->simulated_drive_type);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_simulated_drive_type (fbe_packet_t *packet)
{
    fbe_terminator_get_simulated_drive_type_ioctl_t * fbe_terminator_get_simulated_drive_type_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_get_simulated_drive_type_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_simulated_drive_type_ioctl);
    if (fbe_terminator_get_simulated_drive_type_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_simulated_drive_type(&(fbe_terminator_get_simulated_drive_type_ioctl->simulated_drive_type));

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_simulated_drive_server_name (fbe_packet_t *packet)
{
    fbe_terminator_set_simulated_drive_server_name_ioctl_t * fbe_terminator_set_simulated_drive_server_name_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_simulated_drive_server_name_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_simulated_drive_server_name_ioctl);
    if (fbe_terminator_set_simulated_drive_server_name_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_simulated_drive_server_name(fbe_terminator_set_simulated_drive_server_name_ioctl->server_name);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_simulated_drive_server_port (fbe_packet_t *packet)
{
    fbe_terminator_set_simulated_drive_server_port_ioctl_t * fbe_terminator_set_simulated_drive_server_port_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_simulated_drive_server_port_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_simulated_drive_server_port_ioctl);
    if (fbe_terminator_set_simulated_drive_server_port_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_simulated_drive_server_port(fbe_terminator_set_simulated_drive_server_port_ioctl->server_port);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_simulated_drive_debug_flags (fbe_packet_t *packet)
{
    fbe_terminator_set_simulated_drive_debug_flags_ioctl_t *set_simulated_drive_debug_flags_p = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_simulated_drive_debug_flags_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &set_simulated_drive_debug_flags_p);
    if (set_simulated_drive_debug_flags_p == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_set_simulated_drive_debug_flags(set_simulated_drive_debug_flags_p->drive_select_type,            
                                                                set_simulated_drive_debug_flags_p->first_term_drive_index,                                
                                                                set_simulated_drive_debug_flags_p->last_term_drive_index,                                 
                                                                set_simulated_drive_debug_flags_p->backend_bus_number,                                    
                                                                set_simulated_drive_debug_flags_p->encl_number,                                           
                                                                set_simulated_drive_debug_flags_p->slot_number,                                           
                                                                set_simulated_drive_debug_flags_p->terminator_drive_debug_flags);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_device_attribute (fbe_packet_t *packet)
{
    fbe_terminator_set_device_attribute_ioctl_t * fbe_terminator_set_device_attribute_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_device_attribute_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_device_attribute_ioctl); 
    if (fbe_terminator_set_device_attribute_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_device_attribute (fbe_terminator_set_device_attribute_ioctl->device_handle,
                                                       fbe_terminator_set_device_attribute_ioctl->attribute_name,
                                                       fbe_terminator_set_device_attribute_ioctl->attribute_value);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_sas_enclosure_drive_slot_eses_status (fbe_packet_t *packet)
{
    fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl_t * fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl); 
    if (fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_sas_enclosure_drive_slot_eses_status (fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl->enclosure_handle,
                                                                           fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl->slot_number,
                                                                           fbe_terminator_set_sas_enclosure_drive_slot_eses_status_ioctl->drive_slot_stat);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_drive_product_id(fbe_packet_t *packet)
{
    fbe_terminator_set_drive_product_id_ioctl_t * fbe_terminator_set_drive_product_id_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_drive_product_id_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_drive_product_id_ioctl);
    if (fbe_terminator_set_drive_product_id_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_drive_product_id(fbe_terminator_set_drive_product_id_ioctl->drive_handle,
                                                     fbe_terminator_set_drive_product_id_ioctl->product_id);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_temp_sensor_eses_status (fbe_packet_t *packet)
{
    fbe_terminator_set_temp_sensor_eses_status_ioctl_t * fbe_terminator_set_temp_sensor_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_temp_sensor_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_temp_sensor_eses_status_ioctl); 
    if (fbe_terminator_set_temp_sensor_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_temp_sensor_eses_status (fbe_terminator_set_temp_sensor_eses_status_ioctl->enclosure_handle,
                                                              fbe_terminator_set_temp_sensor_eses_status_ioctl->temp_sensor_id,
                                                              fbe_terminator_set_temp_sensor_eses_status_ioctl->temp_sensor_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_cooling_eses_status (fbe_packet_t *packet)
{
    fbe_terminator_set_cooling_eses_status_ioctl_t * fbe_terminator_set_cooling_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_cooling_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_cooling_eses_status_ioctl); 
    if (fbe_terminator_set_cooling_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_cooling_eses_status(fbe_terminator_set_cooling_eses_status_ioctl->enclosure_handle,
                                                         fbe_terminator_set_cooling_eses_status_ioctl->cooling_id,
                                                         fbe_terminator_set_cooling_eses_status_ioctl->cooling_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_ps_eses_status(fbe_packet_t *packet)
{
    fbe_terminator_set_ps_eses_status_ioctl_t * fbe_terminator_set_ps_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_ps_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_ps_eses_status_ioctl); 
    if (fbe_terminator_set_ps_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_ps_eses_status (fbe_terminator_set_ps_eses_status_ioctl->enclosure_handle,
                                                     fbe_terminator_set_ps_eses_status_ioctl->ps_id,
                                                     fbe_terminator_set_ps_eses_status_ioctl->ps_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_sas_conn_eses_status(fbe_packet_t *packet)
{
    fbe_terminator_set_sas_conn_eses_status_ioctl_t * fbe_terminator_set_sas_conn_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_sas_conn_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_sas_conn_eses_status_ioctl); 
    if (fbe_terminator_set_sas_conn_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_sas_conn_eses_status (fbe_terminator_set_sas_conn_eses_status_ioctl->enclosure_handle,
                                                     fbe_terminator_set_sas_conn_eses_status_ioctl->sas_conn_id,
                                                     fbe_terminator_set_sas_conn_eses_status_ioctl->sas_conn_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_set_lcc_eses_status(fbe_packet_t *packet)
{
    fbe_terminator_set_lcc_eses_status_ioctl_t * fbe_terminator_set_lcc_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_lcc_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_lcc_eses_status_ioctl);
    if (fbe_terminator_set_lcc_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_lcc_eses_status (fbe_terminator_set_lcc_eses_status_ioctl->enclosure_handle,
                                                     fbe_terminator_set_lcc_eses_status_ioctl->lcc_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_buf_by_buf_id(fbe_packet_t *packet)
{
    fbe_terminator_set_buf_by_buf_id_ioctl_t * fbe_terminator_set_buf_by_buf_id_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_buf_by_buf_id_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_buf_by_buf_id_ioctl); 
    if (fbe_terminator_set_buf_by_buf_id_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_buf_by_buf_id (fbe_terminator_set_buf_by_buf_id_ioctl->enclosure_handle,
                                                    fbe_terminator_set_buf_by_buf_id_ioctl->buf_id,
                                                    fbe_terminator_set_buf_by_buf_id_ioctl->buf,
                                                    fbe_terminator_set_buf_by_buf_id_ioctl->len);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_buf(fbe_packet_t *packet)
{
    fbe_terminator_set_buf_ioctl_t * fbe_terminator_set_buf_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_buf_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_buf_ioctl); 
    if (fbe_terminator_set_buf_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_buf (fbe_terminator_set_buf_ioctl->enclosure_handle,
                                          fbe_terminator_set_buf_ioctl->subencl_type,
                                          fbe_terminator_set_buf_ioctl->side,
                                          fbe_terminator_set_buf_ioctl->buf_type,
                                          fbe_terminator_set_buf_ioctl->buf,
                                          fbe_terminator_set_buf_ioctl->len);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_resume_prom_info(fbe_packet_t *packet)
{
    fbe_terminator_set_resume_prom_info_ioctl_t* fbe_terminator_set_resume_prom_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_resume_prom_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_resume_prom_info_ioctl); 
    if (fbe_terminator_set_resume_prom_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_resume_prom_info(fbe_terminator_set_resume_prom_info_ioctl->enclosure_handle,
                                                      fbe_terminator_set_resume_prom_info_ioctl->resume_prom_id,
                                                      fbe_terminator_set_resume_prom_info_ioctl->buf,
                                                      fbe_terminator_set_resume_prom_info_ioctl->len);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_io_mode(fbe_packet_t *packet)
{
    fbe_terminator_set_io_mode_ioctl_t * fbe_terminator_set_io_mode_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_io_mode_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_io_mode_ioctl); 
    if (fbe_terminator_set_io_mode_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_io_mode(fbe_terminator_set_io_mode_ioctl->io_mode);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_set_io_completion_irql(fbe_packet_t *packet)
{
    fbe_terminator_set_io_completion_irql_ioctl_t *set_completion_irql_ioctl_p = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_io_completion_irql_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &set_completion_irql_ioctl_p); 
    if (set_completion_irql_ioctl_p == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_set_io_completion_irql(set_completion_irql_ioctl_p->b_should_completion_be_at_dpc);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_io_global_completion_delay(fbe_packet_t *packet)
{
    fbe_terminator_set_io_global_completion_delay_ioctl_t * fbe_terminator_set_io_global_completion_delay_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_io_global_completion_delay_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_io_global_completion_delay_ioctl); 
    if (fbe_terminator_set_io_global_completion_delay_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_io_global_completion_delay(fbe_terminator_set_io_global_completion_delay_ioctl->global_completion_delay);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_device_attribute (fbe_packet_t *packet)
{
    fbe_terminator_get_device_attribute_ioctl_t * fbe_terminator_get_device_attribute_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_device_attribute_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_device_attribute_ioctl); 
    if (fbe_terminator_get_device_attribute_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_device_attribute (fbe_terminator_get_device_attribute_ioctl->device_handle,
                                                       fbe_terminator_get_device_attribute_ioctl->attribute_name,
                                                       fbe_terminator_get_device_attribute_ioctl->attribute_value_buffer,
                                                       fbe_terminator_get_device_attribute_ioctl->buffer_length);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_port_handle (fbe_packet_t *packet)
{
    fbe_terminator_get_port_handle_ioctl_t * fbe_terminator_get_port_handle_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_port_handle_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_port_handle_ioctl); 
    if (fbe_terminator_get_port_handle_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_port_handle (fbe_terminator_get_port_handle_ioctl->backend_number,
                                                  &fbe_terminator_get_port_handle_ioctl->port_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_enclosure_handle (fbe_packet_t *packet)
{
    fbe_terminator_get_enclosure_handle_ioctl_t * fbe_terminator_get_enclosure_handle_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_enclosure_handle_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_enclosure_handle_ioctl); 
    if (fbe_terminator_get_enclosure_handle_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_enclosure_handle (fbe_terminator_get_enclosure_handle_ioctl->port_number,
                                                       fbe_terminator_get_enclosure_handle_ioctl->enclosure_number,
                                                       &fbe_terminator_get_enclosure_handle_ioctl->enclosure_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_drive_handle (fbe_packet_t *packet)
{
    fbe_terminator_get_drive_handle_ioctl_t * fbe_terminator_get_drive_handle_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_drive_handle_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_drive_handle_ioctl); 
    if (fbe_terminator_get_drive_handle_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_drive_handle (fbe_terminator_get_drive_handle_ioctl->port_number,
                                                   fbe_terminator_get_drive_handle_ioctl->enclosure_number,
                                                   fbe_terminator_get_drive_handle_ioctl->slot_number,
                                                   &fbe_terminator_get_drive_handle_ioctl->drive_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_temp_sensor_eses_status (fbe_packet_t *packet)
{
    fbe_terminator_get_temp_sensor_eses_status_ioctl_t * fbe_terminator_get_temp_sensor_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_temp_sensor_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_temp_sensor_eses_status_ioctl); 
    if (fbe_terminator_get_temp_sensor_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_temp_sensor_eses_status (fbe_terminator_get_temp_sensor_eses_status_ioctl->enclosure_handle,
                                                              fbe_terminator_get_temp_sensor_eses_status_ioctl->temp_sensor_id,
                                                              &fbe_terminator_get_temp_sensor_eses_status_ioctl->temp_sensor_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_cooling_eses_status (fbe_packet_t *packet)
{
    fbe_terminator_get_cooling_eses_status_ioctl_t * fbe_terminator_get_cooling_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_cooling_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_cooling_eses_status_ioctl); 
    if (fbe_terminator_get_cooling_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_cooling_eses_status(fbe_terminator_get_cooling_eses_status_ioctl->enclosure_handle,
                                                         fbe_terminator_get_cooling_eses_status_ioctl->cooling_id,
                                                         &fbe_terminator_get_cooling_eses_status_ioctl->cooling_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_ps_eses_status(fbe_packet_t *packet)
{
    fbe_terminator_get_ps_eses_status_ioctl_t * fbe_terminator_get_ps_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_ps_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_ps_eses_status_ioctl); 
    if (fbe_terminator_get_ps_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_ps_eses_status (fbe_terminator_get_ps_eses_status_ioctl->enclosure_handle,
                                                     fbe_terminator_get_ps_eses_status_ioctl->ps_id,
                                                     &fbe_terminator_get_ps_eses_status_ioctl->ps_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_sas_conn_eses_status(fbe_packet_t *packet)
{
    fbe_terminator_get_sas_conn_eses_status_ioctl_t * fbe_terminator_get_sas_conn_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_sas_conn_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_sas_conn_eses_status_ioctl); 
    if (fbe_terminator_get_sas_conn_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_sas_conn_eses_status (fbe_terminator_get_sas_conn_eses_status_ioctl->enclosure_handle,
                                                     fbe_terminator_get_sas_conn_eses_status_ioctl->sas_conn_id,
                                                     &fbe_terminator_get_sas_conn_eses_status_ioctl->sas_conn_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_get_lcc_eses_status(fbe_packet_t *packet)
{
    fbe_terminator_get_lcc_eses_status_ioctl_t * fbe_terminator_get_lcc_eses_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_get_lcc_eses_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_lcc_eses_status_ioctl);
    if (fbe_terminator_get_lcc_eses_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_lcc_eses_status (fbe_terminator_get_lcc_eses_status_ioctl->enclosure_handle,
                                                     &fbe_terminator_get_lcc_eses_status_ioctl->lcc_stat);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_get_sas_drive_info(fbe_packet_t *packet)
{
    fbe_terminator_get_sas_drive_info_ioctl_t * fbe_terminator_get_sas_drive_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_sas_drive_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_sas_drive_info_ioctl); 
    if (fbe_terminator_get_sas_drive_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_sas_drive_info (fbe_terminator_get_sas_drive_info_ioctl->drive_handle,
                                                     &fbe_terminator_get_sas_drive_info_ioctl->drive_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_get_sata_drive_info(fbe_packet_t *packet)
{
    fbe_terminator_get_sata_drive_info_ioctl_t * fbe_terminator_get_sata_drive_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_sata_drive_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_sata_drive_info_ioctl); 
    if (fbe_terminator_get_sata_drive_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_sata_drive_info (fbe_terminator_get_sata_drive_info_ioctl->drive_handle,
                                                     &fbe_terminator_get_sata_drive_info_ioctl->drive_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_drive_product_id(fbe_packet_t *packet)
{
    fbe_terminator_get_drive_product_id_ioctl_t * fbe_terminator_get_drive_product_id_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_get_drive_product_id_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_drive_product_id_ioctl);
    if (fbe_terminator_get_drive_product_id_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_drive_product_id(fbe_terminator_get_drive_product_id_ioctl->drive_handle,
                                                     fbe_terminator_get_drive_product_id_ioctl->product_id);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_drive_error_count(fbe_packet_t *packet)
{
    fbe_terminator_get_drive_error_count_ioctl_t * fbe_terminator_get_drive_error_count_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_drive_error_count_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_drive_error_count_ioctl); 
    if (fbe_terminator_get_drive_error_count_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_drive_error_count (fbe_terminator_get_drive_error_count_ioctl->handle,
                                                        &fbe_terminator_get_drive_error_count_ioctl->error_count_p);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_device_cpd_device_id(fbe_packet_t *packet)
{
    fbe_terminator_get_device_cpd_device_id_ioctl_t * fbe_terminator_get_device_cpd_device_id_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_device_cpd_device_id_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_device_cpd_device_id_ioctl); 
    if (fbe_terminator_get_device_cpd_device_id_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_device_cpd_device_id(fbe_terminator_get_device_cpd_device_id_ioctl->device_handle,
                                                          &fbe_terminator_get_device_cpd_device_id_ioctl->cpd_device_id);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation)
{
    //fbe_payload_ex_t *     payload = NULL;
	fbe_payload_ex_t * 	payload_ex = NULL;
	fbe_payload_operation_header_t * payload_operation_header = NULL;

	payload_ex = (fbe_payload_ex_t *)fbe_transport_get_payload_ex(packet);

	*control_operation = NULL;

	if(payload_ex->current_operation == NULL){	
		if(payload_ex->next_operation != NULL){
			payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->next_operation;

			/* Check if current operation is control operation */
			if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
				return FBE_STATUS_GENERIC_FAILURE;
			}

			*control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
		} else {
			return FBE_STATUS_GENERIC_FAILURE;
		}
	} else {
		payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->current_operation;

		/* Check if current operation is control operation */
		if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
			return FBE_STATUS_GENERIC_FAILURE;
		}

		*control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
	}
/*
    payload = fbe_transport_get_payload_ex(packet);
    *control_operation = payload->control_operation;
*/
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_insert_sas_drive (fbe_packet_t *packet)
{
    fbe_terminator_insert_sas_drive_ioctl_t * fbe_terminator_insert_sas_drive_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_sas_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_sas_drive_ioctl); 
    if (fbe_terminator_insert_sas_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_insert_sas_drive (fbe_terminator_insert_sas_drive_ioctl->enclosure_handle,
                                                   fbe_terminator_insert_sas_drive_ioctl->slot_number,
                                                   &fbe_terminator_insert_sas_drive_ioctl->drive_info,
                                                   &fbe_terminator_insert_sas_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;}

static fbe_status_t fbe_terminator_service_remove_device (fbe_packet_t *packet)
{
    fbe_terminator_remove_device_ioctl_t * fbe_terminator_remove_device_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_remove_device_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_remove_device_ioctl); 
    if (fbe_terminator_remove_device_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_remove_device (fbe_terminator_remove_device_ioctl->device_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_remove_sas_enclosure (fbe_packet_t *packet)
{
    fbe_terminator_remove_sas_enclosure_ioctl_t * fbe_terminator_remove_sas_enclosure_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_remove_sas_enclosure_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_remove_sas_enclosure_ioctl); 
    if (fbe_terminator_remove_sas_enclosure_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_remove_sas_enclosure (fbe_terminator_remove_sas_enclosure_ioctl->enclosure_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_remove_sas_drive (fbe_packet_t *packet)
{
    fbe_terminator_remove_sas_drive_ioctl_t * fbe_terminator_remove_sas_drive_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_remove_sas_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_remove_sas_drive_ioctl); 
    if (fbe_terminator_remove_sas_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_remove_sas_drive (fbe_terminator_remove_sas_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_remove_sata_drive (fbe_packet_t *packet)

{
    fbe_terminator_remove_sata_drive_ioctl_t * fbe_terminator_remove_sata_drive_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_remove_sata_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_remove_sata_drive_ioctl); 
    if (fbe_terminator_remove_sata_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_remove_sata_drive (fbe_terminator_remove_sata_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_pull_drive (fbe_packet_t *packet)
{
    fbe_terminator_pull_drive_ioctl_t * fbe_terminator_pull_drive_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_pull_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_pull_drive_ioctl);
    if (fbe_terminator_pull_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_pull_drive (fbe_terminator_pull_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_pull_enclosure (fbe_packet_t *packet)
{
    fbe_terminator_pull_enclosure_ioctl_t  * fbe_terminator_pull_enclosure_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_pull_enclosure_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_pull_enclosure_ioctl);
    if (fbe_terminator_pull_enclosure_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_pull_sas_enclosure(fbe_terminator_pull_enclosure_ioctl->enclosure_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_activate_device (fbe_packet_t *packet)
{
    fbe_terminator_activate_device_ioctl_t * fbe_terminator_activate_device_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_activate_device_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_activate_device_ioctl); 
    if (fbe_terminator_activate_device_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_activate_device (fbe_terminator_activate_device_ioctl->device_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_start_port_reset (fbe_packet_t *packet)
{
    fbe_terminator_start_port_reset_ioctl_t * fbe_terminator_start_port_reset_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_start_port_reset_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_start_port_reset_ioctl); 
    if (fbe_terminator_start_port_reset_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_start_port_reset (fbe_terminator_start_port_reset_ioctl->port_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_complete_port_reset (fbe_packet_t *packet)
{
    fbe_terminator_complete_port_reset_ioctl_t * fbe_terminator_complete_port_reset_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_complete_port_reset_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_complete_port_reset_ioctl); 
    if (fbe_terminator_complete_port_reset_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_complete_port_reset (fbe_terminator_complete_port_reset_ioctl->port_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_hardware_info (fbe_packet_t *packet)
{
    fbe_terminator_get_hardware_info_ioctl_t * fbe_terminator_get_hardware_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_hardware_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_hardware_info_ioctl); 
    if (fbe_terminator_get_hardware_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_hardware_info (fbe_terminator_get_hardware_info_ioctl->port_handle, &fbe_terminator_get_hardware_info_ioctl->hdw_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_hardware_info (fbe_packet_t *packet)
{
    fbe_terminator_set_hardware_info_ioctl_t * fbe_terminator_set_hardware_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_hardware_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_hardware_info_ioctl); 
    if (fbe_terminator_set_hardware_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_set_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_hardware_info (fbe_terminator_set_hardware_info_ioctl->port_handle, &fbe_terminator_set_hardware_info_ioctl->hdw_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_sfp_media_interface_info (fbe_packet_t *packet)
{
    fbe_terminator_get_sfp_media_interface_info_ioctl_t * fbe_terminator_get_sfp_media_interface_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_sfp_media_interface_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_sfp_media_interface_info_ioctl); 
    if (fbe_terminator_get_sfp_media_interface_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_sfp_media_interface_info (fbe_terminator_get_sfp_media_interface_info_ioctl->port_handle, &fbe_terminator_get_sfp_media_interface_info_ioctl->sfp_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_sfp_media_interface_info (fbe_packet_t *packet)
{
    fbe_terminator_set_sfp_media_interface_info_ioctl_t * fbe_terminator_set_sfp_media_interface_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_sfp_media_interface_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_sfp_media_interface_info_ioctl); 
    if (fbe_terminator_set_sfp_media_interface_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_set_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_sfp_media_interface_info (fbe_terminator_set_sfp_media_interface_info_ioctl->port_handle, &fbe_terminator_set_sfp_media_interface_info_ioctl->sfp_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/* enclosure */
static fbe_status_t fbe_terminator_service_enclosure_bypass_drive_slot (fbe_packet_t *packet)
{
    fbe_terminator_enclosure_bypass_drive_slot_ioctl_t * fbe_terminator_enclosure_bypass_drive_slot_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_enclosure_bypass_drive_slot_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_enclosure_bypass_drive_slot_ioctl); 
    if (fbe_terminator_enclosure_bypass_drive_slot_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_enclosure_bypass_drive_slot (fbe_terminator_enclosure_bypass_drive_slot_ioctl->enclosure_handle,
                                                              fbe_terminator_enclosure_bypass_drive_slot_ioctl->slot_number);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_enclosure_unbypass_drive_slot (fbe_packet_t *packet)
{
    fbe_terminator_enclosure_unbypass_drive_slot_ioctl_t * fbe_terminator_enclosure_unbypass_drive_slot_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_enclosure_unbypass_drive_slot_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_enclosure_unbypass_drive_slot_ioctl); 
    if (fbe_terminator_enclosure_unbypass_drive_slot_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_enclosure_unbypass_drive_slot (fbe_terminator_enclosure_unbypass_drive_slot_ioctl->enclosure_handle,
                                                              fbe_terminator_enclosure_unbypass_drive_slot_ioctl->slot_number);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_enclosure_getEmcEnclStatus (fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_enclosure_emc_encl_status_ioctl_t * fbe_terminator_enclosure_getEmcEnclStatus_ioctl = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_enclosure_emc_encl_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_enclosure_getEmcEnclStatus_ioctl); 
    if (fbe_terminator_enclosure_getEmcEnclStatus_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_enclosure_getEmcEnclStatus (fbe_terminator_enclosure_getEmcEnclStatus_ioctl->enclosure_handle,
                                                             &fbe_terminator_enclosure_getEmcEnclStatus_ioctl->emcEnclStatus);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_enclosure_setEmcEnclStatus (fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_enclosure_emc_encl_status_ioctl_t * fbe_terminator_enclosure_setEmcEnclStatus_ioctl = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_enclosure_emc_encl_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_enclosure_setEmcEnclStatus_ioctl); 
    if (fbe_terminator_enclosure_setEmcEnclStatus_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_enclosure_setEmcEnclStatus (fbe_terminator_enclosure_setEmcEnclStatus_ioctl->enclosure_handle,
                                                             &fbe_terminator_enclosure_setEmcEnclStatus_ioctl->emcEnclStatus);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_enclosure_getEmcPsInfoStatus (fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_enclosure_emc_ps_info_status_ioctl_t * fbe_terminator_enclosure_getEmcPsInfoStatus_ioctl = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_enclosure_emc_ps_info_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_enclosure_getEmcPsInfoStatus_ioctl); 
    if (fbe_terminator_enclosure_getEmcPsInfoStatus_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_enclosure_getEmcPsInfoStatus (fbe_terminator_enclosure_getEmcPsInfoStatus_ioctl->enclosure_handle,
                                                               &fbe_terminator_enclosure_getEmcPsInfoStatus_ioctl->emcPsInfoStatus);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_enclosure_setEmcPsInfoStatus (fbe_packet_t *packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_enclosure_emc_ps_info_status_ioctl_t * fbe_terminator_enclosure_setEmcPsInfoStatus_ioctl = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_enclosure_emc_ps_info_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_enclosure_setEmcPsInfoStatus_ioctl); 
    if (fbe_terminator_enclosure_setEmcPsInfoStatus_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_enclosure_setEmcPsInfoStatus (fbe_terminator_enclosure_setEmcPsInfoStatus_ioctl->enclosure_handle,
                                                               &fbe_terminator_enclosure_setEmcPsInfoStatus_ioctl->emcPsInfoStatus);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_eses_increment_config_page_gen_code (fbe_packet_t *packet)
{
    fbe_terminator_eses_increment_config_page_gen_code_ioctl_t * fbe_terminator_eses_increment_config_page_gen_code_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_eses_increment_config_page_gen_code_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_eses_increment_config_page_gen_code_ioctl); 
    if (fbe_terminator_eses_increment_config_page_gen_code_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_eses_increment_config_page_gen_code (fbe_terminator_eses_increment_config_page_gen_code_ioctl->enclosure_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_eses_set_download_microcode_stat_page_stat_desc (fbe_packet_t *packet)
{
    fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl_t * fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl); 
    if (fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_eses_set_download_microcode_stat_page_stat_desc (fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl->enclosure_handle,
                                                                                  fbe_terminator_eses_set_download_microcode_stat_page_stat_desc_ioctl->download_stat_desc);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_eses_get_ver_desc (fbe_packet_t *packet)
{
    fbe_terminator_eses_get_ver_desc_ioctl_t * fbe_terminator_eses_get_ver_desc_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_eses_get_ver_desc_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_eses_get_ver_desc_ioctl); 
    if (fbe_terminator_eses_get_ver_desc_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_eses_get_ver_desc (fbe_terminator_eses_get_ver_desc_ioctl->enclosure_handle,
                                                    fbe_terminator_eses_get_ver_desc_ioctl->subencl_type,
                                                    fbe_terminator_eses_get_ver_desc_ioctl->side,
                                                    fbe_terminator_eses_get_ver_desc_ioctl->comp_type,
                                                    &fbe_terminator_eses_get_ver_desc_ioctl->ver_desc);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_eses_set_ver_desc (fbe_packet_t *packet)
{
    fbe_terminator_eses_set_ver_desc_ioctl_t * fbe_terminator_eses_set_ver_desc_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_eses_set_ver_desc_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_eses_set_ver_desc_ioctl); 
    if (fbe_terminator_eses_set_ver_desc_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_eses_set_ver_desc (fbe_terminator_eses_set_ver_desc_ioctl->enclosure_handle,
                                                    fbe_terminator_eses_set_ver_desc_ioctl->subencl_type,
                                                    fbe_terminator_eses_set_ver_desc_ioctl->side,
                                                    fbe_terminator_eses_set_ver_desc_ioctl->comp_type,
                                                    fbe_terminator_eses_set_ver_desc_ioctl->ver_desc);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_eses_set_unit_attention (fbe_packet_t *packet)
{
    fbe_terminator_eses_set_unit_attention_ioctl_t * fbe_terminator_eses_set_unit_attention_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_eses_set_unit_attention_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_eses_set_unit_attention_ioctl); 
    if (fbe_terminator_eses_set_unit_attention_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_unit_attention (fbe_terminator_eses_set_unit_attention_ioctl->enclosure_handle);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_mark_eses_page_unsupported (fbe_packet_t *packet)
{
    fbe_terminator_mark_eses_page_unsupported_ioctl_t * fbe_terminator_mark_eses_page_unsupported_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_mark_eses_page_unsupported_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_mark_eses_page_unsupported_ioctl); 
    if (fbe_terminator_mark_eses_page_unsupported_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_mark_eses_page_unsupported(fbe_terminator_mark_eses_page_unsupported_ioctl->cdb_opcode,
                                                            fbe_terminator_mark_eses_page_unsupported_ioctl->diag_page_code);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_insert_sata_drive (fbe_packet_t *packet)
{
    fbe_terminator_insert_sata_drive_ioctl_t * fbe_terminator_insert_sata_drive_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_insert_sata_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_insert_sata_drive_ioctl); 
    if (fbe_terminator_insert_sata_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_insert_sata_drive (fbe_terminator_insert_sata_drive_ioctl->enclosure_handle,
                                                   fbe_terminator_insert_sata_drive_ioctl->slot_number,
                                                   &fbe_terminator_insert_sata_drive_ioctl->drive_info,
                                                   &fbe_terminator_insert_sata_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_terminator_service_drive_error_injection_init (fbe_packet_t *packet)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    status  = fbe_terminator_api_drive_error_injection_init();
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_drive_error_injection_init_error (fbe_packet_t *packet)
{
    fbe_terminator_drive_error_injection_init_error_ioctl_t * fbe_terminator_drive_error_injection_init_error_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_drive_error_injection_init_error_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_drive_error_injection_init_error_ioctl); 
    if (fbe_terminator_drive_error_injection_init_error_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_drive_error_injection_init_error (&fbe_terminator_drive_error_injection_init_error_ioctl->record);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_drive_error_injection_add_error (fbe_packet_t *packet)
{
    fbe_terminator_drive_error_injection_add_error_ioctl_t * fbe_terminator_drive_error_injection_add_error_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_drive_error_injection_add_error_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_drive_error_injection_add_error_ioctl); 
    if (fbe_terminator_drive_error_injection_add_error_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_drive_error_injection_add_error (fbe_terminator_drive_error_injection_add_error_ioctl->record);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_drive_error_injection_start (fbe_packet_t *packet)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    status  = fbe_terminator_api_drive_error_injection_start();
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_drive_error_injection_stop (fbe_packet_t *packet)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    status  = fbe_terminator_api_drive_error_injection_stop();
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_drive_error_injection_destroy (fbe_packet_t *packet)
{
    fbe_status_t  status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    status  = fbe_terminator_api_drive_error_injection_destroy();
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_reserve_miniport_sas_device_table_index (fbe_packet_t *packet)
{
    fbe_terminator_reserve_miniport_sas_device_table_index_ioctl_t * fbe_terminator_reserve_miniport_sas_device_table_index_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_reserve_miniport_sas_device_table_index_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_reserve_miniport_sas_device_table_index_ioctl); 
    if (fbe_terminator_reserve_miniport_sas_device_table_index_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_reserve_miniport_sas_device_table_index(fbe_terminator_reserve_miniport_sas_device_table_index_ioctl->port_number,
                                                                         fbe_terminator_reserve_miniport_sas_device_table_index_ioctl->cpd_device_id);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_miniport_sas_device_table_force_add (fbe_packet_t *packet)
{
    fbe_terminator_miniport_sas_device_table_force_add_ioctl_t * fbe_terminator_miniport_sas_device_table_force_add_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_miniport_sas_device_table_force_add_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_miniport_sas_device_table_force_add_ioctl); 
    if (fbe_terminator_miniport_sas_device_table_force_add_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_miniport_sas_device_table_force_add(fbe_terminator_miniport_sas_device_table_force_add_ioctl->device_handle,
                                                                     fbe_terminator_miniport_sas_device_table_force_add_ioctl->cpd_device_id);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_miniport_sas_device_table_force_remove (fbe_packet_t *packet)
{
    fbe_terminator_miniport_sas_device_table_force_remove_ioctl_t * fbe_terminator_miniport_sas_device_table_force_remove_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t  len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_miniport_sas_device_table_force_remove_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_miniport_sas_device_table_force_remove_ioctl); 
    if (fbe_terminator_miniport_sas_device_table_force_remove_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_miniport_sas_device_table_force_remove(fbe_terminator_miniport_sas_device_table_force_remove_ioctl->device_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_force_create_sas_drive (fbe_packet_t *packet)
{
    fbe_terminator_force_create_sas_drive_ioctl_t * fbe_terminator_force_create_sas_drive_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_force_create_sas_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_force_create_sas_drive_ioctl); 
    if (fbe_terminator_force_create_sas_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_force_create_sas_drive (&fbe_terminator_force_create_sas_drive_ioctl->drive_info,
                                                   &fbe_terminator_force_create_sas_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_terminator_service_force_create_sata_drive (fbe_packet_t *packet)
{
    fbe_terminator_force_create_sata_drive_ioctl_t * fbe_terminator_force_create_sata_drive_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_force_create_sata_drive_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_force_create_sata_drive_ioctl); 
    if (fbe_terminator_force_create_sata_drive_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_force_create_sata_drive (&fbe_terminator_force_create_sata_drive_ioctl->drive_info,
                                                   &fbe_terminator_force_create_sata_drive_ioctl->drive_handle);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_terminator_service_set_sp_id(fbe_packet_t *packet)
{
    fbe_terminator_miniport_sp_id_ioctl_t *  fbe_terminator_miniport_sp_id_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_miniport_sp_id_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_miniport_sp_id_ioctl); 
    if (fbe_terminator_miniport_sp_id_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_sp_id(fbe_terminator_miniport_sp_id_ioctl->sp_id);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_terminator_service_get_sp_id(fbe_packet_t *packet)
{
    fbe_terminator_miniport_sp_id_ioctl_t *  fbe_terminator_miniport_sp_id_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_miniport_sp_id_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_miniport_sp_id_ioctl); 
    if (fbe_terminator_miniport_sp_id_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_sp_id(&fbe_terminator_miniport_sp_id_ioctl->sp_id);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
static fbe_status_t fbe_terminator_service_drive_set_state (fbe_packet_t *packet)
{
    fbe_terminator_drive_set_state_ioctl_t * fbe_terminator_drive_set_state_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_drive_set_state_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_drive_set_state_ioctl); 
    if (fbe_terminator_drive_set_state_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_drive_set_state (fbe_terminator_drive_set_state_ioctl->device_handle,fbe_terminator_drive_set_state_ioctl->drive_state);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_drive_get_default_page_info(fbe_packet_t *packet)
{
    fbe_terminator_drive_type_default_page_info_ioctl_t * ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_drive_type_default_page_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &ioctl); 
    if (ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_drive_get_default_page_info (ioctl->drive_type, &ioctl->default_info);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_drive_set_default_page_info(fbe_packet_t *packet)
{
    fbe_terminator_drive_type_default_page_info_ioctl_t * ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_drive_type_default_page_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &ioctl); 
    if (ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status  = fbe_terminator_api_drive_set_default_page_info (ioctl->drive_type, &ioctl->default_info);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_drive_set_default_field(fbe_packet_t *packet)
{
    fbe_terminator_drive_default_ioctl_t * p_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_sg_element_t * sg_element = NULL;
    fbe_u32_t sg_elements = 0;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_drive_default_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d != %d\n", 
                         __FUNCTION__, len, (int)sizeof(fbe_terminator_drive_default_ioctl_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &p_ioctl); 
    if (p_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    if (sg_elements != 1)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s invalid sg_element count. %d != 1\n", __FUNCTION__, sg_elements);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_terminator_api_sas_drive_set_default_field(p_ioctl->drive_type, p_ioctl->field, sg_element->address, sg_element->count);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_devices_count_by_type_name(fbe_packet_t *packet)
{
    fbe_terminator_get_devices_count_by_type_name_ioctl_t *  get_devices_count_by_type_name_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_devices_count_by_type_name_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_devices_count_by_type_name_ioctl); 
    if (get_devices_count_by_type_name_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_devices_count_by_type_name(get_devices_count_by_type_name_ioctl->device_type_name, &get_devices_count_by_type_name_ioctl->device_count);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_terminator_service_send_specl_sfi_mask_data(fbe_packet_t *packet)
{
    fbe_terminator_send_specl_sfi_mask_data_ioctl_t *  fbe_terminator_send_specl_sfi_mask_data_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 
    PSPECL_SFI_MASK_DATA sfi_mask_data_ptr = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_send_specl_sfi_mask_data_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_send_specl_sfi_mask_data_ioctl);
    if (fbe_terminator_send_specl_sfi_mask_data_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sfi_mask_data_ptr = &(fbe_terminator_send_specl_sfi_mask_data_ioctl->specl_sfi_mask_data);
    status = fbe_terminator_api_send_specl_sfi_mask_data(sfi_mask_data_ptr);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_need_update_enclosure_resume_prom_checksum (fbe_packet_t *packet)
{
    fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl_t *  fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl);
    if (fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_set_need_update_enclosure_resume_prom_checksum(fbe_terminator_set_need_update_enclosure_resume_prom_checksum_ioctl->need_update_checksum);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_need_update_enclosure_resume_prom_checksum (fbe_packet_t *packet)
{
    fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl_t *  fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl);
    if (fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_get_need_update_enclosure_resume_prom_checksum(&fbe_terminator_get_need_update_enclosure_resume_prom_checksum_ioctl->need_update_checksum);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_need_update_enclosure_firmware_rev (fbe_packet_t *packet)
{
    fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl_t *  fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl);
    if (fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_set_need_update_enclosure_firmware_rev(fbe_terminator_set_need_update_enclosure_firmware_rev_ioctl->need_update_rev);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_need_update_enclosure_firmware_rev (fbe_packet_t *packet)
{
    fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl_t *  fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl);
    if (fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_get_need_update_enclosure_firmware_rev(&fbe_terminator_get_need_update_enclosure_firmware_rev_ioctl->need_update_rev);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_enclosure_firmware_activate_time_interval (fbe_packet_t *packet)
{
    fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl_t *  fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl);
    if (fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_set_enclosure_firmware_activate_time_interval(fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl->enclosure_handle,
                                                                                            fbe_terminator_set_enclosure_firmware_activate_time_interval_ioctl->time_interval);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_enclosure_firmware_activate_time_interval (fbe_packet_t *packet)
{
    fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl_t *  fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl);
    if (fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_get_enclosure_firmware_activate_time_interval(fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl->enclosure_handle,
                                                                                            &fbe_terminator_get_enclosure_firmware_activate_time_interval_ioctl->time_interval);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_enclosure_firmware_reset_time_interval (fbe_packet_t *packet)
{
    fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl_t *  fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl);
    if (fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_set_enclosure_firmware_reset_time_interval(fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl->enclosure_handle,
                                                                                            fbe_terminator_set_enclosure_firmware_reset_time_interval_ioctl->time_interval);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_enclosure_firmware_reset_time_interval (fbe_packet_t *packet)
{
    fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl_t *  fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl = NULL;
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t      len = 0;
    fbe_payload_control_operation_t * control_operation = NULL;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl);
    if (fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_get_enclosure_firmware_reset_time_interval(fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl->enclosure_handle,
                                                                                            &fbe_terminator_get_enclosure_firmware_reset_time_interval_ioctl->time_interval);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_get_port_link_info (fbe_packet_t *packet)
{
    fbe_terminator_get_port_link_info_ioctl_t * terminator_get_port_link_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_port_link_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &terminator_get_port_link_info_ioctl); 
    if (terminator_get_port_link_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_port_link_info (terminator_get_port_link_info_ioctl->port_handle, &terminator_get_port_link_info_ioctl->port_link_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_port_link_info (fbe_packet_t *packet)
{
    fbe_terminator_set_port_link_info_ioctl_t * terminator_set_port_link_info_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_port_link_info_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &terminator_set_port_link_info_ioctl); 
    if (terminator_set_port_link_info_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_set_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_port_link_info (terminator_set_port_link_info_ioctl->port_handle, &terminator_set_port_link_info_ioctl->port_link_info);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_encryption_key (fbe_packet_t *packet)
{
    fbe_terminator_get_encryption_key_t * encryption_key= NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_encryption_key_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &encryption_key); 
    if (encryption_key == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_set_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_encryption_key(encryption_key->port_handle, 
                                                    encryption_key->key_handle,
                                                    encryption_key->key_buffer);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_port_address (fbe_packet_t *packet)
{
    fbe_terminator_get_port_address_t * port_address = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_port_address_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &port_address); 
    if (port_address == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_set_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_port_address(port_address->port_handle, &port_address->address);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_connector_id_list_for_enclosure(fbe_packet_t *packet)
{
    fbe_terminator_get_connector_id_list_for_enclosure_ioctl_t * fbe_terminator_get_connector_id_list_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_connector_id_list_for_enclosure_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_connector_id_list_ioctl); 
    if (fbe_terminator_get_connector_id_list_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_connector_id_list_for_enclosure (fbe_terminator_get_connector_id_list_ioctl->enclosure_handle,
                                                     &fbe_terminator_get_connector_id_list_ioctl->connector_ids);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_get_drive_slot_parent(fbe_packet_t *packet)
{
    fbe_terminator_get_drive_slot_parent_ioctl_t * fbe_terminator_get_drive_slot_parent_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_drive_slot_parent_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_drive_slot_parent_ioctl); 
    if (fbe_terminator_get_drive_slot_parent_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_enclosure_find_slot_parent (&fbe_terminator_get_drive_slot_parent_ioctl->enclosure_handle,
                                                             &fbe_terminator_get_drive_slot_parent_ioctl->slot_number);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_terminator_service_get_terminator_device_count(fbe_packet_t *packet)
{
    fbe_terminator_get_terminator_device_count_ioctl_t * fbe_terminator_get_terminator_device_count_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_terminator_device_count_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_get_terminator_device_count_ioctl); 
    if (fbe_terminator_get_terminator_device_count_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_api_count_terminator_devices(&fbe_terminator_get_terminator_device_count_ioctl->dev_counts);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_persistence_request(fbe_packet_t *packet)
{
    fbe_terminator_set_persistence_request_ioctl_t * set_persistence_request_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_persistence_request_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &set_persistence_request_ioctl); 
    if (set_persistence_request_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_persistent_memory_set_persistence_request(set_persistence_request_ioctl->request);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_persistence_request(fbe_packet_t *packet)
{
    fbe_terminator_get_persistence_request_ioctl_t * get_persistence_request_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_persistence_request_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_persistence_request_ioctl); 
    if (get_persistence_request_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_persistent_memory_get_persistence_request(&get_persistence_request_ioctl->request);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_set_persistence_status(fbe_packet_t *packet)
{
    fbe_terminator_set_persistence_status_ioctl_t * set_persistence_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_set_persistence_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &set_persistence_status_ioctl); 
    if (set_persistence_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_persistent_memory_set_persistence_status(set_persistence_status_ioctl->status);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_get_persistence_status(fbe_packet_t *packet)
{
    fbe_terminator_get_persistence_status_ioctl_t * get_persistence_status_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_get_persistence_status_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &get_persistence_status_ioctl); 
    if (get_persistence_status_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_persistent_memory_get_persistence_status(&get_persistence_status_ioctl->status);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_terminator_service_zero_persistent_memory(fbe_packet_t *packet)
{
    fbe_terminator_zero_persistent_memory_ioctl_t * zero_persistent_memory_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_zero_persistent_memory_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &zero_persistent_memory_ioctl); 
    if (zero_persistent_memory_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_terminator_persistent_memory_zero_memory();

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_terminator_service_set_log_page_31 ()
 *****************************************************************************
 *
 * @brief   Fet the log page 31 data for the specified drive 
 * 
 * @param   packet - pointer to the packet
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/23/2015  Deanna Heng - Created
 *
 *********************************************************************/
static fbe_status_t fbe_terminator_service_set_log_page_31(fbe_packet_t *packet)
{
    fbe_terminator_log_page_31_ioctl_t * fbe_terminator_log_page_31_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_log_page_31_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_log_page_31_ioctl); 
    if (fbe_terminator_log_page_31_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_set_log_page_31(fbe_terminator_log_page_31_ioctl->device_handle,
                                                 fbe_terminator_log_page_31_ioctl->log_page_31,
                                                 fbe_terminator_log_page_31_ioctl->buffer_length);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!***************************************************************************
 *          fbe_terminator_service_get_log_page_31 ()
 *****************************************************************************
 *
 * @brief   Get the log page 31 data for the specified drive 
 * 
 * @param   packet - pointer to the packet
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/23/2015  Deanna Heng - Created
 *
 *********************************************************************/
static fbe_status_t fbe_terminator_service_get_log_page_31(fbe_packet_t *packet)
{
    fbe_terminator_log_page_31_ioctl_t * fbe_terminator_log_page_31_ioctl = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_control_operation_t * control_operation = NULL; 

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, "%s entry\n", __FUNCTION__);

    fbe_terminator_service_get_control_operation(packet, &control_operation);

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_terminator_log_page_31_ioctl_t))
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN, "%s Invalid buffer_len: %d\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &fbe_terminator_log_page_31_ioctl); 
    if (fbe_terminator_log_page_31_ioctl == NULL)
    {
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status  = fbe_terminator_api_get_log_page_31(fbe_terminator_log_page_31_ioctl->device_handle,
                                                 fbe_terminator_log_page_31_ioctl->log_page_31,
                                                 &fbe_terminator_log_page_31_ioctl->buffer_length);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

