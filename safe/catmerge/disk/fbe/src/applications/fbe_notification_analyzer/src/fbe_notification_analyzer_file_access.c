/***************************************************************************
* Copyright (C) EMC Corporation 2012 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_notification_analyzer_file_access.c
****************************************************************************
*
* @brief
*  This file contains file access functions for fbe_notification_analyzer.
*
* @ingroup fbe_notification_analyzer
*
* @date
*  05/30/2012 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_notification_analyzer_file_access.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_time.h"
#include "fbe_api_common_interface.h"
#include "fbe_api_common.h"

/*!********************************************************************* 
 * @struct notification_enum_to_str_t 
 *  
 * @brief 
 *   This contains the data info for notification type to string.
 *
 * @ingroup notification_enum_to_str
 **********************************************************************/
typedef struct notification_enum_to_str_s{
    fbe_notification_type_t     notification_type;      /*!< notification_type */
    const fbe_u8_t *            notification_str;  /*!< pointer to the output notification string */
}notification_enum_to_str_t;

static notification_enum_to_str_t      notification_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_INVALID),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY),           
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE),      
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_NON_PENDING_STATE_CHANGE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_END_OF_LIFE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_RECOVERY),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_CALLHOME),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_SWAP_INFO),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_CMS_TEST_DONE),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_ZEROING),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_LU_DEGRADED_STATE_CHANGED),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_ALL_WITHOUT_PENDING_STATES),
    FBE_API_ENUM_TO_STR(FBE_NOTIFICATION_TYPE_ALL)
};

/*!********************************************************************* 
 * @struct package_enum_to_str_t 
 *  
 * @brief 
 *   This contains the data info for notification source package to string.
 *
 * @ingroup notification_enum_to_str
 **********************************************************************/
typedef struct package_enum_to_str_s{
    fbe_package_id_t package;      /*!< package_type */
    const fbe_u8_t *            package_str;  /*!< pointer to the output package string */
}package_enum_to_str_t;

static package_enum_to_str_t      package_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_PACKAGE_ID_INVALID),
    FBE_API_ENUM_TO_STR(FBE_PACKAGE_ID_PHYSICAL),
    FBE_API_ENUM_TO_STR(FBE_PACKAGE_ID_NEIT),
    FBE_API_ENUM_TO_STR(FBE_PACKAGE_ID_SEP_0),
    FBE_API_ENUM_TO_STR(FBE_PACKAGE_ID_ESP),
    FBE_API_ENUM_TO_STR(FBE_PACKAGE_ID_LAST)

};

/*!********************************************************************* 
 * @struct object_enum_to_str_t 
 *  
 * @brief 
 *   This contains the data info for notification object type to string.
 *
 * @ingroup object_enum_to_str
 **********************************************************************/
typedef struct object_enum_to_str_s{
    fbe_topology_object_type_t object_type;      /*!< object_type */
    const fbe_u8_t *           object_str;  /*!< pointer to the output object type string */
}object_enum_to_str_t;

static object_enum_to_str_t      object_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_INVALID),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_BOARD),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_PORT),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_RAID_GROUP),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_RAID_GROUP),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_LUN),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_LCC),
    FBE_API_ENUM_TO_STR(FBE_TOPOLOGY_OBJECT_TYPE_ALL)
};
/*!********************************************************************* 
 * @struct job_error_to_str_t 
 *  
 * @brief 
 *   This contains the data info for job error code to string.
 *
 * @ingroup notification_enum_to_str
 **********************************************************************/
typedef struct job_error_enum_to_str_s{
   fbe_job_service_error_type_t job_err;      /*!< job_error_code */
   const fbe_u8_t *            job_err_str;  /*!< pointer to the output job error code string */
}job_error_enum_to_str_t;

