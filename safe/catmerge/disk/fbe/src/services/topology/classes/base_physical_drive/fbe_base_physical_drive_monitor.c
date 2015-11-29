#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_stat_api.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_base_object_trace.h"
#include "base_discovering_private.h"
#include "base_physical_drive_private.h"

#define MAX_PRIORITY_DIFFERENCE 30
#define NO_IO_PRIORITY_REDUCTION 10

fbe_status_t 
fbe_base_physical_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = NULL;

    base_physical_drive = (fbe_base_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_base_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive, packet);

    return status;  
}

fbe_status_t fbe_base_physical_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_physical_drive));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t base_physical_drive_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_physical_drive_get_protocol_address_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_protocol_address_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_physical_drive_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t base_physical_drive_specialize_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_init_specialize_time_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_specialize_timer_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_physical_drive_discovery_edge_not_present_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_physical_drive_init_activate_time_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_activate_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_physical_drive_activate_check_io_timer_drvd_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_physical_drive_block_server_not_empty_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_physical_drive_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_physical_drive_packet_canceled_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t base_physical_drive_register_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_unregister_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_set_cached_path_attr_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_config_changed_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_status_t fbe_base_physical_drive_fill_drive_cfg_info(fbe_base_physical_drive_t *base_physical_drive,
                                                         fbe_drive_configuration_registration_info_t *registration_info);
static fbe_lifecycle_status_t base_physical_drive_get_drive_location_info_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_get_drive_location_info_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_physical_drive_power_cycle_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_power_cycle_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_physical_drive_power_cycle_complete_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t base_physical_drive_check_traffic_load_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_dequeue_pending_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_physical_drive_disk_collect_flush_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_disk_collect_flush_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t base_physical_drive_spintime_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_glitch_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_trace_lifecycle_state_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_clear_power_save_attr_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_physical_drive_resume_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

void base_physical_drive_set_fault_bit_in_fail_state(fbe_base_object_t * base_object, fbe_bool_t link_fault);

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_physical_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_physical_drive);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_physical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        base_physical_drive,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        base_physical_drive_pending_func)
};

/*--- constant derived condition entries -----------------------------------------------*/
/* The physical drive object will complete canceled packets from block transport server queues */
static fbe_lifecycle_const_cond_t base_physical_drive_packet_canceled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED,
        base_physical_drive_packet_canceled_cond_function)
};

/* The physical drive object will instantiate base_logical drive_object */
static fbe_lifecycle_const_cond_t base_physical_drive_discovery_update_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
        base_physical_drive_discovery_update_cond_function)
};

/* The physical drive object will instantiate base_logical drive_object */
static fbe_lifecycle_const_cond_t base_physical_drive_power_cycle_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_CYCLE,
        base_physical_drive_power_cycle_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/
static fbe_lifecycle_const_base_cond_t base_physical_drive_init_specialize_time_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_init_specialize_time_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INIT_SPECIALIZE_TIME,
        base_physical_drive_init_specialize_time_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t base_physical_drive_specialize_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_physical_drive_specialize_timer_cond",
            FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_SPECIALIZE_TIMER,
            base_physical_drive_specialize_timer_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY),           /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    6000 /* fires after 60 seconds*/
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_get_protocol_address_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_get_protocol_address_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_PROTOCOL_ADDRESS,
        base_physical_drive_get_protocol_address_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_type_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_type_unknown_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_TYPE_UNKNOWN,
        base_physical_drive_invalid_type_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_init_activate_time_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_init_activate_time_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INIT_ACTIVATE_TIME,
        base_physical_drive_init_activate_time_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t base_physical_drive_activate_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_physical_drive_activate_timer_cond",
            FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_ACTIVATE_TIMER,
            base_physical_drive_activate_timer_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY),           /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    24000 /* fires after 240 seconds (4 min.) */
};

static fbe_lifecycle_const_cond_t base_physical_drive_activate_check_io_timer_drvd_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_TIMER_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_CHECK_QUEUED_IO_ACTIVATE_TIMER,
        base_physical_drive_activate_check_io_timer_drvd_cond_function)
};


static fbe_lifecycle_const_base_cond_t base_physical_drive_reset_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_reset_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_enter_glitch_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_enter_glitch_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_ENTER_GLITCH,
        base_physical_drive_glitch_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
                FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_register_config_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_register_config_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_REGISTER_CONFIG,
        base_physical_drive_register_config_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_unregister_config_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_unregister_config_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_UNREGISTER_CONFIG,
        base_physical_drive_unregister_config_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */    
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_set_cached_path_attr_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_set_cached_path_attr_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_SET_CACHED_PATH_ATTR,
        base_physical_drive_set_cached_path_attr_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_config_changed_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_config_changed_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CONFIG_CHANGED,
        base_physical_drive_config_changed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_get_drive_location_info_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_get_bus_enclosure_slot_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_DRIVE_LOCATION_INFO,
        base_physical_drive_get_drive_location_info_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t base_physical_drive_check_traffic_load_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_physical_drive_check_traffic_load_cond",
            FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CHECK_TRAFFIC_LOAD,
            base_physical_drive_check_traffic_load_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,            /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY),           /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    300 /* fires after 3 seconds*/
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_dequeue_pending_io_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_dequeue_pending_io_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DEQUEUE_PENDING_IO,
        base_physical_drive_dequeue_pending_io_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_disk_collect_flush_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_disk_collect_flush_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISK_COLLECT_FLUSH,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,             /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t base_physical_drive_spintime_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_physical_drive_spintime_timer_cond",
            FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_SPIN_TIME_TIMER,
            base_physical_drive_spintime_timer_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,            /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY),           /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    6000 /* fires after 60 seconds*/
};


static fbe_lifecycle_const_base_cond_t base_physical_drive_health_check_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_health_check",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */

};

