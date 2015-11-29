#ifndef FBE_BOARD_H
#define FBE_BOARD_H

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_eir_info.h"
#include "fbe_pe_types.h"
#include "specl_types.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_sps_interface.h"

#define FBE_PE_NUMBER_OF_BLOCKS 50

#define FBE_BOARD_PE_INFO_SIZE (FBE_MEMORY_CHUNK_SIZE * FBE_PE_NUMBER_OF_BLOCKS)
#define FBE_DUMMY_PORT_SERIAL_NUM "EMCNUM123456"

typedef enum fbe_base_board_control_code_e {
    FBE_BASE_BOARD_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_BOARD),

    FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_NUMBER,
    FBE_BASE_BOARD_CONTROL_CODE_GET_DIPLEX_PORT_NUMBER,    
    FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_INFO,
    // SPS related Control Codes
    FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_STATUS,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_MANUF_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_SPS_COMMAND,  
    FBE_BASE_BOARD_CONTROL_CODE_GET_RESUME,  // Get the entire resume
    FBE_BASE_BOARD_CONTROL_CODE_RESUME_READ, 
    FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE,
    FBE_BASE_BOARD_CONTROL_CODE_RESUME_WRITE_ASYNC, 
    
    FBE_BASE_BOARD_CONTROL_CODE_GET_PE_INFO,
    
    // PS related Control Codes
    FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_INFO,

    FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_EXP_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MISC_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_CACHE_CARD_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_DIMM_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SSD_INFO,

    /* Overall Count */
    FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_PLATFORM_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MISC_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_LCC_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_CACHE_CARD_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_DIMM_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SSD_COUNT,

    /* Per Blade Count */
    FBE_BASE_BOARD_CONTROL_CODE_GET_POWER_SUPPLY_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_COOLING_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SUITCASE_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MGMT_MODULE_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_BMC_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_FLT_REG_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SLAVE_PORT_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_COUNT_PER_BLADE,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_COUNT_PER_BLADE,
    
    /* Set Control Codes */
    FBE_BASE_BOARD_CONTROL_CODE_SET_PS_INFO,

    FBE_BASE_BOARD_CONTROL_CODE_SET_SP_FAULT_LED,
    FBE_BASE_BOARD_CONTROL_CODE_SET_ENCL_FAULT_LED,
    FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_FAULT_LED,
    FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PORT_LED,
    FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_FAULT_LED,
    FBE_BASE_BOARD_CONTROL_CODE_SET_UNSAFE_TO_REMOVE_LED,
    FBE_BASE_BOARD_CONTROL_CODE_SET_RESUME,       // Set the entire resume.       
    FBE_BASE_BOARD_CONTROL_CODE_SET_POWER_SUPPLY_MODE,
    FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_PORT,
    FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_VLAN_CONFIG_MODE,
    FBE_BASE_BOARD_CONTROL_CODE_SET_CLEAR_HOLD_IN_POST_AND_OR_RESET,
    FBE_BASE_BOARD_CONTROL_CODE_GET_IO_MODULE_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_BEM_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_GET_MEZZANINE_INFO,

    FBE_BASE_BOARD_CONTROL_CODE_SET_COMPONENT_MAX_TIMEOUT,
    FBE_BASE_BOARD_CONTROL_CODE_CLEAR_FLT_REG_FAULT,
    
    // Get fbe_base_board_t info
    FBE_BASE_BOARD_CONTROL_CODE_GET_FBE_BASE_BOARD_INFO,
    FBE_BASE_BOARD_CONTROL_CODE_SET_FLUSH_FILES_AND_REG,

    FBE_BASE_BOARD_CONTROL_CODE_GET_EIR_INFO,

    FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_SOFTWARE_BOOT_STATUS,
    FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_FLT_EXP_STATUS,
    FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_COUNT,

    FBE_BASE_BOARD_CONTROL_CODE_GET_BATTERY_STATUS,
    FBE_BASE_BOARD_CONTROL_CODE_GET_BATTERY_COUNT,
    FBE_BASE_BOARD_CONTROL_CODE_BATTERY_COMMAND,

    FBE_BASE_BOARD_CONTROL_CODE_SET_ASYNC_IO,
    FBE_BASE_BOARD_CONTROL_CODE_SET_SYNC_IO,

    FBE_BASE_BOARD_CONTROL_CODE_ENABLE_DMRB_ZEROING,
    FBE_BASE_BOARD_CONTROL_CODE_DISABLE_DMRB_ZEROING,

    FBE_BASE_BOARD_CONTROL_CODE_FIRMWARE_OP,
    FBE_BASE_BOARD_CONTROL_CODE_PARSE_IMAGE_HEADER,
    FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PERSISTED_POWER_STATE,
    FBE_BASE_BOARD_CONTROL_CODE_SIM_SWITCH_PSL,
    FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_SERIAL_NUM,
    FBE_BASE_BOARD_CONTROL_CODE_SET_CNA_MODE,
    FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_VEEPROM_CPU_STATUS,
    FBE_BASE_BOARD_CONTROL_CODE_GET_HARDWARE_SSV_DATA,
    FBE_BASE_BOARD_CONTROL_CODE_LAST
}fbe_base_board_control_code_t;

