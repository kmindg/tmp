#ifndef BASE_OBJECT_PRIVATE_H
#define BASE_OBJECT_PRIVATE_H

#include "fbe_base.h"
#include "fbe_trace.h"

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_object.h"
#include "fbe_base_object.h"

#include "fbe_event.h"
#include "fbe_lifecycle.h"

typedef struct fbe_base_object_s{
	fbe_base_t base;
	fbe_class_id_t		class_id;
	fbe_object_id_t		object_id;
	fbe_trace_level_t	trace_level;
	fbe_object_mgmt_attributes_t mgmt_attributes;
	fbe_spinlock_t		object_lock; /* This lock will protect the object members such as state , etc. */

	fbe_queue_head_t    terminator_queue_head; /* Every object should have terminator queue */
	fbe_spinlock_t		terminator_queue_lock;

	fbe_queue_head_t    usurper_queue_head;
	fbe_spinlock_t		usurper_queue_lock;
	fbe_atomic_t		userper_counter;

	fbe_u32_t           reschedule_count; /* Increment it only when the monitor does not exceed the 3sec limit */

	fbe_scheduler_element_t scheduler_element;
	fbe_physical_object_level_t physical_object_level; /* port 1 , first LCC 2, etc. */

	fbe_lifecycle_inst_base_t lifecycle; /* Special case */
	FBE_LIFECYCLE_DEF_INST_COND(FBE_BASE_OBJECT_LIFECYCLE_COND_MAX);
}fbe_base_object_t;

/* Methods */
fbe_status_t fbe_base_object_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_object_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_object_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_object_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_base_object_monitor_load_verify(void);


void fbe_base_object_release_object(fbe_base_object_t * base_object);
fbe_status_t fbe_base_object_get_object_id(fbe_base_object_t * object, fbe_object_id_t * object_id);

__forceinline static fbe_class_id_t 
fbe_base_object_get_class_id(fbe_object_handle_t handle)
{
	fbe_base_object_t * base_object;

#if 0
	if(FBE_COMPONENT_TYPE_OBJECT != fbe_base_get_component_type(handle)){
		return FBE_CLASS_ID_INVALID;
	}
#endif

	base_object = (fbe_base_object_t *)fbe_base_handle_to_pointer(handle);
	return base_object->class_id;
}

fbe_status_t fbe_base_object_get_lifecycle_state(fbe_base_object_t * base_object, fbe_lifecycle_state_t *lifecycle_state);

fbe_status_t fbe_base_object_increment_userper_counter(fbe_object_handle_t handle);
fbe_status_t fbe_base_object_decrement_userper_counter(fbe_object_handle_t handle);

/* This function should be called from the xxx_create_object function ONLY */
fbe_status_t fbe_base_object_set_class_id(fbe_base_object_t * base_object, fbe_class_id_t class_id);

/* This function should be called when object in specialized state and wants to "morph" into different object */
fbe_status_t fbe_base_object_change_class_id(fbe_base_object_t * base_object, fbe_class_id_t class_id);


