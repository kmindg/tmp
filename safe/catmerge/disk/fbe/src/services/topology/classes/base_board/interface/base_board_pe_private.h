#ifndef BASE_BOARD_PE_PRIVATE_H
#define BASE_BOARD_PE_PRIVATE_H

#include "base_board_private.h"
#include "specl_types.h"

#define FBE_BASE_BOARD_PE_NUMBER_OF_COMPONENT_TYPES  20
/*************************************************************************
 | Macro:  FBE_FOR_ALL_IO_MODULES(sp, iomodule, pSpeclIoSum)
 |
 | Description:
 |  This macro is used to iterate over all of io modules 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_IO_MODULES(sp, iomodule, pSpeclIoSum)          \
for(sp = 0; sp < pSpeclIoSum->bladeCount; sp++)                   \
for(iomodule = 0; \
    iomodule < pSpeclIoSum->ioSummary[sp].ioModuleCount; \
    iomodule++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_MPLXR_IO_PORTS(ioModule, portNumOnModule, pSpeclMplxrSum)
 |
 | Description:
 |  This macro is used to iterate over all of MPLXRs 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_MPLXR_IO_PORTS(ioModule, portNumOnModule, pSpeclMplxrSum)          \
for(ioModule = 0; ioModule < pSpeclMplxrSum->ioModuleCount; ioModule ++)                   \
for(portNumOnModule = 0; \
    portNumOnModule < MAX_IO_PORTS_PER_MODULE; \
    portNumOnModule ++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_FLT_REGS(sp, fltReg, pSpeclFltExpSum)
 |
 | Description:
 |  This macro is used to iterate over all of fault registers 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_FLT_REGS(sp, fltReg, pSpeclFltExpSum)          \
for(sp = 0; sp < pSpeclFltExpSum->bladeCount; sp++)                   \
for(fltReg = 0; \
    fltReg < pSpeclFltExpSum->fltExpSummary[sp].faultRegisterCount; \
    fltReg++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_SLAVE_PORTS(sp, slavePort, pSpeclSlavePortSum)
 |
 | Description:
 |  This macro is used to iterate over all of slave ports 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_SLAVE_PORTS(sp, slavePort, pSpeclSlavePortSum)          \
for(sp = 0; sp < pSpeclSlavePortSum->bladeCount; sp++)                   \
for(slavePort = 0; \
    slavePort < pSpeclSlavePortSum->slavePortSummary[sp].slavePortCount; \
    slavePort++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_MGMT_MODULES(sp, MgmtModNum, pSpeclMgmtSum)
 |
 | Description:
 |  This macro is used to iterate over all of management modules 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_MGMT_MODULES(sp, MgmtModNum, pSpeclMgmtSum)          \
for(sp = 0; sp < pSpeclMgmtSum->bladeCount; sp++)                   \
for(MgmtModNum = 0; \
    MgmtModNum < pSpeclMgmtSum->mgmtSummary[sp].mgmtCount; \
    MgmtModNum++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_POWER_SUPPLIES(sp, psNum, pSpeclPsSum)
 |
 | Description:
 |  This macro is used to iterate over all of power supplies
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_POWER_SUPPLIES(sp, psNum, pSpeclPsSum)          \
for(sp = 0; sp < pSpeclPsSum->bladeCount; sp++)                   \
for(psNum = 0; \
    psNum < pSpeclPsSum->psSummary[sp].psCount; \
    psNum++ )
/*************************************************************************
 | Macro:  FBE_FOR_ALL_BATTERIES(sp, batteryNum, pSpeclBatterySum)
 |
 | Description:
 |  This macro is used to iterate over all of the batteries
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_BATTERIES(sp, batteryNum, pSpeclBatterySum)     \
for(sp = 0; sp < pSpeclBatterySum->bladeCount; sp++)                \
for(batteryNum = 0;                                                 \
    batteryNum < pSpeclBatterySum->batterySummary[sp].batteryCount; \
    batteryNum++ )
/*************************************************************************
 | Macro:  FBE_FOR_ALL_FANS(sp, fanPackNum, fanNum, pSpeclFanSum)
 |
 | Description:
 |  This macro is used to iterate over all of fans 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_FANS(sp, fanPackNum, fanNum, pSpeclFanSum)          \
for(sp = 0; sp < pSpeclFanSum->bladeCount; sp++)     \
for(fanPackNum = 0; \
    fanPackNum < pSpeclFanSum->fanSummary[sp].fanPackCount; \
    fanPackNum++ ) \
for(fanNum = 0; \
    fanNum < pSpeclFanSum->fanSummary[sp].fanPackStatus[fanPackNum].fanCount; \
    fanNum++ )
/*************************************************************************
 | Macro:  FBE_FOR_ALL_FAN_PACKS(sp, fanPackNum, pSpeclFanSum)
 |
 | Description:
 |  This macro is used to iterate over all of fan packs
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_FAN_PACKS(sp, fanPackNum, pSpeclFanSum)          \
for(sp = 0; sp < pSpeclFanSum->bladeCount; sp++)     \
for(fanPackNum = 0; \
    fanPackNum < pSpeclFanSum->fanSummary[sp].fanPackCount; \
    fanPackNum++ ) 
/*************************************************************************
 | Macro:  FBE_FOR_ALL_SUITCASES(sp, suitcaseNum, pSpeclSuitcaseSum)
 |
 | Description:
 |  This macro is used to iterate over all of suitcases 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_SUITCASES(sp, suitcaseNum, pSpeclSuitcaseSum)          \
for(sp = 0; sp < pSpeclSuitcaseSum->bladeCount; sp++)                   \
for(suitcaseNum = 0; \
    suitcaseNum < pSpeclSuitcaseSum->suitcaseSummary[sp].suitcaseCount; \
    suitcaseNum++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_BMC(sp, bmcNum, pSpeclBmcSum)
 |
 | Description:
 |  This macro is used to iterate over all of BMCs 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_BMC(sp, bmcNum, pSpeclBmcSum)          \
for(sp = 0; sp < pSpeclBmcSum->bladeCount; sp++)                   \
for(bmcNum = 0; \
    bmcNum < pSpeclBmcSum->bmcSummary[sp].bmcCount; \
    bmcNum++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_CACHE_CARD(sp, cacheCardNum, pSpeclCacheCardSum)
 |
 | Description:
 |  This macro is used to iterate over all of CacheCard Cache Card 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_CACHE_CARD(cacheCardNum, pSpeclCacheCardSum)          \
for(cacheCardNum = 0; \
    cacheCardNum < pSpeclCacheCardSum->cacheCardCount; \
    cacheCardNum++ )

/*************************************************************************
 | Macro:  FBE_FOR_ALL_DIMM(dimmNum, pSpeclDimmSum)
 |
 | Description:
 |  This macro is used to iterate over all of DIMM 
 |  in processor enclosure.
 ***********************************************************************/
