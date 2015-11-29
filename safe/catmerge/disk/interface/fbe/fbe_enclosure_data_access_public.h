#ifndef FBE_ENCL_DATA_ACCESS_PUBLIC_H
#define FBE_ENCL_DATA_ACCESS_PUBLIC_H

/***************************************************************************
 * Copyright (C) EMC Corporation  2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws
 * of the United States
 ***************************************************************************/

/***************************************************************************
 *  fbe_enclosure_data_access_public.h
 ***************************************************************************
 *
 * DESCRIPTION:
 *      This module is a header file for the library that provides 
 *      access functions (get/set) for Enclosure Object data.  This 
 *      header file exposes an interface to components outside of
 *      the Physical Package that do not know anything about the
 *      various types of Enclosure objects.
 *
 *  FUNCTIONS:
 *
 *  LOCAL FUNCTIONS:
 *
 *  NOTES:
 *
 *  HISTORY:
 *      06-Aug-08   Joe Perry - Created
 *
 *
 **************************************************************************/

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"


typedef void *fbe_edal_trace_context_t ;
typedef void (* fbe_edal_trace_func_t)(fbe_edal_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));
void fbe_edal_trace_func(fbe_edal_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));

/*
 * Macro to verify if edal_status is FBE_EDAL_STATUS_OK or
 * FBE_EDAL_STATUS_OK_VAL_CHANGED
 */
#define EDAL_STAT_OK(edalStatus) (((fbe_edal_status_t) (edalStatus) == FBE_EDAL_STATUS_OK) || \
((fbe_edal_status_t)(edalStatus) == FBE_EDAL_STATUS_OK_VAL_CHANGED))

/*
 * Defines for different Enclosure Object types
 */
typedef fbe_u8_t    fbe_enclosure_types_t;

#define FBE_ENCL_TYPE_INVALID 0
#define FBE_ENCL_BASE       1
#define FBE_ENCL_FC         2
#define FBE_ENCL_SAS        3
#define FBE_ENCL_SAS_VIPER  4
#define FBE_ENCL_ESES      5
#define FBE_ENCL_SAS_MAGNUM 6
#define FBE_ENCL_SAS_CITADEL 7
#define FBE_ENCL_SAS_BUNKER 8
#define FBE_ENCL_SAS_DERRINGER 9
#define FBE_ENCL_PE         10  // Processor Enclosure
#define FBE_ENCL_SAS_VOYAGER_ICM 11  //GJF  changed from  10 in the merge
#define FBE_ENCL_SAS_VOYAGER_EE 12 // GJF changed from 11 in the merge 
#define FBE_ENCL_SAS_FALLBACK   13
#define FBE_ENCL_SAS_BOXWOOD    14
#define FBE_ENCL_SAS_KNOT       15
#define FBE_ENCL_SAS_PINECONE   16
#define FBE_ENCL_SAS_STEELJAW   17
#define FBE_ENCL_SAS_RAMHORN    18
#define FBE_ENCL_SAS_ANCHO      19
#define FBE_ENCL_SAS_VIKING_IOSXP  20
#define FBE_ENCL_SAS_VIKING_DRVSXP 21
#define FBE_ENCL_SAS_CAYENNE_IOSXP  22  
#define FBE_ENCL_SAS_CAYENNE_DRVSXP 23 
#define FBE_ENCL_SAS_NAGA_IOSXP    24
#define FBE_ENCL_SAS_NAGA_DRVSXP   25
#define FBE_ENCL_SAS_TABASCO        26
#define FBE_ENCL_SAS_CALYPSO        27
#define FBE_ENCL_SAS_MIRANDA        28
#define FBE_ENCL_SAS_RHEA           29

/*!*************************************************************************
 *  @def IS_VALID_ENCL
 *  @brief Macro to verify if enclosure type is valid.           
 ***************************************************************************/
#define IS_VALID_ENCL(enclosureType) ((enclosureType == FBE_ENCL_BASE) || \
                                       (enclosureType == FBE_ENCL_SAS) || \
                                       (enclosureType == FBE_ENCL_ESES) || \
                                       (enclosureType == FBE_ENCL_SAS_VIPER) || \
                                       (enclosureType == FBE_ENCL_SAS_MAGNUM) || \
                                       (enclosureType == FBE_ENCL_SAS_CITADEL) || \
                                       (enclosureType == FBE_ENCL_SAS_BUNKER) || \
                                       (enclosureType == FBE_ENCL_SAS_DERRINGER) || \
                                       (enclosureType == FBE_ENCL_SAS_FALLBACK) || \
                                       (enclosureType == FBE_ENCL_SAS_BOXWOOD) || \
                                       (enclosureType == FBE_ENCL_SAS_KNOT) || \
                                       (enclosureType == FBE_ENCL_SAS_PINECONE) || \
                                       (enclosureType == FBE_ENCL_SAS_STEELJAW) || \
                                       (enclosureType == FBE_ENCL_SAS_RAMHORN) || \
                                       (enclosureType == FBE_ENCL_PE)  || \
                                       (enclosureType == FBE_ENCL_SAS_VOYAGER_ICM) || \
                                       (enclosureType == FBE_ENCL_SAS_VOYAGER_EE) || \
                                       (enclosureType == FBE_ENCL_SAS_VIKING_IOSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_VIKING_DRVSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_CAYENNE_IOSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_CAYENNE_DRVSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_NAGA_IOSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_NAGA_DRVSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_ANCHO) || \
                                       (enclosureType == FBE_ENCL_SAS_TABASCO) || \
                                       (enclosureType == FBE_ENCL_SAS_CALYPSO) || \
                                       (enclosureType == FBE_ENCL_SAS_MIRANDA) || \
                                       (enclosureType == FBE_ENCL_SAS_RHEA))


