#ifndef __GENERIC_UTILS_LIB__
#define __GENERIC_UTILS_LIB__

/**********************************************************************
 *      Company Confidential
 *          Copyright (c) EMC Corp, 2007-2009
 *          All Rights Reserved
 *          Licensed material - Property of EMC Corp
 **********************************************************************/

#include "smbus_driver.h"
#include "csx_ext.h"
#include "specl_types.h"
#include "familyfruid.h"
#include "speeds.h"
#include "sfp_support.h"
#include "spid_types.h"
#include "rebootLogging.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**********************************************************************
 *
 *  File Name:
 *          generic_utils_lib.h
 *
 *  Description:
 *          These functions are exported for the convenience of both
 *          user-space apps and kernel components alike, to assist with
 *          handling and printing of various enumerated types.
 *
 *  Revision History:
 *          14-June-07      Phil Leef   created
 *          18-Spetember-08 Joe Ash     renamed to generic_utils_lib
 *          19-March-09     AGN         SFP diagnostics support functions
 *
 **********************************************************************/

#define MINIPORT_DRIVER_NAME_MAX_LENGTH         128

// A structure to assist in printing out the resume prom
typedef struct _RESUME_PROM_HELPER
{
    int         print_index;
    BOOL        is_char;
    BOOL        is_bytes;
    BOOL        extra_decode;
    void*       data_ptr;
    int         length;
    const char* description;
} RESUME_PROM_HELPER, *PRESUME_PROM_HELPER;

const char*
SmbTranslateErrorString(UINT_32 err_code);

SMB_DEVICE
SmbTranslateSMBusDeviceID(SP_ID         spid,
                          SMB_DEVICE    smb_device);

SMB_DEVICE
SmbRestoreSMBusDeviceID(SP_ID       spid,
                        SMB_DEVICE  local_peer_device_id);

const char*
SmbGetDeviceName (SMB_DEVICE smb_device);

const char*
decodevLANConfigMode(VLAN_CONFIG_MODE vLANConfigMode);

const char*
decodeLEDColor(LED_COLOR_TYPE color);

const char*
decodeLEDBlinkRate(LED_BLINK_RATE blink_rate);

#ifdef C4_INTEGRATED
char*
decodeLEDBlinkRate2(LED_BLINK_RATE blinkRate);
#endif /* C4_INTEGRATED - C4HW */

const char*
decodeLanPortIndex(PORT_ID_TYPE port_index);

const char*
decodeLANPortSpeed(PORT_SPEED port_speed);

const char*
decodeLANPortDuplex(PORT_DUPLEX port_duplex);

const char*
decodeFanIndex(UINT_32 fan_index);

const char*
decodeFanPackIndex(UINT_32 fanpack_index);

const char*
decodeComPort(COM_PORT port);

const char*
decodeMuxSelection(COM_PORT_MUX_SELECT_CODE muxSelection);

const char*
decodeFaultExpStatus(csx_uchar_t exp_status);

const char*
decodeCpuStatusRegister(csx_u32_t cpu_status);

const char*
decodePort80Status(csx_uchar_t port80_code);

const char*
decodeAmiPort80Status(csx_uchar_t port80_code);

const char*
decodePOSTExtStatus(UINT_16 post_ext_status);

const char*
decodePowerSupplyType(AC_DC_INPUT ps_type);

const char*
decodeLinkSpeeds(UINT_32 link_speed);

const char*
decodeProtocol(IO_CONTROLLER_PROTOCOL protocol);

const char*
decodeSFPMedia(SFP_MEDIA_TYPE sfp_media);

const char*
decodeSoftwareComponentStatus(UINT_16 sw_status);

const char*
decodeSoftwareComponentExtStatus(UINT_16 sw_ext_status);

/* Reboot Logging stuff */
const char*
decodeRebootDriver(REBOOT_DRIVER_LIST rebootDriver);

char*
decodeRebootReason(REBOOT_REASON_LIST rebootReason, ULONG rebootExtendedRebootStatus, char *return_buffer);

/* Resume Prom stuff */
const char*
decodeFamilyID(FAMILY_ID family_id);

const char*
decodeFamilyFruID(HW_MODULE_TYPE ffid);

HW_MODULE_TYPE
fixNvramFamilyFruID(HW_MODULE_TYPE ffid, PUSHORT familyID, PUSHORT fruID);

UINT_32
getResumeFieldOffset(RESUME_PROM_FIELD resume_field, ...);

UINT_32
getResumeFieldSize(RESUME_PROM_FIELD resume_field);

const char*
getResumeFieldName(RESUME_PROM_FIELD resume_field);

void
initResumeHelper(RESUME_PROM_HELPER     resume_helper[],
                 RESUME_PROM_STRUCTURE  *resume_struct);

char*
getPrettyHexFromResumeField(RESUME_PROM_FIELD   resume_field,
                            RESUME_PROM_HELPER  resume_helper[],
                            char                return_buffer[]);

const char*
decodeResumeField(RESUME_PROM_FIELD resume_field,
                  void              *data_ptr,
                  HW_MODULE_TYPE    ffid);

