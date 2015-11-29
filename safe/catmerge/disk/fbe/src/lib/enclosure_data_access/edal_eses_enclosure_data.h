#ifndef EDAL_ESES_ENCLOSURE_DATA_H
#define EDAL_ESES_ENCLOSURE_DATA_H

/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/***************************************************************************
 *  edal_eses_enclosure_data.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      This module is a header file for Enclosure Data Access Library
 *      and it defined data that is in an ESES Enclosure object.
 *
 *  FUNCTIONS:
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *      16-Sep-08   Joe Perry - Created
 *
 *
 **************************************************************************/

//#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_scsi.h"
#include "edal_sas_enclosure_data.h"
#include "fbe/fbe_eses.h"

// Pack the structures to prevent padding.
#pragma pack(push, fbe_eses_encl, 1) 

// Structure for Buffer Descriptor info (enclosures, LCC's, PS's)
typedef struct fbe_eses_buffer_desc_info_s
{
    fbe_u8_t            bufferDescBufferId;
    fbe_u8_t            bufferDescBufferIdWriteable;    
    fbe_u8_t            bufferDescBufferType;
    fbe_u8_t            bufferDescBufferIndex;
    fbe_u8_t            bufferDescElogBufferID;
    fbe_u8_t            bufferDescActBufferID;
    fbe_u8_t            bufferDescActRamID;
    fbe_u8_t            bufferDescActRamIdWriteable;
    fbe_u32_t           bufferSize;
} fbe_eses_buffer_desc_info_t;

/* Following struct is a single bit data for status pages */
typedef struct fbe_eses_encl_status_flags_s
{
    fbe_u8_t    addl_status_unsupported  : 1;
    fbe_u8_t    emc_specific_unsupported : 1;
    fbe_u8_t    mode_sense_unsupported   : 1;
    fbe_u8_t    mode_select_unsupported  : 1;
    fbe_u8_t    tracePresent             : 1;
    fbe_u8_t    partialShutDown          : 1;
    fbe_u8_t    sscInsert                : 1;
    fbe_u8_t    sscFault                 : 1;
}fbe_eses_encl_status_flags_t;

//  Following struct is single bit data for enclosure and lcc.
typedef struct fbe_eses_encl_lcc_common_flags_s
{
    fbe_u8_t            enclIdent             : 1;
    fbe_u8_t            enclFailureIndication : 1;
    fbe_u8_t            enclWarningIndication : 1;
    fbe_u8_t            enclFailureRequested  : 1;
    fbe_u8_t            enclWarningRequested  : 1;
    fbe_u8_t            enclRequestFailure    : 1;    // Write Data
    fbe_u8_t            enclRequestWarning    : 1;    // Write Data
    fbe_u8_t            reserved              : 1;   
} fbe_eses_encl_lcc_common_flags_t;

//  Following struct is multi bit data for enclosure and lcc.
typedef struct fbe_eses_encl_lcc_common_data_s
{    
    fbe_u8_t            enclPowerCycleDelay   : 6;    // Write Data
    fbe_u8_t            enclPowerCycleRequest : 2;    // Write Data
    fbe_u8_t            reserved              : 2;    // Reserved
    fbe_u8_t            enclPowerOffDuration  : 6;    // Write Data    
    fbe_eses_buffer_desc_info_t             bufferDescriptorInfo;
} fbe_eses_encl_lcc_common_data_t;

// some components inherit data from Base class directly, while other have a SAS class component
/*
 * ESES Enclosure data
 */
typedef struct fbe_eses_enclosure_info_s
{
    fbe_sas_enclosure_info_t                sasEnclosureInfo;
    fbe_eses_encl_status_flags_t            esesStatusFlags;
    fbe_eses_encl_lcc_common_flags_t        esesEnclosureFlags; 
    fbe_eses_encl_lcc_common_data_t         esesEnclosureData;
    fbe_u8_t                                enclElementIndex;
    fbe_u8_t                                enclSubEnclId;
    fbe_u8_t                                enclSerialNumber[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE];
    fbe_u8_t                                enclShutdownReason;
    fbe_u32_t                               gen_code; /* generation code */
    fbe_u64_t                               enclTimeOfLastGoodStatusPage;
} fbe_eses_enclosure_info_t;

/*
 * ESES Lcc data
 */
typedef struct fbe_eses_lcc_info_s
{
    fbe_base_lcc_info_t                     baseLccInfo;
    fbe_u8_t                                lccElementIndex;
    fbe_u8_t                                lccSubEnclId;
    fbe_eses_encl_lcc_common_flags_t        esesLccFlags;
    fbe_eses_encl_lcc_common_data_t         esesLccData;    
    fbe_u8_t                                lccSerialNumber[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE]; 
    fbe_u8_t                                lccProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE]; 
    fbe_u8_t                                ecbFault  : 1; // CDES needs to report the case where one of the drive ECBs may trip causing drives to get power only from the peer PS.
    fbe_u8_t                                reserved  : 7;
    fbe_edal_fw_info_t                      fwInfo;   // TO DO: this should be in base_lcc
    fbe_edal_fw_info_t                      fwInfoExp;
    fbe_edal_fw_info_t                      fwInfoBoot;
    fbe_edal_fw_info_t                      fwInfoInit;
    fbe_edal_fw_info_t                      fwInfoFPGA;
} fbe_eses_lcc_info_t;

