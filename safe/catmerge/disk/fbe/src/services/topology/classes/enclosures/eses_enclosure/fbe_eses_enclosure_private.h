#ifndef ESES_ENCLOSURE_PRIVATE_H
#define ESES_ENCLOSURE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for eses enclosure class.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   10/28/2008:  Created file. BP
 *
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_eses.h"
#include "fbe_scsi.h"
#include "sas_enclosure_private.h"

#define FBE_ESES_ENCLOSURE_NUMBER_OF_PRE_ALLOC_WIRTE_BUFFER  2
#define FBE_ESES_MEMORY_CHUNK_SIZE   FBE_MEMORY_CHUNK_SIZE
#define FBE_ESES_ENCLOSURE_ESES_PAGE_RESPONSE_BUFFER_SIZE (FBE_MEMORY_CHUNK_SIZE - (2 * sizeof(fbe_sg_element_t)))

// define the power supply minimum firmware revision, no upgrade if below this rev
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_CDES_REV_STR              "  26 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_GEN3_PS_REV_STR           " 509 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_DERRINGER_PS_REV_STR      " 506 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_VOYAGER_PS_REV_STR        " 300 "   // JAP - get real value (402)
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_JUNIPER_PS_REV_STR        " 110 "   
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_TABASCO_PS_REV_STR        " 506 "   // JJB need real values ENCL_CLEANUP
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_AC_1080_PS_REV_STR        " 000 "   //  
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_JUNO2_PS_REV_STR          " 000 "   //  
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_ACBEL_3GVE_PS_REV_STR     " 000 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_EMER_3GVE_PS_REV_STR      " 000 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_OPTIMUS_ACBEL_PS_REV_STR  " 000 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_OPTIMUS_FLEX_PS_REV_STR   " 000 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_TABASCO_DC_PS_REV_STR     " 000 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_OPTIMUS_ACBEL2_PS_REV_STR " 000 "
#define FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_OPTIMUS_FLEX2_PS_REV_STR  " 000 "

#define FBE_ESES_ENCL_VIPER_LCC_PRODUCT_ID_STR              "Viper LCC       "
#define FBE_ESES_ENCL_PINECONE_LCC_PRODUCT_ID_STR           "Pinecone LCC    "
#define FBE_ESES_ENCL_DERRINGER_LCC_PRODUCT_ID_STR          "DERRINGER LCC   "
#define FBE_ESES_ENCL_BUNKER_CITADEL_LCC_PRODUCT_ID_STR     "Sentry LCC      "
#define FBE_ESES_ENCL_VOYAGER_ICM_PRODUCT_ID_STR            "VOYAGER ICM     "
#define FBE_ESES_ENCL_VOYAGER_EE_PRODUCT_ID_STR             "VOYAGER LCC     "
#define FBE_ESES_ENCL_FALLBACK_LCC_PRODUCT_ID_STR           "Jetfire BEM     "
#define FBE_ESES_ENCL_STEELJAW_RAMHORN_LCC_PRODUCT_ID_STR   "Beachcomber LCC "
#define FBE_ESES_ENCL_BOXWOOD_KNOT_LCC_PRODUCT_ID_STR       "Silverbolt LCC  "
#define FBE_ESES_ENCL_ANCHO_LCC_PRODUCT_ID_STR              "Ancho LCC       "
#define FBE_ESES_ENCL_VIKING_IOSXP_LCC_PRODUCT_ID_STR       "Viking IOSXP    "
#define FBE_ESES_ENCL_VIKING_DRVSXP_LCC_PRODUCT_ID_STR      "Viking DRVSXP   "
#define FBE_ESES_ENCL_VIKING_ICM_PRODUCT_ID_STR             "Viking ICM      "
#define FBE_ESES_ENCL_VIKING_LCC_PRODUCT_ID_STR             "Viking LCC      "
#define FBE_ESES_ENCL_CAYENNE_IOSXP_LCC_PRODUCT_ID_STR      "Cayenne IOSXP   "
#define FBE_ESES_ENCL_CAYENNE_DRVSXP_LCC_PRODUCT_ID_STR     "Cayenne DRVSXP  "
#define FBE_ESES_ENCL_CAYENNE_ICM_PRODUCT_ID_STR            "Cayenne ICM     "
#define FBE_ESES_ENCL_CAYENNE_LCC_PRODUCT_ID_STR            "Cayenne LCC     "
#define FBE_ESES_ENCL_NAGA_DRVSXP_LCC_PRODUCT_ID_STR        "Naga DRVSXP     "
#define FBE_ESES_ENCL_NAGA_IOSXP_LCC_PRODUCT_ID_STR         "Naga IOSXP      "
#define FBE_ESES_ENCL_NAGA_ICM_PRODUCT_ID_STR               "Naga ICM        "
#define FBE_ESES_ENCL_NAGA_LCC_PRODUCT_ID_STR               "Naga LCC        "
#define FBE_ESES_ENCL_LAPETUS_LCC_PRODUCT_ID_STR            "Lapetus BM      "
#define FBE_ESES_ENCL_RHEA_MIRANDA_LCC_PRODUCT_ID_STR       "Oberon LCC      "
#define FBE_ESES_ENCL_TABASCO_LCC_PRODUCT_ID_STR            "Tabasco LCC     "

// the time (in seconds) allowed for image activation to complete
// also used to set glitch ride through during image activation
/* Increased the expander reset time limit FBE_ESES_FW_RESET_TIME_LIMIT to 35 seconds. It was 25 seconds. 
 * However we saw multiple occurences that the expander reset time exceeded the 25 seconds with CDES-2. 
 * It caused the enclosure to be destroyed and all the drives got killed                  .
 * So I am chaning it to 35 seconds (AR775297).- PINGHE                                                                                       .
 */
#define FBE_ESES_FW_RESET_TIME_LIMIT        35      // 35 seconds for firmware download ride through
#define FBE_ESES_FW_ACTIVATE_TIME_LIMIT     35      // 35 seconds to activate image, before reset occurrs
#define FBE_ESES_FW_ACTIVATE_AND_RESET_LIMIT (FBE_ESES_FW_RESET_TIME_LIMIT + FBE_ESES_FW_ACTIVATE_TIME_LIMIT)
#define FBE_ESES_CDES2_FW_ACTIVATE_TIME_LIMIT      300      // 5 minutes to activate image, before reset occurrs
#define FBE_ESES_CDES2_FW_ACTIVATE_AND_RESET_LIMIT (FBE_ESES_FW_RESET_TIME_LIMIT + FBE_ESES_CDES2_FW_ACTIVATE_TIME_LIMIT)
#define FBE_ESES_PS_CM_FW_ACTIVATE_AND_RESET_LIMIT 300       // 5 minutes for power supply or cooling module activation
#define FBE_ESES_CDES2_PS_CM_FW_ACTIVATE_AND_RESET_LIMIT 600 // 10 minutes for CDES 2 power supplies
#define FBE_ESES_TUNNEL_CMD_TIME_LIMIT     10000     // 10 seconds timer for a tunneled command
#define FBE_ESES_LCC_RESET_TIME            45000    // 45 seconds timer for a LCC to reset
#define FBE_ESES_TUNNEL_GET_WAIT_TIME      1000     // 1 second
#define FBE_ESES_PEER_LCC_RESET_POLL_TIME  1000     // 1 second
#define FBE_ESES_TUNNEL_CMD_WAIT_TIME      100      // 100 milliseconds 
#define FBE_ESES_ENCL_FUP_PERMISSION_DENY_LIMIT_IN_SEC      600      // 10 minutes 
#define FBE_ESES_ENCLOSURE_DL_LOG_TIME 60000 // 60,000 milliseconds (1 minute) tunnel download to peer take a long time, so log status every 60 seconds.
#define FBE_ESES_ENCLOSURE_LCC_ACTIVATION_LOG_TIME 5000 // 5000 milliseconds (5 seconds) 
#define FBE_ESES_ENCLOSURE_PS_ACTIVATION_LOG_TIME 60000 // 60,000 milliseconds (1 minute) 

#define FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE               FBE_ENCL_FIRST_PE_COMPONENT_TYPE
#define FBE_ESES_ENCLOSURE_HA_MODE_DEFAULT                  1 // Enable the eses enclosure HA Mode as default. 
#define FBE_ESES_ENCLOSURE_DISABLE_INDICATOR_CTRL_DEFAULT   1 // Enable the eses enclosure Disable Indicator Control as default. 
#define FBE_ESES_ENCLOSURE_TEST_MODE_DEFAULT                0 // Disable the eses enclosure test mode as default. 
#define FBE_ESES_ENCLOSURE_INVALID_POS      0xFFFF
#define FBE_ESES_ENCLOSURE_OUTSTANDING_SCSI_REQUEST_MAX     1 // 

#define MAX_SIZE_EMC_SERIAL_NUMBER         FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE + 1

// we only support standalone external fan, not fan pack.
#define FBE_ESES_NUMBER_OF_ELEMENTS_PER_EXTERNAL_COOLING_UNIT     2 // one overall, one inidividual
#define FBE_ESES_MAX_SUPPORTED_EXTERNAL_COOLING_UNIT              3

#define FBE_ESES_MAX_SUPPORTED_PS                                 4

// maximum retries for SCSI command
#define MAX_CMD_RETRY   3

// These are frequencies with respect to the number of polls. This 
// means we do that particular event once for every "THIS" number 
// of polls. These should be divisors of FBE_ESES_ENCLOSURE_MAX_POLL_COUNT_WRAP.
#define FBE_ESES_ENCLOSURE_EMC_SPECIFIC_STATUS_POLL_FREQUENCY   1
#define FBE_ESES_ENCLOSURE_TS_TEMPERATUE_UPDATE_POLL_FREQUENCY  10
#define FBE_ESES_ENCLOSURE_INPUT_POWER_UPDATE_POLL_FREQUENCY    10        

#define FBE_ESES_ENCLOSURE_MAX_POLL_COUNT_WRAP         60

/* Increase to 12. Viking has 4 PS(each has one overall elements + two individual elements)*/
#define FBE_ESES_ENCLOSURE_MAX_PS_INFO_COLLECTED       12

//we use drive 0-3 to do vaulting in DPE (like jetfire)
#define FBE_ESES_ENCLOSURE_MAX_DRIVE_NUMBER_FOR_VAULTING       3 

#define FBE_ESES_ENCLOSURE_LCC_FAULT_DEBOUNCE_TIME        9  // in seconds.

#define FBE_ESES_ENCLOSURE_MAX_PRIMARY_PORT_COUNT  2 
#define FBE_ESES_ENCLOSURE_MAX_EXPANSION_PORT_COUNT  2
/*!*************************************************************************
 *  @enum fbe_eses_enclosure_lcc_t
 *  @brief 
 *    This defines all the LCCs.
 ***************************************************************************/ 
