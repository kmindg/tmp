#ifndef FBE_API_BOARD_INTERFACE_H
#define FBE_API_BOARD_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_board_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the Board APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * 
 * @version
 *   02/19/10   Joe Perry    Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_board.h"
//#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_common.h"
#include "fbe_pe_types.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_resume_prom_info.h"

//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief This is the set of definitions for FBE Physical Package APIs.
 *  @ingroup fbe_api_physical_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Enclosure Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_board_usurper_interface FBE API Board Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Board Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

typedef struct fbe_api_board_enclFaultLedInfo_s
{
    LED_BLINK_RATE      blink_rate;
    fbe_encl_fault_led_reason_t           enclFaultLedReason;
} fbe_api_board_enclFaultLedInfo_t;


/*! @} */ /* end of fbe_api_board_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Board Interface.  
// This is where all the function prototypes for the Board API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_board_interface FBE API Board Interface
 *  @brief 
 *    This is the set of FBE API Board Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_board_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL 
fbe_api_get_board_object_id(fbe_object_id_t *object_id);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_sps_status(fbe_object_id_t object_id, fbe_sps_get_sps_status_t *sps_status);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_battery_status(fbe_object_id_t object_id, fbe_base_board_mgmt_get_battery_status_t *battery_status);

fbe_status_t FBE_API_CALL 
fbe_api_board_send_battery_command(fbe_object_id_t object_id, 
                               fbe_base_board_mgmt_battery_command_t *batteryCommand);
fbe_status_t FBE_API_CALL 
fbe_api_board_get_sps_manuf_info(fbe_object_id_t object_id, fbe_sps_get_sps_manuf_info_t *spsManufInfo);

fbe_status_t FBE_API_CALL 
fbe_api_board_send_sps_command(fbe_object_id_t object_id, fbe_sps_action_type_t spsCommand);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_pe_info(fbe_object_id_t object_id, fbe_board_mgmt_get_pe_info_t *pe_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_power_supply_info(fbe_object_id_t object_id, fbe_power_supply_info_t *ps_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_io_port_info(fbe_object_id_t object_id, fbe_board_mgmt_io_port_info_t *io_port_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_mgmt_module_info(fbe_object_id_t object_id, fbe_board_mgmt_mgmt_module_info_t *mgmt_module_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_cooling_info(fbe_object_id_t object_id, fbe_cooling_info_t *cooling_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_platform_info(fbe_object_id_t object_id, fbe_board_mgmt_platform_info_t *platform_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_misc_info(fbe_object_id_t object_id, fbe_board_mgmt_misc_info_t *misc_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_bmc_info(fbe_object_id_t object_id, fbe_board_mgmt_bmc_info_t *bmc_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_suitcase_info(fbe_object_id_t object_id, fbe_board_mgmt_suitcase_info_t *suitcase_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_flt_reg_info(fbe_object_id_t object_id, fbe_peer_boot_flt_exp_info_t *flt_reg_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_slave_port_info(fbe_object_id_t object_id, fbe_board_mgmt_slave_port_info_t *slave_port_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_io_module_count(fbe_object_id_t object_id, fbe_u32_t *iom_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_io_port_count(fbe_object_id_t object_id, fbe_u32_t *iop_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_bem_count(fbe_object_id_t object_id, fbe_u32_t *ioa_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_mezzanine_count(fbe_object_id_t object_id, fbe_u32_t *mezz_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_ps_info(fbe_object_id_t object_id, 
                          fbe_base_board_mgmt_set_ps_info_t *ps_info);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_ps_count(fbe_object_id_t object_id, fbe_u32_t *ps_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_cooling_count(fbe_object_id_t object_id, fbe_u32_t *fan_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_suitcase_count(fbe_object_id_t object_id, fbe_u32_t *sc_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_bmc_count(fbe_object_id_t object_id, fbe_u32_t *bmc_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_fault_reg_count(fbe_object_id_t object_id, fbe_u32_t *fault_reg_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_slave_port_count(fbe_object_id_t object_id, fbe_u32_t *slave_port_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_io_module_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *iom_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_mezzanine_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *mezz_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_bem_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *ioa_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_io_port_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *io_port_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_mgmt_module_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *mgmt_mod_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_cooling_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *cooling_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_suitcase_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *suitcase_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_bmc_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *bmc_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_power_supply_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *ps_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_flt_reg_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *flt_reg_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_get_slave_port_count_per_blade(fbe_object_id_t object_id, fbe_u32_t *slave_port_count);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_spFaultLED(fbe_object_id_t object_id, LED_BLINK_RATE blink_rate, fbe_u32_t status_condition);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_enclFaultLED(fbe_object_id_t object_id, fbe_api_board_enclFaultLedInfo_t *enclFaultLedInfo);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_UnsafetoRemoveLED(fbe_object_id_t object_id, LED_BLINK_RATE blink_rate);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_IOModuleFaultLED(fbe_object_id_t object_id, SP_ID sp_id, 
                                fbe_u32_t slot, fbe_u64_t device_type, LED_BLINK_RATE blink_rate);

fbe_status_t FBE_API_CALL fbe_api_board_set_IOModulePersistedPowerState(fbe_object_id_t object_id,
															 SP_ID sp_id, 
															 fbe_u32_t slot,
															 fbe_u64_t device_type,
                                                             fbe_bool_t persisted_power_enable);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_iom_port_LED(fbe_object_id_t object_id,
                                fbe_board_mgmt_set_iom_port_LED_t *iom_port);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_mgmt_module_vlan_config_mode_async(fbe_object_id_t object_id,
                                                     fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *context);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_mgmt_module_fault_LED(fbe_object_id_t object_id, SP_ID sp_id, LED_BLINK_RATE blink_rate);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_mgmt_module_port_async(fbe_object_id_t object_id,
                                         fbe_board_mgmt_set_mgmt_port_async_context_t *context);
fbe_status_t FBE_API_CALL 
fbe_api_board_set_PostAndOrReset_async(fbe_object_id_t object_id, 
                                 fbe_board_mgmt_set_PostAndOrReset_async_context_t *context);
fbe_status_t FBE_API_CALL 
fbe_api_board_clear_flt_reg_fault(fbe_object_id_t object_id, 
                                  fbe_peer_boot_flt_exp_info_t *flt_reg_info);
fbe_status_t FBE_API_CALL 
fbe_api_board_set_FlushFilesAndReg(fbe_object_id_t object_id,
                                                      fbe_board_mgmt_set_FlushFilesAndReg_t *flushOptions);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_resume(fbe_object_id_t object_id, fbe_board_mgmt_set_resume_t *resume_prom);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_comp_max_timeout(fbe_object_id_t object_id, 
                                   fbe_u32_t component, fbe_u32_t max_timeout);


/* GET declarations */
fbe_status_t FBE_API_CALL 
fbe_api_board_get_resume(fbe_object_id_t object_id, 
                         fbe_board_mgmt_get_resume_t *resume_prom);
