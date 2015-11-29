#ifndef BGSL_SERVICE_H
#define BGSL_SERVICE_H

#include "fbe/bgsl_payload_control_operation.h"
/*
#define BGSL_CONTROL_CODE_SERVICE_SHIFT 16
*/

#define BGSL_SERVICE_ID_MASK  0xff 

typedef enum bgsl_service_id_e{
	BGSL_SERVICE_ID_INVALID,

	BGSL_SERVICE_ID_FIRST = 0xff00, /* Maximum 255 services */
	BGSL_SERVICE_ID_BASE_SERVICE,
	BGSL_SERVICE_ID_MEMORY,
	BGSL_SERVICE_ID_TOPOLOGY,
	BGSL_SERVICE_ID_NOTIFICATION,
	BGSL_SERVICE_ID_SIMULATOR,
	BGSL_SERVICE_ID_SERVICE_MANAGER,
	BGSL_SERVICE_ID_SCHEDULER,
	BGSL_SERVICE_ID_TERMINATOR,
	BGSL_SERVICE_ID_TRANSPORT,
	BGSL_SERVICE_ID_EVENT_LOG,
	BGSL_SERVICE_ID_DRIVE_CONFIGURATION,
	BGSL_SERVICE_ID_TRACE,

    // BGSL_Logical services.
    BGSL_L_SERVICE_ID_BGS,
    BGSL_SERVICE_ID_FLARE,

	BGSL_SERVICE_ID_LAST
}bgsl_service_id_t;

#define BGSL_SERVICE_CONTROL_CODE_INVALID_DEF(_service_id) \
	((BGSL_PAYLOAD_CONTROL_CODE_TYPE_SERVICE << BGSL_PAYLOAD_CONTROL_CODE_TYPE_SHIFT) | (_service_id << BGSL_PAYLOAD_CONTROL_CODE_ID_SHIFT))


typedef enum bgsl_base_service_control_code_e {
	BGSL_BASE_SERVICE_CONTROL_CODE_INVALID = BGSL_SERVICE_CONTROL_CODE_INVALID_DEF(BGSL_SERVICE_ID_BASE_SERVICE),
   	BGSL_BASE_SERVICE_CONTROL_CODE_INIT,
	BGSL_BASE_SERVICE_CONTROL_CODE_DESTROY,

	BGSL_BASE_SERVICE_CONTROL_CODE_GET_COMPOSITION,
	BGSL_BASE_SERVICE_CONTROL_CODE_GET_METHODS,

	BGSL_BASE_SERVICE_CONTROL_CODE_GET_TRACE_LEVEL,
	BGSL_BASE_SERVICE_CONTROL_CODE_SET_TRACE_LEVEL,

	BGSL_BASE_SERVICE_CONTROL_CODE_LAST
} bgsl_base_service_control_code_t;

enum bgsl_base_service_constants_e{
	BGSL_BASE_SERVICE_MAX_ELEMENTS = 100
};

/* control code BGSL_BASE_SERVICE_CONTROL_CODE_GET_COMPOSITION */
typedef struct bgsl_base_service_get_composition_s{
	bgsl_u32_t number_of_elements;
	bgsl_u32_t element_id_array[BGSL_BASE_SERVICE_MAX_ELEMENTS];
}bgsl_base_service_get_composition_t;

struct bgsl_packet_s;
typedef	bgsl_status_t (* bgsl_service_control_entry_t)(struct bgsl_packet_s * packet);
struct bgsl_io_packet_s;
typedef	bgsl_status_t (* bgsl_service_io_entry_t)(struct bgsl_packet_s * io_packet);

typedef const struct bgsl_const_service_info {
	const char * service_name;
	bgsl_service_id_t service_id;
} bgsl_const_service_info_t;

bgsl_status_t bgsl_get_service_by_id(bgsl_service_id_t service_id, bgsl_const_service_info_t ** pp_service_info);
bgsl_status_t bgsl_get_service_by_name(const char * service_name, bgsl_const_service_info_t ** pp_service_info);

#endif /* BGSL_SERVICE_H */