typedef enum {
    FBE_ESES_ENCLOSURE_LCC_A = 0,
    FBE_ESES_ENCLOSURE_LCC_B = 1,
}fbe_eses_enclosure_lcc_t;

typedef enum {
    FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW = 0,
    FBE_ESES_ENCLOSURE_FW_PRIORITY_HIGH
}fbe_enclosure_firmware_priority_t;

// fbe_eses_encl_fw_update_component_info_s contains information
// saved from the version descriptor list.
typedef struct fbe_eses_encl_fw_component_info_s
{
    fbe_enclosure_fw_target_t  enclFupComponent;
    fbe_u8_t                   enclFupComponentSide; 
    ses_comp_type_enum         fwEsesComponentType;
    fbe_u8_t                fwEsesComponentSubEnclId;   
    fbe_u8_t                fwEsesComponentElementIndex;  
    fbe_enclosure_firmware_priority_t fwPriority;
    fbe_bool_t              isDownloadOp;  // Used to differentiate the dowload from the activate.
    /* status related info */
    fbe_enclosure_firmware_status_t  enclFupOperationStatus;
    fbe_enclosure_firmware_status_t  prevEnclFupOperationStatus;
    fbe_enclosure_firmware_ext_status_t  enclFupOperationAdditionalStatus;
    fbe_enclosure_firmware_ext_status_t  prevEnclFupOperationAdditionalStatus; 
    /* something maybe useful coming off download status page */
    fbe_u32_t               fwEsesComponentMaxSize;   
    fbe_u32_t               fwEsesComponentExpectedBufferOffset;
    char                    fwEsesFwRevision[FBE_ESES_FW_REVISION_SIZE];
} fbe_eses_encl_fw_component_info_t;

typedef struct fbe_eses_encl_fw_info_s
{
    // we need to support all external fans
    fbe_eses_encl_fw_component_info_t   subencl_fw_info[FBE_FW_TARGET_MAX];
} fbe_eses_encl_fw_info_t;

typedef struct fbe_eses_encl_fup_permission_info_s
{
    fbe_bool_t  permissionOccupied;
    fbe_u32_t   permissionGrantTime;
    fbe_device_physical_location_t location;
} fbe_eses_encl_fup_permission_info_t;

/* All FBE timeouts in miliseconds */
enum fbe_eses_enclosure_timeouts_e {
    FBE_ESES_ENCLOSURE_ESES_PAGE_TIMEOUT = 10000, /* 10 sec */   
    FBE_ESES_ENCLOSURE_ESES_PAGE_DOWNLOAD_FIRMWARE = 60000, /* 60 sec */ 
};

typedef enum fbe_eses_pg_emc_stats_ctrl_elem_e{
    FBE_ESES_PG_EMC_STATS_CTRL_COMMON_ELEM          = 0x0000,
    FBE_ESES_PG_EMC_STATS_CTRL_POWER_SUPPLY_ELEM    = 0x0001,
    FBE_ESES_PG_EMC_STATS_CTRL_COOLING_ELEM         = 0x0002,
    FBE_ESES_PG_EMC_STATS_CTRL_TEMP_SENSOR_ELEM     = 0x0004,
    FBE_ESES_PG_EMC_STATS_CTRL_EXP_PHY_ELEM         = 0x0008,
    FBE_ESES_PG_EMC_STATS_CTRL_ARRAY_DEV_SLOT_ELEM  = 0x0010,
    FBE_ESES_PG_EMC_STATS_CTRL_SAS_EXP_ELEM         = 0x0020
}fbe_eses_pg_emc_stats_ctrl_elem_t;

typedef enum fbe_eses_mode_select_mode_page_e{
    FBE_ESES_MODE_SELECT_EEP_MODE_PAGE             = 0x0001,  // EMC ESES Persistent mode page.
    FBE_ESES_MODE_SELECT_EENP_MODE_PAGE            = 0x0010   // EMC ESES non-Persistent mode page.
}fbe_eses_mode_select_mode_page_t;  

/* These are the lifecycle condition ids for a eses enclosure object. */
typedef enum fbe_eses_enclosure_lifecycle_cond_id_e {
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_ESES_ENCLOSURE),
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_ADDL_STATUS_UNKNOWN,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_STATUS_UNKNOWN,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CHILDREN_UNKNOWN,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_IDENTITY_NOT_VALIDATED,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_CONFIGURATION_UNKNOWN,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_FIRMWARE_DOWNLOAD,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_DUPLICATE_ESN_UNCHECKED,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSENSED,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_MODE_UNSELECTED,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_STATISTICS_STATUS_UNKNOWN,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_SAS_ENCL_TYPE_UNKNOWN,
    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_RP_SIZE_UNKNOWN,

    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_LAST /* must be last */
} fbe_eses_enclosure_lifecycle_cond_id_t;

typedef enum fbe_eses_ctrl_opcode_e {
    FBE_ESES_CTRL_OPCODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_ESES_ENCLOSURE), /* sets the upper 2 bytes */
    FBE_ESES_CTRL_OPCODE_GET_CONFIGURATION,         /* 1 - configuration page */
    FBE_ESES_CTRL_OPCODE_GET_ENCL_STATUS,           /* status page */
    FBE_ESES_CTRL_OPCODE_GET_ADDITIONAL_STATUS,     /* additional status page */
    FBE_ESES_CTRL_OPCODE_GET_EMC_SPECIFIC_STATUS,   /* EMC specific status page */
    FBE_ESES_CTRL_OPCODE_GET_TRACE_BUF_INFO_STATUS, /* 5- EMC specific status page */
    FBE_ESES_CTRL_OPCODE_GET_DOWNLOAD_STATUS,       /* Download microcode status page */
    FBE_ESES_CTRL_OPCODE_SET_EMC_SPECIFIC_CTRL,     /* EMC specific control page */
    FBE_ESES_CTRL_OPCODE_SET_TRACE_BUF_INFO_CTRL,   /* EMC specific control page */
    FBE_ESES_CTRL_OPCODE_DOWNLOAD_FIRMWARE,         /* Download microcode control page */
    FBE_ESES_CTRL_OPCODE_SET_ENCL_CTRL,             /* 10 - control page */
    FBE_ESES_CTRL_OPCODE_READ_BUF,                  /* Read buffer command */
    FBE_ESES_CTRL_OPCODE_STRING_OUT_CMD,            /* String out diagnostic control page */
    FBE_ESES_CTRL_OPCODE_MODE_SENSE,                /* Mode sense (10) comand */
    FBE_ESES_CTRL_OPCODE_MODE_SELECT,               /* Mode select (10) command */
    FBE_ESES_CTRL_OPCODE_GET_INQUIRY_DATA,          /* 15 - Inquiry command */
    FBE_ESES_CTRL_OPCODE_VALIDATE_IDENTITY,         /* Inquiry command */
    FBE_ESES_CTRL_OPCODE_READ_RESUME,               /* read resume prom */
    FBE_ESES_CTRL_OPCODE_WRITE_RESUME,              /* write resume prom */
    FBE_ESES_CTRL_OPCODE_RAW_RCV_DIAG,              /* pass through receive diag command */
    FBE_ESES_CTRL_OPCODE_RAW_INQUIRY,               /* Raw inquiry command */
    FBE_ESES_CTRL_OPCODE_EMC_STATISTICS_STATUS_CMD, /* EMC Statistics status page */
    FBE_ESES_CTRL_OPCODE_GET_EMC_STATISTICS_STATUS, /* EMC Statistics status page */
    FBE_ESES_CTRL_OPCODE_THRESHOLD_IN_CMD,         /* Threshold In diagnostic control page */
    FBE_ESES_CTRL_OPCODE_THRESHOLD_OUT_CMD,         /* Threshold out diagnostic control page */
    FBE_ESES_CTRL_OPCODE_SPS_IN_BUF_CMD,            /* SPS In Buffer page */
    FBE_ESES_CTRL_OPCODE_SPS_EEPROM_CMD,            /* SPS EEPROM command */
    FBE_ESES_CTRL_OPCODE_TUNNEL_GET_CONFIGURATION,   /* Tunnel get configuration page */ 
    FBE_ESES_CTRL_OPCODE_TUNNEL_DOWNLOAD_FIRMWARE,   /* Tunnel download microcode control page */ 
    FBE_ESES_CTRL_OPCODE_TUNNEL_GET_DOWNLOAD_STATUS, /* Tunnel receive download microcode status page */ 
    FBE_ESES_CTRL_OPCODE_GET_TUNNEL_CMD_STATUS,      /* Tunnel command status page (0x83) */
    FBE_ESES_CTRL_OPCODE_GET_SAS_ENCL_TYPE,         /* configuration page primary subencl product id*/
    FBE_ESES_CTRL_OPCODE_GET_RP_SIZE,
    FBE_ESES_CTRL_OPCODE_LAST
}fbe_eses_ctrl_opcode_t;

typedef enum fbe_eses_update_lifecycle_cond_modes_e{
    FBE_ESES_UPDATE_COND_EMC_SPECIFIC_CONTROL_NEEDED    = 0x0001,
    FBE_ESES_UPDATE_COND_MODE_UNSENSED                  = 0x0010,

    // must be set to all one's
    FBE_ESES_UPDATE_COND_NONE                           =0x1111
}fbe_eses_update_lifecycle_cond_modes_t;  

typedef struct fbe_eses_enclosure_drive_info_s{
    fbe_sas_address_t sas_address;
    fbe_payload_discovery_element_type_t element_type;
    fbe_generation_code_t generation_code;   
    fbe_u32_t power_cycle_duration; 
    fbe_bool_t  drive_need_power_cycle;
    fbe_bool_t  drive_battery_backed;
} fbe_eses_enclosure_drive_info_t;

typedef struct fbe_eses_enclosure_expansion_port_info_s{
    fbe_sas_address_t sas_address;
    fbe_payload_discovery_element_type_t element_type;
    fbe_generation_code_t generation_code;
    fbe_u8_t chain_depth;
    fbe_bool_t  attached_encl_fw_activating;            // attached enclosure is activating new fw
} fbe_eses_enclosure_expansion_port_info_t;

// Power Supply related EIR(Energy Information Reporting) data
typedef struct fbe_eses_enclosure_ps_eir_info_s{
    fbe_u8_t    valid_input_power_sample_count; // Valid input power samples over a finite time period
    fbe_u8_t    invalid_input_power_sample_count; // Invalid input power samples over a finite time period
    fbe_u32_t   total_input_power;  // Total input power over a finite time period
} fbe_eses_enclosure_ps_eir_info_t;

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(eses_enclosure);