/*
 * ESES SPS data
 */
typedef struct fbe_eses_sps_info_s
{
    edal_base_sps_info_t                        baseSpsInfo;
    fbe_u8_t                                    spsElementIndex;
    fbe_u8_t                                    spsSideId;
    fbe_u8_t                                    spsSubEnclId;
    fbe_eses_buffer_desc_info_t                 bufferDescriptorInfo;
    fbe_u16_t                                   spsStatus;
    fbe_u16_t                                   spsBatTime;
    fbe_u8_t                                    spsSerialNumber[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE]; 
    fbe_u8_t                                    spsProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE]; 
    fbe_edal_fw_info_t                          primaryFwInfo; 
    fbe_edal_fw_info_t                          secondaryFwInfo;   
    fbe_edal_fw_info_t                          batteryFwInfo;
    fbe_u32_t                                   spsFfid;
    fbe_u32_t                                   spsBatteryFfid;
} fbe_eses_sps_info_t;

/*
 * ESES Display Characters
 */
typedef struct fbe_eses_display_info_s
{
    fbe_encl_component_general_info_t           generalDisplayInfo;
    fbe_u8_t                                    displayElementIndex;
    fbe_u8_t                                    displaySubtype : 2;
    fbe_u8_t                                    spare          : 6;
    fbe_u8_t                                    displayModeStatus;
    fbe_u8_t                                    displayMode;                // Write data
    fbe_u8_t                                    displayCharacterStatus;
    fbe_u8_t                                    displayCharacter;           // Write data
} fbe_eses_display_info_t;

/*
 * ESES Power Supply data
 */
typedef struct fbe_eses_power_supply_info_s
{
    fbe_encl_power_supply_info_t        generalPowerSupplyInfo;
    fbe_u8_t                            psElementIndex;
    fbe_u8_t                            psSubEnclId;
    fbe_u8_t                            psContainerIndex;
    fbe_eses_buffer_desc_info_t         bufferDescriptorInfo;
    fbe_u8_t                            PSSerialNumber[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE];  
    fbe_u8_t                            psProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE]; 
    fbe_edal_fw_info_t                  fwInfo;  // TO DO: this should be in base_power_supply
} fbe_eses_power_supply_info_t;

/*
 * ESES Fan/Cooling data
 */
typedef struct fbe_eses_cooling_info_s
{
    fbe_encl_cooling_info_t             generalCoolingInfo;
    fbe_u8_t                            coolingElementIndex;
    fbe_u8_t                            coolingSubEnclId;
    fbe_u8_t                            coolingContainerIndex;
    fbe_eses_buffer_desc_info_t         bufferDescriptorInfo;
    fbe_u8_t                            coolingSerialNumber[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE];     
    fbe_u8_t                            coolingProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE]; 
    fbe_edal_fw_info_t                  fwInfo;   // TO DO: This should be in base_cooling
} fbe_eses_cooling_info_t;

/*
 * ESES Temp Sensor data
 */
typedef struct fbe_eses_temp_sensor_info_s
{
    fbe_encl_temp_sensor_info_t         generalTempSensorInfo;
    fbe_u8_t                            tempSensorElementIndex;
    fbe_u8_t                            tempSensorContainerIndex;
    fbe_u8_t                            tempSensorMaxTemperature;
} fbe_eses_temp_sensor_info_t;

/*
 * ESES Drive/Slot data
 */
typedef struct fbe_eses_encl_drive_info_s
{
    fbe_encl_drive_slot_info_t  enclDriveSlotInfo;                  // general Drive-Slot info
    fbe_u8_t                    driveElementIndex;
    fbe_u8_t                    drivePhyIndex;
    fbe_u8_t                    drivePowerDownCount;
} fbe_eses_encl_drive_info_t;

/*
 * ESES Expander PHY data
 */
typedef struct fbe_eses_phy_flags_s
{
    fbe_u8_t                phyDisabled        : 1;
    fbe_u8_t                phyForceDisabled   : 1;
    fbe_u8_t                phyCarrierDetected : 1;
    fbe_u8_t                phySataSpinupHold  : 1;
    fbe_u8_t                phySpinupEnabled   : 1;
    fbe_u8_t                phyLinkReady       : 1;
    fbe_u8_t                phyReady           : 1;
    fbe_u8_t                disable            : 1;
} fbe_eses_phy_flags_t;

typedef struct fbe_eses_expander_phy_info_s
{
    fbe_sas_expander_phy_info_t         sasExpPhyInfo;
    fbe_u8_t                            phyElementIndex;
    fbe_u8_t                            expanderElementIndex;
    fbe_u8_t                            phyIdentifier;
    fbe_u8_t                            phyDisableReason;
    fbe_eses_phy_flags_t                phyFlags;
} fbe_eses_expander_phy_info_t;

/*
 * ESES Connector data
 */