fbe_status_t FBE_API_CALL 
fbe_api_board_resume_read_sync(fbe_object_id_t object_id, 
                               fbe_resume_prom_cmd_t * pResumeReadCmd);
fbe_status_t FBE_API_CALL 
fbe_api_board_resume_read_async(fbe_object_id_t objectId, 
                                fbe_resume_prom_get_resume_async_t *pGetResumeProm);

fbe_status_t FBE_API_CALL 
fbe_api_board_resume_write_sync(fbe_object_id_t object_id, 
                                fbe_resume_prom_cmd_t * pWriteResumeCmd);

fbe_status_t FBE_API_CALL fbe_api_board_resume_write_async(fbe_object_id_t object_id, 
                                                          fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp);
fbe_status_t FBE_API_CALL 
fbe_api_board_get_iom_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_io_comp_info_t *iom_info);
fbe_status_t FBE_API_CALL 
fbe_api_board_get_bem_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_io_comp_info_t *iom_info);
fbe_status_t FBE_API_CALL 
fbe_api_board_get_mezzanine_info(fbe_object_id_t object_id, 
                          fbe_board_mgmt_io_comp_info_t *iom_info);
fbe_status_t FBE_API_CALL
fbe_api_board_get_base_board_info(fbe_object_id_t object_id,
                                  fbe_base_board_get_base_board_info_t *base_board_info_ptr);
fbe_status_t FBE_API_CALL fbe_api_board_get_eir_info(fbe_object_id_t object_id,
                                                     fbe_base_board_get_eir_info_t *eir_info_ptr);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_local_software_boot_status(fbe_object_id_t object_id,
                                             fbe_base_board_set_local_software_boot_status_t * pSetLocalSoftwareBootStatus);

fbe_status_t FBE_API_CALL 
fbe_api_board_set_local_flt_exp_status(fbe_object_id_t object_id,
                                       fbe_u8_t faultStatus);

fbe_status_t FBE_API_CALL fbe_api_board_get_cache_card_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *cache_card_count);
fbe_status_t FBE_API_CALL fbe_api_board_get_cache_card_info(fbe_object_id_t object_id, 
                                   fbe_board_mgmt_cache_card_info_t *cache_card_info);
fbe_status_t FBE_API_CALL fbe_api_board_get_dimm_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *dimm_count);
fbe_status_t FBE_API_CALL fbe_api_board_get_dimm_info(fbe_object_id_t object_id, 
                                   fbe_board_mgmt_dimm_info_t *dimm_info);
fbe_status_t FBE_API_CALL fbe_api_board_get_ssd_count(fbe_object_id_t object_id, 
                                                      fbe_u32_t *ssd_count);
fbe_status_t FBE_API_CALL fbe_api_board_get_ssd_info(fbe_object_id_t object_id, 
                                   fbe_board_mgmt_ssd_info_t *ssd_info);

fbe_status_t FBE_API_CALL fbe_api_board_set_async_io(fbe_object_id_t object_id, fbe_bool_t async_io);
fbe_status_t FBE_API_CALL fbe_api_board_set_dmrb_zeroing(fbe_object_id_t object_id, fbe_bool_t dmrb_zeroing);
fbe_status_t FBE_API_CALL fbe_api_board_sim_set_psl(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_board_set_cna_mode(fbe_object_id_t object_id, SPECL_CNA_MODE cna_mode);
/*! @} */ /* end of group fbe_api_board_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE Physical Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Physical 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_physical_package_interface_class_files FBE Physical Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Physical Package Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_BOARD_INTERFACE_H*/