/*!*************************************************************************
 *  @def IS_VALID_LEAF_ENCL
 *  @brief Macro to verify if enclosure type is valid.           
 ***************************************************************************/
#define IS_VALID_LEAF_ENCL(enclosureType) ((enclosureType == FBE_ENCL_SAS_MAGNUM) || \
                                       (enclosureType == FBE_ENCL_SAS_CITADEL) || \
                                       (enclosureType == FBE_ENCL_SAS_BUNKER) || \
                                       (enclosureType == FBE_ENCL_SAS_VIPER) || \
                                       (enclosureType == FBE_ENCL_SAS_ANCHO) || \
                                       (enclosureType == FBE_ENCL_SAS_DERRINGER) || \
                                       (enclosureType == FBE_ENCL_SAS_FALLBACK) || \
                                       (enclosureType == FBE_ENCL_SAS_BOXWOOD) || \
                                       (enclosureType == FBE_ENCL_SAS_KNOT) || \
                                       (enclosureType == FBE_ENCL_SAS_PINECONE) || \
                                       (enclosureType == FBE_ENCL_SAS_STEELJAW) || \
                                       (enclosureType == FBE_ENCL_SAS_RAMHORN) || \
                                       (enclosureType == FBE_ENCL_SAS_VOYAGER_ICM) || \
                                       (enclosureType == FBE_ENCL_SAS_VOYAGER_EE) || \
                                       (enclosureType == FBE_ENCL_SAS_CAYENNE_IOSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_CAYENNE_DRVSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_NAGA_IOSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_NAGA_DRVSXP) || \
                                       (enclosureType == FBE_ENCL_SAS_TABASCO) || \
                                       (enclosureType == FBE_ENCL_SAS_CALYPSO) || \
                                       (enclosureType == FBE_ENCL_SAS_MIRANDA) || \
                                       (enclosureType == FBE_ENCL_SAS_RHEA))

/*
 * Enumeration for Enclosure Component types
 */
typedef enum fbe_enclosure_component_types_e
{
    FBE_ENCL_ENCLOSURE,
    FBE_ENCL_LCC,
    FBE_ENCL_POWER_SUPPLY,
    FBE_ENCL_COOLING_COMPONENT,
    FBE_ENCL_DRIVE_SLOT,
    FBE_ENCL_TEMP_SENSOR,
    FBE_ENCL_CONNECTOR,
    FBE_ENCL_EXPANDER,
    FBE_ENCL_EXPANDER_PHY,
    FBE_ENCL_DISPLAY,
    FBE_ENCL_SPS,
    FBE_ENCL_SSC,
    /*The following component types only reside in the processor enclosure. */
    FBE_ENCL_IO_MODULE,    
    FBE_ENCL_BEM,
    FBE_ENCL_MGMT_MODULE,
    FBE_ENCL_MEZZANINE,
    FBE_ENCL_IO_PORT,
    FBE_ENCL_PLATFORM,
    FBE_ENCL_SUITCASE,
    FBE_ENCL_BMC,
    FBE_ENCL_MISC,
    FBE_ENCL_FLT_REG,
    FBE_ENCL_SLAVE_PORT,
    FBE_ENCL_RESUME_PROM,
    FBE_ENCL_TEMPERATURE,
    FBE_ENCL_BATTERY,
    FBE_ENCL_CACHE_CARD,
    FBE_ENCL_DIMM,
    FBE_ENCL_SSD,
    FBE_ENCL_MAX_COMPONENT_TYPE,
    FBE_ENCL_INVALID_COMPONENT_TYPE = 0xFF
} fbe_enclosure_component_types_t;

#define FBE_ENCL_FIRST_PE_COMPONENT_TYPE  FBE_ENCL_IO_MODULE
/*******************************************************************************
 *  Base Enclosure class cooling component subtype.  These values describe where
 *  the cooling component is located.
 ******************************************************************************/

/*
 * Enumeration for Attributes within an Enclosure Object
 */
