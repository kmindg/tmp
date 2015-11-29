/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_base_board_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for the base board class.
 *
 * @ingroup fbe_board_class
 *
 * @version
 *   16-Dec-2011:  PHE - Created file header. 
 *
 ***************************************************************************/

#include "fbe_base_discovered.h"
#include "base_board_private.h"
#include "base_board_pe_private.h"
#include "fbe_notification.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_pe_types.h"
#include "edal_processor_enclosure_data.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe_api_board_interface.h"
#include "fbe/fbe_resume_prom_info.h"
#include "generic_utils_lib.h"
#include "fbe_private_space_layout.h"

extern fbe_base_board_pe_init_data_t   pe_init_data[FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES];

/* Forward declarations */
static fbe_status_t base_board_create_edge(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_sps_getSpsStatus(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_sps_getSpsManufacturingInfo(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_sps_processCommand(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_pe_getBatteryStatus(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_battery_processCommand(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_process_firmware_op(fbe_base_board_t *base_board, fbe_packet_t *packet);
static fbe_status_t base_board_process_firmware_download(fbe_base_board_t * base_board, 
                                     fbe_enclosure_mgmt_download_op_t *pDownloadOp,
                                     fbe_sg_element_t  * sg_list);
static fbe_status_t base_board_process_firmware_get_local_upgrade_permission(fbe_base_board_t *base_board, 
                                     fbe_enclosure_mgmt_download_op_t * pFirmwareOp);
static fbe_status_t base_board_process_firmware_return_local_upgrade_permission(fbe_base_board_t *base_board, 
                                     fbe_enclosure_mgmt_download_op_t * pFirmwareOp);

static fbe_status_t base_board_fetch_download_op_status(fbe_base_board_t * base_board,
                                    fbe_enclosure_mgmt_download_op_t * pDownloadOp);

void base_board_sendChangeNotification(fbe_base_board_t *baseBoard, 
                                       fbe_u64_t deviceType,
                                       fbe_u8_t slot);
static fbe_status_t base_board_pe_get_pe_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_power_supply_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_io_port_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_cooling_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_mgmt_module_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_misc_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_bmc_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_platform_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_suitcase_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_fltexp_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_slaveport_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_ps_setPsInfo(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_comp_max_timeout(fbe_base_board_t * base_board, fbe_packet_t * packet);

static fbe_status_t base_board_get_component_count(fbe_base_board_t * base_board, fbe_packet_t * packet, fbe_u32_t component_type);
static fbe_status_t base_board_get_component_count_per_blade(fbe_base_board_t * base_board, fbe_packet_t * packet, fbe_u32_t component_type);

static fbe_status_t base_board_set_spFaultLED(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_enclFaultLED(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_unsafeToRemoveLED(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_iom_fault_LED(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_iom_port_LED(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_mgmt_module_vlan_config_mode(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_mgmt_module_fault_LED(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_mgmt_module_port(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_PostAndOrReset(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_clear_fltreg_sticky_fault(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_FlushFilesAndReg(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_resume(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_cache_card_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_dimm_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_ssd_info(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_iom_get_IoCompInfo(fbe_base_board_t * base_board, fbe_packet_t * packet, 
						  fbe_enclosure_component_types_t comp_type);
static fbe_status_t base_board_get_base_board_info(fbe_base_board_t * base_board, 
                                                   fbe_packet_t * packet);
static fbe_status_t base_board_get_eir_info(fbe_base_board_t * base_board, 
                                            fbe_packet_t * packet);
static fbe_status_t base_board_get_resume(fbe_base_board_t * base_board,
                                               fbe_packet_t * packet);
static fbe_status_t base_board_set_local_software_boot_status(fbe_base_board_t * base_board, 
                                                              fbe_packet_t * packet);
static fbe_status_t base_board_set_local_flt_exp_status(fbe_base_board_t * base_board, 
                                                        fbe_packet_t * packet);
static fbe_status_t base_board_resume_read(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_resume_write(fbe_base_board_t * base_board, fbe_packet_t * packet);

static fbe_status_t 
base_board_resume_write_async(fbe_base_board_t * base_board, fbe_packet_t * packet);

static fbe_status_t base_board_parse_image_header(fbe_base_board_t *base_board, fbe_packet_t *packet);
static fbe_status_t base_board_eses_comp_type_to_fw_target(ses_comp_type_enum eses_comp_type,
                                                            fbe_enclosure_fw_target_t * pFwTarget);
static fbe_status_t 
base_board_set_iom_persisted_power_state(fbe_base_board_t * base_board, fbe_packet_t * packet);

static void base_board_process_group_policy_enable(fbe_packet_t *packet);
static fbe_status_t base_board_sim_switch_psl(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_port_serial_num(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_set_cna_mode(fbe_base_board_t * base_board, fbe_packet_t * packet);
static fbe_status_t base_board_get_hardware_ssv_data(fbe_base_board_t * base_board, fbe_packet_t * packet);

static fbe_status_t base_board_set_local_veeprom_cpu_status(fbe_base_board_t * base_board, 
                                                        fbe_packet_t * packet);
// Remove this when io port count per blade issue gets fixed.
#define NUM_IO_PORTS_PER_BLADE      12 // (iom count(2)+ mezz count(1)) * num_ports_per_iom(4)


fbe_status_t 
fbe_base_board_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_base_board_t * base_board = NULL;

    base_board = (fbe_base_board_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)base_board, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        /* We have to overwrite the create edge function.
           In fact the board edge is fake */
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE:
            status = base_board_create_edge(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_STATUS:
            status = base_board_sps_getSpsStatus(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BATTERY_STATUS:
            status = base_board_pe_getBatteryStatus(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_MANUF_INFO:
            status = base_board_sps_getSpsManufacturingInfo(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SPS_COMMAND:
            status = base_board_sps_processCommand(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_PARSE_IMAGE_HEADER:
            status = base_board_parse_image_header(base_board, packet);
            break;
            
        case FBE_BASE_BOARD_CONTROL_CODE_FIRMWARE_OP:
            status = base_board_process_firmware_op(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_PE_INFO:
            status = base_board_pe_get_pe_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_INFO:
            status = base_board_get_power_supply_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_INFO:
            status = base_board_get_io_port_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_INFO:
            status = base_board_get_mgmt_module_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_MISC_INFO:
            status = base_board_get_misc_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_INFO:
            status = base_board_get_bmc_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_INFO:
            status = base_board_get_cooling_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_INFO:
            status = base_board_get_suitcase_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO:
            status = base_board_get_platform_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_EXP_INFO:
            status = base_board_get_fltexp_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_INFO:
            status = base_board_get_fltexp_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_INFO:
            status = base_board_get_slaveport_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_CACHE_CARD_INFO:
            status = base_board_get_cache_card_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_DIMM_INFO:
            status = base_board_get_dimm_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SSD_INFO:
            status = base_board_get_ssd_info(base_board, packet);
            break;

        /* Get the overall count of the PE components */
        case FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_MGMT_MODULE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_IO_MODULE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_BEM);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_IO_PORT);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_MEZZANINE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_POWER_SUPPLY);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_COOLING_COMPONENT);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_SUITCASE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_BMC);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_PLATFORM);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_MISC_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_MISC);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_FLT_REG);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_SLAVE_PORT);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_LCC_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_LCC);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_CACHE_CARD_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_CACHE_CARD);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_DIMM_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_DIMM);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SSD_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_SSD);
            break;

        /* Per Blade count of PE components */
        case FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_POWER_SUPPLY);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_COOLING_COMPONENT);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_SUITCASE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_BMC);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_FLT_REG);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_SLAVE_PORT);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_BEM);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_IO_PORT);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_IO_MODULE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_MGMT_MODULE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_COUNT_PER_BLADE:
            status = base_board_get_component_count_per_blade(base_board, packet, FBE_ENCL_MEZZANINE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_SP_FAULT_LED:
            status = base_board_set_spFaultLED(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_ENCL_FAULT_LED:
            status = base_board_set_enclFaultLED(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_UNSAFE_TO_REMOVE_LED:
            status = base_board_set_unsafeToRemoveLED(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_FAULT_LED:
            status = base_board_set_iom_fault_LED(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PORT_LED:
            status = base_board_set_iom_port_LED(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_FAULT_LED:
            status = base_board_set_mgmt_module_fault_LED(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_VLAN_CONFIG_MODE:
            status = base_board_set_mgmt_module_vlan_config_mode(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_PORT:
            status = base_board_set_mgmt_module_port(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_CLEAR_HOLD_IN_POST_AND_OR_RESET:
            status = base_board_set_PostAndOrReset(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_CLEAR_FLT_REG_FAULT:
            status = base_board_clear_fltreg_sticky_fault(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_FLUSH_FILES_AND_REG:
            status = base_board_set_FlushFilesAndReg(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_RESUME:  
            status = base_board_set_resume(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_RESUME:  // Get the entire resume prom
            status = base_board_get_resume(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_RESUME_READ: // Get the field of the resume prom
            status = base_board_resume_read(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE:
            status = base_board_resume_write(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE_ASYNC:
            status = base_board_resume_write_async(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_INFO:
            status = base_board_iom_get_IoCompInfo(base_board, packet, FBE_ENCL_IO_MODULE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_INFO:
            status = base_board_iom_get_IoCompInfo(base_board, packet, FBE_ENCL_BEM);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_INFO:
            status = base_board_iom_get_IoCompInfo(base_board, packet, FBE_ENCL_MEZZANINE);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_PS_INFO:
			status = base_board_ps_setPsInfo(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_COMPONENT_MAX_TIMEOUT:
            status = base_board_set_comp_max_timeout(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_FBE_BASE_BOARD_INFO:
            status = base_board_get_base_board_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_EIR_INFO:
            status = base_board_get_eir_info(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_SOFTWARE_BOOT_STATUS:
            status = base_board_set_local_software_boot_status(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_FLT_EXP_STATUS:
            status = base_board_set_local_flt_exp_status(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_SPS);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_GET_BATTERY_COUNT:
            status = base_board_get_component_count(base_board, packet, FBE_ENCL_BATTERY);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_BATTERY_COMMAND:
            status = base_board_battery_processCommand(base_board, packet);
            break;

         case FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_SERIAL_NUM:
            status = base_board_get_port_serial_num(base_board, packet);
            break;

		case FBE_BASE_BOARD_CONTROL_CODE_SET_ASYNC_IO:
			fbe_cpd_shim_set_async_io(FBE_TRUE);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			status = fbe_transport_complete_packet(packet);
			break;
		case FBE_BASE_BOARD_CONTROL_CODE_SET_SYNC_IO:
			fbe_cpd_shim_set_async_io(FBE_FALSE);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			status = fbe_transport_complete_packet(packet);
			break;

        case FBE_BASE_BOARD_CONTROL_CODE_ENABLE_PP_GROUP_PRIORITY:
			base_board_process_group_policy_enable(packet);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			status = fbe_transport_complete_packet(packet);
			break;
		case FBE_BASE_BOARD_CONTROL_CODE_DISABLE_PP_GROUP_PRIORITY:
			fbe_transport_disable_fast_priority();
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			status = fbe_transport_complete_packet(packet);
			break;
          
		case FBE_BASE_BOARD_CONTROL_CODE_ENABLE_DMRB_ZEROING:
			fbe_cpd_shim_set_dmrb_zeroing(FBE_TRUE);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			status = fbe_transport_complete_packet(packet);
			break;
		case FBE_BASE_BOARD_CONTROL_CODE_DISABLE_DMRB_ZEROING:
			fbe_cpd_shim_set_dmrb_zeroing(FBE_FALSE);
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			status = fbe_transport_complete_packet(packet);
			break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PERSISTED_POWER_STATE:
            status = base_board_set_iom_persisted_power_state(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SIM_SWITCH_PSL:
            status = base_board_sim_switch_psl(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_CNA_MODE:
            status = base_board_set_cna_mode(base_board, packet);
            break;

        case FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_VEEPROM_CPU_STATUS:
            status = base_board_set_local_veeprom_cpu_status(base_board, packet);
            break;
        case FBE_BASE_BOARD_CONTROL_CODE_GET_HARDWARE_SSV_DATA:
            status = base_board_get_hardware_ssv_data(base_board, packet);
            break;

        default:
            status = fbe_base_discovering_control_entry(object_handle, packet);
            break;
    }

	return status;
}

static fbe_status_t 
base_board_create_edge(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_edge_t * base_edge = NULL;
	fbe_base_object_trace((fbe_base_object_t*)base_board, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

	/* The board discovery edge is fake 
		We will modify path state here to make our discovered base class happy.
		Note: The direct access to the path_state from client side are NOT allowed
	*/
	base_edge = (fbe_base_edge_t *)(&((fbe_base_discovered_t *)base_board)->discovery_edge);
	base_edge->path_state = FBE_PATH_STATE_ENABLED;

	fbe_base_object_trace((fbe_base_object_t*)base_board,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s calls fbe_lifecycle_reschedule\n", __FUNCTION__);

	status = fbe_lifecycle_reschedule((fbe_lifecycle_const_t *) &FBE_LIFECYCLE_CONST_DATA(base_board),
                                      (fbe_base_object_t *) base_board,
                                      (fbe_lifecycle_timer_msec_t) 0); /* Immediate reschedule */

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);

	return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn base_board_sps_getSpsStatus(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to return SPS Status.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   05-Jan-2010     Joe Perry - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_sps_getSpsStatus(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_sps_get_sps_status_t    *spsStatusInfoPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    edal_pe_sps_sub_info_t                  peSpsInfo = {0};
    edal_pe_sps_sub_info_t                  *pPeSpsInfo = &peSpsInfo;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsStatusInfoPtr);
    if (spsStatusInfoPtr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_sps_get_sps_status_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get current SPS info
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_SPS_INFO,              //Attribute
                                     FBE_ENCL_SPS,                      // component type
                                     0,                                 // comonent index
                                     sizeof(edal_pe_sps_sub_info_t),    // buffer length
                                     (fbe_u8_t *)pPeSpsInfo);                       // buffer pointer 
    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get SPS Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    fbe_copy_memory(&spsStatusInfoPtr->spsInfo,
                   &pPeSpsInfo->externalSpsInfo,
                   sizeof(fbe_base_sps_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}   // end of base_board_sps_getSpsStatus

/*!*************************************************************************
* @fn base_board_sps_getSpsManufacturingInfo(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to return SPS 
*       Manufacturing Info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   05-Jan-2010     Joe Perry - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_sps_getSpsManufacturingInfo(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_sps_get_sps_manuf_info_t    *spsManufInfoPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    edal_pe_sps_sub_info_t                  peSpsInfo = {0};
    edal_pe_sps_sub_info_t                  *pPeSpsInfo = &peSpsInfo;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsManufInfoPtr);
    if (spsManufInfoPtr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_sps_get_sps_manuf_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get current SPS Manufacturing info
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_SPS_INFO,              //Attribute
                                     FBE_ENCL_SPS,                      // component type
                                     0,                                 // comonent index
                                     sizeof(edal_pe_sps_sub_info_t),    // buffer length
                                     (fbe_u8_t *)pPeSpsInfo);                       // buffer pointer 
    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get SPS Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    spsManufInfoPtr->spsManufInfo = pPeSpsInfo->externalSpsManufInfo;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}   // end of base_board_sps_getSpsManufacturingInfo

/*!*************************************************************************
* @fn base_board_sps_processCommand(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code for SPS commands (start Testing, 
*       stop Testing, shutdown SPS).
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   05-Jan-2010     Joe Perry - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_sps_processCommand(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_sps_action_type_t    *spsCommandPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t        status = FBE_STATUS_OK;
    edal_pe_sps_sub_info_t                  peSpsInfo = {0};
    edal_pe_sps_sub_info_t                  *pPeSpsInfo = &peSpsInfo;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spsCommandPtr);
    if (spsCommandPtr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_sps_action_type_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Process the SPS command based on current SPS status & command type
     */
    // get current SPS info
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_SPS_INFO,              //Attribute
                                     FBE_ENCL_SPS,                      // component type
                                     0,                                 // comonent index
                                     sizeof(edal_pe_sps_sub_info_t),    // buffer length
                                     (fbe_u8_t *)pPeSpsInfo);                       // buffer pointer 
    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get SPS Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // determine type of SPS command
    switch (*spsCommandPtr)
    {
    case FBE_SPS_ACTION_START_TEST:
        // start SPS Testing if SPS is valid state to do so
        if (pPeSpsInfo->externalSpsInfo.spsStatus != SPS_STATE_AVAILABLE)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Invalid SPS status (%d) for START_TEST\n", 
                                  __FUNCTION__, pPeSpsInfo->externalSpsInfo.spsStatus);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_payload_control_set_status_qualifier(control_operation, pPeSpsInfo->externalSpsInfo.spsStatus);
        }
        else
        {
            status = fbe_base_board_pe_start_sps_testing();
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Start SPS Test successful\n", 
                                      __FUNCTION__);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, error starting SPS Test, status 0x%x\n", 
                                      __FUNCTION__, status);
            }
        }
        break;

    case FBE_SPS_ACTION_STOP_TEST:
        // start SPS Testing if SPS is valid state to do so
        if (pPeSpsInfo->externalSpsInfo.spsStatus != SPS_STATE_TESTING)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Invalid SPS status (%d) for STOP_TEST\n", 
                                  __FUNCTION__, pPeSpsInfo->externalSpsInfo.spsStatus);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_payload_control_set_status_qualifier(control_operation, pPeSpsInfo->externalSpsInfo.spsStatus);
        }
        else
        {
            status = fbe_base_board_pe_stop_sps_testing();
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Stop SPS Test successful\n", 
                                      __FUNCTION__);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, error stopping SPS Test, status 0x%x\n", 
                                      __FUNCTION__, status);
            }
        }
        break;

    case FBE_SPS_ACTION_SHUTDOWN:
        // start SPS Testing if SPS is valid state to do so
        if (pPeSpsInfo->externalSpsInfo.spsStatus != SPS_STATE_ON_BATTERY)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Invalid SPS status (%d) for START_TEST\n", 
                                  __FUNCTION__, pPeSpsInfo->externalSpsInfo.spsStatus);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_payload_control_set_status_qualifier(control_operation, pPeSpsInfo->externalSpsInfo.spsStatus);
        }
        else
        {
            status = fbe_base_board_pe_shutdown_sps();
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, SPS Shutdown successful\n", 
                                      __FUNCTION__);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, error shutting down SPS, status 0x%x\n", 
                                      __FUNCTION__, status);
            }
        }
        break;

    case FBE_SPS_ACTION_SET_SPS_PRESENT:
        status = fbe_base_board_pe_set_sps_present(base_board->spid, TRUE);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, set SPS Present successful\n", 
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, error setting SPS Present, status 0x%x\n", 
                                  __FUNCTION__, status);
        }
        break;

    case FBE_SPS_ACTION_CLEAR_SPS_PRESENT:
        status = fbe_base_board_pe_set_sps_present(base_board->spid, FALSE);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, clear SPS Present successful\n", 
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, error setting SPS Present, status 0x%x\n", 
                                  __FUNCTION__, status);
        }
        break;

    case FBE_SPS_ACTION_ENABLE_SPS_FAST_SWITCHOVER:
        status = fbe_base_board_pe_set_sps_fastSwitchover(TRUE);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, enable SPS FastSwitchover successful\n", 
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, error enabling SPS FastSwitchover, status 0x%x\n", 
                                  __FUNCTION__, status);
        }
        break;

    case FBE_SPS_ACTION_DISABLE_SPS_FAST_SWITCHOVER:
        status = fbe_base_board_pe_set_sps_fastSwitchover(FALSE);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, disable SPS FastSwitchover successful\n", 
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, error disabling SPS FastSwitchover, status 0x%x\n", 
                                  __FUNCTION__, status);
        }
        break;

    case FBE_SPS_ACTION_NONE:
    default:
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "No action defined for SPS action (%d)\n", *spsCommandPtr);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_control_set_status_qualifier(control_operation, *spsCommandPtr);
        break;
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

	return status;

}   // end of base_board_sps_processCommand



/*!*************************************************************************
* @fn base_board_battery_processCommand(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code for Battery commands (start Testing, 
*       stop Testing).
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   06-Apr-2012     Joe Perry - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_battery_processCommand(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_base_board_mgmt_battery_command_t    *batteryCommandPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t        status = FBE_STATUS_OK;
    edal_pe_battery_sub_info_t                  peBatteryInfo = {0};
    edal_pe_battery_sub_info_t                  *pPeBatteryInfo = &peBatteryInfo;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_u32_t                               componentIndex, componentCount;
    edal_pe_bmc_sub_info_t      peBmcSubInfo = {0};     
    edal_pe_bmc_sub_info_t    * pPeBmcSubInfo = &peBmcSubInfo;
    fbe_edal_block_handle_t     edalBlockHandle = NULL;   

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &batteryCommandPtr);
    if (batteryCommandPtr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_mgmt_battery_command_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Process the Battery command based on current Battery status & command type
     */
    // get current SPS info
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (batteryCommandPtr->device_location.sp == SP_A)
    {
        componentIndex = batteryCommandPtr->device_location.slot;
    }
    else
    {
        edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle,
                                                        FBE_ENCL_BATTERY,
                                                        &componentCount);
            if(!EDAL_STAT_OK(edal_status))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }

        componentIndex = batteryCommandPtr->device_location.slot + (componentCount/2);
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_BATTERY_INFO,          //Attribute
                                     FBE_ENCL_BATTERY,                  // component type
                                     componentIndex,                    // comonent index
                                     sizeof(edal_pe_battery_sub_info_t),// buffer length
                                     (fbe_u8_t *)pPeBatteryInfo);       // buffer pointer 
    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Battery Info, componentIndex %d.\n",
                        __FUNCTION__, componentIndex);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // determine type of SPS command
    switch (batteryCommandPtr->batteryAction)
    {
    case FBE_BATTERY_ACTION_START_TEST:
        // start Battery Testing if Battery is valid state to do so
        if (pPeBatteryInfo->externalBatteryInfo.batteryState != FBE_BATTERY_STATUS_BATTERY_READY)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Invalid Battery status (%d) for START_TEST\n", 
                                  __FUNCTION__, pPeBatteryInfo->externalBatteryInfo.batteryState);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_payload_control_set_status_qualifier(control_operation, pPeBatteryInfo->externalBatteryInfo.batteryState);
        }
        else
        {
            status = fbe_base_board_pe_start_battery_testing(base_board->spid, batteryCommandPtr->device_location.slot);
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Start Battery Test successful, sp %d, slot %d\n", 
                                      __FUNCTION__, batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, error starting Battery Test, sp %d, slot %d, status 0x%x\n", 
                                      __FUNCTION__, batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot, status);
            }
        }
        break;

    case FBE_BATTERY_ACTION_ENABLE:
            status = fbe_base_board_pe_enable_battery(batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot);
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Enable Battery successful, sp %d, slot %d\n", 
                                      __FUNCTION__, batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, error in enabling Battery, sp %d, slot %d, status 0x%x\n", 
                                      __FUNCTION__, batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot, status);
            }
        break;

    case FBE_BATTERY_ACTION_SHUTDOWN:
            status = fbe_base_board_pe_disable_battery(batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot);
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Disable Battery successful, sp %d, slot %d\n", 
                                      __FUNCTION__, batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, error in Disabling Battery, sp %d, slot %d, status 0x%x\n", 
                                      __FUNCTION__, batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot, status);
            }
        break;

    case FBE_BATTERY_ACTION_POWER_REQ_INIT:
        batteryCommandPtr->batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.batteryID = 
            batteryCommandPtr->device_location.slot;
        status = fbe_base_board_pe_set_battery_energy_req(batteryCommandPtr->device_location.sp, 
                                                          batteryCommandPtr->device_location.slot,
                                                          batteryCommandPtr->batteryActionInfo);
        if (status == FBE_STATUS_OK)
        {
            if (batteryCommandPtr->batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.format == BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Init Power Req successful, sp %d, slot %d, energy %d, maxLoad %d\n", 
                                      __FUNCTION__, 
                                      batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot,
                                      batteryCommandPtr->batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.energyAndPower.energy,
                                      batteryCommandPtr->batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.energyAndPower.power);
            }
            else if (batteryCommandPtr->batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.format == BATTERY_ENERGY_REQUIREMENTS_FORMAT_POWER_AND_TIME)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Init Power Req successful, sp %d, slot %d, power %d, time %d\n", 
                                      __FUNCTION__, 
                                      batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot,
                                      batteryCommandPtr->batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.powerAndTime.power,
                                      batteryCommandPtr->batteryActionInfo.setPowerReqInfo.batteryEnergyRequirement.data.powerAndTime.time);
            }
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, error in Init Battery Power Req, sp %d, slot %d, status 0x%x\n", 
                                  __FUNCTION__, batteryCommandPtr->device_location.sp, batteryCommandPtr->device_location.slot, status);
        }
        break;

    case FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH:
    case FBE_BATTERY_ACTION_DISABLE_RIDE_THROUGH:
        // need to get reported BMC attributes to issue commands
        fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);
        edal_status = fbe_edal_getBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_BMC_INFO,                   //Attribute
                                        FBE_ENCL_BMC,                           // component type
                                        batteryCommandPtr->device_location.sp,  // component index
                                        sizeof(edal_pe_bmc_sub_info_t),         // buffer length
                                        (fbe_u8_t *)pPeBmcSubInfo);             // buffer pointer

        if(!EDAL_STAT_OK(edal_status))
        {
            status = FBE_STATUS_GENERIC_FAILURE;  
        }    
        else if (!pPeBmcSubInfo->externalBmcInfo.rideThroughTimeSupported)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, RideThroughTimer not supported, nothing to do\n", 
                                  __FUNCTION__);
            status = FBE_STATUS_OK;
        }
        else
        {
            if (batteryCommandPtr->batteryAction == FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH) 
            {
                status = fbe_base_board_pe_enableRideThroughTimer(batteryCommandPtr->device_location.sp,
                                                                  pPeBmcSubInfo->externalBmcInfo.lowPowerMode,
                                                                  pPeBmcSubInfo->externalBmcInfo.vaultTimeout); 
            }
            else
            {
                status = fbe_base_board_pe_disableRideThroughTimer(batteryCommandPtr->device_location.sp,
                                                                   pPeBmcSubInfo->externalBmcInfo.lowPowerMode,
                                                                   pPeBmcSubInfo->externalBmcInfo.vaultTimeout); 
            }
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, %s RideThroughTimer successful, sp %d, slot %d\n", 
                                      __FUNCTION__, 
                                      ((batteryCommandPtr->batteryAction == FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH) ? "Enable" : "Disable"),
                                      batteryCommandPtr->device_location.sp, 
                                      batteryCommandPtr->device_location.slot);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, error in %s RideThroughTimer, sp %d, slot %d, status 0x%x\n", 
                                      __FUNCTION__, 
                                      ((batteryCommandPtr->batteryAction == FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH) ? "Enable" : "Disable"),
                                      batteryCommandPtr->device_location.sp, 
                                      batteryCommandPtr->device_location.slot, status);
            }
        }
        break;

    case FBE_BATTERY_ACTION_NONE:
    default:
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "No action defined for Battery action (%d)\n", batteryCommandPtr->batteryAction);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_control_set_status_qualifier(control_operation, batteryCommandPtr->batteryAction);
        break;
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

	return status;

}   // end of base_board_battery_processCommand


