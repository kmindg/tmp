#include "fbe_base_transport.h"
#include "fbe_lifecycle.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"

void 
fbe_base_transport_trace(fbe_trace_level_t trace_level,
                         fbe_u32_t message_id,
                         const fbe_char_t * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list argList;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
	/* DLL support
    if (fbe_base_service_is_initialized(&transport_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&transport_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
	*/
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(argList, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_TRANSPORT,
                     trace_level,
                     message_id,
                     fmt, 
                     argList);
    va_end(argList);
}