typedef enum fbe_base_enclosure_attributes_e
{
    // General-Common Component Attributes
    FBE_ENCL_COMP_BASE = 0,
    FBE_ENCL_COMP_INSERTED,
    FBE_ENCL_COMP_INSERTED_PRIOR_CONFIG,
    FBE_ENCL_COMP_FAULTED,
    FBE_ENCL_COMP_POWERED_OFF,
    FBE_ENCL_COMP_TYPE,
    FBE_ENCL_COMP_ELEM_INDEX,
    FBE_ENCL_COMP_SUB_ENCL_ID,
    FBE_ENCL_COMP_FAULT_LED_ON,
    FBE_ENCL_COMP_MARKED,
    FBE_ENCL_COMP_TURN_ON_FAULT_LED,    // Write
    FBE_ENCL_COMP_MARK_COMPONENT,       // Write
    FBE_ENCL_COMP_FAULT_LED_REASON,
    FBE_ENCL_COMP_STATE_CHANGE,
    FBE_ENCL_COMP_WRITE_DATA,
    FBE_ENCL_COMP_WRITE_DATA_SENT,
    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
    FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT,
    FBE_ENCL_COMP_IS_LOCAL,
    FBE_ENCL_COMP_SUBTYPE,
    FBE_ENCL_COMP_STATUS_VALID,  
    FBE_ENCL_COMP_ADDL_STATUS, 
    FBE_ENCL_COMP_CONTAINER_INDEX,   // the containing component index in EDAL, NOT the element index.
    FBE_ENCL_COMP_SIDE_ID,
    FBE_ENCL_COMP_LOCATION,
    FBE_ENCL_COMP_POWER_CYCLE_PENDING,
    FBE_ENCL_COMP_POWER_CYCLE_COMPLETED,
    FBE_ENCL_COMP_POWER_DOWN_COUNT,
    FBE_ENCL_COMP_CLEAR_SWAP,
    FBE_ENCL_COMP_FW_INFO,
    
    // Buffer Descriptor Attributes (enclosures, LCC's, PS's)
    FBE_ENCL_BD_BUFFER_ID,
    FBE_ENCL_BD_BUFFER_ID_WRITEABLE,
    FBE_ENCL_BD_BUFFER_TYPE,
    FBE_ENCL_BD_BUFFER_INDEX,
    FBE_ENCL_BD_ELOG_BUFFER_ID,
    FBE_ENCL_BD_ACT_TRC_BUFFER_ID,
    FBE_ENCL_BD_ACT_RAM_BUFFER_ID,
    FBE_ENCL_BD_ACT_RAM_BUFFER_ID_WRITEABLE,
    FBE_ENCL_BD_BUFFER_SIZE,

    // Power Supply Attributes
    FBE_ENCL_PS_BASE = 0x1000,
    FBE_ENCL_PS_AC_FAIL,
    FBE_ENCL_PS_DC_DETECTED,
    FBE_ENCL_PS_SUPPORTED,
    FBE_ENCL_PS_INPUT_POWER_STATUS,
    FBE_ENCL_PS_INPUT_POWER,
    /* Processor Enclosure PS attributes*/
    FBE_ENCL_PS_PEER_POWER_SUPPLY_ID,
    FBE_ENCL_PS_AC_DC_INPUT,
    FBE_ENCL_PS_ASSOCIATED_SPS,
    FBE_ENCL_PS_FIRMWARE_REV,
    FBE_ENCL_PS_INTERNAL_FAN1_FAULT,
    FBE_ENCL_PS_INTERNAL_FAN2_FAULT,
    FBE_ENCL_PS_AMBIENT_OVERTEMP_FAULT,
    FBE_ENCL_PS_PEER_AVAILABLE,

    FBE_ENCL_PS_MARGIN_TEST_MODE,
    FBE_ENCL_PS_MARGIN_TEST_MODE_CONTROL,       // Write
    FBE_ENCL_PS_MARGIN_TEST_RESULTS,
    // Drive Attributes
    FBE_ENCL_DRIVE_BASE = 0x2000,
    FBE_ENCL_DRIVE_BYPASSED,
    FBE_ENCL_DRIVE_INSERT_MASKED,
    FBE_ENCL_BYPASS_DRIVE,              // Write
    FBE_ENCL_DRIVE_DEVICE_OFF,          // Write
    FBE_ENCL_DRIVE_USER_REQ_POWER_CNTL,     
    FBE_ENCL_DRIVE_DEVICE_OFF_REASON,     
    FBE_ENCL_DRIVE_LOGGED_IN,
    FBE_ENCL_DRIVE_SAS_ADDRESS,
    FBE_ENCL_DRIVE_ESES_SAS_ADDRESS,
    FBE_ENCL_DRIVE_PHY_INDEX,
    FBE_ENCL_DRIVE_SLOT_NUMBER,
    FBE_ENCL_DRIVE_BATTERY_BACKED,
    // Expander PHY Attributes
    FBE_ENCL_EXP_PHY_BASE = 0x3000,
    FBE_ENCL_EXP_PHY_DISABLED,
    FBE_ENCL_EXP_PHY_READY,
    FBE_ENCL_EXP_PHY_LINK_READY,
    FBE_ENCL_EXP_PHY_FORCE_DISABLED,
    FBE_ENCL_EXP_PHY_CARRIER_DETECTED,
    FBE_ENCL_EXP_PHY_SPINUP_ENABLED,
    FBE_ENCL_EXP_PHY_SATA_SPINUP_HOLD,
    FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX,
    FBE_ENCL_EXP_PHY_ID,
    FBE_ENCL_EXP_PHY_DISABLE,           // Write
    FBE_ENCL_EXP_PHY_DISABLE_REASON,    
    // Connector Attributes
    FBE_ENCL_CONNECTOR_BASE = 0x4000,
    FBE_ENCL_CONNECTOR_INSERT_MASKED,
    FBE_ENCL_CONNECTOR_DISABLED,
    FBE_ENCL_CONNECTOR_DEGRADED,
    FBE_ENCL_CONNECTOR_PRIMARY_PORT,
    FBE_ENCL_CONNECTOR_EXP_SAS_ADDRESS,
    FBE_ENCL_CONNECTOR_ATTACHED_SAS_ADDRESS,
    FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,    
    FBE_ENCL_CONNECTOR_ID,
    FBE_ENCL_CONNECTOR_PHY_INDEX,
    FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID,
    FBE_ENCL_CONNECTOR_DEV_LOGGED_IN,
    FBE_ENCL_CONNECTOR_ILLEGAL_CABLE,
    FBE_ENCL_CONNECTOR_TYPE,
    // Expander Attributes
    FBE_ENCL_EXP_BASE = 0x5000,
    FBE_ENCL_EXP_SAS_ADDRESS,
    // Enclosure Attributes
    FBE_ENCLOSURE_BASE = 0x6000,
    FBE_ENCL_SES_SAS_ADDRESS,
    FBE_ENCL_SMP_SAS_ADDRESS,
    FBE_ENCL_SERVER_SAS_ADDRESS,
    FBE_ENCL_UNIQUE_ID,
    FBE_ENCL_POSITION,
    FBE_ENCL_ADDRESS,
    FBE_ENCL_PORT_NUMBER,
    FBE_ENCL_MAX_SLOTS,
    FBE_ENCL_LCC_POWER_CYCLE_REQUEST,
    FBE_ENCL_LCC_POWER_CYCLE_DURATION,
    FBE_ENCL_LCC_POWER_CYCLE_DELAY,
    FBE_ENCL_TRACE_PRESENT,
    FBE_ENCL_ADDITIONAL_STATUS_PAGE_UNSUPPORTED,
    FBE_ENCL_EMC_SPECIFIC_STATUS_PAGE_UNSUPPORTED,
    FBE_ENCL_MODE_SENSE_UNSUPPORTED,
    FBE_ENCL_MODE_SELECT_UNSUPPORTED,
    FBE_ENCL_SERIAL_NUMBER,
    FBE_ENCL_SUBENCL_SERIAL_NUMBER,
    FBE_ENCL_SUBENCL_PRODUCT_ID,
    FBE_ENCL_GENERATION_CODE,
    FBE_ENCL_RESET_RIDE_THRU_COUNT,
    FBE_ENCL_TIME_OF_LAST_GOOD_STATUS_PAGE,
    FBE_ENCL_SHUTDOWN_REASON,
    FBE_ENCL_PARTIAL_SHUTDOWN,

    // Cooling Attributes
    FBE_ENCL_COOLING_BASE = 0x7000,
    /* Needs to be removed after ESP support for fan info is ready. - PHE*/
    FBE_ENCL_COOLING_MULTI_FAN_FAULT,
    // Temperature Sensor Attributes
    FBE_ENCL_TEMP_SENSOR_BASE = 0x8000,
    FBE_ENCL_TEMP_SENSOR_OT_WARNING,
    FBE_ENCL_TEMP_SENSOR_OT_FAILURE,
    FBE_ENCL_TEMP_SENSOR_MAX_TEMPERATURE,
                                            
    FBE_ENCL_TEMP_SENSOR_TEMPERATURE_VALID,                                        
    FBE_ENCL_TEMP_SENSOR_TEMPERATURE,
    // Display Attributes
    FBE_ENCL_DISPLAY_BASE = 0x9000,
    FBE_DISPLAY_MODE_STATUS,
    FBE_DISPLAY_CHARACTER_STATUS,
    FBE_DISPLAY_MODE,   //write
    FBE_DISPLAY_CHARACTER, //write

    // LCC Attributes
    FBE_LCC_BASE = 0xA000,
    FBE_LCC_EXP_FW_INFO,
    FBE_LCC_BOOT_FW_INFO,
    FBE_LCC_INIT_FW_INFO,
    FBE_LCC_FPGA_FW_INFO, 
    FBE_LCC_ECB_FAULT,
    FBE_LCC_FAULT_MASKED,
    FBE_LCC_FAULT_START_TIMESTAMP,

    // SPS Attributes
    FBE_SPS_BASE = 0xB000,
    FBE_SPS_STATUS = 0xB001,
    FBE_SPS_BATTIME,
    FBE_SPS_SECONDARY_FW_INFO,
    FBE_SPS_BATTERY_FW_INFO,
    FBE_SPS_FFID,
    FBE_SPS_BATTERY_FFID,

    // Processor Enclosure Attributes
    FBE_ENCL_PE_BASE = 0xC000,
    FBE_ENCL_PE_IO_COMP_INFO,
    FBE_ENCL_PE_IO_PORT_INFO,
    FBE_ENCL_PE_POWER_SUPPLY_INFO,
    FBE_ENCL_PE_COOLING_INFO,
    FBE_ENCL_PE_MGMT_MODULE_INFO,
    FBE_ENCL_PE_MEZZANINE_INFO,
    FBE_ENCL_PE_PLATFORM_INFO,
    FBE_ENCL_PE_MISC_INFO,   
    FBE_ENCL_PE_FLT_REG_INFO,
    FBE_ENCL_PE_SLAVE_PORT_INFO,
    FBE_ENCL_PE_SUITCASE_INFO,
    FBE_ENCL_PE_BMC_INFO,
    FBE_ENCL_PE_RESUME_PROM_INFO,
    FBE_ENCL_PE_TEMPERATURE_INFO,
    FBE_ENCL_PE_SPS_INFO,
    FBE_ENCL_PE_BATTERY_INFO,
    FBE_ENCL_PE_CACHE_CARD_INFO,
    FBE_ENCL_PE_DIMM_INFO,
    FBE_ENCL_PE_SSD_INFO,

} fbe_base_enclosure_attributes_t;