/*!*************************************************************************
* @fn base_board_pe_getBatteryStatus(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to return Battery Status.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   22-Mar-2012     Joe Perry - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_pe_getBatteryStatus(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_base_board_mgmt_get_battery_status_t    *batteryStatusInfoPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    edal_pe_battery_sub_info_t              peBatteryInfo = {0};
    edal_pe_battery_sub_info_t              *pPeBatteryInfo = &peBatteryInfo;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_u32_t                               componentIndex, componentCount;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &batteryStatusInfoPtr);
    if (batteryStatusInfoPtr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_mgmt_get_battery_status_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get current Battery info
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (batteryStatusInfoPtr->device_location.sp == SP_A)
    {
        componentIndex = batteryStatusInfoPtr->device_location.slot;
    }
    else
    {
        edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle,
                                                        FBE_ENCL_BATTERY,
                                                        &componentCount);
            if(!EDAL_STAT_OK(edal_status))
            {
                return FBE_STATUS_GENERIC_FAILURE;  
            }

        componentIndex = batteryStatusInfoPtr->device_location.slot + (componentCount/2);
    }
    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_BATTERY_INFO,              // Attribute
                                     FBE_ENCL_BATTERY,                      // component type
                                     componentIndex,                        // comonent index
                                     sizeof(edal_pe_battery_sub_info_t),    // buffer length
                                     (fbe_u8_t *)pPeBatteryInfo);           // buffer pointer 
    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Battery Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    batteryStatusInfoPtr->batteryInfo.envInterfaceStatus = 
        pPeBatteryInfo->externalBatteryInfo.envInterfaceStatus;
    batteryStatusInfoPtr->batteryInfo.batteryInserted = 
        pPeBatteryInfo->externalBatteryInfo.batteryInserted;
    batteryStatusInfoPtr->batteryInfo.batteryEnabled = 
        pPeBatteryInfo->externalBatteryInfo.batteryEnabled;
    batteryStatusInfoPtr->batteryInfo.onBattery = 
        pPeBatteryInfo->externalBatteryInfo.onBattery;
    batteryStatusInfoPtr->batteryInfo.batteryReady = 
        pPeBatteryInfo->externalBatteryInfo.batteryReady;
    batteryStatusInfoPtr->batteryInfo.batteryState = 
        pPeBatteryInfo->externalBatteryInfo.batteryState;
    batteryStatusInfoPtr->batteryInfo.batteryChargeState = 
        pPeBatteryInfo->externalBatteryInfo.batteryChargeState;
    batteryStatusInfoPtr->batteryInfo.batteryFaults = 
        pPeBatteryInfo->externalBatteryInfo.batteryFaults;
    batteryStatusInfoPtr->batteryInfo.batteryTestStatus = 
        pPeBatteryInfo->externalBatteryInfo.batteryTestStatus;
    batteryStatusInfoPtr->batteryInfo.batteryFfid = 
        pPeBatteryInfo->externalBatteryInfo.batteryFfid;
    batteryStatusInfoPtr->batteryInfo.batteryDeliverableCapacity = 
        pPeBatteryInfo->externalBatteryInfo.batteryDeliverableCapacity;
    batteryStatusInfoPtr->batteryInfo.batteryStorageCapacity = 
        pPeBatteryInfo->externalBatteryInfo.batteryStorageCapacity;
    batteryStatusInfoPtr->batteryInfo.batteryTestStatus = 
        pPeBatteryInfo->externalBatteryInfo.batteryTestStatus;
    batteryStatusInfoPtr->batteryInfo.associatedSp = 
        pPeBatteryInfo->externalBatteryInfo.associatedSp;
    batteryStatusInfoPtr->batteryInfo.slotNumOnSpBlade = 
        pPeBatteryInfo->externalBatteryInfo.slotNumOnSpBlade;
    batteryStatusInfoPtr->batteryInfo.isFaultRegFail = 
        pPeBatteryInfo->externalBatteryInfo.isFaultRegFail;
    batteryStatusInfoPtr->batteryInfo.hasResumeProm = 
        pPeBatteryInfo->externalBatteryInfo.hasResumeProm;
    batteryStatusInfoPtr->batteryInfo.firmwareRevMajor = 
        pPeBatteryInfo->externalBatteryInfo.firmwareRevMajor;
    batteryStatusInfoPtr->batteryInfo.firmwareRevMinor = 
        pPeBatteryInfo->externalBatteryInfo.firmwareRevMinor;
    fbe_copy_memory(&batteryStatusInfoPtr->batteryInfo.energyRequirement, 
                    &pPeBatteryInfo->externalBatteryInfo.energyRequirement, 
                    sizeof(fbe_battery_energy_requirement_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}   // end of base_board_pe_getBatteryStatus

/*!*************************************************************************
* @fn base_board_ps_setPsInfo(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to modifiy PS Info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   27-Apr-2010     Joe Perry - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_ps_setPsInfo(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_base_board_mgmt_set_ps_info_t       *psInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_bool_t                              psInfoChange = FALSE;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_power_supply_sub_info_t         pePSInfo = {0};
    edal_pe_power_supply_sub_info_t         *pPePSInfo = &pePSInfo;
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &psInfoPtr);
    if (psInfoPtr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_mgmt_set_ps_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Sanity check */
    if( pPePSInfo->externalPowerSupplyInfo.slotNumOnEncl != psInfoPtr->psIndex)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, slotNumOnEncl mismatch, Rqstd: %d, Rcvd: %d.\n",
                        __FUNCTION__,
                        pPePSInfo->externalPowerSupplyInfo.slotNumOnEncl,
                        psInfoPtr->psIndex);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * set current PS info
     */
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_POWER_SUPPLY, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

   
    if(psInfoPtr->psIndex < component_count)
    {
        component_index = psInfoPtr->psIndex;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid slotNumOnEncl %d.\n",
                        __FUNCTION__,
                        psInfoPtr->psIndex);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }
  
    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_POWER_SUPPLY_INFO, //Attribute
                                     FBE_ENCL_POWER_SUPPLY, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_power_supply_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPePSInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Power Supply Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    // check Insert change
    if (psInfoPtr->psInfo.bInserted != pPePSInfo->externalPowerSupplyInfo.bInserted)
    {
        psInfoChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, ps %d, psInserted change, old %d, new %d\n", 
                              __FUNCTION__, 
                              component_index,
                              pPePSInfo->externalPowerSupplyInfo.bInserted,
                              psInfoPtr->psInfo.bInserted);
        pPePSInfo->externalPowerSupplyInfo.bInserted = psInfoPtr->psInfo.bInserted;
    }
    // check Faulted change
    if (psInfoPtr->psInfo.generalFault != pPePSInfo->externalPowerSupplyInfo.generalFault)
    {
        psInfoChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, ps %d, psFault change, old %d, new %d\n", 
                              __FUNCTION__, 
                              component_index,
                              pPePSInfo->externalPowerSupplyInfo.generalFault,
                              psInfoPtr->psInfo.generalFault);
        pPePSInfo->externalPowerSupplyInfo.generalFault = psInfoPtr->psInfo.generalFault;
    }
    // check AC_FAIL change
    if (psInfoPtr->psInfo.ACFail != pPePSInfo->externalPowerSupplyInfo.ACFail)
    {
        psInfoChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, ps %d, psAcFail change, old %d, new %d\n", 
                              __FUNCTION__, 
                              component_index,
                              pPePSInfo->externalPowerSupplyInfo.ACFail,
                              psInfoPtr->psInfo.ACFail);
        pPePSInfo->externalPowerSupplyInfo.ACFail = psInfoPtr->psInfo.ACFail;
    }

    // send notification if state changed
    if (psInfoChange)
    {
    edal_status = fbe_edal_setBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_POWER_SUPPLY_INFO, //Attribute
                                     FBE_ENCL_POWER_SUPPLY, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_power_supply_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPePSInfo); // buffer pointer 

    if((edal_status != FBE_EDAL_STATUS_OK) &&
        (edal_status != FBE_EDAL_STATUS_OK_VAL_CHANGED))
