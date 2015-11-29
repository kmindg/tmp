#include "base_discovered_private.h"

fbe_status_t fbe_base_discovered_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_discovered));
}

/*--- local function prototypes --------------------------------------------------------*/

/* Online monitor forward declaration */

/* Condition functions forward declaration */
static fbe_lifecycle_status_t base_discovered_discovery_edge_invalid_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_discovered_discovery_edge_not_enable_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t base_discovered_get_port_object_id_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_discovered_get_port_object_id_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_discovered_discovery_edge_not_ready_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_discovered_discovery_edge_not_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_discovered_protocol_edges_not_ready_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_discovered_protocol_edges_not_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_discovered_detach_discovery_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_discovered_detach_discovery_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_discovered_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_discovered_activate_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_discovered);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_discovered);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_discovered) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        base_discovered,
        FBE_LIFECYCLE_NULL_FUNC,        /* no online function */
        base_discovered_pending_func)        
};

/*--- constant condition entries -------------------------------------------------------*/

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_SAVE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_power_save_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_SAVE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD condition */
static fbe_lifecycle_const_base_cond_t base_discovered_fw_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_fw_download_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD,
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

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD_PEER condition */
static fbe_lifecycle_const_base_cond_t base_discovered_fw_download_peer_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_fw_download_peer_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_FW_DOWNLOAD_PEER,
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

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_ABORT_FW_DOWNLOAD condition */
static fbe_lifecycle_const_base_cond_t base_discovered_abort_fw_download_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_abort_fw_download_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ABORT_FW_DOWNLOAD,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_INVALID condition */
static fbe_lifecycle_const_base_cond_t base_discovered_discovery_edge_invalid_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_discovery_edge_invalid_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_INVALID,
        base_discovered_discovery_edge_invalid_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_READY condition */
static fbe_lifecycle_const_base_cond_t base_discovered_discovery_edge_not_ready_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_discovery_edge_not_ready_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_READY,
        base_discovered_discovery_edge_not_ready_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */ /* I think it is important to invalidate the object */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY condition */
static fbe_lifecycle_const_base_cond_t base_discovered_protocol_edges_not_ready_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_protocol_edges_not_ready_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PROTOCOL_EDGES_NOT_READY,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */ 
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_INVALID_TYPE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_invalid_type_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_invalid_type_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_INVALID_TYPE,
        FBE_LIFECYCLE_NULL_FUNC),               /* This is virtual condition */
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_CHECK_QUEUED_IO_ACTIVATE_TIMER condition */
static fbe_lifecycle_const_base_timer_cond_t base_discovered_activate_check_queued_io_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_discovered_activate_check_queued_io_timer_cond",
            FBE_BASE_DISCOVERED_LIFECYCLE_COND_CHECK_QUEUED_IO_ACTIVATE_TIMER,
            FBE_LIFECYCLE_NULL_FUNC),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    1600 /* fires after 16 seconds*/
};


/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_ATTACH_PROTOCOL_EDGE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_attach_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_attach_protocol_edge_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ATTACH_PROTOCOL_EDGE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_OPEN_PROTOCOL_EDGE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_open_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_open_protocol_edge_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_OPEN_PROTOCOL_EDGE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_DETACH_PROTOCOL_EDGE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_detach_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_detach_protocol_edge_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_DETACH_PROTOCOL_EDGE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_DETACH_DISCOVERY_EDGE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_detach_discovery_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_detach_discovery_edge_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_DETACH_DISCOVERY_EDGE,
        base_discovered_detach_discovery_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_DESTROY,            /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_DESTROY,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_DESTROY,            /* READY */
        FBE_LIFECYCLE_STATE_DESTROY,            /* HIBERNATE */
        FBE_LIFECYCLE_STATE_DESTROY,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_DESTROY,            /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID condition */
