#ifndef FBE_ENCLOSURE_H
#define FBE_ENCLOSURE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_enclosure.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   10/13/2008:  Created file header. NC
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_devices.h"
#include "fbe_pe_types.h"
#include "fbe/eses_interface.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_sps_info.h"

/*!*************************************************************************
 *  @defgroup fbe_enclosure_class FBE Enclosure Class
 *     This is the set of definitions for the enclosure classes.
 *
 *  @ingroup fbe_classes
 *
 ***************************************************************************/ 

/*!*************************************************************************
 *  @defgroup fbe_enclosure_control_interface Enclosure Usurper Interface
 *     This is the set of definitions that comprise the enclosure class
 *  usurper interface.
 *
 *  @ingroup fbe_classes_usurper_interface
 *
 * \{
 *
 ***************************************************************************/ 

/*!*************************************************************************
 *  @def FBE_BACKEND_INVALID
 *  @brief Invalid backend
 ***************************************************************************/ 
#define FBE_BACKEND_INVALID 0xFFFFFFFF

/*!*************************************************************************
 *  @def FBE_SLOT_NUMBER_INVALID
 *  @brief Invalid slot number
 ***************************************************************************/ 
#define FBE_SLOT_NUMBER_INVALID      FBE_INVALID_SLOT_NUM

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_NUMBER_INVALID
 *  @brief Invalid enclosure number
 ***************************************************************************/ 
#define FBE_ENCLOSURE_NUMBER_INVALID FBE_INVALID_ENCL_NUM

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_VALUE_INVALID
 *  @brief 8-bit -1
 ***************************************************************************/ 
#define FBE_ENCLOSURE_VALUE_INVALID  FBE_INVALID_ENCL_NUM
#define FBE_ENCLOSURE_BUF_ID_UNSPECIFIED 0xFF
#define FBE_ENCLOSURE_DEBUG_ALL_ELEMS 0xFF

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_CONTAINER_INDEX_X
 *  @brief 8-bit 
 ***************************************************************************/ 
#define FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED 0xFE
#define FBE_ENCLOSURE_CONTAINER_INDEX_INVALID 0xFF

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS
 *  @brief Define for maximum number of drive slots in an enclosure
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS         120  // Should be tied to FBE_ENCLOSURE_SLOTS

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_NUMBER_OF_PS
 *  @brief Define for maximum number of PS's in an enclosure
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_NUMBER_OF_PS                  2

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_SUBENCLS_PER_ENCL
 *  @brief Define for maximum number of sub enclosure in an enclosure
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_SUBENCLS_PER_ENCL             4  // if this changes, change FBE_ESP_MAX_SUBENCLS_PER_ENCL too.

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN
 *  @brief Define maximum number of bytes for drive state abbreviation string
 ***************************************************************************/ 
#define FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN           6 

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_EXPANSION_PORT_COUNT
 *  @brief Define maximum number of expansion ports for the enclosure.
 ***************************************************************************/ 
#define FBE_ENCLOSURE_MAX_EXPANSION_PORT_COUNT           2 
 
/*!*************************************************************************
 *  @def ENCL_STAT_OK
 *  @brief Macro to verify if enclosure status is FBE_ENCLOSURE_STATUS_OK or 
           FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED
 ***************************************************************************/
#define ENCL_STAT_OK(encl_status) (((fbe_enclosure_status_t)(encl_status) == FBE_ENCLOSURE_STATUS_OK) || \
((fbe_enclosure_status_t)(encl_status) == FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED))

/*!*************************************************************************
 *  @def IS_VALID_LEAF_ENCL_CLASS
 *  @brief Macro to verify if class id is valid leaf class id.           
 ***************************************************************************/
#define IS_VALID_LEAF_ENCL_CLASS(class_id) ((class_id == FBE_CLASS_ID_SAS_VIPER_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE) ||\
                                       (class_id == FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE) ||\
                                       (class_id == FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE)||\
                                       (class_id == FBE_CLASS_ID_SAS_VOYAGER_EE_LCC) ||\
                                       (class_id == FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE)||\
                                       (class_id == FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC) ||\
                                       (class_id == FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE)||\
                                       (class_id == FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC) ||\
                                       (class_id == FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE)||\
                                       (class_id == FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC) ||\
                                       (class_id == FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE) ||\
                                       (class_id == FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE) ||\
                                       (class_id == FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE) ||\
                                       (class_id == FBE_CLASS_ID_SAS_KNOT_ENCLOSURE) ||\
                                       (class_id == FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_RHEA_ENCLOSURE) || \
                                       (class_id == FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE))

/*!*************************************************************************
 *  @enum fbe_enclosure_type_t
 *  @brief 
 *    This defines all the valid enclosure types.
 *  FBE_ENCLOSURE_TYPE_LAST is always the last in the list.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCLOSURE_TYPE_INVALID,
    /*! Fibre channel enclosure */
    FBE_ENCLOSURE_TYPE_FIBRE,
    /*! SAS backend enclosure */
    FBE_ENCLOSURE_TYPE_SAS,

    FBE_ENCLOSURE_TYPE_LAST
}fbe_enclosure_type_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_cooling_subtype_t
 *  @brief 
 *    This defines all subtypes of the cooling components. 
 *    There are only 3 bits for the subtype.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCL_COOLING_SUBTYPE_INVALID        = 0,
    FBE_ENCL_COOLING_SUBTYPE_STANDALONE     = 1,
    FBE_ENCL_COOLING_SUBTYPE_FANPACK        = 2,
    FBE_ENCL_COOLING_SUBTYPE_PS_INTERNAL    = 3,
    FBE_ENCL_COOLING_SUBTYPE_CHASSIS        = 4,
    FBE_ENCL_COOLING_SUBTYPE_LCC_INTERNAL  = 5,
} fbe_enclosure_cooling_subtype_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_temp_sensor_subtype_t
 *  @brief 
 *    This defines all subtypes of the temp sensors. 
 *    There are only 2 bits for the subtype.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCL_TEMP_SENSOR_SUBTYPE_INVALID       = 0,
    FBE_ENCL_TEMP_SENSOR_SUBTYPE_LCC           = 1,
    FBE_ENCL_TEMP_SENSOR_SUBTYPE_CHASSIS       = 2 ,
    FBE_ENCL_TEMP_SENSOR_SUBTYPE_PS = 3 
} fbe_enclosure_temp_sensor_subtype_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_display_subtype_t
 *  @brief 
 *    This defines all subtypes of the displays. 
 *    There are only 2 bits for the subtype.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCL_DISPLAY_SUBTYPE_INVALID       = 0,
    FBE_ENCL_DISPLAY_SUBTYPE_ONE_DIGIT     = 1,
    FBE_ENCL_DISPLAY_SUBTYPE_TWO_DIGIT     = 2    
} fbe_enclosure_display_subtype_t;

/*!*************************************************************************
 *  @enum fbe_base_enclosure_control_code_t
 *  @brief 
 *    This defines all the valid control code for enclosure object.
 *  The control code is accepted by usurper functions.
 ***************************************************************************/ 
typedef enum 
{
    FBE_BASE_ENCLOSURE_CONTROL_CODE_INVALID =   FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_ENCLOSURE),

    /*! This opcode allows us to get enclosure number. 
     *  @ref  fbe_base_enclosure_mgmt_get_enclosure_number_t "fbe_base_enclosure_mgmt_get_enclosure_number_t" 
     */
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER,

    /*! This opcode allows us to get a component id. 
     *  @ref  fbe_base_enclosure_mgmt_get_component_id_t "fbe_base_enclosure_mgmt_get_component_id_t" 
     */
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_COMPONENT_ID,
    /*! This opcode allows us to get slot number of a specified client. 
     *  @ref  fbe_base_enclosure_mgmt_get_slot_number_t "fbe_base_enclosure_mgmt_get_slot_number_t" 
     */
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ELEM_GROUP_INFO,
    
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MAX_ELEM_GROUPS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MGMT_ADDRESS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_FAULT_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SCSI_ERROR_INFO,
    /*! This opcode allows us to get a copy of enclosure information blob. 
     *  @ref  fbe_base_object_mgmt_get_enclosure_info_t "fbe_base_object_mgmt_get_enclosure_info_t" 
     */
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_RESUME_PROM_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATE,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_SETUP_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_EIR_INFO,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_UNBYPASS_PARENT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_BYPASS_PARENT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_BYPASS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_UNBYPASS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANSION_PORT_BYPASS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANSION_PORT_UNBYPASS,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_READ_STATE,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_DRIVE_FAULT_LED,            // currently not used (only in bullet/SAS)
    FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_CONTROL,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_LEDS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_RESET_ENCLOSURE_SHUTDOWN_TIMER,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_ENCLOSURE_DEBUG_CONTROL,

    // commands for controling Drives & Expander
    FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_DEBUG_CONTROL,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_POWER_CONTROL,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_CONTROL,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_TEST_MODE_CONTROL,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_STRING_OUT_CONTROL,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_OUT_CONTROL,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_PS_MARGIN_TEST_CONTROL,

    // commands for FBE_CLI actions
    FBE_BASE_ENCLOSURE_CONTROL_CODE_LED_STATUS_CONTROL,

    /*! This opcode defines firmware download operation and related context/status. 
     *  @ref  fbe_enclosure_mgmt_download_op_t "fbe_enclosure_mgmt_download_op_t" 
     */
    FBE_BASE_ENCLOSURE_CONTROL_CODE_FIRMWARE_OP,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DL_INFO,

    /*! The following two opcodes allows us to get the trace buffer info status or
     *  set the control of it. 
     *  @ref  fbe_enclosure_mgmt_ctrl_op_t "fbe_enclosure_mgmt_ctrl_op_t" 
     */
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_TRACE_BUF_INFO_CTRL,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_READ_BUF,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_DEBUG_CMD,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_WRITE,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_RCV_DIAG,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_INQUIRY,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_MODE_SENSE,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_TO_PHY_MAPPING,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATISTICS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_CLEAR_ENCLOSURE_STATISTICS,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_IN_CONTROL,
    FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DRIVE_PHY_INFO,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_COUNT,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_PARSE_IMAGE_HEADER,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_COUNT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_STAND_ALONE_LCC_COUNT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_START_SLOT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_COUNT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCL_TYPE,
    
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_COUNT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DRIVE_SLOT_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_COUNT_PER_BANK,
    
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SHUTDOWN_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_COUNT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_COUNT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCL_SIDE,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_NUMBER_OF_DRIVE_MIDPLANE, 

    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DISPLAY_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_OVERTEMP_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_MANUF_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_SPS_COMMAND,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CHASSIS_COOLING_FAULT_STATUS,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_PRESENT,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_BATTERY_BACKED_INFO,
    FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_FAILED,

    FBE_BASE_ENCLOSURE_CONTROL_CODE_LAST
}fbe_base_enclosure_control_code_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_fault_symptom_t
 *  @brief 
 *    This defines the fault symptoms for an enclosure.
 ***************************************************************************/ 
