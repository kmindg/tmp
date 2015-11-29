#ifndef BGSL_TRACE_INTERFACE_H
#define BGSL_TRACE_INTERFACE_H

#include "fbe/bgsl_service.h"
#include "fbe/bgsl_winddk.h"

#define BGSL_TRACE_FLAG_DEFAULT 0xFFFFFFFF

typedef bgsl_u32_t bgsl_trace_flags_t;

typedef enum bgsl_trace_level_e{
    BGSL_TRACE_LEVEL_INVALID,

    BGSL_TRACE_LEVEL_CRITICAL_ERROR,
    BGSL_TRACE_LEVEL_ERROR,  
    BGSL_TRACE_LEVEL_WARNING,
    BGSL_TRACE_LEVEL_INFO,
    BGSL_TRACE_LEVEL_DEBUG_LOW,
    BGSL_TRACE_LEVEL_DEBUG_MEDIUM,
    BGSL_TRACE_LEVEL_DEBUG_HIGH,

    BGSL_TRACE_LEVEL_LAST
}bgsl_trace_level_t;

typedef enum bgsl_trace_type_e {
    BGSL_TRACE_TYPE_INVALID = 0,
    BGSL_TRACE_TYPE_DEFAULT,
    BGSL_TRACE_TYPE_OBJECT,
    BGSL_TRACE_TYPE_SERVICE,
    BGSL_TRACE_TYPE_LIB,
    BGSL_TRACE_TYPE_LAST,

    /* Trace flags */
    BGSL_TRACE_TYPE_FLAG_INVALID,
    BGSL_TRACE_TYPE_MGMT_ATTRIBUTE,
    BGSL_TRACE_TYPE_FLAG_LAST
} bgsl_trace_type_t;

typedef enum bgsl_trace_ring_e {
    BGSL_TRACE_RING_INVALID = 0,
    BGSL_TRACE_RING_DEFAULT,
    BGSL_TRACE_RING_STARTUP,
    BGSL_TRACE_RING_LAST
} bgsl_trace_ring_t;

typedef enum bgsl_trace_message_id_e {
    BGSL_TRACE_MESSAGE_ID_INVALID        = 0x00000000,

    BGSL_TRACE_MESSAGE_ID_STARTUP_FIRST  = 0x00000001, /* This messages will go to TRC_K_START - startup ring buffer */


    BGSL_TRACE_MESSAGE_ID_STARTUP_LAST,

    BGSL_TRACE_MESSAGE_ID_STANDARD_FIRST                 = 0x00010000, /* This messages will go to TRC_K_STD - standard ("low-speed") ring buffer */

    BGSL_TRACE_MESSAGE_ID_CREATE_OBJECT                  = 0x00010010, /* object ID is a first attribute */
    BGSL_TRACE_MESSAGE_ID_DESTROY_OBJECT                 = 0x00010011, /* object ID is a first attribute */
    BGSL_TRACE_MESSAGE_ID_MGMT_STATE_CHANGE              = 0x00010020, /* object mgmt_state is a first attribute  */
    BGSL_TRACE_MESSAGE_ID_MGMT_STATE_INVALID             = 0x00010021, /* object mgmt_state is a first attribute  */

    BGSL_TRACE_MESSAGE_ID_DESTROY_NON_EMPTY_QUEUE        = 0x00010030, /* function name is a first argument */

    BGSL_TRACE_MESSAGE_ID_FUNCTION_ENTRY                 = 0x00010040, /* function name is a first argument */
    BGSL_TRACE_MESSAGE_ID_FUNCTION_FAILED                = 0x00010050, /* function name is a first argument */
    BGSL_TRACE_MESSAGE_ID_FUNCTION_EXIT                  = 0x00010060, /* function name is a first argument */

    BGSL_TRACE_MESSAGE_ID_BOARD_TYPE_UKNOWN              = 0x00010070, /* board type is a first argument */
    BGSL_TRACE_MESSAGE_ID_MGMT_COMPLETE_STATUS           = 0x00010080, /* packet status is a first argument */

    BGSL_TRACE_MESSAGE_ID_TRANSACTION_START              = 0x00010090, /* function name is a first argument */
    BGSL_TRACE_MESSAGE_ID_TRANSACTION_FAILED             = 0x000100A0, /* function name is a first argument */
    BGSL_TRACE_MESSAGE_ID_TRANSACTION_COMPLETE           = 0x000100B0, /* function name is a first argument */

    BGSL_TRACE_MESSAGE_ID_INFO                           = 0x000100C0, /* freestyle text */

    BGSL_TRACE_MESSAGE_ID_LCC_STATUS_CHANGE              = 0x000100D0, /* object ID is a first attribute */
    BGSL_TRACE_MESSAGE_ID_DRIVE_STATUS_CHANGE            = 0x000100E0, /* object ID is a first attribute */

    BGSL_TRACE_MESSAGE_ID_BYPASS_OPERATION               = 0x00010100,

    BGSL_TRACE_MESSAGE_ID_INVALID_IN_LEN                 = 0x00010110, /* in_len is a first argument */
    BGSL_TRACE_MESSAGE_ID_INVALID_LEN                    = 0x00010111, /* in_len is a first argument */
    BGSL_TRACE_MESSAGE_ID_INVALID_OUT_LEN                = 0x00010120, /* out_len is a first argument */
    BGSL_TRACE_MESSAGE_ID_INVALID_PARAMETER              = 0x00010130, /* parameter name is a first argument, value is a second */
    BGSL_TRACE_MESSAGE_ID_LCC_TYPE_UNKNOWN               = 0x00010140, /* LCC type is a first argument */
    BGSL_TRACE_MESSAGE_ID_PORT_DRIVER_TYPE_UNKNOWN       = 0x00010150, /* Port Driver type is a first argument */


    BGSL_TRACE_MESSAGE_ID_STANDARD_LAST,

    BGSL_TRACE_MESSAGE_ID_IO_FIRST                       = 0x00020000, /* This messages will go to  TRC_K_IO  - io high-speed ring buffer */
    BGSL_TRACE_MESSAGE_ID_IO_LAST,

    BGSL_TRACE_MESSAGE_ID_LAST
}bgsl_trace_message_id_t;

typedef enum bgsl_trace_control_code_e {
    BGSL_TRACE_CONTROL_CODE_INVALID = BGSL_SERVICE_CONTROL_CODE_INVALID_DEF(BGSL_SERVICE_ID_TRACE),
    BGSL_TRACE_CONTROL_CODE_GET_LEVEL,
    BGSL_TRACE_CONTROL_CODE_SET_LEVEL,
    BGSL_TRACE_CONTROL_CODE_GET_TRACE_FLAG,
    BGSL_TRACE_CONTROL_CODE_SET_TRACE_FLAG,
    BGSL_TRACE_CONTROL_CODE_LAST
} bgsl_trace_control_code_t;

typedef struct bgsl_trace_level_control_s {
    bgsl_trace_type_t      trace_type;
    bgsl_u32_t             bgsl_id;
    bgsl_trace_level_t     trace_level;
    bgsl_trace_flags_t     trace_flag;
} bgsl_trace_level_control_t;

const char * bgsl_trace_get_level_stamp(bgsl_trace_level_t trace_level);

#endif /* BGSL_TRACE_INTERFACE_H */

