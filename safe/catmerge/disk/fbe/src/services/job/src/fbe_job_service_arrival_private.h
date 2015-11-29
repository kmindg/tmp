#ifndef FBE_JOB_SERVICE_ARRIVAL_PRIVATE_H
#define FBE_JOB_SERVICE_ARRIVAL_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_job_service_arrival_private.h
 ***************************************************************************
 *
 * @brief
 * 	This file contains local definitions for the job service
 * 	arrival queues
 *
 * @version
 *   08/23/2010:  Created. Jesus Medina
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
#include "fbe_job_service.h"
#include "fbe_job_service_arrival_thread.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_random.h"

/*! @def FBE_JOB_SERVICE_ARRIVAL_PACKET_TIMEOUT 
 *  
 *  @brief This is the number of seconds we will wait on a
 *         semaphore for a packet.  If it does not complete
 *         in 20 seconds, then something is wrong.
 */
#define FBE_JOB_SERVICE_ARRIVAL_PACKET_TIMEOUT  (-20 * (10000000L))

/*!*******************************************************************
 * @struct fbe_job_service_arrival_t
 *********************************************************************
 * @brief
 * 	This is the definition of the job service arrival
 * 	data structure
 *
 *********************************************************************/
typedef struct fbe_job_service_arrival_s
{
    /*! Lock to protect our arrival queues.
    */
    fbe_spinlock_t arrival_lock;

    fbe_queue_head_t arrival_recovery_q_head;

    fbe_queue_head_t arrival_creation_q_head;

    /*! Number of arrival recovery objects.
    */
    fbe_u32_t number_arrival_recovery_q_elements;

    /*! Number of arrival creation requests. 
    */
    fbe_u32_t number_arrival_creation_q_elements;

    /*! Used to wake up job control thread to access queues */
    fbe_semaphore_t job_service_arrival_queue_semaphore;

    fbe_thread_t job_service_arrival_thread_handle;

    fbe_job_service_arrival_thread_flags_t arrival_thread_flags;

    /*! process arrival queues or not
    */
    fbe_bool_t  arrival_recovery_queue_access;

    fbe_bool_t  arrival_creation_queue_access;

    /*! tracks outstanding arrival requests so we can shutdown cleanly */
    fbe_atomic_t outstanding_requests;
}
fbe_job_service_arrival_t;

//fbe_job_service_arrival_t fbe_job_service_arrival;


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* fbe_job_service_arrival_utilities.c
*/
extern fbe_status_t fbe_job_service_process_arrival_recovery_queue_access(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_process_arrival_creation_queue_access(fbe_packet_t * packet_p);

extern void fbe_job_service_get_packet_from_arrival_creation_queue(fbe_packet_t ** packet_p);
extern void fbe_job_service_get_packet_from_arrival_recovery_queue(fbe_packet_t ** packet_p);
fbe_status_t fbe_job_service_allocate_and_fill_creation_queue_element(fbe_packet_t * packet_p, fbe_job_queue_element_t ** job_queue_element_p, 
        fbe_job_type_t job_type, fbe_job_control_queue_type_t queue_type);
fbe_status_t fbe_job_service_allocate_and_fill_recovery_queue_element(fbe_packet_t * packet_p, fbe_job_queue_element_t ** job_queue_element_p, fbe_job_type_t job_type, 
        fbe_job_control_queue_type_t queue_type);
extern fbe_status_t fbe_job_service_enqueue_arrival_job_request(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_add_element_to_arrival_recovery_queue(fbe_packet_t * packet_p);
extern fbe_status_t fbe_job_service_add_element_to_arrival_creation_queue(fbe_packet_t * packet_p);


/* Accessors for the number of recovery objects.
*/
extern void fbe_job_service_increment_number_arrival_recovery_q_elements(void);
extern void fbe_job_service_decrement_number_arrival_recovery_q_elements(void);
extern fbe_u32_t fbe_job_service_get_number_arrival_recovery_q_elements(void);
extern void fbe_job_service_remove_all_objects_from_arrival_recovery_q(void);


/* Accessors for the number of creation objects.
*/
extern void fbe_job_service_increment_number_arrival_creation_q_elements(void);
extern void fbe_job_service_decrement_number_arrival_creation_q_elements(void);
extern fbe_u32_t fbe_job_service_get_number_arrival_creation_q_elements(void);
extern void fbe_job_service_remove_all_objects_from_arrival_creation_q(void);

/* Accessors to member fields of job service
*/
extern void fbe_job_service_init_number_arrival_recovery_q_elements(void);
extern void fbe_job_service_init_number_arrival_creation_q_elements(void);
extern void fbe_job_service_set_arrival_recovery_queue_access(fbe_bool_t value);
extern void fbe_job_service_set_arrival_creation_queue_access(fbe_bool_t value);
extern fbe_bool_t fbe_job_service_get_arrival_creation_queue_access(void);
extern fbe_bool_t fbe_job_service_get_arrival_recovery_queue_access(void);

/* job service queue access semaphore */
extern void fbe_job_service_init_arrival_semaphore(void);

/* job service queue client control */
//extern void fbe_job_service_init_control_process_call_semaphore();

/* job control accessors */
extern void fbe_job_service_arrival_enqueue_recovery_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_arrival_dequeue_recovery_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_arrival_enqueue_creation_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_arrival_dequeue_creation_q(fbe_job_queue_element_t *entry_p);
extern void fbe_job_service_get_arrival_recovery_queue_head(fbe_queue_head_t **queue_p);
extern void fbe_job_service_get_arrival_creation_queue_head(fbe_queue_head_t **queue_p);

/* fbe_job_service_arrival_thread.c
*/
extern void fbe_job_service_arrival_lock(void);
extern void fbe_job_service_arrival_unlock(void);

extern void job_service_drive_swap_set_default_command_values(fbe_u8_t * command_data);
extern void job_service_update_raid_group_set_default_command_values(fbe_u8_t * command_data);
extern void job_service_update_virtual_drive_set_default_command_values(fbe_u8_t * command_data);
extern void job_service_update_spare_config_set_default_command_values(fbe_u8_t * command_data);
extern void job_service_update_db_on_peer_set_default_command_values(fbe_u8_t * command_data);
extern void job_service_sep_ndu_commit_set_default_command_values(fbe_u8_t * command_data);

extern void job_service_set_default_job_command_values(fbe_u8_t * command_data, fbe_job_type_t job_type);


/********************************************
 * end file fbe_job_service_arrival_private.h
 *******************************************/

#endif /* end FBE_JOB_SERVICE_ARRIVAL_PRIVATE_H */




