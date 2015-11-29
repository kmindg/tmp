#ifndef FBE_JOB_SERVICE_H
#define FBE_JOB_SERVICE_H

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_job_service_interface.h"
#include "fbe_job_service_swap.h"
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_base_config.h"

#define FBE_JOB_ELEMENT_CONTEXT_SIZE  2048
#define FBE_JOB_CONTROL_MAX_QUEUE_DEPTH 10240 
#define FBE_JOB_CONTROL_IDLE_TIMER 100 /* 100 ms. */
#define FBE_JOB_CONTROL_DEFAULT_TIMER 3000 /* 3 sec. by default */
#define MAX_NUMBER_OF_JOB_CONTROL_ELEMENTS FBE_JOB_CONTROL_MAX_QUEUE_DEPTH
#define FBE_JOB_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS FBE_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS_PER_JOB

typedef void * fbe_job_context_t;
typedef void * fbe_job_command_data_t;

void job_service_thread_func(void * context);
void job_service_arrival_thread_func(void * context);

/* completion function */
struct fbe_job_control_element_s;
typedef void (* fbe_job_control_completion_function_t)
    (struct fbe_job_control_element_s * job_control_request, 
     fbe_job_context_t context);

fbe_status_t fbe_job_service_packet_selection_completion_function(fbe_packet_t * packet, 
        fbe_packet_completion_context_t context);
fbe_status_t fbe_job_service_packet_validation_completion_function(fbe_packet_t *packet_p, 
        fbe_packet_completion_context_t context);
fbe_status_t fbe_job_service_packet_selection_completion_function(fbe_packet_t *packet_p, 
        fbe_packet_completion_context_t context);
fbe_status_t fbe_job_service_packet_update_in_memory_completion_function(fbe_packet_t *packet_p, 
        fbe_packet_completion_context_t context);
fbe_status_t fbe_job_service_packet_persist_completion_function(fbe_packet_t *packet_p, 
        fbe_packet_completion_context_t context);
fbe_status_t fbe_job_service_packet_commit_completion_function(fbe_packet_t *packet_p, 
        fbe_packet_completion_context_t context);
fbe_status_t fbe_job_service_packet_rollback_completion_function(fbe_packet_t *packet_p, 
        fbe_packet_completion_context_t context);

/*!*******************************************************************
 * @enum fbe_job_control_code_e
 *********************************************************************
 * @brief
 *  This is the set of control commands use to send usurper commands
 *  to the Job Service
 *
 *********************************************************************/
typedef enum fbe_job_service_control_code_e
{
    FBE_JOB_CONTROL_CODE_INVALID = 0,
    FBE_JOB_CONTROL_CODE_ADD_ELEMENT_TO_QUEUE_TEST, // this control code should be obsolete
    FBE_JOB_CONTROL_CODE_DRIVE_SWAP,
    FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE,
    FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY,
    FBE_JOB_CONTROL_CODE_LUN_CREATE,
    FBE_JOB_CONTROL_CODE_LUN_DESTROY,
    FBE_JOB_CONTROL_CODE_LUN_UPDATE,
    FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE,
    FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE,
    FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_CREATION_QUEUE,
    FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_CREATION_QUEUE,
    FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_CREATION_QUEUE,
    FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO,
    FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP,
    FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE,
    FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE,
    FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE,
    FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG,
    FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER,
    FBE_JOB_CONTROL_CODE_UPDATE_JOB_ELEMENTS_ON_PEER,
    /* this code triggers PVD creation, it does not have a payload.
       Job service will find the PVD to create and create them in a transaction */
    FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE, 
    FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE,
    FBE_JOB_CONTROL_CODE_TRIGGER_SYSTEM_DRIVE_COPY_BACK_IF_NEEDED,  /*not used anymore. Leave it here just to avoid disruptive upgrade*/
    FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE,
    FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT,
    FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO,
    FBE_JOB_CONTROL_CODE_CONNECT_DRIVE,
    FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID,

    /* mark a job in the execution queue that it has been finished successfully. So when this job runs, it will 
     * just send notification, rather than running the job logic again*/
    FBE_JOB_CONTROL_CODE_MARK_JOB_DONE,

    FBE_JOB_CONTROL_CODE_GET_STATE,

    FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG,
    FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO,
    FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE,
    FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS,
    FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA,
    FBE_JOB_CONTROL_CODE_ADD_DEBUG_HOOK,
    FBE_JOB_CONTROL_CODE_GET_DEBUG_HOOK,
    FBE_JOB_CONTROL_CODE_REMOVE_DEBUG_HOOK,
    FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED,
    FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE,
    FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL,
    FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN,
    FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE,
    FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL,
    FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN,

    /* !!!!!!!!!!!!!!!!! add new job type here !!!!!!!!!!!!!!!!!!!!!
     * When adding new recovery job, please remember to add set_default_value function
     * in job_service_set_default_job_command_values. This allows the normal running
     * of your job in sep ndu stage*/
    FBE_JOB_CONTROL_CODE_LAST    
} fbe_job_service_control_t;

typedef fbe_status_t (*job_action_function_t) (fbe_packet_t * packet_p);

/* This is a general purpose job callback function */
typedef void (* job_callback_function_t)(fbe_u64_t job_number, fbe_status_t status);

/*!*******************************************************************
 * @enum fbe_job_control_queue_type_t
 *********************************************************************
 * @brief
 *  This is the enum that defines what queue the job control will work
 *  on.
 *
 *********************************************************************/
typedef enum fbe_job_control_queue_type_e 
{
    FBE_JOB_INVALID_QUEUE  = 0,
    FBE_JOB_RECOVERY_QUEUE = 1, 
    FBE_JOB_CREATION_QUEUE = 2 

}fbe_job_control_queue_type_t;