typedef enum
{
    FBE_ENCL_FLTSYMPT_NONE = 0x0000,
    // enclosure status fault symptoms
    FBE_ENCL_FLTSYMPT_DUPLICATE_UID = 0x0001,
    FBE_ENCL_FLTSYMPT_ALLOCATED_MEMORY_INSUFFICIENT,
    FBE_ENCL_FLTSYMPT_PROCESS_PAGE_FAILED,
    FBE_ENCL_FLTSYMPT_CMD_FAILED,
    FBE_ENCL_FLTSYMPT_ENCL_FUNC_UNSUPPORTED,        // 0x05
    FBE_ENCL_FLTSYMPT_PARAMETER_INVALID,
    FBE_ENCL_FLTSYMPT_HARDWARE_ERROR,
    FBE_ENCL_FLTSYMPT_CDB_REQUEST_FAILED,
    FBE_ENCL_FLTSYMPT_LIFECYCLE_FAILED,
    FBE_ENCL_FLTSYMPT_EDAL_FAILED,
    FBE_ENCL_FLTSYMPT_CONFIG_INVALID,               //0xb
    FBE_ENCL_FLTSYMPT_MAPPING_FAILED,
    FBE_ENCL_FLTSYMPT_COMP_UNSUPPORTED,
    FBE_ENCL_FLTSYMPT_ESES_PAGE_INVALID,
    FBE_ENCL_FLTSYMPT_CTRL_CODE_UNSUPPORTED,        //0x0f
    FBE_ENCL_FLTSYMPT_DATA_ILLEGAL,
    FBE_ENCL_FLTSYMPT_BUSY,
    FBE_ENCL_FLTSYMPT_DATA_ACCESS_FAILED,
    FBE_ENCL_FLTSYMPT_UNSUPPORTED_PAGE_HANDLED,
    FBE_ENCL_FLTSYMPT_MODE_SELECT_NEEDED,
    FBE_ENCL_FLTSYMPT_MEM_ALLOC_FAILED,             //0x15
    FBE_ENCL_FLTSYMPT_PACKET_FAILED,
    FBE_ENCL_FLTSYMPT_EDAL_NOT_NEEDED,
    FBE_ENCL_FLTSYMPT_TARGET_NOT_FOUND,
    FBE_ENCL_FLTSYMPT_ELEM_GROUP_INVALID,
    FBE_ENCL_FLTSYMPT_PATH_ATTR_UNAVAIL,
    FBE_ENCL_FLTSYMPT_CHECKSUM_ERROR,               //0x1b
    FBE_ENCL_FLTSYMPT_ILLEGAL_REQUEST,
    FBE_ENCL_FLTSYMPT_BUILD_CDB_FAILED,              
    FBE_ENCL_FLTSYMPT_BUILD_PAGE_FAILED,
    FBE_ENCL_FLTSYMPT_NULL_PTR,                     //0x1f
    FBE_ENCL_FLTSYMPT_INVALID_CANARY,
    FBE_ENCL_FLTSYMPT_EDAL_ERROR,
    FBE_ENCL_FLTSYMPT_COMPONENT_NOT_FOUND,
    FBE_ENCL_FLTSYMPT_ATTRIBUTE_NOT_FOUND,           
    FBE_ENCL_FLTSYMPT_COMP_TYPE_INDEX_INVALID,
    FBE_ENCL_FLTSYMPT_COMP_TYPE_UNSUPPORTED,        //0x25
    FBE_ENCL_FLTSYMPT_UNSUPPORTED_ENCLOSURE,
    FBE_ENCL_FLTSYMPT_INSUFFICIENT_RESOURCE,
    FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED,
    FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED,
    FBE_ENCL_FLTSYMPT_ENCL_BELOW_MIN_REV,           //0x2a
    FBE_ENCL_FLTSYMPT_INVALID_ENCL_PLATFORM_TYPE,
    FBE_ENCL_FLTSYMPT_INVALID_SN,
    FBE_ENCL_FLTSYMPT_INVALID_SES_ENCL_DEPTH,
    FBE_ENCL_FLTSYMPT_MODE_CMD_UNSUPPORTED,
    FBE_ENCL_FLTSYMPT_MINIPORT_FAULT_ENCL,          //0x2f
    FBE_ENCL_FLTSYMPT_UPSTREAM_ENCL_FAULTED,
    FBE_ENCL_FLTSYMPT_INVALID_ESES_VERSION_DESCRIPTOR,
    FBE_ENCL_FLTSYMPT_ILLEGAL_CABLE_INSERTED,
    FBE_ENCL_FLTSYMPT_EXCEEDS_MAX_LIMITS,

    // scsi error related fault symptoms

    // add new fault sysmptoms here
    FBE_ENCL_FLTSYMPT_INVALID
} fbe_enclosure_fault_symptom_t;

/*!*************************************************************************
 *  @var typedef fbe_u32_t fbe_enclosure_position_t
 *  @brief type definition for fbe_enclosure_position_t
 ***************************************************************************/ 
typedef fbe_u32_t fbe_enclosure_position_t;

/*!************************************************************************* 
 * @def FBE_ENCLOSURE_POSITION_INVALID 
 *  @brief This is an invalid identifier for the fbe_enclosure_position_t type. 
 ***************************************************************************/
#define FBE_ENCLOSURE_POSITION_INVALID 0xffffffff

/*!*************************************************************************
 *  @var typedef fbe_u32_t fbe_number_of_slots_t
 *  @brief type definition for fbe_number_of_slots_t
 ***************************************************************************/ 
typedef fbe_u32_t fbe_number_of_slots_t;

/*!*************************************************************************
 *  @var typedef fbe_u32_t fbe_number_of_component_ids_t
 *  @brief type definition for fbe_number_of_component_ids_t
 *  This is how many edge expanders there are in this enclosure.
 ***************************************************************************/ 
typedef fbe_u32_t fbe_number_of_component_ids_t;


/*!********************************************************************* 
 * @struct fbe_base_enclosure_mgmt_get_enclosure_number_t
 *  
 * @brief 
 *   Enclosure number we return with control code of
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER. 
 **********************************************************************/
typedef struct 
{
    /*! enclosure number - OUT */
    fbe_enclosure_number_t enclosure_number;
}fbe_base_enclosure_mgmt_get_enclosure_number_t;

/*!********************************************************************* 
 * @struct fbe_base_enclosure_mgmt_get_component_id_t
 *  
 * @brief 
 *   Component id we return with control code of
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_COMPONENT_ID.
 **********************************************************************/
typedef struct 
{
    /*! component_id number - OUT */
    fbe_component_id_t component_id;
}fbe_base_enclosure_mgmt_get_component_id_t;


/*!********************************************************************* 
 * @struct fbe_base_enclosure_mgmt_get_slot_number_t
 *  
 * @brief 
 *   Enclosure slot number returned with control code of
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER. 
 **********************************************************************/
typedef struct 
{
    /*! client id - IN */
    fbe_object_id_t client_id;
    /*! slot number - OUT */
    fbe_enclosure_slot_number_t enclosure_slot_number;
}fbe_base_enclosure_mgmt_get_slot_number_t;

/* FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MGMT_ADDRESS */
typedef struct 
{
    fbe_address_t    address;     /* OUT */
}fbe_base_enclosure_mgmt_get_mgmt_address_t;

// the Get/Set Enclosure Info buffer size should be defined for the largest
// ***** if new enclosure types are added, check for the largest buffer size *****
/*
 * Viper Enclosure Defines
 */
// Size of Enclosure Data blocks for Viper
#define VIPER_ENCL_DATA_BLOCK_SIZE          FBE_MEMORY_CHUNK_SIZE
// Number of blocks used for Enclosure Data
#define VIPER_ENCL_NUMBER_OF_BLOCKS         2
// Total size of Enclosure Data for Viper
#define VIPER_ENCL_TOTAL_DATA_SIZE          (FBE_MEMORY_CHUNK_SIZE * VIPER_ENCL_NUMBER_OF_BLOCKS)