//    if(!EDAL_STATUS_OK(edal_status))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not set Power Supply Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    base_board_sendChangeNotification(base_board, FBE_DEVICE_TYPE_PS, psInfoPtr->psIndex);
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}   // end of base_board_ps_setPsInfo

/*!*************************************************************************
* @fn base_board_sendChangeNotification(                  
*                    fbe_base_object_t * baseBoard,
*                    fbe_u64_t deviceType,
*                    fbe_u8_t slot) 
***************************************************************************
*
* @brief
*       This function send an event notification for the specified device
*       from the Board Object.
*
* @param      baseBoard - The pointer to the fbe_base_object_t.
* @param      deviceType - type of device the event is for.
* @param      slot - slot/index for the changing component.
*
* @return     void.
*
* NOTES
*
* HISTORY
*   27-Apr-2010     Joe Perry - Created.
*
***************************************************************************/
void base_board_sendChangeNotification(fbe_base_board_t *baseBoard,
                                       fbe_u64_t deviceType,
                                       fbe_u8_t slot)
{
    fbe_notification_info_t     notification;
    fbe_object_id_t             my_object_id;
    
    fbe_base_object_get_object_id((fbe_base_object_t *)baseBoard, &my_object_id);

    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
	notification.class_id = FBE_CLASS_ID_BASE_BOARD;
	notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_BOARD;
    notification.notification_data.data_change_info.device_mask = deviceType;
    notification.notification_data.data_change_info.phys_location.slot = slot;
    fbe_notification_send(my_object_id, notification);

}   // end of base_board_sendChangeNotification

/*!*************************************************************************
* @fn base_board_pe_get_pe_info(fbe_base_object_t * base_board, 
*                               fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*   This function services the Control Code to return the processor enclosure info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   16-Mar-2010     Arun S - Created.
*   23-Mar-2010     PHE - copy EDAL blocks.
*
***************************************************************************/
static fbe_status_t 
base_board_pe_get_pe_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_board_mgmt_get_pe_info_t    *pe_info_p = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_edal_block_handle_t edal_block_handle = NULL;
    fbe_edal_status_t edal_status = FBE_EDAL_STATUS_OK;
    fbe_status_t status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pe_info_p);
    if (pe_info_p == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_get_pe_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, get_buffer_length failed, len %d.\n", 
                              __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the edal is there */
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy EDAL blocks over. */
    edal_status = fbe_edal_copyBlockData(edal_block_handle, 
                                         (fbe_u8_t *)pe_info_p,
                                          sizeof(fbe_board_mgmt_get_pe_info_t));

    if(edal_status == FBE_EDAL_STATUS_OK)
    {
        status = FBE_STATUS_OK;
    }
    else
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}// end of board_pe_get_pe_info

/*!*************************************************************************
* @fn base_board_get_power_supply_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Power Supply info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   15-Apr-2010     Joe Perry - Created.
*   19-May-2010     Arun S - Modified to use the new data structure.
*
***************************************************************************/
static fbe_status_t 
base_board_get_power_supply_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_power_supply_info_t                 *psInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_power_supply_sub_info_t         pePSInfo = {0};
    edal_pe_power_supply_sub_info_t         *pPePSInfo = &pePSInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &psInfoPtr);
    if (psInfoPtr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_power_supply_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_POWER_SUPPLY, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

   
    if(psInfoPtr->slotNumOnEncl < component_count)
    {
        component_index = psInfoPtr->slotNumOnEncl;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid slotNumOnEncl %d.\n",
                        __FUNCTION__,
                        psInfoPtr->slotNumOnEncl);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }
  
    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_POWER_SUPPLY_INFO, //Attribute
                                     FBE_ENCL_POWER_SUPPLY, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_power_supply_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPePSInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Power Supply Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if( pPePSInfo->externalPowerSupplyInfo.slotNumOnEncl != psInfoPtr->slotNumOnEncl)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, slotNumOnEncl mismatch, Rqstd: %d, Rcvd: %d.\n",
                        __FUNCTION__,
                        pPePSInfo->externalPowerSupplyInfo.slotNumOnEncl,
                        psInfoPtr->slotNumOnEncl);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(psInfoPtr, &pPePSInfo->externalPowerSupplyInfo, sizeof(fbe_power_supply_info_t));
    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, slot %d, ins %d, flt %d, acFail %d\n",
                          __FUNCTION__,
                          psInfoPtr->slotNumOnEncl,
                          psInfoPtr->bInserted,
                          psInfoPtr->generalFault,
                          psInfoPtr->ACFail);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}   // end of base_board_get_ps_info