static fbe_lifecycle_const_base_cond_t base_physical_drive_initiate_health_check_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_initiate_health_check_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INITIATE_HEALTH_CHECK,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */

};

static fbe_lifecycle_const_base_cond_t base_physical_drive_health_check_complete_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_health_check_complete_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_COMPLETE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */

};


static fbe_lifecycle_const_base_cond_t base_physical_drive_trace_lifecycle_state_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_trace_lifecycle_state_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_TRACE_LIFECYCLE_STATE,
        base_physical_drive_trace_lifecycle_state_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */

};

/*used to clear the power saving flag whenfor some reason we got out of power saving not in a way that
was initiated by SEP above us. For example, someoene reset the drive or issued a ddt io directly to the LDO*/
static fbe_lifecycle_const_base_cond_t base_physical_drive_clear_power_save_attr_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_clear_power_save_attr_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CLEAR_POWER_SAVE_ATTR,
        base_physical_drive_clear_power_save_attr_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_physical_drive_resume_io_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_physical_drive_resume_io_cond",
        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESUME_IO,
        base_physical_drive_resume_io_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* IMPORTANT - The order of this array must align with the enum fbe_base_physical_drive_lifecycle_cond_id_t since
   the enum is used as an index to get the condition */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_physical_drive)[] = {
    &base_physical_drive_init_specialize_time_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_physical_drive_specialize_timer_cond,
    &base_physical_drive_get_protocol_address_cond,
    &base_physical_drive_type_unknown_cond,
    &base_physical_drive_init_activate_time_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_physical_drive_activate_timer_cond,    
    &base_physical_drive_reset_cond,
    &base_physical_drive_register_config_cond,
    &base_physical_drive_unregister_config_cond,
    &base_physical_drive_set_cached_path_attr_cond,
    &base_physical_drive_config_changed_cond,
    &base_physical_drive_get_drive_location_info_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_physical_drive_check_traffic_load_cond,
    &base_physical_drive_dequeue_pending_io_cond,
    &base_physical_drive_disk_collect_flush_cond,    
    (fbe_lifecycle_const_base_cond_t*)&base_physical_drive_spintime_timer_cond,
    &base_physical_drive_enter_glitch_cond,
    &base_physical_drive_health_check_cond,
    &base_physical_drive_initiate_health_check_cond,
    &base_physical_drive_health_check_complete_cond,
    &base_physical_drive_trace_lifecycle_state_cond,
    &base_physical_drive_clear_power_save_attr_cond,
    &base_physical_drive_resume_io_cond,
};


FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_physical_drive);

/*--- constant rotaries ----------------------------------------------------------------*/

/*NOTE: Rotaries will always execute derived conditions first before base conditions, no matter
        which order they're put in. If you need to guarantee ordering then condition should be defined
        in the appropriate class.  Or use a single base condition which implements the ordering.
*/

static fbe_lifecycle_const_rotary_cond_t base_physical_drive_specialize_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_get_drive_location_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_init_specialize_time_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_specialize_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_get_protocol_address_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_type_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_physical_drive_activate_rotary[] = {    
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_power_cycle_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_trace_lifecycle_state_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_activate_check_io_timer_drvd_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_get_protocol_address_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_reset_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_enter_glitch_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_health_check_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_initiate_health_check_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), 
        FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_clear_power_save_attr_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* handles cleanup of health check.  must be at end of rotary after any additional recovery actions such as reset. */   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_health_check_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
};

static fbe_lifecycle_const_rotary_cond_t base_physical_drive_ready_rotary[] = {    
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_packet_canceled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_trace_lifecycle_state_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_resume_io_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_config_changed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_unregister_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_register_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_set_cached_path_attr_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_dequeue_pending_io_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
            
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_disk_collect_flush_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_spintime_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_physical_drive_fail_rotary[] = {
    /* Derived conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_power_cycle_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_trace_lifecycle_state_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_unregister_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_physical_drive_destroy_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_trace_lifecycle_state_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_unregister_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_physical_drive_hibernate_rotary[] = {
    /* Derived conditions */    

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_trace_lifecycle_state_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_spintime_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* If drive is spundown, we should still be able to unregister from DIEH.  When it goes back to ready it will register
       with new table record */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_physical_drive_config_changed_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_physical_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, base_physical_drive_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, base_physical_drive_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, base_physical_drive_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, base_physical_drive_hibernate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, base_physical_drive_fail_rotary), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, base_physical_drive_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_physical_drive);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_physical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        base_physical_drive,
        FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_discovering))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/
static fbe_lifecycle_status_t 
base_physical_drive_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    switch(base_physical_drive->element_type){
        case FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_SSP:
            fbe_base_object_change_class_id((fbe_base_object_t *) base_physical_drive, FBE_CLASS_ID_SAS_PHYSICAL_DRIVE);
            break;
        case FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_STP:
            fbe_base_object_change_class_id((fbe_base_object_t *) base_physical_drive, FBE_CLASS_ID_SATA_PHYSICAL_DRIVE);
            break;
        default:
            return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_physical_drive_get_protocol_address_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, base_physical_drive_get_protocol_address_cond_completion, base_physical_drive);

    /* Call the executer function to do actual job */
    fbe_base_physical_drive_get_protocol_address(base_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
base_physical_drive_get_protocol_address_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    
    base_physical_drive = (fbe_base_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Invalid control_status: 0x%X\n",
                                __FUNCTION__, control_status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
base_physical_drive_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)p_object;
    fbe_u32_t number_of_clients = 0;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *)base_physical_drive, &number_of_clients);
#if 0
    /* no more LDO */
    if(number_of_clients == 0) {
        status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) base_physical_drive, 
                                                        FBE_CLASS_ID_LOGICAL_DRIVE, 
                                                        0 /* We should have only one client */);
    }
