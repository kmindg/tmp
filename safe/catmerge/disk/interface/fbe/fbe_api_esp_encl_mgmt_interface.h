#ifndef FBE_API_ESP_ENCL_MGMT_INTERFACE_H
#define FBE_API_ESP_ENCL_MGMT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_encl_mgmt_interface.h
 ***************************************************************************
 *
 * @brief 
 *  This header file defines the FBE API for the ESP ENCL Mgmt object.  
 *   
 * @ingroup fbe_api_esp_interface_class_files  
 *   
 * @version  
 *   03/16/2010:  Created. Joe Perry  
 ****************************************************************************/

/*************************  
 *   INCLUDE FILES  
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_enclosure.h"


FBE_API_CPP_EXPORT_START

//---------------------------------------------------------------- 
// Define the top level group for the FBE Environment Service Package APIs
// ----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs 
 *
 *  @brief 
 *    This is the set of definitions for FBE ESP APIs.
 * 
 *  @ingroup fbe_api_esp_interface
 */ 
//----------------------------------------------------------------

//---------------------------------------------------------------- 
// Define the FBE API ESP ENCL Mgmt Interface for the Usurper Interface. 
// This is where all the data structures defined.  
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_encl_mgmt_interface_usurper_interface FBE API ESP ENCL Mgmt Interface Usurper Interface  
 *  @brief   
 *    This is the set of definitions that comprise the FBE API ESP ENCL Mgmt Interface class  
 *    usurper interface.  
 *  
 *  @ingroup fbe_api_classes_usurper_interface  
 *  @{  
 */ 
//----------------------------------------------------------------
typedef enum fbe_esp_encl_type_s
{
    FBE_ESP_ENCL_TYPE_INVALID     = 0,    // invalid type
    FBE_ESP_ENCL_TYPE_BUNKER,             // 15 drive sentry DPE
    FBE_ESP_ENCL_TYPE_CITADEL,            // 25 drive sentry DPE
    FBE_ESP_ENCL_TYPE_VIPER,              // 6GB 15 drive SAS DAE
    FBE_ESP_ENCL_TYPE_DERRINGER,          // 6GB 25 drive SAS DAE
    FBE_ESP_ENCL_TYPE_VOYAGER,
    FBE_ESP_ENCL_TYPE_FOGBOW, 
    FBE_ESP_ENCL_TYPE_BROADSIDE,
    FBE_ESP_ENCL_TYPE_RATCHET,            // Megatron
    FBE_ESP_ENCL_TYPE_SIDESWIPE,          // Starscream (its just like a broadside)
    FBE_ESP_ENCL_TYPE_FALLBACK,           // Jetfire 25 drive
    FBE_ESP_ENCL_TYPE_BOXWOOD,            // Beachcomber 12 drive 
    FBE_ESP_ENCL_TYPE_KNOT,               // Beachcomber 25 drive
    FBE_ESP_ENCL_TYPE_PINECONE,           // 15 drive pinecone DPE
    FBE_ESP_ENCL_TYPE_STEELJAW,           // 15 drive steeljaw DPE
    FBE_ESP_ENCL_TYPE_RAMHORN,            // 15 drive ramhorn DPE
    FBE_ESP_ENCL_TYPE_ANCHO,              // 12GB 15 drive SAS DAE
    FBE_ESP_ENCL_TYPE_VIKING,             // 120 drive DAE
    FBE_ESP_ENCL_TYPE_CAYENNE,            // 60 drive 12G DAE
    FBE_ESP_ENCL_TYPE_NAGA,               // 120 drive 12G DAE
    FBE_ESP_ENCL_TYPE_TABASCO,            // 12GB 25 drive SAS DAE
    FBE_ESP_ENCL_TYPE_PROTEUS,            // Triton ERB XPE
    FBE_ESP_ENCL_TYPE_TELESTO,            // Triton XPE
    FBE_ESP_ENCL_TYPE_CALYPSO,            // Hyperion 25 Drive DPE
    FBE_ESP_ENCL_TYPE_MIRANDA,            // Oberon/Charon 25 drive DPE
    FBE_ESP_ENCL_TYPE_RHEA,               // Oberon/Charon 12 drive DPE
    FBE_ESP_ENCL_TYPE_MERIDIAN,           // vVNX VPE
    FBE_ESP_ENCL_TYPE_UNKNOWN     = 255         
} fbe_esp_encl_type_t;

/*!********************************************************************* 
 * @enum fbe_esp_encl_state_t 
 *  
 * @brief 
 *   The ESP Encl state.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 **********************************************************************/
typedef enum fbe_esp_encl_state_s
{
    FBE_ESP_ENCL_STATE_MISSING     = 0,    // Not Present
    FBE_ESP_ENCL_STATE_OK,                 // 15 drive sentry DPE
    FBE_ESP_ENCL_STATE_FAILED
} fbe_esp_encl_state_t;

/*!********************************************************************* 
 * @enum fbe_esp_encl_fault_sym_t 
 *  
 * @brief 
 *   The ESP Encl fault symptom.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 **********************************************************************/
typedef enum fbe_esp_encl_fault_sym_s
{
    FBE_ESP_ENCL_FAULT_SYM_NO_FAULT     = 0,    
    FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL,
    FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX,
    FBE_ESP_ENCL_FAULT_SYM_CROSS_CABLED,
    FBE_ESP_ENCL_FAULT_SYM_BE_LOOP_MISCABLED,
    FBE_ESP_ENCL_FAULT_SYM_LCC_INVALID_UID,
    FBE_ESP_ENCL_FAULT_SYM_UNSUPPORTED,
    FBE_ESP_ENCL_FAULT_SYM_PS_TYPE_MIX,
    FBE_ESP_ENCL_FAULT_SYM_RP_FAULTED,
} fbe_esp_encl_fault_sym_t;

/*!*********************************************************************
 * @struct fbe_esp_bus_info_t
 *
 * @brief
 *   The ESP Bus info.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_bus_info_s
{
    fbe_u32_t   currentBusSpeed;
    fbe_u32_t   capableBusSpeed;
    fbe_u32_t   numOfEnclOnBus;
    fbe_u32_t   maxNumOfEnclOnBus;
    fbe_bool_t  maxNumOfEnclOnBusExceeded;
} fbe_esp_bus_info_t;

/*!********************************************************************* 
 * @struct fbe_esp_encl_mgmt_subencl_info_t 
 *  
 * @brief 
 *   The ESP Sub Encl info.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_encl_mgmt_subencl_info_s
{
    fbe_device_physical_location_t      location; 
} fbe_esp_encl_mgmt_subencl_info_t;


/*!********************************************************************* 
 * @struct fbe_esp_encl_mgmt_encl_info_t 
 *  
 * @brief 
 *   The ESP Encl info.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_encl_mgmt_encl_info_s
{
    fbe_bool_t            enclPresent;
    fbe_esp_encl_state_t  enclState;
    fbe_esp_encl_fault_sym_t enclFaultSymptom;
    fbe_esp_encl_type_t   encl_type; /*!< Enclosure type. */
    fbe_u32_t             lccCount;
    fbe_u32_t             psCount;
    fbe_u32_t             fanCount;
    fbe_u32_t             driveSlotCount;
    fbe_u32_t             driveSlotCountPerBank;
    fbe_u32_t             bbuCount;
    fbe_u32_t             spsCount;
    fbe_u32_t             spCount;
    fbe_u32_t             sscCount;
    fbe_u32_t             dimmCount;
    fbe_u32_t             ssdCount;
    fbe_u32_t             subEnclCount;  // The maximum number of subEnclosures.
    fbe_u32_t             currSubEnclCount;  // The current number of subEnclosures which are up.
    fbe_esp_encl_mgmt_subencl_info_t    subEnclInfo[FBE_ENCLOSURE_MAX_SUBENCLS_PER_ENCL];
    fbe_bool_t            processorEncl;
    fbe_u32_t             shutdownReason;
    fbe_bool_t            overTempWarning;
    fbe_bool_t            overTempFailure;
    fbe_u32_t             maxEnclSpeed;
    fbe_u32_t             currentEnclSpeed;
    fbe_led_status_t      enclFaultLedStatus;
    fbe_encl_fault_led_reason_t enclFaultLedReason;
    fbe_bool_t            crossCable;
    fbe_encl_display_info_t display;
    fbe_led_status_t      enclDriveFaultLeds[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
    fbe_u32_t             driveMidplaneCount;
    fbe_bool_t            dataStale;
    fbe_bool_t            environmentalInterfaceFault;
    fbe_bool_t            ignoreUnsupportedEnclosures;
} fbe_esp_encl_mgmt_encl_info_t;


typedef enum
{
    FBE_LCC_STATE_UNKNOWN = 0,
    FBE_LCC_STATE_OK,
    FBE_LCC_STATE_REMOVED,
    FBE_LCC_STATE_FAULTED,
    FBE_LCC_STATE_DEGRADED,
} fbe_lcc_state_t;

typedef enum
{
    FBE_LCC_SUBSTATE_NO_FAULT = 0,
    FBE_LCC_SUBSTATE_LCC_FAULT,
    FBE_LCC_SUBSTATE_COMPONENT_FAULT,
    FBE_LCC_SUBSTATE_FUP_FAILURE,
    FBE_LCC_SUBSTATE_RP_READ_FAILURE,
} fbe_lcc_subState_t;

/*!********************************************************************* 
 * @struct fbe_esp_encl_mgmt_lcc_info_t 
 *  
 * @brief 
 *   The ESP LCC info.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_encl_mgmt_lcc_info_s
{
    fbe_bool_t inserted;                     
    fbe_bool_t faulted;
    fbe_lcc_state_t     state;
    fbe_lcc_subState_t  subState;
    fbe_bool_t lccComponentFaulted;
    fbe_bool_t isLocal;
    fbe_u16_t       esesRevision;
    fbe_u8_t        firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t        initStrFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t        fpgaFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_bool_t      shunted;
    fbe_bool_t      expansionPortOpen; 
    fbe_lcc_type_t  lccType;
    fbe_u32_t       currentSpeed;
    fbe_u32_t       maxSpeed;
    fbe_bool_t      hasResumeProm;
    fbe_bool_t      dataStale;
    fbe_bool_t      environmentalInterfaceFault;
    fbe_bool_t      resumePromReadFailed;
    fbe_bool_t      fupFailure;
} fbe_esp_encl_mgmt_lcc_info_t;


/*!********************************************************************* 
 * @struct fbe_esp_encl_mgmt_connector_info_t 
 *  
 * @brief 
 *   The ESP Connector info.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_encl_mgmt_connector_info_s
{
    fbe_bool_t               isLocalFru;
    fbe_bool_t               inserted;                     
    fbe_cable_status_t       cableStatus;
    fbe_connector_type_t     connectorType;
    fbe_u32_t                connectorId;
} fbe_esp_encl_mgmt_connector_info_t;

typedef struct fbe_esp_encl_mgmt_ssc_info_s
{
    fbe_bool_t               inserted;
    fbe_bool_t               faulted;
} fbe_esp_encl_mgmt_ssc_info_t;

// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INFO

typedef enum
{
    FBE_ENCL_MGMT_EIR_IP_TOTAL = 0,
    FBE_ENCL_MGMT_EIR_INDIVIDUAL,
} fbe_esp_encl_mgmt_eir_info_type_t;

typedef struct fbe_esp_encl_mgmt_eir_ip_total_s
{
    fbe_u32_t                       enclCount;
    fbe_eir_input_power_sample_t    inputPowerAll;
} fbe_esp_encl_mgmt_eir_ip_total_t;

typedef struct fbe_esp_encl_mgmt_eir_encl_s
{
    fbe_device_physical_location_t  location;
    fbe_esp_indiv_encl_eir_data_t         eirInfo;
} fbe_esp_encl_mgmt_eir_encl_t;

typedef union fbe_esp_encl_mgmt_eir_u
{
    fbe_esp_encl_mgmt_eir_ip_total_t    eirEnclInputPowerTotal;
    fbe_esp_encl_mgmt_eir_encl_t        eirEnclIndividual;
} fbe_esp_encl_mgmt_eir_t;

/*!********************************************************************* 
 * @struct fbe_esp_encl_mgmt_get_eir_info_t 
 *  
 * @brief 
 *   Get the ESP EIR info.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 * @ingroup fbe_esp_encl_mgmt_get_eir_info
 **********************************************************************/
typedef struct fbe_esp_encl_mgmt_get_eir_info_s
{
    fbe_esp_encl_mgmt_eir_info_type_t   eirInfoType;    /*!< IN - typs of EIR Ifno requested */
    fbe_esp_encl_mgmt_eir_t             eirEnclInfo;
} fbe_esp_encl_mgmt_get_eir_info_t;

typedef struct fbe_esp_encl_mgmt_eir_input_power_sample_s
{
    fbe_device_physical_location_t  location;
    fbe_eir_input_power_sample_t    inputSample[FBE_SAMPLE_DATA_SIZE];
} fbe_esp_encl_mgmt_eir_input_power_sample_t;

typedef struct fbe_esp_encl_mgmt_eir_temp_sample_s
{
    fbe_device_physical_location_t  location;
    fbe_eir_temp_sample_t          airInletTempSamples[FBE_SAMPLE_DATA_SIZE];
} fbe_esp_encl_mgmt_eir_temp_sample_t;

/*!********************************************************************* 
 * @struct fbe_esp_encl_mgmt_get_encl_loc_t 
 *  
 * @brief 
 *   Get specific Enclosure location info.
 *
 * @ingroup fbe_api_esp_encl_mgmt_interface
 * @ingroup fbe_esp_encl_mgmt_get_encl_loc
 **********************************************************************/
// FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_LOCATION
typedef struct fbe_esp_encl_mgmt_get_encl_loc_s
{
    fbe_u32_t                       enclIndex;
    fbe_device_physical_location_t  location; 
} fbe_esp_encl_mgmt_get_encl_loc_t;

typedef struct fbe_esp_encl_mgmt_set_led_flash_s
{
    fbe_device_physical_location_t                      location;
    fbe_base_object_mgmt_enclosure_led_control_action_t ledControlType;
    fbe_bool_t                                          enclosure_led_flash_on;
    fbe_bool_t                                          slot_led_flash_on[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
}fbe_esp_encl_mgmt_set_led_flash_t;

typedef struct fbe_esp_encl_mgmt_modify_system_id_info_s{
    fbe_bool_t    changeSerialNumber;
    fbe_char_t    serialNumber[RESUME_PROM_PRODUCT_SN_SIZE];
    fbe_bool_t    changePartNumber;
    fbe_char_t    partNumber[RESUME_PROM_PRODUCT_PN_SIZE];
}fbe_esp_encl_mgmt_modify_system_id_info_t;

typedef struct fbe_esp_encl_mgmt_system_id_info_s{
    fbe_char_t    serialNumber[RESUME_PROM_PRODUCT_SN_SIZE];
    fbe_char_t    partNumber[RESUME_PROM_PRODUCT_PN_SIZE];
    fbe_char_t    revision[RESUME_PROM_PRODUCT_REV_SIZE];
}fbe_esp_encl_mgmt_system_id_info_t;

/*! @} */ /* end of group fbe_api_esp_encl_mgmt_interface_usurper_interface */

//---------------------------------------------------------------- 
// Define the group for the FBE API ESP ENCL Mgmt Interface.   
// This is where all the function prototypes for the FBE API ESP ENCL Mgmt. 
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_encl_mgmt_interface FBE API ESP ENCL Mgmt Interface  
 *  @brief   
 *    This is the set of FBE API ESP ENCL Mgmt Interface.   
 *  
 *  @details   
 *    In order to access this library, please include fbe_api_esp_encl_mgmt_interface.h.  
 *  
 *  @ingroup fbe_api_esp_interface_class  
 *  @{  
 */
//---------------------------------------------------------------- 
fbe_status_t FBE_API_CALL fbe_api_esp_get_bus_info(fbe_u32_t bus,
                                                   fbe_esp_bus_info_t * pBusInfo);

fbe_status_t FBE_API_CALL fbe_api_esp_get_max_be_bus_count(fbe_u32_t * pBeBusCount);
fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_total_encl_count(fbe_u32_t * pTotalEnclCount);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_encl_location(fbe_esp_encl_mgmt_get_encl_loc_t *pEnclLocationInfo);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_encl_count_on_bus(fbe_device_physical_location_t * pLocation, 
                                                                      fbe_u32_t * pEnclCountOnBus);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_encl_info(fbe_device_physical_location_t * pLocation,
                                                              fbe_esp_encl_mgmt_encl_info_t * pEnclInfo);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_encl_presence(fbe_device_physical_location_t * pLocation,
                                                                  fbe_bool_t * pEnclPresent); 

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_lcc_count(fbe_device_physical_location_t * pLocation,
                                                              fbe_u32_t * pLccCount);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_lcc_info(fbe_device_physical_location_t * pLocation,
                                                             fbe_esp_encl_mgmt_lcc_info_t * pLccInfo);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_eir_info(fbe_esp_encl_mgmt_get_eir_info_t *pGetEirInfo);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_ps_count(fbe_device_physical_location_t * pLocation, 
                                                               fbe_u32_t * pCount);
fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_fan_count(fbe_device_physical_location_t * pLocation, 
                                                               fbe_u32_t * pCount);
fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_drive_slot_count(fbe_device_physical_location_t * pLocation,
                                                              fbe_u32_t * pCount);

fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus);

fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_connector_info(fbe_device_physical_location_t * pLocation,
                                   fbe_esp_encl_mgmt_connector_info_t * pConnectorInfo);

fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_ssc_count(fbe_device_physical_location_t * pLocation, 
                                                        fbe_u32_t * pCount);

fbe_status_t FBE_API_CALL fbe_api_esp_encl_mgmt_get_ssc_info(fbe_device_physical_location_t * pLocation,
                                                             fbe_esp_encl_mgmt_ssc_info_t * pSscInfo);

fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_connector_count(fbe_device_physical_location_t * pLocation, 
                                           fbe_u32_t * pCount);

fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_flash_leds(fbe_esp_encl_mgmt_set_led_flash_t *pFlashCommand);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_eir_input_power_sample(fbe_esp_encl_mgmt_eir_input_power_sample_t *pGetPowerSampleInfo);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_eir_temp_sample(fbe_esp_encl_mgmt_eir_temp_sample_t *pGetTempSampleInfo);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_array_midplane_rp_wwn_seed(fbe_u32_t *wwn_seed);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_set_array_midplane_rp_wwn_seed(fbe_u32_t *wwn_seed);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_user_modify_wwn_seed(fbe_u32_t *wwn_seed);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(fbe_bool_t *user_modified_wwn_seed_flag);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_clear_user_modified_wwn_seed_flag(void);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_modify_system_id_info(fbe_esp_encl_mgmt_modify_system_id_info_t *pModifySystemIdInfo);

fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_system_id_info(fbe_esp_encl_mgmt_system_id_info_t *pSystemIdInfo);

fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_drive_insertedness_state(fbe_device_physical_location_t *location,
                                                   char * insertedness_str, fbe_bool_t *inserted, fbe_led_status_t driveFaultLed); 
/*! @} */ /* end of group fbe_api_esp_encl_mgmt_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE ESP Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API ESP 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class_files FBE ESP APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE ESP Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /* FBE_API_ESP_ENCL_MGMT_INTERFACE_H */

/**********************************************
 * end file fbe_api_esp_encl_mgmt_interface.h
 **********************************************/