/*!*************************************************************************
* @fn base_board_get_io_port_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get IO Port info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   14-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_io_port_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_board_mgmt_io_port_info_t           *IOPortInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_status_t                            status = FBE_EDAL_STATUS_OK;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_io_port_sub_info_t              peIoPortSubInfo = {0};
    edal_pe_io_port_sub_info_t              *pPeIoPortSubInfo = &peIoPortSubInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_LOW, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &IOPortInfoPtr);
    if (IOPortInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_io_port_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* Get the EDAL component index for the IO port. */
    status = fbe_base_board_get_io_port_component_index(base_board, 
                                                        IOPortInfoPtr->associatedSp,
                                                        IOPortInfoPtr->slotNumOnBlade, 
                                                        IOPortInfoPtr->portNumOnModule,
                                                        IOPortInfoPtr->deviceType,
                                                        &component_index);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Failed to get io port component index, status 0x%X.\n",
                        __FUNCTION__,
                        status);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    } 
      
    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_IO_PORT_INFO, //Attribute
                                     FBE_ENCL_IO_PORT, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_io_port_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeIoPortSubInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get IO Port Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if(( pPeIoPortSubInfo->externalIoPortInfo.associatedSp != IOPortInfoPtr->associatedSp) ||
        (pPeIoPortSubInfo->externalIoPortInfo.slotNumOnBlade != IOPortInfoPtr->slotNumOnBlade) ||
        (pPeIoPortSubInfo->externalIoPortInfo.portNumOnModule != IOPortInfoPtr->portNumOnModule))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, compIndex %d, deviceType %lld, Rqstd: %d(SP), %d(Slot) %d(Port), Rcvd: %d(SP), %d(Slot), %d(Port).\n",
                        __FUNCTION__,
                        component_index,
                        IOPortInfoPtr->deviceType,
                        pPeIoPortSubInfo->externalIoPortInfo.associatedSp,
                        pPeIoPortSubInfo->externalIoPortInfo.slotNumOnBlade,
                        pPeIoPortSubInfo->externalIoPortInfo.portNumOnModule,
                        IOPortInfoPtr->associatedSp,
                        IOPortInfoPtr->slotNumOnBlade,
                        IOPortInfoPtr->portNumOnModule);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(IOPortInfoPtr, &pPeIoPortSubInfo->externalIoPortInfo, sizeof(fbe_board_mgmt_io_port_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_io_port_info

/*!*************************************************************************
* @fn base_board_get_mgmt_module_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Mgmt Module info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   19-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_mgmt_module_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_board_mgmt_mgmt_module_info_t       *MgmtModInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_mgmt_module_sub_info_t          peMgmtModSubInfo = {0};
    edal_pe_mgmt_module_sub_info_t          *pPeMgmtModSubInfo = &peMgmtModSubInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &MgmtModInfoPtr);
    if (MgmtModInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_mgmt_module_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_MGMT_MODULE, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(MgmtModInfoPtr->associatedSp < SP_ID_MAX)
    {
        if(MgmtModInfoPtr->mgmtID < component_count)
        {
            component_index = (MgmtModInfoPtr->associatedSp == SP_A) ? MgmtModInfoPtr->mgmtID : 
	                                    (MgmtModInfoPtr->mgmtID + (component_count/2));
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, Invalid ID - %d.\n",
                            __FUNCTION__,
                            MgmtModInfoPtr->mgmtID);
        
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;  
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid SP - %d.\n",
                        __FUNCTION__,
                        MgmtModInfoPtr->associatedSp);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_MGMT_MODULE_INFO, //Attribute
                                     FBE_ENCL_MGMT_MODULE, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_mgmt_module_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeMgmtModSubInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Mgmt Module Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if(( pPeMgmtModSubInfo->externalMgmtModuleInfo.associatedSp != MgmtModInfoPtr->associatedSp) ||
        (pPeMgmtModSubInfo->externalMgmtModuleInfo.mgmtID != MgmtModInfoPtr->mgmtID))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Rqstd: %d(SP), %d(Slot), Rcvd: %d(SP), %d(Slot).\n",
                        __FUNCTION__,
                        pPeMgmtModSubInfo->externalMgmtModuleInfo.associatedSp,
                        pPeMgmtModSubInfo->externalMgmtModuleInfo.mgmtID,
                        MgmtModInfoPtr->associatedSp,
                        MgmtModInfoPtr->mgmtID);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(MgmtModInfoPtr, &pPeMgmtModSubInfo->externalMgmtModuleInfo, sizeof(fbe_board_mgmt_mgmt_module_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_mgmt_module_info

/*!*************************************************************************
* @fn base_board_get_cooling_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Cooling info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   19-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_cooling_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_cooling_info_t                      *coolingInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_cooling_sub_info_t              peCoolingInfo = {0};
    edal_pe_cooling_sub_info_t              *pPeCoolingInfo = &peCoolingInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &coolingInfoPtr);
    if (coolingInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_cooling_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

        edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_COOLING_INFO, //Attribute
                                     FBE_ENCL_COOLING_COMPONENT, // component type
                                     coolingInfoPtr->slotNumOnEncl, // comonent index
                                     sizeof(edal_pe_cooling_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeCoolingInfo); // buffer pointer 

        if(edal_status != FBE_EDAL_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Cooling Info.\n",
                        __FUNCTION__);
    
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;  
        }    

    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(coolingInfoPtr, &pPeCoolingInfo->externalCoolingInfo, sizeof(fbe_cooling_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_cooling_info

/*!*************************************************************************
* @fn base_board_get_misc_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Misc info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   17-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_misc_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_board_mgmt_misc_info_t              *miscInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &miscInfoPtr);
    if (miscInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_misc_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_MISC_INFO, //Attribute
                                     FBE_ENCL_MISC, // component type
                                     0, // comonent index
                                     sizeof(fbe_board_mgmt_misc_info_t), // buffer length
                                     (fbe_u8_t *)miscInfoPtr); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Misc Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }  
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_misc_info

/*!*************************************************************************
* @fn base_board_get_platform_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Platform info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   17-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_platform_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_board_mgmt_platform_info_t          *platformInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &platformInfoPtr);
    if (platformInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_platform_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*get the usual plaform info stuff*/
    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_PLATFORM_INFO, //Attribute
                                     FBE_ENCL_PLATFORM, // component type
                                     0, // comonent index
                                     sizeof(fbe_board_mgmt_platform_info_t), // buffer length
                                     (fbe_u8_t *)&platformInfoPtr->hw_platform_info); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Platform Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

	/*and then fill up if we are in a simulation mode or not, this is used mainly by PSL guys*/
	platformInfoPtr->is_simulation = fbe_base_board_is_simulated_hw();
	platformInfoPtr->is_single_sp_system = base_board->isSingleSpSystem;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_platform_info

/*!*************************************************************************
* @fn base_board_get_suitcase_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Suitcase info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   18-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_suitcase_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_board_mgmt_suitcase_info_t          *suitcaseInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_suitcase_sub_info_t             peSuitcaseInfo = {0};
    edal_pe_suitcase_sub_info_t             *pPeSuitcaseInfo = &peSuitcaseInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &suitcaseInfoPtr);
    if (suitcaseInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_suitcase_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_SUITCASE, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(suitcaseInfoPtr->associatedSp < SP_ID_MAX)
    {
        if(suitcaseInfoPtr->suitcaseID < component_count)
        {
            component_index = (suitcaseInfoPtr->associatedSp == SP_A) ? suitcaseInfoPtr->suitcaseID : 
	                                    (suitcaseInfoPtr->suitcaseID + (component_count/2));
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, associatedSp %d, suitcaseCnt %d, Invalid suitcaseID - %d.\n",
                            __FUNCTION__,
                            suitcaseInfoPtr->associatedSp,
                            component_count,
                            suitcaseInfoPtr->suitcaseID);
        
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;  
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid SP - %d.\n",
                        __FUNCTION__,
                        suitcaseInfoPtr->associatedSp);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_SUITCASE_INFO, //Attribute
                                     FBE_ENCL_SUITCASE, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_suitcase_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeSuitcaseInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Suitcase Info, component_index %d\n",
                        __FUNCTION__, component_index);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if(( pPeSuitcaseInfo->externalSuitcaseInfo.associatedSp != suitcaseInfoPtr->associatedSp) ||
        (pPeSuitcaseInfo->externalSuitcaseInfo.suitcaseID != suitcaseInfoPtr->suitcaseID))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Rqstd: %d(SP), %d(ID), Rcvd: %d(SP), %d(ID).\n",
                        __FUNCTION__,
                        pPeSuitcaseInfo->externalSuitcaseInfo.associatedSp,
                        pPeSuitcaseInfo->externalSuitcaseInfo.suitcaseID,
                        suitcaseInfoPtr->associatedSp,
                        suitcaseInfoPtr->suitcaseID);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(suitcaseInfoPtr, &pPeSuitcaseInfo->externalSuitcaseInfo, sizeof(fbe_board_mgmt_suitcase_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_suitcase_info

/*!*************************************************************************
* @fn base_board_get_cache_card_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get cache_card cache card info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   25-Jan-2013     Rui Chang - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_cache_card_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_board_mgmt_cache_card_info_t          *cacheCardInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_cache_card_sub_info_t             peCacheCardInfo = {0};
    edal_pe_cache_card_sub_info_t             *pPeCacheCardInfo = &peCacheCardInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &cacheCardInfoPtr);
    if (cacheCardInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_cache_card_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_CACHE_CARD, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(cacheCardInfoPtr->cacheCardID < component_count)
    {
        component_index = cacheCardInfoPtr->cacheCardID;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid cacheCardID - %d.\n",
                        __FUNCTION__,
                        cacheCardInfoPtr->cacheCardID);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_CACHE_CARD_INFO, //Attribute
                                     FBE_ENCL_CACHE_CARD, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_cache_card_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeCacheCardInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Cache Card Info, component_index %d\n",
                        __FUNCTION__, component_index);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if(pPeCacheCardInfo->externalCacheCardInfo.cacheCardID != cacheCardInfoPtr->cacheCardID)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Rqstd: %d(ID), Rcvd: %d(ID).\n",
                        __FUNCTION__,
                        pPeCacheCardInfo->externalCacheCardInfo.cacheCardID,
                        cacheCardInfoPtr->cacheCardID);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(cacheCardInfoPtr, &pPeCacheCardInfo->externalCacheCardInfo, sizeof(fbe_board_mgmt_cache_card_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_cache_card_info

/*!*************************************************************************
* @fn base_board_get_ssd_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get SSD info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   15-Oct-2014     bphilbin - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_ssd_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_board_mgmt_ssd_info_t          *ssdInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_board_mgmt_ssd_info_t             peSSDInfo = {0};
    fbe_board_mgmt_ssd_info_t             *pPeSSDInfo = &peSSDInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &ssdInfoPtr);
    if (ssdInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_ssd_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_SSD, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(ssdInfoPtr->slot < component_count)
    {
        component_index = ssdInfoPtr->slot;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid slot - %d.\n",
                        __FUNCTION__,
                        ssdInfoPtr->slot);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_SSD_INFO, //Attribute
                                     FBE_ENCL_SSD, // component type
                                     component_index, // comonent index
                                     sizeof(fbe_board_mgmt_ssd_info_t), // buffer length
                                     (fbe_u8_t *)pPeSSDInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get SSD Info, component_index %d\n",
                        __FUNCTION__, component_index);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

   
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(ssdInfoPtr, pPeSSDInfo, sizeof(fbe_board_mgmt_ssd_info_t));

    fbe_base_object_trace((fbe_base_object_t *)base_board,  
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Setting SSD:%d SpBlkCnt:%d PctLifeUsd:%d SlfTstPssd:%d\n",
                           __FUNCTION__, 
                          component_index,
                          ssdInfoPtr->remainingSpareBlkCount, 
                          ssdInfoPtr->ssdLifeUsed,
                          ssdInfoPtr->ssdSelfTestPassed);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_ssd_info

/*!*************************************************************************
* @fn base_board_get_dimm_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get DIMM info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   15-May-2013     Rui Chang - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_dimm_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_board_mgmt_dimm_info_t          *dimmInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_dimm_sub_info_t             peDimmInfo = {0};
    edal_pe_dimm_sub_info_t             *pPeDimmInfo = &peDimmInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &dimmInfoPtr);
    if (dimmInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_dimm_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_DIMM, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(dimmInfoPtr->dimmID < component_count)
    {
        component_index = dimmInfoPtr->dimmID;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid dimmID - %d.\n",
                        __FUNCTION__,
                        dimmInfoPtr->dimmID);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_DIMM_INFO, //Attribute
                                     FBE_ENCL_DIMM, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_dimm_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeDimmInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Cache Card Info, component_index %d\n",
                        __FUNCTION__, component_index);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if(pPeDimmInfo->externalDimmInfo.dimmID != dimmInfoPtr->dimmID)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Rqstd: %d(ID), Rcvd: %d(ID).\n",
                        __FUNCTION__,
                        pPeDimmInfo->externalDimmInfo.dimmID,
                        dimmInfoPtr->dimmID);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(dimmInfoPtr, &pPeDimmInfo->externalDimmInfo, sizeof(fbe_board_mgmt_dimm_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_dimm_info


/*!*************************************************************************
* @fn base_board_get_bmc_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get BMC info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   11-Sep-2012     Eric Zhou - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_bmc_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_board_mgmt_bmc_info_t               *bmcInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_bmc_sub_info_t                  peBmcInfo = {0};
    edal_pe_bmc_sub_info_t                  *pPeBmcInfo = &peBmcInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &bmcInfoPtr);
    if (bmcInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_bmc_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_BMC, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(bmcInfoPtr->associatedSp < SP_ID_MAX)
    {
        if(bmcInfoPtr->bmcID < component_count)
        {
            component_index = (bmcInfoPtr->associatedSp == SP_A) ? bmcInfoPtr->bmcID : 
                                        (bmcInfoPtr->bmcID + (component_count/2));
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, Invalid ID - %d.\n",
                            __FUNCTION__,
                            bmcInfoPtr->bmcID);
        
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;  
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid SP - %d.\n",
                        __FUNCTION__,
                        bmcInfoPtr->associatedSp);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_BMC_INFO, //Attribute
                                     FBE_ENCL_BMC, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_bmc_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeBmcInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get BMC Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if((pPeBmcInfo->externalBmcInfo.associatedSp != bmcInfoPtr->associatedSp) ||
        (pPeBmcInfo->externalBmcInfo.bmcID != bmcInfoPtr->bmcID))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Rqstd: %d(SP), %d(ID), Rcvd: %d(SP), %d(ID).\n",
                        __FUNCTION__,
                        pPeBmcInfo->externalBmcInfo.associatedSp,
                        pPeBmcInfo->externalBmcInfo.bmcID,
                        bmcInfoPtr->associatedSp,
                        bmcInfoPtr->bmcID);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(bmcInfoPtr, &pPeBmcInfo->externalBmcInfo, sizeof(fbe_board_mgmt_bmc_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_bmc_info

/*!*************************************************************************
* @fn base_board_get_fltexp_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Fault Exp info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   18-May-2010     Arun S - Created.
*   17-July-2012    Chengkai Hu - Add fault register support.
***************************************************************************/
static fbe_status_t 
base_board_get_fltexp_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index = 0;
    fbe_u32_t                               component_count = 0;
    fbe_enclosure_component_types_t         component_type;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_peer_boot_flt_exp_info_t           *fltexpInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_flt_exp_sub_info_t              peFltExpInfo = {0};
    edal_pe_flt_exp_sub_info_t              *pPeFltExpInfo = &peFltExpInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &fltexpInfoPtr);
    if (fltexpInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_peer_boot_flt_exp_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (FBE_DEVICE_TYPE_FLT_REG == fltexpInfoPtr->fltHwType)
    {
        component_type = FBE_ENCL_FLT_REG;

    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "base_board_get_fltexp_info: unknow control code\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     component_type, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(component_count == 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, SP:%d, FltID:%d, HW:0x%llx. Component Count is Zero.\n",
                            __FUNCTION__,
                            fltexpInfoPtr->associatedSp,
                            fltexpInfoPtr->fltExpID,
                            fltexpInfoPtr->fltHwType);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    if(fltexpInfoPtr->associatedSp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid SP - %d.\n",
                        __FUNCTION__,
                        fltexpInfoPtr->associatedSp);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    while(component_index < component_count)
    {

        edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                         FBE_ENCL_PE_FLT_REG_INFO, //Attribute
                                         component_type, // component type
                                         component_index, // comonent index
                                         sizeof(edal_pe_flt_exp_sub_info_t), // buffer length
                                         (fbe_u8_t *)pPeFltExpInfo); // buffer pointer 
        
        if(edal_status != FBE_EDAL_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, Could not get Flt Exp Info.\n",
                            __FUNCTION__);
        
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;  
        }    
    
        if(( pPeFltExpInfo->externalFltExpInfo.associatedSp == fltexpInfoPtr->associatedSp) &&
            (pPeFltExpInfo->externalFltExpInfo.fltExpID == fltexpInfoPtr->fltExpID))
        {
            break;
        }
        component_index++;
    }
    
    if(component_index == component_count)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Failed get: SP:%d, FltID:%d, HW:0x%llx. CompIndex:%d,CompCnt:%d.\n",
                        __FUNCTION__,
                        fltexpInfoPtr->associatedSp,
                        fltexpInfoPtr->fltExpID,
                        fltexpInfoPtr->fltHwType,
                        component_index,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }


    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(fltexpInfoPtr, &pPeFltExpInfo->externalFltExpInfo, sizeof(fbe_peer_boot_flt_exp_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_fltexp_info


/*!*************************************************************************
* @fn base_board_get_slaveport_info(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions services the Control Code to get Slave Port info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   18-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_slaveport_info(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                               component_index;
    fbe_u32_t                               component_count;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_board_mgmt_slave_port_info_t        *slavePortInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    edal_pe_slave_port_sub_info_t           peSlavePortInfo = {0};
    edal_pe_slave_port_sub_info_t           *pPeSlavePortInfo = &peSlavePortInfo;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &slavePortInfoPtr);
    if (slavePortInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_slave_port_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                     FBE_ENCL_SLAVE_PORT, 
                                                     &component_count);

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Component Count is Invalid - %d.\n",
                        __FUNCTION__,
                        component_count);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if(slavePortInfoPtr->associatedSp < SP_ID_MAX)
    {
        if(slavePortInfoPtr->slavePortID < component_count)
        {
            component_index = (slavePortInfoPtr->associatedSp == SP_A) ? slavePortInfoPtr->slavePortID : 
	                                    (slavePortInfoPtr->slavePortID + (component_count/2));
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, Invalid ID - %d.\n",
                            __FUNCTION__,
                            slavePortInfoPtr->slavePortID);
        
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;  
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid SP - %d.\n",
                        __FUNCTION__,
                        slavePortInfoPtr->associatedSp);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_SLAVE_PORT_INFO, //Attribute
                                     FBE_ENCL_SLAVE_PORT, // component type
                                     component_index, // comonent index
                                     sizeof(edal_pe_slave_port_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeSlavePortInfo); // buffer pointer 

    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get Flt Exp Info.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if(( pPeSlavePortInfo->externalSlavePortInfo.associatedSp != slavePortInfoPtr->associatedSp) ||
        (pPeSlavePortInfo->externalSlavePortInfo.slavePortID != slavePortInfoPtr->slavePortID))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Rqstd: %d(SP), %d(ID), Rcvd: %d(SP), %d(ID).\n",
                        __FUNCTION__,
                        pPeSlavePortInfo->externalSlavePortInfo.associatedSp,
                        pPeSlavePortInfo->externalSlavePortInfo.slavePortID,
                        slavePortInfoPtr->associatedSp,
                        slavePortInfoPtr->slavePortID);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set the control buffer with the Info we got from EDAL */
    fbe_copy_memory(slavePortInfoPtr, &pPeSlavePortInfo->externalSlavePortInfo, sizeof(fbe_board_mgmt_slave_port_info_t));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_get_slaveport_info

/*!*************************************************************************
* @fn base_board_get_component_count(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet,                  
*                    fbe_u32_t component_type) 
***************************************************************************
*
* @brief
*       This functions services the Control Code to get component count.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
* @param      component_type - Type of the component looking for the count.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   06-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_component_count(fbe_base_board_t * base_board, fbe_packet_t * packet, fbe_u32_t component_type)
{
    fbe_u32_t                               *componentCountPtr;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_u32_t                               spsCountPerBlade = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &componentCountPtr);
    if (componentCountPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the edal is there */
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the total count of the a specificed component type. */
    switch (component_type)
    {
        case FBE_ENCL_BMC:
        case FBE_ENCL_MISC:
        case FBE_ENCL_IO_PORT:
        case FBE_ENCL_FLT_REG:
        case FBE_ENCL_PLATFORM:
        case FBE_ENCL_SUITCASE:
        case FBE_ENCL_BEM:
        case FBE_ENCL_IO_MODULE:
        case FBE_ENCL_MGMT_MODULE:
        case FBE_ENCL_MEZZANINE:
        case FBE_ENCL_SLAVE_PORT:
        case FBE_ENCL_POWER_SUPPLY:
        case FBE_ENCL_COOLING_COMPONENT:
        case FBE_ENCL_BATTERY:
        case FBE_ENCL_CACHE_CARD:
        case FBE_ENCL_DIMM:
        case FBE_ENCL_SSD:
            edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                             component_type, 
                                                             componentCountPtr);
            if(edal_status != FBE_EDAL_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, failed to get componentCount for component %d.\n",
                                __FUNCTION__, component_type);

                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_ENCL_SPS:
            edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                             component_type, 
                                                             &spsCountPerBlade);
            if(edal_status != FBE_EDAL_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, failed to get componentCount for SPS.\n",
                                __FUNCTION__);

                status = FBE_STATUS_GENERIC_FAILURE;
            }
            *componentCountPtr = spsCountPerBlade * SP_ID_MAX;
            break;

        default:
            *componentCountPtr = 0;
            status = FBE_STATUS_OK;
            break;
    }

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, 0);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_get_component_count

/*!*************************************************************************
* @fn base_board_get_component_count_per_blade(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet,                  
*                    fbe_u32_t component_type) 
***************************************************************************
*
* @brief
*       This functions services the Control Code to get local component count.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
* @param      component_type - Type of the component looking for the count.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   14-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_component_count_per_blade(fbe_base_board_t * base_board, fbe_packet_t * packet, fbe_u32_t component_type)
{
    fbe_u32_t                               component_count;
    fbe_u32_t                               *component_count_per_blade;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_edal_status_t                       edal_status = FBE_EDAL_STATUS_OK;
    fbe_edal_block_handle_t                 edal_block_handle = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &component_count_per_blade);
    if (component_count_per_blade == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the edal is there */
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the total count of the a specificed component type. */
    switch (component_type)
    {
        // handle io port count separately as edal does not return the expected value currently.
        // Need to fix this issue.
        case FBE_ENCL_IO_PORT:
            *component_count_per_blade = NUM_IO_PORTS_PER_BLADE;
            status = FBE_STATUS_OK;
            break;

        case FBE_ENCL_BMC:
        case FBE_ENCL_FLT_REG:
        case FBE_ENCL_SUITCASE:
        case FBE_ENCL_BEM:
        case FBE_ENCL_IO_MODULE:
        case FBE_ENCL_MGMT_MODULE:
        case FBE_ENCL_MEZZANINE:
        case FBE_ENCL_SLAVE_PORT:
        case FBE_ENCL_POWER_SUPPLY:
        case FBE_ENCL_COOLING_COMPONENT:
            edal_status = fbe_edal_getSpecificComponentCount(edal_block_handle, 
                                                             component_type, 
                                                             &component_count);

            if(edal_status != FBE_EDAL_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, Invalid count: %d for component: %d.\n",
                                __FUNCTION__, component_count, component_type);

                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                *component_count_per_blade = component_count/2;
                status = FBE_STATUS_OK;
            }
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, Invalid component type: %d.\n",
                            __FUNCTION__, component_type);
    
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_get_component_count_per_blade