typedef enum
{
   FBE_EDAL_ATTRIBUTE_UNSUPPORTED = 0,

   FBE_EDAL_ATTRIBUTE_VALID = 1,       
                                        
   FBE_EDAL_ATTRIBUTE_INVALID = 2,     

}fbe_edal_attribute_status;

typedef enum fbe_base_enclosure_data_type_s
{
    EDAL_BOOL,
    EDAL_U8,
    EDAL_U16,
    EDAL_U32,
    EDAL_U64,
    EDAL_STR
} fbe_base_enclosure_data_type_t;

/*
 * EDAL Status - these are returned by access functions
 */
typedef enum fbe_edal_status_e
{
    FBE_EDAL_STATUS_OK = 0,
    FBE_EDAL_STATUS_OK_VAL_CHANGED,
    FBE_EDAL_STATUS_ERROR,                  // general error (non-specific)
    FBE_EDAL_STATUS_COMPONENT_NOT_FOUND,
    FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND,
    FBE_EDAL_STATUS_NULL_BLK_PTR,           // 5
    FBE_EDAL_STATUS_NULL_RETURN_PTR,
    FBE_EDAL_STATUS_NULL_COMP_TYPE_DATA_PTR,
    FBE_EDAL_STATUS_NULL_COMP_DATA_PTR,
    FBE_EDAL_STATUS_INVALID_BLK_CANARY,
    FBE_EDAL_STATUS_INVALID_COMP_CANARY,    // 10
    FBE_EDAL_STATUS_COMP_TYPE_INDEX_INVALID,
    FBE_EDAL_STATUS_COMP_TYPE_UNSUPPORTED,
    FBE_EDAL_STATUS_INSUFFICIENT_RESOURCE,
    FBE_EDAL_STATUS_UNSUPPORTED_ENCLOSURE,
    FBE_EDAL_STATUS_SIZE_MISMATCH           // 15
} fbe_edal_status_t;