/* Sends packet to child. Object MUST be connected */
fbe_status_t fbe_base_object_init_physical_object_level(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_status_t fbe_base_object_get_physical_object_level(fbe_base_object_t * base_object, fbe_physical_object_level_t * physical_object_level);
fbe_status_t fbe_base_object_set_physical_object_level(fbe_base_object_t * base_object, fbe_physical_object_level_t physical_object_level);

fbe_status_t fbe_base_object_traverse_packet(fbe_base_object_t * base_object, fbe_packet_t * packet);

/* This lock will protect the object members such as state , etc. */
void fbe_base_object_lock(fbe_base_object_t * base_object);
void fbe_base_object_unlock(fbe_base_object_t * base_object);

fbe_status_t fbe_base_object_run_request(fbe_base_object_t * base_object);
fbe_status_t fbe_base_object_reschedule_request(fbe_base_object_t * base_object, fbe_u32_t msecs);

fbe_status_t fbe_base_object_add_to_terminator_queue(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_status_t fbe_base_object_remove_from_terminator_queue(fbe_base_object_t * base_object, fbe_packet_t * packet);

fbe_status_t fbe_base_object_add_to_usurper_queue(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_status_t fbe_base_object_remove_from_usurper_queue(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_status_t fbe_base_object_usurper_queue_drain_io(fbe_lifecycle_const_t * p_class_const, fbe_base_object_t * base_object);
fbe_status_t fbe_base_object_find_from_usurper_queue(fbe_base_object_t* base_object, fbe_packet_t** reinit_packet_p);
fbe_status_t fbe_base_object_find_control_op_from_usurper_queue(fbe_base_object_t* base_object, fbe_packet_t** packet_p, 
																fbe_payload_control_operation_opcode_t control_op);
													 

fbe_status_t fbe_base_object_push_to_pending_queue(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_status_t fbe_base_object_pop_from_pending_queue(fbe_base_object_t * base_object, fbe_packet_t ** packet);

fbe_status_t fbe_base_object_packet_cancel_function(fbe_packet_completion_context_t context);

fbe_status_t fbe_base_object_set_trace_level(fbe_base_object_t * base_object, fbe_trace_level_t trace_level);
fbe_trace_level_t fbe_base_object_get_trace_level(fbe_base_object_t * base_object);

fbe_status_t fbe_base_object_set_trace_flags(fbe_base_object_t * base_object, fbe_trace_flags_t trace_level);
fbe_trace_level_t fbe_base_object_get_trace_flags(fbe_base_object_t * base_object);

void fbe_base_object_trace(fbe_base_object_t * base_object, 
                           fbe_trace_level_t trace_level,
                           fbe_u32_t message_id,
                           const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,4,5)));

void fbe_base_object_trace_at_startup(fbe_base_object_t * base_object, 
                                      fbe_trace_level_t trace_level,
                                      fbe_u32_t message_id,
                                      const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,4,5)));

void
fbe_base_object_customizable_trace(fbe_base_object_t * base_object, 
                      fbe_trace_level_t trace_level,
                      fbe_u8_t *header,
                      const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,4,5)));


fbe_status_t fbe_base_object_get_mgmt_attributes(fbe_base_object_t * base_object, fbe_object_mgmt_attributes_t * mgmt_attributes);
fbe_status_t fbe_base_object_set_mgmt_attributes(fbe_base_object_t * base_object, fbe_object_mgmt_attributes_t mgmt_attributes);

fbe_bool_t fbe_base_object_is_terminator_queue_empty(fbe_base_object_t * base_object);

fbe_status_t fbe_base_object_get_event(fbe_base_object_t * base_object, fbe_event_type_t *event_type);
fbe_status_t fbe_base_object_set_event(fbe_base_object_t * base_object, fbe_event_type_t event_type);

fbe_status_t fbe_base_object_terminator_queue_lock(fbe_base_object_t * base_object);
fbe_status_t fbe_base_object_terminator_queue_unlock(fbe_base_object_t * base_object);
fbe_queue_head_t * fbe_base_object_get_terminator_queue_head(fbe_base_object_t * base_object);

fbe_status_t fbe_base_object_usurper_queue_lock(fbe_base_object_t * base_object);
fbe_status_t fbe_base_object_usurper_queue_unlock(fbe_base_object_t * base_object);
fbe_queue_head_t * fbe_base_object_get_usurper_queue_head(fbe_base_object_t * base_object);


fbe_status_t fbe_base_object_destroy_request(fbe_base_object_t * base_object);
fbe_base_object_t *fbe_base_object_scheduler_element_to_base_object(fbe_scheduler_element_t * scheduler_element);

fbe_status_t fbe_base_object_check_hook(fbe_base_object_t * base_object,
                                        fbe_u32_t monitor_state,
                                        fbe_u32_t monitor_substate,
                                        fbe_u64_t val2,
                                        fbe_scheduler_hook_status_t *status);

fbe_status_t fbe_base_object_enable_lifecycle_debug_trace(fbe_base_object_t * object_p);

#endif /* BASE_OBJECT_PRIVATE_H */