#define FBE_ENCLOSURE_INFO_SIZE             VIPER_ENCL_TOTAL_DATA_SIZE


/*!*************************************************************************
 *  @enum fbe_enclosure_status_t
 *  @brief 
 *    This defines all the enclosure status.
 *  FBE_ENCLOSURE_STATUS_LAST is always the last in the list.
 ***************************************************************************/ 
typedef enum {
    FBE_ENCLOSURE_STATUS_OK                               = 0x0000,
    FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT    = 0x0001,
    FBE_ENCLOSURE_STATUS_PROCESS_PAGE_FAILED,
    FBE_ENCLOSURE_STATUS_CMD_FAILED,
    FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED,
    FBE_ENCLOSURE_STATUS_PARAMETER_INVALID,                 //0x05-general failure attributed to a an input parameter field
    FBE_ENCLOSURE_STATUS_HARDWARE_ERROR,
    FBE_ENCLOSURE_STATUS_CDB_REQUEST_FAILED,
    FBE_ENCLOSURE_STATUS_LIFECYCLE_FAILED,
    FBE_ENCLOSURE_STATUS_EDAL_FAILED,
    FBE_ENCLOSURE_STATUS_EDAL_CORRUPTED,                    //0x0a - not used?
    FBE_ENCLOSURE_STATUS_CONFIG_INVALID,
    FBE_ENCLOSURE_STATUS_MAPPING_FAILED,
    FBE_ENCLOSURE_STATUS_COMP_UNSUPPORTED,
    FBE_ENCLOSURE_STATUS_ESES_PAGE_INVALID,
    FBE_ENCLOSURE_STATUS_CTRL_CODE_UNSUPPORTED,             //0x0f
    FBE_ENCLOSURE_STATUS_DATA_ILLEGAL,
    FBE_ENCLOSURE_STATUS_BUSY,
    FBE_ENCLOSURE_STATUS_DATA_ACCESS_FAILED,
    FBE_ENCLOSURE_STATUS_UNSUPPORTED_PAGE_HANDLED,
    FBE_ENCLOSURE_STATUS_MODE_SELECT_NEEDED,                //0x14
    FBE_ENCLOSURE_STATUS_MEM_ALLOC_FAILED,
    FBE_ENCLOSURE_STATUS_PACKET_FAILED,
    FBE_ENCLOSURE_STATUS_EDAL_NOT_NEEDED,
    FBE_ENCLOSURE_STATUS_TARGET_NOT_FOUND,
    FBE_ENCLOSURE_STATUS_ELEM_GROUP_INVALID,
    FBE_ENCLOSURE_STATUS_OK_VAL_CHANGED,                    //0x1a
    FBE_ENCLOSURE_STATUS_PATH_ATTR_UNAVAIL,
    FBE_ENCLOSURE_STATUS_CHECKSUM_ERROR,
    FBE_ENCLOSURE_STATUS_ILLEGAL_REQUEST,
    FBE_ENCLOSURE_STATUS_BUILD_CDB_FAILED,              // building the cdb failed (typically request size check)
    FBE_ENCLOSURE_STATUS_BUILD_PAGE_FAILED,             // 0x1f - failure when building details of the send/recieve diag page
    FBE_ENCLOSURE_STATUS_NULL_PTR,
    FBE_ENCLOSURE_STATUS_INVALID_CANARY,
    FBE_ENCLOSURE_STATUS_EDAL_ERROR,
    FBE_ENCLOSURE_STATUS_COMPONENT_NOT_FOUND,
    FBE_ENCLOSURE_STATUS_ATTRIBUTE_NOT_FOUND,               //0x24
    FBE_ENCLOSURE_STATUS_COMP_TYPE_INDEX_INVALID,
    FBE_ENCLOSURE_STATUS_COMP_TYPE_UNSUPPORTED,
    FBE_ENCLOSURE_STATUS_UNSUPPORTED_ENCLOSURE,
    FBE_ENCLOSURE_STATUS_INSUFFICIENT_RESOURCE,
    FBE_ENCLOSURE_STATUS_MORE_PROCESSING_REQUIRED,
    FBE_ENCLOSURE_STATUS_INCOMPATIBLE_CDES_VERSION,
    FBE_ENCLOSURE_STATUS_PROCESSING_TUNNEL_CMD,
    FBE_ENCLOSURE_STATUS_TUNNELED_CMD_FAILED,
    FBE_ENCLOSURE_STATUS_TUNNEL_DL_FAILED,
    /* Add new status here */

    FBE_ENCLOSURE_STATUS_INVALID                          = 0xFFFF
} fbe_enclosure_status_t;


/*!*************************************************************************
 *  @enum fbe_enclosure_expander_t
 *  @brief 
 *    This defines all the LCCs.
 *    NOTE: This enum should be removed after EDAL wrapper function is created. -- PINGHE
 ***************************************************************************/ 
typedef enum {
    FBE_EXPANDER_A = 0,
    FBE_EXPANDER_B = 1,
    FBE_EXPANDER_MAX = 2
}fbe_enclosure_expander_t;

/*!*************************************************************************
 *  @enum fbe_eses_encl_device_off_reason_t
 *  @brief 
 *    This defines all the reasons that the device is powered off.
 ***************************************************************************/ 
typedef enum {
    // Do not change the value for FBE_ENCL_POWER_OFF_REASON_INVALID.
    FBE_ENCL_POWER_OFF_REASON_INVALID        = 0,  
    FBE_ENCL_POWER_OFF_REASON_POWER_ON       = 1,  
    FBE_ENCL_POWER_OFF_REASON_NONPERSIST     = 2,
    FBE_ENCL_POWER_OFF_REASON_PERSIST        = 3,
    FBE_ENCL_POWER_OFF_REASON_POWER_SAVE     = 4,
    FBE_ENCL_POWER_OFF_REASON_POWER_SAVE_PASSIVE     = 5,
    FBE_ENCL_POWER_OFF_REASON_HW             = 6,   
}fbe_eses_encl_device_off_reason_t;

/*!*************************************************************************
 *  @enum fbe_eses_encl_phy_disable_reason_t
 *  @brief 
 *    This defines all the reasons that the device is powered off.
 ***************************************************************************/ 
typedef enum {
    // Do not change the value for FBE_ESES_ENCL_PHY_DISABLE_REASON_INVALID.
    FBE_ESES_ENCL_PHY_DISABLE_REASON_INVALID         = 0,  
    FBE_ESES_ENCL_PHY_DISABLE_REASON_ENABLED         = 1,  
    FBE_ESES_ENCL_PHY_DISABLE_REASON_NONPERSIST      = 2,
    FBE_ESES_ENCL_PHY_DISABLE_REASON_PERSIST         = 3,
    FBE_ESES_ENCL_PHY_DISABLE_REASON_HW        = 4,
}fbe_eses_encl_phy_disable_reason_t;

/*!********************************************************************* 
 * @struct fbe_drive_phy_info_t 
 *  
 * @brief 
 *   Contains all the drive phy info. 
 *   @ref FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DRIVE_PHY_INFO. 
 **********************************************************************/
typedef struct fbe_drive_phy_info_s
{
    fbe_u8_t               phy_id;
    fbe_sas_address_t      expander_sas_address;
    fbe_port_number_t      port_number;
    fbe_enclosure_number_t enclosure_number;
    fbe_u32_t              slot_number;
}
fbe_drive_phy_info_t;

/*!********************************************************************* 
 * @struct fbe_base_object_mgmt_get_enclosure_basic_info_t
 *  
 * @brief 
 *   Basic Enclosure information that is needed for LCC_POLL type command
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO. 
 **********************************************************************/
typedef struct 
{
    fbe_u64_t           enclSasAddress;
    fbe_u32_t           enclChangeCount;
    fbe_u32_t           enclTimeSinceLastGoodStatus;
    fbe_u32_t           enclFaultSymptom;
    fbe_u8_t            enclUniqueId[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE+1];
    fbe_u8_t            enclPosition;
    fbe_u8_t            enclSideId;
    fbe_u8_t            enclRelatedExpanders;
}fbe_base_object_mgmt_get_enclosure_basic_info_t;

/*!********************************************************************* 
 * @struct fbe_base_object_mgmt_get_enclosure_info_t
 *  
 * @brief 
 *   Enclosure information blob returned with control code of
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_INFO. 
 *   Right now, we have assumed everything fits within 1K.
 *   But we may need to increase it in the future.
 **********************************************************************/
typedef struct 
{
    /*! Memory (1K) to hold enclosure information blob - OUT */
    fbe_u8_t    enclosureInfo[FBE_ENCLOSURE_INFO_SIZE];
}fbe_base_object_mgmt_get_enclosure_info_t;
typedef struct 
{
    fbe_u8_t    enclosureprvInfo[FBE_MEMORY_CHUNK_SIZE];    
}fbe_base_object_mgmt_get_enclosure_prv_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_CONTROL
typedef struct fbe_base_object_mgmt_set_enclosure_control_s{
    fbe_u8_t    enclosureInfo[FBE_ENCLOSURE_INFO_SIZE];
}fbe_base_object_mgmt_set_enclosure_control_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_SETUP_INFO
typedef struct fbe_base_object_mgmt_get_enclosure_setup_info_s{
    fbe_u32_t   enclosureBufferSize;
} fbe_base_object_mgmt_get_enclosure_setup_info_t;