#define ESES_SUPPORTED_SIDES    2
typedef struct fbe_eses_enclosure_properties_s{
    /* These parameters are filled in by the leaf classes. */
    fbe_bool_t                   *connector_disable_list_ptr;
    fbe_u8_t                   *drive_slot_to_phy_index[ESES_SUPPORTED_SIDES]; /* hard code slot-phy_index mapping for side 0 and 1 */
    fbe_u8_t                   *connector_index_to_phy_index; /* hard code connector-phy mapping */
    fbe_u8_t                   *phy_index_to_phy_id;  /* hard code phy index to id mapping */
    fbe_u8_t                    number_of_comp_types;
    fbe_u8_t                    number_of_power_supplies;
    fbe_u8_t                    number_of_power_supply_subelem;     // Initialized to 0 by eses incase leaf class forgets.
    fbe_u8_t                    number_of_power_supplies_per_side;     // Initialized to 0 by eses incase leaf class forgets.
    fbe_u8_t                    number_of_cooling_components;
    fbe_u8_t                    number_of_cooling_components_per_ps;
    fbe_u8_t                    number_of_cooling_components_on_chassis;
    fbe_u8_t                    number_of_cooling_components_external;
    fbe_u8_t                    number_of_cooling_components_on_lcc;
    fbe_u8_t                    number_of_slots;
    fbe_u8_t                    max_encl_slot;
    fbe_u8_t                    number_of_temp_sensors;
    fbe_u8_t                    number_of_temp_sensors_per_lcc;
    fbe_u8_t                    number_of_temp_sensors_on_chassis;
    fbe_u8_t                    number_of_expanders;
    fbe_u8_t                    number_of_expanders_per_lcc;
    fbe_u8_t                    number_of_expander_phys;
    fbe_u8_t                    number_of_related_expanders;  // 0 for Viper and Derringer, but 2 for Voyager.
    fbe_u8_t                    number_of_encl;
    fbe_u8_t                    number_of_connectors;
    fbe_u8_t                    number_of_connectors_per_lcc;
    fbe_u8_t                    number_of_lanes_per_entire_connector;
    fbe_u8_t                    number_of_lccs;
    fbe_u8_t                    number_of_lccs_with_temp_sensor;
    fbe_u8_t                    number_of_display_chars;
    fbe_u8_t                    number_of_two_digit_displays;
    fbe_u8_t                    number_of_one_digit_displays;
    fbe_u8_t                    number_of_possible_elem_groups; // The maximum possible number of elem groups. 
    fbe_u8_t                    number_of_actual_elem_groups; // The current actual number of elem groups. 
    fbe_u8_t                    number_of_sps;
    fbe_u8_t                    number_of_ssc;
    fbe_u8_t                    first_slot_index;
    fbe_u8_t                    number_of_expansion_ports;
    fbe_u8_t                    first_expansion_port_index;
    fbe_u8_t                    number_of_trace_buffers;    
    fbe_u8_t                    enclosure_fw_dl_retry_max;
    fbe_u8_t                    fw_num_expander_elements;
    fbe_u8_t                    fw_num_subencl_sides;
    fbe_u8_t                    fw_lcc_version_descs;
    fbe_u8_t                    fw_ps_version_descs;
    fbe_u8_t                    default_ride_through_period;    
    fbe_u8_t                    number_of_command_retry_max;
    fbe_u8_t                    number_of_drive_midplane;
    fbe_bool_t                  isPsResumeFormatSpecial;
    fbe_bool_t                  isPsOverallElemSaved;
}fbe_eses_enclosure_properties_t;

// The returned external status for firmware upgrade FSM.  
typedef enum
{
    FBE_ESES_TUNNEL_FUP_DONE,       // Nothing more to do.
    FBE_ESES_TUNNEL_FUP_PEND,       // Wait for a SCSI commands to be processed.
    FBE_ESES_TUNNEL_FUP_DELAY,      // Schedule for more processing after delay 
    FBE_ESES_TUNNEL_FUP_FAIL        // Abnormally stop the FSM. 
} fbe_eses_tunnel_fup_schedule_op_t;

// Tunnel download and activate FSM 
typedef enum
{
    // The order and size of the enum affect the fbe_eses_download_fsm_table and fbe_eses_activate_fsm_table definition. 
    FBE_ESES_ST_TUNNEL_FUP_INIT,
    FBE_ESES_ST_TUNNEL_FUP_SEND_GET_CONFIG,
    FBE_ESES_ST_TUNNEL_FUP_READY,
    FBE_ESES_ST_TUNNEL_FUP_TUNNEL_SEND_DL_CTRL_PG,
    FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_SEND_DL_CTRL_PG_STATUS,
    FBE_ESES_ST_TUNNEL_FUP_TUNNEL_RECEIVE_DL_STATUS_PG,
    FBE_ESES_ST_TUNNEL_FUP_GET_TUNNEL_RECEIVE_DL_STATUS_PG_STATUS,
    FBE_ESES_ST_TUNNEL_FUP_WAIT_FOR_RESET,
    FBE_ESES_ST_TUNNEL_FUP_LAST // invalid state; used as fsm_table boundary
} fbe_eses_tunnel_fup_state_t;

typedef enum
{
    // The order and size of the enum affect the fbe_eses_tunnel_download_fsm_table and fbe_eses_activate_fsm_table definition. 
    FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL, // start the tunnel download or activation
    FBE_ESES_EV_TUNNEL_FUP_SUCCEEDED, // the command succeeded
    FBE_ESES_EV_TUNNEL_FUP_PROCESSING, // still processing command
    FBE_ESES_EV_TUNNEL_FUP_FAILED, // the command failed
    FBE_ESES_EV_TUNNEL_FUP_BUSY, // the device didn't dropped the command
    FBE_ESES_EV_TUNNEL_FUP_TUNNELED_CMD_FAILED, // the received status (page 0x83) says the tunneled command failed.
    FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL_FAILED, // the download status (page 0x0E) says the download failed.
    FBE_ESES_EV_TUNNEL_FUP_LAST // invalid event; used as fsm_table boundary 
} fbe_eses_tunnel_fup_event_t;

#define FBE_ESES_TUNNEL_DOWNLOAD_MAX_FAILURE_RETRY 5
#define FBE_ESES_TUNNEL_DOWNLOAD_MAX_BUSY_RETRY 5
#define FBE_ESES_TUNNEL_ACTIVATE_MAX_FAILURE_RETRY 5
#define FBE_ESES_TUNNEL_ACTIVATE_MAX_BUSY_RETRY 5

typedef struct fbe_eses_tunnel_fup_context_s
{
    fbe_eses_tunnel_fup_state_t current_state;
    fbe_eses_tunnel_fup_state_t previous_state;
    fbe_eses_tunnel_fup_event_t new_event; // The new event to feed into the FUP FSM in next cycle.
    fbe_u8_t max_failure_retry_count; // Max number of retries after getting the failure status.
    fbe_u8_t max_busy_retry_count; // Max number of retries after getting the busy status.
    fbe_u8_t failure_retry_count; // Number of times retried after getting the failure status.
    fbe_u8_t busy_retry_count; // Number of times retried after getting the busy status.
    fbe_eses_tunnel_fup_schedule_op_t schedule_op; // The schedule action we should take next.
    fbe_u32_t generation_code; // The generation code for the download target. 
    ses_pg_code_enum expected_pg_code; // The page code expected from the data field of page 0x83 
    fbe_lifecycle_timer_msec_t delay; // delay in milliseconds for next event processig
    fbe_time_t tunnel_cmd_start; // the time a tunnel command starts in milliseconds.
    fbe_time_t time_marker; // a general purpose time marker to record the start time in milliseconds.
    struct fbe_eses_enclosure_s *eses_enclosure;
    fbe_packet_t *packet;
} fbe_eses_tunnel_fup_context_t;

typedef struct fbe_eses_tunnel_fup_fsm_table_entry_s
{
    fbe_eses_tunnel_fup_state_t (*transition_func)(fbe_eses_tunnel_fup_context_t *context_p,
                                                   fbe_eses_tunnel_fup_event_t event,
                                                   fbe_eses_tunnel_fup_state_t next_state);
    fbe_eses_tunnel_fup_state_t next_state;
} fbe_eses_tunnel_fup_fsm_table_entry_t;

typedef struct fbe_eses_enclosure_s
{
    fbe_sas_enclosure_t       sas_enclosure;
    fbe_u32_t                 status_page_requiered;
    fbe_u32_t                 eses_generation_code;  // updated in config page
    fbe_u16_t                 eses_version_desc;
    fbe_u16_t                 eses_status_control_page_length;
    fbe_sas_enclosure_type_t  sas_enclosure_type;
    //fbe_u32_t                 unique_id;
    fbe_u8_t                  encl_serial_number[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE+1];
    fbe_u8_t                  day; // used for time sync.
    fbe_eses_enclosure_expansion_port_info_t *expansion_port_info;  // index by connector id
    fbe_eses_enclosure_drive_info_t *drive_info;  //index by device slot index

    fbe_eses_enclosure_properties_t properties;

    fbe_eses_elem_group_t * elem_group;

    fbe_spinlock_t  enclGetInfoLock;            // spin lock to protect updating/copying of Status/Read data

    fbe_bool_t  enclosureConfigBeingUpdated;            // configuration has changed & is being updated (unstable)
    fbe_bool_t  inform_fw_activating;                   // inform upstream enclosure about the activation of new fw
  
    fbe_u8_t  primary_port_entire_connector_index[FBE_ESES_ENCLOSURE_MAX_PRIMARY_PORT_COUNT];    // where we come from

    fbe_u8_t  outstanding_write;  
    fbe_u8_t  emc_encl_ctrl_outstanding_write;

    // Used to keep track of the outstanding scsi requests.
    fbe_eses_ctrl_opcode_t outstanding_scsi_request_opcode; // Will need an array if we support more than 1 outstanding request.
    FBE_ALIGN(16)fbe_atomic_t    outstanding_scsi_request_count;    /*!< Number of scsi requests currently in flight. */
    fbe_u32_t outstanding_scsi_request_max;

    fbe_u8_t mode_select_needed;  // For mode select command.
    fbe_u8_t test_mode_status  : 1; // The value indicates the status of test mode. 1-> on, 0-> off.
    fbe_u8_t test_mode_rqstd   : 1; // The value indicates the requested test mode. 1-> on, 0-> off.
    fbe_u8_t sps_dev_supported : 1;
    fbe_u8_t reserved          : 5; // Reserved for other flags.
    fbe_u8_t reset_reason;

    // FW Update Info
    fbe_eses_encl_fup_info_t                        enclCurrentFupInfo;
    /* TO DO: this will be moved to data store later, 
     * added one more space for power supply on side 1. 
     */
    fbe_eses_encl_fw_info_t                   * enclFwInfo;

    /* In ESP, board_mgmt and module_mgmt is in charge of upgrading enclosure 0_0 on some DPE.
     * Meanwhile encl_mgmt is do upgrade for all other enclosures. So there is possiblity they upgrade
     * enclosures at the same time, which will cause problems. To solve that, we use this lock to 
     * ensure only one enclosure to upgrade at one time. 
     * on DPE board_mgmt moudle_mgmt and encl_mgmt should send request to encl 0_0 to race for this permission.
     */
    fbe_spinlock_t  enclGetFupPermissionLock;
    fbe_eses_encl_fup_permission_info_t  enclosurePermission;

    // tunnelling FW upgrade for peer side
    fbe_eses_tunnel_fup_context_t tunnel_fup_context_p;

    fbe_u8_t            active_state_to_update_cond;
    fbe_u8_t update_lifecycle_cond;
    fbe_u8_t mode_sense_retry_cnt;

    // This is actually not an absolute poll count
    // but wraps around.
    fbe_u8_t    poll_count;

    fbe_eses_enclosure_ps_eir_info_t    ps_eir_info[FBE_ESES_ENCLOSURE_MAX_PS_INFO_COLLECTED]; 

    fbe_encl_fault_led_reason_t enclFaultLedReason;         // reason the Enclosure Fault LED is on

    fbe_bool_t reset_shutdown_timer;

    FBE_LIFECYCLE_DEF_INST_DATA;
 
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_ESES_ENCLOSURE_LIFECYCLE_COND_LAST));
}fbe_eses_enclosure_t;

