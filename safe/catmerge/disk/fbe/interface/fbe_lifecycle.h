#ifndef FBE_LIFECYCLE_STATE_H
#define FBE_LIFECYCLE_STATE_H

#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_time.h"

/*!
 * \ingroup FBE_LIFECYCLE_STATE
 *\{*/

typedef fbe_u32_t fbe_lifecycle_cond_id_t;
typedef fbe_u32_t fbe_lifecycle_canary_t;
typedef fbe_u32_t fbe_lifecycle_timer_msec_t;
typedef fbe_u32_t fbe_lifecycle_func_cond_type_t;
typedef fbe_u32_t fbe_lifecycle_inst_base_attr_t;
typedef fbe_u32_t fbe_lifecycle_const_cond_attr_t;
typedef fbe_u32_t fbe_lifecycle_rotary_cond_attr_t;
typedef fbe_u32_t fbe_lifecycle_state_type_t;
typedef fbe_u32_t fbe_lifecycle_trace_type_t;
typedef fbe_u16_t fbe_lifecycle_trace_flags_t;

#define FBE_LIFECYCLE_COND_MASK ((fbe_u32_t)0x0000FFFF)
#define FBE_LIFECYCLE_COND_CLASS_SHIFT 16
#define FBE_LIFECYCLE_COND_FIRST_ID(class_id) ((class_id) << (FBE_LIFECYCLE_COND_CLASS_SHIFT))
#define FBE_LIFECYCLE_COND_MAX(cond_id) ((cond_id) & (FBE_LIFECYCLE_COND_MASK))

#define FBE_LIFECYCLE_NULL_FUNC NULL
#define FBE_LIFECYCLE_INVALID_COND  ((fbe_u32_t)0xffffffff)

#define FBE_LIFECYCLE_CANARY_CONST                  ((fbe_lifecycle_canary_t)0xeee00001) /* fbe_lifecycle_const_t */
#define FBE_LIFECYCLE_CANARY_INST                   ((fbe_lifecycle_canary_t)0xeee10001) /* fbe_lifecycle_inst_t */
#define FBE_LIFECYCLE_CANARY_CONST_BASE             ((fbe_lifecycle_canary_t)0xeee20001) /* fbe_lifecycle_const_base_t */
#define FBE_LIFECYCLE_CANARY_INST_BASE              ((fbe_lifecycle_canary_t)0xeee30001) /* fbe_lifecycle_inst_base_t */
#define FBE_LIFECYCLE_CANARY_CONST_COND             ((fbe_lifecycle_canary_t)0xeee40001) /* fbe_lifecycle_const_cond_t */
#define FBE_LIFECYCLE_CANARY_CONST_BASE_COND        ((fbe_lifecycle_canary_t)0xeee50001) /* fbe_lifecycle_const_base_cond_t */
#define FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND  ((fbe_lifecycle_canary_t)0xeee60001) /* fbe_lifecycle_const_base_cond_t */
#define FBE_LIFECYCLE_CANARY_CONST_BASE_COND_ARRAY  ((fbe_lifecycle_canary_t)0xeee70001) /* fbe_lifecycle_const_base_cond_array_t */
#define FBE_LIFECYCLE_CANARY_CONST_ROTARY_ARRAY     ((fbe_lifecycle_canary_t)0xeee80001) /* fbe_lifecycle_const_rotary_array_t */
#define FBE_LIFECYCLE_CANARY_DEAD                   ((fbe_lifecycle_canary_t)0xffffffff)

#define FBE_LIFECYCLE_COND_MAX_TIMER          ((fbe_u32_t)0xffffffff)
#define FBE_LIFECYCLE_COND_TYPE_BASE          ((fbe_lifecycle_func_cond_type_t)0) /*!< A condition local to the class. */
#define FBE_LIFECYCLE_COND_TYPE_DERIVED       ((fbe_lifecycle_func_cond_type_t)1) /*!< An override for an inherited condition. */
#define FBE_LIFECYCLE_COND_TYPE_BASE_TIMER    ((fbe_lifecycle_func_cond_type_t)2) /*!< A condition local to the class. */
#define FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER ((fbe_lifecycle_func_cond_type_t)3) /*!< An override for an inherited condition. */

#define FBE_LIFECYCLE_TIMER_RESCHED_NORMAL   ((fbe_lifecycle_timer_msec_t)3000)
#define FBE_LIFECYCLE_TIMER_RESCHED_MAX      ((fbe_lifecycle_timer_msec_t)3000)

typedef enum fbe_lifecycle_status_e {
    FBE_LIFECYCLE_STATUS_DONE = 0,
    FBE_LIFECYCLE_STATUS_PENDING,
    FBE_LIFECYCLE_STATUS_RESCHEDULE,
	FBE_LIFECYCLE_STATUS_CONTINUE
} fbe_lifecycle_status_t;

struct fbe_base_object_s;

