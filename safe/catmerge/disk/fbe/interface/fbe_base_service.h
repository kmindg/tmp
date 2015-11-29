#ifndef FBE_BASE_SERVICE_H
#define FBE_BASE_SERVICE_H

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe_base.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe_trace.h"

typedef struct fbe_base_service_s{
	fbe_base_t fbe_base;
	fbe_service_id_t service_id;
	fbe_trace_level_t trace_level;
	fbe_bool_t initialized;
} fbe_base_service_t;

typedef struct fbe_service_methods_s {
    fbe_service_id_t  service_id;
	/*fbe_status_t (* control_entry)(fbe_packet_t * packet);*/
	fbe_service_control_entry_t control_entry;
}fbe_service_methods_t;

fbe_status_t fbe_base_service_set_service_id(fbe_base_service_t * base_service, fbe_service_id_t service_id);

fbe_status_t fbe_base_service_set_trace_level(fbe_base_service_t * base_service, fbe_trace_level_t trace_level);
fbe_trace_level_t fbe_base_service_get_trace_level(fbe_base_service_t * base_service);

void fbe_base_service_trace(fbe_base_service_t * base_service, 
                            fbe_trace_level_t trace_level,
                            fbe_trace_message_id_t message_id,
                            const fbe_char_t * fmt, ...)
                            __attribute__((format(__printf_func__,4,5)));

fbe_status_t fbe_base_service_init(fbe_base_service_t * base_service);

fbe_status_t fbe_base_service_destroy(fbe_base_service_t * base_service);

fbe_bool_t fbe_base_service_is_initialized(fbe_base_service_t * base_service);

fbe_status_t fbe_base_service_send_init_command(fbe_service_id_t service_id);
fbe_status_t fbe_base_service_send_destroy_command(fbe_service_id_t service_id);
fbe_status_t fbe_base_service_control_entry(fbe_base_service_t * base_service, fbe_packet_t * packet);

#endif /* FBE_BASE_SERVICE_H */
