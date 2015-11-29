#ifndef EDAL_BASE_ENCLOSURE_DATA_H
#define EDAL_BASE_ENCLOSURE_DATA_H

/***************************************************************************
 * Copyright (C) EMC Corporation  2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/***************************************************************************
 *  edal_base_enclosure_data.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      This module is a header file for Enclosure Data Access Library
 *      and it defined data that is in a Base Enclosure object.
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

#include "fbe/fbe_enclosure_data_access_public.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

// Canary (Magic Number) values - used for validating data
#define EDAL_BLOCK_CANARY_VALUE         0xFBE0EDA1
#define EDAL_COMPONENT_CANARY_VALUE     0xE3


/*
 * Data for the Component Type Info Header
 */
typedef struct fbe_enclosure_component_s
{
    fbe_enclosure_component_types_t componentType;              // type of component (PS, drive, fan, ...)
    fbe_u32_t                       maxNumberOfComponents;      // number of this type of component saved in the local block
    fbe_u32_t                       firstComponentIndex;     // the first absolute component index based on all the blocks for the component type.
    fbe_u32_t                       componentSize;              // size of an individual component
    fbe_u32_t                       firstComponentStartOffset;  // offset from the start of current Component type data
    fbe_u32_t                       componentOverallStatus;
    fbe_u32_t                       componentOverallStateChangeCount;
} fbe_enclosure_component_t;

typedef struct fbe_enclosure_component_block_s
{
    fbe_u32_t                               enclosureBlockCanary;
    fbe_u8_t                                edalLocale;       
    fbe_enclosure_types_t                   enclosureType;
    fbe_u8_t                                numberComponentTypes;       // number of component in this block
    fbe_u8_t                                maxNumberComponentTypes;    // max number of component in this block
    fbe_u32_t                               blockSize;                  // size of current block
    fbe_u32_t                               availableDataSpace;         // free space left inside the block
    fbe_u32_t                               overallStateChangeCount;
    struct fbe_enclosure_component_block_s  *pNextBlock;                // overflow block
    fbe_u32_t                               generation_count; 
// not needed with current design (assume Component Type Data starts after this structure
//    fbe_u32_t                               enclosureComponentTypeDataOffset;   // start of Component Type data
} fbe_enclosure_component_block_t;

// Pack the structures to prevent padding.
#pragma pack(push, fbe_base_encl, 1) 

/*
 * General data that applies to all component types
 */
typedef struct fbe_encl_read_write_flags_s
{
    fbe_u8_t            componentStateChange    : 1;
    fbe_u8_t            componentStatusValid      :1;   
    fbe_u8_t            writeDataUpdate         : 1;
    fbe_u8_t            writeDataSent           : 1;
    fbe_u8_t            emcEnclCtrlWriteDataUpdate : 1;
    fbe_u8_t            emcEnclCtrlWriteDataSent   : 1;
    fbe_u8_t            spare                      : 2;
} fbe_encl_read_write_flags_t;

typedef struct fbe_encl_component_general_status_info_s
{
    fbe_u8_t                            componentCanary;
    fbe_u8_t                            addlComponentStatus;
    fbe_encl_read_write_flags_t         readWriteFlags;
    fbe_u8_t                            componentSwapCount;
} fbe_encl_component_general_status_info_t;

typedef struct fbe_encl_component_general_flags_s
{
    fbe_u8_t              componentInserted         : 1;
    fbe_u8_t              componentInsertedPriorConfig  : 1;
    fbe_u8_t              componentFaulted          : 1;
    fbe_u8_t              componentPoweredOff       : 1;
    fbe_u8_t              componentFaultLedOn       : 1;
    fbe_u8_t              componentMarked           : 1;
    fbe_u8_t              turnComponentFaultLedOn   : 1;    // Write Data
    fbe_u8_t              markComponent             : 1;    // Write Data
} fbe_encl_component_general_flags_t;

