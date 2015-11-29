#ifndef FBE_TRACE_INTERFACE_H
#define FBE_TRACE_INTERFACE_H

#include "fbe/fbe_service.h"
//#include "fbe/fbe_winddk.h"

#define FBE_TRACE_FLAG_DEFAULT 0xFFFFFFFF
#define MAX_KTRACE_BUFF    200

typedef fbe_u32_t fbe_trace_flags_t;

typedef enum fbe_trace_level_e{
	FBE_TRACE_LEVEL_INVALID,

	FBE_TRACE_LEVEL_CRITICAL_ERROR,
	FBE_TRACE_LEVEL_ERROR,	
	FBE_TRACE_LEVEL_WARNING,
	FBE_TRACE_LEVEL_INFO,
	FBE_TRACE_LEVEL_DEBUG_LOW,
	FBE_TRACE_LEVEL_DEBUG_MEDIUM,
	FBE_TRACE_LEVEL_DEBUG_HIGH,

	FBE_TRACE_LEVEL_LAST
}fbe_trace_level_t;

typedef enum fbe_trace_type_e {
	FBE_TRACE_TYPE_INVALID = 0,
	FBE_TRACE_TYPE_DEFAULT,
	FBE_TRACE_TYPE_OBJECT,
	FBE_TRACE_TYPE_SERVICE,
	FBE_TRACE_TYPE_LIB,
    FBE_TRACE_TYPE_LAST,

    /* Trace flags */
    FBE_TRACE_TYPE_FLAG_INVALID,
    FBE_TRACE_TYPE_MGMT_ATTRIBUTE,
    FBE_TRACE_TYPE_FLAG_LAST
} fbe_trace_type_t;

typedef enum fbe_trace_ring_e {
	FBE_TRACE_RING_INVALID = 0,
	FBE_TRACE_RING_DEFAULT,
	FBE_TRACE_RING_STARTUP,
    FBE_TRACE_RING_XOR_START,       /* Xor Start buffer for tracing sector errors */
    FBE_TRACE_RING_XOR,             /* Xor Default buffer for tracing sector errors */
    FBE_TRACE_RING_TRAFFIC,         /* Traffic buffer */
	FBE_TRACE_RING_LAST
} fbe_trace_ring_t;

typedef enum fbe_trace_message_id_e {
	FBE_TRACE_MESSAGE_ID_INVALID		= 0x00000000,

	FBE_TRACE_MESSAGE_ID_STARTUP_FIRST	= 0x00000001, /* This messages will go to TRC_K_START - startup ring buffer */


	FBE_TRACE_MESSAGE_ID_STARTUP_LAST,

	FBE_TRACE_MESSAGE_ID_STANDARD_FIRST					= 0x00010000, /* This messages will go to TRC_K_STD - standard ("low-speed") ring buffer */

	FBE_TRACE_MESSAGE_ID_CREATE_OBJECT					= 0x00010010, /* object ID is a first attribute */
	FBE_TRACE_MESSAGE_ID_DESTROY_OBJECT					= 0x00010011, /* object ID is a first attribute */
	FBE_TRACE_MESSAGE_ID_MGMT_STATE_CHANGE				= 0x00010020, /* object mgmt_state is a first attribute  */
	FBE_TRACE_MESSAGE_ID_MGMT_STATE_INVALID				= 0x00010021, /* object mgmt_state is a first attribute  */

	FBE_TRACE_MESSAGE_ID_DESTROY_NON_EMPTY_QUEUE		= 0x00010030, /* function name is a first argument */

	FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY					= 0x00010040, /* function name is a first argument */
	FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED				= 0x00010050, /* function name is a first argument */
	FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT					= 0x00010060, /* function name is a first argument */

	FBE_TRACE_MESSAGE_ID_BOARD_TYPE_UKNOWN				= 0x00010070, /* board type is a first argument */
	FBE_TRACE_MESSAGE_ID_MGMT_COMPLETE_STATUS			= 0x00010080, /* packet status is a first argument */

	FBE_TRACE_MESSAGE_ID_TRANSACTION_START				= 0x00010090, /* function name is a first argument */
	FBE_TRACE_MESSAGE_ID_TRANSACTION_FAILED				= 0x000100A0, /* function name is a first argument */
	FBE_TRACE_MESSAGE_ID_TRANSACTION_COMPLETE			= 0x000100B0, /* function name is a first argument */

	FBE_TRACE_MESSAGE_ID_INFO							= 0x000100C0, /* freestyle text */

	FBE_TRACE_MESSAGE_ID_LCC_STATUS_CHANGE				= 0x000100D0, /* object ID is a first attribute */
	FBE_TRACE_MESSAGE_ID_DRIVE_STATUS_CHANGE			= 0x000100E0, /* object ID is a first attribute */

	FBE_TRACE_MESSAGE_ID_BYPASS_OPERATION				= 0x00010100,

	FBE_TRACE_MESSAGE_ID_INVALID_IN_LEN	 				= 0x00010110, /* in_len is a first argument */
	FBE_TRACE_MESSAGE_ID_INVALID_LEN	 				= 0x00010111, /* in_len is a first argument */
	FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN 				= 0x00010120, /* out_len is a first argument */
	FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER 				= 0x00010130, /* parameter name is a first argument, value is a second */
    FBE_TRACE_MESSAGE_ID_LCC_TYPE_UNKNOWN				= 0x00010140, /* LCC type is a first argument */
    FBE_TRACE_MESSAGE_ID_PORT_DRIVER_TYPE_UNKNOWN		= 0x00010150, /* Port Driver type is a first argument */


	FBE_TRACE_MESSAGE_ID_STANDARD_LAST,

	FBE_TRACE_MESSAGE_ID_IO_FIRST						= 0x00020000, /* This messages will go to  TRC_K_IO  - io high-speed ring buffer */
	FBE_TRACE_MESSAGE_ID_IO_LAST,

	FBE_TRACE_MESSAGE_ID_LAST
}fbe_trace_message_id_t;