// Typedef for pointer to Enclosure Data Block (used by EDAL public access methods)
typedef void*   fbe_edal_block_handle_t;

// Typedef for pointer to Enclosure Data Block (used by EDAL public access methods for debugger extensions)
typedef void*   fbe_edal_component_data_handle_t;

// Type for Overall or Individual status
typedef enum
{
    FBE_EDAL_STATUS_CHASSIS_OVERALL,
    FBE_EDAL_STATUS_CHASSIS_INDIVIDUAL,
    FBE_EDAL_STATUS_SIDE_OVERALL,
    FBE_EDAL_STATUS_INDIVIDUAL,
    FBE_EDAL_STATUS_12G_CHASSIS_OVERALL = 13,
} fbe_edal_status_type_flag_t;

typedef enum
{
    FBE_EDAL_INDICATOR_SIDE_A = 0,
    FBE_EDAL_INDICATOR_SIDE_B,
    FBE_EDAL_INDICATOR_NOT_APPLICABLE = 0xFF
} fbe_edal_side_indicator_t;

// Type of Connector
typedef enum fbe_edal_connector_type_e
{
    FBE_EDAL_UNKNOWN_CONNECTOR,
    FBE_EDAL_PRIMARY_CONNECTOR,
    FBE_EDAL_EXPANSION_CONNECTOR
} fbe_edal_connector_type_t;

// Location of Temperature Sensor. Do not change the order of them.
typedef enum fbe_edal_temp_sensor_type_e
{
    FBE_EDAL_LCCA_TEMP_SENSOR_OVERALL = 0,
    FBE_EDAL_LCCA_TEMP_SENSOR_0,
    FBE_EDAL_LCCB_TEMP_SENSOR_OVERALL,
    FBE_EDAL_LCCB_TEMP_SENSOR_0,
    FBE_EDAL_CHASSIS_TEMP_SENSOR_OVERALL,
    FBE_EDAL_CHASSIS_TEMP_SENSOR_0
} fbe_edal_temp_sensor_type_t;

// Location of Cooling Component. Do not change the order of them.
typedef enum fbe_edal_cooling_type_e
{
    FBE_EDAL_PSA_COOLING_OVERALL = 0,
    FBE_EDAL_PSA_COOLING_0,
    FBE_EDAL_PSA_COOLING_1,
    FBE_EDAL_PSB_COOLING_OVERALL,
    FBE_EDAL_PSB_COOLING_0,
    FBE_EDAL_PSB_COOLING_1,
    FBE_EDAL_CHASSIS_COOLING_OVERALL, 
} fbe_edal_cooling_type_t;
typedef enum
{
    FBE_EDAL_COOLING_0 = 0,
    FBE_EDAL_COOLING_1
} fbe_edal_cooling_index_t;

typedef enum
{
    FBE_EDAL_TEMP_SENSOR_0 = 0,
    FBE_EDAL_TEMP_SENSOR_1 = 1,
    FBE_EDAL_TEMP_SENSOR_2 = 2
} fbe_edal_temp_sensor_index_t;

/*
 * Define for types of Displays
 */
typedef enum
{
    FBE_EDAL_DISPLAY_BUS = 0,
    FBE_EDAL_DISPLAY_ENCL
} fbe_edal_display_type_t;

typedef enum
{
    FBE_EDAL_DISPLAY_BUS_0 = 0,
    FBE_EDAL_DISPLAY_BUS_1,
    FBE_EDAL_DISPLAY_ENCL_0
} fbe_edal_display_index_t;

typedef enum
{
    FBE_EDAL_CHAR_0 = 0x30,
    FBE_EDAL_CHAR_1,
    FBE_EDAL_CHAR_2,
    FBE_EDAL_CHAR_3,
    FBE_EDAL_CHAR_4,
    FBE_EDAL_CHAR_5,
    FBE_EDAL_CHAR_6,
    FBE_EDAL_CHAR_7,
    FBE_EDAL_CHAR_8,
    FBE_EDAL_CHAR_9,
} fbe_edal_char_value_t;

/* FBE_FW_TARGET_COOLING has to be the last one before FBE_FW_TARGET_MAX
 * because of subencl_fw_info[target+side] is used to save the cooling component fw info.
 * Please see fbe_eses_enclosure_get_fw_target_addr()
 */

#define FBE_FW_TARGET_MAX_SUPPORTED_PS                                 4
#define FBE_FW_TARGET_MAX_SUPPORTED_EXTERNAL_COOLING_UNIT              3

