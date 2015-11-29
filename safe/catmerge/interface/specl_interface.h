#ifndef __SPECL_INTERFACE__
#define __SPECL_INTERFACE__
/***************************************************************************
 *  Copyright (C)  EMC Corporation 1992-2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. 
 ***************************************************************************/

/***************************************************************************
 * specl_interface.h
 ***************************************************************************
 *
 * File Description:
 *  SPECL [Storage Processor Enclosure Cache Layer] header file which provide
 *  the exported kernel-space interface to other components. 
 *
 * Author:
 *  Phillip Leef
 *  
 * Revision History:
 *  March 3, 2007 - PHL - Created Inital Version
 *  June 3, 2010 - Sameer Bodhe - Mearged specl_kernel_interface.h and 
 *                                specl_user_interface.h files.
 ***************************************************************************/

#include "generic_types.h"
#include "smbus_driver.h"
#include "spid_types.h"
#include "resume_prom.h"
#include "familyfruid.h"
#include "specl_types.h"
#include "specl_sfi_types.h"
#include "rebootGlobal.h"
#include "rebootLogging.h"

#if !(defined(UMODE_ENV) || defined(SIMMODE_ENV))
#include "k10ntddk.h"
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT
typedef EMCPAL_STATUS SPECL_STATUS;
#ifdef _SPECL_
#define SPECLAPI CSX_MOD_EXPORT
#else
#define SPECLAPI CSX_MOD_IMPORT
#endif
#else
typedef DWORD SPECL_STATUS;
#define SPECLAPI
#if defined(__cplusplus)
extern "C"
{
#endif
EMCUTIL_STATUS speclOpenHandle(void);
BOOL speclCloseHandle(void);
#if defined(__cplusplus)
}
#endif
#endif