/*!*************************************************************************
* @fn base_board_set_spFaultLED(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the Fault LED for the SP in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   05-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_spFaultLED(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_status_t                            packet_status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_sp_fault_LED_t       *spFaultLEDPtr;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &spFaultLEDPtr);
    if (spFaultLEDPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_sp_fault_LED_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_spFaultLED(spFaultLEDPtr->blink_rate, spFaultLEDPtr->status_condition);

    if(status != FBE_STATUS_OK)
    {
        packet_status = status;
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, SP Fault LED is not SET. Status: %d.\n",
                              __FUNCTION__, status); 
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, SP Fault LED updated successfully, blink %d, condition %d\n",
                              __FUNCTION__, spFaultLEDPtr->blink_rate, spFaultLEDPtr->status_condition); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    
    return status;
}   // end of base_board_set_spFaultLED

/*!*************************************************************************
* @fn base_board_set_enclFaultLED(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet,
*                    fbe_base_board_led_t component) 
***************************************************************************
*
* @brief
*       This functions sets the Fault LED for the Enclosure in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   07-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_enclFaultLED(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_api_board_enclFaultLedInfo_t        *enclFaultLedInfo;
    fbe_edal_block_handle_t                 edalBlockHandle = NULL;   
    fbe_board_mgmt_misc_info_t              peMiscSubInfo = {0};
    fbe_board_mgmt_misc_info_t              * pPeMiscSubInfo = &peMiscSubInfo; 
    fbe_edal_status_t                       edalStatus = FBE_EDAL_STATUS_OK;
    fbe_bool_t                              ledActionNeeded = FBE_FALSE;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &enclFaultLedInfo);
    if (enclFaultLedInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: fbe_payload_control_get_buffer failed\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_api_board_enclFaultLedInfo_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s: %d len invalid\n", 
                              __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    fbe_zero_memory(pPeMiscSubInfo, sizeof(fbe_board_mgmt_misc_info_t));        
    /* Get old Misc info from EDAL. */
    edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                    FBE_ENCL_PE_MISC_INFO,  //Attribute
                                    FBE_ENCL_MISC,  // component type
                                    0,  // component index
                                    sizeof(fbe_board_mgmt_misc_info_t),  // buffer length
                                    (fbe_u8_t *)pPeMiscSubInfo);  // buffer pointer

    if(!EDAL_STAT_OK(edalStatus))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    if (enclFaultLedInfo->blink_rate == LED_BLINK_ON)
    {
        pPeMiscSubInfo->enclFaultLedReason |= enclFaultLedInfo->enclFaultLedReason;
        ledActionNeeded = FBE_TRUE;
    }
    else if (enclFaultLedInfo->blink_rate == LED_BLINK_OFF)
    {
        pPeMiscSubInfo->enclFaultLedReason &= ~(enclFaultLedInfo->enclFaultLedReason);

        // if no reason for Fault LED to be on, turn it off
        if (pPeMiscSubInfo->enclFaultLedReason == FBE_ENCL_FAULT_LED_NO_FLT)
        {
            ledActionNeeded = FBE_TRUE;
        }
    }

    if (ledActionNeeded == FBE_TRUE)
    {
        status = fbe_base_board_set_enclFaultLED (enclFaultLedInfo->blink_rate);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, LED not SET. Status: 0x%x.\n",
                                  __FUNCTION__, status); 
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return status;  
        }
    }
    
    /* Update EDAL with new Misc info. */
    edalStatus = fbe_edal_setBuffer(edalBlockHandle, 
                                        FBE_ENCL_PE_MISC_INFO,  //Attribute
                                        FBE_ENCL_MISC,  // component type
                                        0,  // component index
                                        sizeof(fbe_board_mgmt_misc_info_t),  // buffer length
                                        (fbe_u8_t *)pPeMiscSubInfo);  // buffer pointer
    

    if(!EDAL_STAT_OK(edalStatus))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_enclFaultLED


/*!*************************************************************************
* @fn base_board_set_unsafeToRemoveLED(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet,
*                    fbe_base_board_led_t component) 
***************************************************************************
*
* @brief
*       This functions sets the Fault LED for the Enclosure in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   07-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_unsafeToRemoveLED(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    LED_BLINK_RATE                          *blink_rate;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &blink_rate);
    if (blink_rate == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: fbe_payload_control_get_buffer failed\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(LED_BLINK_RATE))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s: %X len invalid\n", 
                              __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_UnsafetoRemoveLED (*blink_rate);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, LED not SET. Status: 0x%x.\n",
                              __FUNCTION__, status); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_unsafeToRemoveLED

/*!*************************************************************************
* @fn base_board_set_iom_fault_LED(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the Fault LED for the IO Module in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_iom_fault_LED(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_iom_fault_LED_t      *iomFaultLEDPtr;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &iomFaultLEDPtr);
    if (iomFaultLEDPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_iom_fault_LED_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Hack to allow NDU of changes to single array of IO devices in SPECL
    if ((iomFaultLEDPtr->device_type == FBE_DEVICE_TYPE_MEZZANINE) ||
        (iomFaultLEDPtr->device_type == FBE_DEVICE_TYPE_BACK_END_MODULE))
    {
        iomFaultLEDPtr->slot += (base_board->localIoModuleCount - base_board->localAltIoDevCount);
    }

    status = fbe_base_board_set_iomFaultLED(iomFaultLEDPtr->sp_id,
                                            iomFaultLEDPtr->slot,
                                            iomFaultLEDPtr->blink_rate);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Fault LED is not SET for IO Module %d. Status: %d.\n",
                              __FUNCTION__, iomFaultLEDPtr->slot, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_iom_fault_LED

/*!*************************************************************************
* @fn base_board_set_iom_port_LED(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the Port LED for the IO Module in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_iom_port_LED(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_iom_port_LED_t       *iomPortLEDPtr;
    fbe_board_mgmt_set_iom_fault_LED_t      *iom_ptr;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &iomPortLEDPtr);
    if (iomPortLEDPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_iom_port_LED_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    iom_ptr = &iomPortLEDPtr->iom_LED;

    //Hack to allow NDU of changes to single array of IO devices in SPECL
    if ((iom_ptr->device_type == FBE_DEVICE_TYPE_MEZZANINE) ||
        (iom_ptr->device_type == FBE_DEVICE_TYPE_BACK_END_MODULE))
    {
        iom_ptr->slot += (base_board->localIoModuleCount - base_board->localAltIoDevCount);
    }

    status = fbe_base_board_set_iom_port_LED(iom_ptr->sp_id, 
                                             iom_ptr->slot,
                                             iom_ptr->blink_rate,
                                             iomPortLEDPtr->io_port,
                                             iomPortLEDPtr->led_color);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, IOM %d, Port %d, LED is not SET. status 0x%x\n",
                              __FUNCTION__, 
                              iom_ptr->slot,
                              iomPortLEDPtr->io_port, 
                              status); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_iom_port_LED

/*!*************************************************************************
* @fn base_board_set_mgmt_module_vlan_config_mode(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the Mgmt module vlan config mode in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_mgmt_module_vlan_config_mode(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_mgmt_vlan_mode_t     *mgmt_vlan_mode;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mgmt_vlan_mode);
    if (mgmt_vlan_mode == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_mgmt_vlan_mode_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_mgmt_vlan_config_mode(mgmt_vlan_mode->sp_id, 
                                                      mgmt_vlan_mode->vlan_config_mode);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Vlan Config mode %d is not set. Status: %d.\n",
                              __FUNCTION__, mgmt_vlan_mode->vlan_config_mode, status); 
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
    }
    else
    {
        /*
         * add the packet to the queue of outstanding commands
         * to be checked for successful completion later.
         */
        fbe_base_board_add_command_element(base_board, packet);
        status = FBE_STATUS_PENDING;
    }
    return status;

}   // end of base_board_set_mgmt_vlan_config_mode

/*!*************************************************************************
* @fn base_board_set_mgmt_module_fault_LED(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the Mgmt module fault LED in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_mgmt_module_fault_LED(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_mgmt_fault_LED_t     *mgmt_fault_led;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mgmt_fault_led);
    if (mgmt_fault_led == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_mgmt_fault_LED_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_mgmt_fault_LED(mgmt_fault_led->sp_id, 
                                               mgmt_fault_led->blink_rate);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Fault LED %d is not set for Mgmt module - SP: %d. Status: %d.\n",
                              __FUNCTION__, 
                              mgmt_fault_led->blink_rate, mgmt_fault_led->sp_id, status); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_mgmt_fault_LED

/*!*************************************************************************
* @fn base_board_set_mgmt_module_port(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the Mgmt module fault LED in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_mgmt_module_port(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_mgmt_port_t          *mgmt_port;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &mgmt_port);
    if (mgmt_port == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_mgmt_port_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_mgmt_port(base_board, mgmt_port->sp_id, 
                                          mgmt_port->portIDType,
                                          mgmt_port->mgmtPortConfig.mgmtPortAutoNeg,
                                          mgmt_port->mgmtPortConfig.mgmtPortSpeed,
                                          mgmt_port->mgmtPortConfig.mgmtPortDuplex);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Mgmt Port %d is not set for Mgmt module - SP: %d. Status: %d.\n",
                              __FUNCTION__, 
                              mgmt_port->portIDType, mgmt_port->sp_id, status); 
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet); 
    }
    else
    {
        /*
         * add the packet to the queue of outstanding commands
         * to be checked for successful completion later.
         */
        fbe_base_board_add_command_element(base_board, packet);
        status = FBE_STATUS_PENDING;
    }

    
    return status;

}   // end of base_board_set_mgmt_port

/*!*************************************************************************
* @fn base_board_set_PostAndOrReset(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the Post and Reset status in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_PostAndOrReset(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_PostAndOrReset_t     *post_reset;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &post_reset);
    if (post_reset == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_PostAndOrReset_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    post_reset->retryStatusCount = POST_AND_OR_RESET_STATUS_RETRY_COUNT;

    status = fbe_base_board_set_PostAndOrReset(post_reset->sp_id, 
                                               post_reset->holdInPost,
                                               post_reset->holdInReset,
                                               post_reset->rebootBlade);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Post Reset bits are NOT set for - SP: %d.\n",
                              __FUNCTION__, post_reset->sp_id); 
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet); 
    }
    else
    {
        /*
         * add the packet to the queue of outstanding commands
         * to be checked for successful completion later.
         */
        fbe_base_board_add_command_element(base_board, packet);
        status = FBE_STATUS_PENDING;
    }

    return status;

}   // end of base_board_set_PostAndOrReset

/*!*************************************************************************
* @fn base_board_clear_fltreg_sticky_fault(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*  This functions services the Control Code to clear peer sticky fault in Fault register.
*  Sticky fault is set by peer BIOS/POST and would not be cleared even while the failure 
*  FRU is replaced. This is BIOS/POST restriction, and need ESP clear them from peer side.
*  While the faulted SP comes into OS, it means the failure is fixed and related fault should
*  be removed from FSR.(Fault Status Register).
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   12-Nov-2012     Chengkai Hu - Created.
***************************************************************************/
static fbe_status_t 
base_board_clear_fltreg_sticky_fault(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_payload_ex_t                       *payload = NULL;
    fbe_peer_boot_flt_exp_info_t           *fltregInfoPtr = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_control_operation_t         *control_operation = NULL;
    SPECL_STATUS                            specl_status;
    fbe_u32_t                               eachFault = 0;
    SPECL_FAULT_REGISTER_STATUS             *faultRegPtr = NULL;
    
    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &fltregInfoPtr);
    if (fltregInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_peer_boot_flt_exp_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (FBE_DEVICE_TYPE_FLT_REG != fltregInfoPtr->fltHwType)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "wrong fault hw type.\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    faultRegPtr = &fltregInfoPtr->faultRegisterStatus;
    
    /* Call SPECL API to clear sticky the faults in the Fault Status Register (FSR).*/
    for (eachFault = 0; eachFault < MAX_DIMM_FAULT; eachFault++)
    {
        if(faultRegPtr->dimmFault[eachFault])
        {
            specl_status = speclClearBMCFaults(fltregInfoPtr->associatedSp, IPMI_FSR_FRU_DIMM);
            
            if(specl_status != EMCPAL_STATUS_SUCCESS)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, PBL: Could not clear DIMM sticky fault.\n",
                                __FUNCTION__);
            
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;  
            }    
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: Clearing DIMM sticky fault from peer fault register.\n");
        }
    }
    
    for (eachFault = 0; eachFault < MAX_DRIVE_FAULT; eachFault++)
    {
        if(faultRegPtr->driveFault[eachFault])
        {
            specl_status = speclClearBMCFaults(fltregInfoPtr->associatedSp, IPMI_FSR_FRU_DRIVE);
            
            if(specl_status != EMCPAL_STATUS_SUCCESS)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, PBL: Could not clear DRIVE sticky fault.\n",
                                __FUNCTION__);
            
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;  
            }  
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: Clearing DRIVE sticky fault from peer fault register.\n");
        }
    }

    for (eachFault = 0; eachFault < MAX_SLIC_FAULT; eachFault++)
    {
        if(faultRegPtr->slicFault[eachFault])
        {
            specl_status = speclClearBMCFaults(fltregInfoPtr->associatedSp, IPMI_FSR_FRU_SLIC);
            
            if(specl_status != EMCPAL_STATUS_SUCCESS)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, PBL: Could not clear SLIC sticky fault.\n",
                                __FUNCTION__);
            
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;  
            }  
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: Clearing SLIC sticky fault from peer fault register.\n");
        }
    }

    for (eachFault = 0; eachFault < MAX_I2C_FAULT; eachFault++)
    {
        if(faultRegPtr->i2cFault[eachFault])
        {
            specl_status = speclClearBMCFaults(fltregInfoPtr->associatedSp, IPMI_FSR_FRU_I2C);
            
            if(specl_status != EMCPAL_STATUS_SUCCESS)
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s, PBL: Could not clear I2C sticky fault.\n",
                                __FUNCTION__);
            
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;  
            }  
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: Clearing I2C sticky fault from peer fault register.\n");
        }
    }

    specl_status = speclClearBMCFaults(fltregInfoPtr->associatedSp, IPMI_FSR_FRU_SYSMISC);
    
    if(specl_status != EMCPAL_STATUS_SUCCESS)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, PBL: Could not clear SYSMISC sticky fault.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }  
    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "PBL: Clearing SYSMISC sticky fault from peer fault register.\n"); 
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}   // end of base_board_clear_fltreg_sticky_fault

/*!*************************************************************************
* @fn base_board_set_FlushFilesAndReg(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions requests to flush file and registry IO to disk.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   28-Dec-2010     Brion P - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_FlushFilesAndReg(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_FlushFilesAndReg_t   *flushOptions;
    fbe_payload_control_buffer_length_t     len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &flushOptions);
    if (flushOptions == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_FlushFilesAndReg_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_FlushFilesAndReg(flushOptions->sp_id);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Flush Files and Reg not completed for - SP: %d.\n",
                              __FUNCTION__, flushOptions->sp_id); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}


/*!*************************************************************************
* @fn base_board_set_resume(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the resume prom for a SMB device in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_resume(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_resume_t             *set_resume;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &set_resume);
    if (set_resume == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_resume_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_resume(set_resume->device, &set_resume->resume_prom);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Resume Prom is not set for SMB Device: %d.\n",
                              __FUNCTION__, set_resume->device); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_resume

/*!*************************************************************************
* @fn base_board_resume_write(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions write the resume prom for a SMB device in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   21-Jul-2011     PHE - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_resume_write(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_resume_prom_cmd_t                   *pWriteResumeCmd = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    SPECL_RESUME_DATA                       *pSpeclResumeData = NULL;
    fbe_u8_t                                *pdst = NULL;
    fbe_u32_t                               checksum = 0;
    SMB_DEVICE                              smb_device;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pWriteResumeCmd);
    if (pWriteResumeCmd == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n", 
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_resume_prom_cmd_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__,len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the SMB device by passing in the sp, slot and device_type */
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       pWriteResumeCmd->deviceType,
                                                       &pWriteResumeCmd->deviceLocation,  
                                                       &smb_device);


    pSpeclResumeData = fbe_memory_ex_allocate(sizeof(SPECL_RESUME_DATA));
    if(pSpeclResumeData == NULL) 
    {
        fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(pSpeclResumeData, sizeof(SPECL_RESUME_DATA));

    fbe_base_board_get_resume(smb_device, pSpeclResumeData);

    if((pSpeclResumeData->transactionStatus == EMCPAL_STATUS_SUCCESS) ||
       (pSpeclResumeData->transactionStatus == STATUS_SPECL_INVALID_CHECKSUM))
    {
        pdst = (fbe_u8_t *)(&pSpeclResumeData->resumePromData) + pWriteResumeCmd->offset;

        if (pWriteResumeCmd->pBuffer != NULL)
        {
            fbe_copy_memory(pdst,
                            pWriteResumeCmd->pBuffer,
                            pWriteResumeCmd->bufferSize);
        }

        // calculate the new checksum over the resume data
        checksum = calculateResumeChecksum(&pSpeclResumeData->resumePromData);

        // update the checksum value in resume data going to specl
        pSpeclResumeData->resumePromData.resume_prom_checksum = checksum;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, resume prom read fail, status: 0x%x.\n",
                          __FUNCTION__, pSpeclResumeData->transactionStatus);
        fbe_memory_ex_release(pSpeclResumeData);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_set_resume(smb_device, &pSpeclResumeData->resumePromData);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                    FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, Resume Prom is not set for SMB Device: %d.\n",
                    __FUNCTION__, smb_device);
    }
    else
    {
        /*
         * add the packet to the queue of outstanding commands
         * to be checked for successful completion later.
         */
        fbe_base_board_add_command_element(base_board, packet);
        status = FBE_STATUS_PENDING;
    }

    fbe_memory_ex_release(pSpeclResumeData);
    return status;

}   // end of base_board_resume_write