typedef enum fbe_enclosure_fw_target_e
{
    FBE_FW_TARGET_LCC_EXPANDER = 0,
    FBE_FW_TARGET_LCC_BOOT_LOADER,
    FBE_FW_TARGET_LCC_INIT_STRING,
    FBE_FW_TARGET_LCC_FPGA,
    FBE_FW_TARGET_LCC_MAIN,
    FBE_FW_TARGET_PS,
    FBE_FW_TARGET_SPS_PRIMARY = FBE_FW_TARGET_PS + FBE_FW_TARGET_MAX_SUPPORTED_PS,
    FBE_FW_TARGET_SPS_SECONDARY,
    FBE_FW_TARGET_SPS_BATTERY,
    FBE_FW_TARGET_COOLING,  
    FBE_FW_TARGET_MAX = FBE_FW_TARGET_COOLING + FBE_FW_TARGET_MAX_SUPPORTED_EXTERNAL_COOLING_UNIT,
    FBE_FW_TARGET_INVALID   = 0xFF
}fbe_enclosure_fw_target_t;

#define FBE_EDAL_FW_REV_STR_SIZE     16
typedef struct fbe_edal_fw_info_s
{
    fbe_u8_t    reserved     : 6 ;   // reserved
    fbe_u8_t    valid        : 1;    // information is valid
    fbe_u8_t    downloadable : 1;    // firmware is downloadable
    fbe_u8_t    fwRevision[FBE_EDAL_FW_REV_STR_SIZE];    // revision
} fbe_edal_fw_info_t;

/*
 * Enumeration for Resume Prom Attributes 
 * within an Processor Enclosure
 */
typedef enum fbe_pe_resume_prom_id_s
{
    FBE_PE_INVALID_RESUME_PROM = -1,

    // Resume Prom ID's
    FBE_PE_MIDPLANE_RESUME_PROM,            //0

    FBE_PE_SPA_RESUME_PROM,
    FBE_PE_SPB_RESUME_PROM,

    FBE_PE_SPA_PS0_RESUME_PROM,
    FBE_PE_SPA_PS1_RESUME_PROM,

    FBE_PE_SPB_PS0_RESUME_PROM,             //5
    FBE_PE_SPB_PS1_RESUME_PROM,

    FBE_PE_SPA_MGMT_RESUME_PROM,
    FBE_PE_SPB_MGMT_RESUME_PROM,       

    FBE_PE_SPA_MEZZANINE_RESUME_PROM,
    FBE_PE_SPB_MEZZANINE_RESUME_PROM,       //10

    FBE_PE_SPA_IO_ANNEX_RESUME_PROM,
    FBE_PE_SPB_IO_ANNEX_RESUME_PROM,

    FBE_PE_SPA_IO_SLOT0_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT1_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT2_RESUME_PROM,        //15
    FBE_PE_SPA_IO_SLOT3_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT4_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT5_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT6_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT7_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT8_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT9_RESUME_PROM,
    FBE_PE_SPA_IO_SLOT10_RESUME_PROM,

    FBE_PE_SPB_IO_SLOT0_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT1_RESUME_PROM,        //20
    FBE_PE_SPB_IO_SLOT2_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT3_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT4_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT5_RESUME_PROM,        //24
    FBE_PE_SPB_IO_SLOT6_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT7_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT8_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT9_RESUME_PROM,
    FBE_PE_SPB_IO_SLOT10_RESUME_PROM,

    //FBE_PE_CACHE_CARD_RESUME_PROM,
    FBE_PE_MAX_RESUME_PROM_IDS
}fbe_pe_resume_prom_id_t;

/*
 * ComponentOverallStatus values
 */
#define FBE_ENCL_COMPONENT_TYPE_STATUS_OK               0
#define FBE_ENCL_COMPONENT_TYPE_STATUS_WRITE_NEEDED     1

#define FBE_ENCL_COMPONENT_INDEX_ALL        0xFFFFFFFF

/* This is used for initializing the index of struct 
 * fbe_enclosure_component_accessed_data_t
 */   
#define FBE_EDAL_INDEX_INVALID       0xFFFFFFFF

//***********************  Function Prototypes *****************************

//***********************  Get Functions  **********************************
fbe_edal_status_t
fbe_edal_getEdalLocale(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t  *edalLocale);
fbe_edal_status_t
fbe_edal_get_fw_target_component_type_attr(fbe_enclosure_fw_target_t target,
                                           fbe_enclosure_component_types_t *comp_type, 
                                           fbe_base_enclosure_attributes_t *comp_attr);