static job_error_enum_to_str_t      job_error_table [] =
{

    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_NO_ERROR),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_VALUE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_ID),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_NULL_COMMAND),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_UNKNOWN_ID),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_JOB_ELEMENT),

    /* create/destroy library error codes. */
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_RAID_GROUP_ID_IN_USE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_LUN_ID_IN_USE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_DUPLICATE_PVD),                      
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_PVDS),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_DRIVE_TYPES),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_TYPE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_NUMBER),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_CAPACITY),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_ADDRESS_OFFSET),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SYSTEM_LIMITS_EXCEEDED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID),

    /* Non-terminal errors*/
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED),

    /* spare library job service error codes. */
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_EDGE_INDEX),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_PERM_SPARE_EDGE_INDEX),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_PROACTIVE_SPARE_EDGE_INDEX),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_UNEXPECTED_ERROR),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_PROACTIVE_SPARE_NOT_REQUIRED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_COPY_SOURCE_DRIVE_REMOVED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_REMOVED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_COPY_INVALID_DESTINATION_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_HAS_UPSTREAM_RAID_GROUP),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL),

    /* spare library job service error code used for mapping spare validation failure.*/
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SPARE_NOT_READY),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SPARE_REMOVED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SPARE_BUSY),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_NOT_REDUNDANT),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_DESTROYED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_INVALID_DESIRED_SPARE_DRIVE_TYPE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_OFFSET_MISMATCH),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_BLOCK_SIZE_MISMATCH),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_CAPACITY_MISMATCH),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SYS_DRIVE_MISMATCH),

    /* configure pvd object with different config type - job service error codes. */
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_INVALID_UPDATE_TYPE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_INVALID_CONFIG_TYPE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_UNCONSUMED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_RAID),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_IS_NOT_CONFIGURED),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_PVD_IS_IN_END_OF_LIFE_STATE),

    /* system drive copy back error codes*/
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SDCB_NOT_SYSTEM_PVD),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SDCB_FAIL_GET_ACTION),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SDCB_FAIL_CHANGE_PVD_CONFIG),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SDCB_FAIL_POST_PROCESS),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SDCB_START_DB_TRASACTION_FAIL),
    FBE_API_ENUM_TO_STR(FBE_JOB_SERVICE_ERROR_SDCB_COMMIT_DB_TRASACTION_FAIL)

};

/*!********************************************************************* 
 * @struct job_type_to_str_t 
 *  
 * @brief 
 *   This contains the data info for job type to string.
 *
 * @ingroup notification_enum_to_str
 **********************************************************************/
typedef struct job_type_enum_to_str_s{
   fbe_job_type_t     job_type;      /*!< job_type */
   const fbe_u8_t *   job_type_str;  /*!< pointer to the output job_type string */
}job_type_enum_to_str_t;

static job_type_enum_to_str_t      job_type_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_INVALID),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_DRIVE_SWAP),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_RAID_GROUP_CREATE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_RAID_GROUP_DESTROY),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_LUN_CREATE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_LUN_DESTROY),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_LUN_UPDATE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_RAID_GROUP),    
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_SPARE_CONFIG),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_CREATE_PROVISION_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_DB_ON_PEER),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_CONTROL_SYSTEM_BG_SERVICE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_SEP_NDU_COMMIT),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_CONNECT_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_SCRUB_OLD_USER_DATA),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_VALIDATE_DATABASE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE),
    FBE_API_ENUM_TO_STR(FBE_JOB_TYPE_LAST)

};
/*!********************************************************************* 
 * @struct device_type_to_str_t 
 *  
 * @brief 
 *   This contains the data info for device type to string.
 *
 * @ingroup notification_enum_to_str
 **********************************************************************/
typedef struct device_type_enum_to_str_s{
   fbe_u64_t     device_type;      /*!< device_type */
   const fbe_u8_t *   device_type_str;  /*!< pointer to the output device_type string */
}device_type_enum_to_str_t;

