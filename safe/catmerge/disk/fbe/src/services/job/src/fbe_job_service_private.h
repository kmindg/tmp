#ifndef FBE_JOB_SERVICE_PRIVATE_H
#define FBE_JOB_SERVICE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_job_service_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the job service.
 *
 * @version
 *   10/01/2009:  Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe_block_transport.h"
#include "fbe_job_service_arrival_private.h"
#include "fbe_job_service.h"
#include "fbe_job_service_thread.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_random.h"
#include "fbe_job_service_cmi.h"
#include "fbe/fbe_cmi_interface.h"


/*! @def FBE_JOB_SERVICE_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a
 *         semaphore for a packet.  If it does not complete
 *         in 20 seconds, then something is wrong.
 */
#define FBE_JOB_SERVICE_PACKET_TIMEOUT  (-20 * (10000000L))


enum fbe_job_service_gate_e{
    FBE_JOB_SERVICE_GATE_BIT     = 0x10000000,
    FBE_JOB_SERVICE_GATE_MASK    = 0x0FFFFFFF,
};

/*!*******************************************************************
 * @struct fbe_job_service_t
 *********************************************************************
 * @brief
 *  This is the definition of the job service
 *
 *********************************************************************/
typedef struct fbe_job_service_s
{
    fbe_base_service_t base_service;

    /*! Lock to protect our queues.
    */
    fbe_spinlock_t lock;

    fbe_queue_head_t recovery_q_head;

    fbe_queue_head_t creation_q_head;

    /*! Number of recovery objects.
    */
    fbe_u32_t number_recovery_objects;

    /*! Number of creation requests. 
    */
    fbe_u32_t number_creation_objects;

    /*! packet
    */
    /* packet will be a pointer and will be resused
     * thru all all the job action functions
     */
    fbe_packet_t *job_packet_p;

    /*! Used to wake up job control thread to access queues */
    fbe_semaphore_t job_control_queue_semaphore;

    /*! Used for process communication between job service and client of
     * the job service
     */ 
    fbe_semaphore_t job_control_process_semaphore;

    fbe_thread_t thread_handle;

    fbe_job_service_thread_flags_t thread_flags;

    fbe_u64_t job_number;

    /*! process queues or not
    */
    fbe_bool_t  recovery_queue_access;

    fbe_bool_t  creation_queue_access;

    fbe_job_service_arrival_t arrival;

    /*! used for arrival queues */
    fbe_job_service_cmi_message_t   *job_service_cmi_message_p;

    /*! used for execution queues */
    fbe_job_service_cmi_message_t   *job_service_cmi_message_exec_p;

	/*! helps us execute some logic when peer is dead */
	fbe_bool_t	peer_just_died;

    /*! helps us execute some logic when peer is dead */
    fbe_cmi_sp_state_t  sp_state_for_peer_lost_handle;     
}
fbe_job_service_t;

extern fbe_job_service_t fbe_job_service;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* fbe_job_service_main.c
*/
extern fbe_job_service_operation_t* fbe_job_service_find_operation(fbe_job_type_t job_type);

