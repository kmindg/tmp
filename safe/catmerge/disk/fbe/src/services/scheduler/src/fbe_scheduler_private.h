#ifndef FBE_SCHEDULER_INTERFACE_PRIVATE_H
#define FBE_SCHEDULER_INTERFACE_PRIVATE_H

#include "csx_ext.h"
#include "fbe/fbe_scheduler_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe_testability.h"

static fbe_scheduler_debug_hook_t CSX_MAYBE_UNUSED scheduler_debug_hooks[MAX_SCHEDULER_DEBUG_HOOKS];

fbe_status_t fbe_scheduler_credit_init(void);
fbe_status_t fbe_scheduler_credit_destroy(void);
fbe_status_t fbe_scheduler_remove_credits_per_core(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_add_credits_per_core(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_remove_credits_all_core_cores(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_add_credits_all_cores(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_add_memory_credits(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_remove_memory_credits(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_set_credits(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_get_credits(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_set_scale_usurper(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_get_scale_usurper(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_set_debug_hook(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_get_debug_hook(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_del_debug_hook(fbe_packet_t *packet);
fbe_status_t fbe_scheduler_clear_debug_hooks(fbe_packet_t *packet);
void scheduler_trace(fbe_trace_level_t trace_level, fbe_trace_message_id_t message_id, const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));
void scheduler_credit_reload_table(void);
void scheduler_credit_reset_master_memory(void);


#endif /* FBE_SCHEDULER_INTERFACE_PRIVATE_H*/


	
