#ifndef FBE_JOB_SERVICE_CMI_H
#define FBE_JOB_SERVICE_CMI_H

#include "fbe/fbe_types.h"
#include "fbe_job_service.h"

typedef enum fbe_job_service_cmi_state_e
{
    FBE_JOB_SERVICE_CMI_STATE_INVALID              =  0x00,  
    FBE_JOB_SERVICE_CMI_STATE_IDLE                 =  0x01, 
    FBE_JOB_SERVICE_CMI_STATE_IN_PROGRESS          =  0x02,  
    FBE_JOB_SERVICE_CMI_STATE_COMPLETED            =  0x03,
    FBE_JOB_SERVICE_CMI_STATE_COMPLETED_WITH_ERROR =  0x04
}fbe_job_service_cmi_state_t;

typedef struct fbe_job_service_cmi_block_in_progress_s {
	fbe_u64_t                     job_number;
    fbe_job_control_queue_type_t  queue_type;
	fbe_job_queue_element_t       *block_from_other_side;
    fbe_job_service_cmi_state_t   job_service_cmi_state;
    fbe_job_queue_element_remove_action_t   job_queue_element_remove_action;
}fbe_job_service_cmi_block_in_progress_t;

typedef enum fbe_job_service_cmi_message_type_e {
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_INVALID,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_CREATION_QUEUE,
	FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_CREATION_QUEUE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_CREATION_QUEUE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_CREATION_QUEUE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_RECOVERY_QUEUE,
	FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_RECOVERY_QUEUE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_PEER_JOB_QUEUE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_JOB_QUEUE_DONE,
    FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_SYNC_JOB_STATE, /*sync the state of executing job to peer*/
}fbe_job_service_cmi_message_type_t;

typedef struct fbe_job_service_cmi_element_s{
    fbe_job_queue_element_t job_element;
}fbe_job_service_cmi_element_t;

typedef struct fbe_job_service_cmi_message_s {
    fbe_job_control_queue_type_t    queue_type;

    /*! @note Do NOT include fbe_job_service_info_t since that will create an 
     *        implied dependancy with any changes to the fbe_job_service_info_t
     *        structure.  All CMI messages should be self sufficient.
     */
    fbe_status_t                    status;
    fbe_u64_t                       job_number;
    fbe_object_id_t                 object_id;
    fbe_job_service_error_type_t    error_code;
    fbe_class_id_t                  class_id;

    /* CMI required fields.
     */
    //fbe_u64_t job_service_element_receiver; /* If not NULL should point to the memory of the receiver */
    //fbe_u64_t job_service_element_sender; /* Should point to the memory of the sender */

    fbe_job_service_cmi_message_type_t job_service_cmi_message_type;

    union {
        fbe_job_service_cmi_element_t job_service_queue_element;

    }msg;

	/* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
	fbe_semaphore_t sem;

}fbe_job_service_cmi_message_t;

extern void fbe_job_service_cmi_lock(void);
extern void fbe_job_service_cmi_unlock(void);

fbe_status_t fbe_job_service_cmi_process_remove_request_for_creation_queue(fbe_status_t status,     
                                                                           fbe_u64_t job_number, 
                                                                           fbe_object_id_t object_id,
                                                                           fbe_job_service_error_type_t error_code,
                                                                           fbe_class_id_t class_id,
                                                                           fbe_job_queue_element_t *job_element_p);

fbe_status_t fbe_job_service_cmi_process_remove_request_for_recovery_queue(fbe_status_t status,     
                                                                           fbe_u64_t job_number, 
                                                                           fbe_object_id_t object_id,  
                                                                           fbe_job_service_error_type_t error_code,
                                                                           fbe_class_id_t class_id,
                                                                           fbe_job_queue_element_t *job_element_p);

void fbe_job_service_cmi_print_contents_from_queue_element(fbe_job_queue_element_t * queue_element);

fbe_job_service_cmi_state_t fbe_job_service_cmi_get_state(void);

fbe_u64_t fbe_job_service_cmi_get_job_number_from_block_in_progress(void);

fbe_status_t fbe_job_service_cmi_set_state_on_block_in_progress(fbe_job_service_cmi_state_t state);

fbe_bool_t fbe_job_service_cmi_is_block_from_other_side_valid(void);

void fbe_job_service_cmi_set_block_in_progress_block_from_other_side(fbe_job_queue_element_t *queue_element);

void fbe_job_service_cmi_copy_block_from_other_side(fbe_job_queue_element_t *job_queue_element);

void fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side(void);

fbe_packet_t * fbe_job_service_cmi_get_packet_from_queue_element(void);

fbe_job_queue_element_t * fbe_job_service_cmi_get_queue_element_from_block_in_progress(void);

void fbe_job_service_cmi_invalidate_block_from_other_side(void);

fbe_status_t fbe_job_service_cmi_translate_message_type(fbe_job_type_t message_type,
        const char ** pp_message_type_name);

fbe_status_t fbe_job_service_cmi_init(void);

fbe_status_t fbe_job_service_cmi_destroy(void);

fbe_status_t fbe_job_service_cmi_send_message(fbe_job_service_cmi_message_t * job_service_cmi_message, fbe_packet_t * packet);

void fbe_job_service_cmi_set_queue_type_on_block_in_progress(fbe_job_control_queue_type_t queue_type);

fbe_status_t fbe_job_service_cmi_get_queue_type_from_block_in_progress(fbe_job_control_queue_type_t *queue_type);

void fbe_job_service_cmi_copy_job_queue_element_to_block_from_other_side(fbe_job_queue_element_t *job_queue_element);

void fbe_job_service_cmi_complete_job_from_other_side(fbe_u64_t job_number);

void fbe_job_service_cmi_init_block_from_other_side(void);

fbe_status_t fbe_job_service_cmi_get_job_elements_from_peer(void);

void fbe_job_service_cmi_update_peer_job_queue(fbe_job_queue_element_t *fbe_job_element_p);

fbe_status_t fbe_job_service_cmi_update_job_queue_completion(void);

fbe_status_t fbe_job_service_sync_job_state_to_peer_before_commit(fbe_job_queue_element_t *job_queue_element_p);

fbe_status_t fbe_job_service_cmi_get_remove_job_action_on_block_in_progress
                                        (fbe_job_queue_element_remove_action_t *remove_action);

void fbe_job_service_cmi_set_remove_job_action_on_block_in_progress(
                                        fbe_job_queue_element_remove_flag_t     in_flag,
                                        fbe_u64_t                               in_job_number,
                                        fbe_object_id_t                         in_object_id,
                                        fbe_status_t                            in_status,     
                                        fbe_job_service_error_type_t            in_error_code,
                                        fbe_class_id_t                          in_class_id,
                                        fbe_job_queue_element_t                 *in_job_element_p);

void fbe_job_service_cmi_cleanup_remove_job_action_on_block_in_progress(void);

#endif /* FBE_JOB_SERVICE_CMI_H */