extern void fbe_job_service_print_objects_from_recovery_queue(fbe_u32_t caller);
extern void fbe_job_service_print_objects_from_recovery_queue_no_lock(fbe_u32_t caller);
extern fbe_status_t fbe_job_service_process_recovery_queue_access(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_process_creation_queue_access(fbe_packet_t * packet_p);

extern fbe_status_t fbe_job_service_find_and_remove_by_object_id_from_recovery_queue(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_process_peer_recovery_queue_completion(fbe_status_t in_status,     
                                                                           fbe_u64_t job_number, 
                                                                           fbe_object_id_t object_id,  
                                                                           fbe_job_service_error_type_t error_code,
                                                                           fbe_class_id_t class_id,
                                                                           fbe_job_queue_element_t *in_job_element_p);
extern fbe_status_t fbe_job_service_find_and_remove_element_by_job_number_from_recovery_queue(fbe_u64_t job_number);
extern fbe_status_t fbe_job_service_find_element_by_job_number_in_recovery_queue(fbe_packet_t * packet_p);
extern void fbe_job_service_remove_all_objects_from_recovery_queue(void);

extern fbe_status_t fbe_job_service_find_and_remove_by_object_id_from_creation_queue(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_find_and_remove_element_by_job_number_from_creation_queue(fbe_status_t in_status,     
                                                                            fbe_u64_t job_number, 
                                                                            fbe_object_id_t object_id,  
                                                                            fbe_job_service_error_type_t error_code,
                                                                            fbe_class_id_t class_id,
                                                                            fbe_job_queue_element_t *in_job_element_p);
extern fbe_status_t fbe_job_service_find_element_by_job_number_in_creation_queue(fbe_packet_t * packet_p);
extern void fbe_job_service_remove_all_objects_from_creation_queue(void);

extern fbe_status_t fbe_job_service_find_object_in_recovery_queue(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_find_object_in_creation_queue(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_get_number_of_elements_in_recovery_queue(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_get_number_of_elements_in_creation_queue(fbe_packet_t * packet_p);
extern void fbe_job_service_process_recovery_queue(void);
extern void fbe_job_service_process_creation_queue(void);
extern fbe_status_t fbe_job_service_enqueue_job_request(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_add_element_to_recovery_queue(fbe_job_queue_element_t *job_element_p);
extern fbe_status_t fbe_job_service_add_element_to_creation_queue(fbe_job_queue_element_t *job_element_p);
extern fbe_status_t fbe_job_service_get_state(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_mark_job_done(fbe_packet_t * packet_p);

extern fbe_status_t fbe_job_service_get_job_type(fbe_packet_t * packet_p, fbe_job_type_t *out_job_type);

extern fbe_job_type_t fbe_job_service_get_job_type_by_control_type(fbe_payload_control_operation_opcode_t in_control_code);

extern fbe_status_t fbe_job_service_translate_job_type(fbe_job_type_t job_type,
                                                const char ** pp_job_type_name);

extern fbe_status_t job_service_request_get_queue_type(fbe_packet_t *packet_p, fbe_job_control_queue_type_t *out_queue_type);

extern fbe_status_t fbe_job_service_find_service_operation(fbe_job_type_t job_type);
extern fbe_status_t fbe_job_service_lib_send_control_packet (
        fbe_payload_control_operation_opcode_t control_code,
        fbe_payload_control_buffer_t buffer,
        fbe_payload_control_buffer_length_t buffer_length,
        fbe_package_id_t package_id,
        fbe_service_id_t service_id,
        fbe_class_id_t class_id,
        fbe_object_id_t object_id,
        fbe_packet_attr_t attr);

/* Job service lock accessors.
*/
extern void fbe_job_service_lock(void);
extern void fbe_job_service_unlock(void);

extern void fbe_job_service_increment_job_number(void);
extern void fbe_job_service_set_job_number(fbe_u64_t);
extern fbe_u64_t fbe_job_service_increment_job_number_with_lock(void);
extern fbe_u64_t fbe_job_service_get_job_number(void);
extern void fbe_job_service_decrement_job_number_with_lock(void);
extern void fbe_job_service_decrement_job_number(void);

/* Accessors for the number of recovery objects.
*/
extern void fbe_job_service_increment_number_recovery_objects(void);
extern void fbe_job_service_decrement_number_recovery_objects(void);
extern fbe_u32_t fbe_job_service_get_number_recovery_requests(void);

/* Accessors for the number of creation objects.
*/
extern void fbe_job_service_increment_number_creation_objects(void);
extern void fbe_job_service_decrement_number_creation_objects(void);
extern fbe_u32_t fbe_job_service_get_number_creation_requests(void);

/* Accessors to member fields of job service
*/
extern void fbe_job_service_init_number_recovery_request(void);
extern void fbe_job_service_init_number_creation_request(void);
extern void fbe_job_service_set_recovery_queue_access(fbe_bool_t value);
extern void fbe_job_service_set_creation_queue_access(fbe_bool_t value);
extern fbe_bool_t fbe_job_service_get_creation_queue_access(void);
extern fbe_bool_t fbe_job_service_get_recovery_queue_access(void);
extern void fbe_job_service_handle_queue_access(fbe_bool_t queue_access);

/* job service queue access semaphore */
extern void fbe_job_service_init_control_queue_semaphore(void);

/* job service queue client control */
extern void fbe_job_service_init_control_process_call_semaphore(void);

/* job control accessors */
extern void fbe_job_service_enqueue_recovery_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_dequeue_recovery_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_enqueue_creation_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_dequeue_creation_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_get_recovery_queue_head(fbe_queue_head_t **queue_p);
extern void fbe_job_service_get_creation_queue_head(fbe_queue_head_t **queue_p);

/* Job service debug hook */
extern fbe_status_t fbe_job_service_debug_hook_init(void);
extern fbe_status_t fbe_job_service_debug_hook_destroy(void);
extern fbe_status_t fbe_job_service_hook_add_debug_hook(fbe_packet_t *packet_p);
extern fbe_status_t fbe_job_service_hook_get_debug_hook(fbe_packet_t *packet_p);
extern fbe_status_t fbe_job_service_hook_remove_debug_hook(fbe_packet_t *packet_p);
extern fbe_status_t fbe_job_service_hook_check_hook_and_take_action(fbe_job_queue_element_t *job_queue_element_p,
                                                                    fbe_bool_t b_state_start);

/************************************
 * end file fbe_job_service_private.h
 ************************************/

#endif /* end FBE_JOB_SERVICE_PRIVATE_H */