static device_type_enum_to_str_t      device_type_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_INVALID),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_ENCLOSURE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_LCC),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_PS),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_FAN),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_SPS),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_IOMODULE),
    //FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_IOANNEX),    
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_DRIVE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_MEZZANINE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_MGMT_MODULE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_SLAVE_PORT),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_PLATFORM),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_SUITCASE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_MISC),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_SFP),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_SP),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_TEMPERATURE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_PORT_LINK),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_SP_ENVIRON_STATE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_CONNECTOR),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_DISPLAY),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_BACK_END_MODULE),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_BATTERY),
    FBE_API_ENUM_TO_STR(FBE_DEVICE_TYPE_BMC)

};

/* local functions */
static const fbe_u8_t * fbe_notification_analyzer_notification_to_string (fbe_notification_type_t notification_in);
static const fbe_u8_t * fbe_notification_analyzer_package_to_string (fbe_package_id_t package_in);
static const fbe_u8_t * fbe_notification_analyzer_object_to_string (fbe_topology_object_type_t object_in);
static const fbe_u8_t * fbe_notification_analyzer_job_error_to_string (fbe_job_service_error_type_t job_error_in);
static const fbe_u8_t * fbe_notification_analyzer_job_type_to_string (fbe_job_type_t job_type_in);
static const fbe_u8_t * fbe_notification_analyzer_device_type_to_string (fbe_u64_t device_type_in);
static fbe_char_t * fbe_notification_analyzer_zero_status_to_string(fbe_object_zero_notification_t status);
static fbe_char_t * fbe_notification_analyzer_recontruction_status_to_string(fbe_raid_group_data_reconstrcution_notification_op_t status);
static fbe_char_t * fbe_notification_analyzer_call_home_to_string(fbe_call_hme_notify_rsn_type_t reason);
static fbe_char_t * fbe_notification_analyzer_swap_command_to_string(fbe_spare_swap_command_t command);
static fbe_char_t * fbe_notification_analyzer_device_data_to_string(fbe_device_data_type_t device_data);

/*!**********************************************************************
*             fbe_notification_analyzer_open_file()               
*************************************************************************
*
*  @brief
*    fbe_notification_analyzer_open_file - Open a file for fbe_notification_analyzer
    
*  @param    * file_name - the file name pointer
*  @param    fMode - open file mode(clean or append)
*
*  @return
*			 File*
*************************************************************************/
FILE * fbe_notification_analyzer_open_file(const char* file_name, char fMode)                                                                      
{                                                                                                                          
    FILE * fp;                                                                                                             
    if (fMode == 'c') 
    {
        if((fp = fopen(file_name, "w"))==NULL)                                                                                 
        {                                                                                                                      
            return NULL;                                                                                                       
        }
        else return fp;   
    }
    else if (fMode == 'a') 
    {
        if((fp = fopen(file_name, "a"))==NULL)                                                                                 
        {                                                                                                                      
            return NULL;                                                                                                       
        }
        else return fp;
    }
    else if (fMode == 'r') 
    {
        if((fp = fopen(file_name, "r"))==NULL)                                                                                 
        {                                                                                                                      
            return NULL;                                                                                                       
        }
        else return fp;
    }
    else return NULL;                                                                                                                                                                                                                             
}                                                                                                                          
/******************************************
 * end fbe_notification_analyzer_open_file()
 ******************************************/                                                                                                                           
    