/*!*******************************************************************
 * @enum fbe_job_queue_element_remove_flag_t
 *********************************************************************
 * @brief
 *  This is the enum that defines different states of queue element 
 *  remove action.
 *
 *********************************************************************/
typedef enum fbe_job_queue_element_remove_flag_e 
{
    FBE_JOB_QUEUE_ELEMENT_REMOVE_NO_ACTION              = 0,
    FBE_JOB_QUEUE_ELEMENT_REMOVE_FROM_RECOVERY_QUEUE    = 1, 
    FBE_JOB_QUEUE_ELEMENT_REMOVE_FROM_CREATION_QUEUE    = 2 

}fbe_job_queue_element_remove_flag_t;

/*!*******************************************************************
 * @enum fbe_job_queue_element_remove_flag_t
 *********************************************************************
 * @brief
 *  This is the enum that defines different states of queue element 
 *  sync action.
 *
 *********************************************************************/
typedef enum fbe_job_queue_element_sync_flag_e 
{
    FBE_JOB_QUEUE_ELEMENT_SYNC_NO_ACTION              = 0,
    FBE_JOB_QUEUE_ELEMENT_SYNC_FROM_RECOVERY_QUEUE    = 1, 
    FBE_JOB_QUEUE_ELEMENT_SYNC_FROM_CREATION_QUEUE    = 2 

}fbe_job_queue_element_sync_flag_t;

/*!*******************************************************************
 * @enum fbe_job_system_drive_copy_back_action_t
 *********************************************************************
 * @brief
 *  This is the enum that defines what should be done in the system drive copy
 *  back logic after verification stage
 *
 *********************************************************************/
typedef enum fbe_job_system_drive_copy_back_action_s
{
    FBE_JOB_SYSTEM_DRIVE_COPY_BACK_INVALID,
    FBE_JOB_SYSTEM_DRIVE_COPY_BACK_NO_ACTION,
    FBE_JOB_SYSTEM_DRIVE_COPY_BACK_RECONNECT_SYS_VD_PVD,
    FBE_JOB_SYSTEM_DRIVE_COPY_BACK_TRIGGER_COPY_BACK
}fbe_job_system_drive_copy_back_action_t;


/*!*******************************************************************
 * @struct fbe_job_action_type_s
 *********************************************************************
 * @brief
 *  This is the definition of a job control transaction
 *
 * This structure carries a set of callbacks that a consumer supplies
 * when it uses job service . This makes part of the operation structure
 * which is to be used by any user of the Job service
 *
 *********************************************************************/
typedef struct fbe_job_action_s
{
    job_action_function_t  validation_function;

    job_action_function_t  selection_function;

    job_action_function_t  update_in_memory_function;

    job_action_function_t  persist_function;

    job_action_function_t  rollback_function;

    job_action_function_t  commit_function;

} fbe_job_action_t;

/*!*******************************************************************
 * @struct fbe_job_service_operation_s
 *********************************************************************
 * @brief
 *  This is the definition of the job service operation which 
 *  represents the job type along with the associated set of function
 *  for the specific job type
 *
 *********************************************************************/
typedef struct fbe_job_service_operation_s
{
    fbe_job_type_t      job_type;
    fbe_job_action_t    action;

} fbe_job_service_operation_t;

/*!*******************************************************************
 * @struct fbe_job_control_object_s
 *********************************************************************
 * @brief
 *  This is the definition of the job control element, this represent
 *  an entry in the recovery or creation queue managed by the job service
 *
 *********************************************************************/
typedef struct fbe_job_queue_element_s
{
    /*! We make this first so it is easy to enqueue, dequeue since
     * we can simply cast the queue element to an object.
     */
    fbe_queue_element_t queue_element;

    /*! indicates where a queue elment should go */
    fbe_job_control_queue_type_t queue_type;

    /*! job type, used for queued job service requests
    */
    fbe_job_type_t job_type;

    /*! Object id of the sender/client of the job service
    */
    fbe_object_id_t object_id;

    /*! Class id of object being modified.
    */
    fbe_class_id_t  class_id;

    /*! pointer to a list of functions provided by job service clients
    */
    fbe_job_action_t job_action;

    /*! time stamp when a job request is processed thru the client's functions 
    */
    fbe_time_t timestamp;

    /*! state of element as it is processed thru the various
     * registration functions
     */
    fbe_job_action_state_t current_state;

     /*! previous state of element as it is processed thru the various
     * registration functions
     */
    fbe_job_action_state_t previous_state;

    /*! determines if a queue can process its elements or not
    */
    fbe_bool_t  queue_access;

    /*! Number of objects of specific queue
    */
    fbe_u32_t num_objects;

    /*! job id
    */
    fbe_u64_t job_number;

    fbe_packet_t *packet_p;

    fbe_status_t                    status;

    fbe_job_service_error_type_t    error_code;

    /*! Context represents the client's data
    */
    fbe_u32_t command_size;
    fbe_u8_t command_data[FBE_JOB_ELEMENT_CONTEXT_SIZE]; 

    /*! These two field are used for notifying peer if user want to wait for 
        RG/LUN create/destroy lifecycle ready/destroy before job finishes.
        So that Admin can update RG/LUN only depending on job notification.
    */
    fbe_bool_t          need_to_wait;        /*!< indicate if the user want to wait for create or destroy. */
    fbe_u32_t           timeout_msec;        /*!< indicate how long the user want to wait for create/destroy, otherwise timeout. */

    /*! local info, should not be used by peer */
    fbe_bool_t          local_job_data_valid;  /*!< indicate if local job context is valid */
    fbe_u8_t           *additional_job_data;   /*!< additional data needed for the job on local side */
}
fbe_job_queue_element_t;