typedef struct fbe_encl_component_general_info_s
{
    fbe_encl_component_general_status_info_t    generalStatus;
    fbe_encl_component_general_flags_t          generalFlags;
} fbe_encl_component_general_info_t;

/*
 * Base Power Supply data
 */
//  Following struct is single bit data.
typedef struct fbe_encl_general_power_supply_flags_s
{
    fbe_u8_t                acFail               : 1;
    fbe_u8_t                dcDetected           : 1;
    fbe_u8_t                okToShutdown         : 1;    // Write Data
    fbe_u8_t                psSupported          : 1;
    fbe_u8_t                spare                : 4;
} fbe_encl_general_power_supply_flags_t;

//  Following struct is multi bit data.
typedef struct fbe_encl_general_ps_data_s
{
    fbe_u8_t                psSideId;
    fbe_u8_t                inputPowerStatus;
    fbe_u16_t               inputPower;
    fbe_u8_t                psMarginTestMode;
    fbe_u8_t                psMarginTestModeControl;    // Write Data
    fbe_u8_t                psMarginTestResults;
} fbe_encl_general_ps_data_t;

typedef struct fbe_encl_power_supply_info_s
{
    fbe_encl_component_general_info_t       generalComponentInfo;       // info common to all components
    fbe_encl_general_power_supply_flags_t   psFlags;
    fbe_encl_general_ps_data_t              psData;
} fbe_encl_power_supply_info_t;

/*
 * Base Fan/Cooling data
 */
//  Following struct is single bit data.
typedef struct fbe_encl_general_cooling_flags_s
{
    fbe_u8_t                multiFanFault   : 1;
    fbe_u8_t                spare           : 7;
} fbe_encl_general_cooling_flags_t;

//  Following struct is multi bit data.
typedef struct fbe_encl_general_cooling_data_s
{
    fbe_u8_t                coolingSideId;
    fbe_u8_t                coolingSubtype  : 3;    
    fbe_u8_t                spare           : 5;
} fbe_encl_general_cooling_data_t;

typedef struct fbe_encl_cooling_info_s
{
    fbe_encl_component_general_info_t   generalComponentInfo;       // info common to all components
    fbe_encl_general_cooling_flags_t     coolingFlags;
    fbe_encl_general_cooling_data_t      coolingData;
} fbe_encl_cooling_info_t;

/*
 * Base Temperature Sensor data
 */
//  Following struct is single bit data.
typedef struct fbe_encl_general_temp_sensor_flags_s
{
    fbe_u8_t                overTempFailure    : 1;
    fbe_u8_t                overTempWarning    : 1;
    fbe_u8_t                tempValid          : 1;
    fbe_u8_t                spare              : 5;
} fbe_encl_general_temp_sensor_flags_t;

//  Following struct is multi bit data.
typedef struct fbe_encl_general_temp_sensor_data_s
{    
    fbe_u8_t                tempSensorSideId;
    fbe_u16_t               temp;             
    fbe_u8_t                tempSensorSubtype  : 2;   
    fbe_u8_t                spare              : 6;
} fbe_encl_general_temp_sensor_data_t;


typedef struct fbe_encl_temp_sensor_info_s
{
    fbe_encl_component_general_info_t      generalComponentInfo;       // info common to all components
    fbe_encl_general_temp_sensor_flags_t   tempSensorFlags;
    fbe_encl_general_temp_sensor_data_t    tempSensorData;
} fbe_encl_temp_sensor_info_t;

/*
 * Base Drive/Slot data
 */
typedef struct fbe_encl_general_drive_slot_flags_s
{
    fbe_u8_t                enclDriveBypassed     : 1;
    fbe_u8_t                enclDriveLoggedIn     : 1;
    fbe_u8_t                enclDriveInsertMasked : 1;
    fbe_u8_t                bypassEnclDrive       : 1;  // Write Data
    fbe_u8_t                driveDeviceOff        : 1;  // Write Data
    fbe_u8_t                userReqPowerCntl      : 1;  
    fbe_u8_t                drivePowerCyclePending   : 1;
    fbe_u8_t                drivePowerCycleCompleted : 1;
    fbe_u8_t                driveBatteryBacked    : 1;
    fbe_u8_t                fill                  : 7;
} fbe_encl_general_drive_slot_flags_t;