/*!
 * \fn fbe_lifecycle_status_t (*fbe_lifecycle_func_online_t)(struct fbe_base_object_s * p_object,
 *                                                           struct fbe_packet_s * p_packet);
 *
 * \brief This function runs the class specific online monitor.
 *
 * \param p_object
 *        Pointer to the object or service.
 * \param p_packet
 *        Pointer to monitor control packet.
 */
typedef fbe_lifecycle_status_t (*fbe_lifecycle_func_online_t)(struct fbe_base_object_s * p_object,
                                                              struct fbe_packet_s * p_packet);

/*!
 * \fn fbe_lifecycle_status_t (*fbe_lifecycle_func_pending_t)(struct fbe_base_object_s * p_object,
 *                                                            struct fbe_packet_s * p_packet);
 *
 * \brief This function runs the class specific pending monitor.
 *
 * \param p_object
 *        Pointer to the object or service.
 * \param p_packet
 *        Pointer to monitor control packet.
 */
typedef fbe_lifecycle_status_t (*fbe_lifecycle_func_pending_t)(struct fbe_base_object_s * p_object,
                                                               struct fbe_packet_s * p_packet);

/*!
 * \fn fbe_lifecycle_status_t (*fbe_lifecycle_func_cond_t)(struct fbe_base_object_s * p_object,
 *                                                         struct fbe_packet_s * p_packet);
 *
 * \brief This function runs the class specific condition handler.
 *
 * \param p_object
 *        Pointer to the object or service.
 * \param p_packet
 *        Pointer to monitor control packet.
 */
typedef fbe_lifecycle_status_t (*fbe_lifecycle_func_cond_t)(struct fbe_base_object_s * p_object,
                                                            struct fbe_packet_s * p_packet);

/*!
 * \fn struct fbe_lifecycle_inst_s * (*fbe_lifecycle_get_inst_data_t)(struct fbe_base_object_s * p_object);
 *
 * \brief This function returns the lifecycle instance data for the object class.
 *
 * \param p_object
 *        Pointer to the object.
 */
typedef struct fbe_lifecycle_inst_s * (*fbe_lifecycle_get_inst_data_t)(struct fbe_base_object_s * p_object);

/*!
 * \fn struct fbe_lifecycle_inst_cond_s * (*fbe_lifecycle_get_inst_cond_t)(struct fbe_base_object_s * p_object);
 *
 * \brief This function returns the condition instance data for the object class.
 *
 * \param p_object
 *        Pointer to the object.
 */
typedef struct fbe_lifecycle_inst_cond_s * (*fbe_lifecycle_get_inst_cond_t)(struct fbe_base_object_s * p_object);

/*!
 * \struct fbe_lifecycle_const_callback_t;
 * \brief This structure defines static constant pointers to class callback functions.
 */
typedef const struct fbe_lifecycle_const_callback_s {
    fbe_lifecycle_func_online_t online_func;          /*!< Pointer to the class online function. */
    fbe_lifecycle_func_pending_t pending_func;        /*!< Pointer to the class pending function. */
    fbe_lifecycle_get_inst_data_t get_inst_data;      /*!< Pointer to the class function for getting the instance data. */
    fbe_lifecycle_get_inst_cond_t get_inst_cond;      /*!< Pointer to the class function for getting the instance conditions. */
} fbe_lifecycle_const_callback_t;

#define FBE_LIFECYCLE_CONST_COND_ATTR_NULL    ((fbe_lifecycle_const_cond_attr_t)0)   /*!< No attribute flags set. */
#define FBE_LIFECYCLE_CONST_COND_ATTR_NOSET   ((fbe_lifecycle_const_cond_attr_t)1)   /*!< Never set, just transition the state. */

/*!
 * \struct fbe_lifecycle_const_cond_t;
 * \brief This structure defines the static constant data of a lifecycle condition.
 */
typedef const struct fbe_lifecycle_const_cond_s {
    fbe_lifecycle_canary_t canary;                    /*!< FBE_LIFECYCLE_CANARY_CONST_COND */
    fbe_lifecycle_func_cond_type_t cond_type;         /*!< Type of condition function entry. */
    fbe_lifecycle_cond_id_t cond_id;                  /*!< Condition id. */
    fbe_lifecycle_func_cond_t p_cond_func;            /*!< Condition function pointer. */
    fbe_lifecycle_const_cond_attr_t cond_attr;        /*!< Condition attribute flags. */
} fbe_lifecycle_const_cond_t;

/*!
 * \struct fbe_lifecycle_const_base_cond_t;
 * \brief This structure defines the static constant data of a lifecycle base condition.
 */