/*!**********************************************************************
*             fbe_notification_analyzer_write_to_file()               
*************************************************************************
*
*  @brief
*    fbe_notification_analyzer_write_to_file - Write content into a file
    
*  @param    * fp - the file pointer
*  @param    content - content to be writen to the file
*
*  @return
*			 fbe_status_t
*************************************************************************/                                                                                                                       
fbe_status_t fbe_notification_analyzer_write_to_file(FILE * fp,fbe_notification_analyzer_notification_info_t * content)                                                    
{  
    fbe_system_time_t currentTime;
                                                                                                                           
    fbe_get_system_time(&currentTime);                                                                                     

    fprintf(fp, "\n%04d-%02d-%02d %02d:%02d:%02d-%03d",                                                            
                currentTime.year,currentTime.month,currentTime.day,currentTime.hour,currentTime.minute,currentTime.second,currentTime.milliseconds);

    fprintf(fp, "\n\nnotification_type: %s \n", fbe_notification_analyzer_notification_to_string(content->notification_info.notification_type));
    fprintf(fp, "source_package:    %s \n", fbe_notification_analyzer_package_to_string(content->notification_info.source_package));
    if(content->object_id == FBE_OBJECT_ID_INVALID) 
    {
        fprintf(fp, "object_id: FBE_OBJECT_ID_INVALID \n");
    }
    else 
    {
        fprintf(fp, "object_id:         0x%x \n",content->object_id);
    }
    fprintf(fp, "object_type:       %s \n",fbe_notification_analyzer_object_to_string(content->notification_info.object_type));
    
    switch (content->notification_info.notification_type) 
    {
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_ACTIVATE: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY:       
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_HIBERNATE:   
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_OFFLINE:        
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL:         
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_READY: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_ACTIVATE: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_HIBERNATE: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_OFFLINE: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_FAIL:
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY:
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE: 
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_NON_PENDING_STATE_CHANGE:
             break;
        case FBE_NOTIFICATION_TYPE_END_OF_LIFE: 
            break;
        case FBE_NOTIFICATION_TYPE_RECOVERY:
            break;
        case FBE_NOTIFICATION_TYPE_CHECK_QUEUED_IO:
            break;
        case FBE_NOTIFICATION_TYPE_CALLHOME:
            fprintf(fp, "data associate with this notification:\n");
            fprintf(fp, "\t call home reason %s \n", 
                    fbe_notification_analyzer_call_home_to_string(content->notification_info.notification_data.call_home_reason));
            break;
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:
            /* more fields.*/
            fprintf(fp, "data associate with this notification:\n");
            fprintf(fp, "\t device_type:%s, device_data_type:%s \n",  
                    fbe_notification_analyzer_device_type_to_string(content->notification_info.notification_data.data_change_info.device_mask),
                    fbe_notification_analyzer_device_data_to_string(content->notification_info.notification_data.data_change_info.data_type));
            fprintf(fp, "\t physical location: processorEnclosure:%s, bus:%c, enclosure:%c, componentID:%c, bank:%c, bank_slot:%c, slot:%c, sp:%c, port:%c.\n", 
                    (content->notification_info.notification_data.data_change_info.phys_location.processorEnclosure)?"true":"false",
                    content->notification_info.notification_data.data_change_info.phys_location.bus,
                    content->notification_info.notification_data.data_change_info.phys_location.enclosure,
                    content->notification_info.notification_data.data_change_info.phys_location.componentId,
                    content->notification_info.notification_data.data_change_info.phys_location.bank,
                    content->notification_info.notification_data.data_change_info.phys_location.bank_slot,
                    content->notification_info.notification_data.data_change_info.phys_location.slot,
                    content->notification_info.notification_data.data_change_info.phys_location.sp,
                    content->notification_info.notification_data.data_change_info.phys_location.port);
            break;
        case FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED:
            /* more fields.*/
            fprintf(fp, "data associate with this notification:\n");
            fprintf(fp, "\t object_id:0x%x, job_status:%d, job_type:%s, job_error_code:%s, job_number: 0x%llx \n",  
                    content->notification_info.notification_data.job_service_error_info.object_id,
                    content->notification_info.notification_data.job_service_error_info.status,
                    fbe_notification_analyzer_job_type_to_string(content->notification_info.notification_data.job_service_error_info.job_type),
                    fbe_notification_analyzer_job_error_to_string(content->notification_info.notification_data.job_service_error_info.error_code),
                    content->notification_info.notification_data.job_service_error_info.job_number);
            if (content->notification_info.notification_data.job_service_error_info.job_type == FBE_JOB_TYPE_DRIVE_SWAP) {
                fprintf(fp, "\t swap command type:%s, ori_pvd:0x%x, spare_pvd:0x%x, vd:0x%x", 
                        fbe_notification_analyzer_swap_command_to_string(content->notification_info.notification_data.job_service_error_info.swap_info.command_type),
                        content->notification_info.notification_data.job_service_error_info.swap_info.orig_pvd_object_id,
                        content->notification_info.notification_data.job_service_error_info.swap_info.spare_pvd_object_id,
                        content->notification_info.notification_data.job_service_error_info.swap_info.vd_object_id);
            }
            break;
        case FBE_NOTIFICATION_TYPE_SWAP_INFO:
            fprintf(fp, "data associate with this notification:\n");
            fprintf(fp, "\t swap command type:%s, ori_pvd:0x%x, spare_pvd:0x%x, vd:0x%x \n", 
                    fbe_notification_analyzer_swap_command_to_string(content->notification_info.notification_data.swap_info.command_type),
                    content->notification_info.notification_data.swap_info.orig_pvd_object_id,
                    content->notification_info.notification_data.swap_info.spare_pvd_object_id,
                    content->notification_info.notification_data.swap_info.vd_object_id);
            break;
        case FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED:
            break;
        case FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED:
            break;
        case FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE:
            fprintf(fp, "data associate with this notification:\n");
            fprintf(fp, "\t error_trace_info %s \n", 
                    content->notification_info.notification_data.error_trace_info.bytes);
        case FBE_NOTIFICATION_TYPE_CMS_TEST_DONE:
            break;
        case FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION:
            fprintf(fp, "data associate with this notification:\n");
            fprintf(fp, "\t percent_complete:%d, state:%s \n", 
                    content->notification_info.notification_data.data_reconstruction.percent_complete,
                    fbe_notification_analyzer_recontruction_status_to_string(content->notification_info.notification_data.data_reconstruction.state));
            break;
        case FBE_NOTIFICATION_TYPE_ZEROING:
            fprintf(fp, "data associate with this notification:\n");
            fprintf(fp, "\t progress_percent %d, zero_operation_status:%s \n",
                    content->notification_info.notification_data.object_zero_notification.zero_operation_progress_percent, 
                    fbe_notification_analyzer_zero_status_to_string(content->notification_info.notification_data.object_zero_notification.zero_operation_status));  
            break;
        case FBE_NOTIFICATION_TYPE_LU_DEGRADED_STATE_CHANGED:
            fprintf(fp, "data associate with this notification: lun_degraded_flag:%s \n", 
                    (content->notification_info.notification_data.lun_degraded_flag)?"true":"false"); 
            break; 
    }
                                                                       
                                                                                                                           
    return FBE_STATUS_OK;                                                                                                  
}                                                                                                                          
/******************************************
 * end fbe_notification_analyzer_write_to_file()
 ******************************************/      
                                                                                                                      
