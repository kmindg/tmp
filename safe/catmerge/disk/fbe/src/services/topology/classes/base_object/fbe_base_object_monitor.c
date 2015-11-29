#include "base_object_private.h"

static fbe_lifecycle_status_t base_object_drain_userper_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

fbe_status_t fbe_base_object_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify((fbe_lifecycle_const_t*)&FBE_LIFECYCLE_CONST_DATA(base_object));
}

/*--- lifecycle state transitions ------------------------------------------------------*/

fbe_lifecycle_state_t specialize_transition_states[] = {
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t activate_transition_states[] = {
	FBE_LIFECYCLE_STATE_READY,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t ready_transition_states[] = {
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t hibernate_transition_states[] = {
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t offline_transition_states[] = {
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t fail_transition_states[] = {
    FBE_LIFECYCLE_STATE_SPECIALIZE,
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t ready_pending_transition_states[] = {
	FBE_LIFECYCLE_STATE_READY,
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t activate_pending_transition_states[] = {
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_READY,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t hibernate_pending_transition_states[] = {
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t offline_pending_transition_states[] = {
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t fail_pending_transition_states[] = {
	FBE_LIFECYCLE_STATE_FAIL,
	FBE_LIFECYCLE_STATE_ACTIVATE,
	FBE_LIFECYCLE_STATE_HIBERNATE,
	FBE_LIFECYCLE_STATE_OFFLINE,
	FBE_LIFECYCLE_STATE_DESTROY
};

fbe_lifecycle_state_t destroy_pending_transition_states[] = {
	FBE_LIFECYCLE_STATE_DESTROY
};

/*--- lifecycle state definitions ------------------------------------------------------*/

static fbe_lifecycle_const_base_state_t base_object_states[] = {
	{
		"SPECIALIZE",                                      /* state name */
		FBE_LIFECYCLE_STATE_TYPE_TRANSITIONAL,             /* state type */
		FBE_LIFECYCLE_STATE_SPECIALIZE,                    /* this state */
		FBE_LIFECYCLE_STATE_ACTIVATE,                      /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(specialize_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		specialize_transition_states                       /* transition states */
	},{
		"ACTIVATE",                                        /* state name */
		FBE_LIFECYCLE_STATE_TYPE_TRANSITIONAL,             /* state type */
		FBE_LIFECYCLE_STATE_ACTIVATE,                      /* this state */
		FBE_LIFECYCLE_STATE_READY,                         /* next state */
		FBE_LIFECYCLE_STATE_PENDING_ACTIVATE,              /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(activate_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		activate_transition_states                         /* transition states */
	},{
		"READY",                                           /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PERSISTENT,               /* state type */
		FBE_LIFECYCLE_STATE_READY,                         /* this state */
		FBE_LIFECYCLE_STATE_READY,                         /* next state */
		FBE_LIFECYCLE_STATE_PENDING_READY,                 /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(ready_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		ready_transition_states                            /* transition states */
	},{
		"HIBERNATE",                                       /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PERSISTENT,               /* state type */
		FBE_LIFECYCLE_STATE_HIBERNATE,                     /* this state */
		FBE_LIFECYCLE_STATE_HIBERNATE,                     /* next state */
		FBE_LIFECYCLE_STATE_PENDING_HIBERNATE,             /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(hibernate_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		hibernate_transition_states                        /* transition states */
	},{
		"OFFLINE",                                         /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PERSISTENT,               /* state type */
		FBE_LIFECYCLE_STATE_OFFLINE,                       /* this state */
		FBE_LIFECYCLE_STATE_OFFLINE,                       /* next state */
		FBE_LIFECYCLE_STATE_PENDING_OFFLINE,               /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(offline_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		offline_transition_states                          /* transition states */
	},{
		"FAIL",                                            /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PERSISTENT,               /* state type */
		FBE_LIFECYCLE_STATE_FAIL,                          /* this state */
		FBE_LIFECYCLE_STATE_FAIL,                          /* next state */
		FBE_LIFECYCLE_STATE_PENDING_FAIL,                  /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(fail_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		fail_transition_states                             /* transition states */
	},{
		"DESTROY",                                         /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PERSISTENT,               /* state type */
		FBE_LIFECYCLE_STATE_DESTROY,                       /* this state */
		FBE_LIFECYCLE_STATE_DESTROY,                       /* next state */
		FBE_LIFECYCLE_STATE_PENDING_DESTROY,               /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		0,                                                 /* max transition states */
		NULL                                               /* transition states */
	},{
		"PENDING-READY",                                   /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PENDING,                  /* state type */
		FBE_LIFECYCLE_STATE_PENDING_READY,                 /* this state */
		FBE_LIFECYCLE_STATE_READY,                         /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(ready_pending_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		ready_pending_transition_states						/* transition states */
	},{
		"PENDING-ACTIVATE",                                /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PENDING,                  /* state type */
		FBE_LIFECYCLE_STATE_PENDING_ACTIVATE,              /* this state */
		FBE_LIFECYCLE_STATE_ACTIVATE,                      /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(activate_pending_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		activate_pending_transition_states                 /* transition states */
	},{
		"PENDING-HIBERNATE",                               /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PENDING,                  /* state type */
		FBE_LIFECYCLE_STATE_PENDING_HIBERNATE,             /* this state */
		FBE_LIFECYCLE_STATE_HIBERNATE,                     /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(hibernate_pending_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		hibernate_pending_transition_states                /* transition states */
	},{
		"PENDING-OFFLINE",                                 /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PENDING,                  /* state type */
		FBE_LIFECYCLE_STATE_PENDING_OFFLINE,               /* this state */
		FBE_LIFECYCLE_STATE_OFFLINE,                       /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(offline_pending_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		offline_pending_transition_states                  /* transition states */
	},{
		"PENDING-FAIL",                                    /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PENDING,                  /* state type */
		FBE_LIFECYCLE_STATE_PENDING_FAIL,                  /* this state */
		FBE_LIFECYCLE_STATE_FAIL,                          /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(fail_pending_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		fail_pending_transition_states                     /* transition states */
	},{
		"PENDING-DESTROY",                                 /* state name */
		FBE_LIFECYCLE_STATE_TYPE_PENDING,                  /* state type */
		FBE_LIFECYCLE_STATE_PENDING_DESTROY,               /* this state */
		FBE_LIFECYCLE_STATE_DESTROY,                       /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		(sizeof(destroy_pending_transition_states)/sizeof(fbe_lifecycle_state_t)), /* max transition states */
		destroy_pending_transition_states                  /* transition states */
	},{
		"NOT-EXIST",                                       /* state name */
		FBE_LIFECYCLE_STATE_TYPE_NOT_EXIST,                /* state type */
		FBE_LIFECYCLE_STATE_NOT_EXIST,                     /* this state */
		FBE_LIFECYCLE_STATE_NOT_EXIST,                     /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		0,                                                 /* max transition states */
		NULL                                               /* transition states */
	},{
		"INVALID",                                         /* state name */
		FBE_LIFECYCLE_STATE_TYPE_INVALID,                  /* state type */
		FBE_LIFECYCLE_STATE_INVALID,                       /* this state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* next state */
		FBE_LIFECYCLE_STATE_INVALID,                       /* pending state */
		FBE_LIFECYCLE_TIMER_RESCHED_NORMAL,                /* monitor reschedule msecs */
		0,                                                 /* max transition states */
		NULL                                               /* transition states */
	}
};

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_object);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_object);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_object) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		base_object,
		FBE_LIFECYCLE_NULL_FUNC,         /* no online function */
		FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant base condition entries --------------------------------------------------*/

static fbe_lifecycle_const_base_cond_t cond_specialize = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND_ATTR(
		"cond_specialize",
		FBE_BASE_OBJECT_LIFECYCLE_COND_SPECIALIZE,
		FBE_LIFECYCLE_NULL_FUNC,
		FBE_LIFECYCLE_CONST_COND_ATTR_NOSET),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,       /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,            /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,        /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,          /* OFFLINE */
		FBE_LIFECYCLE_STATE_SPECIALIZE,       /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t cond_activate = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND_ATTR(
		"cond_activate",
		FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE,
		FBE_LIFECYCLE_NULL_FUNC,
		FBE_LIFECYCLE_CONST_COND_ATTR_NOSET),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
		FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
		FBE_LIFECYCLE_STATE_ACTIVATE,            /* HIBERNATE */
		FBE_LIFECYCLE_STATE_ACTIVATE,            /* OFFLINE */
		FBE_LIFECYCLE_STATE_ACTIVATE,            /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t cond_hibernate = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND_ATTR(
		"cond_hibernate",
		FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE,
		FBE_LIFECYCLE_NULL_FUNC,
		FBE_LIFECYCLE_CONST_COND_ATTR_NOSET),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_HIBERNATE,           /* ACTIVATE */
		FBE_LIFECYCLE_STATE_HIBERNATE,           /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t cond_offline = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND_ATTR(
		"cond_offline",
		FBE_BASE_OBJECT_LIFECYCLE_COND_OFFLINE,
		FBE_LIFECYCLE_NULL_FUNC,
		FBE_LIFECYCLE_CONST_COND_ATTR_NOSET),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_OFFLINE,             /* ACTIVATE */
		FBE_LIFECYCLE_STATE_OFFLINE,             /* READY */
		FBE_LIFECYCLE_STATE_OFFLINE,             /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t cond_fail = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND_ATTR(
		"cond_fail",
		FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL,
		FBE_LIFECYCLE_NULL_FUNC,
		FBE_LIFECYCLE_CONST_COND_ATTR_NOSET),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_FAIL,                /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_FAIL,                /* ACTIVATE */
		FBE_LIFECYCLE_STATE_FAIL,                /* READY */
		FBE_LIFECYCLE_STATE_FAIL,                /* HIBERNATE */
		FBE_LIFECYCLE_STATE_FAIL,                /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t cond_destroy = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND_ATTR(
		"cond_destroy",
		FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY,
		FBE_LIFECYCLE_NULL_FUNC,
		FBE_LIFECYCLE_CONST_COND_ATTR_NOSET),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_DESTROY,             /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_DESTROY,             /* ACTIVATE */
		FBE_LIFECYCLE_STATE_DESTROY,             /* READY */
		FBE_LIFECYCLE_STATE_DESTROY,             /* HIBERNATE */
		FBE_LIFECYCLE_STATE_DESTROY,             /* OFFLINE */
		FBE_LIFECYCLE_STATE_DESTROY,             /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* Every time when cladd_id changed the derived class should initialise it internal structure.
	The base_object_set_class_id function will trigger this conditon automatically.
*/
static fbe_lifecycle_const_base_cond_t base_object_self_init_cond = { /* It would be nice if ALL conditions will have consistant naming */
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_object_self_init_cond",
		FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
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

/* This condition is set when object has canceled packet on the queue */
static fbe_lifecycle_const_base_cond_t base_object_packet_canceled_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_object_packet_canceled_cond",
		FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED,
		FBE_LIFECYCLE_NULL_FUNC),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,			 /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,				 /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* This condition is preset when object switches to destroy state */
static fbe_lifecycle_const_base_cond_t base_object_drain_userper_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_object_drain_userper_cond",
		FBE_BASE_OBJECT_LIFECYCLE_COND_DRAIN_USERPER,
		base_object_drain_userper_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,			 /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,				 /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_object)[] = {
    &cond_specialize,
	&cond_activate,
	&cond_hibernate,
	&cond_offline,
	&cond_fail,
	&cond_destroy,

	&base_object_self_init_cond,
	&base_object_packet_canceled_cond,
	&base_object_drain_userper_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_object);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t specialize_rotary[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_object_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t ready_rotary[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_object_packet_canceled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t destroy_rotary[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_object_drain_userper_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_object)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, specialize_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, ready_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_object);

/*--- global base object lifecycle constant data ---------------------------------------*/

fbe_lifecycle_const_base_t FBE_LIFECYCLE_CONST_DATA(base_object) = {
	{
		FBE_LIFECYCLE_CANARY_CONST, \
		FBE_CLASS_ID_BASE_OBJECT,
		NULL, \
		&(FBE_LIFECYCLE_ROTARIES(base_object)), \
		&(FBE_LIFECYCLE_BASE_CONDITIONS(base_object)), \
		&(FBE_LIFECYCLE_CALLBACKS(base_object))
	},
	FBE_LIFECYCLE_CANARY_CONST_BASE,
	(sizeof(base_object_states)/sizeof(fbe_lifecycle_const_base_state_t)),
	base_object_states
};

static fbe_lifecycle_status_t 
base_object_drain_userper_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;

    fbe_base_object_trace(object_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);


	if(object_p->userper_counter == 0){
		status = fbe_lifecycle_clear_current_cond(object_p);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace(object_p, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't clear current condition, status: 0x%X\n",
									__FUNCTION__, status);
			return FBE_LIFECYCLE_STATUS_DONE;
		}
	}
    return FBE_LIFECYCLE_STATUS_DONE;
}