#define FBE_LIFECYCLE_STATE_TRANSITIONS_MAX 7 /* specialize, activate, ready, hibernate, offline, fail, destroy */
typedef const struct fbe_lifecycle_const_base_cond_s {
    fbe_lifecycle_const_cond_t const_cond;            /*!< Condition constant data. */
    fbe_lifecycle_canary_t canary;                    /*!< FBE_LIFECYCLE_CANARY_CONST_BASE_COND */
    const char * p_cond_name;                         /*!< Pointer to a condition name. */
    fbe_lifecycle_state_t p_state_transition[FBE_LIFECYCLE_STATE_TRANSITIONS_MAX]; /*!< Condition state transitions. */
} fbe_lifecycle_const_base_cond_t;

typedef const struct fbe_lifecycle_const_base_timer_cond_s {
    fbe_lifecycle_const_base_cond_t const_base_cond;  /*!< Condition constant data. */
    fbe_lifecycle_canary_t canary;                    /*!< FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND */
    fbe_u32_t interval;                               /*!< 100th of a second resolution. */
} fbe_lifecycle_const_base_timer_cond_t;

typedef const struct fbe_lifecycle_const_base_cond_array_s {
    fbe_lifecycle_canary_t canary;                    /*!< FBE_LIFECYCLE_CANARY_CONST_BASE_COND_ARRAY */
    fbe_u32_t base_cond_max;
    fbe_lifecycle_const_base_cond_t * const * pp_base_cond;
} fbe_lifecycle_const_base_cond_array_t;

/*!
 * \struct fbe_lifecycle_inst_cond_t;
 * \brief This structure defines the instance data of a lifecycle condition.
 */
typedef struct fbe_lifecycle_inst_cond_s {
    union {
        struct {
            fbe_u16_t set_count;                      /*!< Incremented for each set call. */
            fbe_u16_t call_set_count;                 /*!< The set count when the condition function was called. */
        } simple;
        struct {
            fbe_u32_t interval;                       /*!< The time interval. */
        } timer;
    } u;
} fbe_lifecycle_inst_cond_t;

#define FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL         ((fbe_lifecycle_rotary_cond_attr_t)0)     /*!< No attribute flags set. */
#define FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET    ((fbe_lifecycle_rotary_cond_attr_t)1)     /*!< Preset when entering the rotary. */
#define FBE_LIFECYCLE_ROTARY_COND_ATTR_REDO_PRESETS ((fbe_lifecycle_rotary_cond_attr_t)1<<1)  /*!< Redo rotary presets when set. */
#define FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL_FUNC_OK ((fbe_lifecycle_rotary_cond_attr_t)1<<2)  /*!< No error if null cond function. */

/*!
 * \struct fbe_lifecycle_const_rotary_cond_t;
 * \brief This structure defines the a lifecycle condition in a rotary.
 */
typedef const struct fbe_lifecycle_const_rotary_cond_s {
    fbe_lifecycle_const_cond_t * p_const_cond;        /*!< Pointer to a constant condition. */
    fbe_lifecycle_rotary_cond_attr_t attr;            /*!< Rotary condition attributes. */
} fbe_lifecycle_const_rotary_cond_t;

/*!
 * \struct fbe_lifecycle_const_rotary_t
 * \brief This structure identifies the conditions for a lifecycle state.
 */
typedef const struct fbe_lifecycle_const_rotary_s {
    fbe_lifecycle_state_t this_state;                 /*!< The life-cycle state of this rotary. */
    fbe_u32_t rotary_cond_max;                        /*!< The number rotary conditions in the rotary. */
    fbe_lifecycle_const_rotary_cond_t * p_rotary_cond; /*!< Pointer to an array of rotary conditions. */
} fbe_lifecycle_const_rotary_t;

typedef const struct fbe_lifecycle_const_rotary_array_s {
    fbe_lifecycle_canary_t canary;                    /*!< FBE_LIFECYCLE_CANARY_CONST_ROTARY_ARRAY */
    fbe_u32_t rotary_max;
    fbe_lifecycle_const_rotary_t * p_rotary;
} fbe_lifecycle_const_rotary_array_t;

/*
 * Types of trace entries.
 */
#define FBE_LIFECYCLE_TRACE_TYPE_NULL                 ((fbe_lifecycle_trace_type_t) 0)
#define FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN          ((fbe_lifecycle_trace_type_t) 1)
#define FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR           ((fbe_lifecycle_trace_type_t) 2)
#define FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET          ((fbe_lifecycle_trace_type_t) 3)
#define FBE_LIFECYCLE_TRACE_TYPE_COND_SET             ((fbe_lifecycle_trace_type_t) 4)
#define FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE      ((fbe_lifecycle_trace_type_t) 5)
#define FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER       ((fbe_lifecycle_trace_type_t) 6)
#define FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE         ((fbe_lifecycle_trace_type_t) 7)
#define FBE_LIFECYCLE_TRACE_TYPE_COND_STATE_CHANGE    ((fbe_lifecycle_trace_type_t) 8)
#define FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE  ((fbe_lifecycle_trace_type_t) 9)
#define FBE_LIFECYCLE_TRACE_TYPE_RESCHEDULE_MONITOR   ((fbe_lifecycle_trace_type_t)10)
#define FBE_LIFECYCLE_TRACE_TYPE_CRANK_END            ((fbe_lifecycle_trace_type_t)11)
#define FBE_LIFECYCLE_TRACE_TYPE_MAX                  ((fbe_lifecycle_trace_type_t)12)