UINT_32
calculateResumeChecksum(RESUME_PROM_STRUCTURE *resume_struct);

const char*
decodeMiniSetupProgress(SPECL_MINISETUP_PROGRESS minisetup_progress);

const char*
decodeCnaMode(SPECL_CNA_MODE cnaMode);

const char*
decodeIntCblSts(SPECL_INTERNAL_CABLE_STATE cableState);


/* SPID stuff */
const char*
decodeSpId(SP_ID sp_id);

const char*
decodeSpidHwType(SPID_HW_TYPE hw_type);

const char*
decodeCPUModule(HW_CPU_MODULE cpu_module);

const char*
decodeEnclosureType(HW_ENCLOSURE_TYPE enclosureType);

const char* 
decodeFamilyType(HW_FAMILY_TYPE familyType);

const char* 
decodeMidplaneType(HW_MIDPLANE_TYPE midplaneType);

SPID_HW_TYPE
getSpidHwTypeFromFFID(HW_MODULE_TYPE ffid);

HW_MIDPLANE_TYPE
getSpidMidplaneTypeFromFFID(HW_MODULE_TYPE ffid);

HW_FAMILY_TYPE
getHwFamilyTypeFromHwType(SPID_HW_TYPE hw_type);

HW_CPU_MODULE
getCPUModuleFromHwType(SPID_HW_TYPE hw_type);

HW_ENCLOSURE_TYPE
getEnclosureTypeFromMidplaneType(HW_MIDPLANE_TYPE midplane_type);

void
initializePlatformInfo(HW_MODULE_TYPE spFFID, HW_MODULE_TYPE midplaneFFID, PSPID_PLATFORM_INFO platformInfo_ptr);

/* SPD DIMM stuff */
const char*
decodeSpdDimmType(SPD_DEVICE_TYPE dimmType);

ULONG
decodeSpdDimmSize(SPD_DEVICE_TYPE   dimmType,
                  ULONG             dimmSizeIdentifier);

const char*
decodeSpdDimmRevision(SPD_REVISION dimmrevision);

ULONG
decodeSpdBytesInUse(SPD_DEVICE_TYPE  dimmType,
                    ULONG            bytesInUseIdentifier);

ULONG
decodeSpdDimmBanks(SPD_DEVICE_TYPE  dimmType,
                   ULONG            bankIdentifier);

ULONG
decodeSpdDimmBankGroups(SPD_DEVICE_TYPE  dimmType,
                        ULONG            bankGroupIdentifier);

ULONG
decodeSpdDimmColAddrBits(SPD_DEVICE_TYPE    dimmType,
                         ULONG              colAddrIdentifier);

ULONG
decodeSpdDimmRowAddrBits(SPD_DEVICE_TYPE    dimmType,
                         ULONG              rowAddrIdentifier);

ULONG
decodeSpdDimmPrimaryBusWidth(SPD_DEVICE_TYPE   dimmType,
                             ULONG             devWidthIdentifier);

ULONG
decodeSpdDimmDeviceWidth(SPD_DEVICE_TYPE    dimmType,
                         ULONG              devWidthIdentifier);

ULONG
decodeSpdDimmRank(SPD_DEVICE_TYPE    dimmType,
                  ULONG              rankIdentifier);

ULONG
decodeSpdDimmDieCount(SPD_DEVICE_TYPE    dimmType,
                      ULONG              dieIdentifier);

const char*
decodeJedecIDCode(csx_uchar_t jedecID);

/* START: SFP stuff */

#define ALARMS_OR_WARNINGS_DELIMITER ";"

const char*
SfpTranslateFibreChannelSpeed(SFP_FIBRE_CHANNEL_SPEED fc_speed);

const char*
SfpTranslateSASSpeed(UINT_8E sas_speed);

const char*
SfpTranslateAlarmOrWarning(SFP_ALARM_AND_WARNING_FLAG alarmOrWarningFlag);

const char*
SfpTranslateConnectorType(UINT_8E connectorType);

const char*
SfpTranslateAlarms(SPECL_SFP_STATUS* sfpStatusPtr, UINT_16E alarms, char* outBuf);

const char*
SfpTranslateWarnings(UINT_16E warnings, char* outBuf, UINT_16E alarms, BOOL maskAlarmWarnings);

UINT_8E
decodeConnectorType(SPECL_SFP_STATUS* specl_sfp_status_ptr);