#endif

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
base_physical_drive_init_specialize_time_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_force_clear_cond(&fbe_base_physical_drive_lifecycle_const, 
                (fbe_base_object_t*)base_physical_drive, 
                 FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_SPECIALIZE_TIMER);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear timer condition, status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_physical_drive_specialize_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_path_attr_t path_attr;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)base_physical_drive, &path_attr);
    if ((path_attr & FBE_DISCOVERY_PATH_ATTR_LINK_READY) && !base_physical_drive->slot_reset)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s RESET slot once\n", __FUNCTION__);
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, base_physical_drive_specialize_timer_cond_completion, base_physical_drive);

        /* Call the executer function to do actual job */
        fbe_base_physical_drive_reset_slot(base_physical_drive, packet);

        /* Return pending to the lifecycle */
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                        FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Specialize timer expired\n", __FUNCTION__);

    fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_object, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED);

    /* We need to set the BO:FAIL lifecycle condition */
    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
            (fbe_base_object_t*)base_physical_drive, FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't set BO:FAIL condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_status_t 
base_physical_drive_specialize_timer_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Remember we already sent the reset */
    base_physical_drive->slot_reset++;

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

#if 0
static fbe_lifecycle_status_t 
base_physical_drive_discovery_edge_not_present_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_path_attr_t path_attr;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_discovery_transport_get_path_attributes(&((fbe_base_discovered_t *)base_physical_drive)->discovery_edge, &path_attr);
    if(!(path_attr & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT)) {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
#endif

static fbe_lifecycle_status_t 
base_physical_drive_init_activate_time_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_force_clear_cond(&fbe_base_physical_drive_lifecycle_const, 
                (fbe_base_object_t*)base_physical_drive, 
                 FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_ACTIVATE_TIMER);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear timer condition, status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_physical_drive_activate_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    /* We need to set BO:FAIL lifecycle condition */
    fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_object, FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ACTIVATE_TIMER_EXPIRED);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
            (fbe_base_object_t*)base_physical_drive, 
            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't set BO:FAIL condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_physical_drive_activate_check_io_timer_drvd_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t number_of_clients = 0;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          "%s entry\n", __FUNCTION__);

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            "%s set path attributes to check timers of queued IOs\n", __FUNCTION__);
    
    fbe_block_transport_server_get_number_of_clients(&base_physical_drive->block_transport_server, &number_of_clients);
    
    if(number_of_clients > 0) {
    
        status = fbe_block_transport_server_set_path_attr_all_servers(&base_physical_drive->block_transport_server,
                                                    FBE_BLOCK_PATH_ATTR_FLAGS_CHECK_QUEUED_IO_TIMER);    
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s Failed to set path attribute condition FBE_BLOCK_PATH_ATTR_FLAGS_CHECK_QUEUED_IO_TIMER, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
base_physical_drive_block_server_not_empty_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_u32_t number_of_clients;

    base_physical_drive = (fbe_base_physical_drive_t *)base_object;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        "%s entry\n", __FUNCTION__);


    status = fbe_block_transport_server_get_number_of_clients(&base_physical_drive->block_transport_server, &number_of_clients);

    if((status == FBE_STATUS_OK) && (number_of_clients == 0)){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_physical_drive_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_lifecycle_status_t      lifecycle_status;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_status_t                status;
    fbe_lifecycle_state_t       lifecycle_state;
    fbe_path_attr_t             path_attr = 0;
    fbe_bool_t                  link_fault = FBE_FALSE;

    base_physical_drive = (fbe_base_physical_drive_t *)base_object;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        "%s entry\n", __FUNCTION__);

    /* Clear path attribute. */    
    status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const, base_object, &lifecycle_state);    
        
    /* LINK_FAULT should only be modified in monitor context to avoid any race conditions.  */
    if (fbe_base_pdo_are_flags_set(base_physical_drive, FBE_PDO_FLAGS_SET_LINK_FAULT))
    {
        fbe_block_transport_server_set_path_attr_all_servers(
                    &base_physical_drive->block_transport_server,
                    FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT);

        fbe_base_pdo_clear_flags_under_lock(base_physical_drive, FBE_PDO_FLAGS_SET_LINK_FAULT);

        /* If this happens during pending_ready, it most likely means we got a drive logout while in
           the activate rotary.  The LF gets cleared anyway at end of this function when in pending_ready.   */
        if (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_READY)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "%s setting link_fault in pending_ready\n", __FUNCTION__);
        }
    }

    /* Set the drive's fault bit when the drive is failed. */
    if (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_FAIL)
    {
        /* Get the current path attribute. if the link fault is set and death reaseon is the activate timer expired, then don't set it to drive fault.
        */
        if (fbe_block_transport_server_is_edge_connected(&base_physical_drive->block_transport_server) )
        {
            fbe_block_transport_server_get_path_attr(&(base_physical_drive->block_transport_server), 0, &path_attr);
            
            link_fault = (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT)? FBE_TRUE : FBE_FALSE;
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                    "%s link fault = %d \n", __FUNCTION__, link_fault);
            base_physical_drive_set_fault_bit_in_fail_state(base_object, link_fault);
        }
    }
    

    status = fbe_base_object_usurper_queue_drain_io(&fbe_base_physical_drive_lifecycle_const,
                                                (fbe_base_object_t *) base_physical_drive);

    if (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_OFFLINE ||
        lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_FAIL ||
        lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY ||
        lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_READY)
    {
        // we should clear FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK, otherwise PVD might stuck on download_condition
        fbe_block_transport_server_clear_path_attr_all_servers(&(base_physical_drive->block_transport_server),
                                                               FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
        fbe_base_physical_drive_lock(base_physical_drive);

        // If download state is set, then just clear it.
        if (fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD))
        {
            fbe_base_physical_drive_clear_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD);
        }
        fbe_base_physical_drive_unlock(base_physical_drive);
    }

    if (lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_READY)
    {
        /* If we were in Drive Lockup error handling and now going back to Ready, then we are done.  Clear
           the state and resume the queue. */
        fbe_base_physical_drive_lock(base_physical_drive);

        if (fbe_base_physical_drive_is_local_state_set(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DRIVE_LOCKUP_RECOVERY))
        {
            fbe_base_physical_drive_clear_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_DRIVE_LOCKUP_RECOVERY);
            fbe_block_transport_server_resume(&base_physical_drive->block_transport_server);             
        }

        fbe_base_physical_drive_unlock(base_physical_drive);
    }

    lifecycle_status = fbe_block_transport_server_pending(&base_physical_drive->block_transport_server,
                                                                &fbe_base_physical_drive_lifecycle_const,
                                                                (fbe_base_object_t *) base_physical_drive);

    /* SLF requires that the edge must go enabled before clearing link fault bit.  So we must do this after
       fbe_block_transport_server_pending() 
    */
    if ((lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_READY) &&
        (base_physical_drive->block_transport_server.base_transport_server.client_list != NULL))
    {
        fbe_block_transport_server_clear_path_attr_all_servers(&(base_physical_drive->block_transport_server),
                                                   FBE_BLOCK_PATH_ATTR_FLAGS_FAULT_MASK);
    }


    return lifecycle_status;
}

