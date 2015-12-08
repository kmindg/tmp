#include "fbe_base_discovered.h"
#include "fbe_base_physical_drive.h"
#include "base_physical_drive_private.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_stat_api.h"
#include "fbe_block_transport.h"
#include "fbe_imaging_structures.h"
#include "fbe_private_space_layout.h"

/* Forward declarations */
static fbe_status_t base_physical_drive_get_slot_number(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);

static fbe_status_t base_physical_drive_attach_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_detach_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);

static fbe_status_t base_physical_drive_attach_block_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_detach_block_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_open_block_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);

static fbe_status_t base_physical_drive_get_dieh_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_clear_error_counts(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_clear_eol(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_qdepth(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_qdepth(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_drive_qdepth(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_is_wr_cache_enabled(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_default_qdepth(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_drive_information(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_reset_drive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_fw_download(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_fw_download_start_stop(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_fw_download_permit(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_fw_download_reject(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_fw_download_abort(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_default_io_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_default_io_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_enter_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_exit_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_stat_config_table_cahnged(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_enter_power_save_passive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_power_cycle(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_cached_dev_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_start_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_end_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_fail_drive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet); 
static fbe_status_t base_physical_drive_enter_glitch_drive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet); 
static fbe_status_t 
base_physical_drive_send_block_operation(fbe_base_physical_drive_t * base_physical_drive_p, 
                                     fbe_packet_t * packet_p);
static fbe_status_t base_physical_drive_set_drive_death_reason(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_reset_slot(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_handle_logical_error_from_raid_object(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet); 
static fbe_status_t handle_logical_error_unexpected_crc_multi_bits(fbe_base_physical_drive_t *base_physical_drive_p);
static fbe_status_t handle_logical_error_unexpected_crc_single_bit(fbe_base_physical_drive_t *base_physical_drive_p);
static fbe_status_t handle_logical_error_unexpected_crc(fbe_base_physical_drive_t *base_physical_drive_p);
static fbe_status_t handle_logical_error_timeout(fbe_base_physical_drive_t *base_physical_drive_p);
static fbe_status_t base_physical_drive_get_location(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_download_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_service_time(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_service_time(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_handle_clear_logical_errors(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_logical_error_crc_action(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_handle_logical_drive_state_change(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_drive_fault(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_clear_end_of_life(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_update_link_fault(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_power_saving_stats(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_power_saving_stats(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_logical_drive_state(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);

static fbe_status_t base_physical_drive_set_io_throttle(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_throttle_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);

static fbe_status_t base_physical_drive_get_attributes(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_end_of_life(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_generate_ica_stamp(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_read_ica_stamp(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_generate_and_send_ica_stamp(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_carve_memory_for_buffer_and_sgl(fbe_sg_element_t **sg_list_p,
                                                                        fbe_u8_t *write_buffer,
                                                                        fbe_u32_t write_size);
static fbe_status_t base_physical_drive_generate_and_send_ica_stamp_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_physical_drive_send_read_ica_stamp(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_send_read_ica_stamp_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_physical_drive_get_port_info(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_set_dieh_media_threshold(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_dieh_media_threshold(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_reactivate(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet);
static fbe_u32_t base_physical_drive_get_ica_block_count(fbe_base_physical_drive_t * physical_drive);

fbe_status_t 
fbe_base_physical_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_base_physical_drive_t * base_physical_drive = NULL;

    if (object_handle == NULL) {
       return fbe_base_physical_drive_class_control_entry(packet);
    }

    /*KvTrace("fbe_base_physical_drive_main: %s entry\n", __FUNCTION__);*/
    base_physical_drive = (fbe_base_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_INFO:
            status = base_physical_drive_get_dieh_info(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_ERROR_COUNTS:
            status = base_physical_drive_clear_error_counts(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_RESET_DRIVE:
            status = base_physical_drive_reset_drive(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_QDEPTH:
            status = base_physical_drive_set_qdepth(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_QDEPTH:
            status = base_physical_drive_get_qdepth(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_QDEPTH:
            status = base_physical_drive_get_drive_qdepth(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_IS_WR_CACHE_ENABLED:
            status = base_physical_drive_is_wr_cache_enabled(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION:
            status = base_physical_drive_get_drive_information(base_physical_drive, packet);
            break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = base_physical_drive_attach_edge( base_physical_drive, packet);
            break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            status = base_physical_drive_detach_edge( base_physical_drive, packet);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = base_physical_drive_attach_block_edge(base_physical_drive, packet);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            status = base_physical_drive_detach_block_edge(base_physical_drive, packet);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
            status = base_physical_drive_open_block_edge(base_physical_drive, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER:
            status = base_physical_drive_get_slot_number(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD:
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_TRIAL_RUN:
            status = base_physical_drive_fw_download(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_START:
            status = base_physical_drive_fw_download_start_stop(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_STOP:
            status = base_physical_drive_fw_download_start_stop(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_PERMIT:
            status = base_physical_drive_fw_download_permit(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_REJECT:
            status = base_physical_drive_fw_download_reject(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_ABORT:
            status = base_physical_drive_fw_download_abort(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DEFAULT_IO_TIMEOUT:
            status = base_physical_drive_set_default_io_timeout(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DEFAULT_IO_TIMEOUT:
            status = base_physical_drive_get_default_io_timeout(base_physical_drive, packet);
            break;
            /*should be obsolete, we will use new opcode FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING*/
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE:
            status = base_physical_drive_enter_power_save(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_EXIT_POWER_SAVE:
            status = base_physical_drive_exit_power_save(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_STAT_CONFIG_TABLE_CHANGED:
            status = base_physical_drive_stat_config_table_cahnged(base_physical_drive, packet);
            break;
            /*should be obsolete, we will use new opcode FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING*/
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE_PASSIVE:
            status = base_physical_drive_enter_power_save_passive(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_POWER_CYCLE:
            status = base_physical_drive_power_cycle(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_CACHED_DRIVE_INFO:
            status = base_physical_drive_get_cached_dev_info(base_physical_drive, packet);
            break;
		case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING:
			status = base_physical_drive_start_power_save(base_physical_drive, packet);
			break;
		case FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION:
			status = base_physical_drive_end_power_save(base_physical_drive, packet);
			break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_FAIL_DRIVE:
            status = base_physical_drive_fail_drive(base_physical_drive, packet);
            break;
		case FBE_PHYSICAL_DRIVE_CONTROL_ENTER_GLITCH_MODE:
            status = base_physical_drive_enter_glitch_drive(base_physical_drive, packet);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND:
            status = base_physical_drive_send_block_operation(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DRIVE_DEATH_REASON:
            status = base_physical_drive_set_drive_death_reason(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_RESET_SLOT:
            status = base_physical_drive_reset_slot(base_physical_drive, packet);
            break;
        /* this is control code to handle the logical error from raid object
         */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS:
            status = base_physical_drive_handle_logical_error_from_raid_object(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOCATION:
            status = base_physical_drive_get_location(base_physical_drive,
                                                      packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DOWNLOAD_INFO:
            status = base_physical_drive_get_download_info(base_physical_drive, packet);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLEAR_LOGICAL_ERRORS:
            status = base_physical_drive_handle_clear_logical_errors(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SERVICE_TIME:
            status = base_physical_drive_set_service_time(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SERVICE_TIME:
            status = base_physical_drive_get_service_time(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_EOL:
            status = base_physical_drive_clear_eol(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_CRC_ACTION:
            status = base_physical_drive_set_logical_error_crc_action(base_physical_drive, packet);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_LOGICAL_DRIVE_STATE_CHANGED:
            status = base_physical_drive_handle_logical_drive_state_change(base_physical_drive, packet);
            break;        
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_UPDATE_DRIVE_FAULT:
            status = base_physical_drive_set_drive_fault(base_physical_drive, packet);
            break;            
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_UPDATE_LINK_FAULT:
            status = base_physical_drive_update_link_fault(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_END_OF_LIFE:
            status = base_physical_drive_clear_end_of_life(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_END_OF_LIFE:
            status = base_physical_drive_set_end_of_life(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_POWER_SAVING_STATS:
            status = base_physical_drive_get_power_saving_stats(base_physical_drive,packet);
            break;  
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_POWER_SAVING_STATS:
            status = base_physical_drive_set_power_saving_stats(base_physical_drive,packet);
            break;   
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOGICAL_STATE:
            status = base_physical_drive_get_logical_drive_state(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_IO_THROTTLE:
            status = base_physical_drive_set_io_throttle(base_physical_drive, packet);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_THROTTLE_INFO:
            status = base_physical_drive_get_throttle_info(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES:
            status = base_physical_drive_get_attributes(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_GENERATE_ICA_STAMP:
            status = base_physical_drive_generate_ica_stamp(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_READ_ICA_STAMP:
            status = base_physical_drive_read_ica_stamp(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_PORT_INFO:
            status = base_physical_drive_get_port_info(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DIEH_MEDIA_THRESHOLD:
            status = base_physical_drive_set_dieh_media_threshold(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_MEDIA_THRESHOLD:
            status = base_physical_drive_get_dieh_media_threshold(base_physical_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_REACTIVATE:
            status = base_physical_drive_reactivate(base_physical_drive, packet);
            break;

        default:           
            status = fbe_base_discovering_control_entry(object_handle, packet);
            break;
    }
    return status;
}

static fbe_status_t 
base_physical_drive_attach_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_discovery_transport_control_attach_edge_t * discovery_transport_control_attach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_edge_index_t server_index;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_attach_edge);
    if(discovery_transport_control_attach_edge == NULL){
        fbe_base_object_trace((fbe_base_object_t *) base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_discovery_transport_control_attach_edge_t)){
        fbe_base_object_trace((fbe_base_object_t *) base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Right now we do not know all the path state and path attributes rules,
       so we will do something straight forward and may be not exactly correct.
     */

    fbe_discovery_transport_get_server_index(discovery_transport_control_attach_edge->discovery_edge, &server_index);
    /* Edge validatein need to be done here */

    fbe_base_discovering_attach_edge((fbe_base_discovering_t *) base_physical_drive, discovery_transport_control_attach_edge->discovery_edge);
    fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) base_physical_drive, server_index, 0 /* attr */);
    fbe_base_discovering_update_path_state((fbe_base_discovering_t *) base_physical_drive);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
base_physical_drive_detach_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_discovery_transport_control_detach_edge_t * discovery_transport_control_detach_edge = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH /*FBE_TRACE_LEVEL_DEBUG_HIGH*/,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &discovery_transport_control_detach_edge);
    if(discovery_transport_control_detach_edge == NULL){
        fbe_base_object_trace((fbe_base_object_t *) base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    status = fbe_base_discovering_detach_edge((fbe_base_discovering_t *) base_physical_drive, discovery_transport_control_detach_edge->discovery_edge);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t 
base_physical_drive_attach_block_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t    status;
    fbe_block_transport_control_attach_edge_t * block_transport_control_attach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_lba_t capacity_in_physical_drive_blocks;
    fbe_block_size_t block_size_in_bytes;
    fbe_lba_t capacity_in_client_blocks = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_attach_edge);
    if(block_transport_control_attach_edge == NULL){
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_block_transport_control_attach_edge_t)){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Right now we do not know all the path state and path attributes rules,
       so we will do something straight forward and may be not exactly correct.
     */
    fbe_block_transport_server_lock(&base_physical_drive->block_transport_server);

    fbe_block_transport_server_attach_edge(&base_physical_drive->block_transport_server, 
                                        block_transport_control_attach_edge->block_edge,
                                        &fbe_base_physical_drive_lifecycle_const,
                                        (fbe_base_object_t *)base_physical_drive );

    /* Get the physical block size information. */
    fbe_base_physical_drive_get_capacity(base_physical_drive, &capacity_in_physical_drive_blocks, &block_size_in_bytes);
    switch(block_size_in_bytes){
        case 512:
            fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_512_512);
            break;
        case 520:
            fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_520_520);
            break;
        case 4160:
            fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_4160_4160);
            break;
        case 4096:
            fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_4096_4096);
            break;
        default:
            /* Peter Puhov. Workaround terminator problem */            
            if(base_physical_drive->element_type == FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_SSP){ /* SAS drive */
                fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_520_520);
            } else {
                fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, FBE_BLOCK_EDGE_GEOMETRY_512_512);
            }
            fbe_base_object_trace((fbe_base_object_t *) base_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid block_size_in_bytes %d set default value\n", __FUNCTION__, block_size_in_bytes);
            /* Peter Puhov. Workaround terminator problem
            fbe_block_transport_server_unlock(&base_physical_drive->block_transport_server);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
            */
    }

    /* Set the exported capacity in the edge.  The exported capacity is in
     * terms of the client block size.
     */
    status = fbe_base_physical_drive_get_exported_capacity(base_physical_drive, FBE_BE_BYTES_PER_BLOCK, &capacity_in_client_blocks);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Failed to get exported capacity - status: 0x%x\n", 
                                                   __FUNCTION__, status);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_block_transport_edge_set_capacity(block_transport_control_attach_edge->block_edge, capacity_in_client_blocks);

    fbe_block_transport_edge_set_traffic_priority(block_transport_control_attach_edge->block_edge, FBE_TRAFFIC_PRIORITY_VERY_LOW);
    fbe_block_transport_server_unlock(&base_physical_drive->block_transport_server);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
base_physical_drive_detach_block_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_block_transport_control_detach_edge_t * block_transport_control_detach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_u32_t number_of_clients;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_detach_edge); 
    if(block_transport_control_detach_edge == NULL){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_block_transport_control_detach_edge_t)){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Right now we do not know all the path state and path attributes rules,
       so we will do something straight forward and may be not exactly correct.
     */
    fbe_block_transport_server_lock(&base_physical_drive->block_transport_server);
    fbe_block_transport_server_detach_edge(&base_physical_drive->block_transport_server, 
                                        block_transport_control_detach_edge->block_edge);
    fbe_block_transport_server_unlock(&base_physical_drive->block_transport_server);

    fbe_block_transport_server_get_number_of_clients(&base_physical_drive->block_transport_server, &number_of_clients);

    /* It is possible that we waiting for discovery server to become empty.
        It whould be nice if we can reschedule monitor when we have no clients any more 
    */
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_LOW /*FBE_TRACE_LEVEL_DEBUG_HIGH*/,
                            "%s number_of_clients = %d\n", __FUNCTION__, number_of_clients);

    if(number_of_clients == 0){
        status = fbe_lifecycle_reschedule(&fbe_base_physical_drive_lifecycle_const,
                                        (fbe_base_object_t *) base_physical_drive,
                                        (fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t 
base_physical_drive_open_block_edge(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_block_transport_control_open_edge_t * block_transport_control_open_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_open_edge); 
    if(block_transport_control_open_edge == NULL){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len != sizeof(fbe_block_transport_control_open_edge_t)){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Right now we do not know all the path state and path attributes rules,
       so we will do something straight forward and may be not exactly correct.
     */

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
base_physical_drive_get_slot_number(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_base_enclosure_mgmt_get_slot_number_t * base_enclosure_mgmt_get_slot_number = NULL;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &base_enclosure_mgmt_get_slot_number); 
    if(base_enclosure_mgmt_get_slot_number == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_enclosure_mgmt_get_slot_number_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &base_enclosure_mgmt_get_slot_number->client_id);

    status = fbe_base_discovered_send_control_packet((fbe_base_discovered_t *) base_physical_drive, packet);

    return status;
}

/*!**************************************************************
 * @fn base_physical_drive_get_dieh_info(fbe_base_physical_drive_t * base_physical_drive,
 *                                       fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function get the DIEH releated information for a given drive.
 *
 * @param base_physical_drive - drive object ptr.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * HISTORY:
 *  12/15/2011 -  Wayne Garrett - Renamed and enhanced function.
 *
 ****************************************************************/
static fbe_status_t
base_physical_drive_get_dieh_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_dieh_stats_t * physical_drive_mgmt_get_dieh_stats = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t status;
    fbe_drive_threshold_and_exceptions_t threshold_rec = {0};

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &physical_drive_mgmt_get_dieh_stats);
    if(physical_drive_mgmt_get_dieh_stats == NULL){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s physical_drive_mgmt_get_dieh_stats is NULL\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_dieh_stats_t)){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid out_buffer_length %X\n", __FUNCTION__ , out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    physical_drive_mgmt_get_dieh_stats->drive_configuration_handle = base_physical_drive->drive_configuration_handle;

    fbe_base_physical_drive_update_error_counts(base_physical_drive, &physical_drive_mgmt_get_dieh_stats->error_counts);

    fbe_copy_memory(&physical_drive_mgmt_get_dieh_stats->error_counts.drive_error, &base_physical_drive->drive_error, sizeof(fbe_drive_error_t));
    fbe_copy_memory(&physical_drive_mgmt_get_dieh_stats->dieh_state, &base_physical_drive->dieh_state, sizeof(fbe_drive_dieh_state_t));
    physical_drive_mgmt_get_dieh_stats->dieh_mode = base_physical_drive->dieh_mode;

    /* get dieh record */
    status = fbe_drive_configuration_get_threshold_info(base_physical_drive->drive_configuration_handle, &threshold_rec);

    if (status != FBE_STATUS_OK ||
        FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE == base_physical_drive->drive_configuration_handle)
    { 
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s get_threshold_info failed. handle:0x%x\n", __FUNCTION__ , base_physical_drive->drive_configuration_handle);

        /* clear dieh record */
        fbe_zero_memory(&physical_drive_mgmt_get_dieh_stats->drive_stat, sizeof(fbe_drive_stat_t));

        /* mark ratios as invalid */
        physical_drive_mgmt_get_dieh_stats->error_counts.io_error_ratio =           FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.recovered_error_ratio =    FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.media_error_ratio =        FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.hardware_error_ratio =     FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.healthCheck_error_ratio =  FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.link_error_ratio =         FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.reset_error_ratio =        FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.power_cycle_error_ratio =  FBE_STAT_INVALID_RATIO;
        physical_drive_mgmt_get_dieh_stats->error_counts.data_error_ratio =         FBE_STAT_INVALID_RATIO;
    }
    else
    {
        fbe_copy_memory(&physical_drive_mgmt_get_dieh_stats->drive_stat, &threshold_rec.threshold_info, sizeof(fbe_drive_stat_t));    

        /* Calculate thresholds */
        fbe_stat_get_ratio(&threshold_rec.threshold_info.io_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.io_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.io_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.recovered_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.recovered_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.recovered_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.media_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.media_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.media_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.hardware_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.hardware_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.hardware_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.healthCheck_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.healthCheck_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.healthCheck_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.link_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.link_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.link_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.reset_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.reset_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.reset_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.power_cycle_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.power_cycle_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.power_cycle_error_ratio);
        
        fbe_stat_get_ratio(&threshold_rec.threshold_info.data_stat, 
                            base_physical_drive->drive_error.io_counter, 
                            base_physical_drive->drive_error.data_error_tag, 
                            &physical_drive_mgmt_get_dieh_stats->error_counts.data_error_ratio);
    }


    /* get logical error actions */
    physical_drive_mgmt_get_dieh_stats->crc_action = base_physical_drive->logical_error_action;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
base_physical_drive_clear_error_counts(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                               "%s entry .\n", __FUNCTION__);

    fbe_base_physical_drive_reset_error_counts(base_physical_drive);

    fbe_stat_drive_error_init(&base_physical_drive->drive_error);

    fbe_base_physical_drive_clear_dieh_state(base_physical_drive);

    if (base_physical_drive->block_transport_server.base_transport_server.client_list != NULL)
    {
        fbe_block_transport_server_clear_path_attr_all_servers(
                        &(base_physical_drive)->block_transport_server, 
                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
base_physical_drive_clear_eol(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                               "%s entry .\n", __FUNCTION__);

    if (base_physical_drive->block_transport_server.base_transport_server.client_list != NULL)
    {
        fbe_block_transport_server_clear_path_attr_all_servers(
                    &(base_physical_drive)->block_transport_server, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);
    }

    base_physical_drive->cached_path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE;


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
base_physical_drive_set_qdepth(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_qdepth_t * set_qdepth = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &set_qdepth); 
    if(set_qdepth == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_qdepth_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }	

    if (set_qdepth->qdepth > 0 && set_qdepth->qdepth <= info_ptr->drive_qdepth) {
        fbe_base_physical_drive_set_outstanding_io_max(base_physical_drive, set_qdepth->qdepth);

        status = FBE_STATUS_OK;
    
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_QDEPTH: 0x%x, total 0x%x\n", 
                                set_qdepth->qdepth, info_ptr->drive_qdepth); 
    }else{
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_QDEPTH, Illegal value:0x%X\n", set_qdepth->qdepth); 

        status = FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}


static fbe_status_t
base_physical_drive_get_drive_qdepth(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_qdepth_t * get_qdepth = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &get_qdepth); 
    if(get_qdepth == NULL){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_qdepth_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    get_qdepth->qdepth = base_physical_drive->drive_info.drive_qdepth;
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}

static fbe_status_t
base_physical_drive_get_qdepth(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_qdepth_t * get_qdepth = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &get_qdepth); 
    if(get_qdepth == NULL){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_qdepth_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_base_physical_drive_get_outstanding_io_max(base_physical_drive, &get_qdepth->qdepth);
	fbe_block_transport_server_get_outstanding_io_count(&base_physical_drive->block_transport_server, &get_qdepth->outstanding_io_count);
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}

static fbe_status_t
base_physical_drive_is_wr_cache_enabled(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_rd_wr_cache_value_t * mgmt_rd_wr_cache_value = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u8_t rd_wr_cache_value = 0;
   
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &mgmt_rd_wr_cache_value); 
    if(mgmt_rd_wr_cache_value == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_rd_wr_cache_value_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    rd_wr_cache_value = base_physical_drive->rd_wr_cache_value;

    if (rd_wr_cache_value & ((fbe_u8_t) 0x04))
    /* FBE_PG08_WR_CACHE_MASK = ((fbe_u8_t) 0x04))  */
    {
        // Write cache is enabled
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "FBE_PHYSICAL_DRIVE_CONTROL_CODE_IS_WR_CACHE_ENABLED is enabled. \n");
    
        mgmt_rd_wr_cache_value->fbe_physical_drive_mgmt_rd_wr_cache_value = FBE_TRUE;
    }
    else
    {
        // Write cache is disabled
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "FBE_PHYSICAL_DRIVE_CONTROL_CODE_IS_WR_CACHE_ENABLED is disable. \n");
    
        mgmt_rd_wr_cache_value->fbe_physical_drive_mgmt_rd_wr_cache_value = FBE_FALSE;
    }
  
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}

static fbe_status_t
base_physical_drive_get_drive_information(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_get_drive_information_t * physical_drive_mgmt_get_drive_information = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &physical_drive_mgmt_get_drive_information);
    if(physical_drive_mgmt_get_drive_information == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_get_drive_information_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_physical_drive_get_dev_info(base_physical_drive, &physical_drive_mgmt_get_drive_information->get_drive_info);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
base_physical_drive_reset_drive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                          FBE_TRACE_LEVEL_INFO,
                          "%s Request accepted.\n", __FUNCTION__);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}

/*!****************************************************************
 * base_physical_drive_fw_download()
 ******************************************************************
 * @brief
 *  This function handles the request to firmware download.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/21/2011  chenl6  - Created.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_fw_download(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_u32_t out_len;
    fbe_u32_t number_of_clients;

    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "BPDO fwdl: command received, state: 0x%x\n",
                                lifecycle_state);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &download_info); 
    if(download_info == NULL){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_fw_download_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                               FBE_TRACE_LEVEL_INFO,
                                               "BPDO fwdl: object_id: 0x%x trial_run: %s fast_download: %s force:%s\n",
                                               download_info->object_id,
                                               (download_info->trial_run ? "TRUE" : "FALSE"),
                                               (download_info->fast_download ? "TRUE" : "FALSE"),
                                               (download_info->force_download ? "TRUE" : "FALSE"));

    /* Only valid states for handling this request is Ready.  
       NOTE: It's possible that some day we may need a reason to upgrade a drive that's stuck in the FAILED state.  If that
       happens, then a special flag should be provided to this control operation to allow it.  This
       specific case should only be allowed in engineering mode.
    */
    if(lifecycle_state != FBE_LIFECYCLE_STATE_READY) 
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                   FBE_TRACE_LEVEL_WARNING,
                                                   "BPDO fwdl: Failure. Not Ready. state=%d\n",
                                                   lifecycle_state);
        /* TODO: change to use control status & qualifier to indicating reason for failure.
           the packet itself is fine so the packet status and return value should be set to
           OK.  Using packet status now since it's a quick fix for failure case. 
        */
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Enqueue usurper packet */
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                               FBE_TRACE_LEVEL_INFO,
                                               "BPDO fwdl: added packet 0x%p to usurper queue\n",
                                               packet);
    fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)base_physical_drive, packet);

    fbe_block_transport_server_get_number_of_clients(&base_physical_drive->block_transport_server, &number_of_clients);


    if (download_info->trial_run) {
        if (number_of_clients == 0) 
        {
            /* If no clients then ack immediately 
            */
            fbe_base_object_remove_from_usurper_queue((fbe_base_object_t *)base_physical_drive, packet);

            download_info->download_error_code = 0;
            download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
            download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;

            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                            "BPDO fwdl_permit no clients: object_id: 0x%x\n", download_info->object_id);

            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        else
        {
            fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&base_physical_drive->block_transport_server,   
                                                                                 FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN,
                                                                                 FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
        }
    }    
    /* Engineering mode option to force a download when it may be rejected by PVD or RAID.  This
     * will immediately download without asking permission.  Also force it if no clients are connected.
     * PDO doesn't have to ask for persmission if no body connects to it.
    */
    else if ((download_info->force_download) || (number_of_clients == 0)){

        fbe_base_physical_drive_customizable_trace(base_physical_drive,FBE_TRACE_LEVEL_INFO,
                                                   "BPDO fwdl: forcing download, num clients:%d\n", number_of_clients);

        /* set download state */
        fbe_base_physical_drive_lock(base_physical_drive);
        fbe_base_physical_drive_set_local_state(base_physical_drive, 
                                                FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD);
        fbe_base_physical_drive_unlock(base_physical_drive);
        
        fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                               (fbe_base_object_t*)base_physical_drive, 
                               FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD);
    }    
    else if (download_info->fast_download) {
        /* set download state */
        fbe_base_physical_drive_lock(base_physical_drive);
        fbe_base_physical_drive_set_local_state(base_physical_drive, 
                                                FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD);
        fbe_base_physical_drive_unlock(base_physical_drive);

        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&base_physical_drive->block_transport_server,
                                                                             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL,
                                                                             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
    }
    else { /* Normal ODFU download */
        /* set download state */
        fbe_base_physical_drive_lock(base_physical_drive);
        fbe_base_physical_drive_set_local_state(base_physical_drive, 
                                                FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD);
        fbe_base_physical_drive_unlock(base_physical_drive);

        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&base_physical_drive->block_transport_server,
                                                                             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ,
                                                                             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
    }

    return FBE_STATUS_PENDING;            
}
/**********************************************************************
 * end base_physical_drive_fw_download()
 **********************************************************************/

/*!****************************************************************
 * base_physical_drive_fw_download_start_stop()
 ******************************************************************
 * @brief
 *  This function handles the request to start/stop download on the
 *  passive (peer) side.  The intent is to hold/release the
 *  PDO in/from the Activate state.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/21/2011  chenl6  - Created.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_fw_download_start_stop(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t control_code;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    control_code = fbe_transport_get_control_code(packet);

    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);

    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "BPDO fwdl_start_stop: command 0x%x received, state: %d\n", control_code, lifecycle_state);


    if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_START)   
    {    
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)base_physical_drive, 
                                        FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD_PEER);
        if (status != FBE_STATUS_OK)  
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                    "BPDO fwdl_start_stop: could not set DOWNLOAD_PEER condition, status=%d\n", status);
    
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(packet);
            return status;
        }
    }
    else if (control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_STOP)
    {
        status = fbe_lifecycle_force_clear_cond(&fbe_base_physical_drive_lifecycle_const, 
                                                (fbe_base_object_t*)base_physical_drive, 
                                                FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD_PEER);
        if (status != FBE_STATUS_OK)  
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                    "BPDO fwdl_start_stop: could not clear DOWNLOAD_PEER condition, status=%d\n", status);
    
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(packet);
            return status;
        }
    }
    else  /* unexpected */
    {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                    "BPDO fwdl_start_stop: unexpected control code 0x%x\n", control_code);
    
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/**********************************************************************
 * end base_physical_drive_fw_download_start_stop()
 **********************************************************************/

/*!****************************************************************
 * base_physical_drive_fw_download_permit()
 ******************************************************************
 * @brief
 *  This function handles the download permission from client.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/21/2011  chenl6  - Created.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_fw_download_permit(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t                            status;
    fbe_lifecycle_state_t                   lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_control_operation_t       * control_operation = NULL;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_base_transport_server_t           * base_transport_server = (fbe_base_transport_server_t *)&base_physical_drive->block_transport_server;
    fbe_block_edge_t                      * block_edge_p = (fbe_block_edge_t *)(base_transport_server->client_list);
    fbe_path_attr_t                         path_attr;
    fbe_base_object_t                     * base_object = (fbe_base_object_t *)base_physical_drive;
    fbe_queue_element_t *                   queue_element = NULL;
    fbe_queue_head_t *                      queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_packet_t *                          usurper_packet;
    fbe_payload_ex_t                      * usurper_payload = NULL;
    fbe_payload_control_operation_t       * usurper_control_operation = NULL;
    fbe_bool_t                            * trial_run = NULL;
    fbe_u32_t                               out_len;
    fbe_u32_t                               i = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);

    fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);

    fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_INFO,
                                               "BPDO fwdl_permit: entry, state:%d path_attr:0x%x\n",
                                               lifecycle_state, path_attr);

    /* Check whether the request is already aborted */
    if ((path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK) == 0)
    {
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* Check whether a GRANT is already issued. */
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT)
    {
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* Check lifecycle state */
    if(lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        if((lifecycle_state == FBE_LIFECYCLE_STATE_FAIL)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_FAIL)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_OFFLINE)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_OFFLINE)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_NOT_EXIST)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_INVALID)||
           (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE))
        {
            /* Drive object is either in inavlid state or drive is going down,
               So we can not continue with the download.
               Retrurn failure status */

            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        else
        {
            /*Drive may be coming up. Return status OK, so PVD will retry the operation*/
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    fbe_payload_control_get_buffer(control_operation, &trial_run);
    if (trial_run != NULL)
    {
        fbe_payload_control_get_buffer_length(control_operation, &out_len);
        if (out_len != sizeof(fbe_bool_t))
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (*trial_run == FBE_TRUE)
        {
            /* Clear the download edge attributes, since PVD has called us back and so that we'll
             * be able to send another event to PVD the next time these edge attributes are set.
             */
            fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&base_physical_drive->block_transport_server, 0,		
                                                                                 FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);

            /* Search the download packet in usurper queue */
            fbe_base_object_usurper_queue_lock(base_object);
            queue_element = fbe_queue_front(queue_head);
            while(queue_element){
                usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
                usurper_payload = fbe_transport_get_payload_ex(usurper_packet);
                usurper_control_operation = fbe_payload_ex_get_control_operation(usurper_payload);
                fbe_payload_control_get_opcode(usurper_control_operation, &control_code);
                if(control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_TRIAL_RUN)
                {
                    queue_element = fbe_queue_next(queue_head, queue_element);

                    /* Remove the usurper_packet from the usurper queue while it is locked to prevent threading issues.
                     * Since the usurper queue is locked, directly call the transport layer to remove the packet.
                     */
                    fbe_transport_remove_packet_from_queue(usurper_packet);
                    fbe_base_object_usurper_queue_unlock(base_object);
    
                    fbe_payload_control_get_buffer(usurper_control_operation, &download_info);
                    download_info->download_error_code = 0;
                    download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
                    download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;

                    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                    "BPDO fwdl_permit: object_id: 0x%x packet 0x%p was accepted by RAID\n",
                                    download_info->object_id, usurper_packet);

                    fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
                    fbe_payload_control_set_status(usurper_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                    fbe_transport_complete_packet(usurper_packet);
    
                    fbe_base_object_usurper_queue_lock(base_object);
                }
                else
                {
                    queue_element = fbe_queue_next(queue_head, queue_element);
                }
                i++;
                if(i > 0x00ffffff){
                    fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                             "BPDO fwdl_permit: Critical error looped usurper queue\n");
                    break;
                }
            }
            fbe_base_object_usurper_queue_unlock(base_object);

            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    /* Now set the condition */
    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD);
    if (status == FBE_STATUS_OK)
    {
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&base_physical_drive->block_transport_server,
                                                                             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT,		
                                                                             FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                "BPDO fwdl_permit: could not set DOWNLOAD condition, status=%d", status);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/**********************************************************************
 * end base_physical_drive_fw_download_permit()
 **********************************************************************/

/*!****************************************************************
 * base_physical_drive_fw_download_reject()
 ******************************************************************
 * @brief
 *  This function handles the download permission from client.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/08/2011  chenl6  - Created.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_fw_download_reject(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t                            status;
    fbe_lifecycle_state_t                   lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_transport_server_t           * base_transport_server = (fbe_base_transport_server_t *)&base_physical_drive->block_transport_server;
    fbe_block_edge_t                      * block_edge_p = (fbe_block_edge_t *)(base_transport_server->client_list);
    fbe_path_attr_t                         path_attr;
    fbe_payload_ex_t                      * payload = NULL;
    fbe_payload_control_operation_t       * control_operation = NULL;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_physical_drive_mgmt_fw_download_t * download_info = NULL;
    fbe_base_object_t                     * base_object = (fbe_base_object_t *)base_physical_drive;
    fbe_queue_element_t *                   queue_element = NULL;
    fbe_queue_head_t *                      queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_packet_t *                          usurper_packet;
    fbe_u32_t                               i = 0;
    fbe_bool_t                              is_found = FBE_FALSE;

    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);

    fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);

    fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_INFO,
                                               "BPDO fwdl_reject: entry, state:%d path_attr:0x%x\n",
                                               lifecycle_state, path_attr);

    /* Clear edge attr now. */
    fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(&base_physical_drive->block_transport_server, 0,		
                                                                         FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);

    /* Search the download packet in usurper queue */
    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        payload = fbe_transport_get_payload_ex(usurper_packet);
        control_operation = fbe_payload_ex_get_control_operation(payload);
        fbe_payload_control_get_opcode(control_operation, &control_code);
        if((control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD) ||
           (control_code == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_TRIAL_RUN))
        {
            queue_element = fbe_queue_next(queue_head, queue_element);

            /* Remove the usurper_packet from the usurper queue while it is locked to prevent threading issues.
             * Since the usurper queue is already locked, directly call the transport layer to remove the packet.
             */
            fbe_transport_remove_packet_from_queue(usurper_packet);
            fbe_base_object_usurper_queue_unlock(base_object);

            fbe_payload_control_get_buffer(control_operation, &download_info);
            download_info->download_error_code = 0;
            download_info->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
            download_info->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;

            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                "BPDO fwdl_reject: object_id: 0x%x packet 0x%p was rejected by RAID\n",
                                download_info->object_id, usurper_packet);

            fbe_transport_set_status(usurper_packet, FBE_STATUS_OK, 0);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(usurper_packet);
                
            fbe_base_object_usurper_queue_lock(base_object);

            is_found = FBE_TRUE;
        }
        else
        {
            /* We already started processing the download request to physical packet. return busy. */
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
        i++;
        if(i > 0x00ffffff){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "BPDO fwdl_reject: Critical error looped usurper queue\n");
            break;
        }
    }
    fbe_base_object_usurper_queue_unlock(base_object);


    if (!is_found) /* Most likely aborted by DCS prior to this rejection */
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                   "BPDO fwdl_reject: Download pkt already removed. \n");
    }

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/**********************************************************************
 * end base_physical_drive_fw_download_reject()
 **********************************************************************/

/*!****************************************************************
 * base_physical_drive_fw_download_abort()
 ******************************************************************
 * @brief
 *  This function handles the request to abort download.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/21/2011  chenl6  - Created.
 *  03/08/2013  Wayne Garrett - Changed to use monitor for aborting.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_fw_download_abort(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t                      status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lifecycle_state_t             lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t                * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "BPDO fwdl_abort: command received, state: 0x%x\n",
                                lifecycle_state);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_DISCOVERED_LIFECYCLE_COND_ABORT_FW_DOWNLOAD);

    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}
/**********************************************************************
 * end base_physical_drive_fw_download_abort()
 **********************************************************************/

static fbe_status_t base_physical_drive_set_default_io_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_default_timeout_t * set_timeout = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
   
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &set_timeout); 
    if(set_timeout == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_default_timeout_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    
    fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DEFAULT_IO_TIMEOUT: 0x%llx\n", (unsigned long long)set_timeout->timeout); 

    
    status = fbe_base_physical_drive_set_default_timeout(base_physical_drive, set_timeout->timeout);
    if (status != FBE_STATUS_OK){
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DEFAULT_IO_TIMEOUT, had an error\n"); 
    
            status = FBE_STATUS_GENERIC_FAILURE;
    }
    
   
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 

}

static fbe_status_t base_physical_drive_get_default_io_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_default_timeout_t * get_timeout = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
   
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &get_timeout); 
    if(get_timeout == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_default_timeout_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    
    fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DEFAULT_IO_TIMEOUT request\n"); 

    
    status = fbe_base_physical_drive_get_default_timeout(base_physical_drive, &get_timeout->timeout);
    if (status != FBE_STATUS_OK){
            fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DEFAULT_IO_TIMEOUT, had an error\n"); 
    
            status = FBE_STATUS_GENERIC_FAILURE;
    }
    
   
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 

}

static fbe_status_t base_physical_drive_enter_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry.\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
   
    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);
    if((status != FBE_STATUS_OK) || (lifecycle_state != FBE_LIFECYCLE_STATE_READY))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s: not in READY state, state: %d\n", __FUNCTION__, lifecycle_state);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
   }

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_SAVE);

    if (status == FBE_STATUS_OK)
    {
        /* Enqueue usurper packet */
        fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)base_physical_drive, packet);
        status = FBE_STATUS_PENDING;
    }
    else
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
    }

    return status;

}

static fbe_status_t base_physical_drive_exit_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry.\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
   
    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);
    if((status != FBE_STATUS_OK) || (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s: not in HIBERNATE state, state: %d\n", __FUNCTION__, lifecycle_state);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
   }

    /* Enqueue usurper packet */
    status = fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)base_physical_drive, packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
    else
    {
        return FBE_STATUS_PENDING;

    }
}

static fbe_status_t base_physical_drive_stat_config_table_cahnged(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CONFIG_CHANGED);
   
    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}

static fbe_status_t base_physical_drive_enter_power_save_passive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry.\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
   
    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                    &lifecycle_state);
    if((status != FBE_STATUS_OK) || (lifecycle_state != FBE_LIFECYCLE_STATE_READY))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s: not in READY state, state: %d\n", __FUNCTION__, lifecycle_state);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
   }

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_SAVE);

    if (status == FBE_STATUS_OK)
    {
        /* Enqueue usurper packet */
        fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)base_physical_drive, packet);
        status = FBE_STATUS_PENDING;
    }
    else
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
    }

    return status;

}

static fbe_status_t base_physical_drive_power_cycle(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u32_t * duration = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry.\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &duration);
    if(duration == NULL){
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "fbe_payload_control_get_buffer fail\n");
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_u32_t)){
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_physical_drive_set_powercycle_duration(base_physical_drive, *duration);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_CYCLE);

    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}

static fbe_status_t
base_physical_drive_get_cached_dev_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_get_drive_information_t * physical_drive_mgmt_get_drive_information = NULL;
    fbe_physical_drive_information_t * drive_information = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &physical_drive_mgmt_get_drive_information);
    if(physical_drive_mgmt_get_drive_information == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_get_drive_information_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    drive_information = & physical_drive_mgmt_get_drive_information->get_drive_info;
    fbe_zero_memory (drive_information, sizeof(fbe_physical_drive_information_t));

    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)base_physical_drive, &lifecycle_state);
    if (lifecycle_state == FBE_LIFECYCLE_STATE_INVALID)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the object is being destroyed return `no such object'
     */
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)         && 
	    (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY)    )
    {
        fbe_base_object_trace((fbe_base_object_t *)base_physical_drive, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: b/e/s:%d/%d/%d lifecycle state: %d \n",
                              __FUNCTION__, base_physical_drive->port_number, base_physical_drive->enclosure_number, 
                              base_physical_drive->slot_number, lifecycle_state);
        if ((lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY)         || 
            (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY)    )
        {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_BUSY);
            fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_NO_OBJECT;
        }
    }

    fbe_copy_memory (drive_information->product_serial_num, info_ptr->serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    fbe_copy_memory (drive_information->dg_part_number_ascii, info_ptr->part_number, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE);
    fbe_copy_memory (drive_information->product_rev, info_ptr->revision, FBE_SCSI_INQUIRY_REVISION_SIZE);
    fbe_copy_memory (drive_information->tla_part_number, info_ptr->tla, FBE_SCSI_INQUIRY_TLA_SIZE);
    drive_information->drive_vendor_id = info_ptr->drive_vendor_id;
    drive_information->spin_down_qualified = 
		(info_ptr->powersave_attr & FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_SUPPORTED)? FBE_TRUE : FBE_FALSE;
    fbe_copy_memory (drive_information->vendor_id, info_ptr->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);

	base_physical_drive_get_up_time(base_physical_drive, &drive_information->spin_up_time_in_minutes);
	base_physical_drive_get_down_time(base_physical_drive, &drive_information->stand_by_time_in_minutes);
	drive_information->spin_up_count = base_physical_drive->drive_info.spinup_count;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t base_physical_drive_start_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
	fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t * 		control_operation = NULL;
	fbe_status_t							status;
	fbe_block_client_hibernating_info_t *	hibernate_info = NULL;
	fbe_payload_control_buffer_length_t 	len = 0;
    fbe_bool_t                              is_spindown_qualified;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_payload_control_get_buffer(control_operation, &hibernate_info);
    if(hibernate_info == NULL){
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);   
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_block_client_hibernating_info_t)){
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, %X len invalid\n", __FUNCTION__, len);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);   
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    is_spindown_qualified = fbe_base_physical_drive_is_spindown_qualified(base_physical_drive, base_physical_drive->drive_info.tla);
	if (hibernate_info->max_latency_time_is_sec < base_physical_drive->drive_info.max_becoming_ready_latency_in_seconds
        || !is_spindown_qualified) {
		 fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s Can't power save, client requested:%d sec., we come up in:%d sec. Or spindown qualified:%d \n",__FUNCTION__,
								(int)hibernate_info->max_latency_time_is_sec, (int)base_physical_drive->drive_info.max_becoming_ready_latency_in_seconds,
                                is_spindown_qualified);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);   
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;

	}

	/*call the active or passive command, base on what was sent*/
	if (hibernate_info->active_side) {
		status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
										(fbe_base_object_t*)base_physical_drive, 
										FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVE_POWER_SAVE);
	}else{
		status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
										(fbe_base_object_t*)base_physical_drive, 
										FBE_BASE_DISCOVERED_LIFECYCLE_COND_PASSIVE_POWER_SAVE);

	}


	fbe_payload_control_set_status(control_operation,(status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK: FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t base_physical_drive_end_power_save(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
	fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t * 		control_operation = NULL;
	fbe_status_t							status = FBE_STATUS_OK;
	fbe_lifecycle_state_t					lifecycle_state;	

	payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

	fbe_base_object_get_lifecycle_state((fbe_base_object_t *)base_physical_drive, &lifecycle_state);

	if (lifecycle_state == FBE_LIFECYCLE_STATE_HIBERNATE || lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_HIBERNATE) {
	
		/*simply jump to activate, this will make sure we will get out of hibernation*/
		status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
										(fbe_base_object_t*)base_physical_drive, 
										FBE_BASE_DISCOVERED_LIFECYCLE_COND_EXIT_POWER_SAVE);
	
		if (status != FBE_STATUS_OK) {
			fbe_base_physical_drive_customizable_trace( base_physical_drive, 
													FBE_TRACE_LEVEL_ERROR,
													"%s Failed to set condition\n",__FUNCTION__);
		}else{
			fbe_base_physical_drive_customizable_trace( base_physical_drive, 
													FBE_TRACE_LEVEL_INFO,
													"%s Drive being asked to get out of power savings\n",__FUNCTION__);
		}
	}else{
		fbe_base_physical_drive_customizable_trace( base_physical_drive, 
													FBE_TRACE_LEVEL_INFO,
													"%s Drive is not hibernating, nothing to do\n",__FUNCTION__);
	}

	fbe_payload_control_set_status(control_operation,(status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK: FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t base_physical_drive_fail_drive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_base_physical_drive_death_reason_t *reason;
    fbe_status_t                            status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &reason);
        
    status = fbe_base_physical_drive_executor_fail_drive(base_physical_drive, *reason);  
                                                                          
    fbe_payload_control_set_status(control_operation,(status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK: FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    
    return FBE_STATUS_OK;
}

static fbe_status_t base_physical_drive_enter_glitch_drive(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_status_t							status;
    fbe_time_t 								*glitch_time_sec;
    fbe_time_t                              current_time_sec = fbe_get_time()/1000;
    fbe_u64_t                               end_time_sec;
	
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &glitch_time_sec);
	
    end_time_sec = current_time_sec + *glitch_time_sec;
    fbe_base_physical_drive_set_glitch_time(base_physical_drive, *glitch_time_sec);
    fbe_base_physical_drive_set_glitch_end_time(base_physical_drive, end_time_sec);	
		
    fbe_base_physical_drive_customizable_trace( base_physical_drive, FBE_TRACE_LEVEL_INFO,
				"%s entry.  current_time_sec=%d glitch len=%d sec\n",__FUNCTION__, (int)current_time_sec, (int)*glitch_time_sec);		

    /*drive logs out */
    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_ENTER_GLITCH);
	
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
            FBE_TRACE_LEVEL_ERROR,
            "%s Failed to log out\n",__FUNCTION__);
    }else{
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
            FBE_TRACE_LEVEL_INFO,
            "%s Logging out\n",__FUNCTION__);
    }
	
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
	
    return FBE_STATUS_OK;
}

/*!**************************************************************
 *  base_physical_drive_send_block_operation()
 ****************************************************************
 * @brief
 *  This function sends block operations for this object.
 *  This is an alternative IO entry point for PDO via the
 *  usurper.
 *
 * @param base_physical_drive_p - The physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  08/16/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_send_block_operation(fbe_base_physical_drive_t * base_physical_drive_p, 
                                     fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;

    /* Set the time stamp of the I/O in the packet when IO first enters PDO, before having a chance to be queued.  
       We will use this to determine service time. 
    */
    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());    

    status = fbe_block_transport_send_block_operation(
     (fbe_block_transport_server_t*)&base_physical_drive_p->block_transport_server,
     packet_p,
     (fbe_base_object_t *)base_physical_drive_p);
    return status;
}
/**************************************
 * end fbe_logical_drive_send_block_operation()
 **************************************/

static fbe_status_t
base_physical_drive_set_drive_death_reason(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_drive_death_reason_t * set_death_reason = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &set_death_reason); 
    if(set_death_reason == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_drive_death_reason_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s death reason: 0x%x\n", __FUNCTION__, set_death_reason->death_reason); 

    status = base_physical_drive_fill_dc_death_reason(base_physical_drive, set_death_reason->death_reason);
    
    fbe_payload_control_set_status (control_operation, (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}

static fbe_status_t base_physical_drive_reset_slot(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_physical_drive_customizable_trace( base_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry.\n", __FUNCTION__);

    /* Send to executer for reset. */
    status = fbe_base_physical_drive_reset_slot(base_physical_drive, packet);

    return FBE_STATUS_PENDING;
}
/*!**************************************************************
 * base_physical_drive_set_service_time()
 ****************************************************************
 * @brief
 *  Set the service time of the base physical drive.
 *
 * @param base_physical_drive - physical drive to set for.
 * @param packet - The packet to set for.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/23/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t base_physical_drive_set_service_time(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_control_set_service_time_t * set_service_time_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &set_service_time_p); 
    if(set_service_time_p == NULL){
        fbe_base_object_trace( (fbe_base_object_t *)base_physical_drive,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_physical_drive_control_set_service_time_t)){
        fbe_base_object_trace( (fbe_base_object_t *)base_physical_drive,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid out_buffer_length %X \n", __FUNCTION__, length);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                "%s service_time %d msec\n", __FUNCTION__, (int)set_service_time_p->service_time_ms); 

    fbe_base_object_lock((fbe_base_object_t *)base_physical_drive);
    base_physical_drive->service_time_limit_ms = set_service_time_p->service_time_ms;
    fbe_base_object_unlock((fbe_base_object_t *)base_physical_drive);
    
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end base_physical_drive_set_service_time()
 ******************************************/

/*!**************************************************************
 * base_physical_drive_get_service_time()
 ****************************************************************
 * @brief
 *  Get the service time of the base physical drive.  There are
 *  several service time command timeout values:
 *      o Current service time - client may change the service time
 *      o Default service time - Default I/O service time in ms
 *      o EMEH Service time - The increased service time when EMEH is enabled
 *      o ReMap Serviec time - The increase service time during remap
 *
 * @param base_physical_drive - physical drive to set for.
 * @param packet - The packet to set for.
 *
 * @return fbe_status_t
 *
 * @author
 *  06/22/2015  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_get_service_time(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_control_get_service_time_t  *get_service_time_p = NULL;
    fbe_payload_control_buffer_length_t             length = 0;
    fbe_payload_ex_t                               *payload = NULL;
    fbe_payload_control_operation_t                *control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &get_service_time_p); 
    if (get_service_time_p == NULL)
    {
        fbe_base_object_trace( (fbe_base_object_t *)base_physical_drive,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if (length != sizeof(fbe_physical_drive_control_get_service_time_t))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_physical_drive,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Invalid out_buffer_length: %d expected: %d\n", 
                              __FUNCTION__, length, (unsigned int)sizeof(fbe_physical_drive_control_get_service_time_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_base_object_lock((fbe_base_object_t *)base_physical_drive);
    get_service_time_p->current_service_time_ms = base_physical_drive->service_time_limit_ms;
    get_service_time_p->default_service_time_ms = fbe_dcs_get_svc_time_limit();
    get_service_time_p->emeh_service_time_ms = fbe_dcs_get_emeh_svc_time_limit();
    get_service_time_p->remap_service_time_ms = fbe_dcs_get_remap_svc_time_limit();
    fbe_base_object_unlock((fbe_base_object_t *)base_physical_drive);
    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                               FBE_TRACE_LEVEL_INFO,
                                               "get service times: current: %llu default: %llu emeh: %llu remap: %llu \n",
                                               get_service_time_p->current_service_time_ms,
                                               get_service_time_p->default_service_time_ms,
                                               get_service_time_p->emeh_service_time_ms,
                                               get_service_time_p->remap_service_time_ms);
    
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end base_physical_drive_get_service_time()
 ******************************************/

/*!****************************************************************
 * fbe_base_physical_drive_handle_logical_error_from_raid_object()
 ******************************************************************
 * @brief
 *  This function handles the action on logical error for this
 *  raid group object.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/18/2010  Pradnya Patankar  - Created.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_handle_logical_error_from_raid_object(fbe_base_physical_drive_t * base_physical_drive_p, 
                                                          fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_block_transport_control_logical_error_t *    logical_error_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &logical_error_p);
    if (logical_error_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_physical_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if(length != sizeof(fbe_block_transport_control_logical_error_t))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_physical_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer length mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

#if 0
    if (logical_error_p->pdo_object_id != 
        base_physical_drive_p->base_discovering.base_discovered.base_object.object_id)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_physical_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s object ID provided does not match. actual=0x%x, expected 0x%x.\n",
                                __FUNCTION__, logical_error_p->pdo_object_id, base_physical_drive_p->base_discovering.base_discovered.base_object.object_id);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
#endif


    switch(logical_error_p->error_type)
    {
        case FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_MULTI_BITS:
            status = handle_logical_error_unexpected_crc_multi_bits(base_physical_drive_p);
            break;
        case FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_SINGLE_BIT:
            status = handle_logical_error_unexpected_crc_single_bit(base_physical_drive_p);
            break;
        case FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC:
            status = handle_logical_error_unexpected_crc(base_physical_drive_p);
            break;
        case FBE_BLOCK_TRANSPORT_ERROR_TYPE_TIMEOUT:
            status = handle_logical_error_timeout(base_physical_drive_p);
            break;

        default:
            fbe_base_physical_drive_customizable_trace(base_physical_drive_p, FBE_TRACE_LEVEL_ERROR,
                     "%s: unknown error_type=%d\n", __FUNCTION__, logical_error_p->error_type);
            status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation_p,
                                   (status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK: FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**********************************************************************
 * end base_physical_drive_handle_logical_error_from_raid_object()
 **********************************************************************/


/*!****************************************************************
 * handle_logical_error_unexpected_crc_multi_bits()
 ******************************************************************
 * @brief
 *  Error handling function for the multi-bit crc sent from RAID.   
 *
 * @param base_physical_drive - The Base physical drive object.
 *
 * @return fbe_status_t - status of request
 *
 * @author
 *  05/07/2012  Wayne Garrett - Created.
 *
 ******************************************************************/
static fbe_status_t
handle_logical_error_unexpected_crc_multi_bits(fbe_base_physical_drive_t *base_physical_drive_p)
{
    fbe_base_physical_drive_customizable_trace(base_physical_drive_p, FBE_TRACE_LEVEL_WARNING,
                                               "%s: action bitmap=0x%x\n", __FUNCTION__, base_physical_drive_p->logical_error_action);

    if (base_physical_drive_p->logical_error_action & FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC)
    {
        fbe_base_physical_drive_executor_fail_drive(base_physical_drive_p, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_MULTI_BIT); 
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************
 * handle_logical_error_unexpected_crc_single_bit()
 ******************************************************************
 * @brief
 *  Error handling function for the single-bit crc sent from RAID.  
 *
 * @param base_physical_drive - The Base physical drive object.
 *
 * @return fbe_status_t - status of request
 *
 * @author
 *  05/07/2012  Wayne Garrett - Created.
 *
 ******************************************************************/
static fbe_status_t
handle_logical_error_unexpected_crc_single_bit(fbe_base_physical_drive_t *base_physical_drive_p)
{
    fbe_base_physical_drive_customizable_trace(base_physical_drive_p, FBE_TRACE_LEVEL_WARNING,
                                               "%s: action bitmap=0x%x\n", __FUNCTION__, base_physical_drive_p->logical_error_action);

    if (base_physical_drive_p->logical_error_action & FBE_PDO_ACTION_KILL_ON_SINGLE_BIT_CRC)
    {
        fbe_base_physical_drive_executor_fail_drive(base_physical_drive_p,FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_SINGLE_BIT);
    }
    return FBE_STATUS_OK;
}

/*!****************************************************************
 * handle_logical_error_unexpected_crc()
 ******************************************************************
 * @brief
 *  Error handling function for the crc error sent from RAID.  
 *
 * @param base_physical_drive - The Base physical drive object.
 *
 * @return fbe_status_t - status of request
 *
 * @author
 *  05/07/2012  Wayne Garrett - Created.
 *
 ******************************************************************/
static fbe_status_t
handle_logical_error_unexpected_crc(fbe_base_physical_drive_t *base_physical_drive_p)
{
    fbe_base_physical_drive_customizable_trace(base_physical_drive_p,  FBE_TRACE_LEVEL_WARNING,
                                               "%s: action bitmap=0x%x\n", __FUNCTION__, base_physical_drive_p->logical_error_action);

    if (base_physical_drive_p->logical_error_action & FBE_PDO_ACTION_KILL_ON_OTHER_CRC)
    {
        fbe_base_physical_drive_executor_fail_drive(base_physical_drive_p, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_OTHER);
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************
 * handle_logical_error_timeout()
 ******************************************************************
 * @brief
 *  Error handling function for the IO servicablity timeout sent 
 *  from RAID.  
 *
 * @param base_physical_drive - The Base physical drive object.
 *
 * @return fbe_status_t - status of request
 *
 * @author
 *  05/07/2012  Wayne Garrett - Created.
 *
 ******************************************************************/
static fbe_status_t 
handle_logical_error_timeout(fbe_base_physical_drive_t *base_physical_drive_p)
{
    fbe_status_t status;

    /* We first transition the drive to activate.
     * The upper level RAID always assumes the drive will get transitioned to activate. 
     */
    fbe_base_physical_drive_customizable_trace(base_physical_drive_p, FBE_TRACE_LEVEL_WARNING,
                                               "%s: move to activate for timeout errors\n", __FUNCTION__);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive_p, 
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive_p, FBE_TRACE_LEVEL_ERROR,
                                                   "%s can't set BO:ACTIVATE condition, status: 0x%x\n",
                                                   __FUNCTION__, status);
    }
    /*! @todo  Support needs to be added to initiate a health check on the drive.
     */

    return FBE_STATUS_OK;
}


static fbe_status_t base_physical_drive_get_location(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_get_location_t * get_location = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_location);
    if(get_location == NULL){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s get_location buffer is NULL\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_get_location_t)){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid buffer_length 0x%X\n", __FUNCTION__ , out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_physical_drive_get_port_number(base_physical_drive, &get_location->port_number);
    fbe_base_physical_drive_get_enclosure_number(base_physical_drive, &get_location->enclosure_number);
    fbe_base_physical_drive_get_slot_number(base_physical_drive, &get_location->slot_number);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


/*!****************************************************************
 * base_physical_drive_get_download_info()
 ******************************************************************
 * @brief
 *  This function handles the request to get drive info for download.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/16/2011  chenl6  - Created.
 *
 ****************************************************************/
static fbe_status_t
base_physical_drive_get_download_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_get_download_info_t * get_download_info = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_download_info);
    if (get_download_info == NULL){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_get_download_info_t)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(get_download_info, sizeof(fbe_physical_drive_get_download_info_t));
    fbe_copy_memory (get_download_info->vendor_id, info_ptr->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE); 
    fbe_copy_memory (get_download_info->product_id, info_ptr->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);
    fbe_copy_memory (get_download_info->product_rev, info_ptr->revision, info_ptr->product_rev_len);
    fbe_copy_memory (get_download_info->tla_part_number, info_ptr->tla, FBE_SCSI_INQUIRY_TLA_SIZE);
    get_download_info->port = base_physical_drive->port_number;
    get_download_info->enclosure = base_physical_drive->enclosure_number;
    get_download_info->slot = base_physical_drive->slot_number;
    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                    (fbe_base_object_t*)base_physical_drive,
                                     &get_download_info->state);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/**********************************************************************
 * end base_physical_drive_get_download_info()
 **********************************************************************/

/*!****************************************************************
 * fbe_base_physical_drive_handle_clear_logical_errors()
 ******************************************************************
 * @brief
 *  Clear out any logical errors on the upstream edge.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  1/24/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_handle_clear_logical_errors(fbe_base_physical_drive_t * base_physical_drive_p, 
                                                fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_payload_control_operation_t * 		control_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_block_transport_server_clear_path_attr_all_servers(&base_physical_drive_p->block_transport_server,
                                                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
    if (status == FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t *)base_physical_drive_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "clear FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS\n");
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_physical_drive_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "error %d clearing FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS\n", status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**********************************************************************
 * end base_physical_drive_handle_clear_logical_errors()
 **********************************************************************/


/*!****************************************************************
 * @fn base_physical_drive_set_logical_error_crc_action()
 ******************************************************************
 * @brief
 *  Set action for logical CRC errors.   Logical errors are sent
 *  from RAID.
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/07/2012  Wayne Garrett - created.
 *
 ****************************************************************/
static fbe_status_t 
base_physical_drive_set_logical_error_crc_action(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_control_crc_action_t * crc_action_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &crc_action_p); 
    if(crc_action_p == NULL){
        fbe_base_object_trace( (fbe_base_object_t *)base_physical_drive,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_physical_drive_control_crc_action_t)){
        fbe_base_object_trace( (fbe_base_object_t *)base_physical_drive,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid out_buffer_length %d, expected %d\n", __FUNCTION__, length, (int)sizeof(fbe_physical_drive_control_crc_action_t));
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                "%s crc_action_p 0x%x\n", __FUNCTION__, crc_action_p->action); 

    base_physical_drive->logical_error_action = crc_action_p->action;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!****************************************************************
 * @fn base_physical_drive_handle_logical_drive_state_change()
 ******************************************************************
 * @brief
 *  Handle notification from clients that the logical (SEP) state of
 *  a drive has changed.  This will indicate if drive is available
 *  from a user perspective.   This is not to be confused with the
 *  LDO state. 
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/18/2012  Wayne Garrett - created.
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_handle_logical_drive_state_change(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_control_buffer_length_t length = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_block_transport_logical_state_t *logical_state_p;
    fbe_u32_t expected_size;
    fbe_notification_data_changed_info_t dataChangeInfo = {0};

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &logical_state_p); 
    if(logical_state_p == NULL){
        fbe_base_object_trace( (fbe_base_object_t *)base_physical_drive,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    expected_size = sizeof(fbe_block_transport_logical_state_t);
    if(length != expected_size){
        fbe_base_object_trace( (fbe_base_object_t *)base_physical_drive,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid out_buffer_length %d, expected %d\n", __FUNCTION__, length, expected_size);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_physical_drive->logical_state = *logical_state_p;

    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_LOW,
                                "%s logical_state:%d\n", __FUNCTION__, base_physical_drive->logical_state);    
    
    dataChangeInfo.phys_location.bus = base_physical_drive->port_number;
    dataChangeInfo.phys_location.enclosure = base_physical_drive->enclosure_number;
    dataChangeInfo.phys_location.slot = (fbe_u8_t)base_physical_drive->slot_number;    
    dataChangeInfo.data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;
    dataChangeInfo.device_mask = FBE_DEVICE_TYPE_DRIVE;
    
    fbe_base_physical_drive_send_data_change_notification(base_physical_drive, &dataChangeInfo); 

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!****************************************************************
 * @fn fbe_base_physical_drive_update_drive_fault()
 ******************************************************************
 * @brief Set/Clear Path Attr for a drive_fault
 *  
 *
 * @param base_physical_drive_p - The Base physical drive object.
 * @param flag - Flag to set or clear drive_fault
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/23/2012  kothal - created.
 *
 ****************************************************************/
fbe_status_t 
fbe_base_physical_drive_update_drive_fault(fbe_base_physical_drive_t * base_physical_drive_p, fbe_bool_t flag)
{
    fbe_status_t status;
    
    /* If flag is True set the `Drive Fault' path attribute then transition to
     * fail.
     */
    if (flag)
    {
        status = fbe_block_transport_server_set_path_attr_all_servers(&(base_physical_drive_p)->block_transport_server,
                                                          FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive_p, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s Failed to set path attr: %d status: 0x%x\n",
                                                       __FUNCTION__, FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT, status);
            return status;
        }
        status = fbe_base_physical_drive_executor_fail_drive(base_physical_drive_p, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive_p, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s Failed to set death reason: %d status: 0x%x\n",
                                                       __FUNCTION__, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR, status);
            return status;
        }

    }    
    else
    {
        /* Else we were requested to clear the `Drive Fault'.  Sequence:
         *  1. First set `clear drive fault pending' so that we know why the pdo
         *     is broken.
         *  2. Then clear the drive fault path attribute.
         *  3. Clear the death reason
         *  4. Transition to activate
         */
        status = fbe_block_transport_server_set_path_attr_all_servers(&(base_physical_drive_p)->block_transport_server,
                                                          FBE_BLOCK_PATH_ATTR_FLAGS_CLEAR_DRIVE_FAULT_PENDING);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive_p, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s Failed to set path attr: %d status: 0x%x\n",
                                                       __FUNCTION__, FBE_BLOCK_PATH_ATTR_FLAGS_CLEAR_DRIVE_FAULT_PENDING, status);
            return status;
        }
        status = fbe_block_transport_server_clear_path_attr_all_servers(&(base_physical_drive_p)->block_transport_server,
                                                            FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive_p, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s Failed to clear path attr: %d status: 0x%x\n",
                                                       __FUNCTION__, FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT, status);
            return status;
        }
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_physical_drive_p, FBE_DEATH_REASON_INVALID);
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)base_physical_drive_p, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive_p, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s Failed to transition to activate. status: 0x%x\n",
                                                       __FUNCTION__, status);
            return status;
        }
    }
    
    return FBE_STATUS_OK;
}

/*!****************************************************************
 * @fn base_physical_drive_set_drive_fault()
 ******************************************************************
 * @brief Set action for drive_fault. 
 *  
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/23/2012  kothal - created.
 *
 ****************************************************************/
static fbe_status_t
base_physical_drive_set_drive_fault(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t * flag = NULL;    /* INPUT */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  
    fbe_payload_control_buffer_length_t len = 0;
    
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
            FBE_TRACE_LEVEL_INFO,
            "%s entry .\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  
    
    fbe_payload_control_get_buffer(control_operation, &flag);
    if (flag == NULL)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                FBE_TRACE_LEVEL_INFO,
                "fbe_payload_control_get_buffer failed.\n");        
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_bool_t))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,                
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Invalid buffer length: %d\n", 
                                                   __FUNCTION__, len);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_base_physical_drive_update_drive_fault(base_physical_drive, *flag);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,                
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Failed to update drive fault to: %d status: 0x%x\n", 
                                                   __FUNCTION__, *flag, status);
    
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
    
}

/*!****************************************************************
 * @fn fbe_base_physical_drive_update_link_fault()
 ******************************************************************
 * @brief Set/Clear Path Attr for a link_fault
 *  
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param flag - Flag to set or clear link_fault
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/23/2012  kothal - created.
 *
 ****************************************************************/
fbe_status_t 
fbe_base_physical_drive_update_link_fault(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t flag)
{
    fbe_status_t status;
    
    if(flag)
    {
        status = fbe_block_transport_server_set_path_attr_all_servers(&(base_physical_drive)->block_transport_server,
                 FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT);
        return status;
    }    
    else
    {
        fbe_block_transport_server_clear_path_attr_all_servers(
            &(base_physical_drive)->block_transport_server, 
            FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT);
    }
    
    return FBE_STATUS_OK;
}

/*!****************************************************************
 * @fn base_physical_drive_update_link_fault()
 ******************************************************************
 * @brief Set action for link_fault. 
 *  
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/23/2012  kothal - created.
 *
 ****************************************************************/
static fbe_status_t
base_physical_drive_update_link_fault(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    
    fbe_bool_t * flag = NULL;    /* INPUT */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;  
    fbe_payload_control_buffer_length_t len = 0;
    
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
            FBE_TRACE_LEVEL_INFO,
            "%s entry .\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  
    
    fbe_payload_control_get_buffer(control_operation, &flag);
    if (flag == NULL)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                FBE_TRACE_LEVEL_INFO,
                "fbe_payload_control_get_buffer failed.\n");        
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_bool_t))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,                
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Invalid buffer length: %d \n",
                                                   __FUNCTION__, len);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_base_physical_drive_update_link_fault(base_physical_drive, *flag);
    
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
    
}

static fbe_status_t
base_physical_drive_clear_end_of_life(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                               FBE_TRACE_LEVEL_INFO,
                                               "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_block_transport_server_clear_path_attr_all_servers(
                &(base_physical_drive)->block_transport_server, 
                FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
base_physical_drive_set_end_of_life(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                               FBE_TRACE_LEVEL_INFO,
                                               "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_block_transport_server_set_path_attr_all_servers(&(base_physical_drive->block_transport_server),
    										 FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*!****************************************************************
 * @fn base_physical_drive_get_power_saving_stats()
 ******************************************************************
 * @brief
 *  Get power saving statistics info. 
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/09/2012  Vera Wang - created.
 *
 ****************************************************************/
static fbe_status_t
base_physical_drive_get_power_saving_stats(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_power_saving_stats_t * power_saving_stats = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &power_saving_stats);
    if(power_saving_stats == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_power_saving_stats_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory (power_saving_stats, sizeof(fbe_physical_drive_power_saving_stats_t));

	base_physical_drive_get_up_time(base_physical_drive, &power_saving_stats->spin_up_time_in_minutes);
	base_physical_drive_get_down_time(base_physical_drive, &power_saving_stats->stand_by_time_in_minutes);
	power_saving_stats->spin_up_count = base_physical_drive->drive_info.spinup_count;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!****************************************************************
 * @fn base_physical_drive_set_power_saving_stats()
 ******************************************************************
 * @brief
 *  Set power saving statistics info. 
 *
 * @param base_physical_drive - The Base physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  7/09/2012  Vera Wang - created.
 *
 ****************************************************************/
static fbe_status_t
base_physical_drive_set_power_saving_stats(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_power_saving_stats_t * power_saving_stats = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &power_saving_stats);
    if(power_saving_stats == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_power_saving_stats_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	base_physical_drive_set_up_time(base_physical_drive, &power_saving_stats->spin_up_time_in_minutes);
	base_physical_drive_set_down_time(base_physical_drive, &power_saving_stats->stand_by_time_in_minutes); 
	base_physical_drive->drive_info.spinup_count = power_saving_stats->spin_up_count;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn base_physical_drive_get_logical_drive_state(fbe_base_physical_drive_t * base_physical_drive,
 *                                       fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function gets the logical state of a given drive.
 *
 * @param base_physical_drive - drive object ptr.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author  08/02/2012 kothal - created.
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_get_logical_drive_state(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{    
    fbe_block_transport_logical_state_t  *logical_state = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    
    fbe_base_physical_drive_customizable_trace( base_physical_drive,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    
    fbe_payload_control_get_buffer(control_operation, &logical_state); 
    if(logical_state == NULL){         
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    
    if(out_len != sizeof(fbe_block_transport_logical_state_t)){       
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }     
    
    status = fbe_base_physical_drive_get_logical_drive_state(base_physical_drive, logical_state);
    if (status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace( base_physical_drive, 
                FBE_TRACE_LEVEL_ERROR,
                "%s status = %d", __FUNCTION__, status); 
        
        status = FBE_STATUS_GENERIC_FAILURE;
    }        
    
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 

}


static fbe_status_t
base_physical_drive_set_io_throttle(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_physical_drive_mgmt_qdepth_t * set_qdepth = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &set_qdepth); 
    if(set_qdepth == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_mgmt_qdepth_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_base_physical_drive_set_io_throttle_max(base_physical_drive, set_qdepth->io_throttle_max);
   
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}
static fbe_status_t
base_physical_drive_get_throttle_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_block_transport_get_throttle_info_t *get_throttle_info = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &get_throttle_info); 
    if(get_throttle_info == NULL){
        KvTrace("%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_block_transport_get_throttle_info_t)){
        KvTrace("%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_server_get_throttle_info(&base_physical_drive->block_transport_server,
                                                 get_throttle_info);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}


static fbe_status_t
base_physical_drive_get_attributes(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t    status;
    fbe_u32_t       index;
    fbe_physical_drive_attributes_t *get_info = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u32_t       min_len = 0;
   
    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
  
    fbe_payload_control_get_buffer(control_operation, &get_info); 
    if(get_info == NULL){
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_ERROR,
                            "%s fbe_payload_control_get_buffer fail .\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_physical_drive_attributes_t)){
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_ERROR,
                            "%s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_info->physical_block_size = base_physical_drive->block_size; 
    

    /* Copy the identify infos into the input buffer.
     */
    min_len = FBE_MIN(FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH, sizeof(base_physical_drive->drive_info.serial_number));
    fbe_zero_memory(get_info->initial_identify.identify_info, FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH);
    fbe_zero_memory(get_info->last_identify.identify_info, FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH);

    for (index = 0; index < min_len; index++)
    {
        get_info->initial_identify.identify_info[index] = base_physical_drive->drive_info.serial_number[index];
        get_info->last_identify.identify_info[index] = base_physical_drive->drive_info.serial_number[index];
    }

    get_info->maintenance_mode_bitmap = base_physical_drive->maintenance_mode_bitmap;


    /* Get the capacity in terms of client block size.
       If we are in maintenance mode, then the block size might not be set, so skip checking status on 
       get_exported_capacity.  It's the callers reponsibility to not use the drive attributes if
       PDO is in maintenance mode. 
     */   
    status = fbe_base_physical_drive_get_exported_capacity(base_physical_drive, FBE_BE_BYTES_PER_BLOCK,
                                                           &get_info->imported_capacity);
    if (status != FBE_STATUS_OK  && get_info->maintenance_mode_bitmap == 0) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                                   FBE_TRACE_LEVEL_WARNING,
                                                   "%s get exported capacity failed - status: 0x%x\n", 
                                                   __FUNCTION__, status);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy over the server information.
     */
    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &get_info->server_info.server_object_id);
    fbe_block_transport_server_get_number_of_clients(&base_physical_drive->block_transport_server,
                                                     &get_info->server_info.number_of_clients);

    /* ! @TODO
    get_info->server_info.b_optional_queued = (!fbe_queue_is_empty(&base_physical_drive->block_transport_server.priority_optional_queue_head));
    get_info->server_info.b_low_queued = (!fbe_queue_is_empty(&base_physical_drive->block_transport_server.priority_low_queue_head));
    get_info->server_info.b_normal_queued = (!fbe_queue_is_empty(&base_physical_drive->block_transport_server.priority_normal_queue_head));
    get_info->server_info.b_urgent_queued = (!fbe_queue_is_empty(&base_physical_drive->block_transport_server.priority_urgent_queue_head));
    */
    get_info->server_info.attributes = (fbe_u32_t)base_physical_drive->block_transport_server.attributes;
    get_info->server_info.outstanding_io_count = (fbe_u32_t)(base_physical_drive->block_transport_server.outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK);
    get_info->server_info.outstanding_io_max = base_physical_drive->block_transport_server.outstanding_io_max;
    get_info->server_info.tag_bitfield = base_physical_drive->block_transport_server.tag_bitfield;    

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * base_physical_drive_generate_ica_stamp()
 ****************************************************************
 * @brief
 *  This function generates the ICA stamp on the drive
 *
 * @param physical_drive - The physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_generate_ica_stamp(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * 					payload = NULL;
    fbe_payload_control_operation_t * 	control_operation = NULL;  

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_base_physical_drive_customizable_trace(physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry .\n", __FUNCTION__);

    /*are we allowed to do that*/
	if ((physical_drive->port_number != 0) ||
		(physical_drive->enclosure_number != 0) ||
		(physical_drive->slot_number >= 3)) {

        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, illegal drive position for ICA:p=%d, e=%d, s=%d\n", __FUNCTION__,
								physical_drive->port_number,
								physical_drive->enclosure_number,
								physical_drive->slot_number);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}
    
#if 0
	/*as a hack we'll use the buffer of this control operation down the road so we want to protect it here*/
	fbe_payload_control_get_buffer(control_operation, &temp_buffer);
	if (temp_buffer != 0) {
		fbe_base_object_trace((fbe_base_object_t *) physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, Please fix this usurper if you are going to use the buffer\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}
#endif
    
	return base_physical_drive_generate_and_send_ica_stamp(physical_drive, packet);
}

/**************************************
 * end fbe_logical_drive_generate_ica_stamp()
 **************************************/

static fbe_status_t base_physical_drive_generate_and_send_ica_stamp(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet)
{
    fbe_status_t						status;
	fbe_u8_t *							write_buffer = NULL;
    fbe_sg_element_t *					sg_list_p = NULL;
	fbe_payload_control_operation_t * 	control_operation = NULL;  
	fbe_payload_ex_t * 					payload = NULL;
	fbe_payload_block_operation_t	*	block_operation = NULL;
	fbe_private_space_layout_region_t	psl_region_info;
	fbe_packet_t *						new_packet_p = NULL;
    fbe_object_id_t                     objectId;
    fbe_u32_t                           block_count;

    fbe_base_object_get_object_id((fbe_base_object_t *)physical_drive, &objectId);
    
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  
    
    write_buffer = fbe_transport_allocate_buffer();
	if (write_buffer ==NULL) {
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, can't get memory\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*this is a bit of a hack but we need a place holder for that, and the buffer of this control operation is not in use
	There could be other implementation for that*/
	control_operation->buffer = write_buffer;
   
    /*get ica block count*/
    block_count = base_physical_drive_get_ica_block_count(physical_drive);

	/*let's carve the memory and sg lists and fill it up*/
    status = base_physical_drive_carve_memory_for_buffer_and_sgl(&sg_list_p, write_buffer, block_count * FBE_BE_BYTES_PER_BLOCK);
	if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, can't format memory for ICA write\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}


	new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL){
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s_allocate_packet failed\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_transport_initialize_packet(new_packet_p);
    payload = fbe_transport_get_payload_ex(new_packet_p);
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    
    
    fbe_transport_add_subpacket(packet, new_packet_p);

    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)physical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)physical_drive, packet);
    
	/*we are good to go, let's send the packet down*/
    fbe_payload_ex_set_sg_list(payload, sg_list_p, 2);

    /*get some information about the ICA region*/
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, &psl_region_info);


	fbe_payload_block_build_operation(block_operation,
									  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
									  psl_region_info.starting_block_address,
									  block_count,/*we care about the first block only for now*/
									  FBE_BE_BYTES_PER_BLOCK,
									  1,
									  NULL);

	
	fbe_transport_set_completion_function(new_packet_p,
										  base_physical_drive_generate_and_send_ica_stamp_completion,
										  physical_drive);
    /*set address correctly*/
    fbe_transport_set_address(new_packet_p,
            FBE_PACKAGE_ID_PHYSICAL,
            FBE_SERVICE_ID_TOPOLOGY,
            FBE_CLASS_ID_INVALID,
            objectId);

    status = fbe_payload_ex_increment_block_operation_level(payload);

    /* start io.*/
    status = fbe_topology_send_io_packet(new_packet_p);
    
	return status;
}

/*this function fills the buffers for the ICA stamp 
IT IS NOT COMPLETED YET AND WILL SERVE SIMULATION ONLY RIGHT NOW !!!!!*/
static fbe_status_t base_physical_drive_carve_memory_for_buffer_and_sgl(fbe_sg_element_t **sg_list_p,
                                                                        fbe_u8_t *write_buffer,
                                                                        fbe_u32_t write_size)
{
	
    fbe_u8_t *					chunk_start_address = write_buffer;
	fbe_imaging_flags_t *		imaging_flags;
	fbe_u32_t					image_count = 0;

    /*let's start with the data buffer*/
    fbe_zero_memory(write_buffer, write_size);

	/*skip to the sgl area*/
	chunk_start_address += write_size;

	/*populate it, no need to terminate explicitly since we zero out the memory*/
	*sg_list_p = (fbe_sg_element_t *)chunk_start_address;
	fbe_zero_memory(*sg_list_p, sizeof(fbe_sg_element_t) * 2);
	(*sg_list_p)->address = write_buffer;
	(*sg_list_p)->count = write_size;

    /*let's fill the memory with the pattern we like*/
	imaging_flags = (fbe_imaging_flags_t *)(write_buffer);
	fbe_zero_memory(imaging_flags, sizeof(fbe_imaging_flags_t));

    fbe_copy_memory(imaging_flags->magic_string, FBE_IMAGING_FLAGS_MAGIC_STRING, FBE_IMAGING_FLAGS_MAGIC_STRING_LENGTH);
	imaging_flags->invalidate_configuration = FBE_IMAGING_FLAGS_TRUE;
	
	for (image_count = 0; image_count < FBE_IMAGING_IMAGE_TYPE_MAX; image_count++) {
		imaging_flags->images_installed[image_count] = FBE_IMAGING_FLAGS_FALSE;
	}

    return FBE_STATUS_OK;

}

static fbe_status_t 
base_physical_drive_generate_and_send_ica_stamp_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_base_physical_drive_t *             physical_drive = (fbe_base_physical_drive_t * )context;
	fbe_payload_control_operation_t * 		control_operation = NULL;  
	fbe_payload_ex_t * 						payload = NULL;
	fbe_payload_block_operation_t	*		block_operation = NULL;
	fbe_payload_block_operation_status_t	block_status;
	fbe_packet_t * 							master_packet_p = NULL;
    
    master_packet_p = fbe_transport_get_master_packet(packet);
	fbe_transport_remove_subpacket(packet);

	/*first let's get the block status of the IO*/
	payload = fbe_transport_get_payload_ex(packet);
	block_operation = fbe_payload_ex_get_block_operation(payload);  
	fbe_payload_block_get_status(block_operation, &block_status);
	fbe_payload_ex_release_block_operation(payload, block_operation);
    
	/*now the control operation of the master packets*/
	payload = fbe_transport_get_payload_ex(master_packet_p);
	control_operation = fbe_payload_ex_get_control_operation(payload);

	/*this is a bit of a hack but we need a place holder for that and the buffer of this control operation is not in use
	There could be other implementation for that*/
	fbe_transport_release_buffer(control_operation->buffer);
    
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)physical_drive, master_packet_p);

	fbe_transport_release_packet(packet);/*don't need it anymore*/

	if (FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS != block_status) {
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s,failed to write ICA stamp, blk status 0x%x\n", __FUNCTION__, block_status);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
		return FBE_STATUS_OK;

	}

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet_p);
    
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_logical_drive_read_ica_stamp()
 ****************************************************************
 * @brief
 *  This function read the ICA stamp from the drive
 *  !!!! the memory for read buffer has to be contiguos !!!
 *
 * @param logical_drive_p - The logical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_read_ica_stamp(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet)
{
	fbe_payload_ex_t * 					payload = NULL;
    fbe_payload_control_operation_t * 	control_operation = NULL;  
	fbe_physical_drive_read_ica_stamp_t *get_ica_stamp;

	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_base_physical_drive_customizable_trace(physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

	/*are we allowed to do that*/
	if ((physical_drive->port_number != 0) ||
		(physical_drive->enclosure_number != 0) ||
		(physical_drive->slot_number >= 3)) {

        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, illegal drive position for ICA:p=%d, e=%d, s=%d\n", __FUNCTION__,
								physical_drive->port_number,
								physical_drive->enclosure_number,
								physical_drive->slot_number);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_payload_control_get_buffer(control_operation, &get_ica_stamp);
	if (get_ica_stamp == NULL) {
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, can't find bufer to read to\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return base_physical_drive_send_read_ica_stamp(physical_drive, packet);

}

static fbe_status_t base_physical_drive_send_read_ica_stamp(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet)
{
	fbe_status_t						status;
	fbe_u8_t *							read_buffer = NULL;
	fbe_payload_control_operation_t * 	control_operation = NULL;  
	fbe_payload_ex_t * 					payload = NULL;
	fbe_payload_block_operation_t	*	block_operation = NULL;
	fbe_sg_element_t *					sg_element;
	fbe_packet_t *						new_packet_p = NULL;
	fbe_private_space_layout_region_t	psl_region_info;
    fbe_object_id_t                     objectId;
    
    fbe_base_object_get_object_id((fbe_base_object_t *)physical_drive, &objectId);
    
	payload = fbe_transport_get_payload_ex(packet);
	control_operation = fbe_payload_ex_get_control_operation(payload);  

	fbe_payload_control_get_buffer(control_operation, &read_buffer);
	if (read_buffer == NULL) {
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, can't find bufer to read to\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL){
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s_allocate_packet failed\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_transport_initialize_packet(new_packet_p);
    payload = fbe_transport_get_payload_ex(new_packet_p);
    block_operation = fbe_payload_ex_allocate_block_operation(payload);
    
    fbe_transport_add_subpacket(packet, new_packet_p);

    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)physical_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)physical_drive, packet);

	/*sg stuff*/
	fbe_zero_memory(read_buffer, sizeof(fbe_physical_drive_read_ica_stamp_t));/*will also make the terminating sgl null*/
	sg_element = (fbe_sg_element_t *)(fbe_u8_t *)((fbe_u8_t*)read_buffer + FBE_BE_BYTES_PER_BLOCK);
	sg_element->address = read_buffer;
	sg_element->count = FBE_BE_BYTES_PER_BLOCK;
    fbe_payload_ex_set_sg_list(payload, sg_element, 2);

    /*get some information about the ICA region*/
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS, &psl_region_info);

	fbe_payload_block_build_operation(block_operation,
									  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
									  psl_region_info.starting_block_address,
									  1,/*we care about the first block only for now*/
									  FBE_BE_BYTES_PER_BLOCK,
									  1,
									  NULL);

	
	fbe_transport_set_completion_function(new_packet_p,
										  base_physical_drive_send_read_ica_stamp_completion,
										  physical_drive);

    /*set address correctly*/
    fbe_transport_set_address(new_packet_p,
            FBE_PACKAGE_ID_PHYSICAL,
            FBE_SERVICE_ID_TOPOLOGY,
            FBE_CLASS_ID_INVALID,
            objectId);

    status = fbe_payload_ex_increment_block_operation_level(payload);
    
    /* start io.*/
    status = fbe_topology_send_io_packet(new_packet_p);
    
	return status;

}

static fbe_status_t 
base_physical_drive_send_read_ica_stamp_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_base_physical_drive_t *             physical_drive = (fbe_base_physical_drive_t * )context;
	fbe_payload_control_operation_t * 		control_operation = NULL;  
	fbe_payload_ex_t * 						payload = NULL;
	fbe_payload_block_operation_t	*		block_operation = NULL;
	fbe_payload_block_operation_status_t	block_status;
	fbe_packet_t * 							master_packet_p = NULL;
    
    master_packet_p = fbe_transport_get_master_packet(packet);
	fbe_transport_remove_subpacket(packet);

	/*first let's get the block status of the IO*/
	payload = fbe_transport_get_payload_ex(packet);
	block_operation = fbe_payload_ex_get_block_operation(payload);  
	fbe_payload_block_get_status(block_operation, &block_status);
	fbe_payload_ex_release_block_operation(payload, block_operation);
    
	/*now the control operation of the master packets*/
	payload = fbe_transport_get_payload_ex(master_packet_p);
	control_operation = fbe_payload_ex_get_control_operation(payload);
   
	fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)physical_drive, master_packet_p);

	fbe_transport_release_packet(packet);/*don't need it anymore*/

	if (FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS != block_status) {
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s,failed to read ICA stamp\n", __FUNCTION__);

		fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
		return FBE_STATUS_OK;

	}

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet_p);
    
	return FBE_STATUS_OK;
}

static fbe_status_t base_physical_drive_get_port_info(fbe_base_physical_drive_t * physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * 					payload = NULL;
    fbe_payload_control_operation_t * 	control_operation = NULL;  
    fbe_physical_drive_get_port_info_t  *get_port_info;
    fbe_payload_control_buffer_length_t len;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_base_physical_drive_customizable_trace(physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    fbe_payload_control_get_buffer(control_operation, &get_port_info);
    if (get_port_info == NULL) {
        fbe_base_physical_drive_customizable_trace(physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, can't find bufer to read to\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_get_port_info_t)){
        fbe_base_object_trace((fbe_base_object_t *) physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%X len invalid\n", len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_base_discovered_get_port_object_id((fbe_base_discovered_t *)physical_drive, &get_port_info->port_object_id);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * base_physical_drive_set_dieh_media_threshold()
 ****************************************************************
 * @brief
 *  This function changes DIEH's media bucket threshold.
 *
 * @param physical_drive - The Physical drive object.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_set_dieh_media_threshold(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * 					payload = NULL;
    fbe_payload_control_operation_t * 	control_operation = NULL;  
    fbe_physical_drive_set_dieh_media_threshold_t  *set_dieh_media_threshold_info;
    fbe_payload_control_buffer_length_t len;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    fbe_payload_control_get_buffer(control_operation, &set_dieh_media_threshold_info);
    if (set_dieh_media_threshold_info == NULL) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, can't find bufer to read to\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_set_dieh_media_threshold_t)){
        fbe_base_object_trace((fbe_base_object_t *) base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_LEN,
                                "%X len invalid\n", len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_LOW,
                            "%s cmd:%d, option:%d\n", __FUNCTION__, set_dieh_media_threshold_info->cmd, set_dieh_media_threshold_info->option);


        
    switch(set_dieh_media_threshold_info->cmd)
    {            
        case FBE_DRIVE_THRESHOLD_CMD_RESTORE_DEFAULTS:
            fbe_base_object_lock((fbe_base_object_t *)base_physical_drive);
                base_physical_drive->dieh_mode = FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DEFAULT;
                base_physical_drive->dieh_state.media_weight_adjust = FBE_STAT_WEIGHT_CHANGE_NONE;
                /* Only if the service time is the emeh value do we want to reduce it.
                 */
                if (base_physical_drive->service_time_limit_ms == fbe_dcs_get_emeh_svc_time_limit())
                {
                    base_physical_drive->service_time_limit_ms = fbe_dcs_get_svc_time_limit();
                }
            fbe_base_object_unlock((fbe_base_object_t *)base_physical_drive);
            break;

        case FBE_DRIVE_THRESHOLD_CMD_INCREASE:
            /* If change disabled or already done just ignore.*/
            if ((base_physical_drive->dieh_state.media_weight_adjust == FBE_STAT_WEIGHT_CHANGE_DISABLE) ||
                (base_physical_drive->dieh_state.media_weight_adjust != FBE_STAT_WEIGHT_CHANGE_NONE)       )
            {
                fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                        "%s, current adjust: %d disabled or increased - ignore increase\n", 
                         __FUNCTION__, base_physical_drive->dieh_state.media_weight_adjust);
            }
            else
            {
                /* Else use the `option' as a percentage and convert to the new media adjust.
                 */
                fbe_u32_t new_media_weight_adjust;

                new_media_weight_adjust = fbe_physical_drive_convert_percent_increase_to_weight_adjust(base_physical_drive->dieh_state.media_weight_adjust,
                                                                                                       set_dieh_media_threshold_info->option); 
                /* Need to clear DIEH buckets when increasing the threshold.  The buckets which are affected by media errors are
                    Recoverable, Media, Health Check, and Cummulative. */
                fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                           "%s, changing weight adjust from: %d to: %d clear all buckets\n", 
                                                           __FUNCTION__, base_physical_drive->dieh_state.media_weight_adjust,
                                                           new_media_weight_adjust);
                fbe_base_object_lock((fbe_base_object_t *)base_physical_drive);
                    /* An increase in threshold is equivalent to a decrease in weight.  So reducing weight by the percentage that is being increased.  */                    
                    base_physical_drive->dieh_mode = FBE_DRIVE_MEDIA_MODE_THRESHOLDS_INCREASED;
                    base_physical_drive->dieh_state.media_weight_adjust = new_media_weight_adjust;
                    fbe_stat_drive_error_clear(&base_physical_drive->drive_error);  /* reset DIEH ratios to 0 */
                fbe_base_object_unlock((fbe_base_object_t *)base_physical_drive);
            }
            break;

        case FBE_DRIVE_THRESHOLD_CMD_DISABLE:
            fbe_base_object_lock((fbe_base_object_t *)base_physical_drive);
                base_physical_drive->dieh_mode = FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DISABLED;
                /* We leave the adjust but disable taking any action so that we can see that
                 * we WOULD HAVE tripped the thresholds.
                 */
                base_physical_drive->dieh_state.media_weight_adjust = FBE_STAT_WEIGHT_CHANGE_NONE;
                /* Only if the service time is still the default value do we want to increase it.
                 */
                if (base_physical_drive->service_time_limit_ms == fbe_dcs_get_svc_time_limit())
                {
                    base_physical_drive->service_time_limit_ms = fbe_dcs_get_emeh_svc_time_limit();
                }                
            fbe_base_object_unlock((fbe_base_object_t *)base_physical_drive);
            break;

        default:
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                    "%s, Invalid cmd: %d\n", __FUNCTION__, set_dieh_media_threshold_info->cmd);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    /* Push to run Q to break context to avoid stack overflow when RAID sends to multiple positions.*/
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * base_physical_drive_get_dieh_media_threshold()
 ****************************************************************
 * @brief
 *  This function get DIEH's media bucket threshold.
 *
 * @param physical_drive - The Physical drive object.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_get_dieh_media_threshold(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;  
    fbe_physical_drive_get_dieh_media_threshold_t  *get_dieh_media_threshold_info;
    fbe_u32_t                           current_weight_adjust;
    fbe_bool_t                          b_is_disabled;
    fbe_bool_t                          b_is_increased;
    fbe_bool_t                          b_is_decreased;
    fbe_u32_t                           adjust_default;
    fbe_u32_t                           percentage_change;
    fbe_payload_control_buffer_length_t len;
    fbe_drive_configuration_record_t *  threshold_rec = NULL;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            "%s entry\n", __FUNCTION__);

    fbe_payload_control_get_buffer(control_operation, &get_dieh_media_threshold_info);
    if (get_dieh_media_threshold_info == NULL) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s, can't find bufer to read to\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);    
        fbe_transport_complete_packet(packet);        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_get_dieh_media_threshold_t)){
        fbe_base_object_trace((fbe_base_object_t *) base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INVALID_LEN,
                                "%X len invalid\n", len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Convert from weight adjust to percentage increase.
     */
    current_weight_adjust = base_physical_drive->dieh_state.media_weight_adjust;
    percentage_change = fbe_physical_drive_convert_weight_adjust_to_percent_increase(current_weight_adjust,
                                                                                     &b_is_disabled,
                                                                                     &b_is_increased,
                                                                                     &b_is_decreased,
                                                                                     &adjust_default);

    /* Populate the response.
     */
    get_dieh_media_threshold_info->media_weight_adjust = current_weight_adjust;   

    /* Set dieh reliablity
     */   
    fbe_base_object_lock((fbe_base_object_t *)base_physical_drive);

    status = fbe_drive_configuration_get_threshold_info_ptr(base_physical_drive->drive_configuration_handle, &threshold_rec);

    if (status != FBE_STATUS_OK || threshold_rec == NULL)
    {
        /* if we can't get threshold info with a valid handle then something seriously went wrong */
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                       "Failed to get threshold rec. Setting drive reliablity to VERY_LOW. status=%d\n", status);  
        /* assume drive is not reliable if can't get DIEH record. */
        get_dieh_media_threshold_info->dieh_reliability = FBE_DRIVE_MEDIA_RELIABILITY_VERY_LOW;
    }
    else
    {
        fbe_u32_t media_ratio = 0;
        fbe_u32_t recovered_ratio = 0;
        fbe_stat_get_ratio(&threshold_rec->threshold_info.media_stat, base_physical_drive->drive_error.io_counter, base_physical_drive->drive_error.media_error_tag, &media_ratio);
        fbe_stat_get_ratio(&threshold_rec->threshold_info.recovered_stat, base_physical_drive->drive_error.io_counter, base_physical_drive->drive_error.media_error_tag, &recovered_ratio);

        if (threshold_rec->threshold_info.record_flags & FBE_STATS_FLAG_RELIABLY_CHALLENGED)
        {
            get_dieh_media_threshold_info->dieh_reliability = FBE_DRIVE_MEDIA_RELIABILITY_VERY_LOW;
        }
        else if (media_ratio > 0 || recovered_ratio > 0)
        {
            get_dieh_media_threshold_info->dieh_reliability = FBE_DRIVE_MEDIA_RELIABILITY_LOW;
        }
        else  /* drive is healthy and a known good drive */
        {
            get_dieh_media_threshold_info->dieh_reliability = FBE_DRIVE_MEDIA_RELIABILITY_VERY_HIGH;
        }
    }
    fbe_base_object_unlock((fbe_base_object_t *)base_physical_drive);

    get_dieh_media_threshold_info->dieh_mode = base_physical_drive->dieh_mode;
    
    fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_LOW,
                                               "get_dieh_media_threshold: reliability:%d mode:%d weight adjust:%d prcnt:%d \n", 
                                               get_dieh_media_threshold_info->dieh_reliability, get_dieh_media_threshold_info->dieh_mode,
                                               current_weight_adjust, percentage_change);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    /* Push to run Q to break context to avoid stack overflow when RAID sends to multiple positions.*/
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * base_physical_drive_reactivate()
 ****************************************************************
 * @brief
 *  This function reactivates the PDO by sending it through its
 *  ACTIVATE state in-order to refresh any of its drive info
 *  and/or notify upper layers due to a state change back to
 *  READY, such as the database service when deciding if a drive
 *  can go online.
 *
 * @param physical_drive - The Physical drive object.
 * @param packet - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
static fbe_status_t base_physical_drive_reactivate(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive, 
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                              FBE_TRACE_LEVEL_ERROR,
                              "%s can't set ACTIVATE condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;

}

static fbe_u32_t base_physical_drive_get_ica_block_count(fbe_base_physical_drive_t * physical_drive)
{
    return physical_drive->block_size/FBE_BE_BYTES_PER_BLOCK;
}