typedef enum fbe_base_board_led_e {
    FBE_BASE_BOARD_LED_INVALID,
    FBE_BASE_BOARD_LED_UNSAFE_TO_REMOVE,
    FBE_BASE_BOARD_LED_ENCLOSURE_FAULT,
    FBE_BASE_BOARD_LED_LAST
}fbe_base_board_led_t;

typedef enum fbe_port_driver_e {
    FBE_PORT_DRIVER_INVALID,
    FBE_PORT_DRIVER_SAS_LSI,
    FBE_PORT_DRIVER_SAS_CPD,
    FBE_PORT_DRIVER_FIBRE_CPD,
    FBE_PORT_DRIVER_LAST
}fbe_port_driver_t;

/*Macro to verify the valid board class */
#define IS_VALID_BOARD_CLASS(class_id) (class_id == FBE_CLASS_ID_ARMADA_BOARD) 

/* FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_INFO */
typedef struct fbe_base_board_get_port_info_s {
    fbe_u32_t   server_index;
    fbe_u32_t   io_port_number;
    fbe_u32_t   io_portal_number;
} fbe_base_board_get_port_info_t;

/* FBE_BASE_BOARD_CONTROL_CODE_GET_IO_PORT_NUMBER */
typedef struct fbe_base_board_mgmt_get_io_port_number_s{
    fbe_u32_t       io_port_number;
    fbe_object_id_t parent_id;
}fbe_base_board_mgmt_get_io_port_number_t;

/* FBE_BASE_BOARD_CONTROL_CODE_GET_DIPLEX_PORT_NUMBER */
typedef struct fbe_base_board_mgmt_get_diplex_port_number_s{
    fbe_u32_t diplex_port_number;
    fbe_object_id_t parent_id;
}fbe_base_board_mgmt_get_diplex_port_number_t;

/*
 * SPS related Control Code structures
 */
// FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_STATUS
typedef struct fbe_base_board_mgmt_get_sps_status_s{
    fbe_u16_t                       spsIndex;           // input specifier
    fbe_bool_t                      spsModuleInserted;
    fbe_bool_t                      spsBatteryInserted;
    SPS_STATE                       spsStatus;
    fbe_sps_fault_info_t            spsFaultInfo;
    SPS_TYPE                        spsModel;
    HW_MODULE_TYPE                  spsModuleFfid;
    HW_MODULE_TYPE                  spsBatteryFfid;
    fbe_eir_input_power_sample_t    spsInputPower;
    fbe_u32_t                       spsBatteryTime;
}fbe_base_board_mgmt_get_sps_status_t;