/*!**********************************************************************
*             fbe_notification_analyzer_close_file()               
*************************************************************************
*
*  @brief
*    fbe_notification_analyzer_close_file - Close the file for fbe_notification_analyzer
    
*  @param    * fp - the file pointerr
*
*  @return
*			 fbe_status_t
*************************************************************************/                                                                                                                      
fbe_status_t fbe_notification_analyzer_close_file(FILE * fp)                                                                           
{                                                                                                                          
    fclose(fp);                                                                                                            
    return FBE_STATUS_OK;                                                                                                  
}          
/******************************************
 * end fbe_notification_analyzer_close_file()
 ******************************************/

/*!***************************************************************
 * @fnfbe_notification_analyzer_notification_to_string(
 *        fbe_lifecycle_state_t state_in)
 *****************************************************************
 * @brief
 *   converts notification type to a string
 *
 * @param notification_in - notification type
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    05/31/12: Vera Wang  created
 *    
 ****************************************************************/
static const fbe_u8_t * fbe_notification_analyzer_notification_to_string (fbe_notification_type_t notification_in)
{                                                                                                 
    notification_enum_to_str_t *   notification_lookup = notification_table;                                           
                                                                                                  
    while (notification_lookup->notification_type != FBE_NOTIFICATION_TYPE_ALL) {                                  
        if (notification_lookup->notification_type == notification_in) {                                                    
            return notification_lookup->notification_str;                                                       
        }                                                                                          
        notification_lookup++;                                                                           
    }                                                                                             
                                                                                                  
    return " ";                                                               
}  
                                                                                               