/*!*************************************************************************
* @fn base_board_resume_write_async(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions handles async write the resume prom 
*       for a SMB device in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   17-Dec-2012   Dipak Patel - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_resume_write_async(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_resume_prom_write_asynch_cmd_t      *rpWriteAsynchCmd = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    SPECL_RESUME_DATA                       *pSpeclResumeData = NULL;
    fbe_u8_t                                *pdst = NULL;
    fbe_u32_t                               checksum = 0;
    SMB_DEVICE                              smb_device;
    fbe_sg_element_t                        *sg_element = NULL;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &rpWriteAsynchCmd);
    if (rpWriteAsynchCmd == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n", 
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_resume_prom_write_asynch_cmd_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__,len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the SMB device by passing in the sp, slot and device_type */
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       rpWriteAsynchCmd->deviceType,
                                                       &rpWriteAsynchCmd->deviceLocation,  
                                                       &smb_device);


    pSpeclResumeData = fbe_memory_ex_allocate(sizeof(SPECL_RESUME_DATA));
    if(pSpeclResumeData == NULL) 
    {
        fbe_transport_set_status(packet, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(pSpeclResumeData, sizeof(SPECL_RESUME_DATA));

    fbe_base_board_get_resume(smb_device, pSpeclResumeData);

    // get the sg element and the sg count
    fbe_payload_ex_get_sg_list(payload, &sg_element, NULL);

    if (sg_element != NULL)
    {
        rpWriteAsynchCmd->pBuffer = sg_element[0].address;
        rpWriteAsynchCmd->bufferSize = sg_element[0].count;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                        FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s, SG_LIST is NULL.\n", 
                        __FUNCTION__);
        fbe_memory_ex_release(pSpeclResumeData);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if((pSpeclResumeData->transactionStatus == EMCPAL_STATUS_SUCCESS) ||
       (pSpeclResumeData->transactionStatus == STATUS_SPECL_INVALID_CHECKSUM))
    {
        pdst = (fbe_u8_t *)(&pSpeclResumeData->resumePromData) + rpWriteAsynchCmd->offset;

        if (rpWriteAsynchCmd->pBuffer != NULL)
        {
            fbe_copy_memory(pdst,
                            rpWriteAsynchCmd->pBuffer,
                            rpWriteAsynchCmd->bufferSize);
        }

        // calculate the new checksum over the resume data
        checksum = calculateResumeChecksum(&pSpeclResumeData->resumePromData);

        // update the checksum value in resume data going to specl
        pSpeclResumeData->resumePromData.resume_prom_checksum = checksum;

    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, resume prom read fail, status: 0x%x.\n",
                          __FUNCTION__, pSpeclResumeData->transactionStatus);
        fbe_memory_ex_release(pSpeclResumeData);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //kostring: the specl api does not support transformers family now, we just print a trace here.
   /* if(base_board->platformInfo.familyType == TRANSFORMERS_FAMILY)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, will not write resume prom. Specl set resume prom api not support in transformers platform.\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
    }
    else
    {*/
        status = fbe_base_board_set_resume(smb_device, &pSpeclResumeData->resumePromData);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Resume Prom is not set for SMB Device: %d.\n",
                              __FUNCTION__, smb_device); 
        }
        else
        {
            /*
             * add the packet to the queue of outstanding commands
             * to be checked for successful completion later.
             */
            fbe_base_board_add_command_element(base_board, packet);
            status = FBE_STATUS_PENDING;
        }
    //}

    fbe_memory_ex_release(pSpeclResumeData);
    return status;

}   // end of base_board_resume_write

/*!*************************************************************************
* @fn base_board_get_resume(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions gets the resume prom for a SMB device in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   20-May-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_resume_read(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    SMB_DEVICE                              smb_device;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_resume_prom_cmd_t                   *pReadResumeCmd = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    SPECL_RESUME_DATA                       specl_resume_data = {0};
    SPECL_RESUME_DATA                       *pSpecl_resume_data = &specl_resume_data;
    fbe_sg_element_t                        *sg_element = NULL;
    fbe_u8_t                                *pSrc = NULL;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pReadResumeCmd);
    if (pReadResumeCmd == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_resume_prom_cmd_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the SMB device by passing in the sp, slot and device_type */
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       pReadResumeCmd->deviceType, 
                                                       &pReadResumeCmd->deviceLocation, 
                                                       &smb_device);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "GET SMB Device failed for SP: %d, Slot: %d, Device: %s.\n",
                              pReadResumeCmd->deviceLocation.sp,
                              pReadResumeCmd->deviceLocation.slot,
                              fbe_base_board_decode_device_type(pReadResumeCmd->deviceType)); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get the sg element and the sg count
    fbe_payload_ex_get_sg_list(payload, &sg_element, NULL);

    if (sg_element != NULL)
    {
        pReadResumeCmd->pBuffer = sg_element[0].address;
        pReadResumeCmd->bufferSize = sg_element[0].count;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, SG_LIST is NULL.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Go and get the Resume information for the SMB device */
    status = fbe_base_board_get_resume(smb_device, pSpecl_resume_data);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, GET Resume Prom failed for SMB Device: %d.\n",
                              __FUNCTION__, smb_device); 
    }

    pSrc = (fbe_u8_t *)(&pSpecl_resume_data->resumePromData) + pReadResumeCmd->offset;

    /* Set the control buffer with the Info we got from SPECL to resume buffer */
    fbe_copy_memory(pReadResumeCmd->pBuffer, 
                    pSrc, 
                    pReadResumeCmd->bufferSize);

    status = fbe_base_board_pe_xlate_smb_status(pSpecl_resume_data->transactionStatus, 
                                                &pReadResumeCmd->readOpStatus,
                                                pSpecl_resume_data->retriesExhausted);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Resume Prom status Translation failed. Xaction status: %d.\n", 
                              pSpecl_resume_data->transactionStatus);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}   // end of base_board_resume_read

/*!***************************************************************
 * base_board_get_resume()
 ****************************************************************
 * @brief
*    This functions gets the resume prom for a SMB device in PE.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
 *
 * @return fbe_status_t
 *
 * @author 
 *  18-April-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t 
base_board_get_resume(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    SMB_DEVICE                              smb_device;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_get_resume_t             *get_resume;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &get_resume);
    if (get_resume == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_get_resume_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the SMB device by passing in the sp, slot and device_type */
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       get_resume->device_type, 
                                                       &get_resume->device_location,
                                                       &smb_device);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, GET SMB Device failed for SP: %d, Slot: %d, Device Type: %lld.\n",
                              __FUNCTION__, 
                              get_resume->device_location.sp,
                              get_resume->device_location.slot,
                              get_resume->device_type); 
    }

    /* Go and get the Resume information for the SMB device */
    status = fbe_base_board_get_resume(smb_device, &get_resume->resume_data);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, GET Resume Prom failed for SMB Device: %d.\n",
                              __FUNCTION__, smb_device); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   
/**************************************************
 *  end of base_board_get_resume
 *************************************************/

/*!*************************************************************************
* @fn base_board_iom_get_IoCompInfo(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet,
*                    fbe_enclosure_component_types_t comp_type)                  
***************************************************************************
*
* @brief
*       This function services the Control Code to return the io component info.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
* @param      comp_type - component type whether io module, io annex or mezzanine
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   10-May-2010     Nayana Chaudhari - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_iom_get_IoCompInfo(fbe_base_board_t * base_board, fbe_packet_t * packet, 
						  fbe_enclosure_component_types_t comp_type)
{
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_edal_block_handle_t edalBlockHandle = NULL;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    fbe_status_t status = FBE_STATUS_OK;
	edal_pe_io_comp_sub_info_t peIoModuleSubInfo = {0}; 
    edal_pe_io_comp_sub_info_t *pPeIoModuleSubInfo = &peIoModuleSubInfo;  
	fbe_board_mgmt_io_comp_info_t    *iom_info_p = NULL;
	fbe_u32_t componentIndex=0, componentCount=0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_LOW, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &iom_info_p);
    if (iom_info_p == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer failed.\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_io_comp_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, get_buffer_length failed, len %d.\n", 
                              __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Make sure the edal is there */
    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);
    if (edalBlockHandle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* Get the total component count. */
	edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
					comp_type, 
					&componentCount);

	if(edalStatus != FBE_EDAL_STATUS_OK)
	{
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, Invalid Component Count: %d.\n",
                __FUNCTION__,
                componentCount);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

    if(iom_info_p->associatedSp < SP_ID_MAX)
    {
        if(iom_info_p->slotNumOnBlade < componentCount)
        {
            /* Get the component index for the IO Comp (Module/Mezzanine/Annex). */
	        componentIndex = (iom_info_p->associatedSp == SP_A) ? 
	                            iom_info_p->slotNumOnBlade : 
                                (iom_info_p->slotNumOnBlade + (componentCount/2));
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s, Slot num on Blade is Invalid - %d.\n",
                            __FUNCTION__,
                            iom_info_p->slotNumOnBlade);
        
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;  
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Invalid SP - %d.\n",
                        __FUNCTION__,
                        iom_info_p->associatedSp);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }

    edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                     FBE_ENCL_PE_IO_COMP_INFO, //Attribute
                                     comp_type, // component type
                                     componentIndex, // comonent index
                                     sizeof(edal_pe_io_comp_sub_info_t), // buffer length
                                     (fbe_u8_t *)pPeIoModuleSubInfo); // buffer pointer 

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get IO Comp (%d) Info.\n",
                        __FUNCTION__,
                        comp_type);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    /* Sanity check */
    if(( pPeIoModuleSubInfo->externalIoCompInfo.associatedSp != iom_info_p->associatedSp) ||
        (pPeIoModuleSubInfo->externalIoCompInfo.slotNumOnBlade != iom_info_p->slotNumOnBlade))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Rqstd: %d(SP), %d(Slot), Rcvd: %d(SP), %d(Slot).\n",
                        __FUNCTION__,
                        pPeIoModuleSubInfo->externalIoCompInfo.associatedSp,
                        pPeIoModuleSubInfo->externalIoCompInfo.slotNumOnBlade,
                        iom_info_p->associatedSp,
                        iom_info_p->slotNumOnBlade);
    
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* set the control buffer to what we got from edal */
	fbe_copy_memory(iom_info_p, 
		&pPeIoModuleSubInfo->externalIoCompInfo,
		sizeof(fbe_board_mgmt_io_comp_info_t));

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}// end of base_board_iom_get_IoCompInfo


/*!*************************************************************************
* @fn base_board_set_comp_max_timeout(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the max timeout value for any PE component.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   16-Aug-2010     Arun S - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_comp_max_timeout(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_u32_t                                   index = 0;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t                               *payload = NULL;
    fbe_payload_control_operation_t             *control_operation = NULL;
    fbe_base_board_mgmt_set_max_timeout_info_t  *comp_max_timeout;
    fbe_payload_control_buffer_length_t         len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &comp_max_timeout);
    if (comp_max_timeout == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_mgmt_set_max_timeout_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(index = 0; index < FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES; index ++ )
    {
        if (pe_init_data[index].component_type == comp_max_timeout->component)
        {
            pe_init_data[index].max_status_timeout = comp_max_timeout->max_timeout;
            break;
        }
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_comp_max_timeout

/*!***************************************************************
 *  base_board_get_base_board_info(fbe_base_board_t * base_board, 
 *                                fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function is used get base board info
 *
* @param  base_board - The pointer to the fbe_base_object_t.
* @param  packet - The pointer to the fbe_packet_t.
 *
 * @return  - fbe_status_t
 *
 * @author 
 *  11-Aug-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t
base_board_get_base_board_info(fbe_base_board_t * base_board, 
                               fbe_packet_t * packet)
{
    fbe_base_board_get_base_board_info_t *fbe_base_board_info_ptr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &fbe_base_board_info_ptr);
    if (fbe_base_board_info_ptr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_get_base_board_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Set the base board info
    fbe_base_board_info_ptr->number_of_io_ports = base_board->number_of_io_ports;
    fbe_base_board_info_ptr->edal_block_handle = base_board->edal_block_handle;
    fbe_base_board_info_ptr->spid = base_board->spid;
    fbe_base_board_info_ptr->localIoModuleCount = base_board->localIoModuleCount;
    fbe_base_board_info_ptr->platformInfo = base_board->platformInfo;
    fbe_base_board_info_ptr->mfgMode = base_board->mfgMode;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/**************************************************
*  end of base_board_get_base_board_info()
**************************************************/

/*!***************************************************************
 *  base_board_set_local_software_boot_status()
 ****************************************************************
 * @brief
 *  This function is used to set local software boot status
 *
* @param  base_board - The pointer to the fbe_base_object_t.
* @param  packet - The pointer to the fbe_packet_t.
 *
 * @return  - fbe_status_t
 *
 * @author 
 *  04-Aug-2011: PHE - Created
 *
 ****************************************************************/
static fbe_status_t 
base_board_set_local_software_boot_status(fbe_base_board_t * base_board, 
                                fbe_packet_t * packet)
{
    fbe_status_t    status;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_base_board_set_local_software_boot_status_t   *setLocalSoftwareBootStatus = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &setLocalSoftwareBootStatus);
    if (setLocalSoftwareBootStatus == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_set_local_software_boot_status_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_base_board_set_local_software_boot_status(setLocalSoftwareBootStatus->componentExtStatus,
                                                          setLocalSoftwareBootStatus->componentStatus,
                                                          setLocalSoftwareBootStatus->componentExtStatus);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Error in setting localSoftwareBootStatus, Status: 0x%x.\n",
                              __FUNCTION__, status);
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/**************************************************
*  end of base_board_set_local_software_boot_status()
**************************************************/

/*!***************************************************************
 *  base_board_set_local_flt_exp_status()
 ****************************************************************
 * @brief
 *  This function is used to set local fault expander status
 *
* @param  base_board - The pointer to the fbe_base_object_t.
* @param  packet - The pointer to the fbe_packet_t.
 *
 * @return  - fbe_status_t
 *
 * @author 
 *  04-Aug-2011: PHE - Created
 *
 ****************************************************************/
static fbe_status_t 
base_board_set_local_flt_exp_status(fbe_base_board_t * base_board, 
                                    fbe_packet_t * packet)
{
    fbe_status_t    status;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_u8_t                                *pFaultStatus = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pFaultStatus);
    if (pFaultStatus == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u8_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_base_board_set_local_flt_exp_status(*pFaultStatus);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Error in setting localFltExpStatus, Status: 0x%x.\n",
                              __FUNCTION__, status);
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/**************************************************
*  end of base_board_set_local_flt_exp_status()
**************************************************/

/*!***************************************************************
 *  base_board_get_eir_info(fbe_base_board_t * base_board, 
 *                          fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function is used get EIR info
 *
* @param  base_board - The pointer to the fbe_base_object_t.
* @param  packet - The pointer to the fbe_packet_t.
 *
 * @return  - fbe_status_t
 *
 * @author 
 *  29-Dec-2010: Created  Joe Perry
 *
 ****************************************************************/