/*
// FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_MANUF_INFO
#define FBE_SPS_SERIAL_NUM_REVSION_SIZE         16
#define FBE_SPS_PART_NUM_REVSION_SIZE           16
#define FBE_SPS_PART_NUM_REVSION_SIZE           16
#define FBE_SPS_VENDOR_NAME_SIZE                32
#define FBE_SPS_VENDOR_PART_NUMBER_SIZE         32
#define FBE_SPS_FW_REVISION_SIZE                8
#define FBE_SPS_MODEL_ID_QUERY_SIZE             8
#define FBE_SPS_MODEL_STRING_SIZE               8

typedef struct fbe_base_board_sps_manuf_info_s{
    fbe_u8_t    spsSerialNumber[FBE_SPS_SERIAL_NUM_REVSION_SIZE];
    fbe_u8_t    spsPartNumber[FBE_SPS_PART_NUM_REVSION_SIZE];
    fbe_u8_t    spsPartNumRevision[FBE_SPS_PART_NUM_REVSION_SIZE];
    fbe_u8_t    spsVendor[FBE_SPS_VENDOR_NAME_SIZE];
    fbe_u8_t    spsVendorModelNumber[FBE_SPS_VENDOR_PART_NUMBER_SIZE];
    fbe_u8_t    spsFirmwareRevision[FBE_SPS_FW_REVISION_SIZE];
    fbe_u8_t    spsModelString[FBE_SPS_MODEL_STRING_SIZE];

}fbe_base_board_sps_manuf_info_t;
*/

typedef struct fbe_base_board_mgmt_get_sps_manuf_info_s{
    fbe_sps_manuf_info_t            spsManufInfo;
}fbe_base_board_mgmt_get_sps_manuf_info_t;

// FBE_BASE_BOARD_CONTROL_CODE_SPS_COMMAND
typedef struct fbe_base_board_mgmt_sps_command_s{
    fbe_sps_action_type_t    spsAction;
} fbe_base_board_mgmt_sps_command_t;

// FBE_BASE_BOARD_CONTROL_CODE_SET_SPS_STATUS
typedef struct fbe_base_board_mgmt_set_sps_status_s{
    SPS_STATE               spsStatus;
    fbe_sps_fault_info_t    spsFaultInfo;
}fbe_base_board_mgmt_set_sps_status_t;

/*
 * Battery related Control Code structures
 */
// FBE_BASE_BOARD_CONTROL_CODE_GET_BATTERY_STATUS
typedef struct fbe_base_board_mgmt_get_battery_status_s{
    fbe_device_physical_location_t  device_location;
    fbe_base_battery_info_t         batteryInfo;
}fbe_base_board_mgmt_get_battery_status_t;

// FBE_BASE_BOARD_CONTROL_CODE_BATTERY_COMMAND
typedef enum fbe_base_board_battery_action_type_e {
    FBE_BATTERY_ACTION_NONE = 0,
    FBE_BATTERY_ACTION_START_TEST,
    FBE_BATTERY_ACTION_ENABLE,
    FBE_BATTERY_ACTION_SHUTDOWN,
    FBE_BATTERY_ACTION_POWER_REQ_INIT,
    FBE_BATTERY_ACTION_ENABLE_RIDE_THROUGH,
    FBE_BATTERY_ACTION_DISABLE_RIDE_THROUGH,
} fbe_base_board_battery_action_type_t;

typedef struct fbe_base_board_set_power_req_s
{
    BATTERY_ENERGY_REQUIREMENTS batteryEnergyRequirement;
} fbe_base_board_set_power_req_t;

typedef union fbe_base_board_battery_action_data_u
{
    fbe_base_board_set_power_req_t setPowerReqInfo;
} fbe_base_board_battery_action_data_t;

typedef struct fbe_base_board_mgmt_battery_command_s{
    fbe_device_physical_location_t device_location;
    fbe_base_board_battery_action_type_t    batteryAction;
    fbe_base_board_battery_action_data_t    batteryActionInfo;
} fbe_base_board_mgmt_battery_command_t;

// FBE_BASE_BOARD_CONTROL_CODE_GET_PE_INFO
typedef struct fbe_board_mgmt_get_pe_info_t{
    fbe_u8_t    pe_info[FBE_BOARD_PE_INFO_SIZE];
}fbe_board_mgmt_get_pe_info_t;

#if FALSE
// FBE_BASE_BOARD_CONTROL_CODE_GET_PS_INFO
typedef struct fbe_board_mgmt_get_ps_info_s
{
    fbe_u16_t       psCount;
    fbe_ps_info_t   psInfo[FBE_ESP_PS_MAX_COUNT];
} fbe_board_mgmt_get_ps_info_t;
#endif

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_SP_FAULT_LED
 */