// Enumeration for LED operations
typedef enum
{
    FBE_ESES_FLASH_ON_ALL_DRIVES = 0,
    FBE_ESES_FLASH_OFF_ALL_DRIVES
} fbe_eses_led_actions_t;

typedef struct fbe_eses_enclosure_buf_info_s
{
    fbe_enclosure_component_types_t componentType;
    fbe_u32_t componentIndex;
    fbe_u32_t bufId;
} fbe_eses_enclosure_buf_info_t;


typedef struct fbe_eses_enclosure_format_fw_rev_s
{
    fbe_u32_t majorNumber;
    fbe_u32_t minorNumber;
    fbe_u32_t patchNumber;
} fbe_eses_enclosure_format_fw_rev_t;

/* Methods */
/* Class methods forward declaration */
fbe_status_t fbe_eses_enclosure_load(void);
fbe_status_t fbe_eses_enclosure_unload(void);
fbe_status_t fbe_eses_enclosure_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_eses_enclosure_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_eses_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_eses_enclosure_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);
fbe_status_t fbe_eses_enclosure_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_eses_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_eses_enclosure_monitor_load_verify(void);

fbe_status_t fbe_eses_enclosure_init(fbe_eses_enclosure_t * fbe_eses_enclosure_t);

/* fbe_eses_enclosure_executer.c */
fbe_status_t fbe_eses_enclosure_send_get_element_list_command(fbe_eses_enclosure_t * eses_enclosure, 
                                                              fbe_packet_t * packet);
fbe_status_t fbe_eses_enclosure_send_get_config_completion(fbe_packet_t * packet, 
                                                           fbe_packet_completion_context_t context);
fbe_enclosure_status_t fbe_eses_enclosure_process_download_ucode_status(fbe_eses_enclosure_t *eses_enclosure, 
                                                              fbe_eses_download_status_page_t * dl_status_page );
fbe_status_t fbe_eses_enclosure_send_scsi_cmd(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet); 

fbe_status_t 
fbe_eses_enclosure_validate_encl_SN(fbe_eses_enclosure_t * eses_enclosure, 
                                            fbe_packet_t * packet);
fbe_status_t 
fbe_eses_enclosure_check_dup_encl_SN(fbe_eses_enclosure_t * eses_enclosure, 
                                            fbe_packet_t * packet);
fbe_status_t 
fbe_eses_enclosure_notify_upstream_fw_activation(fbe_eses_enclosure_t * eses_enclosure, 
                                            fbe_packet_t * packet);

fbe_enclosure_status_t  fbe_eses_enclosure_handle_unsupported_additional_status_page(fbe_eses_enclosure_t * eses_enclosure);

fbe_status_t 
fbe_eses_enclosure_discovery_transport_entry(fbe_base_object_t * base_object, 
                                         fbe_packet_t * packet);

fbe_bool_t fbe_eses_enclosure_is_activate_timeout(fbe_eses_enclosure_t * eses_enclosure);
fbe_bool_t fbe_eses_enclosure_is_rev_change(fbe_eses_enclosure_t *eses_enclosure_p);
fbe_status_t fbe_eses_enclosure_fup_check_for_more_work(fbe_eses_enclosure_t *eses_enclosure);
fbe_status_t fbe_eses_enclosure_fup_check_for_more_bytes(fbe_eses_enclosure_t *eses_enclosure);
fbe_status_t fbe_eses_enclosure_fup_check_for_resend(fbe_packet_t * packet,
                                                        fbe_eses_enclosure_t *eses_enclosure);
fbe_status_t fbe_eses_enclosure_handle_activate_op(fbe_packet_t * packet,
                                                   fbe_eses_enclosure_t *eses_enclosure);

fbe_enclosure_status_t fbe_eses_enclosure_handle_scsi_cmd_response(fbe_eses_enclosure_t * eses_enclosure, 
                                                   fbe_packet_t * packet,
                                                   fbe_eses_ctrl_opcode_t opcode,
                                                   fbe_scsi_error_code_t * scsi_error_code_p);

fbe_enclosure_status_t fbe_eses_control_page_write_complete(fbe_eses_enclosure_t * eses_enclosure);
fbe_enclosure_status_t fbe_eses_emc_encl_ctrl_page_write_complete(fbe_eses_enclosure_t * eses_enclosure);
fbe_status_t fbe_eses_enclosure_set_outstanding_scsi_request_opcode(fbe_eses_enclosure_t * eses_enclosure,
                                                          fbe_eses_ctrl_opcode_t opcode);

