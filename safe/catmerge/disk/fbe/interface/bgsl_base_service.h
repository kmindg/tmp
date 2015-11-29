#ifndef BGSL_BASE_SERVICE_H
#define BGSL_BASE_SERVICE_H

#include "csx_ext.h"
#include "fbe/bgsl_types.h"
#include "bgsl_base.h"
#include "fbe/bgsl_service.h"
#include "fbe/bgsl_transport.h"
#include "bgsl_trace.h"

typedef struct bgsl_base_service_s{
	bgsl_base_t bgsl_base;
	bgsl_service_id_t service_id;
	bgsl_trace_level_t trace_level;
	bgsl_bool_t initialized;
} bgsl_base_service_t;

typedef struct bgsl_service_methods_s {
    bgsl_service_id_t  service_id;
	/*bgsl_status_t (* control_entry)(bgsl_packet_t * packet);*/
	bgsl_service_control_entry_t control_entry;
}bgsl_service_methods_t;

bgsl_status_t bgsl_base_service_set_service_id(bgsl_base_service_t * base_service, bgsl_service_id_t service_id);

bgsl_status_t bgsl_base_service_set_trace_level(bgsl_base_service_t * base_service, bgsl_trace_level_t trace_level);
bgsl_trace_level_t bgsl_base_service_get_trace_level(bgsl_base_service_t * base_service);

void bgsl_base_service_trace(bgsl_base_service_t * base_service, 
                            bgsl_trace_level_t trace_level,
                            bgsl_trace_message_id_t message_id,
                            const char * fmt, ...)
                            __attribute__((format(__printf_func__,4,5)));

bgsl_status_t bgsl_base_service_init(bgsl_base_service_t * base_service);

bgsl_status_t bgsl_base_service_destroy(bgsl_base_service_t * base_service);

bgsl_bool_t bgsl_base_service_is_initialized(bgsl_base_service_t * base_service);

bgsl_status_t bgsl_base_service_send_init_command(bgsl_service_id_t service_id);
bgsl_status_t bgsl_base_service_send_destroy_command(bgsl_service_id_t service_id);
bgsl_status_t bgsl_base_service_control_entry(bgsl_base_service_t * base_service, bgsl_packet_t * packet);

#endif /* BGSL_BASE_SERVICE_H */