typedef struct fbe_board_mgmt_set_sp_fault_LED_s
{
    LED_BLINK_RATE blink_rate;
    fbe_u32_t status_condition;
}fbe_board_mgmt_set_sp_fault_LED_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_FAULT_LED
 */
typedef struct fbe_board_mgmt_set_iom_fault_LED_s
{
    SP_ID           sp_id;
    fbe_u32_t       slot;
    fbe_u64_t device_type;
    LED_BLINK_RATE  blink_rate;
}fbe_board_mgmt_set_iom_fault_LED_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PORT_LED
 */
typedef struct fbe_board_mgmt_set_iom_port_LED_s
{
    fbe_board_mgmt_set_iom_fault_LED_t iom_LED;
    fbe_u32_t       io_port;
    LED_COLOR_TYPE  led_color;
}fbe_board_mgmt_set_iom_port_LED_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_IOM_PERSISTED_POWER_STATE
 */
typedef struct fbe_board_mgmt_set_iom_persisted_power_state_s
{
    SP_ID           sp_id;
    fbe_u32_t       slot;
    fbe_u64_t device_type;
    fbe_bool_t  persisted_power_enable;
}fbe_board_mgmt_set_iom_persisted_power_state_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_VLAN_CONFIG_MODE
 */
typedef struct fbe_board_mgmt_set_mgmt_vlan_mode_s
{
    SP_ID             sp_id;
    VLAN_CONFIG_MODE  vlan_config_mode;
}fbe_board_mgmt_set_mgmt_vlan_mode_t;

/*!********************************************************************* 
 * @struct fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t 
 *  
 * @brief 
 * This struct is used to store a context, function pointer and status 
 * for the completion of an asynchronous request.  In this case 
 * the caller would only be concerned with the status of the operation 
 * they requested. 
 *      
 **********************************************************************/
struct fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_s;
typedef struct fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_s
{
    void (* completion_function)(struct fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_s *context);
    fbe_board_mgmt_set_mgmt_vlan_mode_t     command;
    fbe_status_t                            status;
    void                                    *object_context;
}fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_MGMT_FAULT_LED
 */
typedef struct fbe_board_mgmt_set_mgmt_fault_LED_s
{
    SP_ID             sp_id;
    LED_BLINK_RATE    blink_rate;
}fbe_board_mgmt_set_mgmt_fault_LED_t;

/*!********************************************************************* 
 * @struct fbe_board_mgmt_set_mgmt_port_async_context_t 
 *  
 * @brief 
 * This struct is used to store a context, function pointer and status 
 * for the completion of an asynchronous request.  In this case 
 * the caller would only be concerned with the status of the operation 
 * they requested. 
 *      
 **********************************************************************/
struct fbe_board_mgmt_set_mgmt_port_async_context_s;
typedef struct fbe_board_mgmt_set_mgmt_port_async_context_s
{
    void (* completion_function)(struct fbe_board_mgmt_set_mgmt_port_async_context_s *context);
    fbe_board_mgmt_set_mgmt_port_t          command;
    fbe_status_t                            status;
    void                                    *object_context;
}fbe_board_mgmt_set_mgmt_port_async_context_t;


/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_CLEAR_HOLD_IN_POST_AND_OR_RESET
 */
typedef struct fbe_board_mgmt_set_PostAndOrReset_s
{
    SP_ID             sp_id;
    fbe_bool_t        holdInPost;
    fbe_bool_t        holdInReset;
    fbe_bool_t        rebootBlade;
    fbe_bool_t        rebootLocalSp;        // set reboot for local SP
    fbe_bool_t        flushBeforeReboot;    // flush info before reboot SP
    fbe_u8_t          retryStatusCount;
}fbe_board_mgmt_set_PostAndOrReset_t;

#define POST_AND_OR_RESET_STATUS_RETRY_COUNT  9

/*!********************************************************************* 
 * @struct fbe_board_mgmt_set_PostAndOrReset_async_context_t 
 *  
 * @brief 
 * This struct is used to store a context, function pointer and status 
 * for the completion of an asynchronous request.  In this case 
 * the caller would only be concerned with the status of the operation 
 * they requested. 
 *      
 **********************************************************************/