/*
 * Trace flags for filtering the trace.
 */
#define FBE_LIFECYCLE_TRACE_FLAGS_NO_TRACE            ((fbe_u16_t)0)
#define FBE_LIFECYCLE_TRACE_FLAGS_CRANKING            ((fbe_u16_t)1)
#define FBE_LIFECYCLE_TRACE_FLAGS_STATE_CHANGE        ((fbe_u16_t)(1<<1))
#define FBE_LIFECYCLE_TRACE_FLAGS_CONDITIONS          ((fbe_u16_t)(1<<2))
#define FBE_LIFECYCLE_TRACE_FLAGS_ALL                 ((fbe_u16_t)0xffff)

/*!
 * \struct fbe_lifecycle_trace_entry_s {
 * \brief This structure is used to trace state events.
 */
typedef struct fbe_lifecycle_trace_entry_s {
    fbe_lifecycle_trace_type_t type;                   /*!< Type of trace entry. */
    union {
        struct {
            fbe_lifecycle_state_t current_state;       /*!< Current lifecycle state. */
            fbe_u32_t crank_interval;                  /*!< Time interval since the last crank ended. */
        } crank_begin;                                 /*!< type = FBE_LIFECYCLE_TRACE_TYPE_CRANK_BEGIN */
        struct {
            fbe_lifecycle_state_t current_state;       /*!< Current lifecycle state. */
            fbe_lifecycle_cond_id_t cond_id;           /*!< Condition id. */
        } condition_clear;                             /*!< type = FBE_LIFECYCLE_TRACE_TYPE_COND_CLEAR */
        struct {
            fbe_lifecycle_state_t current_state;       /*!< Current lifecycle state. */
            fbe_lifecycle_cond_id_t cond_id;           /*!< Condition id. */
        } condition_preset;                            /*!< type = FBE_LIFECYCLE_TRACE_TYPE_COND_PRESET */
        struct {
            fbe_lifecycle_state_t current_state;       /*!< Current lifecycle state. */
            fbe_lifecycle_cond_id_t cond_id;           /*!< Condition id. */
        } condition_set;                               /*!< type = FBE_LIFECYCLE_TRACE_TYPE_COND_SET */
        struct {
            fbe_lifecycle_cond_id_t cond_id;           /*!< Condition id. */
        } condition_run_before;                        /*!< type = FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_BEFORE */
        struct {
            fbe_lifecycle_status_t status;             /*!< Lifecycle return status. */
            fbe_lifecycle_cond_id_t cond_id;           /*!< Condition id. */
        } condition_run_after;                         /*!< type = FBE_LIFECYCLE_TRACE_TYPE_COND_RUN_AFTER */
        struct {
            fbe_lifecycle_state_t old_state;           /*!< Old lifecycle state. */
            fbe_lifecycle_state_t new_state;           /*!< New lifecycle state. */
        } state_change;                                /*!< type = FBE_LIFECYCLE_TRACE_TYPE_STATE_CHANGE */
        struct {
            fbe_lifecycle_state_t old_state;           /*!< Old lifecycle state. */
            fbe_lifecycle_state_t new_state;           /*!< New lifecycle state. */
        } cond_state_change;                           /*!< type = FBE_LIFECYCLE_TRACE_TYPE_COND_STATE_CHANGE */
        struct {
            fbe_lifecycle_state_t old_state;           /*!< Old lifecycle state. */
            fbe_lifecycle_state_t new_state;           /*!< New lifecycle state. */
        } forced_state_change;                         /*!< type = FBE_LIFECYCLE_TRACE_TYPE_FORCED_STATE_CHANGE */
        struct {
            fbe_lifecycle_timer_msec_t msecs;          /*!< Resedule interval. */
        } reschedule_monitor;                          /*!< type = FBE_LIFECYCLE_TRACE_TYPE_RESCHEDULE_MONITOR */
        struct {
            fbe_lifecycle_state_t ending_state;        /*!< Ending lifecycle state. */
        } crank_end;                                   /*!< type = FBE_LIFECYCLE_TRACE_TYPE_CRANK_END */
    } u;
} fbe_lifecycle_trace_entry_t;

typedef enum fbe_lifecycle_trace_env_e {
    FBE_LIFECYCLE_TRACE_EXT = 0xeeee0000,
    FBE_LIFECYCLE_TRACE_CLI = 0xeeee0001,
} fbe_lifecycle_trace_env_t;

typedef void (*fbe_lifecycle_trace_func_t)(const char * fmt, ...) __attribute__((format(__printf_func__,1,2)));