typedef struct fbe_eses_connector_flags_s
{
    fbe_u8_t                connectorInsertMasked : 1;
    fbe_u8_t                connectorDisabled : 1;
    fbe_u8_t                isEntireConnector : 1;
    fbe_u8_t                primaryPort       : 1;     
    fbe_u8_t                connectorDegraded : 1;      
    fbe_u8_t                isLocalConnector  : 1;
    fbe_u8_t                clearSwap  : 1 ;
    fbe_u8_t                deviceLoggedIn    : 1;
    fbe_u8_t                illegalCable      : 1;
} fbe_eses_connector_flags_t;

typedef struct fbe_eses_encl_connector_info_s
{
    fbe_base_connector_info_t           baseConnectorInfo;
    fbe_eses_connector_flags_t          connectorFlags;
    fbe_u8_t                            connectorElementIndex;
    fbe_u8_t                            connectedComponentType;
    fbe_sas_address_t                   expanderSasAddress;  // The sas address of the expander which contains the connector.
    fbe_sas_address_t                   attachedSasAddress;  // The neighbor sas address
    fbe_u8_t                            connectorPhyIndex;
    fbe_u8_t                            connectorContainerIndex;
    fbe_u8_t                            ConnectorId ;
    fbe_u8_t                            connectorLocation;
    fbe_u8_t                            attachedSubEncId ;
    fbe_u8_t                            connectorType ;
} fbe_eses_encl_connector_info_t;

/*
 * ESES Expander data
 */
typedef struct fbe_eses_encl_expander_info_s
{
    fbe_sas_expander_info_t             sasExpanderInfo;
    fbe_sas_address_t                   expanderSasAddress;
    fbe_u8_t                            expanderElementIndex;
    fbe_u8_t                            expanderSideId;
} fbe_eses_encl_expander_info_t;


/*
 * SSC(System Status Card) data
 */
typedef struct fbe_eses_encl_ssc_info_s
{
    fbe_base_ssc_info_t                 baseSscInfo;
    fbe_u8_t                            sscElementIndex;
} fbe_eses_encl_ssc_info_t;

#pragma pack(pop, fbe_eses_encl) /* Go back to default alignment.*/


/*
 * Viper Enclosure function prototypes
 */
// Get function prototypes
fbe_edal_status_t 
eses_enclosure_access_getEnclosureBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t *returnDataPtr);

fbe_edal_status_t 
eses_enclosure_access_getLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t *returnDataPtr);

fbe_edal_status_t 
eses_enclosure_access_getExpPhyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getConnectorBool(void   *generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_bool_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_bool_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getExpPhyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getConnectorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getExpanderU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getDisplayU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u32_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u64_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getConnectorU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getExpanderU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u64_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u32_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u16_t *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getStr(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t enclosure_component,
                             fbe_u32_t length,
                             char *returnDataPtr);
fbe_edal_status_t 
eses_enclosure_access_getEnclStr(void * generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u32_t length,
                                 char *newString);
fbe_edal_status_t eses_enclosure_access_getLccStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString);
fbe_edal_status_t eses_enclosure_access_getPSStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString);                                            
fbe_edal_status_t eses_enclosure_access_getCoolingStr(void * generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString);
fbe_edal_status_t eses_enclosure_access_getPowerSupplyU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u32_t *returnDataPtr);
fbe_edal_status_t eses_enclosure_access_getSscU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u8_t *returnDataPtr);


// Set function prototypes
fbe_edal_status_t 
eses_enclosure_access_setEnclosureBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t newValue);

fbe_edal_status_t 
eses_enclosure_access_setLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_bool_t newValue);

fbe_edal_status_t 
eses_enclosure_access_setExpPhyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setConnectorBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_bool_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t component,
                              fbe_bool_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setCoolingU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setTempSensorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setExpPhyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setConnectorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setExpanderU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setDisplayU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u64_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u32_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setConnectorU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setExpanderU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u64_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t component,
                            fbe_u8_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u64_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u32_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t component,
                             fbe_u16_t newValue);
fbe_edal_status_t 
eses_enclosure_access_setEnclStr(fbe_edal_general_comp_handle_t generalDataPtr,
                               fbe_base_enclosure_attributes_t attribute,
                               fbe_u32_t length,
                               char *newString);
fbe_edal_status_t 
eses_enclosure_access_setLCCStr(fbe_edal_general_comp_handle_t generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString);
fbe_edal_status_t 
eses_enclosure_access_setPSStr(fbe_edal_general_comp_handle_t generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString);
fbe_edal_status_t 
eses_enclosure_access_setCoolingStr(fbe_edal_general_comp_handle_t generalDataPtr,
                                            fbe_base_enclosure_attributes_t attribute,
                                            fbe_u32_t length,
                                            char *newString);
fbe_edal_status_t 
eses_enclosure_access_setStr(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t enclosure_component,
                             fbe_u32_t length,
                             char *newString);

fbe_edal_status_t 
eses_enclosure_access_setPowerSupplyU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u32_t newValue);

fbe_edal_status_t 
eses_enclosure_access_setSscU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u8_t newValue);


#endif  // ifndef EDAL_ESES_ENCLOSURE_DATA_H
