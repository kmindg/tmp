#ifndef FBE_ESP_BASE_ENVIRONMENT_H
#define FBE_ESP_BASE_ENVIRONMENT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_esp_base_environment.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the base_environment.
 *
 * @revision
 *   22-Jul-2010:  PHE - Created. 
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "specl_types.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_resume_prom_info.h"
#include "fbe_enclosure_interface.h"
#include "fbe/fbe_eses.h"

/*!*************************************************************************
 *  @def FBE_ENCLOSURE_MAX_PROGRAMMABLE_COUNT
 *  @brief Define the maximum number of programmable component in a device slot
 *  which are firmware updatable. Eg. two expanders on one EE LCC 
 ***************************************************************************/ 
#define FBE_ESP_MAX_PROGRAMMABLE_COUNT     5

/* For CDES2 firmware upgrade */
#define FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST     20
#define FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL       4
#define FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN           64


/*!********************************************************************* 
 * @enum fbe_esp_base_env_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the ESP base environment specific control
 * codes which the base environment object will accept.
 *         
 **********************************************************************/
typedef enum fbe_esp_base_env_control_code_e
{
    FBE_ESP_BASE_ENV_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_ENVIRONMENT),

    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_ANY_UPGRADE_IN_PROGRESS,
    FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_UPGRADE,
    FBE_ESP_BASE_ENV_CONTROL_CODE_ABORT_UPGRADE,
    FBE_ESP_BASE_ENV_CONTROL_CODE_RESUME_UPGRADE,
    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_WORK_STATE,
    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_INFO,
    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_SP_ID,
    
    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_RESUME_PROM_INFO,  // Get resume prom info from the memory
    FBE_ESP_BASE_ENV_CONTROL_CODE_WRITE_RESUME_PROM,
    FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_RESUME_PROM_READ, // Force to read the resume prom again.
    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_ANY_RESUME_PROM_READ_IN_PROGRESS,
    FBE_ESP_BASE_ENV_CONTROL_CODE_TERMINATE_UPGRADE,
    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_SERVICE_MODE,
    FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_MANIFEST_INFO,
    FBE_ESP_BASE_ENV_CONTROL_CODE_MODIFY_FUP_DELAY,
    /* Insert new control codes here. */

    FBE_ESP_BASE_ENV_CONTROL_CODE_LAST
}fbe_esp_base_env_control_code_t;


/*!********************************************************************* 
 * @enum fbe_base_env_fup_completion_status_t 
 *  
 * @brief 
 * This enumeration lists all the ESP base environment firmware upgrade completion status.
 *         
 **********************************************************************/
typedef enum fbe_base_env_fup_completion_status_e 
{
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_NONE  = 0,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_QUEUED, 
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS,      
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED,      
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_NO_REV_CHANGE,   
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_EXIT_UP_TO_REV,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NULL_IMAGE_PATH,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_REG_IMAGE_PATH,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_REV_CHANGE,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_IMAGE_HEADER,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_PARSE_IMAGE_HEADER, 
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_ENTIRE_IMAGE,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE, 
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_MISMATCHED_IMAGE,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_IMAGE_TYPES_TOO_SMALL,            
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DOWNLOAD_CMD_DENIED,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_DOWNLOAD_IMAGE,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_DOWNLOAD_STATUS,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_ACTIVATE_IMAGE,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_ACTIVATE_STATUS,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DEVICE_REMOVED,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_TRANSITION_STATE,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_WAIT_BEFORE_UPGRADE, 
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_TERMINATED, 
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_ACTIVATION_DEFERRED,
    FBE_BASE_ENV_FUP_COMPLETION_STATUS_LAST  /* This should always be at the end*/
} fbe_base_env_fup_completion_status_t;


/*!********************************************************************* 
 * @enum fbe_base_env_fup_work_state_t 
 *  
 * @brief 
 *  This enumeration lists all the ESP base environment firmware upgrade work state.
 *         
 **********************************************************************/
/* Do not change the order of the enum. */
typedef enum fbe_base_env_fup_work_state_e 
{
    FBE_BASE_ENV_FUP_WORK_STATE_NONE                   = 0,
    FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE, 
    FBE_BASE_ENV_FUP_WORK_STATE_QUEUED,
    FBE_BASE_ENV_FUP_WORK_STATE_WAIT_FOR_INTER_DEVICE_DELAY_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_READ_IMAGE_HEADER_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_CHECK_REV_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_READ_ENTIRE_IMAGE_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED,
    FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_RECEIVED,
    FBE_BASE_ENV_FUP_WORK_STATE_CHECK_ENV_STATUS_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT,
    FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT,
    FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_CHECK_RESULT_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_REFRESH_DEVICE_STATUS_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_END_UPGRADE_DONE,
    FBE_BASE_ENV_FUP_WORK_STATE_ABORT_CMD_SENT
} fbe_base_env_fup_work_state_t;