static fbe_status_t
base_board_get_eir_info(fbe_base_board_t * base_board, 
                        fbe_packet_t * packet)
{
    fbe_base_board_get_eir_info_t *fbe_base_board_eir_info_ptr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    edal_pe_temperature_sub_info_t    peTempSubInfo;
    edal_pe_temperature_sub_info_t    *pPeTempSubInfo = &peTempSubInfo;
    fbe_edal_block_handle_t           edalBlockHandle = NULL;   
    fbe_u32_t           componentCount = 0;
    fbe_edal_status_t   edalStatus = FBE_EDAL_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &fbe_base_board_eir_info_ptr);
    if (fbe_base_board_eir_info_ptr == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_get_eir_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_get_edal_block_handle(base_board, &edalBlockHandle);

    /*
     * Get the InputPower info from PS's
     */
    edalStatus = fbe_edal_getSpecificComponentCount(edalBlockHandle, 
                                                     FBE_ENCL_POWER_SUPPLY, 
                                                     &componentCount);

    if(edalStatus != FBE_EDAL_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;  
    }    

    fbe_base_board_eir_info_ptr->peEirData.enclCurrentInputPower = base_board->inputPower;
    
    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, inputPower %d, status 0x%x\n", 
                          __FUNCTION__,
                          fbe_base_board_eir_info_ptr->peEirData.enclCurrentInputPower.inputPower,
                          fbe_base_board_eir_info_ptr->peEirData.enclCurrentInputPower.status);

    /*
     * Get the AirInletTemp info
     */
    edalStatus = fbe_edal_getBuffer(edalBlockHandle, 
                                    FBE_ENCL_PE_TEMPERATURE_INFO,   //Attribute
                                    FBE_ENCL_TEMPERATURE,           // component type
                                    0,                              // component index
                                    sizeof(edal_pe_temperature_sub_info_t), // buffer length
                                    (fbe_u8_t *)pPeTempSubInfo);    // buffer pointer
    if(!EDAL_STAT_OK(edalStatus))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;  
    }  
    
    fbe_base_board_eir_info_ptr->peEirData.enclCurrentAirInletTemp.status |=
        pPeTempSubInfo->externalTempInfo.status;
    if (pPeTempSubInfo->externalTempInfo.status == FBE_ENERGY_STAT_VALID)
    {
        fbe_base_board_eir_info_ptr->peEirData.enclCurrentAirInletTemp.airInletTemp =
            pPeTempSubInfo->externalTempInfo.airInletTemp;
    }
    
    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, inputPower %d, status 0x%x\n", 
                          __FUNCTION__,
                          fbe_base_board_eir_info_ptr->peEirData.enclCurrentInputPower.inputPower,
                          fbe_base_board_eir_info_ptr->peEirData.enclCurrentInputPower.status);
    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, airInletTemp %d, status 0x%x\n", 
                          __FUNCTION__,
                          fbe_base_board_eir_info_ptr->peEirData.enclCurrentAirInletTemp.airInletTemp,
                          fbe_base_board_eir_info_ptr->peEirData.enclCurrentAirInletTemp.status);
 
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/**************************************************
*  end of base_board_get_eir_info()
**************************************************/

/*!***************************************************************
 *  @fn base_board_process_firmware_op(fbe_base_board_t *base_board, 
 *                                     fbe_packet_t *packet)
 ****************************************************************
 * @brief
 *  This function is to process the firmware upgrade related operations.
 *
* @param  base_board - The pointer to the fbe_base_object_t.
* @param  packet - The pointer to the fbe_packet_t.
 *
 * @return  - fbe_status_t
 *
 * @author 
 *  29-Dec-2010: GBB - Created.
 *
 ****************************************************************/
static fbe_status_t base_board_process_firmware_op(fbe_base_board_t *base_board, fbe_packet_t *packet)
{
    fbe_enclosure_mgmt_download_op_t    *pDownloadOp = NULL;
    fbe_payload_control_buffer_length_t bufferLength = 0;
    fbe_payload_ex_t                    *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sg_element_t                    *sg_list = NULL;
    fbe_u32_t                           item_count = 0;


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pDownloadOp);
    if (pDownloadOp == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &bufferLength); 
    if (bufferLength != sizeof(fbe_enclosure_mgmt_download_op_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, bufferLength);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(pDownloadOp->op_code)
    {
    case FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD:
        fbe_payload_ex_get_sg_list(payload, &sg_list, &item_count);
        status = base_board_process_firmware_download(base_board, pDownloadOp, sg_list);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS:
        status = base_board_fetch_download_op_status(base_board, pDownloadOp);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_ABORT:
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,ABORT received, no action needed. \n",
                              __FUNCTION__);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE:
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,ACTIVATE received, no action needed. \n",
                              __FUNCTION__);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION:
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,NOTIFY_COMPLETION received, no action needed. \n",
                              __FUNCTION__);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION:
        status = base_board_process_firmware_get_local_upgrade_permission(base_board, pDownloadOp);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION:
        status = base_board_process_firmware_return_local_upgrade_permission(base_board, pDownloadOp);
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,unsupport opcode %d.\n",
                              __FUNCTION__,
                              pDownloadOp->op_code);

        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, return status %d \n",
                              __FUNCTION__, status);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

/*!**************************************************************************
* @fn base_board_process_firmware_download()                  
***************************************************************************
*
* @brief
*       Process the DOWNLOAD op.
*
* @param   base_board - The pointer to the fbe_base_board_t.
* @param   pDownloadOp - The pointer to the fbe_enclosure_mgmt_download_op_t.
* @param   sg_list - The sg list for the image.
*
* @return  fbe_status_t.
*
* NOTES
*
* @version
*   11-Oct-2012 PHE  - Created.
***************************************************************************/
static fbe_status_t
base_board_process_firmware_download(fbe_base_board_t * base_board, 
                                     fbe_enclosure_mgmt_download_op_t *pDownloadOp,
                                     fbe_sg_element_t  * sg_list)
{
    SPS_FW_TYPE                         imageType;
    fbe_u8_t                          * image_p = NULL;
    fbe_u32_t                           image_size = 0;
    fbe_status_t                        status = FBE_STATUS_OK;

    if (sg_list == NULL)
    {
        // use image pointer
        image_p = pDownloadOp->image_p;
        image_size = pDownloadOp->size;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Using sg list.\n",
                          __FUNCTION__);
        // use sg list pointer
        image_p = fbe_sg_element_address(&sg_list[0]);
        image_size = fbe_sg_element_count(&sg_list[0]);
    }

    // board specific (specl): adjust the image size and image pointer to remove the header
    image_p += pDownloadOp->header_size;
    image_size -= pDownloadOp->header_size;

    if (pDownloadOp->target == FBE_FW_TARGET_SPS_PRIMARY)
    {
        imageType = PRIMARY_FW_TYPE;
    }
    else if (pDownloadOp->target == FBE_FW_TARGET_SPS_SECONDARY)
    {
        imageType = SECONDARY_FW_TYPE;
    }
    else if (pDownloadOp->target == FBE_FW_TARGET_SPS_BATTERY)
    {
        imageType = BATTERY_PACK_FW_TYPE;
    }
    else
    {
        // target not supported
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported fw target(%d)\n",
                              __FUNCTION__,
                              pDownloadOp->target);
       
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Sending Download command ImageType:%d Size:0x%x Imzgep:%p.\n",
                          __FUNCTION__,
                          imageType,
                          image_size,
                          image_p);

    // send command to specl
    status = fbe_base_board_sps_download_command(imageType, image_p, image_size);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                    FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s, fbe_base_board_sps_download_command failed.\n",
                    __FUNCTION__);
    }

    return status;
}

/*!**************************************************************************
* @fn base_board_fetch_download_op_status()                  
***************************************************************************
*
* @brief
*       Fetch the download op status.
*
* @param   base_board - The pointer to the fbe_base_board_t.
* @param   pDownloadOp - The pointer to the fbe_enclosure_mgmt_download_op_t.
* @param   sg_list - The sg list for the image.
*
* @return  fbe_status_t.
*
* NOTES
*
* @version
*   11-Oct-2012 PHE  - Created.
***************************************************************************/
static fbe_status_t 
base_board_fetch_download_op_status(fbe_base_board_t * base_board,
                                    fbe_enclosure_mgmt_download_op_t * pDownloadOp)
{
    edal_pe_sps_sub_info_t              peSpsInfo = {0};
    fbe_edal_block_handle_t             edal_block_handle = NULL;
    fbe_edal_status_t                   edal_status = FBE_EDAL_STATUS_OK;
             
    fbe_base_board_get_edal_block_handle(base_board, &edal_block_handle);
    if (edal_block_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, EDAL not initialized.\n",
                        __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if((pDownloadOp->target != FBE_FW_TARGET_SPS_PRIMARY) &&
       (pDownloadOp->target != FBE_FW_TARGET_SPS_SECONDARY) &&
       (pDownloadOp->target != FBE_FW_TARGET_SPS_BATTERY)) 
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported fw target(%d)\n",
                              __FUNCTION__,
                              pDownloadOp->target);

        return FBE_STATUS_GENERIC_FAILURE;
    }
     
    /* Get current SPS info */
    edal_status = fbe_edal_getBuffer(edal_block_handle, 
                                     FBE_ENCL_PE_SPS_INFO,              //Attribute
                                     FBE_ENCL_SPS,                      // component type
                                     0,                                 // comonent index
                                     sizeof(edal_pe_sps_sub_info_t),    // buffer length
                                     (fbe_u8_t *)&peSpsInfo);           // buffer pointer 
    if(edal_status != FBE_EDAL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_board,
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s, Could not get SPS Info.\n",
                        __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    pDownloadOp->status =  peSpsInfo.fupStatus;
    pDownloadOp->extended_status = peSpsInfo.fupExtStatus; 
    
    return FBE_STATUS_OK;
}