static fbe_lifecycle_status_t 
base_physical_drive_packet_canceled_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)p_object;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    fbe_block_transport_server_process_canceled_packets(&base_physical_drive->block_transport_server);
     
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
base_physical_drive_register_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_drive_configuration_registration_info_t registration_info;

    fbe_zero_memory(&registration_info, sizeof(fbe_drive_configuration_registration_info_t));

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry.  current handle 0x%x\n", __FUNCTION__, base_physical_drive->drive_configuration_handle);

    if(base_physical_drive->drive_configuration_handle == FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE){
        switch(base_physical_drive->element_type){
            case FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_SSP:
                registration_info.drive_type = FBE_DRIVE_TYPE_SAS;
                break;
            case FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_STP:
                registration_info.drive_type = FBE_DRIVE_TYPE_SATA;
                break;
            default:
                registration_info.drive_type = FBE_DRIVE_TYPE_INVALID;
        }

        fbe_base_physical_drive_fill_drive_cfg_info(base_physical_drive, &registration_info);
        fbe_drive_configuration_register_drive (&registration_info, &base_physical_drive->drive_configuration_handle);  
        /* whenever a new dieh configuration is reloaded the current stats will be cleared. */
        fbe_stat_drive_error_clear(&base_physical_drive->drive_error);  /* reset DIEH ratios to 0 */
        fbe_base_physical_drive_clear_dieh_state(base_physical_drive);  

        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH,
                              "%s new handle 0x%x\n", __FUNCTION__, base_physical_drive->drive_configuration_handle);
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_physical_drive_unregister_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry. current handle:0x%x\n", __FUNCTION__, base_physical_drive->drive_configuration_handle);


    if(base_physical_drive->drive_configuration_handle != FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE){
        fbe_drive_configuration_unregister_drive (base_physical_drive->drive_configuration_handle);
        base_physical_drive->drive_configuration_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * base_physical_drive_set_cached_path_attr_cond_function()
 ****************************************************************
 * @brief
 *  This condition function sets the cached path attributes on the edge.
 *  The path attributes may be cached if the edge didn't exist when the
 *  attribute was attempted to be set.  If the edge still doesn't exist
 *  when this condition function is called it will pend until it is created.
 * 
 *  The design for PDO is such that when it reaches the Ready state it will
 *  create the LDO object.  During LDO specialize it will connect it's edge
 *  to PDO.  Due to this design the edge creation is asynchronous to PDO, so
 *  it may have to wait a bit before it's created and can successfully set
 *  the edge attributes.
 * 
 * @param - base_object - ptr to base object.
 * @param - packet  -  monitor packet
 *
 * @author
 *  17-Jul-2012:  Wayne Garrett  -  Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t base_physical_drive_set_cached_path_attr_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t*)base_object;

    if (0 == base_physical_drive->cached_path_attr)  /* don't set if there is nothing to set */
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
        }
    }
    else if( fbe_block_transport_server_is_edge_connected(&base_physical_drive->block_transport_server) &&
             FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED != base_physical_drive->logical_state)  /* when drive is logically online/failed all edge paths are connected up to PVD.*/
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO, 
                                                   "%s setting path_attr:0x%x\n", __FUNCTION__, base_physical_drive->cached_path_attr);

        fbe_block_transport_server_set_path_attr_all_servers( &base_physical_drive->block_transport_server, base_physical_drive->cached_path_attr);

        /* Clear the cached path attributes as we have set path attributes*/
        base_physical_drive->cached_path_attr = 0;
    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                    "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
        }        
    }
    else  /* edge not connect.  so retry in a bit*/
    {
        if (base_object->reschedule_count % 600 == 0)  /* log once a minute, based on reschuling time below */
        {
            /*Note, first trace probably won't show up until a minute has elapsed*/
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO, 
                                                       "%s LDO edge not connected.  retrying... path_attr:0x%x\n", __FUNCTION__, base_physical_drive->cached_path_attr);
        }

        fbe_lifecycle_reschedule(&fbe_base_physical_drive_lifecycle_const, 
                                 (fbe_base_object_t*)base_physical_drive,  
                                 100); /*msec*/
    }   

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*we want to check if the handle changed which means we would need to process errors with a new handle
we find ourselvs in this function when the drive configuration table had new values pushed in*/
static fbe_lifecycle_status_t 
base_physical_drive_config_changed_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_physical_drive_t *                 base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_drive_configuration_registration_info_t registration_info;
    fbe_bool_t                                  handle_changed = FBE_FALSE;

    fbe_zero_memory(&registration_info, sizeof(fbe_drive_configuration_registration_info_t));

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          "%s entry\n", __FUNCTION__);

    switch(base_physical_drive->element_type){
    case FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_SSP:
        registration_info.drive_type = FBE_DRIVE_TYPE_SAS;
        break;
    case FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_STP:
        registration_info.drive_type = FBE_DRIVE_TYPE_SATA;
        break;
    default:
        registration_info.drive_type = FBE_DRIVE_TYPE_INVALID;
    }

    /* only check if handle changed if we had a valid handle.   otherwise go ahead and re-register.*/
    if (base_physical_drive->drive_configuration_handle != FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE)
    {    
        
        fbe_base_physical_drive_fill_drive_cfg_info(base_physical_drive, &registration_info);
    
        status = fbe_drive_configuration_did_drive_handle_change(&registration_info,
                                                                 base_physical_drive->drive_configuration_handle,
                                                                 &handle_changed);
    }
    else
    {
        handle_changed = FBE_TRUE;
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    /*if the handale changed we would need to go to activate to aquire a new handle*/
    if (handle_changed) {

        /*we have to unregister the old one, and register the new one */
        fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive, 
                               FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_UNREGISTER_CONFIG);

        fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, (fbe_base_object_t*)base_physical_drive, 
                               FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_REGISTER_CONFIG);
        
    }
    else
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                              "%s handle 0x%x hasn't changed\n", __FUNCTION__, base_physical_drive->drive_configuration_handle);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_status_t fbe_base_physical_drive_fill_drive_cfg_info(fbe_base_physical_drive_t *base_physical_drive,
                                                                fbe_drive_configuration_registration_info_t *registration_info)
{
    fbe_base_physical_drive_get_drive_vendor_id(base_physical_drive, &registration_info->drive_vendor);
    fbe_zero_memory(registration_info->fw_revision, FBE_SCSI_INQUIRY_REVISION_SIZE + 1);
    fbe_copy_memory(registration_info->fw_revision, base_physical_drive->drive_info.revision,FBE_SCSI_INQUIRY_REVISION_SIZE);
    fbe_zero_memory(registration_info->serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
    fbe_copy_memory (registration_info->serial_number, base_physical_drive->drive_info.serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    fbe_zero_memory(registration_info->part_number, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1);
    fbe_copy_memory (registration_info->part_number, base_physical_drive->drive_info.part_number, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE);

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
base_physical_drive_get_drive_location_info_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)p_object;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, base_physical_drive_get_drive_location_info_cond_completion, base_physical_drive);

    /* Call the executer function to do actual job */
    fbe_base_physical_drive_get_drive_location_info(base_physical_drive, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
base_physical_drive_get_drive_location_info_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    
    base_physical_drive = (fbe_base_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_status(control_operation, &control_status);

    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Invalid control_status: 0x%X",
                                __FUNCTION__, control_status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* We are done getting the bus, enclosure number, and slot number. 
     * I would assume that it is OK to clear CURRENT condition here.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
base_physical_drive_power_cycle_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;
    fbe_path_attr_t path_attr = 0;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH, "%s entry\n", __FUNCTION__);

    /* debug hook */
    fbe_base_object_check_hook(base_object,  SCHEDULER_MONITOR_STATE_PHYSICAL_DRIVE_OBJECT_ACTIVATE,
                               FBE_PHYSICAL_DRIVE_OBJECT_SUBSTATE_POWERCYCLE, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    { 
        /* pause this condition by preventing it form completing. */
        return FBE_LIFECYCLE_STATUS_DONE; 
    }


    status = fbe_base_discovered_get_path_state((fbe_base_discovered_t *) base_physical_drive, &path_state);
    if(status != FBE_STATUS_OK || path_state != FBE_PATH_STATE_ENABLED)
    {
        if(path_state == FBE_PATH_STATE_BROKEN)
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s path state broken", __FUNCTION__);

            /* We need to set the BO:FAIL lifecycle condition */
            status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)base_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
            fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_object, 
                                                 FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t *)base_physical_drive, &path_attr);

    if ((path_attr & FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_COMPLETE) == 0)
    {
        if ((path_attr & FBE_DISCOVERY_PATH_ATTR_POWERCYCLE_PENDING) == 0)
        {
            /* Call the executer function to send power cycle packet. */
            status = fbe_transport_set_completion_function(packet, 
                         base_physical_drive_power_cycle_cond_completion, base_physical_drive);
            fbe_base_physical_drive_send_power_cycle(base_physical_drive, packet, FALSE);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }
    else
    {
        /* Call the executer function to send power cycle completed packet to Encl. */
        status = fbe_transport_set_completion_function(packet, 
                     base_physical_drive_power_cycle_complete_completion, base_physical_drive);
        fbe_base_physical_drive_send_power_cycle(base_physical_drive, packet, TRUE);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_status_t 
base_physical_drive_power_cycle_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    
    base_physical_drive = (fbe_base_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);


    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
    }

    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Invalid control_status: 0x%X\n",
                                __FUNCTION__, control_status);

        /* Clear the current condition if Encl object rejects the request. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }

    return FBE_STATUS_OK;
}


static fbe_status_t 
base_physical_drive_power_cycle_complete_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t control_status;
    fbe_lifecycle_state_t lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    
    base_physical_drive = (fbe_base_physical_drive_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);

    if ((status == FBE_STATUS_OK) && (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_INFO,
                                "%s Power cycle completed\n",
                                __FUNCTION__);


        /* Clear the current condition if power-cycle is complete. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }

        /* If the object is in FAIL state, destroy it to get out if the state. */
        status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const,
                                        (fbe_base_object_t*)base_physical_drive,
                                        &lifecycle_state);
        if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_FAIL))
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                  FBE_TRACE_LEVEL_INFO,
                                  "%s force to recover from FAIL state.\n",
                                  __FUNCTION__);

            status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                            (fbe_base_object_t*)base_physical_drive, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      "%s can't set DESTROY condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }        
        }
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t base_physical_drive_check_traffic_load_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
        fbe_atomic_t                            average_load = 0;
        fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
        fbe_atomic_t                            load_factor = 0;
        fbe_u32_t                                       drive_buffer_size = 0;
        fbe_atomic_t                            numerator = 0;
        fbe_atomic_t                            denominator = 0;
        fbe_traffic_priority_t          delta_load = 0;
        fbe_traffic_priority_t          current_priority = FBE_TRAFFIC_PRIORITY_INVALID;
        fbe_atomic_t                            total_outstanding_ios;
        fbe_atomic_t                            total_io_priority;
        fbe_atomic_t                            highest_outstanding_ios;
        fbe_u32_t                                       load_difference = 0;
        fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                          FBE_TRACE_LEVEL_ERROR,
                                                          "%s can't clear current condition, status: 0x%X\n",
                                                          __FUNCTION__, status);
        }

        /*if this is a drive from one of the DB drives we do not apply this algorithm here. These drive already have a smaller queue than
        usual since we allow also NT MIRROR IO on them. If we run the algorithm on them we will keep generating events up the edges because the small
        queue if filling very quick. Also NT MIRROR has a IO profile of small bursts that causes the edges to change every few seconds which is just 
        bugging down the system. The fact the queue is small will make sure BG IO that comes in will have a chance to be queued just as other IO. It is 
        also subjected to the credit system so it might be throtteld. Urget IOs from a chace dump or any other important module will get out of
        the block transport faster so we don't delay them anyway.*/

        if ((base_physical_drive->slot_number >= 0 && base_physical_drive->slot_number <= 3) &&
                base_physical_drive->enclosure_number == 0 && base_physical_drive->port_number == 0) {

                return FBE_LIFECYCLE_STATUS_DONE;
        }

        /*get a snapshot of all the parameters we need to use*/
        fbe_atomic_exchange(&total_outstanding_ios, base_physical_drive->total_outstanding_ios);
        fbe_atomic_exchange(&total_io_priority, base_physical_drive->total_io_priority);
        fbe_atomic_exchange(&highest_outstanding_ios, base_physical_drive->highest_outstanding_ios );

    /*did we even have any IO ?*/
        if (total_outstanding_ios == 0) {
                /*if we did not have any IO previously but the edge had some IO load on it, we eventually want to bring the edge 
                IO load down. To do it, we artificailly reduce the previous_traffic_priority and if it's finally zeroed out, we then 
                set a low priority on the edge*/
                fbe_block_transport_server_get_traffic_priority(&base_physical_drive->block_transport_server, &current_priority);
                if ((current_priority != FBE_TRAFFIC_PRIORITY_VERY_LOW) && 
                        (base_physical_drive->previous_traffic_priority == FBE_TRAFFIC_PRIORITY_VERY_LOW)) {
                        fbe_block_transport_server_set_traffic_priority(&base_physical_drive->block_transport_server, FBE_TRAFFIC_PRIORITY_VERY_LOW);
                        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_LOW, " NO IO, load set to:FBE_TRAFFIC_PRIORITY_VERY_LOW\n");

                }else if (base_physical_drive->previous_traffic_priority != FBE_TRAFFIC_PRIORITY_VERY_LOW){
        
                        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_LOW,
                                                                                                           "No IO reducing previous in 10%%:old:%d, new:%d\n", base_physical_drive->previous_traffic_priority, ((base_physical_drive->previous_traffic_priority * 90) / 100));
        
                        base_physical_drive->previous_traffic_priority = ((base_physical_drive->previous_traffic_priority * (100 - NO_IO_PRIORITY_REDUCTION)) / 100);
                        if (base_physical_drive->previous_traffic_priority <= FBE_TRAFFIC_PRIORITY_INVALID) {
                                base_physical_drive->previous_traffic_priority = FBE_TRAFFIC_PRIORITY_VERY_LOW;
                        }
                }
                
        return FBE_LIFECYCLE_STATUS_DONE;
        }

        /*set all parameters for next time so they can keep rolling on the IO thread context while we process here this snapshot*/
        fbe_atomic_exchange(&base_physical_drive->total_outstanding_ios, 0);
        fbe_atomic_exchange(&base_physical_drive->total_io_priority, 0);
        fbe_atomic_exchange(&base_physical_drive->highest_outstanding_ios, 0);

        /*let do some math to calculate how loaded the drive is and advertise it up to all edges*/
        average_load = total_io_priority / total_outstanding_ios;

        /*we need to factor in the highet load we had on the drive buffer, the fuller the buffer gets,
        we need to push the average up, all the way to the highest priority, even if all operations 
        were of a low priority. The fact the drive buffer is filling up, indicates we have a high load*/
        fbe_block_transport_server_get_outstanding_io_max(&base_physical_drive->block_transport_server, &drive_buffer_size);

        /*we look at how many IOs we had below or above teh 50% mark. Then we multiply that by half of the priority
        so it can add or reduce the priority on the edge*/
        numerator = ((highest_outstanding_ios - (drive_buffer_size / 2)) * ((FBE_TRAFFIC_PRIORITY_LAST - 1) / 2));
        denominator = (drive_buffer_size / 2);
        load_factor = (numerator / denominator);

        /*and apply the factor*/
        average_load += load_factor;
        if (average_load >= FBE_TRAFFIC_PRIORITY_LAST){
                average_load = FBE_TRAFFIC_PRIORITY_LAST - 1;
        }else if (average_load <= FBE_TRAFFIC_PRIORITY_INVALID) {
                average_load = FBE_TRAFFIC_PRIORITY_INVALID + 1;
        }

        /*now, we don't want to change the state of the edge too frequently, so only if we have a MAX_PRIORITY_DIFFERENCE% change in the load, 
        only then we change the edge state, if not, we just zero everything and wait for the next monitor. we can have a case
        where the previous one is either bigger or smaller than us so we have to reduce us or the previous one
        */

        if (base_physical_drive->previous_traffic_priority > average_load) {
                delta_load = base_physical_drive->previous_traffic_priority - average_load;
        }else if (base_physical_drive->previous_traffic_priority < average_load) {
                delta_load = average_load - base_physical_drive->previous_traffic_priority;
        }

        load_difference = ((delta_load * 100) / (FBE_TRAFFIC_PRIORITY_LAST - 1));

        if (load_difference > MAX_PRIORITY_DIFFERENCE) {
                /*if even after the reduction, we have a big enough difference, we change the edge state (the avergage is alreay including the factor)*/
                fbe_block_transport_server_set_traffic_priority(&base_physical_drive->block_transport_server, average_load);

                fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_DEBUG_LOW,
                        "(prev load:%d,(total_pri:%lld/total_io:%lld)=%lld + Factor:%lld ->new load:%lld,Qusage:%lld,Qmax %d\n",
                        base_physical_drive->previous_traffic_priority,
                        (long long)total_io_priority,
                        (long long)total_outstanding_ios,
                        (long long)(total_io_priority / total_outstanding_ios),
                        (long long)load_factor,
                        (long long)average_load,
                        (long long)highest_outstanding_ios,
                        drive_buffer_size);

        base_physical_drive->previous_traffic_priority = average_load;
        }
    
        return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t base_physical_drive_dequeue_pending_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
        fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
        fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;

        status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s can't clear current condition, status: 0x%X\n",
                                                                                                   __FUNCTION__, status);
    }

        /*need to send all IOs that were queud for some reason(e.g. we were hibernating and we got IO)*/
        fbe_block_transport_server_process_io_from_queue(&base_physical_drive->block_transport_server);

        return FBE_LIFECYCLE_STATUS_DONE;
}