/*!
 * \struct fbe_lifecycle_trace_context_s {
 * \brief This structure is used to determine the tracing context.
 */
typedef struct fbe_lifecycle_trace_context_s {
    fbe_lifecycle_trace_env_t trace_env;
    char * p_module_name;
    fbe_lifecycle_trace_func_t trace_func;
} fbe_lifecycle_trace_context_t;

/*!
 * \struct fbe_lifecycle_const_t
 * \brief This structure defines the static constant data of a class that implements lifecycle states.
 */
typedef const struct fbe_lifecycle_const_s {
    fbe_lifecycle_canary_t canary;                     /*!< FBE_LIFECYCLE_CANARY_CONST */
    fbe_class_id_t class_id;                           /*!< Class id of this class. */
    const struct fbe_lifecycle_const_s * p_super;      /*!< Pointer to the constant data of the super class. */
    fbe_lifecycle_const_rotary_array_t * p_rotary_array; /*!< Pointer to an array of rotary pointers. */
    fbe_lifecycle_const_base_cond_array_t * p_base_cond_array; /*!< Pointer to an array of base condition pointers. */
    fbe_lifecycle_const_callback_t * p_callback;       /*!< Pointer to the class callback functions. */
} fbe_lifecycle_const_t;

/*!
 * Types of lifecycle states.
 */
#define FBE_LIFECYCLE_STATE_TYPE_NOT_EXIST     ((fbe_lifecycle_state_type_t)0)
#define FBE_LIFECYCLE_STATE_TYPE_INVALID       ((fbe_lifecycle_state_type_t)1)
#define FBE_LIFECYCLE_STATE_TYPE_PERSISTENT    ((fbe_lifecycle_state_type_t)2)
#define FBE_LIFECYCLE_STATE_TYPE_TRANSITIONAL  ((fbe_lifecycle_state_type_t)3)
#define FBE_LIFECYCLE_STATE_TYPE_PENDING       ((fbe_lifecycle_state_type_t)4)

/*!
 * \struct fbe_lifecycle_const_base_state_t
 * \brief This structure defines the per-state, constant data in a base class.
 */
typedef const struct fbe_lifecycle_const_base_state_s {
    const char * p_name;                               /*!< Pointer to the state name. */
    fbe_lifecycle_state_type_t type;                   /*!< Type of this state. */
    fbe_lifecycle_state_t this_state;                  /*!< This state. */
    fbe_lifecycle_state_t next_state;                  /*!< Next state. */
    fbe_lifecycle_state_t pending_state;               /*!< Pending state. */
    fbe_lifecycle_timer_msec_t reschedule_msecs;       /*!< Monitor reschedule interval. */
    fbe_u32_t transition_max;                          /*!< The number of transitions states. */
    const fbe_lifecycle_state_t * p_transition_states; /*!< Pointer to the allowed transition states.*/
} fbe_lifecycle_const_base_state_t;

/*!
 * \struct fbe_lifecycle_const_base_t
 * \brief This structure defines the additional static constant data of a base class.
 */
typedef const struct fbe_lifecycle_const_base_s {
    fbe_lifecycle_const_t class_const;                 /*!< Class constant data. */
    fbe_lifecycle_canary_t canary;                     /*!< FBE_LIFECYCLE_CANARY_CONST_BASE */
    fbe_u32_t state_max;                               /*!< Number of lifecycle states. */
    fbe_lifecycle_const_base_state_t * p_base_state;   /*!< Pointer to an array of state constant data. */
} fbe_lifecycle_const_base_t;

/*!
 * \struct fbe_lifecycle_t
 * \brief This structure defines the dynamic instance data of a class that implements lifecycle states.
 */
typedef struct fbe_lifecycle_inst_s {
    fbe_lifecycle_canary_t canary;                     /*!< FBE_LIFECYCLE_CANARY_INST */
} fbe_lifecycle_inst_t;

/*!
 * Bit flags for lifecycle instace base attributes.
 */
#define FBE_LIFECYCLE_INST_BASE_ATTR_NULL           ((fbe_lifecycle_inst_base_attr_t)0)     /*!< No attribute flags set. */
#define FBE_LIFECYCLE_INST_BASE_ATTR_IN_CRANK       ((fbe_lifecycle_inst_base_attr_t)1)     /*!< The monitor is cranking. */
#define FBE_LIFECYCLE_INST_BASE_ATTR_STATE_CHANGE   ((fbe_lifecycle_inst_base_attr_t)1<<1)  /*!< The lifecycle state changed. */
#define FBE_LIFECYCLE_INST_BASE_ATTR_CLEAR_CUR_COND ((fbe_lifecycle_inst_base_attr_t)1<<2)  /*!< Clear the current condition. */

/*!
 * \struct fbe_lifecycle_inst_base_t
 * \brief This structure defines the additional dynamic instance data of a base class.
 */