/*!*************************************************************************
* @fn base_board_parse_image_header()                  
***************************************************************************
* @brief
*       This function parses the mcode image header to return the image rev and 
*       whether there is a subencl product id in the subencl product id list
*       of the image header which matches the specified subencl product id.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   22-June-2010     GBB - Created.
*   11-Oct-2012      PHE - convert the component type to fw target.
***************************************************************************/
static fbe_status_t 
base_board_parse_image_header(fbe_base_board_t *base_board,
                              fbe_packet_t *packet)
{
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;   
    fbe_payload_control_buffer_t            control_buffer = NULL;    
    fbe_payload_control_buffer_length_t     control_buffer_length = 0;
    fbe_enclosure_mgmt_parse_image_header_t *pParseImageHeader = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               byteOffset = 0;
    fbe_u8_t                                numOfSubEnclProductId = 0;
    fbe_u8_t                                index = 0;
    fbe_u8_t                                *pImageHeader = NULL;
    fbe_u32_t                               sizeTemp = 0;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {

        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_payload failed.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                              __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    pParseImageHeader = (fbe_enclosure_mgmt_parse_image_header_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_parse_image_header_t)))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_buffer_length, bufferLen %d, expectedLen: %d, status: 0x%x.\n", 
                              __FUNCTION__,
                              control_buffer_length, 
                              (int)sizeof(fbe_enclosure_mgmt_parse_image_header_t), 
                              status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* The XPE SPS and DAE SPS share the same image. The image header
     * was generated by ESES so we need to parst it following the format of ESES.
     */
    pImageHeader = &pParseImageHeader->imageHeader[0];
    numOfSubEnclProductId = pImageHeader[FBE_ESES_MCODE_NUM_SUBENCL_PRODUCT_ID_OFFSET];
    if(numOfSubEnclProductId > FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, Too many subencl prod ids act:%d, limit:%d\n",
                                __FUNCTION__,
                               numOfSubEnclProductId,
                               FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }
    /* Loop through the subencl product id list in the image header to 
     * save all the subEnclosureProductid. 
     */
    for(index = 0; index < numOfSubEnclProductId; index ++)
    {
        byteOffset = FBE_ESES_MCODE_SUBENCL_PRODUCT_ID_OFFSET + (index * FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);

        fbe_copy_memory(&pParseImageHeader->subenclProductId[index][0], 
                        &pImageHeader[byteOffset], 
                        FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
    }
    
    base_board_eses_comp_type_to_fw_target(pImageHeader[FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_OFFSET],
                                    &pParseImageHeader->firmwareTarget);
    
    // save the header size, used to find the start of the image,
    // only used for board obj but copy it here
    fbe_copy_memory((fbe_u8_t *)&sizeTemp, 
                     pImageHeader + FBE_ESES_MCODE_IMAGE_HEADER_SIZE_OFFSET,
                     FBE_ESES_MCODE_IMAGE_HEADER_SIZE_BYTES);
    pParseImageHeader->headerSize = swap32(sizeTemp);

    /* Save image rev. */
    fbe_copy_memory(&pParseImageHeader->imageRev[0], 
                    &pImageHeader[FBE_ESES_MCODE_IMAGE_REV_OFFSET], 
                    FBE_ESES_FW_REVISION_SIZE);
    
    /* Save image size. */
    fbe_copy_memory((fbe_u8_t *)&sizeTemp, 
                     pImageHeader + FBE_ESES_MCODE_IMAGE_SIZE_OFFSET,
                     FBE_ESES_MCODE_IMAGE_SIZE_BYTES);
    pParseImageHeader->imageSize = swap32(sizeTemp);

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Set the packet status. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_base_board_parse_image_header

/*!***************************************************************
 * @fn base_board_eses_comp_type_to_fw_target(ses_comp_type_enum eses_comp_type
 *                                            fbe_enclosure_fw_target_t * pFwTarget)
 ****************************************************************
 * @brief:
 *    This function converts the eses comp type to fw target.
 *    Please note the same SPS image file is used for XPE SPS firmware upgrade.    
 *
 * @param  eses_comp_type (INPUT)-
 * @param  pFwTarget (OUTPUT) -
 *
 * @return  fbe_status_t
 *         FBE_STATUS_OK  - no error.
 *         otherwise - error found.
 *
 * HISTORY:
 *    11-Oct-2012: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t base_board_eses_comp_type_to_fw_target(ses_comp_type_enum eses_comp_type,
                                                            fbe_enclosure_fw_target_t * pFwTarget)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(eses_comp_type)
    {
        case SES_COMP_TYPE_SPS_FW: 
           *pFwTarget = FBE_FW_TARGET_SPS_PRIMARY;
           break; 
              
        case SES_COMP_TYPE_SPS_SEC_FW:  
           *pFwTarget = FBE_FW_TARGET_SPS_SECONDARY;
           break;
                    
        case SES_COMP_TYPE_SPS_BAT_FW:   
           *pFwTarget = FBE_FW_TARGET_SPS_BATTERY;
           break;
                   
        default:
            *pFwTarget = FBE_FW_TARGET_INVALID;
            status = FBE_STATUS_GENERIC_FAILURE;
           break; 
    }

    return status;
}

/*!*************************************************************************
* @fn base_board_process_firmware_get_local_upgrade_permission
***************************************************************************
* @brief
*      This function gets called when board_mgmt module_mgmt and encl_mgmt
*      need to get permission to update CDES firmware. 
*      In ESP, board_mgmt and module_mgmt is in charge of upgrading enclosure 0_0 
*      on some DPE. Meanwhile encl_mgmt is doing upgrade for all other enclosures. 
*      So there is possiblity they upgrade enclosures at the same time, which 
*      will cause problems. To solve that, we use this lock to ensure only one 
*      enclosure to upgrade at one time. on DPE board_mgmt moudle_mgmt and encl_mgmt 
*      should send request to board object to race for this permission. 
*
* @param     base_board - The pointer to the base_board_t.
* @param     pFirmwareOp - The pointer to the firmware op.
*
* @return    fbe_status_t.
*
* @note
*
* @version
*   18-Apr-2013 Rui Chang - Created.
***************************************************************************/
static fbe_status_t 
base_board_process_firmware_get_local_upgrade_permission(fbe_base_board_t * base_board, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp)
{
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;
    fbe_u32_t currentTime = (fbe_u32_t)(fbe_get_time() / FBE_TIME_MILLISECONDS_PER_SECOND);

    // only LCC need this exclusive permission, other component do not care this, grant the permission directly. 
    fbe_edal_get_fw_target_component_type_attr(pFirmwareOp->target,
                                               &fw_comp_type,
                                               &fw_comp_attr);
    if (fw_comp_type != FBE_ENCL_LCC)
    {
        pFirmwareOp->permissionGranted = TRUE;
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_board,
                            FBE_TRACE_LEVEL_INFO,
                            "Board Object",
                            "Fup permission granted to %s at bus %d encl %d slot %d cid %d.\n",
                            fbe_eses_enclosure_fw_targ_to_text(pFirmwareOp->target),
                            pFirmwareOp->location.bus,
                            pFirmwareOp->location.enclosure,
                            pFirmwareOp->location.slot,
                            pFirmwareOp->location.componentId); 
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock(&base_board->enclGetLocalFupPermissionLock);

    // if no one holds the permission, then grant it.
    // if some one holds the permission for too long, then we also grant permission to current one.
    if (base_board->enclosureFupPermission.permissionOccupied == TRUE)
    {
        if (currentTime - base_board->enclosureFupPermission.permissionGrantTime < FBE_BASE_BOARD_FUP_PERMISSION_DENY_LIMIT_IN_SEC)
        {
            // you come again !?
            if (base_board->enclosureFupPermission.location.bus ==  pFirmwareOp->location.bus &&
                base_board->enclosureFupPermission.location.enclosure == pFirmwareOp->location.enclosure)
            {
                pFirmwareOp->permissionGranted = TRUE;
            }
            else
            {
                pFirmwareOp->permissionGranted = FALSE;
                pFirmwareOp->location = base_board->enclosureFupPermission.location;
            }
        }
        else
        {
            pFirmwareOp->permissionGranted = TRUE;
            base_board->enclosureFupPermission.location = pFirmwareOp->location;
            base_board->enclosureFupPermission.permissionOccupied = TRUE;
            base_board->enclosureFupPermission.permissionGrantTime = currentTime;
        }
    }
    else
    {
        pFirmwareOp->permissionGranted = TRUE;
        base_board->enclosureFupPermission.location = pFirmwareOp->location;
        base_board->enclosureFupPermission.permissionOccupied = TRUE;
        base_board->enclosureFupPermission.permissionGrantTime = currentTime;
    }

    fbe_spinlock_unlock(&base_board->enclGetLocalFupPermissionLock);

    if (pFirmwareOp->permissionGranted == TRUE)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_board,
                            FBE_TRACE_LEVEL_INFO,
                            "Board Object",
                            "Firmware upgrade permission granted to bus %d encl %d.\n",
                            base_board->enclosureFupPermission.location.bus, base_board->enclosureFupPermission.location.enclosure); 

    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)base_board,
                            FBE_TRACE_LEVEL_INFO,
                            "Board Object",
                            "Request denied. Encl %d_%d holds the permission.\n",
                            base_board->enclosureFupPermission.location.bus, base_board->enclosureFupPermission.location.enclosure); 
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn base_board_process_firmware_return_local_upgrade_permission                  
***************************************************************************
* @brief
*      This function gets called when board_mgmt module_mgmt and encl_mgmt
*      need return back permission after firmware upgrade complete. 
*      In ESP, board_mgmt and module_mgmt is in charge of upgrading enclosure 0_0 
*      on some DPE. Meanwhile encl_mgmt is do upgrade for all other enclosures. 
*      So there is possiblity they upgrade enclosures at the same time, which 
*      will cause problems. To solve that, we use this lock to ensure only one 
*      enclosure to upgrade at one time. on DPE board_mgmt moudle_mgmt and encl_mgmt 
*      should send request to board object to race for this permission. 
*
* @param     pEsesEncl - The pointer to the base_board_t.
* @param     pFirmwareOp - The pointer to the firmware op.
*
* @return    fbe_status_t.
*
* @note
*
* @version
*   18-Apr-2013 Rui Chang - Created.
***************************************************************************/
static fbe_status_t 
base_board_process_firmware_return_local_upgrade_permission(fbe_base_board_t * base_board, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp)
{
    fbe_enclosure_component_types_t fw_comp_type; 
    fbe_base_enclosure_attributes_t fw_comp_attr;
    fbe_bool_t                      permissionReturned;

    // only LCC need this exclusive permission, other component do not care this. 
    fbe_edal_get_fw_target_component_type_attr(pFirmwareOp->target,
                                               &fw_comp_type,
                                               &fw_comp_attr);
    if (fw_comp_type != FBE_ENCL_LCC)
    {
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock(&base_board->enclGetLocalFupPermissionLock);

    if (base_board->enclosureFupPermission.permissionOccupied == TRUE)
    {
        if (pFirmwareOp->location.bus == base_board->enclosureFupPermission.location.bus &&
            pFirmwareOp->location.enclosure == base_board->enclosureFupPermission.location.enclosure)
        {
            fbe_zero_memory(&(base_board->enclosureFupPermission.location), sizeof(fbe_device_physical_location_t));
            base_board->enclosureFupPermission.permissionOccupied = FALSE;
            base_board->enclosureFupPermission.permissionGrantTime = 0;
            permissionReturned = TRUE;
        }
    }
 
    fbe_spinlock_unlock(&base_board->enclGetLocalFupPermissionLock);

    fbe_base_object_customizable_trace((fbe_base_object_t*)base_board,
                            FBE_TRACE_LEVEL_INFO,
                            "Board Object",
                            "Firmware upgrade permission returned by bus %d encl %d.\n",
                            pFirmwareOp->location.bus, pFirmwareOp->location.enclosure); 

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn base_board_set_iom_persisted_power_state(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions sets the persisted power state of the specified io module.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   7-Feb-2013     bphilbin - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_iom_persisted_power_state(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_board_mgmt_set_iom_persisted_power_state_t       *persistedPowerStateCTRL;
    fbe_payload_control_buffer_length_t     len = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_DEBUG_LOW, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &persistedPowerStateCTRL);
    if (persistedPowerStateCTRL == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",
                              __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_board_mgmt_set_iom_persisted_power_state_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", 
                              __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Hack to allow NDU of changes to single array of IO devices in SPECL
    if ((persistedPowerStateCTRL->device_type == FBE_DEVICE_TYPE_MEZZANINE) ||
        (persistedPowerStateCTRL->device_type == FBE_DEVICE_TYPE_BACK_END_MODULE))
    {
        persistedPowerStateCTRL->slot += (base_board->localIoModuleCount - base_board->localAltIoDevCount);
    }

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s setting SP:%d slot:%d persisted power enable:%d\n", 
                           __FUNCTION__, 
                          persistedPowerStateCTRL->sp_id,
                          persistedPowerStateCTRL->slot,
                          persistedPowerStateCTRL->persisted_power_enable);

    status = fbe_base_board_set_io_module_persisted_power_state(persistedPowerStateCTRL->sp_id, 
                                             persistedPowerStateCTRL->slot,
                                             persistedPowerStateCTRL->persisted_power_enable);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, IOM %d, persisted power state is not SET. status 0x%x\n",
                              __FUNCTION__, 
                              persistedPowerStateCTRL->slot,
                              status); 
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;

}   // end of base_board_set_iom_persisted_power_state

static void base_board_process_group_policy_enable(fbe_packet_t *packet)
{
    fbe_physical_board_group_policy_info_t  *pGroupPolicy = NULL;
    fbe_payload_control_buffer_length_t     bufferLength = 0;
    fbe_payload_ex_t                        *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &pGroupPolicy);
    if (pGroupPolicy == NULL){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return;
    }

    fbe_payload_control_get_buffer_length(control_operation, &bufferLength); 
    if (bufferLength != sizeof(fbe_physical_board_group_policy_info_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return;
    }

    fbe_transport_run_queue_set_external_handler(pGroupPolicy->fbe_transport_external_run_queue_current);
    fbe_transport_fast_queue_set_external_handler(pGroupPolicy->fbe_transport_external_fast_queue_current);
}

/*!*************************************************************************
* @fn base_board_get_port_serial_num(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This functions get the serial number of the IO Module this port is on .
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   17-Jan-2014     Ashok Tamilarasan - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_get_port_serial_num(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_base_board_get_port_serial_num_t          *port_info = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &port_info);
    if (port_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_get_port_serial_num_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s %X len invalid\n", __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_board_get_serial_num_from_pci(base_board,
                                                    port_info->pci_bus,
                                                    port_info->pci_function, 
                                                    port_info->pci_slot,
                                                    port_info->phy_map,
                                                    port_info->serial_num,
                                                    FBE_EMC_SERIAL_NUM_SIZE);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t base_board_sim_switch_psl(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    /* Initialize a special layout using existing hardware.
     */
    fbe_private_space_layout_initialize_fbe_sim();

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

/*!*************************************************************************
* @fn base_board_set_cna_mode(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function sets the CNA mode (FC/Ethernet) for this system.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   07-Jan-2015     bphilbin - Created.
*
***************************************************************************/
static fbe_status_t 
base_board_set_cna_mode(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_base_board_set_cna_mode_t          *cna_mode = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &cna_mode);
    if (cna_mode == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_base_board_set_cna_mode_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s %X len invalid\n", __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_board_set_cna_mode(cna_mode->cna_mode);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
    
}

/*!***************************************************************
 *  base_board_set_local_veeprom_cpu_status()
 ****************************************************************
 * @brief
 *  This function is used to set local veeprom cpu status
 *
* @param  base_board - The pointer to the fbe_base_object_t.
* @param  packet - The pointer to the fbe_packet_t.
 *
 * @return  - fbe_status_t
 *
 * @author 
 *  04-July-2015: PHE - Created
 *
 ****************************************************************/
static fbe_status_t 
base_board_set_local_veeprom_cpu_status(fbe_base_board_t * base_board, 
                                    fbe_packet_t * packet)
{
    fbe_status_t    status;
    fbe_payload_ex_t   *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_u32_t                               *pCpuStatus = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &pCpuStatus);
    if (pCpuStatus == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, fbe_payload_control_get_buffer fail\n",__FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_u32_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s, %d len invalid\n", __FUNCTION__, len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_base_board_set_local_veeprom_cpu_status(*pCpuStatus);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Error in setting localVeepromCpuStatus, Status: 0x%x.\n",
                              __FUNCTION__, status);
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/**************************************************
*  end of base_board_set_local_veeprom_cpu_status()
**************************************************/

/*!*************************************************************************
* @fn base_board_get_hardware_ssv_data(                  
*                    fbe_base_object_t * base_board, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This function retrieves hardware SSV for this system.
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   27-July-2015     nqiu - merged from Rockies.
*
***************************************************************************/
static fbe_status_t base_board_get_hardware_ssv_data(fbe_base_board_t * base_board, fbe_packet_t * packet)
{
    fbe_encryption_hardware_ssv_t          *ssv_info = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t                               offset;
    fbe_device_physical_location_t deviceLocation; 
    SMB_DEVICE smb_device;

    fbe_base_object_trace((fbe_base_object_t*)base_board, 
                           FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                           "%s entry\n", 
                           __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &ssv_info);
    if (ssv_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer fail\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_encryption_hardware_ssv_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_board, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s %X len invalid\n", __FUNCTION__, len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(ssv_info, sizeof(fbe_encryption_hardware_ssv_t));

    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       FBE_DEVICE_TYPE_ENCLOSURE, 
                                                       NULL, 
                                                       &smb_device);

    offset = getResumeFieldOffset(RESUME_PROM_PRODUCT_SN);
    status = fbe_base_board_get_resume_field_data(base_board,
                                                  smb_device, 
                                                  ssv_info->hw_ssv_midplane_product_sn,
                                                  offset, 
                                                  RESUME_PROM_PRODUCT_SN_SIZE);

    offset = getResumeFieldOffset(RESUME_PROM_EMC_TLA_SERIAL_NUM);

    /* Read in the serial number in the Midplane Resume PROM */
    status = fbe_base_board_get_resume_field_data(base_board,
                                                  smb_device, 
                                                  ssv_info->hw_ssv_midplane_sn,
                                                  offset, 
                                                  RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);

    /* Now get SPA's Serial Number */
    deviceLocation.sp = 0;
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       FBE_DEVICE_TYPE_SP, 
                                                       &deviceLocation, 
                                                       &smb_device);
    status = fbe_base_board_get_resume_field_data(base_board,
                                                  smb_device, 
                                                  ssv_info->hw_ssv_spa_sn,
                                                  offset, RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);


    /* Now get SPB's Serial Number */
    deviceLocation.sp = 1;
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       FBE_DEVICE_TYPE_SP, 
                                                       &deviceLocation, 
                                                       &smb_device);
    status = fbe_base_board_get_resume_field_data(base_board,
                                                  smb_device, 
                                                  ssv_info->hw_ssv_spb_sn,
                                                  offset, RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);

    /* Now get the SPA Management Module Serial Number */
    deviceLocation.sp = 0;
    deviceLocation.slot = 0;
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       FBE_DEVICE_TYPE_MGMT_MODULE, 
                                                       &deviceLocation, 
                                                       &smb_device);
    status = fbe_base_board_get_resume_field_data(base_board,
                                                  smb_device, 
                                                  ssv_info->hw_ssv_mgmta_sn,
                                                  offset, RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);

    /* Now get the SPB Management Module Serial Number */
    deviceLocation.sp = 1;
    deviceLocation.slot = 0;
    status = fbe_base_board_get_resume_prom_smb_device(base_board,
                                                       FBE_DEVICE_TYPE_MGMT_MODULE, 
                                                       &deviceLocation, 
                                                       &smb_device);
    status = fbe_base_board_get_resume_field_data(base_board,
                                                  smb_device, 
                                                  ssv_info->hw_ssv_mgmtb_sn,
                                                  offset, RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

/*!*************************************************************************
* @fn fbe_base_board_get_resume_field_data(fbe_base_board_t * base_board,
*                                          SMB_DEVICE smb_device,
*                                          fbe_u8_t *buffer,
*                                          fbe_u32_t offset,
*                                          fbe_u32_t buffer_size)
***************************************************************************
*
* @brief
*       This function read specific smb device at offset by number of byytes
*
* @param      base_board - The pointer to the fbe_base_object_t.
* @param      smb_device - smb device
* @param      buffer     - pointer to read result
* @param      offset     - offset into resume
* @param      buffer_size - number of bytes to read
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   27-July-2015     nqiu - merged from Rockies.
*
***************************************************************************/
fbe_status_t fbe_base_board_get_resume_field_data(fbe_base_board_t * base_board,
                                                  SMB_DEVICE smb_device,
                                                  fbe_u8_t *buffer,
                                                  fbe_u32_t offset,
                                                  fbe_u32_t buffer_size)
{
    SPECL_RESUME_DATA                       specl_resume_data = {0};
    fbe_resume_prom_status_t                resumePromStatus ;
    fbe_status_t                            status;
    fbe_u8_t                                *pSrc;    
    fbe_u32_t count = 0;

    do
    {
        /* Read the resume PROM */
        status = fbe_base_board_get_resume(smb_device, &specl_resume_data);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_board, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, GET Resume Prom failed for SMB Device: %d.\n",
                                  __FUNCTION__, smb_device); 
            return FBE_STATUS_GENERIC_FAILURE;
        }

        pSrc = (fbe_u8_t *)(&specl_resume_data.resumePromData) + offset;


        status = fbe_base_board_pe_xlate_smb_status(specl_resume_data.transactionStatus, 
                                                    &resumePromStatus,
                                                    specl_resume_data.retriesExhausted);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_board,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "Resume Prom status Translation failed. Xaction status: %d.\n", 
                                  specl_resume_data.transactionStatus);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if(resumePromStatus == FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS)
        {
            fbe_thread_delay(1000); // 1Sec
            count++;
        }
        else
        {
            if(resumePromStatus == FBE_RESUME_PROM_STATUS_READ_SUCCESS)
            {
                fbe_copy_memory(buffer, pSrc, buffer_size);
                status = FBE_STATUS_OK;
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)base_board,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Resume Prom read failed %d.\n", 
                                      __FUNCTION__, resumePromStatus);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            return status;
        }
    } while ((count < 10) && (resumePromStatus == FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS));

    fbe_base_object_trace((fbe_base_object_t *)base_board,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Resume Prom read failed after retries: %d.\n", 
                          resumePromStatus);

    return FBE_STATUS_GENERIC_FAILURE;
}

//End of file fbe_base_board_usurper.c