/*increment in one the counter for either hibernation time (not spinning) or ready time (spinning)*/
static fbe_lifecycle_status_t base_physical_drive_spintime_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
        fbe_status_t                            status;
        fbe_lifecycle_state_t           lifecycle_state;
        fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
        fbe_notification_data_changed_info_t dataChangeInfo = {0};
        const fbe_u32_t send_notification_minutes = 15;
        fbe_u32_t spin_time;

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
                fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                          FBE_TRACE_LEVEL_ERROR,
                                                          "%s can't clear current condition, status: 0x%X\n",
                                                          __FUNCTION__, status);
        }

        status = fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const, base_object, &lifecycle_state);
    if(status != FBE_STATUS_OK) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s error: fbe_lifecycle_get_state failed, status = %X", __FUNCTION__,
                                status);

                return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*increment based on the state*/
    if (lifecycle_state == FBE_LIFECYCLE_STATE_READY) {
        base_physical_drive_increment_up_time(base_physical_drive);
        base_physical_drive_get_up_time(base_physical_drive, &spin_time);
    }else if (lifecycle_state == FBE_LIFECYCLE_STATE_HIBERNATE) {
        base_physical_drive_increment_down_time(base_physical_drive);
        base_physical_drive_get_down_time(base_physical_drive, &spin_time);
    } else {
        spin_time = 1;
    }
    if (spin_time % send_notification_minutes == 0) {
        /* Notify DMO, which notifies Admin */
        dataChangeInfo.phys_location.bus = base_physical_drive->port_number;
        dataChangeInfo.phys_location.enclosure = base_physical_drive->enclosure_number;
        dataChangeInfo.phys_location.slot = (fbe_u8_t)base_physical_drive->slot_number;
        dataChangeInfo.data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;
        dataChangeInfo.device_mask = FBE_DEVICE_TYPE_DRIVE;
        fbe_base_physical_drive_send_data_change_notification(base_physical_drive, &dataChangeInfo);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
    
}