typedef enum 
{
    FBE_ENCLOSURE_FLASH_ALL_DRIVE_SLOTS,   //Flash all drive slots
    FBE_ENCLOSURE_FLASH_DRIVE_SLOT_LIST,   //Flash multiple specified drive slots
    FBE_ENCLOSURE_FLASH_ENCL_FLT_LED,      //Flash the enclosure fault LED
}fbe_base_object_mgmt_enclosure_led_control_action_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_LEDS
typedef struct fbe_base_object_mgmt_set_enclosure_led_s {
    fbe_bool_t      flashLedsActive;
    fbe_bool_t      flashLedsOn;
} fbe_base_object_mgmt_set_enclosure_led_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_ENCLOSURE_DEBUG_CONTROL
typedef enum
{
    FBE_ENCLOSURE_ACTION_NONE = 0,
    FBE_ENCLOSURE_ACTION_REMOVE,
    FBE_ENCLOSURE_ACTION_INSERT
} fbe_base_object_mgmt_encl_dbg_action_t;
typedef struct
{
    fbe_base_object_mgmt_encl_dbg_action_t  enclDebugAction;
} fbe_base_object_mgmt_encl_dbg_ctrl_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_DEBUG_CONTROL
typedef enum
{
    FBE_DRIVE_ACTION_NONE = 0,
    FBE_DRIVE_ACTION_REMOVE,
    FBE_DRIVE_ACTION_INSERT
} fbe_base_object_mgmt_drv_dbg_action_t;
typedef struct
{
    fbe_u8_t                                driveCount;
    fbe_base_object_mgmt_drv_dbg_action_t   driveDebugAction[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
} fbe_base_object_mgmt_drv_dbg_ctrl_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_POWER_CONTROL
typedef enum
{
    FBE_DRIVE_POWER_ACTION_NONE = 0,
    FBE_DRIVE_POWER_ACTION_POWER_DOWN,
    FBE_DRIVE_POWER_ACTION_POWER_UP,
    FBE_DRIVE_POWER_ACTION_POWER_CYCLE
} fbe_base_object_mgmt_drv_power_action_t;
typedef struct
{
    fbe_u8_t                                driveCount;
    fbe_base_object_mgmt_drv_power_action_t drivePowerAction[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
    // drivePowerCycleDuration is only valid for drive power cycle command. 
    // Reserved for other power control commands. 
    fbe_u8_t                                drivePowerCycleDuration[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
} fbe_base_object_mgmt_drv_power_ctrl_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_CONTROL
typedef enum
{
    FBE_EXPANDER_ACTION_NONE = 0,
    FBE_EXPANDER_ACTION_COLD_RESET,
    FBE_EXPANDER_ACTION_SILENT_MODE
} fbe_base_object_mgmt_exp_ctrl_action_t;
typedef struct
{
    fbe_base_object_mgmt_exp_ctrl_action_t  expanderAction;
    fbe_u32_t                               powerCycleDuration;     // in seconds
    fbe_u32_t                               powerCycleDelay;        // in seconds
} fbe_base_object_mgmt_exp_ctrl_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_TEST_MODE_CONTROL
typedef enum
{
    FBE_EXP_TEST_MODE_NO_ACTION = 0,
    FBE_EXP_TEST_MODE_STATUS,
    FBE_EXP_TEST_MODE_DISABLE,
    FBE_EXP_TEST_MODE_ENABLE
} fbe_base_object_mgmt_exp_test_mode_action_t;
typedef struct
{
    fbe_base_object_mgmt_exp_test_mode_action_t     testModeAction;     // input
    fbe_base_object_mgmt_exp_test_mode_action_t     testModeStatus;     // output
} fbe_base_object_mgmt_exp_test_mode_ctrl_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_STRING_OUT_CONTROL
#define FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH      100
typedef struct
{
    fbe_u8_t        stringOutCmd[FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH];
} fbe_base_object_mgmt_exp_string_out_ctrl_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_PS_MARGIN_TEST_CONTROL
typedef enum
{
    FBE_PS_MARGIN_TEST_NO_ACTION = 0,
    FBE_PS_MARGIN_TEST_STATUS,
    FBE_PS_MARGIN_TEST_DISABLE,
    FBE_PS_MARGIN_TEST_ENABLE
} fbe_base_object_mgmt_ps_margin_test_action_t;



typedef struct
{
    fbe_u8_t                                        psCount;
    fbe_base_object_mgmt_ps_margin_test_action_t    psMarginTestStatus[FBE_ENCLOSURE_MAX_NUMBER_OF_PS];
    fbe_base_object_mgmt_ps_margin_test_action_t    psMarginTestAction;  //[FBE_ENCLOSURE_MAX_NUMBER_OF_PS];
} fbe_base_enclosure_ps_margin_test_ctrl_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_LED_STATUS_CONTROL
typedef enum
{
    FBE_ENCLOSURE_LED_NO_ACTION = 0,
    FBE_ENCLOSURE_LED_STATUS,
    FBE_ENCLOSURE_LED_CONTROL
} fbe_base_enclosure_led_cmd_action_t;

typedef enum
{
    FBE_ENCLOSURE_LED_BEHAVIOR_NONE = 0,
    FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON,
    FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF,
    FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON,
    FBE_ENCLOSURE_LED_BEHAVIOR_MARK_OFF,
    FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY
} fbe_base_enclosure_led_behavior_t;

typedef enum
{
    FBE_LED_STATUS_OFF     = 0,
    FBE_LED_STATUS_ON      = 1,
    FBE_LED_STATUS_MARKED  = 2,
    FBE_LED_STATUS_INVALID = 0xFF
} fbe_led_status_t;


/* We met compiling issue in windows build when trying to set fbe_encl_fault_led_reason_t as a 64 bit enum
 * So we have to define macros for LED faults
 */
typedef fbe_u64_t fbe_encl_fault_led_reason_t;

#define    FBE_ENCL_FAULT_LED_NO_FLT                           0x0000ULL
#define    FBE_ENCL_FAULT_LED_PS_FLT                           0x0001ULL
#define    FBE_ENCL_FAULT_LED_FAN_FLT                          0x0002ULL 
#define    FBE_ENCL_FAULT_LED_DRIVE_FLT                        0x0004ULL
#define    FBE_ENCL_FAULT_LED_SPS_FLT                          0x0008ULL
#define    FBE_ENCL_FAULT_LED_OVERTEMP_FLT                     0x0010ULL
#define    FBE_ENCL_FAULT_LED_LCC_FLT                          0x0020ULL
#define    FBE_ENCL_FAULT_LED_CONNECTOR_FLT                    0x0040ULL
#define    FBE_ENCL_FAULT_LED_LCC_RESUME_PROM_FLT              0x0080ULL
#define    FBE_ENCL_FAULT_LED_PS_RESUME_PROM_FLT               0x0100ULL
#define    FBE_ENCL_FAULT_LED_FAN_RESUME_PROM_FLT              0x0200ULL
#define    FBE_ENCL_FAULT_LED_ENCL_LIFECYCLE_FAIL              0x0400ULL
#define    FBE_ENCL_FAULT_LED_SUBENCL_LIFECYCLE_FAIL           0x0800ULL
#define    FBE_ENCL_FAULT_LED_BATTERY_FLT                      0x1000ULL
#define    FBE_ENCL_FAULT_LED_BATTERY_RESUME_PROM_FLT          0x2000ULL
#define    FBE_ENCL_FAULT_LED_LCC_CABLING_FLT                  0x4000ULL
#define    FBE_ENCL_FAULT_LED_EXCEEDED_MAX                     0x8000ULL
#define    FBE_ENCL_FAULT_LED_SP_FLT                           0x10000ULL
#define    FBE_ENCL_FAULT_LED_SP_FAULT_REG_FLT                 0x20000ULL
#define    FBE_ENCL_FAULT_LED_SP_RESUME_PROM_FLT               0x40000ULL
#define    FBE_ENCL_FAULT_LED_SYSTEM_SERIAL_NUMBER_FLT         0x80000ULL
#define    FBE_ENCL_FAULT_LED_PEER_SP_FLT                     0x100000ULL
#define    FBE_ENCL_FAULT_LED_MGMT_MODULE_FLT                 0x200000ULL
#define    FBE_ENCL_FAULT_LED_MGMT_MODULE_RESUME_PROM_FLT     0x400000ULL
#define    FBE_ENCL_FAULT_LED_IO_MODULE_RESUME_PROM_FLT       0x800000ULL
#define    FBE_ENCL_FAULT_LED_BEM_RESUME_PROM_FLT             0x1000000ULL
#define    FBE_ENCL_FAULT_LED_MEZZANINE_RESUME_PROM_FLT       0x2000000ULL
#define    FBE_ENCL_FAULT_LED_MIDPLANE_RESUME_PROM_FLT        0x4000000ULL
#define    FBE_ENCL_FAULT_LED_DRIVE_MIDPLANE_RESUME_PROM_FLT  0x8000000ULL
#define    FBE_ENCL_FAULT_LED_IO_PORT_FLT                    0x10000000ULL
#define    FBE_ENCL_FAULT_LED_IO_MODULE_FLT                  0x20000000ULL
#define    FBE_ENCL_FAULT_LED_CACHE_CARD_FLT                0x040000000ULL
#define    FBE_ENCL_FAULT_LED_DIMM_FLT                      0x080000000ULL
#define    FBE_ENCL_FAULT_LED_NOT_SUPPORTED_FLT             0x100000000ULL
#define    FBE_ENCL_FAULT_LED_SSC_FLT                       0x200000000ULL
#define    FBE_ENCL_FAULT_LED_SSC_RESUME_PROM_FLT           0x400000000ULL
#define    FBE_ENCL_FAULT_LED_INTERNAL_CABLE_FLT            0x800000000ULL
#define    FBE_ENCL_FAULT_LED_BITMASK_MAX                  0x1000000000ULL


typedef struct
{
    fbe_base_enclosure_led_behavior_t       enclFaultLed;               // write\control
    fbe_bool_t                              enclFaultLedStatus;         // read/status
    fbe_encl_fault_led_reason_t             enclFaultReason;            // reason the Encl Fault LED is on
    fbe_bool_t                              enclFaultLedMarked;         // For Read.
    fbe_base_enclosure_led_behavior_t       busDisplay;                 // Display for Bus #
    fbe_u8_t                                busNumber[2];
    fbe_base_enclosure_led_behavior_t       enclDisplay;                // Display for Encl #
    fbe_u8_t                                enclNumber;
    fbe_base_enclosure_led_behavior_t       enclDriveFaultLeds[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
    fbe_led_status_t                        enclDriveFaultLedStatus[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
    fbe_base_enclosure_led_behavior_t       mez_connectorLed;           // 366046 mezzanine expansion connector led
} fbe_base_enclosure_leds_info_t;

typedef struct
{
    fbe_base_enclosure_led_cmd_action_t     ledAction;
    fbe_base_enclosure_leds_info_t          ledInfo;
    fbe_u32_t                               driveCount;
    fbe_u32_t                               driveSlot;
} fbe_base_enclosure_led_status_control_t;

typedef struct
{
    fbe_encl_fault_led_reason_t led_reason;
    fbe_enclosure_fault_symptom_t flt_symptom;

} fbe_base_enclosure_fail_encl_cmd_t;

/* FBE_BASE_ENCLOSURE_CONTROL_CODE_UNBYPASS_PARENT */
typedef struct fbe_base_enclosure_unbypass_parent_command_s {
    fbe_object_id_t parent_id;
} fbe_base_enclosure_unbypass_parent_command_t;

/* FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_BYPASS */
typedef struct fbe_base_enclosure_mgmt_drive_bypass_s {
    fbe_u32_t drive_bypass_control;
} fbe_base_enclosure_drive_bypass_command_t;

/* FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_UNBYPASS */
typedef struct fbe_base_enclosure_mgmt_drive_unbypass_s {
    fbe_u32_t drive_unbypass_control;
} fbe_base_enclosure_drive_unbypass_command_t;

typedef enum fbe_enclosure_state_type_e {
    FBE_ENCLOSURE_STATE_TYPE_INVALID       = 0x00,
    FBE_ENCLOSURE_STATE_TYPE_FIBRE         = 0x01,
    FBE_ENCLOSURE_STATE_TYPE_LAST
} fbe_enclosure_state_type_t;

typedef enum fbe_enclosure_drive_fault_led_e {
    FBE_ENCLOSURE_DRIVE_FAULT_LED_TURN_OFF = 0x00,
    FBE_ENCLOSURE_DRIVE_FAULT_LED_TURN_ON  = 0x01,
    FBE_ENCLOSURE_DRIVE_FAULT_LED_FLASH = 0x02,
    FBE_ENCLOSURE_DRIVE_FAULT_LED_LAST
} fbe_enclosure_drive_fault_led_t;

/* FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_DRIVE_FAULT_LED */
typedef struct fbe_base_enclosure_mgmt_set_drive_fault_led_s {
    fbe_object_id_t parent_id;
    fbe_enclosure_drive_fault_led_t fault_led_control;
} fbe_base_enclosure_mgmt_set_drive_fault_led_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_firmware_opcode_t
 *  @brief 
 *    This defines all the valid op code for firmware download process.
 *  FBE_ENCLOSURE_FIRMWARE_OP_LAST is always the last in the list.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCLOSURE_FIRMWARE_OP_NONE       = 0x00,
    FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD   = 0x01,   //!< download
    FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE   = 0x02,   //!< activate
    FBE_ENCLOSURE_FIRMWARE_OP_ABORT      = 0x03,   //!< abort
    FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS = 0x04,   //!< get status
    FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION = 0x05,   //!< notify completion
    FBE_ENCLOSURE_FIRMWARE_OP_SET_PRIORITY = 0x06,   //!< notify completion
    FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION    = 0x07,   //!< get permission from board object to upgrade
    FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION = 0x08,   //!< after fup, return the permission 
    FBE_ENCLOSURE_FIRMWARE_OP_LAST
} fbe_enclosure_firmware_opcode_t;


/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_download_op_t
 *  
 * @brief 
 *   defines firmware download related operations 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_FIRMWARE_OP. 
 **********************************************************************/
typedef struct 
{
    fbe_enclosure_firmware_opcode_t     op_code; //!< download/activate/abort/get_status
    fbe_enclosure_fw_target_t           target;  //!< device to download
    fbe_u32_t                           size;    //!< total image size
    fbe_u32_t                           header_size;
    CSX_ALIGN_N(8) fbe_u8_t          *image_p;   //!< pointer to the image(we allign to 64 bits to make sure user/krenl passing works)
    fbe_u8_t                            side_id;    //!< device side id to download
    fbe_enclosure_firmware_status_t     status;     //!< operation status (out)
    fbe_enclosure_firmware_ext_status_t extended_status; //!< extended operation status (out)
    fbe_u32_t                           bytes_downloaded;//!<number of bytes downloaded to expander (out)
    fbe_bool_t                          checkPriority;
    fbe_device_physical_location_t      location; //!device location that request permission(in) device location that holds permission (out)
    fbe_bool_t                          permissionGranted; //!<indicate if permission is granted (out)
} fbe_enclosure_mgmt_download_op_t;

// fbe_eses_encl_fup_info_t contains information related to 
// a current download/activate operation. The status
// information can be queried even when there is no
// operation in progress.
/*!********************************************************************* 
 * @struct fbe_eses_encl_fup_info_t
 *  
 * @brief 
 *   Enclosure firmware download operation context
 **********************************************************************/
typedef struct 
{
    /* firmware related info */
    fbe_u8_t               *enclFupImagePointer;
    fbe_u32_t               enclFupImageSize;
    fbe_u8_t                enclFupComponent;
    fbe_u8_t                enclFupComponentSide;
    fbe_u8_t                enclFupOperation;
    fbe_bool_t              enclFupUseTunnelling;
    /* status related info */
    fbe_enclosure_firmware_status_t      enclFupOperationStatus;
    fbe_enclosure_firmware_ext_status_t  enclFupOperationAdditionalStatus;
    fbe_u8_t                enclFupOperationRetryCnt;
    fbe_u32_t               enclFupNoBytesSent;
    fbe_u32_t               enclFupBytesTransferred;
    fbe_u32_t               enclFupCurrTransferSize;
    fbe_time_t              enclFupStartTime;
} fbe_eses_encl_fup_info_t;

/*!********************************************************************* 
 * @struct fbe_eses_encl_eir_info_t
 *  
 * @brief 
 *   Enclosure firmware download operation context
 **********************************************************************/
typedef struct 
{
    fbe_device_physical_location_t  location;
    fbe_esp_current_encl_eir_data_t enclEirData;
} fbe_eses_encl_eir_info_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_mgmt_trace_buf_op_t
 *  @brief 
 *    Defines the trace buffer operations.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCLOSURE_TRACE_BUF_OP_INVALID        = 0x00,   // Invalid.
    FBE_ENCLOSURE_TRACE_BUF_OP_GET_NUM        = 0x01,   // Get the number of trace buffers.
    FBE_ENCLOSURE_TRACE_BUF_OP_GET_STATUS     = 0x02,   // Get the status of all the trace buffers.
    FBE_ENCLOSURE_TRACE_BUF_OP_SAVE           = 0x03,   // Save the currently active trace buffer.
    FBE_ENCLOSURE_TRACE_BUF_OP_CLEAR          = 0x04,   // Clear the trace buffer.
} fbe_enclosure_mgmt_trace_buf_op_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_trace_buf_cmd_t
 *  
 * @brief 
 *   defines the content of trace buffer command. It is used for the following both control codes.
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS. 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_TRACE_BUF_INFO_CTRL. 
 ***********************************************************************/
typedef struct 
{
    /*
     * (1) If buf_op is save buffer,
     * EMA should choose an empty buffer or overwrite the oldest buffer
     * (this is the same behavior it takes when it auto-saves a trace.) 
     * (2) All platforms support the ability to save the active trace of the ESC electronics 
     * element in the primary LCC. Magnum, Viper and Derringer do not support the ability to 
     * save any other traces.
     */
    fbe_u8_t                            buf_id;
    fbe_enclosure_mgmt_trace_buf_op_t   buf_op;
}fbe_enclosure_mgmt_trace_buf_cmd_t;
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_max_elem_group
 *  
 * @brief 
 *   defines the content of that fbe_enclosure_mgmt_max_elem_group points to 
 *   @ref fbe_enclosure_mgmt_ctrl_op_t for the control code 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MAX_ELEM_GROUPS. 
 ***********************************************************************/
typedef struct
{
    fbe_u8_t max_elem_group;
}fbe_enclosure_mgmt_max_elem_group;


/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_read_buf_cmd_t
 *  
 * @brief 
 *   defines the content of that fbe_enclosure_mgmt_ctrl_cmd_t points to 
 *   @ref fbe_enclosure_mgmt_ctrl_op_t for the control code 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_READ_BUF. 
 ***********************************************************************/
typedef struct 
{
    fbe_u8_t mode;   //The mode of the read buffer command.
    fbe_u8_t buf_id; // The identity of the buffer to be read. 
    fbe_u32_t buf_offset; // The byte offset inot a buffer described by buffer id. not used in all modes. 
}fbe_enclosure_mgmt_read_buf_cmd_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_resume_prom_ids_t
 *  @brief 
 *    Defines the resume proms that Flare can access.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCLOSURE_LCC_A_RESUME_PROM,
    FBE_ENCLOSURE_LCC_B_RESUME_PROM,
    FBE_ENCLOSURE_PS_A_RESUME_PROM,
    FBE_ENCLOSURE_PS_B_RESUME_PROM,
    FBE_ENCLOSURE_MIDPLANE_RESUME_PROM,
    FBE_ENCLOSURE_EE_LCC_A_RESUME_PROM,
    FBE_ENCLOSURE_EE_LCC_B_RESUME_PROM,
    FBE_ENCLOSURE_FAN_1_RESUME_PROM,
    FBE_ENCLOSURE_FAN_2_RESUME_PROM,
    FBE_ENCLOSURE_FAN_3_RESUME_PROM,
    FBE_ENCLOSURE_INVALID_RESUME_PROM
} fbe_enclosure_resume_prom_ids_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_resume_prom_req_type_t
 *  @brief 
 *    Defines the resume proms request type.
 ***************************************************************************/ 
typedef enum 
{
    FBE_ENCLOSURE_READ_RESUME_PROM,
    FBE_ENCLOSURE_WRITE_RESUME_PROM,
    FBE_ENCLOSURE_READ_ACTIVE_RAM,
    FBE_ENCLOSURE_WRITE_ACTIVE_RAM,
} fbe_enclosure_resume_prom_req_type_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_resume_read_op_t
 *  
 * @brief 
 *  Defines the struct for resume prom read operation through read
 *  buffer operation. Client enters prom_id and read_buf.buf_offset. 
 *  Resume_type is translated into buffer id by the fbe_eses_enclosure. 
 *  Buf_offset is the read starting point. Size Response buffer
 *  are part of fbe_enclosure_mgmt_ctrl_op_t.
 ***********************************************************************/
typedef struct 
{
    fbe_u64_t  deviceType;
    fbe_device_physical_location_t deviceLocation;
    fbe_enclosure_mgmt_read_buf_cmd_t   read_buf;   //!< client fills in read_buf.buf_offset
}fbe_enclosure_mgmt_resume_read_cmd_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_resume_write_cmd_t
 *  
 * @brief 
 *  Defines the struct for resume prom write operation.
 ***********************************************************************/

typedef struct 
{
    fbe_u64_t  deviceType;
    fbe_device_physical_location_t deviceLocation;
    CSX_ALIGN_N(8) fbe_u8_t *data_buf_p; /*allign to 8 bytes to make sure it work if ever passed down to kernel via FBE API*/
    fbe_u32_t data_buf_size; 
    fbe_u32_t data_offset;
}fbe_enclosure_mgmt_resume_write_cmd_t;

/*!********************************************************************* 
 * @enum fbe_enclosure_drive_debug_op_t
 *  
 * @brief 
 *   defines the drive debug operations. 
 ***********************************************************************/
typedef enum
{
    FBE_ENCLOSURE_DEBUG_CMD_OP_INVALID = 0,
    FBE_ENCLOSURE_DEBUG_CMD_OP_DRIVE_REMOVE = 1,
    FBE_ENCLOSURE_DEBUG_CMD_OP_DRIVE_INSERT = 2,
    FBE_ENCLOSURE_DEBUG_CMD_OP_DRIVE_BYPASS = 3,
    FBE_ENCLOSURE_DEBUG_CMD_OP_DRIVE_UNBYPASS = 4
}fbe_enclosure_debug_cmd_op_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_debug_cmd_t
 *  
 * @brief 
 *   defines the content of that fbe_enclosure_mgmt_ctrl_cmd_t points to 
 *   @ref fbe_enclosure_mgmt_ctrl_op_t for the control code 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_DEBUG. 
 ***********************************************************************/
typedef struct 
{
    fbe_u8_t target_id; // The target. 0xFF for all the targets in the group. For example, all the drives or all the phys.
    fbe_enclosure_debug_cmd_op_t debug_cmd_op; // The debug command opcode.  
}fbe_enclosure_mgmt_debug_cmd_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_is_slot_present_cmd_t
 *  
 * @brief  
 *   Caller enters parent obj id and slot number. Boolean is returned
 *   to indicate if the disk slot was found. If the slot was not found
 *   it must have a different parent object. This is useful for voyager
 *   enclosure where slots are split between multiple expanders.
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_PRESENT
 ***********************************************************************/
typedef struct 
{
    fbe_u8_t        targetSlot; // (in) the target slot number
    fbe_bool_t      slotFound;  // (out) TRUE if the slot was found
}fbe_enclosure_mgmt_disk_slot_present_cmd_t;

/*!********************************************************************* 
 * @enum fbe_enclosure_raw_rcv_diag_pg_t
 *  
 * @brief 
 *   defines the different receive diagnostic pages. 
 ***********************************************************************/
typedef enum
{
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_INVALID = 0,
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_CFG = 1,
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STATUS = 2,
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_ADDL_STATUS = 3,
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_EMC_ENCL_STATUS = 4,
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STRING_IN = 5,
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_STATISTICS = 6,
    FBE_ENCLOSURE_RAW_RCV_DIAG_PG_THRESHOLD_IN = 7,
}fbe_enclosure_raw_rcv_diag_pg_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_raw_rcv_diag_cmd_t
 *  
 * @brief 
 *   defines the content of that fbe_enclosure_mgmt_ctrl_cmd_t points to 
 *   This command expects response_buf_p be supplied as part of the 
 *   fbe_enclosure_mgmt_ctrl_op_t.
 *   @ref fbe_enclosure_raw_rcv_diag_pg_t for the page code 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_RCV_DIAG. 
 ***********************************************************************/
typedef struct 
{
    fbe_enclosure_raw_rcv_diag_pg_t rcv_diag_page; // The page opcode.  
}fbe_enclosure_mgmt_raw_rcv_diag_cmd_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_mgmt_slot_to_phy_mapping_cmd_t
 *  @brief 
 *    Defines the drive slot to phy mapping command.
 ***************************************************************************/ 
typedef struct 
{
    fbe_enclosure_slot_number_t slot_num_start;
    fbe_enclosure_slot_number_t slot_num_end;
} fbe_enclosure_mgmt_slot_to_phy_mapping_cmd_t;

/*!*************************************************************************
 *  @enum fbe_enclosure_mgmt_slot_to_phy_mapping_info_t
 *  @brief 
 *    Defines the drive slot to phy mapping response.
 ***************************************************************************/
typedef struct 
{
    fbe_sas_address_t expander_sas_address;
    fbe_u8_t          phy_id;
}fbe_enclosure_mgmt_slot_to_phy_mapping_info_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_raw_mode_sense_pg_t
 *  
 * @brief 
 *   defines the different Mode Sense pages. 
 ***********************************************************************/
typedef enum
{
    FBE_ENCLOSURE_RAW_MODE_SENSE_PG_ESM         = 0x00,    /* Enclosure Services Management Mode */            
    FBE_ENCLOSURE_RAW_MODE_SENSE_PG_PSP_SAS_SSP = 0x01,    /* Protocol Specific Port Mode for SAS SSP */
    FBE_ENCLOSURE_RAW_MODE_SENSE_PG_EEP         = 0x20,    /* 20h - EMC ESES Persistent Mode */
    FBE_ENCLOSURE_RAW_MODE_SENSE_PG_EENP        = 0x21,    /* 21h - EMC ESES Non-Persistent Mode */
    FBE_ENCLOSURE_RAW_MODE_SENSE_PG_INVALID     = 0xFF,
}fbe_enclosure_raw_mode_sense_pg_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_raw_mode_sense_cmd_t
 *  
 * @brief 
 *   defines the content of that fbe_eses_cmd_info_t points to 
 *   This command expects response_buf_p be supplied as part of the 
 *   fbe_enclosure_mgmt_eses_page_op_t.
 *   @ref fbe_enclosure_raw_mode_sense_pg_t for the sense page code 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_MODE_SENSE. 
 ***********************************************************************/
typedef struct 
{
    fbe_enclosure_raw_mode_sense_pg_t sense_page; // The page opcode.  
}fbe_enclosure_mgmt_raw_mode_sense_cmd_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_raw_inquiry_cmd_t
 *  
 * @brief 
 *   defines the different VPD Inquiry pages. 
 ***********************************************************************/
typedef enum
{
    FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_SUPPORTED      = 0x00, /* 00h */
    FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_UNIT_SERIALNO  = 0x80, /* 80h */
    FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_DEVICE_ID      = 0x83, /* 83h */
    FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_EXTENDED       = 0x86, /* 86h */
    FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_SUBENCL        = 0xC0, /* C0h - DFh */
    FBE_ENCLOSURE_RAW_INQUIRY_VPD_PG_INVALID        = 0xFF,
}fbe_enclosure_raw_inquiry_vpd_pg_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_raw_inquiry_cmd_t
 *  
 * @brief 
 *   defines the content of that fbe_eses_cmd_info_t points to 
 *   This command expects response_buf_p be supplied as part of the 
 *   fbe_enclosure_mgmt_eses_page_op_t.
 *   @ref fbe_enclosure_raw_inquiry_vpd_pg_t for the page code 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_INQUIRY. 
 ***********************************************************************/
typedef struct 
{
    fbe_u8_t    evpd;   // EVPD
    fbe_enclosure_raw_inquiry_vpd_pg_t page_code; // The page opcode.  
}fbe_enclosure_mgmt_raw_inquiry_cmd_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_raw_scsi_cmd_t
 *  
 * @brief 
 *   defines the scsi commands Inquiry and Mode sense. 
 *   @ref fbe_enclosure_mgmt_raw_inquiry_cmd_t for the Inquiry command 
 *   @ref fbe_enclosure_mgmt_raw_mode_sense_cmd_t for the Mode sense command 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_SCSI. 
 ***********************************************************************/
typedef struct 
{
    fbe_enclosure_mgmt_raw_inquiry_cmd_t    raw_inquiry; // Raw Inquiry command.  
    fbe_enclosure_mgmt_raw_mode_sense_cmd_t raw_mode_sense; // Raw Mode Sense command.
}fbe_enclosure_mgmt_raw_scsi_cmd_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_element_type_t
 *  
 * @brief 
 *   defines the element types
 ***********************************************************************/
typedef enum
{
    FBE_ELEMENT_TYPE_PS         = 0x02,
    FBE_ELEMENT_TYPE_COOLING    = 0x03,
    FBE_ELEMENT_TYPE_TEMP_SENSOR = 0x04,
    FBE_ELEMENT_TYPE_DEV_SLOT   = 0x17,
    FBE_ELEMENT_TYPE_SAS_EXP    = 0x18,
    FBE_ELEMENT_TYPE_EXP_PHY    = 0x81,
    FBE_ELEMENT_TYPE_ALL        = 0xFE, // no ses_element_type equivalent
    FBE_ELEMENT_TYPE_INVALID    = 0xFF 
}fbe_enclosure_element_type_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_statistics_data_t
 *  
 * @brief 
 *  This struct is used to format the data returned from the control op.
 *  The structs are used for returning data that has been parsed from
 *  the eses statistics page.
 ***********************************************************************/
typedef struct
{
    fbe_u8_t        element_type;
    fbe_u8_t        slot_or_id;
    //fbe_u8_t        side;
        fbe_u8_t        drv_or_conn;
        fbe_u8_t                drv_or_conn_num;
} fbe_enclosure_mgmt_stats_resp_id_info_t;

typedef struct
{
    fbe_enclosure_mgmt_stats_resp_id_info_t stats_id_info;
    fbe_u8_t                       stats_data;
} fbe_enclosure_mgmt_statistics_data_t;

typedef struct
{
    fbe_u32_t                               data_length;
    fbe_enclosure_mgmt_statistics_data_t    data;
} fbe_enclosure_mgmt_statistscs_response_page_t;


/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_statistics_cmd_t
 *  
 * @brief 
 ***********************************************************************/
typedef struct
{
    fbe_enclosure_element_type_t    type;
    fbe_u8_t                        first;
    fbe_u8_t                        last;
} fbe_enclosure_mgmt_statistics_cmd_t;


/*!********************************************************************* 
 * @struct fbe_expander_threshold_info_t
 *  
 * @brief 
 *   describes the threshold information associated with an enclosure. 
 **********************************************************************/
typedef struct fbe_expander_threshold_individual_info_s
{
    fbe_u8_t                   high_critical_threshold;
    fbe_u8_t                   high_warning_threshold;
    fbe_u8_t                   low_warning_threshold;
    fbe_u8_t                   low_critical_threshold;
} fbe_expander_threshold_overall_info_t;

typedef struct fbe_enclosure_mgmt_exp_threshold_info_s 
{
    fbe_enclosure_element_type_t   elem_type;
    fbe_expander_threshold_overall_info_t  exp_overall_info;
}fbe_enclosure_mgmt_exp_threshold_info_t;


typedef struct fbe_enclosure_mgmt_threshold_in_cmd_s 
{
    fbe_enclosure_component_types_t componentType;
}fbe_enclosure_mgmt_threshold_in_cmd_t;

/*!********************************************************************* 
 * @struct fbe_sps_in_buffer_cmd_t
 *  
 * @brief 
 ***********************************************************************/
typedef struct
{
    fbe_sps_action_type_t           spsAction;
} fbe_sps_in_buffer_cmd_t;

/*!********************************************************************* 
 * @struct fbe_sps_eeprom_cmd_t
 *  
 * @brief 
 ***********************************************************************/
typedef struct
{
    fbe_sps_manuf_info_t           *spsManufInfo;
} fbe_sps_eeprom_cmd_t;

/*!********************************************************************* 
 * @union fbe_enclosure_mgmt_ctrl_cmd_t
 *  
 * @brief 
 *   defines the content of different control commands.
 ***********************************************************************/
typedef union 
{
    fbe_enclosure_mgmt_read_buf_cmd_t       read_buf;// Read Buffer 
    fbe_enclosure_mgmt_trace_buf_cmd_t      trace_buf_cmd; // trace buffer command
    fbe_enclosure_mgmt_debug_cmd_t          drive_dbg_op;// Drive debug operations
    fbe_enclosure_mgmt_resume_read_cmd_t    resume_read; // Resume prom read
    fbe_enclosure_mgmt_resume_write_cmd_t   resume_write; //Resume prom write
    fbe_base_object_mgmt_exp_string_out_ctrl_t  stringOutInfo;
    fbe_base_object_mgmt_exp_test_mode_ctrl_t   testModeInfo;
    fbe_base_enclosure_ps_margin_test_ctrl_t    ps_Margin_Ctl;
    fbe_enclosure_mgmt_raw_rcv_diag_cmd_t       raw_rcv_diag_page;
    fbe_enclosure_mgmt_slot_to_phy_mapping_cmd_t slot_to_phy_mapping_cmd;
    fbe_enclosure_mgmt_raw_scsi_cmd_t     raw_scsi; // Raw SCSI commands
    fbe_enclosure_mgmt_statistics_cmd_t     element_statistics; // statistics
    fbe_enclosure_mgmt_threshold_in_cmd_t threshold_in; //Threshold In
    fbe_enclosure_mgmt_exp_threshold_info_t     thresholdOutInfo;    //Threshold out
    fbe_sps_in_buffer_cmd_t                 spsInBufferCmd;
    fbe_sps_eeprom_cmd_t                    spsEepromCmd;
}fbe_enclosure_mgmt_ctrl_cmd_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_ctrl_op_t
 *  
 * @brief 
 *   defines eses page related control operation. 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS.
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_TRACE_BUF_INFO_CTRL.
 *   The user request needs to fill in all the inputs and the output 
 *   will be returned by the physical package.
 **********************************************************************/
typedef struct 
{
    fbe_enclosure_mgmt_ctrl_cmd_t    cmd_buf;         // The command buffer (input) 
    CSX_ALIGN_N(8) fbe_u8_t    *response_buf_p;            // The pointer to the response buffer (input) 
    fbe_u32_t   response_buf_size;          // The size of the command/response buffer allocated(input).
    fbe_u32_t   required_response_buf_size; // The required response buffer size (output).  
} fbe_enclosure_mgmt_ctrl_op_t;

typedef struct fbe_enclosure_fibre_state_s {
    fbe_u32_t diplex_address;
    fbe_u32_t fibre_enclosure_state;
    fbe_u32_t enclosure_info;                /* shadows the diplex poll byte for this enclosure */
    fbe_u32_t expansion_port_state;    /* shadows the expansion port state from the hardware */
    fbe_u32_t hardware_type_code;
    fbe_u32_t diplex_read_count;
    fbe_u32_t diplex_write_count;
    fbe_u32_t power_status;            /* shadows power regulator status from the hardware (READ) */
    fbe_u32_t drive_presence;          /* shadows drive presence from the hardware (READ) */
    fbe_u32_t drive_fault_sensor;      /* shadows drive fault indications from the hardware (READ) */
    fbe_u32_t drive_bypass_request;    /* shadows drive bypass requests from the hardware (READ) */
    fbe_u32_t drive_bypass_status;     /* shadows drive bypass status from the hardware (READ) */
    fbe_u32_t cable_port_status;       /* shadows cable port status (READ). */
    fbe_u32_t misc_status;             /* shadows misc status registers (READ). */
    fbe_u32_t additional_status;       /* shadows additional status (READ). */
    fbe_u32_t drive_fault_led;         /* shadows drive fault led commands to the hardware (WRITE) */
    fbe_u32_t drive_bypass_control;    /* shadows drive bypass commands to the hardware (WRITE) */
    fbe_u32_t drive_ODIS_enable;       /* shadows drive ODIS enable commands to the hardware (WRITE) */
    fbe_u32_t misc_control;            /* shadows misc control registers (WRITE). */
} fbe_enclosure_fibre_state_t;

typedef struct fbe_enclosure_type_state_u {
    fbe_enclosure_fibre_state_t fibre;
} fbe_enclosure_type_state_t;

typedef struct fbe_enclosure_state_s {
    fbe_enclosure_state_type_t type;
    fbe_enclosure_type_state_t state;
} fbe_enclosure_state_t;

typedef enum fbe_sas_enclosure_type_e {
    FBE_SAS_ENCLOSURE_TYPE_INVALID,
    FBE_SAS_ENCLOSURE_TYPE_BULLET,
    FBE_SAS_ENCLOSURE_TYPE_VIPER,
    FBE_SAS_ENCLOSURE_TYPE_MAGNUM,
    FBE_SAS_ENCLOSURE_TYPE_CITADEL,
    FBE_SAS_ENCLOSURE_TYPE_BUNKER,
    FBE_SAS_ENCLOSURE_TYPE_DERRINGER,
    FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM,
    FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE,  
    FBE_SAS_ENCLOSURE_TYPE_CAYENNE,
    FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP,
    FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP,  
    FBE_SAS_ENCLOSURE_TYPE_FALLBACK,  
    FBE_SAS_ENCLOSURE_TYPE_BOXWOOD,  
    FBE_SAS_ENCLOSURE_TYPE_KNOT,  
    FBE_SAS_ENCLOSURE_TYPE_PINECONE,  
    FBE_SAS_ENCLOSURE_TYPE_STEELJAW,  
    FBE_SAS_ENCLOSURE_TYPE_RAMHORN,
    FBE_SAS_ENCLOSURE_TYPE_ANCHO,
    FBE_SAS_ENCLOSURE_TYPE_VIKING,
    FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP,
    FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP,
    FBE_SAS_ENCLOSURE_TYPE_NAGA,
    FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP,
    FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP,
    FBE_SAS_ENCLOSURE_TYPE_RHEA,  
    FBE_SAS_ENCLOSURE_TYPE_MIRANDA,  
    FBE_SAS_ENCLOSURE_TYPE_CALYPSO,  
    FBE_SAS_ENCLOSURE_TYPE_TABASCO,
    FBE_SAS_ENCLOSURE_TYPE_UNKNOWN,
    FBE_SAS_ENCLOSURE_TYPE_LAST
} fbe_sas_enclosure_type_t;

typedef enum fbe_fc_enclosure_type_e {
    FBE_FC_ENCLOSURE_TYPE_INVALID,
    FBE_FC_ENCLOSURE_TYPE_STILLETO,
    FBE_FC_ENCLOSURE_TYPE_KLONDIKE,
    FBE_FC_ENCLOSURE_TYPE_KATANA,
    FBE_FC_ENCLOSURE_TYPE_UNKNOWN,

    FBE_FC_ENCLOSURE_TYPE_LAST
} fbe_fc_enclosure_type_t;

typedef enum fbe_sas_enclosure_protuct_type_e {
    FBE_SAS_ENCLOSURE_PRODUCT_TYPE_INVALID,
    FBE_SAS_ENCLOSURE_PRODUCT_TYPE_ESES,

    FBE_SAS_ENCLOSURE_PRODUCT_TYPE_LAST
}
fbe_sas_enclosure_product_type_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_fault_t
 *  
 * @brief 
 *   describes the fault information associated with an enclosure. 
 *   More elements may get added to this structure so that other
 *   modules could understand why we have failed this enclosure.
 **********************************************************************/
typedef struct fbe_enclosure_fault_s {
    fbe_u32_t                               faultSymptom;
    fbe_u32_t                               faultyComponentIndex;
    fbe_u32_t                               additional_status;
    fbe_u32_t                               faultyComponentType;
} fbe_enclosure_fault_t;

/*!********************************************************************* 
 * @struct fbe_enclosure_scsi_error_info_t
 *  
 * @brief 
 *   describes the scsi error information associated with an enclosure. 
 *   More elements may get added to this structure so that other
 *   modules could understand why we have failed this enclosure.
 **********************************************************************/
typedef struct fbe_enclosure_scsi_error_info_s {
    fbe_u32_t                   scsi_error_code;
    fbe_u32_t                   scsi_status;
    fbe_u32_t                   payload_request_status;
    fbe_u32_t                   sense_key;
    fbe_u32_t                   addl_sense_code;
    fbe_u32_t                   addl_sense_code_qualifier;
} fbe_enclosure_scsi_error_info_t;

// fbe_eses_enclosure_debug.c
char * fbe_eses_enclosure_fw_targ_to_text(fbe_enclosure_fw_target_t fw_target);
char * fbe_eses_enclosure_fw_op_to_text(fbe_enclosure_firmware_opcode_t fw_op);
char * fbe_eses_enclosure_fw_status_to_text(fbe_enclosure_firmware_status_t dl_status);
char * fbe_eses_enclosure_fw_extstatus_to_text(fbe_enclosure_firmware_ext_status_t dl_extstatus);

typedef enum fbe_base_enclosure_death_reason_e{
    FBE_BASE_ENCLOSURE_DEATH_REASON_INVALID = FBE_OBJECT_DEATH_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_ENCLOSURE),
    FBE_BASE_ENCLOSURE_DEATH_REASON_FAULTY_COMPONENT,
    FBE_BASE_ENCLOSURE_DEATH_REASON_LAST
}fbe_base_enclosure_death_reason_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_get_ps_info_t
 *  
 * @brief 
 *   describes the Power Supply information associated with an enclosure. 
 **********************************************************************/
typedef struct fbe_enclosure_get_ps_info_s {
    fbe_device_physical_location_t  location;
    fbe_power_supply_info_t         psInfo;
} fbe_enclosure_get_ps_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_PARSE_IMAGE_HEADER
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_parse_image_header_info_t
 *  
 * @brief 
 *   parse the image header and return the required info. 
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_parse_image_header_s{
    fbe_char_t imageHeader[FBE_ENCLOSURE_MAX_IMAGE_HEADER_SIZE];  //input
    fbe_char_t subenclProductId[FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE][FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1]; //output
    fbe_char_t imageRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; // output
    fbe_u32_t imageSize; // output
    fbe_u32_t headerSize;  // output
    fbe_enclosure_fw_target_t firmwareTarget; // output

} fbe_enclosure_mgmt_parse_image_header_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_get_lcc_info_t
 *  
 * @brief 
 *   get the information of the LCC in the specified location.  
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_get_lcc_info_s{
    fbe_device_physical_location_t location;  // INPUT
    fbe_lcc_info_t                 lccInfo;   // OUTPUT
} fbe_enclosure_mgmt_get_lcc_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_get_connector_info_t
 *  
 * @brief 
 *   get the information of the Connector in the specified location.  
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_get_connector_info_s{
    fbe_device_physical_location_t location;  // INPUT
    fbe_connector_info_t           connectorInfo;   // OUTPUT
} fbe_enclosure_mgmt_get_connector_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_get_ssc_info_t
 *  
 * @brief 
 *   get the SSC information for the location.  
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_get_ssc_info_s{
    fbe_device_physical_location_t  location;  // INPUT
    fbe_ssc_info_t                  sscInfo;   // OUTPUT
} fbe_enclosure_mgmt_get_ssc_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DRIVE_SLOT_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_get_drive_slot_info_t
 *  
 * @brief 
 *   Get the drive slot info. 
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_get_drive_slot_info_s{
    fbe_device_physical_location_t  location; // INPUT
    fbe_u8_t localSideId;       // OUTPUT (A side or B side)
    fbe_bool_t bypassed;        // OUTPUT
    fbe_bool_t selfBypassed;    // OUTPUT
    fbe_bool_t poweredOff;      // OUTPUT
    fbe_bool_t inserted;        // OUTPUT (physical insertedness)
} fbe_enclosure_mgmt_get_drive_slot_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SHUTDOWN_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_get_shutdown_info_t
 *  
 * @brief 
 *   Get the enclosure's Shutdown info. 
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_get_shutdown_info_s{
    fbe_u8_t                        shutdownReason;
} fbe_enclosure_mgmt_get_shutdown_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_OVERTEMP_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_get_overtemp_info_t
 *  
 * @brief 
 *   Get the enclosure's OverTemp info. 
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_get_overtemp_info_s{
    fbe_bool_t                        overTempWarning;
    fbe_bool_t                        overTempFailure;
} fbe_enclosure_mgmt_get_overtemp_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_INFO
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_get_sps_info_t
 *  
 * @brief 
 *   Get the enclosure's SPS info. 
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_get_sps_info_s{
    fbe_device_physical_location_t  location; // INPUT
    fbe_base_sps_info_t             spsInfo;
} fbe_enclosure_mgmt_get_sps_info_t;

// FBE_BASE_ENCLOSURE_CONTROL_CODE_SPS_COMMAND
/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_sps_command_t
 *  
 * @brief 
 *   Send SPS command to enclosure. 
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_sps_command_s{
    fbe_sps_action_type_t    spsAction;
} fbe_enclosure_mgmt_sps_command_t;

/*!********************************************************************* 
 * @struct fbe_encl_fault_led_info_t
 *  
 * @brief 
 *   Get the enclosure's fault led info. 
 **********************************************************************/
typedef struct fbe_encl_fault_led_info_s{
    fbe_led_status_t        enclFaultLedStatus; 
    fbe_encl_fault_led_reason_t enclFaultLedReason; // see fbe_encl_fault_led_reason_t for bit mask.
} fbe_encl_fault_led_info_t;

/*!********************************************************************* 
 * @struct fbe_encl_display_info_t
 *  
 * @brief 
 *   Get the enclosure's display led info. 
 **********************************************************************/
typedef struct fbe_encl_display_info_s{
    fbe_u8_t                                busNumber[2];
    fbe_u8_t                                enclNumber;
}fbe_encl_display_info_t;

/*!*************************************************************************
 *  \} 
 *  end of fbe_enclosure_control_interface group
 ***************************************************************************/
#endif /* FBE_ENCLOSURE_H */