fbe_edal_status_t
fbe_edal_getNumberOfComponents(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                               fbe_u32_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getComponentType(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                          fbe_u32_t index,
                          fbe_enclosure_component_types_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getOverallStateChangeCount(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                    fbe_u32_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getComponentOverallStateChangeCount(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t componentType,
                                   fbe_u32_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getComponentOverallStatus(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t component,
                                   fbe_u32_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getSpecificComponentCount(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t component,
                                   fbe_u32_t *componentCountPtr);
fbe_edal_status_t
fbe_edal_updateBlockPointers(fbe_edal_block_handle_t enclosureComponentBlockPtr);
fbe_edal_status_t
fbe_edal_connectorControl(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t   connector_id,
                          fbe_bool_t disable);
fbe_edal_status_t
fbe_edal_getEnclosureType(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_enclosure_types_t *enclosureTypePtr);
fbe_edal_status_t
fbe_edal_getEnclosureSide(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getGenerationCount(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u32_t  *generation_count);

fbe_edal_status_t
fbe_edal_setGenerationCount(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u32_t  generation_count);
                          
fbe_edal_status_t
fbe_edal_getBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u32_t index,
                 fbe_bool_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getBool_direct(fbe_edal_component_data_handle_t generalDataPtr,
                                                  fbe_enclosure_types_t  enclosureType,  
                                                  fbe_base_enclosure_attributes_t attribute,
                                                  fbe_enclosure_component_types_t component,
                                                  fbe_bool_t *returnValuePtr);



fbe_edal_status_t
fbe_edal_getU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
               fbe_base_enclosure_attributes_t attribute,
               fbe_enclosure_component_types_t component,
               fbe_u32_t index,
               fbe_u8_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getU8_direct(fbe_edal_component_data_handle_t generalDataPtr,
                                                  fbe_enclosure_types_t  enclosureType,  
                                                  fbe_base_enclosure_attributes_t attribute,
                                                  fbe_enclosure_component_types_t component,
                                                  fbe_u8_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getU16(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u16_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getU16_direct(fbe_edal_component_data_handle_t generalDataPtr,
                                                  fbe_enclosure_types_t  enclosureType,  
                                                  fbe_base_enclosure_attributes_t attribute,
                                                  fbe_enclosure_component_types_t component,
                                                  fbe_u16_t *returnValuePtr);
                                                  
fbe_edal_status_t
fbe_edal_getU32(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getU32_direct(fbe_edal_component_data_handle_t generalDataPtr,
                                                  fbe_enclosure_types_t  enclosureType,  
                                                  fbe_base_enclosure_attributes_t attribute,
                                                  fbe_enclosure_component_types_t component,
                                                  fbe_u32_t *returnValuePtr);
                                                  
fbe_edal_status_t
fbe_edal_getU64(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u64_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getU64_direct(fbe_edal_component_data_handle_t generalDataPtr,
                                                  fbe_enclosure_types_t  enclosureType,  
                                                  fbe_base_enclosure_attributes_t attribute,
                                                  fbe_enclosure_component_types_t component,
                                                  fbe_u64_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getBuffer(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t componentType,
                fbe_u32_t componentIndex,
                fbe_u32_t bufferLength,
                fbe_u8_t *bufferPtr);

fbe_edal_status_t
fbe_edal_getStr(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t length,
                char *returnStringPtr);

fbe_edal_status_t
fbe_edal_getStr_direct(fbe_edal_component_data_handle_t componentTypeDataPtr,
                fbe_enclosure_types_t  enclosureType,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t length,
                char *returnStringPtr);

fbe_edal_status_t
fbe_edal_getConnectorBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                          fbe_base_enclosure_attributes_t attribute,
                          fbe_edal_connector_type_t connectorType,
                          fbe_bool_t *returnValuePtr);

fbe_edal_status_t 
fbe_edal_getConnectorIndex (fbe_edal_block_handle_t enclosureComponentBlockPtr,
                            fbe_base_enclosure_attributes_t entireConnector,
                            fbe_base_enclosure_attributes_t localConnector,
                            fbe_base_enclosure_attributes_t primaryPort,
                            fbe_u8_t *connectorIndex);

fbe_edal_status_t
fbe_edal_getConnectorU64(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_edal_connector_type_t connectorType,
                         fbe_u64_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getDrivePhyBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_u32_t drive,
                         fbe_bool_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getDrivePhyU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_u32_t drive,
                       fbe_u8_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getDriveBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_u32_t drive_slot,
                         fbe_bool_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getDriveU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                       fbe_base_enclosure_attributes_t attribute,
                       fbe_u32_t drive_slot,
                       fbe_u8_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getTempSensorBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                           fbe_base_enclosure_attributes_t attribute,
                           fbe_edal_status_type_flag_t statusType,
                           fbe_edal_side_indicator_t sideIndicator,
                           fbe_u32_t index,
                           fbe_bool_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getTempSensorU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_edal_status_type_flag_t statusType,
                         fbe_edal_side_indicator_t sideIndicator,
                         fbe_u32_t index,
                         fbe_u8_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getTempSensorU16(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_edal_status_type_flag_t statusType,
                         fbe_edal_side_indicator_t sideIndicator,
                         fbe_u32_t index,
                         fbe_u16_t *returnValuePtr);

fbe_edal_status_t
fbe_edal_getCoolingBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                        fbe_base_enclosure_attributes_t attribute,
                        fbe_edal_status_type_flag_t statusType,
                        fbe_edal_side_indicator_t sideIndicator,
                        fbe_u32_t index,
                        fbe_bool_t *returnValuePtr);
fbe_edal_status_t
fbe_edal_getDisplayValue(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_edal_display_type_t displayType,
                         fbe_u32_t *returnValuePtr);

//***********************  Set Functions  **********************************
fbe_edal_status_t
fbe_edal_setBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u32_t index,
                 fbe_bool_t newValue);

fbe_edal_status_t
fbe_edal_setBool_direct(fbe_edal_component_data_handle_t generalDataPtr,
                 fbe_enclosure_types_t  enclosureType,  
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_bool_t newValue);



fbe_edal_status_t
fbe_edal_setU8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
               fbe_base_enclosure_attributes_t attribute,
               fbe_enclosure_component_types_t component,
               fbe_u32_t index,
               fbe_u8_t newValue);
fbe_edal_status_t
fbe_edal_setU8_direct(fbe_edal_component_data_handle_t generalDataPtr,
                 fbe_enclosure_types_t  enclosureType,  
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u8_t newValue);

fbe_edal_status_t
fbe_edal_setU16(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u16_t newValue);

fbe_edal_status_t
fbe_edal_setU16_direct(fbe_edal_component_data_handle_t generalDataPtr,
                 fbe_enclosure_types_t  enclosureType,  
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u16_t newValue);

fbe_edal_status_t
fbe_edal_setU32(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t newValue);
fbe_edal_status_t
fbe_edal_setU32_direct(fbe_edal_component_data_handle_t generalDataPtr,
                 fbe_enclosure_types_t  enclosureType,  
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u32_t newValue);

fbe_edal_status_t
fbe_edal_setU64(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u64_t newValue);

fbe_edal_status_t
fbe_edal_setU64_direct(fbe_edal_component_data_handle_t generalDataPtr,
                 fbe_enclosure_types_t  enclosureType,  
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u64_t newValue);

fbe_edal_status_t
fbe_edal_setBuffer(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t componentType,
                fbe_u32_t componentIndex,
                fbe_u32_t bufferLength,
                fbe_u8_t *bufferPtr);

fbe_edal_status_t
fbe_edal_setStr(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                fbe_base_enclosure_attributes_t attribute,
                fbe_enclosure_component_types_t component,
                fbe_u32_t index,
                fbe_u32_t length,
                char *returnStringPtr);

fbe_edal_status_t
fbe_edal_setStr_direct(fbe_edal_component_data_handle_t generalDataPtr,
                 fbe_enclosure_types_t  enclosureType,  
                 fbe_base_enclosure_attributes_t attribute,
                 fbe_enclosure_component_types_t component,
                 fbe_u32_t length,
                 char * returnStringPtr);
fbe_edal_status_t
fbe_edal_setDisplayValue(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_edal_display_type_t displayType,
                         fbe_bool_t numericValue,
                         fbe_bool_t flashDisplay,
                         fbe_u32_t newValue);

fbe_edal_status_t
fbe_edal_setDriveBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                      fbe_base_enclosure_attributes_t attribute,
                      fbe_u32_t drive_slot,
                      fbe_bool_t newValue);

fbe_edal_status_t
fbe_edal_setDrivePhyBool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                         fbe_base_enclosure_attributes_t attribute,
                         fbe_u32_t drive,
                         fbe_bool_t newValue);

fbe_edal_status_t
fbe_edal_setComponentOverallStatus(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t component,
                                   fbe_u32_t newStatus);
fbe_edal_status_t
fbe_edal_copyBlockData(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                       fbe_u8_t *targetBuffer,
                       fbe_u32_t bufferSize);

fbe_edal_status_t
fbe_edal_fixNextBlockPtr(fbe_edal_block_handle_t baseCompBlockPtr);

fbe_edal_status_t
fbe_edal_get_component_size(fbe_enclosure_types_t  enclosure_type, 
                            fbe_enclosure_component_types_t component_type,
                            fbe_u32_t * component_size_p);

fbe_edal_status_t
fbe_edal_incrementOverallStateChange(fbe_edal_block_handle_t enclosureComponentBlockPtr);

fbe_edal_status_t
fbe_edal_incrementComponentOverallStateChange(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                   fbe_enclosure_component_types_t componentType);


fbe_edal_status_t
fbe_edal_copy_backup_data(fbe_edal_block_handle_t componentBlockPtr,
                       fbe_edal_block_handle_t backupComponentBlockPtr);

fbe_bool_t
fbe_edal_is_pe_component(fbe_enclosure_component_types_t component_type);




fbe_edal_status_t
fbe_edal_setEdalLocale(fbe_edal_block_handle_t enclosureComponentBlockPtr, 
                          fbe_u8_t  edalLocale);

fbe_edal_status_t
fbe_edal_getBoolMatchingComponent(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_enclosure_component_types_t componentType,
                                  fbe_bool_t check_value,
                                  void **encl_component);

fbe_edal_status_t
fbe_edal_getU8MatchingComponent(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                fbe_base_enclosure_attributes_t attribute,
                                fbe_enclosure_component_types_t componentType,
                                fbe_u8_t check_value,
                                void **encl_component);

fbe_u32_t
fbe_edal_find_first_edal_match_U8(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                  fbe_base_enclosure_attributes_t attribute,
                                  fbe_enclosure_component_types_t component,
                                  fbe_u32_t start_index,
                                  fbe_u8_t check_value);
fbe_u32_t
fbe_edal_find_first_edal_match_bool(fbe_edal_block_handle_t enclosureComponentBlockPtr,
                                    fbe_base_enclosure_attributes_t attribute,
                                    fbe_enclosure_component_types_t component,
                                    fbe_u32_t start_index,
                                    fbe_bool_t check_value);


// jap
#if FALSE
fbe_status_t
enclosure_access_stateChangeDetected(fbe_enclosure_component_block_t *enclosureComponentTypeBlock, 
                                     fbe_u32_t currentChangeCount,
                                     fbe_u32_t *numberOfChangedComponents);
fbe_status_t
enclosure_access_componentStateChangeDetected(fbe_enclosure_component_t   *component_data,
                                              fbe_u32_t currentChangeCount,
                                              fbe_bool_t *stateChangeDetected);
#endif
// jap

//***********************  Print Functions  **********************************
void
fbe_edal_printAllComponentData(void *baseCompBlockPtr, fbe_edal_trace_func_t trace_func);
void
fbe_edal_printSpecificComponentData(void *compBlockPtr,
                                    fbe_enclosure_component_types_t componentType,
                                    fbe_u32_t componentIndex,
                                    fbe_bool_t printFullData,
                                    fbe_edal_trace_func_t trace_func);
void
fbe_edal_printFaultedComponentData(void *compBlockPtr, fbe_edal_trace_func_t trace_func);

void
fbe_edal_printAllComponentBlockInfo(void *baseCompBlockPtr);

void fbe_edal_setTraceLevel(fbe_trace_level_t traceLevel);
fbe_trace_level_t fbe_edal_getTraceLevel(void);
void fbe_edal_setCliContext(fbe_bool_t cliContext);
fbe_bool_t fbe_edal_getCliContext(void);
char * fbe_edal_print_Enclosuretype(fbe_u32_t enclosureType);

#endif  // ifndef FBE_ENCL_DATA_ACCESS_PUBLIC_H