static fbe_lifecycle_status_t 
base_physical_drive_glitch_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                status;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_time_t glitch_time_sec;
    fbe_time_t glitch_end_time_sec;
    fbe_time_t current_time_sec = fbe_get_time() / 1000; //in seconds
    
    fbe_base_physical_drive_get_glitch_time(base_physical_drive, &glitch_time_sec);
    fbe_base_physical_drive_get_glitch_end_time(base_physical_drive, &glitch_end_time_sec);

    if(glitch_end_time_sec > current_time_sec)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_INFO,
                "%s end_time (%d) > current_time (%d) \n",__FUNCTION__, (int)glitch_end_time_sec, (int)current_time_sec);
    }
    else
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                    "%s Failed to re-login; can't clear current condition, status: 0x%X\n",
                    __FUNCTION__, status);
        }
        else{
            fbe_base_physical_drive_customizable_trace( base_physical_drive, FBE_TRACE_LEVEL_INFO,
                    "%s Logged in\n",__FUNCTION__);
        }       
        return FBE_LIFECYCLE_STATUS_DONE;
    }  
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!***************************************************************
 * base_physical_drive_trace_lifecycle_state_cond_function()
 ****************************************************************
 * @brief
 *  This functions traces the PDO state changes.   Note, that at
 *  the point this is called.  The rotaries for the base classes
 *  have already been run.
 *
 * @param  - base_object - Ptr to PDO base object
 * @param  - packet      - monitor packet.
 *
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  07/23/2012  Wayne Garrett - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t base_physical_drive_trace_lifecycle_state_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t *)base_object;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_u8_t *state_str;
    fbe_status_t status;

    fbe_lifecycle_get_state(&fbe_base_physical_drive_lifecycle_const, base_object, &lifecycle_state);

    state_str = fbe_base_object_trace_get_state_string(lifecycle_state);

    if (FBE_LIFECYCLE_STATE_ACTIVATE == lifecycle_state)
    {
        base_physical_drive->activate_count++;
    }

    fbe_base_physical_drive_customizable_trace( base_physical_drive, FBE_TRACE_LEVEL_INFO, 
                                                "state = %s, activate count = %d\n", state_str, base_physical_drive->activate_count);


    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                "%s can't clear current condition, status: 0x%X\n", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!***************************************************************
 * base_physical_drive_clear_power_save_attr_cond_function()
 ****************************************************************
 * @brief
 *  This functions clear the power save flag just in case since the drive may have been woken up in a non traditional way (like ddt under sep)
 *
 * @param  - base_object - Ptr to PDO base object
 * @param  - packet      - monitor packet.
 *
 * @return fbe_lifecycle_status_t 
 *
 ****************************************************************/