typedef struct fbe_encl_drive_slot_info_s
{
    fbe_encl_component_general_info_t   generalComponentInfo;
    fbe_u8_t                            driveSlotNumber;
    fbe_u8_t                            enclDriveType;
    fbe_u8_t                            driveDeviceOffReason;
    fbe_encl_general_drive_slot_flags_t driveFlags;
} fbe_encl_drive_slot_info_t;

/*
 * Base Lcc data
 */
//  Following struct is single bit data.
typedef struct fbe_encl_general_lcc_flags_s
{
    fbe_u8_t                             isLocalLcc        : 1;
    fbe_u8_t                             lccFaultMasked    : 1;
    fbe_u8_t                             spare             : 6;
} fbe_encl_general_lcc_flags_t;

//  Following struct is multi bit data.
typedef struct fbe_encl_general_lcc_data_s
{    
    fbe_u8_t                             lccSideId;
    fbe_u8_t                             lccLocation;
    fbe_u64_t                            lccFaultStartTimestamp;
} fbe_encl_general_lcc_data_t;

typedef struct fbe_encl_lcc_info_s
{
    fbe_encl_component_general_info_t    generalComponentInfo; 
    fbe_encl_general_lcc_flags_t         lccFlags;
    fbe_encl_general_lcc_data_t          lccData;
} fbe_base_lcc_info_t;

/*
 * Base Enclosure data
 */
typedef struct fbe_base_enclosure_info_s
{
    fbe_encl_component_general_info_t    generalEnclosureInfo;
    fbe_u8_t                        enclPosition;
    fbe_u8_t                        enclAddress;
    fbe_u8_t                        enclPortNumber;
    fbe_u8_t                        enclSideId;
    fbe_u32_t                       enclResetRideThruCount;
    fbe_u64_t                       enclFaultLedReason;
    fbe_u8_t                        enclMaxSlot;
} fbe_base_enclosure_info_t;

/*
 * Base Connector Data
 */
typedef struct fbe_base_connector_info_s
{
    fbe_encl_component_general_info_t    generalComponentInfo;
} fbe_base_connector_info_t;

/*
 * Base IO Module/IO Annex Info.
 */
typedef struct edal_base_io_comp_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_io_comp_info_t;

/*
 * Base IO Module/IO Annex Info.
 */
typedef struct edal_base_io_port_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_io_port_info_t;

/*
 * Base Management Module Info.
 */
typedef struct edal_base_mgmt_module_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_mgmt_module_info_t;

/*
 * Base Mezzanine Info.
 */
typedef struct edal_base_mezzanine_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_mezzanine_info_t;

/*
 * Base BMC Info.
 */
typedef struct edal_base_bmc_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_bmc_info_t;

/*
 * Base Cache Card Info.
 */
typedef struct edal_base_cache_card_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_cache_card_info_t;

/*
 * Base DIMM Info.
 */
typedef struct edal_base_dimm_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_dimm_info_t;

/*
 * Base SSD Info.
 */
typedef struct edal_base_ssd_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_ssd_info_t;

/*
 * Base Suitcase Info.
 */
typedef struct edal_base_suitcase_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_suitcase_info_t;

/*
 * Base Platform Info.
 */
typedef struct edal_base_platform_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_platform_info_t;

/*
 * Base Misc Info.
 */
typedef struct edal_base_misc_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_misc_info_t;

/*
 * Base Fault Expander Info.
 */
typedef struct edal_base_flt_exp_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_flt_exp_info_t;

/*
 * Base Slave Port Info.
 */