UINT_8E
decodeSFPBaseChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
decodeSFPExtChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
decodeSFPDMIChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_32E
decodeSFPEMCChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
calculateSFPBaseChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
calculateSFPExtChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
calculateSFPDMIChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_32E
calculateSFPEMCChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
validateBaseAndExtChecksums(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
validateEMCChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
checkConnectorOptical(SPECL_SFP_STATUS* specl_sfp_status_ptr);

UINT_8E
decodeComplianceCode(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_32E
getSupportedSpeeds(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
getFibreChannelSpeeds(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

const char*
decodeFibreChannelSpeeds(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

const char*
decodeSASSpeeds(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

UINT_8E
decodeMaxBitRate(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

const char*
decodeVendorName(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

const char*
decodeVendorPartNumber(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

const char*
decodeVendorPartRevision(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

const char*
decodeVendorSerialNumber(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

UINT_8E
decodeEMCHeaderFormatValue(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
checkEMCHeaderFormatValue(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
checkEMCPart(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

const char*
decodeEMCPartNumber(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

const char*
decodeEMCPartRevision(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

const char*
decodeEMCSerialNumber(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

UINT_8E
decodeOUIByte1(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
decodeOUIByte2(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_8E
decodeOUIByte3(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

const char*
decodeCiscoProductID(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

BOOL
checkEMCOUI(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
checkBrocadeOUI(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
checkCiscoProductID(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type, char* outBuf);

UINT_8E
decodeCableTech(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

BOOL
validateDMIChecksum(SPECL_SFP_STATUS* specl_sfp_status_ptr, UINT_8E connector_type);

UINT_16E
decodeSFPAlarms(SPECL_SFP_STATUS* sfpStatusPtr, UINT_8E connectorType);

UINT_16E
decodeSFPWarnings(SPECL_SFP_STATUS* sfpStatusPtr, UINT_8E connectorType);

/* END: SFP stuff */

/* Multiplexer stuff */
const char*
MplxrGetDeviceName (MPLXR_DEVICE mplxr_device);

const char*
decodeMplxrFlowControl(csx_uchar_t flowCtrl);

const char*
decodeMplxrBaudRate(csx_uchar_t baudRate);

const char*
decodeMplxrLEDConfig(csx_uchar_t ledConfig);

/* START: SFI stuff */
const char*
decodeSFICommandStatus(ULONG sfiCommandStatus);
/* END: SFI stuff */

/* START: Battery stuff */
const char*
decodeBatteryChargeState(BATTERY_CHARGE_STATE battery_charge_state);

const char*
decodeBatteryId(BATTERY_ID battery_id);

const char*
decodeBatterySelfTestResult(csx_uchar_t testResult);

const char*
decodeBatteryFaultStatus(csx_uchar_t battery_fault_status);

const char*
spsGetDeviceName (SPS_DEV sps_device);

const char*
decodeSpsState(SPS_STATE sps_state);

const char*
decodeSpsIspState(SPS_ISP_STATE sps_isp_state);

const char*
decodeSpsMode(SPS_MODE sps_mode);

const char*
decodeSpsType(SPS_TYPE sps_type);

const char*
decodeSpsFirmwareType(SPS_FW_TYPE spsFw);

const char*
decodeSpsTestStatus(SPS_TEST_STATUS sps_test_status);
/* END: Battery stuff */

/* Date stuff */
void
calculateIsoWeek(PULONG iso_year,
                 PULONG iso_week,
                 ULONG  year,
                 ULONG  month,          // 1 for Jaunary, and so on
                 ULONG  day_of_month,   // like a calendar, starts with 1
                 ULONG  day_of_week     // 0 for Sunday, and so on easy to get from functions, annoying to calculate
                 );

/* START: BMC stuff */
const char*
decodeBMCSelState(SEL_LOGGING_STATUS selState);

const char*
decodeBMCBistResult(BMC_BIST_RESULT bistResult);

const char*
decodeMfbAction(MFB_ACTION mfbAction);

const char*
decodeFirmwareUpdateStatus(SPECL_FW_UPDATE_STATUS updateStatus);

const char*
decodeNetworkMode(SPECL_NETWORK_MODE networkMode);

const char*
decodeNetworkPort(SPECL_NETWORK_PORT networkPort);
/* END: BMC stuff */

/* START: SDR stuff */
const char*
decodeSensorType(IPMI_SENSOR_TYPE type);

const char*
decodeSensorReadingType(IPMI_SDR_READING_TYPE type);

const char*
decodeSensorRecordType(IPMI_SDR_RECORD_TYPE type);

const char*
decodeSensorUnitType(IPMI_SENSOR_UNIT_TYPE type);

const char*
decodeSensorUnitDataFormat(IPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT format);

const char*
decodeSensorDirection(IPMI_SDR_SENSOR_DIRECTION dir);

const char*
decodeSensorLinearizationType(IPMI_SDR_LINEARIZATION type);

const char*
decodeSensorEntityID(IPMI_SENSOR_ENTITY_ID entityId);
/* END: SDR stuff */

/* START: IPMI stuff */
const char*
decodeIpmiCompletionCode(csx_uchar_t completionCode);

const char*
ipmiGetDeviceName (IPMI_DEV ipmi_device);

UINT_8E
getIpmiFruIdFromDevice (IPMI_DEV            ipmi_device,
                        SPID_PLATFORM_INFO  platformInfo);

BOOL
ipmiIsDeviceLocal (IPMI_DEV ipmi_device);
/* END: IPMI stuff */

const char*
decodePInternalErrorCodes(csx_u32_t errorCode);

#if defined(__cplusplus)
}
#endif

#endif // __GENERIC_UTILS_LIB__