/*!********************************************************************* 
 * @enum fbe_base_env_fup_force_flags_t 
 *  
 * @brief 
 * This enumeration lists all the ESP base environment firmware upgrade force flags.
 *         
 **********************************************************************/
typedef enum fbe_base_env_fup_force_flags_e 
{
    FBE_BASE_ENV_FUP_FORCE_FLAG_NONE                = 0,
    FBE_BASE_ENV_FUP_FORCE_FLAG_NO_REV_CHECK        = 0x00000001,  // force upgrade mode
    FBE_BASE_ENV_FUP_FORCE_FLAG_SINGLE_SP_MODE      = 0x00000002,  // single sp mode
    FBE_BASE_ENV_FUP_FORCE_FLAG_NO_ENV_CHECK        = 0x00000004,  // ignore environmental faults
    FBE_BASE_ENV_FUP_FORCE_FLAG_READ_IMAGE          = 0x00000008,  // force to read image
    FBE_BASE_ENV_FUP_FORCE_FLAG_NO_BAD_IMAGE_CHECK  = 0x00000010,  // don't check the bad image
    FBE_BASE_ENV_FUP_FORCE_FLAG_NO_PRIORITY_CHECK   = 0x00000020,  // don't check the upgrade priority
    FBE_BASE_ENV_FUP_FORCE_FLAG_ACTIVATION_DEFERRED = 0x00000040,  // just do the download image, activation deferred.
    FBE_BASE_ENV_FUP_FORCE_FLAG_READ_MANIFEST_FILE  = 0x00000080,  // re-read the manifest file.
} fbe_base_env_fup_force_flags_t;

/*!********************************************************************* 
 * @enum fbe_base_env_sp_t 
 *  
 * @brief 
 * This enumeration lists SP IDs.
 *         
 **********************************************************************/
typedef enum fbe_base_env_sp_e
{
    FBE_BASE_ENV_SPA,
    FBE_BASE_ENV_SPB,
    FBE_BASE_ENV_INVALID
}fbe_base_env_sp_t;

/*!********************************************************************* 
 * @enum fbe_base_env_resume_prom_work_state_t 
 *  
 * @brief 
 *  This enumeration lists all the ESP base environment Resume Prom work state.
 *         
 **********************************************************************/
/* Do not change the order of the enum. */
typedef enum fbe_base_env_resume_prom_work_state_e 
{
    FBE_BASE_ENV_RESUME_PROM_INVALID = 0,  
    FBE_BASE_ENV_RESUME_PROM_READ_QUEUED,
    FBE_BASE_ENV_RESUME_PROM_READ_SENT,
    FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED
} fbe_base_env_resume_prom_work_state_t;

// structure to store resume prom info for IO modules
// This structure will be used by ESP to report resume
// prom info to its clients.

typedef struct fbe_resume_prom_s
{
    CHAR            emc_tla_part_num        [RESUME_PROM_EMC_TLA_PART_NUM_SIZE];
    CHAR            emc_tla_artwork_rev     [RESUME_PROM_EMC_TLA_ARTWORK_REV_SIZE];
    CHAR            emc_tla_assembly_rev    [RESUME_PROM_EMC_TLA_ASSEMBLY_REV_SIZE];
    CHAR            emc_tla_serial_num      [RESUME_PROM_EMC_TLA_SERIAL_NUM_SIZE];
    CHAR            product_part_number     [RESUME_PROM_PRODUCT_PN_SIZE];
    CHAR            product_serial_number   [RESUME_PROM_PRODUCT_SN_SIZE];
    CHAR            product_revision        [RESUME_PROM_PRODUCT_REV_SIZE];
    CHAR            vendor_part_num         [RESUME_PROM_VENDOR_PART_NUM_SIZE];
    CHAR            vendor_artwork_rev      [RESUME_PROM_VENDOR_ARTWORK_REV_SIZE];
    CHAR            vendor_assembly_rev     [RESUME_PROM_VENDOR_ASSEMBLY_REV_SIZE];
    CHAR            vendor_serial_num       [RESUME_PROM_VENDOR_SERIAL_NUM_SIZE];
    CHAR            vendor_name             [RESUME_PROM_VENDOR_NAME_SIZE];
    CHAR            loc_mft                 [RESUME_PROM_LOCATION_MANUFACTURE_SIZE];
    CHAR            year_mft                [RESUME_PROM_YEAR_MANUFACTURE_SIZE];
    CHAR            month_mft               [RESUME_PROM_MONTH_MANUFACTURE_SIZE];
    CHAR            day_mft                 [RESUME_PROM_DAY_MANUFACTURE_SIZE];
    CHAR            tla_assembly_name       [RESUME_PROM_TLA_ASSEMBLY_NAME_SIZE];
    UINT_32         num_programmables;
    RESUME_PROM_PROG_DETAILS prog_details   [RESUME_PROM_MAX_PROG_COUNT];
    UINT_32         wwn_seed;
    UINT_8E         system_orientation      [2]; //This should go away!!! it is now the FRU capability register
    CHAR            emc_serial_num          [RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE];
}fbe_resume_prom_t;


