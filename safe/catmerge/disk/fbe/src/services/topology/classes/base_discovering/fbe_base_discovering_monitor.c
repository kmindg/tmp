#include "base_discovering_private.h"

fbe_status_t fbe_base_discovering_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_discovering));
}

/*--- local function prototypes --------------------------------------------------------*/

/* Pending function */
static fbe_lifecycle_status_t base_discovering_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet);

/* Condition functions forward declaration */
/*
static fbe_lifecycle_status_t base_discovering_discovery_gone_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
*/
/*
static fbe_lifecycle_status_t base_discovering_discovery_server_not_empty_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
*/

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_discovering);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_discovering);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_discovering) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        base_discovering,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        base_discovering_pending_func)         /* no pending function */
};

/*--- constant base condition entries -------------------------------------------------------*/

/* FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL condition */
static fbe_lifecycle_const_base_timer_cond_t base_discovering_discovery_poll_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_discovering_discovery_poll_cond",
            FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
            FBE_LIFECYCLE_NULL_FUNC),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,           /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    300 /* fires every 3 seconds */
};

/* The object should take care of instantiation / removal of the client discovery edges */
/* FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE condition */
static fbe_lifecycle_const_base_cond_t base_discovering_discovery_update_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovering_discovery_update_cond",
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL condition */
static fbe_lifecycle_const_base_timer_cond_t base_discovering_discovery_poll_per_day_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_discovering_discovery_poll_per_day_cond",
            FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL_PER_DAY,
            FBE_LIFECYCLE_NULL_FUNC),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,       /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,        /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    8640000 /*fires every 24 hours */
};
#if 0
/* This condition preset in destroy rotary and should set path state of all client discovery edges to GONE */
/* FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_GONE condition */
static fbe_lifecycle_const_base_cond_t base_discovering_discovery_gone_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovering_discovery_gone_cond",
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_GONE,
        base_discovering_discovery_gone_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};
#endif

#if 0
/* This condition preset in destroy rotary and should ensure that all client discovery edges are detached */
/* FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_SERVER_NOT_EMPTY condition */
static fbe_lifecycle_const_base_cond_t base_discovering_discovery_server_not_empty_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_discovering_discovery_server_not_empty_cond",
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_SERVER_NOT_EMPTY,
        base_discovering_discovery_server_not_empty_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};
#endif

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_discovering)[] = {
    &base_discovering_discovery_update_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_discovering_discovery_poll_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_discovering_discovery_poll_per_day_cond,
    /* Destroy conditions */
    /* &base_discovering_discovery_gone_cond, */
    /* &base_discovering_discovery_server_not_empty_cond, */
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_discovering);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t base_discovering_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovering_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovering_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovering_discovery_poll_per_day_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

#if 0
static fbe_lifecycle_const_rotary_cond_t base_discovering_destroy_rotary[] = {
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovering_discovery_gone_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), */
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_discovering_discovery_server_not_empty_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), */
};
#endif

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_discovering)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, base_discovering_ready_rotary),
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, base_discovering_destroy_rotary), */
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_discovering);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_discovering) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        base_discovering,
        FBE_CLASS_ID_BASE_DISCOVERING,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_discovered))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/
#if 0 
static fbe_lifecycle_status_t 
base_discovering_discovery_gone_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_discovering_t * base_discovering = NULL;

    base_discovering = (fbe_base_discovering_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovering,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    fbe_discovery_transport_server_set_path_state(&base_discovering->discovery_transport_server, FBE_PATH_STATE_GONE);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_discovering);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_discovering, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
#endif

#if 0
static fbe_lifecycle_status_t 
base_discovering_discovery_server_not_empty_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_discovering_t * base_discovering = NULL;
    fbe_u32_t number_of_clients;

    base_discovering = (fbe_base_discovering_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovering,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    fbe_base_discovering_get_number_of_clients(base_discovering, &number_of_clients);

    if(number_of_clients == 0){
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_discovering);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_discovering, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
#endif

static fbe_lifecycle_status_t 
base_discovering_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_base_discovering_t * base_discovering = NULL;

    base_discovering = (fbe_base_discovering_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_discovering,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    lifecycle_status = fbe_discovery_transport_server_pending(&base_discovering->discovery_transport_server,
                                                                &fbe_base_discovering_lifecycle_const,
                                                                (fbe_base_object_t *) base_discovering);

    return lifecycle_status;
}