typedef struct edal_base_slave_port_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_slave_port_info_t;

/*
 * Resume Prom Info.
 */
typedef struct edal_base_resume_prom_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_resume_prom_info_t;

/*
 * SPS.
 */
typedef struct fbe_encl_general_sps_info_s
{
    fbe_u16_t                                   spsStatus;
    fbe_u16_t                                   spsBatTime;
} fbe_encl_general_sps_info_t;

typedef struct edal_base_sps_info_s
{
    fbe_encl_component_general_info_t       generalComponentInfo;
    fbe_encl_general_sps_info_t             spsData;
} edal_base_sps_info_t;

/*
 * Battery.
 */
typedef struct edal_base_battery_info_s
{
    fbe_encl_component_general_info_t        generalComponentInfo;
} edal_base_battery_info_t;


/*
 * Base SSC(System Status Card).
 */
typedef struct fbe_base_ssc_info_s
{
    fbe_encl_component_general_info_t    generalComponentInfo;
} fbe_base_ssc_info_t;

#pragma pack(pop, fbe_base_encl) /* Go back to default alignment.*/

// Typedef for pointer to Component Data Block (used by EDAL private access methods)
typedef void*   fbe_edal_general_comp_handle_t;


/*
 * Base Enclosure function prototypes
 */
// Get function prototypes
fbe_edal_status_t
fbe_base_enclosure_access_getSpecificComponent(fbe_enclosure_component_block_t *componentBlockPtr,
                                               fbe_enclosure_component_types_t componentType,
                                               fbe_u32_t index,
                                               void **encl_component);
fbe_edal_status_t
fbe_base_enclosure_access_getSpecificComponent_with_sanity_check(fbe_enclosure_component_block_t *componentBlockPtr,
                                               fbe_enclosure_component_types_t componentType,
                                               fbe_u32_t index,
                                               void **encl_component);


fbe_edal_status_t
enclosure_access_getEnclosureComponentTypeData
    (fbe_enclosure_component_block_t *baseCompBlockPtr, 
     fbe_enclosure_component_t **enclosureComponentTypeDataPtr);


fbe_edal_status_t
enclosure_access_getSpecifiedComponentTypeDataWithIndex
    (fbe_enclosure_component_block_t *baseCompBlockPtr, 
     fbe_enclosure_component_t **enclosureComponentTypeDataPtr,
     fbe_enclosure_component_types_t componentType,
     fbe_u32_t comonentIndex);

fbe_edal_status_t
enclosure_access_getSpecifiedComponentTypeDataInLocalBlock
    (fbe_enclosure_component_block_t *localCompBlockPtr, 
     fbe_enclosure_component_t **enclosureComponentTypeDataPtr,
     fbe_enclosure_component_types_t componentType);

fbe_edal_status_t
enclosure_access_getSpecifiedComponentData(fbe_enclosure_component_t *enclosureComponentTypeDataPtr,
                                           fbe_u32_t index,
                                           void **componentDataPtr);
fbe_edal_status_t
enclosure_access_getComponentStateChangeCount(fbe_enclosure_component_t *enclosureComponentTypeDataPtr,
                                              fbe_u32_t *stateChangeCountPtr);

fbe_edal_status_t 
base_enclosure_access_component_status_data_ptr(fbe_edal_general_comp_handle_t generalDataPtr,
                                                fbe_enclosure_component_types_t componentType,
                                                fbe_encl_component_general_info_t   **genComponentDataPtr,
                                                fbe_encl_component_general_status_info_t  **genStatusDataPtr);

fbe_edal_status_t 
base_enclosure_access_getBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_bool_t *returnDataPtr);
fbe_edal_status_t 
base_enclosure_access_getU8(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
base_enclosure_access_getU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t componentType,
                             fbe_u16_t *returnDataPtr);