typedef enum fbe_trace_control_code_e {
	FBE_TRACE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_TRACE),
	FBE_TRACE_CONTROL_CODE_GET_LEVEL,
	FBE_TRACE_CONTROL_CODE_SET_LEVEL,
	FBE_TRACE_CONTROL_CODE_GET_ERROR_COUNTERS,
    FBE_TRACE_CONTROL_CODE_GET_TRACE_FLAG,
    FBE_TRACE_CONTROL_CODE_SET_TRACE_FLAG,
    FBE_TRACE_CONTROL_CODE_SET_ERROR_LIMIT, /*! Indicate what to do when we see various error thresholds. */
    FBE_TRACE_CONTROL_CODE_GET_ERROR_LIMIT, /*! Fetch our error thresholds. */
	FBE_TRACE_CONTROL_CODE_RESET_ERROR_COUNTERS,
    FBE_TRACE_CONTROL_CODE_TRACE_ERR_SET_NOTIFY_LEVEL,
	FBE_TRACE_CONTROL_CODE_SET_LIFECYCLE_DEBUG_TRACE_FLAG,
	FBE_TRACE_CONTROL_CODE_ENABLE_BACKTRACE,
	FBE_TRACE_CONTROL_CODE_DISABLE_BACKTRACE,
	FBE_TRACE_CONTROL_CODE_COMMAND_TO_KTRACE_BUFF,
	FBE_TRACE_CONTROL_CODE_CLEAR_ERROR_COUNTERS,

	FBE_TRACE_CONTROL_CODE_LAST
} fbe_trace_control_code_t;

/*! @enum fbe_lifecycle_state_debug_tracing_flags_t 
 *  
 *  @brief Debug flags that allow run-time lifecycle
 *  state debug tracing.
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 */
typedef enum fbe_lifecycle_state_debug_tracing_flags_e
{
    FBE_LIFECYCLE_DEBUG_FLAG_NONE                                = 0x00000000,

    /*! Log all sep objects lifecycle state.
     */
//    FBE_LIFECYCLE_DEBUG_FLAG_ALL_SEP_OBJECTS_TRACING             = 0x00000001,

    /*! Log all lifecycle state traces for a particular class.
     */
//    FBE_LIFECYCLE_DEBUG_FLAG_BASE_CONFIG_CLASS_TRACING          = 0x00000002, /* 0x49 */

	FBE_LIFECYCLE_DEBUG_FLAG_BVD_INTERFACE_CLASS_TRACING        = 0x00000001, /* 0x53 */ 

	FBE_LIFECYCLE_DEBUG_FLAG_LUN_CLASS_TRACING                  = 0x00000002, /* 0x52 */

	FBE_LIFECYCLE_DEBUG_FLAG_PARITY_CLASS_TRACING               = 0x00000004, /* 0x4e */

	FBE_LIFECYCLE_DEBUG_FLAG_STRIPER_CLASS_TRACING               = 0x00000008, /* 0x4d */

	FBE_LIFECYCLE_DEBUG_FLAG_MIRROR_CLASS_TRACING                = 0x00000010, /* 0x4c */

	FBE_LIFECYCLE_DEBUG_FLAG_VIRTUAL_DRIVE_CLASS_TRACING         = 0x00000020, /* 0x50 */

	FBE_LIFECYCLE_DEBUG_FLAG_PROVISION_DRIVE_CLASS_TRACING       = 0x00000040, /* 0x51 */

	/*! Log all sep objects lifecycle state.
     */
    FBE_LIFECYCLE_DEBUG_FLAG_ALL_SEP_OBJECTS_TRACING             = (FBE_LIFECYCLE_DEBUG_FLAG_BVD_INTERFACE_CLASS_TRACING ||   /* 0x7F */
																	FBE_LIFECYCLE_DEBUG_FLAG_LUN_CLASS_TRACING ||
																	FBE_LIFECYCLE_DEBUG_FLAG_PARITY_CLASS_TRACING ||
																	FBE_LIFECYCLE_DEBUG_FLAG_STRIPER_CLASS_TRACING ||
																	FBE_LIFECYCLE_DEBUG_FLAG_MIRROR_CLASS_TRACING ||
																	FBE_LIFECYCLE_DEBUG_FLAG_VIRTUAL_DRIVE_CLASS_TRACING ||
																	FBE_LIFECYCLE_DEBUG_FLAG_PROVISION_DRIVE_CLASS_TRACING),

    /*! Log all lifecycle state traces for a particular object.
     */
    FBE_LIFECYCLE_DEBUG_FLAG_OBJECT_TRACING                      = 0x00000200,

    FBE_LIFECYCLE_DEBUG_FLAG_LAST                                = 0x00000400,

} fbe_lifecycle_state_debug_tracing_flags_t;