#define FBE_FOR_ALL_DIMM(dimmNum, pSpeclDimmSum)          \
for(dimmNum = 0; \
    dimmNum < pSpeclDimmSum->dimmCount; \
    dimmNum++ )


/* fbe_base_board_pe_read.c */
fbe_status_t fbe_base_board_init_component_count(fbe_base_board_t * base_board, 
                                    fbe_base_board_pe_init_data_t pe_init_data[]);
fbe_status_t fbe_base_board_init_component_size(fbe_base_board_pe_init_data_t pe_init_data[]);
fbe_status_t fbe_base_board_pe_get_required_buffer_size(fbe_base_board_pe_init_data_t pe_init_data[],
                                    fbe_u32_t *pe_buffer_size_p);
fbe_status_t fbe_base_board_get_platform_info_and_board_type(fbe_base_board_t * base_board,
                                    fbe_board_type_t * board_type);
fbe_status_t fbe_base_board_read_misc_and_led_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_io_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_mezzanine_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_temperature_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_mplxr_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_power_supply_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_sps_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_battery_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_cooling_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_mgmt_module_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_flt_exp_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_flt_reg_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_slave_port_status(fbe_base_board_t * base_board);

fbe_status_t fbe_base_board_read_suitcase_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_bmc_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_cache_card_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_ssd_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_dimm_status(fbe_base_board_t * base_board);
fbe_status_t fbe_base_board_read_resume_prom_status(fbe_base_board_t * base_board);
fbe_status_t base_board_process_state_change(fbe_base_board_t * base_board, 
                                fbe_base_board_pe_init_data_t pe_init_data[]); 
fbe_status_t fbe_base_board_process_fup_status_change(fbe_base_board_t * pBaseBoard); 