/*!*******************************************************************
 * @struct fbe_job_control_object_s
 *********************************************************************
 * @brief
 *  This is the definition of the job control packet element, this 
 *  represents an entry in the arrival recovery or arrival 
 *  creation queue managed by the job service
 *
 *********************************************************************/
typedef struct fbe_job_queue_packet_s
{
    /*! We make this first so it is easy to enqueue, dequeue since 
     * we can simply cast the queue element to an object. 
     */
    fbe_queue_element_t queue_element;

    /*! Number of packets - Removed - Not used anywhere in the code. */

    /*! job arrival packet, used for queued job service arrival requests 
    */
    fbe_packet_t *packet_p;
}
fbe_job_queue_packet_element_t;

/*!*******************************************************************
 * @struct fbe_job_queue_element_remove_action_t
 *********************************************************************
 * @brief
 *  This is the struct that defines queue element remove action.
 *  It is a member of global variable block_in_progress. If a deferred 
 *  remove operation is needed, the struct will be used to refine the 
 *  job service synchronization between "ADDED" and "REMOVE" CMI message. 
 *
 *********************************************************************/
typedef struct fbe_job_queue_element_remove_action_s 
{
    fbe_job_queue_element_remove_flag_t     flag;
    fbe_u64_t                               job_number;
    fbe_object_id_t                         object_id;
    fbe_status_t                            status;     
    fbe_job_service_error_type_t            error_code;
    fbe_class_id_t                          class_id;
    fbe_job_queue_element_t                 job_element;
}fbe_job_queue_element_remove_action_t;

/*!*******************************************************************
 * @struct fbe_job_queue_element_remove_action_t
 *********************************************************************
 * @brief
 *  This is the struct that defines queue element remove action.
 *  It is a member of global variable block_in_progress. If a deferred 
 *  remove operation is needed, the struct will be used to refine the 
 *  job service synchronization between "ADDED" and "SYNC" CMI message. 
 *
 *********************************************************************/
typedef struct fbe_job_queue_element_sync_action_s 
{
    fbe_job_queue_element_sync_flag_t       flag;
    fbe_u64_t                               job_number;
    fbe_object_id_t                         object_id;
    fbe_status_t                            status;     
    fbe_job_service_error_type_t            error_code;
    fbe_class_id_t                          class_id;
    fbe_job_queue_element_t                 job_element;
}fbe_job_queue_element_sync_action_t;


extern fbe_status_t fbe_job_service_fill_lun_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_lun_destroy_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_lun_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_provision_drive_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_provision_drive_destroy_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);

extern fbe_status_t fbe_job_service_fill_raid_group_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_raid_group_destroy_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_system_power_save_change_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_raid_group_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_drive_swap_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_virtual_drive_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_update_spare_library_config_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_provision_drive_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_update_provision_drive_block_size_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_spare_config_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_update_db_on_peer_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_system_bg_service_control_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_ndu_commit_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_system_time_threshold_set_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_connect_drive_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_update_multi_pvds_pool_id_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p); 

void fbe_job_service_set_job_number_on_packet(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);

