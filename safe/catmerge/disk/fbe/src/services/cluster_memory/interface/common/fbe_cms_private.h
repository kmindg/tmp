#ifndef FBE_CMS_PRIVATE_H
#define FBE_CMS_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the cluster memory service.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe_base_object.h"
#include "fbe_cms.h"
#include "fbe/fbe_multicore_queue.h"


/*!*******************************************************************
 * @struct fbe_cms_t
 *********************************************************************
 * @brief
 *  This is the definition of the cluster memory service
 *
 *********************************************************************/
typedef struct fbe_cms_s {
    fbe_base_service_t        base_service;  /* base service must be the first */

    /************************************ 
     * cluster memory service private members *
     ************************************/
    

}fbe_cms_t;


/*cms_main.c*/
void cms_trace(fbe_trace_level_t trace_level,
	       const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));
void cms_env_event_callback(fbe_cms_env_event_t event_type);

/*cms_common.c*/
fbe_status_t cms_complete_packet(fbe_packet_t *packet, fbe_status_t packet_status);
fbe_status_t cms_get_payload(fbe_packet_t *packet, void **payload_struct, fbe_u32_t expected_payload_len);

fbe_status_t cms_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
													  fbe_payload_control_buffer_t buffer,
													  fbe_payload_control_buffer_length_t buffer_length,
													  fbe_service_id_t service_id,
													  fbe_package_id_t package_id);

fbe_status_t cms_send_packet_to_object(fbe_payload_control_operation_opcode_t control_code,
													  fbe_payload_control_buffer_t buffer,
													  fbe_payload_control_buffer_length_t buffer_length,
													  fbe_object_id_t object_id,
													  fbe_package_id_t package_id);

fbe_status_t fbe_cms_wait_for_multicore_queue_empty(fbe_multicore_queue_t *queue_ptr, fbe_u32_t sec_to_wait, fbe_u8_t *q_name);
fbe_bool_t fbe_cms_common_is_active_sp(void);


/*cms_run_q*/
typedef fbe_status_t (*fbe_cms_run_queue_completion_t)(void *context);

enum fbe_cms_run_queue_flags_e{
	FBE_CMS_RUN_QUEUE_FLAGS_NONE = 	0x00000000,
};

typedef fbe_u64_t fbe_cms_runs_queue_flags_t;/*to be sued with fbe_cms_run_queue_flags_e*/

typedef struct fbe_cms_run_queue_client_s{
	fbe_queue_element_t				queue_element;/*!!leave first for simplicity!!*/
	fbe_cms_run_queue_completion_t 	callback;
    void *							context;/*context for completion*/
	fbe_cms_runs_queue_flags_t		run_queue_flags;/*special user requests*/
}fbe_cms_run_queue_client_t;

typedef struct fbe_cms_async_conext_s{
	fbe_semaphore_t sem;
	fbe_status_t	completion_status;
}fbe_cms_async_conext_t;

fbe_status_t fbe_cms_run_queue_init(void);
fbe_status_t fbe_cms_run_queue_destroy(void);
fbe_status_t fbe_cms_run_queue_push(fbe_cms_run_queue_client_t *waiter_struct,
									fbe_cms_run_queue_completion_t callback,
									fbe_cpu_id_t cpu_id,
									void * context,
									fbe_cms_runs_queue_flags_t run_queue_flags);

/***********************************************
 * end file fbe_cms_private.h
 ***********************************************/
#endif /* end FBE_CMS_PRIVATE_H */