static fbe_lifecycle_const_base_cond_t base_discovered_get_port_object_id_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_get_port_object_id_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID,
        base_discovered_get_port_object_id_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_timer_cond_t base_discovered_activate_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_physical_drive_activate_timer_cond",
            FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVATE_TIMER,
            base_discovered_activate_timer_cond_function),
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
    17000 /* fires after 170 seconds (around 3 min.) */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_CYCLE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_power_cycle_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_power_cycle_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_POWER_CYCLE,
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


/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVE_POWER_SAVE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_active_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_active_power_save_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVE_POWER_SAVE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,			/* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,			/* ACTIVATE */
		FBE_LIFECYCLE_STATE_HIBERNATE,			/* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,			/* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,			/* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,				/* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)			/* DESTROY */
};

/* FBE_BASE_DISCOVERED_LIFECYCLE_COND_PASSIVE_POWER_SAVE condition */
static fbe_lifecycle_const_base_cond_t base_discovered_passive_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_passive_power_save_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_PASSIVE_POWER_SAVE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,			/* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,			/* ACTIVATE */
		FBE_LIFECYCLE_STATE_HIBERNATE,			/* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,			/* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,			/* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,				/* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)			/* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_discovered_exit_power_save_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovered_exit_power_save_cond",
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_EXIT_POWER_SAVE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};



static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_discovered)[] = {
    &base_discovered_discovery_edge_invalid_cond,
    &base_discovered_discovery_edge_not_ready_cond,
    &base_discovered_protocol_edges_not_ready_cond,
    &base_discovered_get_port_object_id_cond,

    (fbe_lifecycle_const_base_cond_t *)&base_discovered_activate_check_queued_io_timer_cond,

    &base_discovered_attach_protocol_edge_cond,
    &base_discovered_open_protocol_edge_cond,
    &base_discovered_detach_protocol_edge_cond,

    &base_discovered_detach_discovery_edge_cond,
    &base_discovered_invalid_type_cond,

    &base_discovered_fw_download_cond,
    &base_discovered_fw_download_peer_cond,
    &base_discovered_abort_fw_download_cond,
    &base_discovered_power_save_cond,
    (fbe_lifecycle_const_base_cond_t *)&base_discovered_activate_timer_cond,

    &base_discovered_power_cycle_cond,
    &base_discovered_active_power_save_cond,
    &base_discovered_passive_power_save_cond,
    &base_discovered_exit_power_save_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_discovered);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t base_discovered_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_discovery_edge_invalid_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_discovery_edge_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_get_port_object_id_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_invalid_type_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_protocol_edges_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_attach_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_open_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
};

static fbe_lifecycle_const_rotary_cond_t base_discovered_activate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_power_cycle_cond,       FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_abort_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* must be before fw_download cond*/
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_fw_download_cond,       FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* must be before inquiry */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_fw_download_peer_cond,  FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* must be before inquiry */

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_activate_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_activate_check_queued_io_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_discovery_edge_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_protocol_edges_not_ready_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t base_discovered_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_abort_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  /*no dependencies*/
};

static fbe_lifecycle_const_rotary_cond_t base_discovered_hibernate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_abort_fw_download_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  /*no dependencies*/	
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_active_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_passive_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_exit_power_save_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
};


static fbe_lifecycle_const_rotary_cond_t base_discovered_destroy_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_detach_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovered_detach_discovery_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),      
};


static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_discovered)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, base_discovered_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, base_discovered_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, base_discovered_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, base_discovered_hibernate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, base_discovered_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_discovered);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_discovered) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        base_discovered,
        FBE_CLASS_ID_BASE_DISCOVERED,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_object))    /* The super class */
};