typedef struct fbe_lifecycle_inst_base_s {
    struct fbe_lifecycle_inst_s class_inst;            /*!< Class instance data. */
    fbe_lifecycle_canary_t canary;                     /*!< FBE_LIFECYCLE_CANARY_INST_BASE */
    fbe_lifecycle_state_t state;                       /*!< Current lifecycle state. */
    fbe_lifecycle_inst_base_attr_t attr;               /*!< Lifecycle attributes. */
    fbe_lifecycle_timer_msec_t reschedule_msecs;       /*!< Monitor reschedule interval. */
    fbe_u16_t trace_flags;                             /*!< Flags for tracing. */
    fbe_u16_t trace_max;                               /*!< Number of trace entries. */
    fbe_u16_t first_trace_index;                       /*!< Index of the first trace entry. */
    fbe_u16_t next_trace_index;                        /*!< Index of the next trace entry. */
    fbe_spinlock_t state_lock;                         /*!< Lock for changing the state. */
    fbe_spinlock_t cond_lock;                          /*!< Lock for changing any condition. */
    fbe_spinlock_t trace_lock;                         /*!< Lock for changing the state. */
    fbe_time_t last_crank_time;                        /*!< Timestamp of the previous monitor crank */
    fbe_lifecycle_const_t * p_leaf_class_const;        /*!< pointer to the leaf class const data */
    struct fbe_lifecycle_inst_cond_s * p_cond_inst;    /*!< Condition inst pointer, when in a condition function. */
    const struct fbe_lifecycle_const_base_cond_s * p_const_base_cond; /*!< Const base cond pointer, when in a cond function. */
    struct fbe_base_object_s * p_object;               /*!< Pointer to the object. */
    struct fbe_packet_s * p_packet;                    /*!< Pointer to the monitor control packet. */
    fbe_lifecycle_trace_entry_t * p_trace;             /*!< Pointer to an array of trace entries. */
} fbe_lifecycle_inst_base_t;

/*\}*//* FBE_LIFECYCLE_STATE */

/*--- macros ----------------------------------------------------------------------------*/

/*
 * The following nasty macros supposedly make it easier to specify the lifecycle constant structures.
 */
#define FBE_LIFECYCLE_DEF_INST_DATA \
fbe_lifecycle_inst_t lifecycle

#define FBE_LIFECYCLE_DEF_INST_COND(size) \
fbe_lifecycle_inst_cond_t lifecycle_cond[size]

/* --- Callback Macro/Defines --- */

#define FBE_LIFECYCLE_CALLBACKS(class_name) class_name##_lifecycle_callbacks
#define FBE_LIFECYCLE_CALLBACK_GET_INST_DATA(class_name) class_name##_lifecycle_get_inst_data
#define FBE_LIFECYCLE_CALLBACK_GET_INST_COND(class_name) class_name##_lifecycle_get_inst_cond