fbe_edal_status_t 
base_enclosure_access_getU64(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u64_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getU32(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t *returnDataPtr);

fbe_edal_status_t
base_enclosure_access_getStr(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_u32_t length,
                              char *returnDataPtr);

fbe_edal_status_t
base_enclosure_access_getLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t *returnDataPtr);
fbe_edal_status_t 
base_enclosure_access_getPowerSupplyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_bool_t *returnDataPtr);
fbe_edal_status_t 
base_enclosure_access_getCoolingBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_bool_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getTempSensorBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getPowerSupplyU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u16_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getCoolingU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getTempSensorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getTempSensorU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u16_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getConnectorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t *returnDataPtr);
fbe_edal_status_t 
base_enclosure_access_getEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u32_t *returnDataPtr);
fbe_edal_status_t 
base_enclosure_access_getEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_u64_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getDriveBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_bool_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getSscU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u8_t *returnDataPtr);

fbe_edal_status_t 
base_enclosure_access_getLccU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_u64_t *returnDataPtr);

// Set function prototypes
fbe_edal_status_t 
base_enclosure_access_setBool(fbe_edal_general_comp_handle_t generalDataPtr,
                              fbe_base_enclosure_attributes_t attribute,
                              fbe_enclosure_component_types_t componentType,
                              fbe_bool_t newValue);
fbe_edal_status_t 
base_enclosure_access_setU8(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u8_t newValue);
fbe_edal_status_t 
base_enclosure_access_setU16(fbe_edal_general_comp_handle_t generalDataPtr,
                             fbe_base_enclosure_attributes_t attribute,
                             fbe_enclosure_component_types_t componentType,
                             fbe_u16_t newValue);
fbe_edal_status_t 
base_enclosure_access_setU64(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u64_t newValue);
fbe_edal_status_t 
base_enclosure_access_setU32(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u32_t newValue);
fbe_edal_status_t 
base_enclosure_access_setEnclosureU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u64_t newValue);


fbe_edal_status_t 
base_enclosure_access_setStr(fbe_edal_general_comp_handle_t generalDataPtr,
                            fbe_base_enclosure_attributes_t attribute,
                            fbe_enclosure_component_types_t componentType,
                            fbe_u32_t length,
                            char * newValue);
fbe_edal_status_t 
base_enclosure_access_setPowerSupplyBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                         fbe_base_enclosure_attributes_t attribute,
                                         fbe_bool_t newValue);
fbe_edal_status_t 
base_enclosure_access_setCoolingBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_bool_t newValue);
fbe_edal_status_t 
base_enclosure_access_setTempSensorBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t newValue);

fbe_edal_status_t
base_enclosure_access_setLccBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_bool_t newValue);
fbe_edal_status_t 
base_enclosure_access_setDriveU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t newValue);
fbe_edal_status_t 
base_enclosure_access_setEnclosureU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue);
fbe_edal_status_t 
base_enclosure_access_setLccU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                     fbe_base_enclosure_attributes_t attribute,
                                     fbe_u8_t newValue);
fbe_edal_status_t 
base_enclosure_access_setPowerSupplyU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue);
fbe_edal_status_t 
base_enclosure_access_setPowerSupplyU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u16_t newValue);
fbe_edal_status_t 
base_enclosure_access_setCoolingU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t newValue);
fbe_edal_status_t 
base_enclosure_access_setTempSensorU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                 fbe_base_enclosure_attributes_t attribute,
                                 fbe_u8_t newValue);
fbe_edal_status_t 
base_enclosure_access_setTempSensorU16(fbe_edal_general_comp_handle_t generalDataPtr,
                                       fbe_base_enclosure_attributes_t attribute,
                                       fbe_u16_t newValue);
fbe_edal_status_t 
base_enclosure_access_setEnclosureU32(fbe_edal_general_comp_handle_t generalDataPtr,
                                      fbe_base_enclosure_attributes_t attribute,
                                      fbe_u32_t newValue);