/*--- Condition Functions --------------------------------------------------------------*/
static fbe_lifecycle_status_t 
base_discovered_discovery_edge_invalid_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_path_state_t path_state;
    fbe_base_discovered_t * base_discovered = (fbe_base_discovered_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* If discovery_edge state is invalid we should wait for the next monitor */
    status = fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, &path_state);

    if(path_state == FBE_PATH_STATE_INVALID){
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_discovered);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn base_discovered_discovery_edge_not_ready_cond_function(fbe_base_object_t * base_object, 
*                                                  fbe_packet_t * packet)          
***************************************************************************
*
* @brief
*       This function runs when it comes to the discovery edge not ready condition.
*       (1) If the path status is not enabled, wait for the next monitor .  
*       (2) If the path state is enabled, calls the executer functon to handle 
*           these discovery edge attributes need to be handled before 
*           the discovery edge becomes ready. 
*       (3) If the path state is ENABLED and there is no attributes need to be 
*           handled before the discovery edge becomes ready, the condition gets cleared. 
*
* @param      base_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*   12-Jan-2009 PHE - Sent the command to handle the discovery edge attributes.
***************************************************************************/
static fbe_lifecycle_status_t 
base_discovered_discovery_edge_not_ready_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_path_state_t path_state;
    fbe_base_discovered_t * base_discovered = (fbe_base_discovered_t *)base_object;
    fbe_path_attr_t path_attr;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* If discovery_edge path_state is not enable we should wait for the next monitor */
    status = fbe_discovery_transport_get_path_state(&base_discovered->discovery_edge, &path_state);

    if(path_state != FBE_PATH_STATE_ENABLED){
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_base_discovered_get_path_attributes(base_discovered, &path_attr);

    if((status == FBE_STATUS_OK) && 
       (path_attr & (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_UNRECOV | 
                     FBE_DISCOVERY_PATH_ATTR_BYPASSED_UNRECOV)))
    {
        fbe_base_discovered_set_death_reason(base_discovered, FBE_BASE_DISCOVERED_DEATH_REASON_HARDWARE_NOT_READY);

        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, Fail the object, death reason HARDWARE_NOT_READY.\n",
                                    __FUNCTION__);

        /* We need to set BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_base_discovered_lifecycle_const, 
                                        (fbe_base_object_t*)base_discovered, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set BO:FAIL condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else if((status == FBE_STATUS_OK) && 
       (path_attr & (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST | 
                     FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST)))
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else if((status == FBE_STATUS_OK) && 
            (path_attr & (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST | 
                          FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST | 
                          FBE_DISCOVERY_PATH_ATTR_POWERSAVE_ON)))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s, making discovery edge ready.\n",
                                    __FUNCTION__);
        
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, base_discovered_discovery_edge_not_ready_cond_completion, base_discovered);
        
        /* Call the executer function to do actual job */
        fbe_base_discovered_send_handle_edge_attr_command(base_discovered, packet); 

        /* Return pending to the lifecycle */
        return FBE_LIFECYCLE_STATUS_PENDING;    
    }
    else 
    {
        /* The path state is ENABLED and there are no attributes set that need to be handled before the discovery edge becomes ready.
         So, clear the condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_discovered);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X.\n",
                                    __FUNCTION__, status);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }   
}

/*!*************************************************************************
* @fn base_discovered_discovery_edge_not_ready_cond_completion(fbe_packet_t * packet, 
*                                               fbe_packet_completion_context_t context)
***************************************************************************
* @brief
*       This routine is the completion function for the discovery edge not read condition.
*       It checks whether the attributes which caused the discovery edge to be not ready
*       has been cleared or not. If these attributes have been cleared, 
*       clear the current condition.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The completion context.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   12-Jan-2009 PHE - Created.
***************************************************************************/
static fbe_status_t 
base_discovered_discovery_edge_not_ready_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_discovered_t * base_discovered = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * payload_control_operation = NULL;
    fbe_payload_control_status_t  control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_path_attr_t path_attr;
    
    base_discovered = (fbe_base_discovered_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%x.\n",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }
    
    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, NULL payload.\n", __FUNCTION__);
        return FBE_STATUS_OK; /* this is completion OK */
    }

    payload_control_operation = fbe_payload_ex_get_control_operation(payload);  
    if(payload_control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, NULL payload_control_operation.\n", __FUNCTION__);
        return FBE_STATUS_OK; /* this is completion OK */
    }  

    fbe_payload_control_get_status(payload_control_operation, &control_status);

    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s bad control status: 0x%x.\n",
                                __FUNCTION__, control_status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }   

    status = fbe_base_discovered_get_path_attributes(base_discovered, &path_attr);    

    /* Check whether the discovery edge becomes ready or not. */
    if((status == FBE_STATUS_OK) && 
        ((path_attr & (FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_NONPERSIST |
                       FBE_DISCOVERY_PATH_ATTR_BYPASSED_NONPERSIST |
                       FBE_DISCOVERY_PATH_ATTR_POWERED_OFF_PERSIST |
                       FBE_DISCOVERY_PATH_ATTR_BYPASSED_PERSIST)) == 0)) {
        /* None of attributes are set, the edge becomes ready.
         * Clear the current condition. 
         * The monitor thread will be rescheduled in function base_discovered_check_attributes 
         * if any attribute changes.
         */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_discovered);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%x.\n",
                                    __FUNCTION__, status);
        }
    }
   
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
base_discovered_get_port_object_id_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_discovered_t * base_discovered = (fbe_base_discovered_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, base_discovered_get_port_object_id_cond_completion, base_discovered);
    
    /* Call the executer function to do actual job */
    fbe_base_discovered_send_get_port_object_id_command(base_discovered, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
base_discovered_get_port_object_id_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_discovered_t * base_discovered = NULL;
    
    base_discovered = (fbe_base_discovered_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_discovered);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
base_discovered_detach_discovery_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_discovered_t * base_discovered = (fbe_base_discovered_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, base_discovered_detach_discovery_edge_cond_completion, base_discovered);

    fbe_base_discovered_detach_edge(base_discovered, packet);
    
    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}


