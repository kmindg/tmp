#ifndef FBE_OBJECT_H
#define FBE_OBJECT_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_payload_control_operation.h"
#include "fbe/fbe_trace_interface.h"

/*
#define FBE_CONTROL_CODE_CLASS_MASK 0x0000FFFF
#define FBE_CONTROL_CODE_CLASS_SHIFT 16
*/

/*
 * These are the canonical lifecycle states for an object.
 * The sequence of these values is significant (because they are widely used as indexes).
 */
typedef enum fbe_lifecycle_state_e {
	FBE_LIFECYCLE_STATE_SPECIALIZE = 0,       /* Object is specializing by type. */
	FBE_LIFECYCLE_STATE_ACTIVATE,             /* Object is activating itself. */
	FBE_LIFECYCLE_STATE_READY,                /* Object is in its normal mode. */
	FBE_LIFECYCLE_STATE_HIBERNATE,            /* Object is in power saving mode. */
	FBE_LIFECYCLE_STATE_OFFLINE,              /* Object is offline. */
	FBE_LIFECYCLE_STATE_FAIL,                 /* Object has a physical failure. */
	FBE_LIFECYCLE_STATE_DESTROY,              /* Object is destroying itself. */
	FBE_LIFECYCLE_STATE_NON_PENDING_MAX,  
	FBE_LIFECYCLE_STATE_PENDING_READY = FBE_LIFECYCLE_STATE_NON_PENDING_MAX,
	FBE_LIFECYCLE_STATE_PENDING_ACTIVATE,
	FBE_LIFECYCLE_STATE_PENDING_HIBERNATE,
	FBE_LIFECYCLE_STATE_PENDING_OFFLINE,
	FBE_LIFECYCLE_STATE_PENDING_FAIL,
	FBE_LIFECYCLE_STATE_PENDING_DESTROY,
	FBE_LIFECYCLE_STATE_PENDING_MAX,
	FBE_LIFECYCLE_STATE_NOT_EXIST = FBE_LIFECYCLE_STATE_PENDING_MAX, /* Object does not exist. */
	FBE_LIFECYCLE_STATE_INVALID /* Invalid state for an object. */
}fbe_lifecycle_state_t;

#define FBE_OBJECT_CONTROL_CODE_INVALID_DEF(_class_id) \
	((FBE_PAYLOAD_CONTROL_CODE_TYPE_CLASS << FBE_PAYLOAD_CONTROL_CODE_TYPE_SHIFT) | (_class_id << FBE_PAYLOAD_CONTROL_CODE_ID_SHIFT))

/* Control codes */
typedef enum fbe_base_object_control_code_e {
	FBE_BASE_OBJECT_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_OBJECT), /* FBE_CLASS_ID_BASE_OBJECT << FBE_CONTROL_CODE_CLASS_SHIFT, */
	FBE_BASE_OBJECT_CONTROL_CODE_MONITOR,	

	FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,

	FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
	FBE_BASE_OBJECT_CONTROL_CODE_LOG_LIFECYCLE_TRACE,

	FBE_BASE_OBJECT_CONTROL_CODE_SET_ACTIVATE_COND,
	FBE_BASE_OBJECT_CONTROL_CODE_SET_HIBERNATE_COND,
	FBE_BASE_OBJECT_CONTROL_CODE_SET_OFFLINE_COND,
	FBE_BASE_OBJECT_CONTROL_CODE_SET_FAIL_COND,
	FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
	FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_LEVEL,
	FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_LEVEL,
	FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_FLAG,
	FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_FLAG,

	FBE_BASE_OBJECT_CONTROL_CODE_SET_DEBUG_HOOK,
	FBE_BASE_OBJECT_CONTROL_CODE_CLEAR_DEBUG_HOOK,

    FBE_BASE_OBJECT_CONTROL_CODE_LAST
}fbe_base_object_control_code_t;

/* FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID */
typedef struct fbe_base_object_mgmt_get_class_id_s{
	fbe_class_id_t class_id;
}fbe_base_object_mgmt_get_class_id_t;

/* FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE */
typedef struct fbe_base_object_mgmt_get_lifecycle_state_s {
	fbe_lifecycle_state_t lifecycle_state;
} fbe_base_object_mgmt_get_lifecycle_state_t;

typedef fbe_u32_t fbe_object_death_reason_t;
#define FBE_DEATH_REASON_INVALID 0xFFFFFFFF
#define FBE_DEATH_CODE_TYPE_SHIFT 	FBE_PAYLOAD_CONTROL_CODE_TYPE_SHIFT
#define FBE_DEATH_CODE_ID_SHIFT		FBE_PAYLOAD_CONTROL_CODE_ID_SHIFT

#define FBE_OBJECT_DEATH_CODE_INVALID_DEF(_class_id) \
	((FBE_PAYLOAD_CONTROL_CODE_TYPE_CLASS << FBE_DEATH_CODE_TYPE_SHIFT) | (_class_id << FBE_DEATH_CODE_ID_SHIFT))

typedef enum fbe_base_discovered_death_reason_e{
	FBE_BASE_DISCOVERED_DEATH_REASON_INVALID = FBE_OBJECT_DEATH_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_DISCOVERED),
	FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED,
	FBE_BASE_DISCOVERED_DEATH_REASON_HARDWARE_NOT_READY,

	FBE_BASE_DISCOVERED_DEATH_REASON_LAST
}fbe_base_discovered_death_reason_t;

typedef struct fbe_lifecycle_timer_info_t {
    fbe_u32_t lifecycle_condition;
    fbe_u32_t interval;
} fbe_lifecycle_timer_info_t;

#endif /* FBE_OBJECT_H */