#if defined(__cplusplus)
extern "C"
{
#endif

/************************* E-X-P-O-R-T-E-D   I-N-T-E-R-F-A-C-E   F-U-N-C-T-I-O-N-S *************************/

/* In Kernel mode drivers, NONE OF THESE ROUTINES can be called above dispatch level
 */

SPECLAPI SPECL_STATUS speclGetResume(SMB_DEVICE             smbDevice,
                                     PSPECL_RESUME_DATA     resume_ptr);

SPECLAPI SPECL_STATUS speclSetResume(SMB_DEVICE             smbDevice,
                                     RESUME_PROM_STRUCTURE  *resumePromData);

SPECLAPI SPECL_STATUS speclForceResumeRead(SMB_DEVICE smbDevice);

SPECLAPI SPECL_STATUS speclInvalidateResumeChecksum(SMB_DEVICE smbDevice);

SPECLAPI SPECL_STATUS speclGetPowerSupplyStatus(PSPECL_PS_SUMMARY ps_status_ptr);

SPECLAPI SPECL_STATUS speclGetFanStatus(PSPECL_FAN_SUMMARY fan_status_ptr);

SPECLAPI SPECL_STATUS speclSetAutomaticFanControl(SP_ID     spid,
                                                  BOOL      autoFanCtrlEnable);

SPECLAPI SPECL_STATUS speclSetFanSpeeds(SP_ID           spid,
                                        csx_uchar_t     fanAPWM,
                                        csx_uchar_t     fanBPWM,
                                        csx_uchar_t     fanCPWM,
                                        csx_uchar_t     fanDPWM,
                                        csx_uchar_t     fanEPWM);

SPECLAPI SPECL_STATUS speclSetFanPackFaultLed(SP_ID             spid,
                                              ULONG             fanPackID,
                                              LED_BLINK_RATE    blinkRate);

SPECLAPI SPECL_STATUS speclGetBatteryStatus(PSPECL_BATTERY_SUMMARY battery_status_ptr);

SPECLAPI SPECL_STATUS speclSetBatterySelfTest(SP_ID                 spid,
                                              BATTERY_ID            batteryID,
                                              BOOL                  initiateSelfTest);

SPECLAPI SPECL_STATUS speclSetBatteryEnableState(SP_ID                 spid,
                                                 BOOL                  enable);

SPECLAPI SPECL_STATUS speclSetBatteryEnergyRequirements(SP_ID                        spid,
                                                        BATTERY_ENERGY_REQUIREMENTS  requirements);

SPECLAPI SPECL_STATUS speclClearBatteryFaults(SP_ID                 spid,
                                              BATTERY_ID            batteryID);

SPECLAPI SPECL_STATUS speclSetBatteryFaultLED(SP_ID             spid,
                                              BATTERY_ID        batteryID,
                                              LED_BLINK_RATE    blinkRate);

SPECLAPI SPECL_STATUS speclGetDimmStatus(PSPECL_DIMM_SUMMARY dimm_status_ptr);

SPECLAPI SPECL_STATUS speclGetMgmtStatus(PSPECL_MGMT_SUMMARY mgmt_status_ptr);

SPECLAPI SPECL_STATUS speclSetMgmtPort(SP_ID            spid,
                                       PORT_ID_TYPE     portIDType,
                                       BOOL             autoNegotiate,
                                       PORT_SPEED       portSpeed,
                                       PORT_DUPLEX      portDuplex);

SPECLAPI SPECL_STATUS speclSetMgmtFaultLED(SP_ID            spid,
                                           LED_BLINK_RATE   blinkRate);

SPECLAPI SPECL_STATUS speclSetMgmtvLanConfig(SP_ID              spid,
                                             VLAN_CONFIG_MODE   vLanConfig);

SPECLAPI SPECL_STATUS speclSetMgmtLoopDetect(SP_ID              spid,
                                             BOOL               loopDetect);

SPECLAPI SPECL_STATUS speclSetComPortMux(COM_PORT                   port,
                                     COM_PORT_MUX_SELECT_CODE   muxSelectCode);

SPECLAPI SPECL_STATUS speclGetIOModuleStatus(PSPECL_IO_SUMMARY io_status_ptr);

SPECLAPI SPECL_STATUS speclGetIndividualIOModuleStatus(SP_ID                spid,
                                                       ULONG                io_module,
                                                       PSPECL_IO_STATUS     io_status_ptr);

SPECLAPI SPECL_STATUS speclSetIOModuleFaultLED(SP_ID            spid,
                                               ULONG            io_module,
                                               LED_BLINK_RATE   blinkRate);

SPECLAPI SPECL_STATUS speclSetIOModulePowerUpMode(SP_ID spid,
                                                  ULONG io_module,
                                                  BOOL  pupEnable);

SPECLAPI SPECL_STATUS speclSetIOModulePersistedPowerState(SP_ID         spid,
                                                          ULONG         io_module,
                                                          BOOL          persistedPowerState);

SPECLAPI SPECL_STATUS speclGetBMCStatus(PSPECL_BMC_SUMMARY bmc_status_ptr);

SPECLAPI SPECL_STATUS speclSetBMCDelayedShtdnTimer(SP_ID   spid,
                                                   ULONG   timerValue);

SPECLAPI SPECL_STATUS speclSetBMCPollingMode(SP_ID    spid,
                                             ULONG    i2cBus,
                                             BOOL     pollEnable);

SPECLAPI SPECL_STATUS speclClearBMCFaults(SP_ID     spid,
                                          IPMI_FSR_FRU_TYPES   fru);

SPECLAPI SPECL_STATUS speclBMCChassisControl(SP_ID                          spid,
                                             IPMI_CHASSIS_CONTROL_ACTION    chassisControl);

SPECLAPI SPECL_STATUS speclSetVaultModeConfig(SP_ID spid,
                                              BOOL  lowPowerMode,
                                              UCHAR rideThroughTime,
                                              UCHAR vaultTimeout);

SPECLAPI SPECL_STATUS speclGetSdrStatus(PSPECL_SDR_SUMMARY  sdr_status_ptr);

SPECLAPI SPECL_STATUS speclGetSuitcaseStatus(PSPECL_SUITCASE_SUMMARY suitcase_status_ptr);

#ifdef C4_INTEGRATED
SPECLAPI SPECL_STATUS speclGetI2CStatus(PSPECL_I2C_SUMMARY   i2c_status_ptr);
SPECLAPI SPECL_STATUS speclSetChassisFault(BOOL cmi_fault, BOOL i2c_fault, BOOL other_fault);
#endif /* C4_INTEGRATED - C4HW */

SPECLAPI SPECL_STATUS speclGetFaultExpanderStatus(PSPECL_FLT_EXP_SUMMARY fault_status_ptr);

SPECLAPI SPECL_STATUS speclSetLocalFaultExpanderStatus(csx_uchar_t fault_status);

SPECLAPI SPECL_STATUS speclSetLocalVeepromCpuStatus(csx_u32_t cpu_status);

SPECLAPI SPECL_STATUS speclGetPeerSoftwareBootStatus(PSPECL_SLAVE_PORT_SUMMARY boot_status_ptr);

SPECLAPI SPECL_STATUS speclSetLocalSoftwareBootStatus(csx_uchar_t      generalStatus,
                                                      csx_uchar_t      componentStatus,
                                                      csx_uchar_t      componentExtStatus);

SPECLAPI SPECL_STATUS speclSetClearHoldInPostAndOrReset(SP_ID               spid,
                                                        BOOL                holdInPost,
                                                        BOOL                holdInReset,
                                                        BOOL                rebootBlade,
                                                        REBOOT_DRIVER_LIST  rebootDriver,
                                                        REBOOT_REASON_LIST  rebootReason,
                                                        ULONG               rebootExtendedStatus);

SPECLAPI SPECL_STATUS speclResetSP(SP_ID                spid,
                                   SPECL_RESET_TYPE     resetType,
                                   REBOOT_DRIVER_LIST   rebootDriver,
                                   REBOOT_REASON_LIST   rebootReason,
                                   ULONG                rebootExtendedStatus);

SPECLAPI SPECL_STATUS speclSetQuickBootFlag(SP_ID    spid,
                                            BOOL     flag);

SPECLAPI SPECL_STATUS speclIssuePeerNMI(void);

SPECLAPI SPECL_STATUS speclGetLEDStatus(PSPECL_LED_SUMMARY led_status_ptr);

SPECLAPI SPECL_STATUS speclSetSPFaultLED(LED_BLINK_RATE         blink_rate,
                                         ULONG                  statusCondition);

SPECLAPI SPECL_STATUS speclSetUnsafeToRemoveLED(LED_BLINK_RATE blink_rate);

SPECLAPI SPECL_STATUS speclGetCacheCardStatus(PSPECL_CACHE_CARD_SUMMARY cache_card_status_ptr);

SPECLAPI SPECL_STATUS speclSetCacheCardFaultLED(LED_BLINK_RATE blink_rate);

SPECLAPI SPECL_STATUS speclSetCacheCardPowerUpMode(BOOL pupEnable);

SPECLAPI SPECL_STATUS speclSetCacheCardPowerCtrl(BOOL pupEnable);

SPECLAPI SPECL_STATUS speclSetCacheCardPowercycle(void);

SPECLAPI SPECL_STATUS speclSetEnclosureFaultLED(LED_BLINK_RATE blink_rate);

SPECLAPI SPECL_STATUS speclSetMemoryFaultLED(LED_BLINK_RATE blink_rate);

SPECLAPI SPECL_STATUS speclSetIOModulePortLED(SP_ID             spid,
                                              ULONG             io_module,
                                              ULONG             io_port,
                                              LED_BLINK_RATE    blinkRate,
                                              LED_COLOR_TYPE    led_color);

SPECLAPI SPECL_STATUS speclGetMiscStatus(PSPECL_MISC_SUMMARY misc_status_ptr);

SPECLAPI SPECL_STATUS speclSetDriveFaultLED(ULONG           driveID,
                                            LED_BLINK_RATE  blinkRate);

SPECLAPI SPECL_STATUS speclSetDrivePowerEnable(ULONG    driveID,
                                               BOOL     powerEnable);

SPECLAPI SPECL_STATUS speclGetTemperatureStatus(PSPECL_TEMPERATURE_SUMMARY temperature_status_ptr);

SPECLAPI SPECL_STATUS speclSetMiniSetupProgress(BOOL                        utilityMode,
                                                SPECL_MINISETUP_PROGRESS    miniSetupProgress);

SPECLAPI SPECL_STATUS speclGetMiniSetupStatus(PSPECL_MINISETUP_SUMMARY minisetup_status_ptr);

SPECLAPI SPECL_STATUS speclSetClearUtilityPartitionBoot(BOOL    bootUtilityPartition);

SPECLAPI SPECL_STATUS speclSetCnaMode(SPECL_CNA_MODE    cnaMode);

SPECLAPI SPECL_STATUS speclGetLocalSFPStatus(ULONG                   io_module,
                                             ULONG                   io_port,
                                             PSPECL_SFP_STATUS       sfp_ptr);

SPECLAPI SPECL_STATUS speclSetLocalSFPTxDisable(ULONG               io_module,
                                                ULONG               io_port,
                                                BOOL                TxDisable);

SPECLAPI SPECL_STATUS speclMapPCIBusDevFuncToIOModulePortNum(ULONG                              bus,
                                                         ULONG                              device,
                                                         ULONG                              function,
                                                         ULONG                              phy_mapping,
                                                         PSPECL_PCI_TO_IO_MODULE_MAPPING    pci_mapping_ptr);

SPECLAPI SPECL_STATUS speclGetBootDeviceInfo(PSPECL_PCI_DATA            pci_address_ptr,
                                             IO_CONTROLLER_PROTOCOL*    controller_protocol_ptr);

SPECLAPI SPECL_STATUS speclGetTimeData(PSPECL_TIME_DATA time_data_ptr);

SPECLAPI SPECL_STATUS speclCvtTimeData(EMCPAL_LARGE_INTEGER *output_time_ptr,
                                       EMCPAL_LARGE_INTEGER input_time,
                                       SPECL_TIMESCALE_UNIT cvt_timescale);

SPECLAPI SPECL_STATUS speclGetElapsedTimeData(EMCPAL_LARGE_INTEGER *output_time_ptr,
                                              EMCPAL_LARGE_INTEGER input_time,
                                              SPECL_TIMESCALE_UNIT cvt_timescale);

SPECLAPI SPECL_STATUS speclGetFirmwareStatus(PSPECL_FIRMWARE_SUMMARY firmware_status_ptr);

SPECLAPI SPECL_STATUS speclGetPciStatus(PSPECL_PCI_SUMMARY pci_status_ptr);

SPECLAPI SPECL_STATUS speclGetMplxrStatus(PSPECL_MPLXR_SUMMARY mplxr_status_ptr);

SPECLAPI SPECL_STATUS speclSetLEDExpanderMux(SP_ID              spid,
                                             ULONG              io_module,
                                             ULONG              io_port,
                                             BOOL               globalBus);

SPECLAPI SPECL_STATUS speclGetSpsResume(PSPECL_SPS_RESUME sps_resume_ptr);

SPECLAPI SPECL_STATUS speclGetSpsStatus(PSPECL_SPS_SUMMARY sps_status_ptr);

SPECLAPI SPECL_STATUS speclStartSpsTest(void);

SPECLAPI SPECL_STATUS speclStopSpsTest(void);

SPECLAPI SPECL_STATUS speclShutdownSps(BOOL forceShutdown);

SPECLAPI SPECL_STATUS speclSetSpsFastSwitchover(BOOL fastSw);

SPECLAPI SPECL_STATUS speclSpsDownloadFirmware(SPS_FW_TYPE imageType, CHAR * imageBuff, ULONG len);

/* This can only be called at passive level */
SPECLAPI SPECL_STATUS speclShutdownSoftErrorHandler(void);

SPECLAPI SPECL_STATUS speclDisableSMIGeneration(ULONG io_module);

SPECLAPI SPECL_STATUS speclHaltPollingDevice(SPECL_DEVICE_CLASS dev_class,
                                             OPAQUE_32          opaque_device);

SPECLAPI SPECL_STATUS speclResumePollingDevice(SPECL_DEVICE_CLASS   dev_class,
                                               OPAQUE_32            opaque_device);

SPECLAPI SPECL_STATUS speclHaltPolling(void);

SPECLAPI SPECL_STATUS speclResumePolling(void);

SPECLAPI SPECL_STATUS speclSetSpsPresent(SP_ID spid,
                                         BOOL spsPresent);

SPECLAPI SPECL_STATUS speclClearFltstsMirrorRegion(IPMI_COMMAND            ipmiCmd);

SPECLAPI SPECL_STATUS speclMonitorPasswordResetButton(void);

//SFI common interface API.
SPECLAPI SPECL_STATUS 
speclSFISetTestingStatus(IN BOOL sfiStatus);

SPECLAPI SPECL_STATUS 
speclSFIGetTestingStatus(void);

SPECLAPI SPECL_STATUS 
speclSFIGetNSetCacheData(PSPECL_SFI_MASK_DATA psfiMaskData);

//SFI Simulation env. specific interface API
SPECLAPI SPECL_STATUS 
speclInitializeArrayAndCacheData(IN SP_ID               spid,
                                 IN SPID_PLATFORM_INFO  platforminfo);

SPECLAPI SPECL_STATUS 
speclUnInitializeArrayData(void);
//SFI End

SPECLAPI SPECL_STATUS speclInjectSensorReading(SP_ID        spid,
                                               csx_uchar_t  sensorID,
                                               csx_uchar_t  operation,
                                               csx_uchar_t  analogReading,
                                               csx_uchar_t  discrete0,
                                               csx_uchar_t  discrete1);

SPECLAPI SPECL_STATUS 
speclSFISetTransactionTimeStatus(BOOL updateStatus);

SPECLAPI SPECL_STATUS speclSetRebootLog(REBOOT_DRIVER_LIST   rebootDriver,
                                        REBOOT_REASON_LIST   rebootReason,
                                        ULONG                rebootExtendedStatus);

SPECLAPI SPECL_STATUS speclGetRebootLog(PREBOOT_LOG   pRebootLog);

#if defined(__cplusplus)
}
#endif

#endif

/***************************************************************************
 * END specl_interface.h
 ***************************************************************************/
