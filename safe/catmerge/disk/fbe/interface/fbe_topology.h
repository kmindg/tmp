#ifndef FBE_TOPOLOGY_H
#define FBE_TOPOLOGY_H

#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe_base_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_topology_interface.h"

fbe_status_t fbe_topology_init_class_table(const fbe_class_methods_t * class_table[]);

fbe_status_t fbe_topology_get_class_table(const fbe_class_methods_t ** class_table[]);

fbe_bool_t fbe_topology_is_reference_count_zero(fbe_object_id_t id);
void fbe_topology_clear_gate(fbe_object_id_t id);

#if 0/*FBE3, sharel: moved outside so it's visible to the shim that needs to use new FBE API to set this entry
	this can be moved back here once we take out Flare*/
fbe_status_t fbe_topology_send_io_packet(fbe_packet_t * packet);
#endif /*FBE3*/
fbe_status_t fbe_topology_send_event(fbe_object_id_t object_id,
                                     fbe_event_type_t event_type,
                                     fbe_event_context_t event_context);

fbe_status_t 
fbe_topology_send_event_to_another_package(fbe_object_id_t object_id, 
										   fbe_package_id_t package_id, 
										   fbe_event_type_t event_type, 
										   fbe_event_context_t event_context);


void fbe_topology_class_trace(fbe_u32_t class_id,
                              fbe_trace_level_t trace_level,
                              fbe_u32_t message_id,
                              const fbe_char_t * fmt, ...)
                              __attribute__((format(__printf_func__,4,5)));

/* This function may be called by scheduler only */
fbe_status_t fbe_topology_send_monitor_packet(fbe_packet_t * packet);
fbe_status_t fbe_topology_class_id_to_object_type(fbe_class_id_t class_id, fbe_topology_object_type_t *object_type);


#endif /* FBE_TOPOLOGY_H */