struct fbe_board_mgmt_set_PostAndOrReset_async_context_s;
typedef struct fbe_board_mgmt_set_PostAndOrReset_async_context_s
{
    void (* completion_function)(struct fbe_board_mgmt_set_PostAndOrReset_async_context_s *context);
    fbe_board_mgmt_set_PostAndOrReset_t     command;
    fbe_u8_t                                retry_count;
    void                                    *object_context;
    fbe_packet_t                            *packet;
    fbe_status_t                            status;
}fbe_board_mgmt_set_PostAndOrReset_async_context_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_FLUSH_FILES_AND_REG
 */
typedef struct fbe_board_mgmt_set_FlushFilesAndReg_s
{
    SP_ID             sp_id;
}fbe_board_mgmt_set_FlushFilesAndReg_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_SET_RESUME
 */
typedef struct fbe_board_mgmt_set_resume_s
{
    SMB_DEVICE             device;
    RESUME_PROM_STRUCTURE  resume_prom;
}fbe_board_mgmt_set_resume_t;

/*
 * FBE_BASE_BOARD_CONTROL_CODE_GET_RESUME
 */
typedef struct fbe_board_mgmt_get_resume_s
{
    fbe_device_physical_location_t device_location;
    fbe_u64_t       device_type;
    SPECL_RESUME_DATA       resume_data;
} fbe_board_mgmt_get_resume_t;

// FBE_BASE_BOARD_CONTROL_CODE_SET_PS_INFO
typedef struct fbe_base_board_mgmt_set_ps_info_s{
    fbe_u8_t                    psIndex;
    fbe_power_supply_info_t     psInfo;
}fbe_base_board_mgmt_set_ps_info_t;

// FBE_BASE_BOARD_CONTROL_CODE_SET_COMPONENT_MAX_TIMEOUT
typedef struct fbe_base_board_mgmt_set_max_timeout_info_s{
    fbe_u32_t    component;
    fbe_u32_t    max_timeout;
}fbe_base_board_mgmt_set_max_timeout_info_t;

//FBE_BASE_BOARD_CONTROL_CODE_GET_FBE_BASE_BOARD_INFO
typedef struct fbe_base_board_get_base_board_info_s{
    fbe_u32_t number_of_io_ports;
    fbe_edal_block_handle_t edal_block_handle;
    SP_ID spid;
    fbe_u32_t localIoModuleCount;
    SPID_PLATFORM_INFO platformInfo;
    fbe_bool_t mfgMode;
}fbe_base_board_get_base_board_info_t;

//FBE_BASE_BOARD_CONTROL_CODE_GET_EIR_INFO
typedef struct fbe_base_board_get_eir_info_s
{
    fbe_device_physical_location_t  location;
    fbe_esp_current_encl_eir_data_t peEirData;
}fbe_base_board_get_eir_info_t;


//FBE_BASE_BOARD_CONTROL_CODE_SET_LOCAL_SOFTWARE_BOOT_STATUS
typedef struct fbe_base_board_set_local_software_boot_status_s
{
    SW_GENERAL_STATUS_CODE          generalStatus;
    BOOT_SW_COMPONENT_CODE          componentStatus;
    SW_COMPONENT_EXT_CODE           componentExtStatus;
}fbe_base_board_set_local_software_boot_status_t;


//FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_SERIAL_NUM
typedef struct fbe_base_board_get_port_serial_num_s{
    fbe_u32_t   pci_bus;
    fbe_u32_t   pci_slot;
    fbe_u32_t   pci_function;
    fbe_u32_t   phy_map;
    fbe_u8_t    serial_num[FBE_EMC_SERIAL_NUM_SIZE];
}fbe_base_board_get_port_serial_num_t;


//FBE_BASE_BOARD_CONTROL_CODE_SET_CNA_MODE
typedef struct fbe_base_board_set_cna_mode
{
    SPECL_CNA_MODE cna_mode;
}fbe_base_board_set_cna_mode_t;

#endif /* FBE_BOARD_H */
