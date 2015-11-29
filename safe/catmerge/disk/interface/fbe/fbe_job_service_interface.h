#ifndef FBE_JOB_SERVICE_INTERFACE_H
#define FBE_JOB_SERVICE_INTERFACE_H

#include "fbe_spare.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_job_service_error.h"

/************** 
 * LITERALS
 **************/
#define FBE_MAX_JOB_SERVICE_DEBUG_HOOKS     64
#ifndef UEFI_ENV 
#define FBE_JOB_SERVICE_JOB_NUMBER_INVALID  CSX_CONST_U64(0xFFFFFFFFFFFFFFFF)
#else
#define FBE_JOB_SERVICE_JOB_NUMBER_INVALID  0xFFFFFFFFFFFFFFFF
#endif /* #ifndef UEFI_ENV  */

/*!*******************************************************************
 * @enum fbe_job_action_state_e
 *********************************************************************
 * @brief
 *  This is the set of job states that define where a job is being
 *  processed
 *
 *********************************************************************/
typedef enum fbe_job_action_state_e 
{
    FBE_JOB_ACTION_STATE_INIT = 0,     
    FBE_JOB_ACTION_STATE_IN_QUEUE,
    FBE_JOB_ACTION_STATE_DEQUEUED,
    FBE_JOB_ACTION_STATE_VALIDATE,      
    FBE_JOB_ACTION_STATE_SELECTION,      
    FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY,
    FBE_JOB_ACTION_STATE_PERSIST, 
    FBE_JOB_ACTION_STATE_ROLLBACK,
    FBE_JOB_ACTION_STATE_COMMIT,
    FBE_JOB_ACTION_STATE_DONE,
    FBE_JOB_ACTION_STATE_EXIT,
    FBE_JOB_ACTION_STATE_INVALID /* Invalid state for an object. */
}fbe_job_action_state_t;

/*!*******************************************************************
 * @enum fbe_job_type_e
 *********************************************************************
 * @brief
 *  This is the set of job type defines used for queued job service
 *  requests
 *
 *********************************************************************/
typedef enum fbe_job_type_e
{
    FBE_JOB_TYPE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_JOB_SERVICE),
    FBE_JOB_TYPE_DRIVE_SWAP,
    FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST,
    FBE_JOB_TYPE_RAID_GROUP_CREATE,
    FBE_JOB_TYPE_RAID_GROUP_DESTROY,
    FBE_JOB_TYPE_LUN_CREATE,
    FBE_JOB_TYPE_LUN_DESTROY,
    FBE_JOB_TYPE_LUN_UPDATE,
    FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO,
    FBE_JOB_TYPE_UPDATE_RAID_GROUP,    
    FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE,
    FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE,
    FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE,
    FBE_JOB_TYPE_UPDATE_SPARE_CONFIG,
    FBE_JOB_TYPE_CREATE_PROVISION_DRIVE,
    FBE_JOB_TYPE_UPDATE_DB_ON_PEER,
    FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER,
    FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE,
    FBE_JOB_TYPE_TRIGGER_SYSTEM_DRIVE_COPY_BACK_IF_NEEDED,  /*not used anymore. Leave it here just to avoid disruptive upgrade*/
    FBE_JOB_TYPE_CONTROL_SYSTEM_BG_SERVICE,
    FBE_JOB_TYPE_SEP_NDU_COMMIT,
    FBE_JOB_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO,
    FBE_JOB_TYPE_CONNECT_DRIVE,
    FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID,
    FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG,
    FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO,
    FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE,
    FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS,
    FBE_JOB_TYPE_SCRUB_OLD_USER_DATA,
    FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED,
    FBE_JOB_TYPE_VALIDATE_DATABASE,
    FBE_JOB_TYPE_CREATE_EXTENT_POOL,
    FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN,
    FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE,
    FBE_JOB_TYPE_DESTROY_EXTENT_POOL,
    FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN,
    FBE_JOB_TYPE_LAST
} fbe_job_type_t;

/*!*******************************************************************
 * @struct fbe_job_service_swap_notification_info_s
 *********************************************************************
 * @brief
 *  It defines job service notification data information for
 *  FBE_JOB_CONTROL_CODE_DRIVE_SWAP control code.
 *********************************************************************/
typedef struct fbe_job_service_swap_notification_info_s
{
    fbe_spare_swap_command_t                command_type;           // swap in or swap out commands.
    fbe_object_id_t                         vd_object_id;           // VD object id for the drive which fails.
    fbe_object_id_t                         orig_pvd_object_id;     // Original pvd object-id.
    fbe_object_id_t                         spare_pvd_object_id;    // Spare pvd object-id.
} fbe_job_service_swap_notification_info_t;

/*!*******************************************************************
 * @struct fbe_job_service_info_s
 *********************************************************************
 * @brief
 *  This is the definition of a job control information we send as a
 *  response in notification
 *
 *********************************************************************/
typedef struct fbe_job_service_info_s {
    fbe_object_id_t                 object_id;
    fbe_job_type_t                  job_type;
    fbe_status_t                    status;
    fbe_job_service_error_type_t    error_code;
    fbe_u64_t                       job_number;
    //void *                          context;

    union {
        fbe_job_service_swap_notification_info_t swap_info;
    };

}fbe_job_service_info_t;

/*!*******************************************************************
 * @enum fbe_job_debug_hook_action_e
 *********************************************************************
 * @brief
 *  This is the set of actions to take when a job service debug hook
 *  is reached.
 *
 *********************************************************************/
typedef enum fbe_job_debug_hook_action_e
{
    FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_INVALID,
    FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LOG,
    FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_PAUSE,    /*! Pause the job service thread */
    FBE_JOB_SERVICE_DEBUG_HOOK_ACTION_LAST
} fbe_job_debug_hook_action_t;

/*!*******************************************************************
 * @enum fbe_job_debug_hook_state_phase_e
 *********************************************************************
 * @brief
 *  Defines which phase of the selected state to check if the hook is
 *  reached.
 *
 *********************************************************************/
typedef enum fbe_job_debug_hook_state_phase_e
{
    FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_INVALID,
    FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_START,   /*! Stop or log at the beginning of the requested state */
    FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_END,     /*! Stop or log at the end of the requested state */    
    FBE_JOB_SERVICE_DEBUG_HOOK_STATE_PHASE_LAST
} fbe_job_debug_hook_state_phase_t;

/*!*******************************************************************
 * @struct fbe_job_service_debug_hook_t
 *********************************************************************
 * @brief
 *  This is the definition of a job service debug hook
 *
 *********************************************************************/
typedef struct fbe_job_service_debug_hook_s {
    fbe_object_id_t                     object_id;      /*! IN - Object id associated with this job */
    fbe_job_type_t                      hook_type;      /*! IN - The job type expected */
    fbe_job_action_state_t              hook_state;     /*! IN - The job state for the hook */
    fbe_job_debug_hook_state_phase_t    hook_phase;     /*! IN - The point in the specified state to check for the hook */
	fbe_job_debug_hook_action_t         hook_action;    /*! IN - The action to take when the hook is reached: log, pause job service thread etc.*/
	fbe_u32_t                           hit_counter;    /*! OUT - The number of times this hook has been hit */
    fbe_u64_t                           job_number;     /*! OUT - The job number associated with the hook */
} fbe_job_service_debug_hook_t;

#endif /* FBE_JOB_SERVICE_INTERFACE_H */