/* fbe_base_board_pe_shim.c */
fbe_status_t fbe_base_board_pe_get_io_comp_status(SPECL_IO_SUMMARY * pSpeclIoSum);
fbe_status_t fbe_base_board_pe_get_temperature_status(SPECL_TEMPERATURE_SUMMARY * pSpeclTempSum);
fbe_status_t fbe_base_board_pe_get_mplxr_status(SPECL_MPLXR_SUMMARY * pSpeclMplxrSum);
fbe_status_t fbe_base_board_pe_get_misc_status(SPECL_MISC_SUMMARY * pSpeclMiscSum);
fbe_status_t fbe_base_board_pe_get_led_status(SPECL_LED_SUMMARY * pSpeclLedSum);
fbe_status_t fbe_base_board_pe_get_flt_exp_status(SPECL_FLT_EXP_SUMMARY * pSpeclFltExpSum);
fbe_status_t fbe_base_board_pe_get_slave_port_status(SPECL_SLAVE_PORT_SUMMARY * pSpeclSlavePortSum);
fbe_status_t fbe_base_board_pe_get_mgmt_module_status(SPECL_MGMT_SUMMARY * pSpeclMgmtSum);
fbe_status_t fbe_base_board_pe_get_cooling_status(SPECL_FAN_SUMMARY * pSpeclCoolingSum);
fbe_status_t fbe_base_board_pe_get_power_supply_status(SPECL_PS_SUMMARY * pSpeclPSSum);
fbe_status_t fbe_base_board_pe_get_sps_status(SPECL_SPS_SUMMARY * pSpeclSpsSum);
fbe_status_t fbe_base_board_pe_get_sps_resume(PSPECL_SPS_RESUME  pSpeclSpsResume);
fbe_status_t fbe_base_board_pe_get_battery_status(SPECL_BATTERY_SUMMARY * pSpeclBatterySum);
fbe_status_t fbe_base_board_pe_start_battery_testing(SP_ID spid, fbe_u8_t slot);
fbe_status_t fbe_base_board_pe_stop_battery_testing(SP_ID spid, fbe_u8_t slot);
fbe_status_t fbe_base_board_pe_enable_battery(SP_ID spid, fbe_u8_t slot);
fbe_status_t fbe_base_board_pe_disable_battery(SP_ID spid, fbe_u8_t slot);
fbe_status_t fbe_base_board_pe_set_battery_energy_req(SP_ID spid, fbe_u8_t slot, fbe_base_board_battery_action_data_t batteryRequirements);
fbe_status_t fbe_base_board_pe_enableRideThroughTimer(SP_ID spid, fbe_bool_t lowPowerMode, fbe_u8_t vaultTimeout);
fbe_status_t fbe_base_board_pe_disableRideThroughTimer(SP_ID spid, fbe_bool_t lowPowerMode, fbe_u8_t vaultTimeout);
fbe_status_t fbe_base_board_pe_get_suitcase_status(SPECL_SUITCASE_SUMMARY * pSpeclSuitcaseSum);
fbe_status_t fbe_base_board_pe_get_bmc_status(SPECL_BMC_SUMMARY * pSpeclBmcSum);
fbe_status_t fbe_base_board_pe_start_sps_testing(void);
fbe_status_t fbe_base_board_pe_stop_sps_testing(void);
fbe_status_t fbe_base_board_pe_shutdown_sps(void);
fbe_status_t fbe_base_board_pe_set_sps_present(SP_ID spid, fbe_bool_t spsPresent);
fbe_status_t fbe_base_board_pe_get_cache_card_status(SPECL_CACHE_CARD_SUMMARY * pSpeclCacheCardSum);
fbe_status_t fbe_base_board_pe_set_sps_fastSwitchover(fbe_bool_t spsFastSwitchover);
fbe_status_t fbe_base_board_pe_get_dimm_status(SPECL_DIMM_SUMMARY * pSpeclDimmSum);


/* Set functions */
fbe_status_t fbe_base_board_set_enclFaultLED(LED_BLINK_RATE blink_rate);
fbe_status_t fbe_base_board_set_UnsafetoRemoveLED(LED_BLINK_RATE blink_rate);
fbe_status_t fbe_base_board_set_spFaultLED(LED_BLINK_RATE blink_rate, fbe_u32_t status_condition);

fbe_status_t fbe_base_board_set_iomFaultLED(SP_ID sp_id, fbe_u32_t io_module, LED_BLINK_RATE blink_rate);
fbe_status_t fbe_base_board_set_iom_port_LED(SP_ID sp_id, fbe_u32_t io_module, LED_BLINK_RATE blink_rate,
                                             fbe_u32_t io_port, LED_COLOR_TYPE led_color);