extern void fbe_job_service_set_job_number_on_lun_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_lun_destroy_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_lun_update_request(fbe_packet_t *packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_raid_group_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_raid_group_destroy_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_system_power_save_change_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern void fbe_job_service_set_job_number_on_raid_group_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_drive_swap_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_virtual_drive_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_provision_drive_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_update_provision_drive_block_size_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_spare_config_update_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_provision_drive_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_provision_drive_destroy_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_db_update_on_peer_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_system_bg_service_control_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern void fbe_job_service_set_job_number_on_ndu_commit_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_generate_and_send_notification(fbe_job_queue_element_t *request_p);
extern void fbe_job_service_set_job_number_on_drive_connect_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_write_to_event_log(fbe_job_queue_element_t *request_p);
extern fbe_database_state_t fbe_job_service_get_database_state(void);
extern void fbe_job_service_set_job_number_on_system_time_threshold_set_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern fbe_status_t fbe_job_service_get_object_lifecycle_state(fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state);
extern void fbe_job_service_set_job_number_on_update_multi_pvds_pool_id_request(fbe_packet_t *packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_update_spare_library_config_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_system_encryption_change_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern void fbe_job_service_set_job_number_on_system_encryption_change_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_encryption_pause_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern void fbe_job_service_set_job_number_on_encryption_pause_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_update_encryption_mode_request(fbe_packet_t * packet_p, 
                                                                        fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_update_encryption_mode_request(fbe_packet_t * packet_p, 
                                                                             fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_process_encryption_keys_request(fbe_packet_t * packet_p, 
                                                                        fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_process_encryption_keys_request(fbe_packet_t * packet_p, 
                                                                      fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_scrub_old_user_data_request(fbe_packet_t * packet_p, 
                                                                          fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_scrub_old_user_data_request(fbe_packet_t * packet_p, 
                                                                               fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_create_extent_pool_request(fbe_packet_t * packet_p, 
                                                                    fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_create_extent_pool_request(fbe_packet_t * packet_p, 
                                                                         fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_create_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                                    fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_create_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                                         fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_destroy_extent_pool_request(fbe_packet_t * packet_p, 
                                                                    fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_destroy_extent_pool_request(fbe_packet_t * packet_p, 
                                                                         fbe_job_queue_element_t *job_queue_element_p);
extern fbe_status_t fbe_job_service_fill_destroy_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                                    fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_destroy_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                                         fbe_job_queue_element_t *job_queue_element_p);

extern fbe_status_t fbe_job_service_get_peer_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state);
extern fbe_status_t fbe_job_service_wait_for_expected_lifecycle_state(fbe_object_id_t obj_id, 
                                                                      fbe_lifecycle_state_t expected_lifecycle_state, 
                                                                      fbe_u32_t timeout_ms);

extern fbe_status_t fbe_job_service_wait_for_expected_lifecycle_state_on_peer(fbe_object_id_t obj_id, 
                                                                              fbe_lifecycle_state_t expected_lifecycle_state, 
                                                                              fbe_u32_t timeout_ms);
extern fbe_status_t fbe_job_service_get_database_peer_state(fbe_database_state_t *peer_state);

extern fbe_status_t fbe_job_service_fill_pause_encryption_request(fbe_packet_t * packet_p, 
                                                           fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_pause_encryption_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p);

extern fbe_status_t fbe_job_service_fill_validate_database_request(fbe_packet_t * packet_p, 
                                                           fbe_job_queue_element_t *job_queue_element_p);
extern void fbe_job_service_set_job_number_on_validate_database_request(fbe_packet_t * packet_p, 
                                                                fbe_job_queue_element_t *job_queue_element_p);

/*!*******************************************************************
 * @struct fbe_job_service_test_command_data_s
 *********************************************************************
 * @brief
 *  This is the definition of the job control test, used for testing
 *  Job service functionality. 
 *********************************************************************/
typedef struct fbe_job_service_test_command_data_s
{
    /*! Object id of the sender/client of the job service, for testing
     * purposes only
    */
    fbe_object_id_t object_id;

}fbe_job_service_test_command_data_t;

fbe_status_t fbe_job_service_build_queue_element(fbe_job_queue_element_t *job_queue_element,
                                                 fbe_job_service_control_t job_control_code,
                                                 fbe_object_id_t object_id,
                                                 fbe_u8_t *command_data_p,
                                                 fbe_u32_t command_size);

void job_service_trace(fbe_trace_level_t trace_level,
        fbe_trace_message_id_t message_id,
        const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));

void job_service_trace_object(fbe_object_id_t object_id,
                       fbe_trace_level_t trace_level,
                       fbe_trace_message_id_t message_id,
                       const char * fmt, ...) __attribute__((format(__printf_func__,4,5)));

fbe_bool_t fbe_job_service_cmi_is_active(void);

typedef struct fbe_job_service_lun_create_s {
    /*! for validation */
    fbe_raid_group_type_t   raid_type;
    fbe_raid_group_number_t raid_group_id;

    /*! the following are the user specified parameters for the LUN */
    fbe_assigned_wwid_t          world_wide_name;      /*! World-Wide Name associated with the LUN */
    fbe_user_defined_lun_name_t  user_defined_name;    /*! User-Defined Name for the LUN */
    fbe_lun_number_t        lun_number;
    fbe_lba_t               capacity;  
    fbe_block_transport_placement_t placement; 
    fbe_lba_t               addroffset; /*! used for NDB */
    fbe_bool_t              ndb_b;
    fbe_bool_t              noinitialverify_b;
    fbe_u64_t               job_number;   
 

    /*! @todo: these are the intermediate data to be passed between job operation functions.
     * this is not the best place to put the intermediate data.*/
    fbe_object_id_t                 lun_object_id;
    fbe_database_transaction_id_t     transaction_id;
    fbe_object_id_t             rg_object_id;
    fbe_object_id_t             bvd_object_id;
    fbe_lba_t                   lun_imported_capacity;
    fbe_lba_t                   lun_exported_capacity; /* equals user capacity + bvd_edge_offset*/

    fbe_lba_t                   rg_stripe_size; /* rg stripe size.  it is used in calculating the padding for align lba */

    fbe_edge_index_t            rg_client_index; /* edge client index when attached an edge to rg */
    fbe_lba_t                   rg_block_offset; /* block offset when attached an edge to rg */
    /* block offset when attached an edge between BVD and LUN.
     * this offset may not be zero if align_lba is not zero.
     * this is the case where user wants to align lun lba with the stripe boundry for performance.
     * bvd_edge_offset = (rg_stripe_size - (align_lba % rg_stripe_size))*/
    fbe_lba_t                   bvd_edge_offset;

    //fbe_status_t                    fbe_status;         /* FBE return status from lun create call */
    //fbe_job_service_error_type_t    js_error_code;

    /* Job Service error code associated with FBE status; 
     * set to "no error" if FBE success status returned
     */

    /* add additional parameters here */
    fbe_time_t                  bind_time;
    fbe_bool_t                  user_private;
    fbe_bool_t                  is_system_lun; /*used for creating a system lun*/
    fbe_bool_t                  wait_ready;            /*!< indicate if the user want to wait for ready. */
    fbe_u32_t                   ready_timeout_msec;    /*!< indicate how long the user want to wait for ready, otherwise timeout. */
    fbe_bool_t                  export_lun_b;
}fbe_job_service_lun_create_t;

/*!*******************************************************************
 * @struct fbe_job_service_lun_destroy_t
 *********************************************************************
 * @brief
 *  This is the struct for FBE_JOB_CONTROL_CODE_LUN_DESTROY. 
 *********************************************************************/
typedef struct fbe_job_service_lun_destroy_s {
    /*! the following are the user specified parameters for the LUN */
    fbe_lun_number_t                lun_number;
    fbe_u64_t                       job_number;  

    /*! @todo: these are the intermediate data to be passed between job operation functions.
     * this is not the best place to put the intermediate data. */
    fbe_database_transaction_id_t     transaction_id;
    fbe_object_id_t                 lun_object_id;
    fbe_object_id_t                 bvd_object_id;
    fbe_bool_t                      allow_destroy_broken_lun;
    fbe_bool_t                      wait_destroy;                /*!< indicate if the user want to wait for destroy. */
    fbe_u32_t                       destroy_timeout_msec;        /*!< indicate how long the user want to wait for destroy, otherwise timeout. */

    //fbe_status_t                    fbe_status;         /* FBE return status from lun create call */
    //fbe_job_service_error_type_t    js_error_code;  
    /* Job Service error code associated with FBE status; 
     * set to "no error" if FBE success status returned
     */
} fbe_job_service_lun_destroy_t;


/*!*******************************************************************
 * @struct fbe_job_service_update_lun_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_SERVICE_UPDATE_LUN
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_lun_update_s
{
    fbe_u64_t                       job_number;         // job number.
    fbe_database_control_update_lun_t  update_lun;         // update lun 
} fbe_job_service_lun_update_t;

typedef struct fbe_job_service_bes_s
{
    fbe_u32_t bus;
    fbe_u32_t enclosure;
    fbe_u32_t slot;
} fbe_job_service_bes_t;

typedef struct fbe_job_service_be_s
{
    fbe_u32_t bus;
    fbe_u32_t enclosure;
} fbe_job_service_be_t;

/*! @enum fbe_job_service_tri_state_enum_t 
 *  
 *  @brief private Raid Group settings
 *
 */
typedef enum fbe_job_service_tri_state_e
{
    FBE_TRI_FALSE   = 0,

    FBE_TRI_TRUE    = 1,

    FBE_TRI_INVALID = 0xffffffff
}
fbe_job_service_tri_state_t;

/*!*******************************************************************
 * @struct fbe_job_service_raid_group_create_s
 *********************************************************************
 * @brief
 *  This is the definition of the job service Raid Group data structure
 *  which is visible to clients of the job service
 *
 *********************************************************************/
typedef struct fbe_job_service_raid_group_create_s
{
    fbe_raid_group_number_t  raid_group_id;         /*!< User provided id */
    fbe_u32_t                drive_count;           /*!< number of drives that make up the Raid Group */
    fbe_u64_t                power_saving_idle_time_in_seconds; /*!< how long to wait before starting to power save */
    fbe_u64_t                max_raid_latency_time_in_seconds; /*!< how long are we willing ot wait in sec for RG to become ready after power saving */
    fbe_raid_group_type_t    raid_type; /*!< indicates Raid Group type, i.e Raid 5, Raid 0, etc */
    //fbe_bool_t               explicit_removal;      /*!< Indicates to remove the Raid Group after removing the last lun in the Raid Group */
    fbe_job_service_tri_state_t   is_private;       /*!< defines raid group as private */ 
    fbe_bool_t               b_bandwidth;           /*!< Determines if the user selected bandwidth or iops */
    fbe_lba_t                capacity;              /*!< indicates specific capacity settings for Raid group */
    fbe_job_service_bes_t    disk_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< set of disks for raid group creation */
    fbe_block_transport_placement_t placement;      /*!< indicates specific placement algorithm that determineswhere to find free space */
    fbe_u64_t                job_number;            /*!< job number */
    fbe_bool_t               user_private;			/*!< Indicate the RG is a private one or not*/
    fbe_bool_t               is_system_rg;          /* Define raid group is a system RG*/
    fbe_object_id_t           rg_id;                /* If we create a system Raid group, we need to define the rg object id*/
    fbe_bool_t               wait_ready;            /*!< indicate if the user want to wait for ready. */
    fbe_u32_t                ready_timeout_msec;    /*!< indicate how long the user want to wait for ready, otherwise timeout. */
} fbe_job_service_raid_group_create_t;

/*!*******************************************************************
 * @struct fbe_job_service_raid_group_create_s
 *********************************************************************
 * @brief
 *  This is the definition of the job service Raid Group data structure
 *  internal to job service only
 *
 *********************************************************************/
typedef struct job_service_raid_group_create_s
{
    fbe_job_service_raid_group_create_t user_input;

    /* these are the intermediate data to be passed between job operation functions.
     * this is not the best place to put the intermediate data.
     */

    /* sep object id */
    fbe_object_id_t  raid_group_object_id;

    /* transaction id to be used to interact with the database service */
    fbe_database_transaction_id_t transaction_id;

    /* return status with error if any  */
    //fbe_status_t  status;

    /* if job creation finds an error, report it here */
    //fbe_job_service_error_type_t  error_code;

    /*! Raid Group's set of provision drive object ids*/
    fbe_object_id_t provision_drive_id_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /*! Raid Group's set of physical drive object ids*/
    fbe_object_id_t physical_drive_id_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /*! Raid Group's set of virtual drives */
    fbe_object_id_t virtual_drive_id_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /* edge client index when attached an edge */
    fbe_edge_index_t         client_index[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /* block offset when attached an edge */
    fbe_lba_t                block_offset[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; 

    /* array of exported capacities for the pre-configured provision drives */
    fbe_lba_t pvd_exported_capacity_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /* array of block edge geometry */
    fbe_block_edge_geometry_t block_edge_geometry;

    /*class id*/
    fbe_class_id_t class_id;

} job_service_raid_group_create_t;

/*!*******************************************************************
 * @struct fbe_job_service_raid_group_destroy_s
 *********************************************************************
 * @brief
 *  This is the struct for FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY. 
 *********************************************************************/
typedef struct fbe_job_service_raid_group_destroy_s {
    fbe_raid_group_number_t      raid_group_id;  /*!< User provided id */
    fbe_u64_t                    job_number;     /*!< job number */
    fbe_bool_t          allow_destroy_broken_rg; /* whether allow destroy a broken rg */
    fbe_bool_t                   wait_destroy;                /*!< indicate if the user want to wait for destroy. */
    fbe_u32_t                    destroy_timeout_msec;        /*!< indicate how long the user want to wait for destroy, otherwise timeout. */
}
fbe_job_service_raid_group_destroy_t;

/*!*******************************************************************
 * @struct fbe_job_service_raid_group_destroy_s
 *********************************************************************
 * @brief
 *  This is the struct for FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY. 
 *********************************************************************/
typedef struct job_service_raid_group_destroy_s {

    fbe_job_service_raid_group_destroy_t user_input;

    /* these are the intermediate data to be passed between job operation functions.
    */
    fbe_object_id_t              raid_group_object_id;           /*!< SEP specific raid group id */
    fbe_status_t                 status;                         /*!< return status with error if any  */
    fbe_object_id_t              virtual_drive_object_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< set of VD objects */
    fbe_object_id_t              raid_object_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH/2]; /*!< set of mirror RG objects */
    fbe_u32_t                    number_of_virtual_drive_objects;
    fbe_u32_t                    number_of_mirror_under_striper_objects; /*!< number of mirror under striper raid objects */
    fbe_bool_t                   virtual_drive_object_destroy_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_raid_group_type_t        raid_type;                     
    fbe_database_transaction_id_t  transaction_id;                 /*!< id for interfacing with config service */
}
job_service_raid_group_destroy_t;

typedef struct fbe_job_service_change_system_power_saving_info_s {
    fbe_database_transaction_id_t     transaction_id;

    fbe_system_power_saving_info_t  system_power_save_info;
    fbe_u64_t                       job_number;              
    fbe_job_service_error_type_t  	error_code;
}fbe_job_service_change_system_power_saving_info_t;

/*  Only append members at the end is allowed. DO NOT modify
*  the existing data members.*/
typedef struct fbe_job_service_update_raid_group_s
{
    fbe_object_id_t                         object_id;
    fbe_update_raid_type_t                  update_type;
    fbe_database_transaction_id_t           transaction_id;
    fbe_u64_t                               power_save_idle_time_in_sec;
    fbe_u64_t                               job_number;
	fbe_lba_t								new_rg_size;/*used for expansion*/
	fbe_lba_t								new_edge_size;/*used for expansion*/
}fbe_job_service_update_raid_group_t;


/*  Only append members at the end is allowed. DO NOT modify
*  the existing data members.*/
typedef struct fbe_job_service_update_encryption_mode_s
{
    fbe_object_id_t                         object_id;
    fbe_base_config_encryption_mode_t       encryption_mode;
    fbe_database_transaction_id_t           transaction_id;
    fbe_u64_t                               job_number;
	job_callback_function_t					job_callback; /* Not NULL if internal request */
}fbe_job_service_update_encryption_mode_t;

typedef struct fbe_job_service_pause_encryption_s
{
    fbe_object_id_t                         object_id;
    fbe_bool_t                              encryption_paused;
    fbe_job_service_error_type_t            error_code;
    fbe_database_transaction_id_t           transaction_id;
    fbe_u64_t                               job_number;
    job_callback_function_t                 job_callback; /* Not NULL if internal request */
}fbe_job_service_pause_encryption_t;

/*!*******************************************************************
 * @struct fbe_job_service_update_virtual_drive_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_SERVICE_UPDATE_VIRTUAL_DRIVE
 *  control code.
 *  Only append members at the end is allowed. DO NOT modify
 *  the existing data members.
 *********************************************************************/
typedef struct fbe_job_service_update_virtual_drive_s {
    fbe_object_id_t                                 object_id;          // VD object id for the drive which fails.
    fbe_update_vd_type_t                            update_type;        // Update type of the virtual drive configuration.
    fbe_virtual_drive_configuration_mode_t          configuration_mode; // configuration mode if swap needed to change.
    fbe_database_transaction_id_t                   transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                                       job_number;         // job number.
    /*!@todo fill the error code information with update virtual drive error codes. */
} fbe_job_service_update_virtual_drive_t;

/*!*******************************************************************
 * @struct fbe_job_service_update_provision_drive_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_SERVICE_UPDATE_PROVISION_DRIVE
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_update_provision_drive_s
{
    fbe_object_id_t                         object_id;          // pvd object id.
    fbe_update_pvd_type_t                   update_type;        // update type 
    fbe_provision_drive_config_type_t       config_type;        // configuration type of the pvd object.
    fbe_bool_t                              sniff_verify_state; //sniff verify enable flag.
    fbe_database_transaction_id_t           transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                               job_number;         // job number.
    fbe_u8_t                                opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX]; /* opaque_data for the spare stuff*/
    fbe_u32_t                               pool_id;
    fbe_lba_t                               configured_capacity;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;
    fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
} fbe_job_service_update_provision_drive_t;

/*!*******************************************************************
 * @struct fbe_job_service_update_provision_drive_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_SERVICE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_update_pvd_block_size_s
{
    fbe_object_id_t                         object_id;          // pvd object id.
    fbe_provision_drive_configured_physical_block_size_t block_size;
    fbe_database_transaction_id_t           transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                               job_number;         // job number.
} fbe_job_service_update_pvd_block_size_t;

/*!*******************************************************************
 * @struct fbe_job_service_provision_drive_configuration_s
 *********************************************************************
 * @brief
 *  It defines information about PVD that job service needs 
 *  for FBE_JOB_SERVICE_CREATE_PROVISION_DRIVE control code.
 *********************************************************************/
typedef struct fbe_job_service_provision_drive_configuration_s
{
    fbe_object_id_t                     object_id;	   /* store PVD object id during the job */		       
    fbe_provision_drive_config_type_t   config_type;
    fbe_lba_t configured_capacity;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;
    fbe_bool_t                          sniff_verify_state;    /* leave it in here for now */
    fbe_config_generation_t             generation_number;     /* leave it in here for now */
    fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];
} fbe_job_service_provision_drive_configuration_t;

/*!*******************************************************************
 * @struct fbe_job_service_create_provision_drive_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_SERVICE_CREATE_PROVISION_DRIVE
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_create_provision_drive_s
{
    fbe_database_transaction_id_t              transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                                  job_number;         // job number.    

    fbe_u32_t                                  request_size;
    fbe_job_service_provision_drive_configuration_t PVD_list[FBE_JOB_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS];
} fbe_job_service_create_provision_drive_t;


/*!*******************************************************************
 * @struct fbe_job_service_update_spare_config_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_SERVICE_UPDATE_SPARE_CONFIG
 *  control code.
 *  Only append members at the end is allowed. DO NOT modify
 *  the existing data members.
 *********************************************************************/
typedef struct fbe_job_service_update_spare_config_s
{
    fbe_database_update_spare_config_type_t          update_type;        // update type 
    fbe_system_spare_config_info_t                   system_spare_info;  // system spare configuration information to update.
    fbe_database_transaction_id_t                    transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                                        job_number;         // job number.
} fbe_job_service_update_spare_config_t;

/*!*******************************************************************
 * @struct fbe_job_service_update_db_on_peer_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_TYPE_UPDATE_DB_ON_PEER + FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER
 *  control code.
 *  Only append members at the end is allowed. DO NOT modify
 *  the existing data members.
 *********************************************************************/
typedef struct fbe_job_service_update_db_on_peer_s
{
    fbe_u64_t     job_number;         // job number.
} fbe_job_service_update_db_on_peer_t;

/*!*******************************************************************
 * @struct fbe_job_service_destroy_provision_drive_t
 *********************************************************************
 * @brief
 *  This is the struct for FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE. 
 *********************************************************************/
typedef struct fbe_job_service_destroy_provision_drive_s
{
    fbe_database_transaction_id_t              transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                                  job_number;         // job number.    

    fbe_object_id_t                            object_id;
} fbe_job_service_destroy_provision_drive_t;


/*!*******************************************************************
 * @struct fbe_job_service_control_system_bg_service_s
 *********************************************************************
 * @brief
 *  This is the struct for FBE_JOB_SERVICE_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE.
 *********************************************************************/
typedef struct fbe_job_service_control_system_bg_service_s {
    fbe_base_config_control_system_bg_service_t  system_bg_service;
    fbe_u64_t                       job_number;              
    fbe_job_service_error_type_t  	error_code;
}fbe_job_service_control_system_bg_service_t;

/*!*******************************************************************
 * @struct fbe_job_service_ndu_commit_s
 *********************************************************************
 * @brief
 *  It defines job service request for commiting NDU process.
 *  job type: FBE_JOB_TYPE_SEP_NDU_COMMIT
 *  Only append members at the end is allowed. DO NOT modify
 *  the existing data members.
 *********************************************************************/
typedef struct fbe_job_service_sep_ndu_commit_s
{
    fbe_u64_t     job_number;         // job number.
} fbe_job_service_sep_ndu_commit_t;

/*!*******************************************************************
 * @struct fbe_job_service_set_system_time_threshold_info_s
 *********************************************************************
 * @brief
 *  This is the struct for job service to set system time threshold info.
 *********************************************************************/

typedef struct fbe_job_service_set_system_time_threshold_info_s {
    fbe_database_transaction_id_t     transaction_id;

    fbe_system_time_threshold_info_t  system_time_threshold_info;
    fbe_u64_t                       job_number;              
    fbe_job_service_error_type_t  	error_code;
}fbe_job_service_set_system_time_threshold_info_t;


/*!*******************************************************************
 * @struct fbe_job_service_connect_drive_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_CONNECT_DRIVE
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_connect_drive_s
{
    fbe_database_transaction_id_t              transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                                  job_number;         // job number.    

    fbe_u32_t                                  request_size;
    fbe_object_id_t                        PVD_list[FBE_JOB_DATABASE_TRANSACTION_MAX_CREATE_OBJECTS];
} fbe_job_service_connect_drive_t;

/*!*******************************************************************
 * @struct fbe_job_service_update_multi_pvds_pool_id_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID
 *  control code.
 *********************************************************************/
#define MAX_UPDATE_PVD_POOL_ID  DATABASE_TRANSACTION_MAX_USER_ENTRY
/* The definition of update_pvd_pool_id_t and update_pvd_pool_id_list_t 
 * should be same with the structure update_pvd_pool_data_t and update_pvd_pool_data_list_t which are defined in fbe_api_provision_drive_interface.h */
typedef struct update_pvd_pool_id_s{
    fbe_u32_t          pvd_object_id;
    fbe_u32_t          pvd_pool_id;
} update_pvd_pool_id_t;

typedef struct update_pvd_pool_id_list_s {
    update_pvd_pool_id_t  pvd_pool_data[MAX_UPDATE_PVD_POOL_ID];
    fbe_u32_t               total_elements;
}update_pvd_pool_id_list_t;

typedef struct fbe_job_service_update_multi_pvds_pool_id_s 
{
    fbe_database_transaction_id_t              transaction_id;     // transaction id to interact with configuration service.
    fbe_u64_t                                  job_number;         // job number.    

    update_pvd_pool_id_list_t                pvd_pool_data_list;  
} fbe_job_service_update_multi_pvds_pool_id_t;

/*!*******************************************************************
 * @struct fbe_job_service_change_system_encryption_info_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_change_system_encryption_info_s {
    fbe_database_transaction_id_t     transaction_id;

    fbe_system_encryption_info_t  system_encryption_info;
    fbe_u64_t                       job_number;              
    fbe_job_service_error_type_t  	error_code;
}fbe_job_service_change_system_encryption_info_t;


typedef enum fbe_job_service_encryption_key_operation_e{
    FBE_JOB_SERVICE_ENCRYPTION_KEY_OPERATION_NULL,
    FBE_JOB_SERVICE_SETUP_ENCRYPTION_KEYS,
    FBE_JOB_SERVICE_UPDATE_ENCRYPTION_KEYS,
    FBE_JOB_SERVICE_DELETE_ENCRYPTION_KEYS,
    FBE_JOB_SERVICE_ENCRYPTION_KEY_OPERATION_INVALID,
}fbe_job_service_encryption_key_operation_t;

/*!*******************************************************************
 * @struct fbe_job_service_process_encryption_keys_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_process_encryption_keys_s
{
    fbe_database_transaction_id_t               transaction_id; /*!< transaction id to interact with configuration service*/
    fbe_job_service_encryption_key_operation_t  operation;  /*!< operation */
    fbe_u64_t                                   job_number; /*!< job id */
}fbe_job_service_process_encryption_keys_t;

/*!*******************************************************************
 * @struct fbe_job_service_scrub_old_user_data_s
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_scrub_old_user_data_s
{
    fbe_u64_t                                   job_number; /*!< job id */
}fbe_job_service_scrub_old_user_data_t;

/*!*******************************************************************
 * @struct fbe_job_service_add_debug_hook_t
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_ADD_DEBUG_HOOK
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_add_debug_hook_s
{
    fbe_job_service_debug_hook_t    debug_hook; /* The debug hook to set */
} fbe_job_service_add_debug_hook_t;

/*!*******************************************************************
 * @struct fbe_job_service_get_debug_hook_t
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_GET_DEBUG_HOOK
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_get_debug_hook_s
{
    fbe_job_service_debug_hook_t    debug_hook; /* The debug hook to get */
    fbe_bool_t                      b_hook_found;   /*! If the hook was found*/
} fbe_job_service_get_debug_hook_t;

/*!*******************************************************************
 * @struct fbe_job_service_remove_debug_hook_t
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_REMOVE_DEBUG_HOOK
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_remove_debug_hook_s
{
    fbe_job_service_debug_hook_t    debug_hook; /* The debug hook to remove */
    fbe_bool_t                      b_hook_found;   /*! If the hook was found*/
} fbe_job_service_remove_debug_hook_t;

/*!*******************************************************************
 * @struct fbe_job_service_validate_database_t
 *********************************************************************
 * @brief
 *  This is the struct for to validate the in-memory database against
 *  the on-disk database.
 *********************************************************************/

typedef struct fbe_job_service_validate_database_s {
    fbe_database_transaction_id_t       transaction_id;
    fbe_u64_t                           job_number; 
    fbe_database_validate_request_type_t validate_caller;
    fbe_database_validate_failure_action_t failure_action;             
    fbe_job_service_error_type_t  	    error_code;
}fbe_job_service_validate_database_t;

/*!*******************************************************************
 * @struct fbe_job_service_create_extent_pool_t
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_create_extent_pool_s
{
    fbe_object_id_t                         object_id;
    fbe_object_id_t                         metadata_lun_object_id;
    fbe_u32_t                               pool_id;
    fbe_u32_t                               drive_count;
    fbe_drive_type_t                        drive_type;
    fbe_database_transaction_id_t           transaction_id;
    fbe_u64_t                               job_number;
    job_callback_function_t					job_callback; /* Not NULL if internal request */
}fbe_job_service_create_extent_pool_t;

/*!*******************************************************************
 * @struct fbe_job_service_create_ext_pool_lun_t
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_create_ext_pool_lun_s
{
    fbe_object_id_t                         object_id;
    fbe_u32_t                               lun_id;
    fbe_u32_t                               pool_id;
    fbe_assigned_wwid_t                     world_wide_name;
    fbe_user_defined_lun_name_t             user_defined_name;
    fbe_lba_t                               capacity;
    fbe_database_transaction_id_t           transaction_id;
    fbe_u64_t                               job_number;
}fbe_job_service_create_ext_pool_lun_t;

/*!*******************************************************************
 * @struct fbe_job_service_destroy_extent_pool_t
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_destroy_extent_pool_s
{
    fbe_object_id_t                         object_id;
    fbe_object_id_t                         metadata_lun_object_id;
    fbe_u32_t                               pool_id;
    fbe_database_transaction_id_t           transaction_id;
    fbe_u64_t                               job_number;
}fbe_job_service_destroy_extent_pool_t;

/*!*******************************************************************
 * @struct fbe_job_service_destroy_ext_pool_lun_t
 *********************************************************************
 * @brief
 *  It defines job service request for FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN
 *  control code.
 *********************************************************************/
typedef struct fbe_job_service_destroy_ext_pool_lun_s
{
    fbe_object_id_t                         object_id;
    fbe_u32_t                               lun_id;
    fbe_u32_t                               pool_id;
    fbe_database_transaction_id_t           transaction_id;
    fbe_u64_t                               job_number;
}fbe_job_service_destroy_ext_pool_lun_t;

#endif /* FBE_JOB_SERVICE_H */