/*!***************************************************************
 * @fnfbe_notification_analyzer_package_to_string(
 *        fbe_package_notification_id_mask_t package_in)
 *****************************************************************
 * @brief
 *   converts package type to a string
 *
 * @param package_in - package
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    05/31/12: Vera Wang  created
 *    
 ****************************************************************/
static const fbe_u8_t * fbe_notification_analyzer_package_to_string (fbe_package_id_t package_in)
{                                                                                                 
    package_enum_to_str_t *   package_lookup = package_table;                                           
                                                                                                  
    while (package_lookup->package != FBE_PACKAGE_ID_LAST) {                                  
        if (package_lookup->package == package_in) {                                                    
            return package_lookup->package_str;                                                       
        }                                                                                         
        package_lookup++;                                                                           
    }                                                                                             
                                                                                                  
    return " ";                                                               
}   
                                                                                                
/*!***************************************************************
 * @fnfbe_notification_analyzer_object_to_string(
 *        fbe_lifecycle_state_t state_in)
 *****************************************************************
 * @brief
 *   converts object type to a string
 *
 * @param object_in - object
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    05/31/12: Vera Wang  created
 *    
 ****************************************************************/
static const fbe_u8_t * fbe_notification_analyzer_object_to_string (fbe_topology_object_type_t object_in)
{                                                                                                 
    object_enum_to_str_t *   object_lookup = object_table;                                           
                                                                                                  
    while (object_lookup->object_type != FBE_TOPOLOGY_OBJECT_TYPE_ALL) {                                  
        if (object_lookup->object_type == object_in) {                                                    
            return object_lookup->object_str;                                                       
        }                                                                                         
        object_lookup++;                                                                           
    }                                                                                             
                                                                                                  
    return " ";                                                                     
}                                                                                                     
 
static fbe_char_t * fbe_notification_analyzer_zero_status_to_string(fbe_object_zero_notification_t status)
{
    switch(status)
    {
        case FBE_OBJECT_ZERO_STARTED:
            return "FBE_OBJECT_ZERO_STARTED";
        case FBE_OBJECT_ZERO_ENDED:
            return "FBE_OBJECT_ZERO_ENDED";
        case FBE_OBJECT_ZERO_IN_PROGRESS:
            return "FBE_OBJECT_ZERO_IN_PROGRESS";
        default:
            return "unknown zero operation status";
    }
}
                                                                                                                
static fbe_char_t * fbe_notification_analyzer_recontruction_status_to_string(fbe_raid_group_data_reconstrcution_notification_op_t status)
{
    switch(status)
    {
        case DATA_RECONSTRUCTION_START:
            return "DATA_RECONSTRUCTION_START";
        case DATA_RECONSTRUCTION_END:
            return "DATA_RECONSTRUCTION_END";
        case DATA_RECONSTRUCTION_IN_PROGRESS:
            return "DATA_RECONSTRUCTION_IN_PROGRESS";
        default:
            return "unknown data recontruction status";
    }
}

static fbe_char_t * fbe_notification_analyzer_call_home_to_string(fbe_call_hme_notify_rsn_type_t  reason)
{
    switch(reason)
    {
        case NOTIFY_DIEH_END_OF_LIFE_THRSLD_RCHD:
            return "NOTIFY_DIEH_END_OF_LIFE_THRSLD_RCHD";
        case NOTIFY_DIEH_FAIL_THRSLD_RCHD:
            return "NOTIFY_DIEH_FAIL_THRSLD_RCHD";
        default:
            return "unknown call home reason";
    }
}  