typedef struct fbe_lifecycle_debug_trace_control_s {
    fbe_lifecycle_state_debug_tracing_flags_t     trace_flag;
} fbe_lifecycle_debug_trace_control_t;

typedef struct fbe_trace_level_control_s {
    fbe_trace_type_t      trace_type;
    fbe_u32_t             fbe_id;
    fbe_trace_level_t     trace_level;
    fbe_trace_flags_t     trace_flag;
} fbe_trace_level_control_t;

typedef struct fbe_trace_error_counters_s {
    fbe_u32_t	trace_error_counter;
    fbe_u32_t   trace_critical_error_counter;
} fbe_trace_error_counters_t;

/*!*******************************************************************
 * @enum fbe_trace_error_limit_action_t
 *********************************************************************
 * @brief This describes the type of action to take when we
 *        reach a given limit of error counts.
 *
 *********************************************************************/
typedef enum fbe_trace_error_limit_action_e {
	FBE_TRACE_ERROR_LIMIT_ACTION_INVALID = 0,

    /*! We simply trace when we reach this limit.
     */
    FBE_TRACE_ERROR_LIMIT_ACTION_TRACE,

    /*! Bring the system down if we see this limit.
     */
    FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM,
	FBE_TRACE_ERROR_LIMIT_ACTION_LAST
} fbe_trace_error_limit_action_t;

/*!*******************************************************************
 * @struct fbe_trace_error_limit_t
 *********************************************************************
 * @brief This structure is used as part of the 
 *        FBE_TRACE_CONTROL_CODE_SET_ERROR_limit
 *        usurper command to control what the system will do for
 *        a given trace level.
 *********************************************************************/
typedef struct fbe_trace_error_limit_s {

    /*! Trace level to set the limit for.
     */
    fbe_trace_level_t trace_level;

    /*! Action to take when we reach this limit.
     */
    fbe_trace_error_limit_action_t action;

    /*! The number of errors needed to reach this limit.
     */
    fbe_u32_t num_errors;

} fbe_trace_error_limit_t;

/*!*******************************************************************
 * @struct fbe_trace_get_error_limit_record_t
 *********************************************************************
 * @brief This structure is used as part of the 
 *        FBE_TRACE_CONTROL_CODE_GET_ERROR_limit usurper command
 *        to control what the system will do for a given trace level.
 *********************************************************************/
typedef struct fbe_trace_get_error_limit_record_s {

    /*! Action to take when we reach this limit.
     */
    fbe_trace_error_limit_action_t action;

    /*! The number of errors encountered at this level.
     */
    fbe_u32_t num_errors;

    /*! The number of errors needed to reach this limit.
     */
    fbe_u32_t error_limit;

} fbe_trace_get_error_limit_record_t;

/*!*******************************************************************
 * @struct fbe_trace_get_error_limit_t
 *********************************************************************
 * @brief This structure is used as part of the 
 *        FBE_TRACE_CONTROL_CODE_GET_ERROR_limit usurper command
 *        to control what the system will do for a given trace level.
 *********************************************************************/
typedef struct fbe_trace_get_error_limit_s {
    
    /*! The array of records, that describe the error limits 
     * for each trace level. 
     */ 
    fbe_trace_get_error_limit_record_t records[FBE_TRACE_LEVEL_LAST];

} fbe_trace_get_error_limit_t;

typedef struct fbe_trace_err_set_notify_level_s{
    /*! The trace level to send notification for fbe_err_sniff
     */
    fbe_trace_level_t level;
}fbe_trace_err_set_notify_level_t;

typedef struct fbe_trace_command_to_ktrace_buff_s {
   /*! Send fbe_cli command to ktrace start buffer 
     */
   fbe_u32_t length;
   fbe_char_t cmd[MAX_KTRACE_BUFF];

} fbe_trace_command_to_ktrace_buff_t;

const char * fbe_trace_get_level_stamp(fbe_trace_level_t trace_level);

#endif /* FBE_TRACE_INTERFACE_H */