#define FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(class_name) \
static fbe_lifecycle_inst_t * FBE_LIFECYCLE_CALLBACK_GET_INST_DATA(class_name) (fbe_base_object_t * p_object) \
{ return (fbe_lifecycle_inst_t*)&((fbe_##class_name##_t*)p_object)->lifecycle; }

#define FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(class_name) \
static fbe_lifecycle_inst_cond_t * FBE_LIFECYCLE_CALLBACK_GET_INST_COND(class_name) (fbe_base_object_t * p_object) \
{ return ((fbe_##class_name##_t*)p_object)->lifecycle_cond; }

#define FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_NULL_COND(class_name) \
static fbe_lifecycle_inst_cond_t * FBE_LIFECYCLE_CALLBACK_GET_INST_COND(class_name) (fbe_base_object_t * p_object) \
{ return NULL; }

#define FBE_LIFECYCLE_DEF_CONST_CALLBACKS(class_name, online_func, pending_func) \
(online_func), (pending_func), FBE_LIFECYCLE_CALLBACK_GET_INST_DATA(class_name), FBE_LIFECYCLE_CALLBACK_GET_INST_COND(class_name)

/*---  Condition Macro/Defines --- */

#define FBE_LIFECYCLE_BASE_COND_ARRAY(class_name) class_name##_lifecycle_base_cond_array
#define FBE_LIFECYCLE_BASE_CONDITIONS(class_name) class_name##_lifecycle_base_conditions

#define FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(id,func) \
FBE_LIFECYCLE_CANARY_CONST_COND, FBE_LIFECYCLE_COND_TYPE_DERIVED, (id), (func)

#define FBE_LIFECYCLE_DEF_CONST_DERIVED_TIMER_COND(id,func) \
FBE_LIFECYCLE_CANARY_CONST_COND, FBE_LIFECYCLE_COND_TYPE_DERIVED_TIMER, (id), (func)

#define FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(specialize, activate, ready, hibernate, offline, fail, destroy) \
{ specialize, activate, ready, hibernate, offline, fail, destroy }

#define FBE_LIFECYCLE_DEF_CONST_BASE_COND(name,id,func) \
{ FBE_LIFECYCLE_CANARY_CONST_COND, FBE_LIFECYCLE_COND_TYPE_BASE, (id), (func), FBE_LIFECYCLE_CONST_COND_ATTR_NULL }, \
FBE_LIFECYCLE_CANARY_CONST_BASE_COND, (name)

#define FBE_LIFECYCLE_DEF_CONST_BASE_COND_ATTR(name,id,func,attr) \
{ FBE_LIFECYCLE_CANARY_CONST_COND, FBE_LIFECYCLE_COND_TYPE_BASE, (id), (func), (attr) }, \
FBE_LIFECYCLE_CANARY_CONST_BASE_COND, (name)

#define FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(name,id,func) \
{ FBE_LIFECYCLE_CANARY_CONST_COND, FBE_LIFECYCLE_COND_TYPE_BASE_TIMER, (id), (func), FBE_LIFECYCLE_CONST_COND_ATTR_NULL }, \
FBE_LIFECYCLE_CANARY_CONST_BASE_COND, (name)

#define FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND_ATTR(name,id,func,attr) \
{ FBE_LIFECYCLE_CANARY_CONST_COND, FBE_LIFECYCLE_COND_TYPE_BASE_TIMER, (id), (func), (attr) }, \
FBE_LIFECYCLE_CANARY_CONST_BASE_COND, (name)

#define FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(class_name) \
static fbe_lifecycle_const_base_cond_array_t FBE_LIFECYCLE_BASE_CONDITIONS(class_name) = { \
FBE_LIFECYCLE_CANARY_CONST_BASE_COND_ARRAY, \
(sizeof(FBE_LIFECYCLE_BASE_COND_ARRAY(class_name))/sizeof(fbe_lifecycle_const_base_cond_t*)), \
FBE_LIFECYCLE_BASE_COND_ARRAY(class_name) }

#define FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(class_name) \
static fbe_lifecycle_const_base_cond_array_t FBE_LIFECYCLE_BASE_CONDITIONS(class_name) = \
{ FBE_LIFECYCLE_CANARY_CONST_BASE_COND_ARRAY, 0, NULL }

/* --- Rotary Macro/Defines --- */

#define FBE_LIFECYCLE_ROTARY_ARRAY(class_name) class_name##_lifecycle_rotary_array
#define FBE_LIFECYCLE_ROTARIES(class_name) class_name##_lifecycle_rotaries

#define FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(cond_name,cond_attr) \
{ (fbe_lifecycle_const_cond_t*)&(cond_name), (cond_attr) }

#define FBE_LIFECYCLE_DEF_CONST_NULL_ROTARY(state) \
{ (state), 0, NULL }

#define FBE_LIFECYCLE_DEF_CONST_ROTARY(state, array) \
{ (state), (sizeof(array)/sizeof(fbe_lifecycle_const_rotary_cond_t)), (array) }

#define FBE_LIFECYCLE_DEF_CONST_ROTARIES(class_name) \
static fbe_lifecycle_const_rotary_array_t FBE_LIFECYCLE_ROTARIES(class_name) = { \
FBE_LIFECYCLE_CANARY_CONST_ROTARY_ARRAY, \
(sizeof(FBE_LIFECYCLE_ROTARY_ARRAY(class_name))/sizeof(fbe_lifecycle_const_rotary_t)), \
FBE_LIFECYCLE_ROTARY_ARRAY(class_name) }

#define FBE_LIFECYCLE_DEF_CONST_NULL_ROTARIES(class_name) \
static fbe_lifecycle_const_rotary_array_t FBE_LIFECYCLE_ROTARIES(class_name) = \
{ FBE_LIFECYCLE_CANARY_CONST_ROTARY_ARRAY, 0, NULL }

/* --- Const Data Macro --- */

#define FBE_LIFECYCLE_CONST_DATA(class_name) fbe_##class_name##_lifecycle_const

#define FBE_LIFECYCLE_DEF_CONST_DATA(class_name, class_id, super) \
FBE_LIFECYCLE_CANARY_CONST, \
(class_id), \
(fbe_lifecycle_const_t*)(&(super)), \
&(FBE_LIFECYCLE_ROTARIES(class_name)), \
&(FBE_LIFECYCLE_BASE_CONDITIONS(class_name)), \
&(FBE_LIFECYCLE_CALLBACKS(class_name))

/*--- function prototypes ---------------------------------------------------------------*/

static __forceinline fbe_u32_t fbe_lifecycle_get_cond_class_num(fbe_lifecycle_cond_id_t cond_id)
{
    return (fbe_u32_t)(cond_id & FBE_LIFECYCLE_COND_MASK);
}

static __forceinline fbe_class_id_t fbe_lifecycle_get_cond_class_id(fbe_lifecycle_cond_id_t cond_id)
{
    return (fbe_class_id_t)((cond_id >> FBE_LIFECYCLE_COND_CLASS_SHIFT) & FBE_LIFECYCLE_COND_MASK);
}

fbe_status_t fbe_lifecycle_get_state(fbe_lifecycle_const_t * p_class_const,
                                     struct fbe_base_object_s * p_object,
                                     fbe_lifecycle_state_t * p_state);

fbe_status_t fbe_lifecycle_set_state(fbe_lifecycle_const_t * p_class_const,
                                     struct fbe_base_object_s * p_object,
                                     fbe_lifecycle_state_t state);

fbe_status_t fbe_lifecycle_set_cond(fbe_lifecycle_const_t * p_class_const,
                                    struct fbe_base_object_s * p_object,
                                    fbe_lifecycle_cond_id_t cond_id);

fbe_status_t fbe_lifecycle_clear_current_cond(struct fbe_base_object_s * p_object);

fbe_status_t fbe_lifecycle_force_clear_cond(fbe_lifecycle_const_t * p_class_const,
                                            struct fbe_base_object_s * p_object,
                                            fbe_lifecycle_cond_id_t cond_id);

fbe_status_t fbe_lifecycle_get_super_cond_func(fbe_lifecycle_const_t * p_class_const,
                                               struct fbe_base_object_s * p_object,
                                               fbe_lifecycle_state_t state,
                                               fbe_lifecycle_cond_id_t cond_id,
                                               fbe_lifecycle_func_cond_t * pp_cond_func);

fbe_status_t fbe_lifecycle_class_const_verify(fbe_lifecycle_const_t * p_class_const);

fbe_status_t fbe_lifecycle_class_const_hierarchy(fbe_lifecycle_const_t * p_class_const,
                                                 fbe_class_id_t * p_class_ids,
                                                 fbe_u32_t id_count);

fbe_status_t fbe_lifecycle_crank_object(fbe_lifecycle_const_t * p_class_const,
                                        struct fbe_base_object_s * p_object,
                                        struct fbe_packet_s * p_packet);

fbe_status_t fbe_lifecycle_do_all_cond_presets(fbe_lifecycle_const_t * p_class_const, 
                                                struct fbe_base_object_s * p_object,
                                                fbe_lifecycle_state_t this_state);

fbe_status_t fbe_lifecycle_reschedule(fbe_lifecycle_const_t * p_class_const,
                                      struct fbe_base_object_s * p_object,
                                      fbe_lifecycle_timer_msec_t msecs);

fbe_status_t fbe_lifecycle_set_trace_flags(fbe_lifecycle_const_t * p_class_const,
                                           struct fbe_base_object_s * p_object,
                                           fbe_lifecycle_trace_flags_t flags);

fbe_status_t fbe_lifecycle_set_trace_buffer(fbe_lifecycle_const_t * p_class_const,
                                            struct fbe_base_object_s * p_object,
                                            void * p_buffer,
                                            fbe_u32_t buffer_size);

fbe_status_t fbe_lifecycle_log_trace(fbe_lifecycle_const_t * p_class_const,
                                     struct fbe_base_object_s * p_object,
                                     fbe_u16_t entry_count);

fbe_status_t fbe_lifecycle_debug_process_rotary(fbe_lifecycle_trace_context_t * p_trace_context,
                                                fbe_object_id_t expected_object_id,
                                                fbe_lifecycle_state_t lifecycle_state);

void fbe_lifecycle_debug_trace_rotary(fbe_lifecycle_trace_context_t * p_trace_context,
                                      fbe_object_id_t expected_object_id,
                                      fbe_dbgext_ptr p_object,
                                      fbe_lifecycle_state_t lifecycle_state);

fbe_dbgext_ptr lifecycle_get_object_ptr(fbe_object_id_t object_id,
                                        fbe_dbgext_ptr topology_object_tbl_ptr,
                                        unsigned long entry_size);

fbe_status_t fbe_lifecycle_get_state_name(fbe_lifecycle_state_t state, const char ** pp_state_name);
fbe_status_t fbe_lifecycle_debug_get_state_name(fbe_lifecycle_state_t state, const char ** pp_state_name);

fbe_status_t
fbe_lifecycle_stop_timer(fbe_lifecycle_const_t * p_class_const,
                         struct fbe_base_object_s * p_object,
                         fbe_lifecycle_cond_id_t cond_id);

fbe_status_t fbe_lifecycle_verify(fbe_lifecycle_const_t * p_class_const,
                                   struct fbe_base_object_s * p_object);

void fbe_lifecycle_destroy_object(fbe_lifecycle_inst_t * p_class_inst,
                                  struct fbe_base_object_s * p_object);

fbe_status_t fbe_lifecycle_set_interval_timer_cond(fbe_lifecycle_const_t * p_class_const,
                                                   struct fbe_base_object_s * p_object,
                                                   fbe_lifecycle_cond_id_t cond_id, 
                                                   fbe_u32_t interval);

#endif /* FBE_LIFECYCLE_STATE_H */