fbe_edal_status_t 
base_enclosure_access_setDriveU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_u64_t newValue);
fbe_edal_status_t 
base_enclosure_access_setDriveBool(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_bool_t newValue);

fbe_edal_status_t 
base_enclosure_access_setSscU8(fbe_edal_general_comp_handle_t generalDataPtr,
                                   fbe_base_enclosure_attributes_t attribute,
                                   fbe_u8_t newValue);

fbe_edal_status_t 
base_enclosure_access_setLccU64(fbe_edal_general_comp_handle_t generalDataPtr,
                                        fbe_base_enclosure_attributes_t attribute,
                                        fbe_u64_t newValue);

fbe_edal_status_t
base_enclosure_Increment_ComponentSwapCount(fbe_edal_component_data_handle_t generalDataPtr);
// Print function prototypes
// jap
//void
//enclosure_access_printUpdatedComponentData(fbe_enclosure_component_block_t *baseCompBlockPtr,
//                                           fbe_enclosure_types_t enclosureType,
//                                           fbe_u32_t currentChangeCount);
char*
enclosure_access_printComponentType(fbe_enclosure_component_types_t componentType);
char*
enclosure_access_printAttribute(fbe_base_enclosure_attributes_t attribute);
void
enclosure_access_printSpecificComponentData(fbe_enclosure_component_t   *componentTypeDataPtr,
                                            fbe_enclosure_types_t enclosureType,
                                            fbe_u32_t componentIndex,
                                            fbe_bool_t printFullData,
                                            fbe_edal_trace_func_t trace_func);
void
enclosure_access_printSpecificComponentDataInAllBlocks(fbe_enclosure_component_block_t   *baseCompBlockPtr,
                                            fbe_enclosure_component_types_t componentType,
                                            fbe_u32_t componentIndex,
                                            fbe_bool_t printFullData,
                                            fbe_edal_trace_func_t trace_func);

/*
 * Inline functions
 */
// The following two function are associated with dealing with the offset between the
// start of the Enclosure data & the start of Component Type data
static __forceinline 
fbe_enclosure_component_t *
edal_calculateComponentTypeDataStart(fbe_enclosure_component_block_t *compBlockPtr)
{
    return ((fbe_enclosure_component_t *)
        ((fbe_u8_t *)compBlockPtr + sizeof(fbe_enclosure_component_block_t)));
}

static __forceinline 
fbe_u32_t
edal_calculateComponentTypeDataOffset(fbe_enclosure_component_block_t *compBlockPtr)
{
    return ((fbe_u32_t)sizeof(fbe_enclosure_component_t));
}


fbe_edal_status_t
fbe_edal_init_block(fbe_edal_block_handle_t edal_block_handle, 
                         fbe_u32_t block_size, 
                         fbe_u8_t enclosure_type,
                         fbe_u8_t number_of_component_types);


fbe_edal_status_t
fbe_edal_can_individual_component_fit_in_block(fbe_edal_block_handle_t edal_block_handle, 
                                         fbe_enclosure_component_types_t component_type);

fbe_edal_status_t
fbe_edal_fit_component(fbe_enclosure_component_block_t *component_block,
                       fbe_enclosure_component_types_t componentType,
                       fbe_u32_t number_elements,
                       fbe_u32_t componentTypeSize);
fbe_edal_status_t
fbe_edal_fit_component_in_local_block(fbe_enclosure_component_block_t *local_block,
                       fbe_enclosure_component_types_t componentType,
                       fbe_u32_t firstComponentIndex,
                       fbe_u32_t numberOfElements,
                       fbe_u32_t componentSize);
fbe_edal_status_t
fbe_edal_append_block(fbe_enclosure_component_block_t *component_block,
                      fbe_enclosure_component_block_t *new_block);



fbe_enclosure_component_block_t *
fbe_edal_drop_the_tail_block(fbe_enclosure_component_block_t *component_block);


#endif  // ifndef EDAL_BASE_ENCLOSURE_DATA_H