static fbe_status_t 
base_discovered_detach_discovery_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_discovered_t * base_discovered = NULL;
    
    base_discovered = (fbe_base_discovered_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid packet status: 0x%X",
                                __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_discovered);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}


static fbe_lifecycle_status_t 
base_discovered_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_base_discovered_t * base_discovered = NULL;
    fbe_status_t status;
    fbe_lifecycle_state_t lifecycle_state;

    base_discovered = (fbe_base_discovered_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);


    status = fbe_lifecycle_get_state(&fbe_base_discovered_lifecycle_const, base_object, &lifecycle_state);
    if(status != FBE_STATUS_OK){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
        /* We have no error code to return */
    }

    lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;

    if(lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_ACTIVATE){
        status = fbe_lifecycle_force_clear_cond(&fbe_base_discovered_lifecycle_const, 
                                                base_object, 
                                                FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVATE_TIMER);

        if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear condition FBE_BASE_DISCOVERED_LIFECYCLE_COND_ACTIVATE_TIMER, status: 0x%X",
                                __FUNCTION__, status);
        }
        
        status = fbe_lifecycle_force_clear_cond(&fbe_base_discovered_lifecycle_const, 
                                                base_object, 
                                                FBE_BASE_DISCOVERED_LIFECYCLE_COND_CHECK_QUEUED_IO_ACTIVATE_TIMER);
        if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear condition FBE_BASE_DISCOVERED_LIFECYCLE_COND_CHECK_QUEUED_IO_ACTIVATE_TIMER, status: 0x%X",
                                __FUNCTION__, status);
        }
    }

    return lifecycle_status;
}

static fbe_lifecycle_status_t 
base_discovered_activate_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_discovered_t * base_discovered = (fbe_base_discovered_t*)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovered,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We need to set BO:FAIL lifecycle condition */
    status = fbe_lifecycle_set_cond(&fbe_base_discovered_lifecycle_const, 
                                    (fbe_base_object_t*)base_discovered, 
                                    FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
    fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_object, FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovered, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't set BO:FAIL condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