/* fbe_eses_enclosure_read.c */
fbe_enclosure_status_t  fbe_eses_enclosure_ps_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t  fbe_eses_enclosure_connector_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t  fbe_eses_enclosure_array_dev_slot_extract_status(void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t  fbe_eses_enclosure_exp_phy_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t  fbe_eses_enclosure_cooling_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t  fbe_eses_enclosure_temp_sensor_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t fbe_eses_enclosure_encl_extract_status(
                                              void *enclosure,
                                              fbe_u8_t group_id,
                                              fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t  fbe_eses_enclosure_chassis_extract_status(void *enclosure,
                                                                  fbe_u8_t group_id,
                                                                  fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t fbe_eses_enclosure_display_extract_status(void *enclosure,
                                                                 fbe_u8_t group_id,
                                                                 fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t fbe_eses_enclosure_expander_extract_status(void *enclosure,
                                                                 fbe_u8_t group_id,
                                                                 fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t fbe_eses_enclosure_sps_extract_status(void *enclosure,
                                                             fbe_u8_t group_id,
                                                             fbe_eses_ctrl_stat_elem_t * group_overall_status);

fbe_enclosure_status_t fbe_eses_enclosure_esc_elec_extract_status(void *enclosure,
                                                             fbe_u8_t group_id,
                                                             fbe_eses_ctrl_stat_elem_t * group_overall_status);

/* fbe_eses_enclosure_process_pages.c */
fbe_enclosure_status_t fbe_eses_enclosure_process_configuration_page(fbe_eses_enclosure_t * eses_enclosure,                                                                
                                                                fbe_u8_t * config_page);

fbe_enclosure_status_t fbe_eses_enclosure_get_sas_encl_type_from_config_page(fbe_eses_enclosure_t * eses_enclosure,
                                                              ses_pg_config_struct *config_page_p);

fbe_enclosure_status_t fbe_eses_enclosure_process_addl_status_page(
                                    struct fbe_eses_enclosure_s * eses_enclosure,
                                    fbe_u8_t * addl_status_page);

fbe_enclosure_status_t fbe_eses_enclosure_process_status_page(struct fbe_eses_enclosure_s * eses_enclosure, 
                                                         fbe_u8_t * status_page);

fbe_enclosure_status_t fbe_eses_enclosure_process_emc_specific_page(
                                                    fbe_eses_enclosure_t * eses_enclosure, 
                                                    ses_pg_emc_encl_stat_struct * emc_specific_status_page_p);

fbe_enclosure_status_t fbe_eses_enclosure_process_emc_statistics_page(fbe_eses_enclosure_t * eses_enclosure, 
                                          ses_pg_emc_statistics_struct * emc_statistics_status_page_p);

fbe_enclosure_status_t fbe_eses_enclosure_populate_connector_primary_port(fbe_eses_enclosure_t * eses_enclosure);

fbe_enclosure_status_t fbe_eses_enclosure_get_info_elem_group_hdr_p(fbe_eses_enclosure_t * eses_enclosure, 
                                                ses_pg_emc_encl_stat_struct * emc_specific_status_page_p,
                                                fbe_eses_info_elem_type_t info_elem_type,
                                                fbe_eses_info_elem_group_hdr_t ** info_elem_group_hdr_p);

fbe_enclosure_status_t fbe_eses_enclosure_process_mode_param_list(fbe_eses_enclosure_t * eses_enclosure,
                                                                  fbe_eses_mode_param_list_t * mode_param_list_p);

fbe_enclosure_status_t fbe_eses_enclosure_process_download_ucode_status(fbe_eses_enclosure_t *eses_enclosure, 
                                                                        fbe_eses_download_status_page_t  * dl_status_page_p);

fbe_enclosure_status_t fbe_eses_enclosure_process_emc_statistics_status_page(
                                          fbe_eses_enclosure_t * eses_enclosure, 
                                          ses_pg_emc_statistics_struct * emc_statistics_status_page_p);

/* fbe_eses_enclosure_main.c*/
void fbe_eses_encl_init_encl_component_data(fbe_enclosure_component_t *EsesEnclComponentDataPtr);
fbe_status_t fbe_eses_enclosure_init_enclCurrentFupInfo(fbe_eses_enclosure_t * pEsesEncl);
fbe_status_t fbe_eses_enclosure_init_enclFwInfo(fbe_eses_enclosure_t * pEsesEncl);

// fbe_eses_enclosure_monitor.c
fbe_bool_t 
fbe_eses_enclosure_saveControlPageInfo(fbe_eses_enclosure_t * eses_enclosure,
                                       void *newControlInfoPtr);

/* fbe_eses_enclosure_build_pages.c */
fbe_status_t 
fbe_eses_enclosure_buildControlPage(fbe_eses_enclosure_t * eses_enclosure,     
                                    fbe_sg_element_t * sg_list,
                                    fbe_u16_t * page_size_ptr,
                                    fbe_u32_t *sg_count);

fbe_status_t 
fbe_eses_enclosure_buildStringOutPage(fbe_eses_enclosure_t * eses_enclosure, 
                                      fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                      fbe_u16_t *parameter_length,
                                      fbe_sg_element_t * sg_list,
                                      fbe_u32_t *sg_count);

fbe_status_t 
fbe_eses_enclosure_buildThresholdOutPage(fbe_eses_enclosure_t * eses_enclosure, 
                                      fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                      fbe_u16_t *page_size_ptr,
                                      fbe_sg_element_t * sg_list,
                                      fbe_u32_t *sg_count);
fbe_status_t fbe_eses_enclosure_build_emc_specific_ctrl_page(fbe_eses_enclosure_t * eses_enclosure,
                                                         fbe_enclosure_mgmt_trace_buf_cmd_t *trace_buf_cmd,
                                                         fbe_sg_element_t * sg_list,
                                                         fbe_u16_t * page_size_ptr,
                                                         fbe_u32_t *sg_count);

fbe_status_t fbe_eses_ucode_build_download_or_activate(fbe_eses_enclosure_t * eses_enclosure_p, 
                                                        fbe_sg_element_t *sg_p, 
                                                        fbe_u16_t *parameter_length_p,
                                                        fbe_u32_t *sg_count);

fbe_status_t
fbe_eses_enclosure_fup_send_funtional_packet(fbe_eses_enclosure_t *eses_enclosure,
                                         fbe_packet_t *packet,
                                         fbe_eses_ctrl_opcode_t opcode);

fbe_status_t fbe_eses_ucode_build_tunnel_download_or_activate(fbe_eses_enclosure_t * eses_enclosure_p,
                                                              fbe_sg_element_t *sg_p,
                                                              fbe_u16_t *parameter_length_p,
                                                              fbe_u32_t *sg_count);
fbe_status_t
fbe_eses_ucode_build_tunnel_receive_diag_results_cdb(fbe_eses_enclosure_t * eses_enclosure_p,
                                                     ses_pg_code_enum page_code,
                                                     fbe_sg_element_t *sg_p,
                                                     fbe_u16_t *parameter_length_p,
                                                     fbe_u32_t *sg_count);

fbe_eses_tunnel_fup_schedule_op_t
fbe_eses_tunnel_download_fsm(fbe_eses_tunnel_fup_context_t *context_p, fbe_eses_tunnel_fup_event_t event);

fbe_eses_tunnel_fup_schedule_op_t
fbe_eses_tunnel_activate_fsm(fbe_eses_tunnel_fup_context_t *context_p, fbe_eses_tunnel_fup_event_t event);

void
fbe_eses_tunnel_fup_context_init(fbe_eses_tunnel_fup_context_t *context_p,
                                 fbe_eses_enclosure_t *eses_enclosure_p,
                                 fbe_eses_tunnel_fup_state_t state,
                                 fbe_eses_tunnel_fup_event_t event,
                                 fbe_u8_t max_failure_retry,
                                 fbe_u8_t max_busy_retry);

void
fbe_eses_tunnel_fup_context_set_packet(fbe_eses_tunnel_fup_context_t *context_p,
                                       fbe_packet_t *packet_p);

void
fbe_eses_tunnel_fup_context_set_event(fbe_eses_tunnel_fup_context_t *context_p, 
                                      fbe_eses_tunnel_fup_event_t event);

fbe_eses_tunnel_fup_event_t
fbe_eses_tunnel_fup_context_get_event(fbe_eses_tunnel_fup_context_t *context_p);

fbe_eses_tunnel_fup_event_t
fbe_eses_tunnel_fup_encl_status_to_event(fbe_enclosure_status_t encl_status);

void
fbe_eses_enclosure_tunnel_set_delay(fbe_eses_tunnel_fup_context_t* fup_context_p,
                                    fbe_lifecycle_timer_msec_t delay);

fbe_lifecycle_timer_msec_t
fbe_eses_enclosure_tunnel_get_delay(fbe_eses_tunnel_fup_context_t* fup_context_p);

fbe_enclosure_status_t
fbe_eses_enclosure_handle_status_page_83h(fbe_eses_enclosure_t *eses_enclosure,
                                          fbe_sg_element_t *sg_list);
fbe_lifecycle_status_t
fbe_eses_enclosure_handle_schedule_op(fbe_eses_enclosure_t *eses_enclosure, fbe_eses_tunnel_fup_schedule_op_t schedule_op);

fbe_enclosure_status_t fbe_eses_enclosure_build_mode_param_list(fbe_eses_enclosure_t * eses_enclosure,                                                      
                                                         fbe_sg_element_t * sg_list,
                                                         fbe_u16_t * page_size_ptr,
                                                         fbe_u32_t *sg_count);

// Setup Control Handler functions
fbe_enclosure_status_t 
fbe_eses_enclosure_array_dev_slot_setup_control(void * enclosure,
                                                fbe_u32_t index,
                                                fbe_u32_t compType,
                                                fbe_u8_t *ctrl_data);
fbe_enclosure_status_t 
fbe_eses_enclosure_exp_phy_setup_control(void * enclosure,
                                         fbe_u32_t index,
                                         fbe_u32_t compType,
                                         fbe_u8_t *ctrl_data);
fbe_enclosure_status_t 
fbe_eses_enclosure_connector_setup_control(void * enclosure,
                                           fbe_u32_t index,
                                           fbe_u32_t compType,
                                           fbe_u8_t *ctrl_data);
fbe_enclosure_status_t 
fbe_eses_enclosure_ps_setup_control(void * enclosure,
                                    fbe_u32_t index,
                                    fbe_u32_t compType,
                                    fbe_u8_t *ctrl_data);
fbe_enclosure_status_t 
fbe_eses_enclosure_cooling_setup_control(void * enclosure,
                                         fbe_u32_t index,
                                         fbe_u32_t compType,
                                         fbe_u8_t *ctrl_data);
fbe_enclosure_status_t 
fbe_eses_enclosure_encl_setup_control(void * enclosure,
                                      fbe_u32_t index,
                                      fbe_u32_t compType,
                                      fbe_u8_t *ctrl_data);
fbe_enclosure_status_t 
fbe_eses_enclosure_temp_sensor_setup_control(void * enclosure,
                                             fbe_u32_t index,
                                             fbe_u32_t compType,
                                             fbe_u8_t *ctrl_data);
fbe_enclosure_status_t 
fbe_eses_enclosure_display_setup_control(void * enclosure,
                                         fbe_u32_t index,
                                         fbe_u32_t compType,
                                         fbe_u8_t *ctrl_data);
// Copy Control Handler functions
fbe_enclosure_status_t 
fbe_eses_enclosure_array_dev_slot_copy_control(void *enclosure,
                                               fbe_u32_t index,
                                               fbe_u32_t compType,
                                               void *controlDataPtr);
fbe_enclosure_status_t 
fbe_eses_enclosure_exp_phy_copy_control(void *enclosure,
                                        fbe_u32_t index,
                                        fbe_u32_t compType,
                                        void *controlDataPtr);
fbe_enclosure_status_t 
fbe_eses_enclosure_connector_copy_control(void *enclosure,
                                          fbe_u32_t index,
                                          fbe_u32_t compType,
                                          void *controlDataPtr);
fbe_enclosure_status_t 
fbe_eses_enclosure_ps_copy_control(void *enclosure,
                                   fbe_u32_t index,
                                   fbe_u32_t compType,
                                   void *controlDataPtr);
fbe_enclosure_status_t 
fbe_eses_enclosure_cooling_copy_control(void *enclosure,
                                        fbe_u32_t index,
                                        fbe_u32_t compType,
                                        void *controlDataPtr);
fbe_enclosure_status_t 
fbe_eses_enclosure_encl_copy_control(void *enclosure,
                                     fbe_u32_t index,
                                     fbe_u32_t compType,
                                     void *controlDataPtr);
fbe_enclosure_status_t 
fbe_eses_enclosure_temp_sensor_copy_control(void *enclosure,
                                            fbe_u32_t index,
                                            fbe_u32_t compType,
                                            void *controlDataPtr);
fbe_enclosure_status_t 
fbe_eses_enclosure_display_copy_control(void *enclosure,
                                        fbe_u32_t index,
                                        fbe_u32_t compType,
                                        void *controlDataPtr);



fbe_enclosure_status_t fbe_eses_enclosure_build_str_out_diag_page(fbe_eses_enclosure_t * eses_enclosure,
                                                         fbe_enclosure_mgmt_debug_cmd_t * debug_cmd_p,                                                        
                                                         fbe_sg_element_t * sg_list,
                                                         fbe_u16_t * page_size_p,
                                                         fbe_u32_t *sg_count);

/* fbe_eses_enclosure_usurper.c*/
fbe_enclosure_status_t 
fbe_eses_enclosure_get_edal_fw_info(fbe_eses_enclosure_t *eses_enclosure_p, 
                                    fbe_enclosure_fw_target_t fw_target,
                                    fbe_u32_t side,
                                    fbe_edal_fw_info_t * edal_fw_info_p);

/* fbe_eses_enclosure_debug.c */
fbe_status_t fbe_eses_enclosure_xlate_eses_comp_type(ses_comp_type_enum component, 
                                                     fbe_enclosure_fw_target_t * fw_target);

void fbe_eses_printSendControlPage(fbe_eses_enclosure_t *eses_enclosure, 
                                   fbe_u8_t *controlPagePtr);
void printElemGroupInfo(fbe_eses_enclosure_t *eses_enclosure);

fbe_status_t fbe_eses_enclosure_convert_subencl_product_id_to_unique_id(fbe_eses_enclosure_t * eses_enclosure,
                                                           fbe_char_t * pPsSubenclProductId,
                                                           HW_MODULE_TYPE * pUniqueId);

fbe_bool_t
fbe_eses_enclosure_is_ps_downloadable(fbe_eses_enclosure_t *eses_enclosure,   
                                      fbe_u32_t psSlot,
                                      fbe_char_t * pPsFirmwareRev,
                                      HW_MODULE_TYPE psUniqueId);

AC_DC_INPUT
fbe_eses_enclosure_get_ps_ac_dc_type(fbe_eses_enclosure_t *eses_enclosure,   
                                     HW_MODULE_TYPE psUniqueId);

/* fbe_eses_enclosure_utils.c */
fbe_status_t fbe_eses_enclosure_get_fw_target_addr(fbe_eses_enclosure_t * eses_enclosure_p,
                                                        fbe_enclosure_fw_target_t target, 
                                                        fbe_u8_t side,
                                                        fbe_eses_encl_fw_component_info_t **fw_info_p);

fbe_cmd_retry_code_t fbe_eses_enclosure_is_cmd_retry_needed(fbe_eses_enclosure_t * eses_enclosure, fbe_eses_ctrl_opcode_t opcode,
                                                  fbe_enclosure_status_t encl_status);

fbe_bool_t fbe_eses_enclosure_is_time_sync_needed(fbe_eses_enclosure_t * eses_enclosure);

fbe_enclosure_status_t 
fbe_eses_enclosure_first_status_read_completed(fbe_eses_enclosure_t * eses_enclosure,
                                               fbe_bool_t * first_status_read_completed_p);

fbe_enclosure_status_t 
fbe_eses_enclosure_slot_update_device_off_reason(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_num);

fbe_enclosure_status_t 
fbe_eses_enclosure_phy_update_disable_reason(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t phy_index);
fbe_enclosure_status_t 
fbe_eses_enclosure_slot_update_user_req_power_ctrl(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t slot_num);

fbe_enclosure_status_t 
fbe_eses_enclosure_slot_status_update_power_cycle_state(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_num,
                                                  fbe_bool_t new_slot_powered_off);

fbe_enclosure_status_t 
fbe_eses_enclosure_slot_statistics_update_power_cycle_state(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t slot_index,
                                                  fbe_u8_t new_power_down_count);
fbe_enclosure_status_t 
fbe_eses_enclosure_ride_through_handle_slot_power_cycle(fbe_eses_enclosure_t * eses_enclosure);

fbe_enclosure_status_t fbe_eses_get_chunk_and_add_to_list(fbe_eses_enclosure_t * eses_enclosure,
                                                          fbe_sg_element_t *sgp, 
                                                          fbe_u32_t request_size);

fbe_u32_t fbe_eses_enclosure_sg_list_bytes_left(fbe_u32_t number_of_items, fbe_u32_t bytes_used);

void fbe_eses_enclosure_copy_data_to_sg(fbe_sg_element_t *sg_p, fbe_u8_t *data_p);

fbe_enclosure_element_type_t fbe_get_fbe_element_type(fbe_eses_enclosure_t *eses_enclosure, 
                                                      fbe_u8_t element_offset);
fbe_enclosure_status_t fbe_eses_slot_index_to_element_index(fbe_eses_enclosure_t *enclosure, 
                                                      fbe_u8_t slot, 
                                                      fbe_u8_t element_type,
                                                      fbe_u8_t *element_index);
fbe_enclosure_status_t 
fbe_eses_enclosure_setup_connector_control(fbe_eses_enclosure_t * eses_enclosure,
                                     fbe_u8_t conn_id,
                                     fbe_bool_t disable);
fbe_enclosure_status_t 
fbe_eses_enclosure_verify_cabling(fbe_eses_enclosure_t * eses_enclosure,
                                  fbe_u8_t *illegal_cable_exist);
fbe_enclosure_status_t fbe_eses_enclosure_get_expansion_port_connector_id(fbe_eses_enclosure_t * eses_enclosure,
                                                   fbe_u8_t * connector_id_p,
                                                   fbe_u8_t max_ports);

fbe_status_t fbe_eses_enclosure_eses_comp_type_to_fw_target(ses_comp_type_enum eses_comp_type,
                                                            fbe_enclosure_fw_target_t * pFwTarget);

fbe_status_t fbe_eses_enclosure_get_buf_id(fbe_eses_enclosure_t *eses_enclosure, 
                                           fbe_u64_t deviceType, 
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u8_t *buf_id, 
                                           fbe_enclosure_resume_prom_req_type_t request_type);

fbe_status_t fbe_eses_enclosure_get_buf_size(fbe_eses_enclosure_t *eses_enclosure,
                                           fbe_u64_t deviceType,
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u32_t *pBufSize,
                                           fbe_enclosure_resume_prom_req_type_t request_type);

fbe_status_t fbe_eses_enclosure_get_buf_id_with_unavail_buf_size(fbe_eses_enclosure_t * eses_enclosure, 
                                        fbe_eses_enclosure_buf_info_t * pBufInfo);

fbe_status_t fbe_eses_enclosure_is_eeprom_resume_valid(fbe_eses_enclosure_t * eses_enclosure,
                                                dev_eeprom_rev0_info_struct * pEepromRev0,
                                                fbe_bool_t * pResumeValid);

fbe_status_t fbe_eses_enclosure_reformat_resume(fbe_eses_enclosure_t * eses_enclosure,
                                      RESUME_PROM_STRUCTURE * pEmcStandardRp,
                                      dev_eeprom_rev0_info_struct * pEepromRev0);

fbe_u32_t fbe_eses_enclosure_convert_string_to_hex(char * string);

/* fbe_eses_enclosure_mapping.c */
fbe_enclosure_status_t fbe_eses_enclosure_init_config_info(fbe_eses_enclosure_t * eses_enclosure);

fbe_enclosure_status_t 
fbe_eses_enclosure_chassis_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                         fbe_enclosure_component_types_t component_type,
                                         fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_lcc_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                        fbe_enclosure_component_types_t component_type,
                                        fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_array_dev_slot_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                          fbe_enclosure_component_types_t component_type,
                                          fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_exp_phy_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_connector_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                              fbe_enclosure_component_types_t component_type,
                                              fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_expander_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_enclosure_component_types_t component_type,
                                             fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_sps_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                        fbe_enclosure_component_types_t component_type,
                                        fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_ps_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_enclosure_component_types_t component_type,
                                       fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_cooling_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_temp_sensor_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                                fbe_enclosure_component_types_t component_type,
                                                fbe_u8_t component_index);

fbe_enclosure_status_t 
fbe_eses_enclosure_display_init_config_info(fbe_eses_enclosure_t * eses_enclosure,
                                            fbe_enclosure_component_types_t component_type,
                                            fbe_u8_t component_index);

fbe_u8_t fbe_eses_enclosure_get_next_available_phy_index(fbe_eses_enclosure_t * eses_enclosure);
fbe_u8_t fbe_eses_enclosure_get_next_available_lcc_index(fbe_eses_enclosure_t * eses_enclosure);

fbe_enclosure_status_t fbe_eses_enclosure_elem_index_to_subencl_id(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_u8_t elem_index,
                                                                fbe_u8_t *subencl_id_p);

fbe_enclosure_status_t fbe_eses_enclosure_get_local_lcc_side_id(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_u8_t *local_lcc_side_id_p);

fbe_enclosure_status_t fbe_eses_enclosure_subencl_id_to_side_id_component_type(fbe_eses_enclosure_t * eses_enclosure,
                                                                fbe_u8_t subencl_id,
                                                                fbe_u8_t *side_id_p,
                                                                fbe_enclosure_component_types_t *subencl_component_type_p);

fbe_enclosure_status_t fbe_eses_enclosure_get_comp_index(fbe_eses_enclosure_t *eses_enclosure,
                                                                  fbe_u8_t group_id,
                                                                  fbe_u8_t elem,
                                                                  fbe_u8_t *comp_index_p);
fbe_enclosure_status_t 
fbe_eses_enclosure_map_cooling_elem_to_component_index(fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_enclosure_component_types_t subencl_component_type,
                                                       fbe_u8_t elem,
                                                       fbe_u8_t side_id,
                                                       fbe_u8_t *comp_index_p,
                                                       fbe_u8_t *subtype);
fbe_enclosure_status_t 
fbe_eses_enclosure_map_fw_target_type_to_component_index(fbe_eses_enclosure_t * eses_enclosure,
                                                    fbe_enclosure_component_types_t fw_comp_type, 
                                                    fbe_u8_t slot_id, 
                                                    fbe_u8_t rev_id,
                                                    fbe_u8_t *component_index);

fbe_enclosure_status_t fbe_eses_enclosure_get_nth_group_among_same_type(
                                                       fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_u8_t group_id,
                                                       fbe_u8_t * nth_group_p);

fbe_enclosure_status_t fbe_eses_enclosure_elem_index_to_component_index(fbe_eses_enclosure_t * eses_enclosure,
                                                     fbe_enclosure_component_types_t component_type,
                                                     fbe_u8_t elem_index,
                                                     fbe_u8_t *component_index_p);

fbe_enclosure_status_t fbe_eses_enclosure_get_comp_type(fbe_eses_enclosure_t * eses_enclosure,
                                                             fbe_u8_t group_id,
                                                             fbe_enclosure_component_types_t * component_type_p);
                                                             
fbe_enclosure_status_t fbe_eses_enclosure_phy_index_to_drive_slot_index(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_u8_t phy_index,
                                              fbe_u8_t * drive_slot_num_p);

fbe_enclosure_status_t fbe_eses_enclosure_phy_index_to_connector_index(fbe_eses_enclosure_t * eses_enclosure, 
                                              fbe_u8_t phy_index,
                                              fbe_u8_t * connector_index_p);

fbe_enclosure_status_t fbe_eses_enclosure_exp_elem_index_phy_id_to_phy_index(fbe_eses_enclosure_t  * eses_enclosure, 
                fbe_u8_t expander_elem_index, fbe_u8_t phy_id, fbe_u8_t * phy_index_p);

fbe_enclosure_status_t fbe_eses_enclosure_expander_sas_address_to_elem_index(fbe_eses_enclosure_t * eses_enclosure,
                                                     fbe_sas_address_t expander_sas_address,
                                                     fbe_u8_t *elem_index_p);

fbe_enclosure_status_t fbe_eses_enclosure_component_type_to_element_type(fbe_eses_enclosure_t * eses_enclosure,
                                                fbe_enclosure_component_types_t component_type,
                                                ses_elem_type_enum * elem_type_p);

fbe_enclosure_status_t fbe_eses_enclosure_elem_type_to_component_type(fbe_eses_enclosure_t * eses_enclosure,
                                                ses_elem_type_enum elem_type,
                                                fbe_enclosure_component_types_t subencl_component_type,
                                                fbe_enclosure_component_types_t * component_type_p);

fbe_enclosure_status_t 
fbe_eses_enclosure_get_expansion_port_entire_connector_index(fbe_eses_enclosure_t * eses_enclosure,
                                                      fbe_u8_t connector_id,
                                                      fbe_u8_t * expansion_connector_index_p);
fbe_enclosure_status_t 
fbe_eses_enclosure_get_connector_entire_connector_index(fbe_eses_enclosure_t * eses_enclosure,
                                                   fbe_u8_t connector_id,
                                                   fbe_u8_t * expansion_port_entire_connector_index_p);

fbe_enclosure_status_t fbe_eses_enclosure_get_local_sas_exp_elem_index(fbe_eses_enclosure_t * eses_enclosure, 
                                                       fbe_u8_t *elem_index_p);

fbe_u16_t fbe_eses_elem_index_to_byte_offset(fbe_eses_enclosure_t *eses_enclosure,
                                             fbe_u8_t elem_index);

fbe_status_t fbe_eses_enclosure_prom_id_to_buf_id(fbe_eses_enclosure_t *eses_enclosure, 
                                                  fbe_u8_t prom_id, fbe_u8_t *buf_id, 
                                                  fbe_enclosure_resume_prom_req_type_t request_type);

void fbe_eses_enclosure_dl_release_buffers(fbe_eses_enclosure_t * eses_enclosure,
                                           fbe_u8_t *memory_start_address, 
                                           fbe_sg_element_t *dl_item_p);

fbe_enclosure_status_t fbe_eses_parse_statistics_page(fbe_eses_enclosure_t *enclosure,
                                                fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                                ses_pg_emc_statistics_struct *sourcep);
fbe_enclosure_status_t fbe_eses_parse_threshold_page(fbe_eses_enclosure_t *eses_enclosure,
                        fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,   
                        fbe_u8_t * sourcep);
fbe_enclosure_status_t fbe_eses_enclosure_elem_index_to_elem_offset(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t elem_index,
                                             fbe_u8_t * elem_offset_p);

fbe_enclosure_status_t fbe_eses_enclosure_elem_offset_to_elem_index(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t elem_offset,
                                             fbe_u8_t * elem_index_p);

fbe_enclosure_status_t 
fbe_eses_enclosure_elem_offset_to_comp_type_comp_index(fbe_eses_enclosure_t * eses_enclosure,
                                             fbe_u8_t elem_offset,
                                             fbe_enclosure_component_types_t * comp_type_p,
                                             fbe_u8_t * comp_index_p);

fbe_enclosure_status_t fbe_eses_enclosure_elem_index_to_component_type(fbe_eses_enclosure_t * eses_enclosure,
                                                     fbe_u8_t elem_index,
                                                     fbe_enclosure_component_types_t * component_type_p);

fbe_status_t fbe_eses_enclosure_lcc_prom_id_to_index(fbe_eses_enclosure_t *eses_enclosure,
                                                     fbe_u8_t prom_id,
                                                     fbe_u32_t *index);

static fbe_status_t 
fbe_eses_enclosure_get_battery_backed_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);

fbe_status_t fbe_eses_enclosure_format_fw_rev(fbe_eses_enclosure_t *eses_enclosure,
                                    fbe_eses_enclosure_format_fw_rev_t * pFwRevAfterFormatting,
                                    fbe_u8_t * pFwRev,
                                    fbe_u8_t fwRevSize);
fbe_enclosure_status_t
fbe_eses_enclosure_get_connector_type(fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_u8_t connectorId,
                                      fbe_bool_t isLocalConnector,
                                      fbe_u8_t * pConnectorType);

fbe_bool_t fbe_eses_enclosure_at_lower_rev(fbe_eses_enclosure_t *eses_enclosure,
                                fbe_u8_t * pDestFwRev,
                                fbe_u8_t * pSrcFwRev,
                                fbe_u32_t fwRevSize);

/**************************************************************
 **************     Inline Utility Functions     **************
 **************************************************************/


static __forceinline 
fbe_eses_elem_group_t * fbe_eses_enclosure_get_elem_group_ptr(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->elem_group);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_elem_group(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_eses_elem_group_t * elem_group)
{
    eses_enclosure->elem_group = elem_group;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_eses_encl_fw_info_t * fbe_eses_enclosure_get_enclFwInfo_ptr(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->enclFwInfo);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_enclFwInfo_ptr(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_eses_encl_fw_info_t * enclFwInfo)
{
    eses_enclosure->enclFwInfo = enclFwInfo;
    return FBE_STATUS_OK;
}


// Write Buffer functions
static __forceinline fbe_bool_t fbe_eses_no_write_outstanding(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->outstanding_write==0);
}

static __forceinline fbe_bool_t fbe_eses_no_emc_encl_ctrl_write_outstanding(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->emc_encl_ctrl_outstanding_write==0);
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_comp_types(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_comp_types);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_comp_types(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_comp_types)
{
    eses_enclosure->properties.number_of_comp_types = number_of_comp_types;
    return FBE_STATUS_OK;
}


static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_power_supplies(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_power_supplies);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_power_supplies(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_power_supplies)
{
    eses_enclosure->properties.number_of_power_supplies = number_of_power_supplies;
    return FBE_STATUS_OK;
}


static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_power_supply_subelem(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_power_supply_subelem);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_power_supply_subelem(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_power_supply_subelem)
{
    eses_enclosure->properties.number_of_power_supply_subelem = number_of_power_supply_subelem;
    return FBE_STATUS_OK;
}


static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_cooling_components(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_cooling_components);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_cooling_components(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_cooling_components)
{
    eses_enclosure->properties.number_of_cooling_components = number_of_cooling_components;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_cooling_components_per_ps(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_cooling_components_per_ps);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_cooling_components_per_ps(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_cooling_components_per_ps)
{
    eses_enclosure->properties.number_of_cooling_components_per_ps = number_of_cooling_components_per_ps;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_cooling_components_on_chassis(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_cooling_components_on_chassis);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_cooling_components_on_chassis(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_cooling_components_on_chassis)
{
    eses_enclosure->properties.number_of_cooling_components_on_chassis = number_of_cooling_components_on_chassis;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_cooling_components_external(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_cooling_components_external);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_cooling_components_external(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_cooling_components_external)
{
    eses_enclosure->properties.number_of_cooling_components_external = number_of_cooling_components_external;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_cooling_components_on_lcc(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_cooling_components_on_lcc);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_cooling_components_on_lcc(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_cooling_components_on_lcc)
{
    eses_enclosure->properties.number_of_cooling_components_on_lcc = number_of_cooling_components_on_lcc;
    return FBE_STATUS_OK;
}


static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_slots(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_slots);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_slots(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_slots)
{
    eses_enclosure->properties.number_of_slots = number_of_slots;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_eses_enclosure_set_max_encl_slots(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t max_encl_slots)
{
    eses_enclosure->properties.max_encl_slot = max_encl_slots;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_u8_t fbe_eses_enclosure_get_max_encl_slots(fbe_eses_enclosure_t * eses_enclosure)
{    
    return (eses_enclosure->properties.max_encl_slot);
}
static __forceinline 
fbe_status_t fbe_eses_enclosure_set_number_of_drive_midplane(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_drive_midplane)
{
    eses_enclosure->properties.number_of_drive_midplane = number_of_drive_midplane;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_u8_t fbe_eses_enclosure_get_number_of_drive_midplane(fbe_eses_enclosure_t * eses_enclosure)
{    
    return (eses_enclosure->properties.number_of_drive_midplane);
}
static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_temp_sensors(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_temp_sensors);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_temp_sensors(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_temp_sensors)
{
    eses_enclosure->properties.number_of_temp_sensors = number_of_temp_sensors;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_temp_sensors_per_lcc(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_temp_sensors_per_lcc);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_temp_sensors_per_lcc(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_temp_sensors_per_lcc)
{
    eses_enclosure->properties.number_of_temp_sensors_per_lcc = number_of_temp_sensors_per_lcc;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_temp_sensors_on_chassis(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_temp_sensors_on_chassis);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_temp_sensors_on_chassis(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_temp_sensors_on_chassis)
{
    eses_enclosure->properties.number_of_temp_sensors_on_chassis = number_of_temp_sensors_on_chassis;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_expanders(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_expanders);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_expanders(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_expanders)
{
    eses_enclosure->properties.number_of_expanders = number_of_expanders;
    return FBE_STATUS_OK;
}
static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_expanders_per_lcc(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_expanders_per_lcc);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_expanders_per_lcc(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_expanders_per_lcc)
{
    eses_enclosure->properties.number_of_expanders_per_lcc = number_of_expanders_per_lcc;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_expander_phys(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_expander_phys);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_expander_phys(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_expander_phys)
{
    eses_enclosure->properties.number_of_expander_phys = number_of_expander_phys;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_related_expanders(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_related_expanders);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_related_expanders(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_related_expanders)
{
    eses_enclosure->properties.number_of_related_expanders = number_of_related_expanders;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_encl(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_encl);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_encl(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_encl)
{
    eses_enclosure->properties.number_of_encl = number_of_encl;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_connectors(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_connectors);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_connectors(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_connectors)
{
    eses_enclosure->properties.number_of_connectors = number_of_connectors;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_connectors_per_lcc(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_connectors_per_lcc);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_connectors_per_lcc(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_connectors_per_lcc)
{
    eses_enclosure->properties.number_of_connectors_per_lcc = number_of_connectors_per_lcc;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_lanes_per_entire_connector(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_lanes_per_entire_connector);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_lanes_per_entire_connector(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_lanes_per_entire_connector)
{
    eses_enclosure->properties.number_of_lanes_per_entire_connector = number_of_lanes_per_entire_connector;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_first_slot_index(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.first_slot_index);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_first_slot_index(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t first_slot_index)
{
    eses_enclosure->properties.first_slot_index = first_slot_index;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_expansion_ports(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_expansion_ports);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_expansion_ports(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_expansion_ports)
{
    eses_enclosure->properties.number_of_expansion_ports = number_of_expansion_ports;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_first_expansion_port_index(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.first_expansion_port_index);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_first_expansion_port_index(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t first_expansion_port_index)
{
    eses_enclosure->properties.first_expansion_port_index = first_expansion_port_index;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_lccs(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_lccs);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_lccs(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_lccs)
{
    eses_enclosure->properties.number_of_lccs = number_of_lccs;
    return FBE_STATUS_OK;
}
static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_lccs_with_temp_sensor(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_lccs_with_temp_sensor);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_lccs_with_temp_sensor(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_lccs_with_temp_sensor)
{
    eses_enclosure->properties.number_of_lccs_with_temp_sensor = number_of_lccs_with_temp_sensor;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_display_chars(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_display_chars);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_display_chars(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_display_chars)
{
    eses_enclosure->properties.number_of_display_chars = number_of_display_chars;
    return FBE_STATUS_OK;
}
static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_sps(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_sps);
}
static __forceinline 
fbe_status_t fbe_eses_enclosure_set_number_of_sps(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u8_t number_of_sps)
{
    eses_enclosure->properties.number_of_sps = number_of_sps;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_u8_t fbe_eses_enclosure_get_number_of_ssc(fbe_eses_enclosure_t * eses_enclosure)
{
    return(eses_enclosure->properties.number_of_ssc);
}

static __forceinline
fbe_status_t fbe_eses_enclosure_set_number_of_ssc(fbe_eses_enclosure_t * eses_enclosure, 
                                                  fbe_u8_t number_of_ssc)
{
    eses_enclosure->properties.number_of_ssc = number_of_ssc;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_two_digit_displays(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_two_digit_displays);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_two_digit_displays(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_two_digit_displays)
{
    eses_enclosure->properties.number_of_two_digit_displays = number_of_two_digit_displays;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_one_digit_displays(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_one_digit_displays);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_one_digit_displays(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_one_digit_displays)
{
    eses_enclosure->properties.number_of_one_digit_displays = number_of_one_digit_displays;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_possible_elem_groups(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_possible_elem_groups);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_possible_elem_groups(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_possible_elem_groups)
{
    eses_enclosure->properties.number_of_possible_elem_groups = number_of_possible_elem_groups;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_actual_elem_groups(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_actual_elem_groups);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_actual_elem_groups(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_actual_elem_groups)
{
    eses_enclosure->properties.number_of_actual_elem_groups = number_of_actual_elem_groups;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_enclosure_fw_dl_retry_max(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.enclosure_fw_dl_retry_max);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_enclosure_fw_dl_retry_max(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t enclosure_fw_dl_retry_max)
{
    eses_enclosure->properties.enclosure_fw_dl_retry_max = enclosure_fw_dl_retry_max;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_fw_num_expander_elements(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.fw_num_expander_elements);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_fw_num_expander_elements(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t fw_num_expander_elements)
{
    eses_enclosure->properties.fw_num_expander_elements = fw_num_expander_elements;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_fw_num_subencl_sides(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.fw_num_subencl_sides);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_fw_num_subencl_sides(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t fw_num_subencl_sides)
{
    eses_enclosure->properties.fw_num_subencl_sides = fw_num_subencl_sides;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_fw_lcc_version_descs(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.fw_lcc_version_descs);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_fw_lcc_version_descs(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t fw_lcc_version_descs)
{
    eses_enclosure->properties.fw_lcc_version_descs = fw_lcc_version_descs;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_fw_ps_version_descs(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.fw_ps_version_descs);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_fw_ps_version_descs(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t fw_ps_version_descs)
{
    eses_enclosure->properties.fw_ps_version_descs = fw_ps_version_descs;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_default_ride_through_period(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.default_ride_through_period);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_default_ride_through_period(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t default_ride_through_period)
{
    eses_enclosure->properties.default_ride_through_period = default_ride_through_period;
    return FBE_STATUS_OK;
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_drive_slot_to_phy_index_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t side_id,
                                       fbe_u8_t  *mapping)
{
    if (side_id >= ESES_SUPPORTED_SIDES)
    {
        return FBE_STATUS_FAILED;
    }
    eses_enclosure->properties.drive_slot_to_phy_index[side_id] = mapping;
    return FBE_STATUS_OK;
}
static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_drive_slot_to_phy_index_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t side_id,
                                       fbe_u8_t slot)
{
    if ((side_id >= ESES_SUPPORTED_SIDES)||(slot >= eses_enclosure->properties.number_of_slots))
    {
        return (0xff);
    }

    return (eses_enclosure->properties.drive_slot_to_phy_index[side_id][slot]);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_connector_index_to_phy_index_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t  *mapping)
{
    eses_enclosure->properties.connector_index_to_phy_index = mapping;
    return FBE_STATUS_OK;
}
static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_connector_index_to_phy_index_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t index)
{
    if (index>eses_enclosure->properties.number_of_connectors_per_lcc)
    {
        return (0xff);
    }

    return (eses_enclosure->properties.connector_index_to_phy_index[index]);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_phy_index_to_phy_id_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t  *mapping)
{
    eses_enclosure->properties.phy_index_to_phy_id = mapping;
    return FBE_STATUS_OK;
}
static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_phy_index_to_phy_id_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t index)
{
    if (index>eses_enclosure->properties.number_of_expander_phys)
    {
        return (0xff);
    }

    return (eses_enclosure->properties.phy_index_to_phy_id[index]);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_init_enclFupInfo(fbe_eses_enclosure_t * eses_enclosure)
{
    eses_enclosure->enclCurrentFupInfo.enclFupImagePointer = NULL;
    eses_enclosure->enclCurrentFupInfo.enclFupImageSize = 0;
    eses_enclosure->enclCurrentFupInfo.enclFupComponent = FBE_FW_TARGET_INVALID;
    eses_enclosure->enclCurrentFupInfo.enclFupBytesTransferred = 0;
    eses_enclosure->enclCurrentFupInfo.enclFupOperationRetryCnt = 0;
    eses_enclosure->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FIRMWARE_OP_NONE;
    eses_enclosure->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
    eses_enclosure->enclCurrentFupInfo.enclFupUseTunnelling = FALSE;
    eses_enclosure->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
    eses_enclosure->enclCurrentFupInfo.enclFupCurrTransferSize = 0;
    return FBE_STATUS_OK;
}

static   
fbe_status_t fbe_eses_enclosure_init_expansion_port_info(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u32_t i;

    for (i = 0; i < fbe_eses_enclosure_get_number_of_expansion_ports(eses_enclosure) ; i++){
        eses_enclosure->expansion_port_info[i].sas_address = FBE_SAS_ADDRESS_INVALID;
        eses_enclosure->expansion_port_info[i].generation_code = 0;
        eses_enclosure->expansion_port_info[i].element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_INVALID;
        eses_enclosure->expansion_port_info[i].chain_depth = 0;
        eses_enclosure->expansion_port_info[i].attached_encl_fw_activating = FALSE;
    } 
    return FBE_STATUS_OK;
}

static   
fbe_status_t fbe_eses_enclosure_init_drive_info(fbe_eses_enclosure_t * eses_enclosure)
{
    fbe_u32_t i;

    for(i = 0 ; i < fbe_eses_enclosure_get_number_of_slots(eses_enclosure); i ++)
    {
        eses_enclosure->drive_info[i].sas_address = FBE_SAS_ADDRESS_INVALID;
        eses_enclosure->drive_info[i].generation_code = 0;
        eses_enclosure->drive_info[i].element_type = FBE_PAYLOAD_SMP_ELEMENT_TYPE_INVALID;
        eses_enclosure->drive_info[i].power_cycle_duration = FBE_ESES_DRIVE_POWER_CYCLE_DURATION_DEFAULT;
    }
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_eses_enclosure_drive_info_t * fbe_eses_enclosure_get_drive_info_ptr(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->drive_info);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_drive_info_ptr(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_eses_enclosure_drive_info_t * drive_info)
{
    eses_enclosure->drive_info = drive_info;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_eses_enclosure_expansion_port_info_t * fbe_eses_enclosure_get_expansion_port_info_ptr(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->expansion_port_info);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_expansion_port_info_ptr(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_eses_enclosure_expansion_port_info_t * expansion_port_info)
{
    eses_enclosure->expansion_port_info = expansion_port_info;
    return FBE_STATUS_OK;
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_connector_disable_list_ptr(fbe_eses_enclosure_t * eses_enclosure,
                                                               fbe_bool_t * connector_disable_list_ptr)
{
    eses_enclosure->properties.connector_disable_list_ptr = connector_disable_list_ptr;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_bool_t * fbe_eses_enclosure_get_connector_disable_list_ptr(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.connector_disable_list_ptr);
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_drive_power_cycle_duration(fbe_eses_enclosure_t * eses_enclosure,
                                                             fbe_u8_t slot_num)
{
    return (eses_enclosure->drive_info[slot_num].power_cycle_duration);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_drive_power_cycle_duration(fbe_eses_enclosure_t * eses_enclosure,
                                                               fbe_u8_t slot_num,
                                                               fbe_u8_t drive_power_cycle_duration)
{
    eses_enclosure->drive_info[slot_num].power_cycle_duration = drive_power_cycle_duration;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_bool_t fbe_eses_enclosure_get_drive_power_cycle_request(fbe_eses_enclosure_t * eses_enclosure,
                                                             fbe_u8_t slot_num)
{
    return (eses_enclosure->drive_info[slot_num].drive_need_power_cycle);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_drive_power_cycle_request(fbe_eses_enclosure_t * eses_enclosure,
                                                               fbe_u8_t slot_num,
                                                               fbe_bool_t  drive_need_power_cycle)
{
    eses_enclosure->drive_info[slot_num].drive_need_power_cycle = drive_need_power_cycle;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_bool_t fbe_eses_enclosure_get_drive_battery_backed_request(fbe_eses_enclosure_t * eses_enclosure,
                                                             fbe_u8_t slot_num)
{
    return (eses_enclosure->drive_info[slot_num].drive_battery_backed);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_drive_battery_backed_request(fbe_eses_enclosure_t * eses_enclosure,
                                                               fbe_u8_t slot_num,
                                                               fbe_bool_t  drive_battery_backed)
{
    eses_enclosure->drive_info[slot_num].drive_battery_backed = drive_battery_backed;
    return FBE_STATUS_OK;
}

//static __forceinline
//fbe_u32_t fbe_eses_enclosure_get_unique_id(fbe_eses_enclosure_t * eses_enclosure)
//{
//    return (eses_enclosure->unique_id);
//}

static __forceinline 
fbe_status_t fbe_eses_enclosure_get_serial_number(fbe_eses_enclosure_t * eses_enclosure,
                                                  fbe_u32_t length,
                                                  fbe_u8_t *newString)
{
    if (length > FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE) 
    {
        length = FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE;
    }
    fbe_copy_memory(newString, 
                    eses_enclosure->encl_serial_number, 
                    length);
   return (FBE_STATUS_OK);
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_command_retry_max(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_command_retry_max);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_command_retry_max(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_command_retry_max)
{
    eses_enclosure->properties.number_of_command_retry_max = number_of_command_retry_max;
    return FBE_STATUS_OK;
}

/* The below listed 2 functions fbe_scsi_get_vpd_inquiry_pg_size and 
 * fbe_scsi_get_standard_inquiry_pg_size are SCSI specific, for time being 
 * we are placing the declaration in eses_xx_private.h 
 */
static __inline fbe_u8_t fbe_scsi_get_vpd_inquiry_pg_size(fbe_u8_t *d) 
{
    //Add 3 because pg_len is the length of the rest after the page length bytes.
    //There are 3 bytes before it.
    return (fbe_u8_t)(*(d + 3) + 4);
}

static __inline fbe_u8_t fbe_scsi_get_standard_inquiry_pg_size(fbe_u8_t *d) 
{
    //Add 4 because pg_len is the length of the rest after the page length bytes.
    //There are 4 bytes before it.
    return (fbe_u8_t)(*(d + 4) + 5);
}

static __forceinline 
fbe_bool_t fbe_eses_enclosure_is_ps_resume_format_special(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.isPsResumeFormatSpecial);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_ps_resume_format_special(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_bool_t isPsResumeFormatSpecial)
{
    eses_enclosure->properties.isPsResumeFormatSpecial = isPsResumeFormatSpecial;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_bool_t fbe_eses_enclosure_is_ps_overall_elem_saved(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.isPsOverallElemSaved);
}

static __forceinline  
fbe_status_t fbe_eses_enclosure_set_ps_overall_elem_saved(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_bool_t isPsOverallElemSaved)
{
    eses_enclosure->properties.isPsOverallElemSaved = isPsOverallElemSaved;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t fbe_eses_enclosure_get_number_of_power_supplies_per_side(fbe_eses_enclosure_t * eses_enclosure)
{
    return (eses_enclosure->properties.number_of_power_supplies_per_side);
}
static __forceinline  
fbe_status_t fbe_eses_enclosure_set_number_of_power_supplies_per_side(fbe_eses_enclosure_t * eses_enclosure,
                                       fbe_u8_t number_of_power_supplies_per_side)
{
    eses_enclosure->properties.number_of_power_supplies_per_side = number_of_power_supplies_per_side;
    return FBE_STATUS_OK;
}

#endif /*ESES_ENCLOSURE_PRIVATE_H */