static fbe_lifecycle_status_t base_physical_drive_clear_power_save_attr_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t *    base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_u32_t                      powersave_attr = 0;
    fbe_status_t                   status;

    /*flip the attribute*/
    fbe_base_physical_drive_get_powersave_attr(base_physical_drive, &powersave_attr);
    powersave_attr &= ~FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON;
    fbe_base_physical_drive_set_powersave_attr(base_physical_drive, powersave_attr);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_physical_drive);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                "%s can't clear current condition, status: 0x%X\n", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!***************************************************************
 * base_physical_drive_resume_io_cond_function()
 ****************************************************************
 * @brief
 *  This functions will resume IO after all oustanding IO is drained
 *  from drive.   
 *
 * @param  - base_object - Ptr to PDO base object
 * @param  - packet      - monitor packet.
 *
 * @return fbe_lifecycle_status_t
 * 
 * @note If we are in this condition too long, then we will automatically
 *       resume.  This is a failsafe mechanism to prevent a possible
 *       hang or long delay resulting in a panic from uppler layers.
 *
 * @author
 *  03/08/2013  Wayne Garrett - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t base_physical_drive_resume_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t *    base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_u32_t outstanding_io_count = -1;
    fbe_time_t service_time_limit_ms;

    service_time_limit_ms = base_physical_drive->service_time_limit_ms;
    if (fbe_base_physical_drive_dieh_is_emeh_mode(base_physical_drive)) {
        service_time_limit_ms = fbe_dcs_get_emeh_svc_time_limit();
    }

    fbe_block_transport_server_get_outstanding_io_count(&base_physical_drive->block_transport_server, &outstanding_io_count);

    fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "resume_io_cond_function: outstanding_io_count=%d\n", outstanding_io_count);


    if (0 == outstanding_io_count || 
        fbe_get_elapsed_milliseconds(base_physical_drive->quiesce_start_time_ms) > service_time_limit_ms)
    {

        if (0 == outstanding_io_count) 
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_INFO,
                                                       "resume_io_cond_function: drain completed. resuming\n");
        }
        else
        {
            fbe_base_physical_drive_customizable_trace(base_physical_drive,  FBE_TRACE_LEVEL_ERROR,
                                                       "resume_io_cond_function: drain failed to complete in %llu ms. resuming\n",
                                                       service_time_limit_ms);
        }

        fbe_base_physical_drive_lock(base_physical_drive);
        fbe_base_physical_drive_clear_local_state(base_physical_drive, FBE_BASE_DRIVE_LOCAL_STATE_TIMEOUT_QUIESCE_HANDLING);                
        fbe_base_physical_drive_unlock(base_physical_drive);

        /*resume IO*/
        fbe_block_transport_server_resume(&base_physical_drive->block_transport_server); 
        fbe_block_transport_server_process_io_from_queue(&base_physical_drive->block_transport_server);     
        
        (void)fbe_lifecycle_clear_current_cond(base_object);    

        return FBE_LIFECYCLE_STATUS_DONE;
    }   

    fbe_lifecycle_reschedule(&fbe_base_physical_drive_lifecycle_const, 
                             (fbe_base_object_t*)base_physical_drive,  
                             100); /*msec*/    

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * base_physical_drive_set_fault_bit_in_fail_state()
 ****************************************************************
 * @brief
 *  This function will identify drive and link fault values based on
 *  death reason and set edge accordingly. It will do it in the Pending Fail lifecycyle state.
 *
 * @param  - base_object - Ptr to PDO base object
 *
 * @return 
 *
 * @author
 *  05/24/2013 : CLC - Created
 *
 ****************************************************************/