fbe_status_t fbe_base_board_set_mgmt_vlan_config_mode(SP_ID sp_id, VLAN_CONFIG_MODE vlan_mode);
fbe_status_t fbe_base_board_set_mgmt_fault_LED(SP_ID sp_id, LED_BLINK_RATE blink_rate);
fbe_status_t fbe_base_board_set_mgmt_port(fbe_base_board_t * base_board,
                                          SP_ID sp_id, PORT_ID_TYPE portIDType, 
                                          fbe_mgmt_port_auto_neg_t mgmtPortAutoNeg,
                                          fbe_mgmt_port_speed_t mgmtPortSpeed,
                                          fbe_mgmt_port_duplex_mode_t mgmtPortDuplex);

fbe_status_t fbe_base_board_set_PostAndOrReset(SP_ID sp_id, fbe_bool_t holdInPost, 
                                               fbe_bool_t holdInReset, fbe_bool_t rebootBlade);

fbe_status_t fbe_base_board_set_resume(SMB_DEVICE device, RESUME_PROM_STRUCTURE * pResumePromStructure);
fbe_status_t fbe_base_board_get_resume(SMB_DEVICE device, SPECL_RESUME_DATA *resume_data);

fbe_status_t 
fbe_base_board_get_io_port_component_index(fbe_base_board_t * pBaseBoard,
                                           SP_ID associatedSp,
                                           fbe_u32_t slotNumOnBlade,
                                           fbe_u32_t portNumOnModule,
                                           fbe_u64_t deviceType, 
                                           fbe_u32_t * pComponentIndex);

fbe_status_t fbe_base_board_get_elapsed_time_data(fbe_u64_t *out_elapsed_time, fbe_u64_t in_elapsed_time, 
                                                  SPECL_TIMESCALE_UNIT convert_time);
fbe_status_t fbe_base_board_set_local_software_boot_status(SW_GENERAL_STATUS_CODE  generalStatus,
                                                    BOOT_SW_COMPONENT_CODE  componentStatus,
                                                    SW_COMPONENT_EXT_CODE  componentExtStatus);
fbe_status_t fbe_base_board_set_local_flt_exp_status(fbe_u8_t faultStatus);

fbe_status_t fbe_base_board_sps_download_command(SPS_FW_TYPE imageType, fbe_u8_t *pImageBuffer, fbe_u32_t imageLength);
fbe_status_t fbe_base_board_set_io_module_persisted_power_state(SP_ID sp_id, fbe_u32_t io_module, fbe_bool_t powerup_enabled);
fbe_status_t fbe_base_board_set_cna_mode(SPECL_CNA_MODE cna_mode);

/* fbe_base_board_pe_utils.c */
fbe_status_t fbe_base_board_get_resume_prom_id(SP_ID sp, fbe_u32_t slotNum, 
                                               fbe_u64_t device_type, 
                                               fbe_pe_resume_prom_id_t *resumePromId);

fbe_status_t fbe_base_board_get_resume_prom_device_type_and_location(fbe_base_board_t * base_board,
                                                         fbe_pe_resume_prom_id_t rp_comp_index, 
                                                         fbe_device_physical_location_t *device_location, 
                                                         fbe_u64_t *device_type);
fbe_status_t fbe_base_board_get_resume_prom_smb_device(fbe_base_board_t * base_board,
                                                       fbe_u64_t device_type, 
                                                       fbe_device_physical_location_t * pLocation, 
                                                       SMB_DEVICE *smb_device);

/* fbe_base_board_pe_write.c */
fbe_status_t fbe_base_board_check_mgmt_vlan_mode_command(fbe_base_board_t *base_board, 
                                            fbe_payload_control_operation_t *control_operation);
fbe_status_t fbe_base_board_check_mgmt_port_command(fbe_base_board_t *base_board, 
                                            fbe_payload_control_operation_t *control_operation);
fbe_status_t fbe_base_board_check_iom_fault_led_command(fbe_base_board_t *base_board, 
                                               fbe_payload_control_operation_t *control_operation);
fbe_status_t fbe_base_board_check_hold_in_post_and_or_reset(fbe_base_board_t *base_board, 
                                               fbe_payload_control_operation_t *control_operation);
fbe_status_t fbe_base_board_check_resume_prom_last_write_status(fbe_base_board_t *base_board,
                                                               fbe_payload_control_operation_t *control_operation);
fbe_status_t fbe_base_board_check_resume_prom_last_write_status_async(fbe_base_board_t *base_board,
                                                                      fbe_payload_control_operation_t *control_operation);
fbe_status_t fbe_base_board_set_local_veeprom_cpu_status(fbe_u32_t cpuStatus);
#endif/* FBE_BASE_BOARD_PE_PRIVATE_H */
