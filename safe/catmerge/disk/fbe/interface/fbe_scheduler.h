#ifndef FBE_SCHEDULER_H
#define FBE_SCHEDULER_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_scheduler_interface.h"
#include "fbe_testability.h"

typedef enum fbe_scheduler_queue_type_e{
	FBE_SCHEDULER_QUEUE_TYPE_INVALID,

	FBE_SCHEDULER_QUEUE_TYPE_IDLE,
	FBE_SCHEDULER_QUEUE_TYPE_RUN,
	FBE_SCHEDULER_QUEUE_TYPE_PENDING,
	FBE_SCHEDULER_QUEUE_TYPE_DESTROYED,

	FBE_SCHEDULER_QUEUE_TYPE_LAST
}fbe_scheduler_queue_type_t;

typedef enum fbe_scheduler_state_flag_e{
	FBE_SCHEDULER_STATE_FLAG_RUN_REQUEST = 0x0001,
}fbe_scheduler_state_flag_t;

typedef fbe_u32_t fbe_scheduler_state_t;

typedef struct fbe_scheduler_element_s {
	fbe_queue_element_t			queue_element; /* MUST be first */
	fbe_packet_t			  * packet; /* When on pending_queue points to monitor packet */
	fbe_u32_t					time_counter;  /* Relative reverse timer in milliseconds when zero element moved to run_queue */
	fbe_scheduler_queue_type_t	current_queue; /* The current queue where the element located */
	fbe_scheduler_state_t		scheduler_state; /* Additional flags */
	fbe_u32_t					hook_counter;  /* number of hook installed for the object */
	fbe_scheduler_debug_hook_function_t hook; /* scheduler debug hook */
}fbe_scheduler_element_t;

fbe_status_t fbe_scheduler_register(fbe_scheduler_element_t * scheduler_element);
fbe_status_t fbe_scheduler_unregister(fbe_scheduler_element_t * scheduler_element);
fbe_status_t fbe_scheduler_run_request(fbe_scheduler_element_t * scheduler_element);
fbe_status_t fbe_scheduler_set_time_counter(fbe_scheduler_element_t * scheduler_element, fbe_u32_t time_counter);
fbe_status_t fbe_scheduler_request_credits(fbe_scheduler_credit_t *credits_to_use, fbe_u32_t core_number, fbe_bool_t *grant_status);
fbe_status_t fbe_scheduler_set_scale(fbe_atomic_t credits_scale);
fbe_status_t fbe_scheduler_return_credits(fbe_scheduler_credit_t *credits_to_return, fbe_u32_t core_number);
void fbe_scheduler_set_startup_hooks(fbe_scheduler_debug_hook_t *hooks); 
fbe_scheduler_hook_status_t fbe_scheduler_debug_hook(fbe_object_id_t object_id, fbe_u32_t monitor_state, fbe_u32_t monitor_substate, fbe_u64_t val1, fbe_u64_t val2);
void fbe_scheduler_get_control_mem_use_mb(fbe_u32_t *memory_use_mb_p);
void fbe_scheduler_start_to_monitor_background_service_enabled(void);
void fbe_scheduler_start_to_monitor_system_drive_queue_depth(void);
void fbe_scheduler_stop_monitoring_system_drive_queue_depth(void);

#endif /* FBE_SCHEDULER_H */