static fbe_char_t * fbe_notification_analyzer_swap_command_to_string(fbe_spare_swap_command_t command)
{
    switch(command)
    {
        case FBE_SPARE_SWAP_INVALID_COMMAND :
            return "FBE_SPARE_SWAP_INVALID_COMMAND";
        case FBE_SPARE_COMPLETE_COPY_COMMAND :
            return "FBE_SPARE_COMPLETE_COPY_COMMAND";
        case FBE_SPARE_ABORT_COPY_COMMAND:
            return "FBE_SPARE_ABORT_COPY_COMMAND";
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND :
            return "FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND";
        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
            return "FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND";
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND :
            return "FBE_SPARE_INITIATE_USER_COPY_COMMAND ";
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            return "FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND";
        default:
            return "unknown swap commend type";
    }
}     

static fbe_char_t * fbe_notification_analyzer_device_data_to_string(fbe_device_data_type_t device_data)
{
    switch(device_data)
    {
        case FBE_DEVICE_DATA_TYPE_INVALID :
            return "FBE_DEVICE_DATA_TYPE_INVALID";
        case FBE_DEVICE_DATA_TYPE_GENERIC_INFO :
            return "FBE_DEVICE_DATA_TYPE_GENERIC_INFO";
        case FBE_DEVICE_DATA_TYPE_PORT_INFO :
            return "FBE_DEVICE_DATA_TYPE_PORT_INFO";
        case FBE_DEVICE_DATA_TYPE_FUP_INFO:
            return "FBE_DEVICE_DATA_TYPE_FUP_INFO";
        case FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO :
            return "FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO";
        default:
            return "unknown device data type";
    }
}  

static const fbe_u8_t * fbe_notification_analyzer_job_error_to_string (fbe_job_service_error_type_t job_error_in)
{                                                                                                 
    job_error_enum_to_str_t *   job_error_lookup = job_error_table;                                           
                                                                                                  
    while (job_error_lookup->job_err != FBE_JOB_SERVICE_ERROR_SDCB_COMMIT_DB_TRASACTION_FAIL) {                                  
        if (job_error_lookup->job_err == job_error_in) {                                                    
            return job_error_lookup->job_err_str;                                                       
        }                                                                                         
        job_error_lookup++;                                                                           
    }                                                                                             
                                                                                                  
    return " ";                                                                    
}   
                                                                                                  
static const fbe_u8_t * fbe_notification_analyzer_job_type_to_string (fbe_job_type_t job_type_in)
{                                                                                                 
    job_type_enum_to_str_t *   job_type_lookup = job_type_table;                                           
                                                                                                  
    while (job_type_lookup->job_type != FBE_JOB_TYPE_LAST) {                                  
        if (job_type_lookup->job_type == job_type_in) {                                                    
            return job_type_lookup->job_type_str;                                                       
        }                                                                                         
        job_type_lookup++;                                                                           
    }                                                                                             
                                                                                                  
    return " ";                                                              
}                                                                                                     

static const fbe_u8_t * fbe_notification_analyzer_device_type_to_string (fbe_u64_t device_type_in)
{                                                                                                 
    device_type_enum_to_str_t *   device_type_lookup = device_type_table;                                           
                                                                                                  
    while (device_type_lookup->device_type != FBE_DEVICE_TYPE_BATTERY) {                                  
        if (device_type_lookup->device_type == device_type_in) {                                                    
            return device_type_lookup->device_type_str;                                                       
        }                                                                                         
        device_type_lookup++;                                                                           
    }                                                                                             
                                                                                                  
    return " ";                                                              
}           
                                                                                          
/******************************************
* end file fbe_notification_analyzer_file_access.c
*******************************************/