void base_physical_drive_set_fault_bit_in_fail_state(fbe_base_object_t * base_object, fbe_bool_t link_fault)
{
    fbe_base_physical_drive_t *    base_physical_drive = (fbe_base_physical_drive_t*)base_object;
    fbe_base_physical_drive_fault_bit_translation_t xlate_fault_bit = {0};
    fbe_object_death_reason_t   death_reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID;

    /* Get the death reason. */
    fbe_base_discovered_get_death_reason ((fbe_base_discovered_t *)base_object, &death_reason);
    xlate_fault_bit.death_reason = death_reason;
    
    if ((death_reason == FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED) && (link_fault == FBE_TRUE))
    {
        /* No need to change the fault bit. */
        return;
    }
    
    /* get fault bit info from death reason and update it on edge. */
    fbe_base_physical_drive_xlate_death_reason_to_fault_bit(&xlate_fault_bit);

    if (xlate_fault_bit.drive_fault == true)
    {
        /* notify upper layer. */
        fbe_base_physical_drive_set_path_attr(base_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT);
    }
    else if(xlate_fault_bit.link_fault == true)
    {
       /* notify upper layer. */
        fbe_base_physical_drive_set_path_attr(base_physical_drive, FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT);
    }
    else
    {
        /* No need to set any path attributes*/
    }

    return;
}