// FBE_BASE_ENV_CONTROL_CODE_GET_ANY_UPGRADE_IN_PROGRESS
/*!********************************************************************* 
 * @struct fbe_esp_base_env_get_any_upgrade_in_progress_t 
 *  
 * @brief 
 * This struct gets whether there is any upgrade in progress for the specified device type. 
 * deviceType - Input 
 * location - Input
 *         
 **********************************************************************/
typedef struct fbe_esp_base_env_get_any_upgrade_in_progress_s
{
    fbe_u64_t              deviceType;
    fbe_bool_t                     anyUpgradeInProgress;
} fbe_esp_base_env_get_any_upgrade_in_progress_t;


// FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_UPGRADE
/*!********************************************************************* 
 * @struct fbe_esp_base_env_initiate_upgrade_t 
 *  
 * @brief 
 * This struct initiates the firmware upgrade for the specified device. 
 * deviceType - Input 
 * location - Input
 *         
 **********************************************************************/
typedef struct fbe_esp_base_env_initiate_upgrade_s
{
    fbe_u64_t              deviceType;
    fbe_device_physical_location_t location;
    fbe_base_env_fup_force_flags_t forceFlags;
} fbe_esp_base_env_initiate_upgrade_t;


// FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_WORK_STATE
/*!********************************************************************* 
 * @struct fbe_esp_base_env_get_fup_work_state_t 
 *  
 * @brief 
 * This struct gets the ESP firmware upgrade work state for the specified device. 
 * deviceType - Input 
 * location - Input 
 * workState - Output 
 *         
 **********************************************************************/
typedef struct fbe_esp_base_env_get_fup_work_state_s
{
    fbe_u64_t              deviceType;
    fbe_device_physical_location_t location;
    fbe_base_env_fup_work_state_t  workState;
} fbe_esp_base_env_get_fup_work_state_t;

// FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_COMPLETION_STATUS
/*!********************************************************************* 
 * @struct fbe_esp_base_env_fup_info_t 
 *  
 * @brief 
 * This struct gives fup info of one expander
 * workState - Output 
 * completionStatus - Output 
 * imageRev - Output  image file revision on SP
 * preFirmwareRev - Output  Previous HW firmware revision
 * currentFirmwareRev - Output  Current HW firmware revision
 **********************************************************************/
typedef struct fbe_esp_base_env_fup_info_s
{
    fbe_base_env_fup_work_state_t         workState;
    fbe_base_env_fup_completion_status_t  completionStatus;  
    fbe_u8_t                              imageRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; 
    fbe_u8_t                              preFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; 
    fbe_u8_t                              currentFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                              componentId; 
    fbe_u32_t                             waitTimeBeforeUpgrade; 
}fbe_esp_base_env_fup_info_t;

/*!********************************************************************* 
 * @struct fbe_esp_base_env_get_fup_info_t 
 *  
 * @brief 
 * This struct gets the ESP firmware upgrade related information for the specified device. 
 * deviceType - Input 
 * location - Input 
 * completionStatus - Output 
 * imageRev - Output  image file revision on SP
 * prevImageRev - Output  Previous HW firmware revision
 * currImageRev - Output  Current HW firmware revision
 **********************************************************************/
typedef struct fbe_esp_base_env_get_fup_info_s
{
    fbe_u64_t                     deviceType;   // Input
    fbe_device_physical_location_t        location;     // Input
    fbe_u32_t                             programmableCount;  // Output
    fbe_esp_base_env_fup_info_t         fupInfo[FBE_ESP_MAX_PROGRAMMABLE_COUNT]; // Output
} fbe_esp_base_env_get_fup_info_t;

/*!********************************************************************* 
 * @struct fbe_base_env_fup_manifest_info_t 
 *  
 * @brief 
 * This struct defines the fup manifest info. 
 **********************************************************************/
typedef struct fbe_base_env_fup_manifest_info_s 
{
    fbe_char_t                   subenclProductId[FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1]; 
    fbe_char_t                   imageFileName[FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL][FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN + 1];
    ses_comp_type_enum           firmwareCompType[FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL];
    fbe_enclosure_fw_target_t    firmwareTarget[FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL];
    fbe_u32_t                    imageFileCount;
    fbe_char_t                   imageRev[FBE_BASE_ENV_FUP_MAX_IMAGE_COUNT_PER_SUBENCL][FBE_ESES_CDES2_MCODE_IMAGE_REV_SIZE_BYTES];
} fbe_base_env_fup_manifest_info_t;


// FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_MANIFEST_INFO
/*!********************************************************************* 
 * @struct fbe_esp_base_env_get_fup_manifest_info_t 
 *  
 * @brief 
 * This struct gets the firmware upgrade manifest info for the specified device type. 
 * deviceType - Input 
 * manifestInfo - Input
 *         
 **********************************************************************/
typedef struct fbe_esp_base_env_get_fup_manifest_info_s
{
    fbe_u64_t              deviceType;  // Input
    fbe_base_env_fup_manifest_info_t manifestInfo[FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST];  // Output
} fbe_esp_base_env_get_fup_manifest_info_t;

// FBE_ESP_BASE_ENV_CONTROL_CODE_MODIFY_FUP_DELAY
/*!********************************************************************* 
 * @struct fbe_esp_base_env_modify_fup_delay_t 
 *  
 * @brief 
 * This struct modifies the firmware upgrade delay from now for the specified device type. 
 * deviceType - Input 
 * delayInSec - Input
 *         
 **********************************************************************/
typedef struct fbe_esp_base_env_modify_fup_delay_s
{
    fbe_u64_t                      deviceType;  // Input
    fbe_u32_t                      delayInSec;  // Input
} fbe_esp_base_env_modify_fup_delay_t;

/*!********************************************************************* 
 * @struct fbe_esp_base_env_get_sp_id_s 
 *  
 * @brief 
 *   gives sp_id SPA or SPB 
 *
 * @ingroup fbe_esp_base_env_get_sp_id_s
 **********************************************************************/
typedef struct fbe_esp_base_env_get_sp_id_s
{
    fbe_base_env_sp_t sp_id;
} fbe_esp_base_env_get_sp_id_t;

// FBE_ESP_BASE_ENV_CONTROL_CODE_GET_RESUME_PROM_INFO
/*!********************************************************************* 
 * @struct fbe_esp_base_env_get_resume_prom_info_cmd_t 
 *  
 * @brief 
 * This struct gets the ESP Resume prom info for the specified device. 
 * deviceType - Input 
 * location - Input 
 * resume_prom_data - Output 
 * op_status - output
 *         
 **********************************************************************/
typedef struct fbe_esp_base_env_get_resume_prom_info_cmd_s
{
    fbe_u64_t               deviceType;
    fbe_device_physical_location_t  deviceLocation;
    RESUME_PROM_STRUCTURE           resume_prom_data;
    fbe_resume_prom_status_t        op_status;
    fbe_bool_t                      resumeFault;
} fbe_esp_base_env_get_resume_prom_info_cmd_t;



// FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_RESUME_PROM_READ
/*!********************************************************************* 
 * @struct fbe_esp_base_env_initiate_resume_prom_read_cmd_t 
 *  
 * @brief 
 * This struct is used to initiate the Resume prom read from the hardware. 
 * deviceType - Input 
 * location - Input 
 **********************************************************************/
typedef struct fbe_esp_base_env_initiate_resume_prom_read_cmd_s
{
    fbe_u64_t               deviceType;      // Input
    fbe_device_physical_location_t  deviceLocation;  // Input
} fbe_esp_base_env_initiate_resume_prom_read_cmd_t;


// FBE_ESP_BASE_ENV_CONTROL_CODE_GET_ANY_RESUME_PROM_READ_IN_PROGRESS
/*!*********************************************************************
 * @struct fbe_esp_base_env_get_any_resume_prom_read_in_progress_t
 *
 * @brief
 * This struct gets whether there is any resume prom read in progress for the specified class.
 * anyReadInProgress - Input
 *
 **********************************************************************/
typedef struct fbe_esp_base_env_get_any_resume_prom_read_in_progress_s
{
    fbe_bool_t                     anyReadInProgress;
} fbe_esp_base_env_get_any_resume_prom_read_in_progress_t;


// FBE_ESP_BASE_ENV_CONTROL_CODE_GET_SERVICE_MODE
/*!*********************************************************************
 * @struct fbe_esp_base_env_get_service_mode_t
 *
 * @brief
 * This struct gets whether the SP is currently in service mode.
 * isServiceMode - Input
 *
 **********************************************************************/
typedef struct fbe_esp_base_env_get_service_mode_s
{
    fbe_bool_t                     isServiceMode;
} fbe_esp_base_env_get_service_mode_t;

#endif /* FBE_ESP_BASE_ENVIRONMENT_H */
/*******************************
 * end file fbe_esp_base_environment.h
 *******************************/

