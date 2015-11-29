#ifndef FBE_SERVICE_H
#define FBE_SERVICE_H

#include "fbe/fbe_payload_control_operation.h"
//#include "fbe/fbe_trace_interface.h"
/*
#define FBE_CONTROL_CODE_SERVICE_SHIFT 16
*/

#define FBE_SERVICE_ID_MASK  0xff 

typedef enum fbe_service_id_e{
    FBE_SERVICE_ID_INVALID,

    FBE_SERVICE_ID_FIRST = 0xff00, /* Maximum 255 services */
    FBE_SERVICE_ID_BASE_SERVICE,
    FBE_SERVICE_ID_MEMORY,
    FBE_SERVICE_ID_TOPOLOGY,
    FBE_SERVICE_ID_NOTIFICATION,
    FBE_SERVICE_ID_SIMULATOR,
    FBE_SERVICE_ID_SERVICE_MANAGER,
    FBE_SERVICE_ID_SCHEDULER,
    FBE_SERVICE_ID_TERMINATOR,
    FBE_SERVICE_ID_TRANSPORT,
    FBE_SERVICE_ID_EVENT_LOG,
    FBE_SERVICE_ID_ENVIRONMENT_LIMIT,
    FBE_SERVICE_ID_DRIVE_CONFIGURATION,
    FBE_SERVICE_ID_RDGEN,
    FBE_SERVICE_ID_TRACE,
    FBE_SERVICE_ID_TRAFFIC_TRACE,
    FBE_SERVICE_ID_SECTOR_TRACE,
    FBE_SERVICE_ID_SIM_SERVER,
    FBE_SERVICE_ID_METADATA,
    FBE_SERVICE_ID_JOB_SERVICE,    
    FBE_SERVICE_ID_EVENT,    
    FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
    FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION,
    FBE_SERVICE_ID_CMI,
    FBE_SERVICE_ID_EIR,
    FBE_SERVICE_ID_PERSIST,
    FBE_SERVICE_ID_USER_SERVER,
    FBE_SERVICE_ID_DATABASE,
    FBE_SERVICE_ID_CMS,
    FBE_SERVICE_ID_RAW_MIRROR,
    FBE_SERVICE_ID_CMS_EXERCISER,
    FBE_SERVICE_ID_DEST,
    FBE_SERVICE_ID_PERFSTATS,
    FBE_SERVICE_ID_ENVSTATS,
    FBE_SERVICE_ID_LAST
}fbe_service_id_t;

#define FBE_SERVICE_CONTROL_CODE_INVALID_DEF(_service_id) \
    ((FBE_PAYLOAD_CONTROL_CODE_TYPE_SERVICE << FBE_PAYLOAD_CONTROL_CODE_TYPE_SHIFT) | (_service_id << FBE_PAYLOAD_CONTROL_CODE_ID_SHIFT))


typedef enum fbe_base_service_control_code_e {
    FBE_BASE_SERVICE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_BASE_SERVICE),
    FBE_BASE_SERVICE_CONTROL_CODE_INIT,
    FBE_BASE_SERVICE_CONTROL_CODE_DESTROY,

    FBE_BASE_SERVICE_CONTROL_CODE_GET_COMPOSITION,
    FBE_BASE_SERVICE_CONTROL_CODE_GET_METHODS,

    FBE_BASE_SERVICE_CONTROL_CODE_GET_TRACE_LEVEL,
    FBE_BASE_SERVICE_CONTROL_CODE_SET_TRACE_LEVEL,    

    FBE_BASE_SERVICE_CONTROL_CODE_USER_INDUCED_PANIC,

    FBE_BASE_SERVICE_CONTROL_CODE_LAST
} fbe_base_service_control_code_t;

enum fbe_base_service_constants_e{
    FBE_BASE_SERVICE_MAX_ELEMENTS = 100
};

/* control code FBE_BASE_SERVICE_CONTROL_CODE_GET_COMPOSITION */
typedef struct fbe_base_service_get_composition_s{
    fbe_u32_t number_of_elements;
    fbe_u32_t element_id_array[FBE_BASE_SERVICE_MAX_ELEMENTS];
}fbe_base_service_get_composition_t;

/* FBE_BASE_SERVICE_CONTROL_CODE_SET_TRACE_LEVEL */
typedef struct fbe_base_service_control_set_trace_level_s{
    fbe_u32_t trace_level;
}fbe_base_service_control_set_trace_level_t;

typedef const struct fbe_const_service_info {
    const char * service_name;
    fbe_service_id_t service_id;
} fbe_const_service_info_t;

struct fbe_packet_s;
#if defined(__cplusplus)
typedef	fbe_status_t (__stdcall * fbe_service_control_entry_t)(struct fbe_packet_s * packet);
#else
typedef	fbe_status_t (* fbe_service_control_entry_t)(struct fbe_packet_s * packet);
#endif

struct fbe_io_packet_s;
typedef	fbe_status_t (* fbe_service_io_entry_t)(struct fbe_packet_s * io_packet);

#endif /* FBE_SERVICE_H */
